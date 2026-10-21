"""실제 TestClient의 콘솔 입력, IOCP 수신, 설정 검증, 정상 종료를 검사한다.

python TestClient/Tests/client_lifecycle_test.py path/to/TestClient.exe path/to/GameServer.exe
임시 설정과 별도 loopback 서버만 사용하며 기존 실행 중인 서버에는 접속하지 않는다.
"""
import contextlib
import pathlib
import re
import socket
import struct
import subprocess
import sys
import tempfile
import time

sys.dont_write_bytecode = True
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / "Server/Tests"))
from session_lifecycle_test import Client, field, fields, packet, scalar


def config(port, **overrides):
    values = dict(ip="127.0.0.1", port=port, session_count=16, worker_thread_count=4,
                  bot_thread_count=2, chat_interval_ms=20, bot_room_count=4,
                  all_chat_interval=10, whisper_interval=15)
    values.update(overrides)
    return ("[server]\nip={ip}\nport={port}\n[client]\nsession_count={session_count}\n"
            "worker_thread_count={worker_thread_count}\nbot_thread_count={bot_thread_count}\n"
            "chat_interval_ms={chat_interval_ms}\nbot_room_count={bot_room_count}\n"
            "all_chat_interval={all_chat_interval}\nwhisper_interval={whisper_interval}\n").format(**values)


class RunningClient:
    def __init__(self, executable, directory, settings):
        self.directory = directory
        (directory / "Config").mkdir(parents=True)
        # 실제 사용자가 저장할 수 있는 UTF-8 BOM 설정도 계속 읽을 수 있어야 한다.
        (directory / "Config/config.ini").write_text(settings, encoding="utf-8-sig")
        self.log = directory / "client.log"
        self.output = self.log.open("wb")
        self.process = subprocess.Popen([str(executable)], cwd=directory, stdin=subprocess.PIPE,
                                        stdout=self.output, stderr=subprocess.STDOUT,
                                        creationflags=subprocess.CREATE_NO_WINDOW)

    def write(self, text):
        self.process.stdin.write(text.encode("ascii"))
        self.process.stdin.flush()

    def finish(self, expected=0):
        try:
            code = self.process.wait(timeout=6)
        except subprocess.TimeoutExpired:
            raise AssertionError("client did not finish within 6 seconds")
        assert code == expected, (code, self.log.read_bytes().decode("cp949", errors="replace"))

    def close(self):
        if self.process.poll() is None:
            self.process.terminate()
        self.process.wait(timeout=5)
        if not self.process.stdin.closed:
            self.process.stdin.close()
        self.output.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


def exact(sock, count):
    data = b""
    while len(data) < count:
        chunk = sock.recv(count - len(data))
        assert chunk, "client disconnected before expected packet"
        data += chunk
    return data


def receive(sock, kind):
    size, actual = struct.unpack("<HH", exact(sock, 4))
    body = fields(exact(sock, size - 4))
    assert actual == kind, (kind, actual, body)
    return body


def ping_barrier(sock):
    sock.sendall(packet(1016, field(1, 123456)))
    assert scalar(receive(sock, 1015), 1) == 123456


@contextlib.contextmanager
def listener():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        sock.listen()
        sock.settimeout(5)
        yield sock, sock.getsockname()[1]


def manual_scenario(executable, root):
    with listener() as (server, port):
        with RunningClient(executable, root / "manual", config(port)) as client:
            client.write("1\n1\nuser\nwrong\n")
            with server.accept()[0] as peer:
                peer.settimeout(3)
                assert scalar(receive(peer, 1000), 3) == b"wrong"
                peer.sendall(packet(1001, field(3, 3)))
                # 재입력은 아직 주지 않는다. IOCP 스레드가 입력에 막히면 핑 응답이 안 온다.
                ping_barrier(peer)
                client.write("1\nuser\ncorrect\n")
                assert scalar(receive(peer, 1000), 3) == b"correct"
                peer.sendall(packet(1001, field(1, 42) + field(3, 8)))
                ping_barrier(peer)

                client.write("/enter 3\n/enter 3\n")
                assert scalar(receive(peer, 1008), 1) == 3
                ping_barrier(peer)
                peer.sendall(packet(1009, field(1, 3) + field(2, 2)))
                peer.sendall(packet(1009, field(2, 1)))
                ping_barrier(peer)
                client.write("/chat room-kept\n")
                assert scalar(receive(peer, 1012), 3) == 3
                client.write("/exit\n/exit\n")
                receive(peer, 1010)
                ping_barrier(peer)
                peer.sendall(packet(1011, field(1, 1)))
                ping_barrier(peer)
                client.write("/chat exit-failed\n")
                assert scalar(receive(peer, 1012), 3) == 3
                client.write("/exit\n")
                receive(peer, 1010)
                peer.sendall(packet(1011, field(1, 2)))
                ping_barrier(peer)
                client.write("/enter 4\n")
                assert scalar(receive(peer, 1008), 1) == 4
                peer.sendall(packet(1009, field(1, 4) + field(2, 2)))
                ping_barrier(peer)
                client.write("/whisper Bob hello\n")
                whisper = receive(peer, 1012)
                assert scalar(whisper, 4) == b"Bob" and scalar(whisper, 1) == b"hello"
                client.write("/q\n")
                assert scalar(receive(peer, 1004), 1) == 42
                peer.sendall(packet(1005, field(1, 6)))
                client.finish()
                assert peer.recv(1) == b""
    print("PASS: manual login retry keeps PONG active; room ACKs, whisper and /q work", flush=True)


def bot_failure_scenario(executable, root):
    with listener() as (server, port):
        with RunningClient(executable, root / "bot-failure",
                           config(port, session_count=1, chat_interval_ms=60000)) as client:
            client.write("2\n")
            with server.accept()[0] as peer:
                peer.settimeout(3)
                assert scalar(receive(peer, 1002), 1) == 1
                peer.sendall(packet(1003, field(3, 5)))
                assert scalar(receive(peer, 1002), 1) == 0
                peer.sendall(packet(1003, field(3, 3)))
                assert peer.recv(1) == b"", "bot must not retry wrong password forever"
            started = time.monotonic()
            client.write("/q\n")
            client.finish()
            assert time.monotonic() - started < 3, "stop should interrupt 60-second bot interval"
    print("PASS: existing bot account retries login once; terminal error stops and /q wakes bots", flush=True)


def invalid_configs(executable, root):
    cases = [
        ("port", -1), ("port", 0), ("port", 65536), ("port", 65537),
        ("port", "7777junk"), ("port", ""), ("port", "999999999999999"),
        ("session_count", -1), ("session_count", 100001),
        ("worker_thread_count", 0), ("worker_thread_count", 65537),
        ("bot_thread_count", -1), ("bot_thread_count", 257),
        ("chat_interval_ms", 0), ("chat_interval_ms", -1), ("chat_interval_ms", 60001),
        ("bot_room_count", 0), ("bot_room_count", 257),
        ("all_chat_interval", 0), ("whisper_interval", "10oops"),
        ("ip", "bad-address"),
    ]
    for index, (key, value) in enumerate(cases):
        with RunningClient(executable, root / f"invalid-{index}",
                           config(7777, **{key: value}) if key != "port" else config(value)) as client:
            client.write("2\n")
            client.finish(expected=1)
            assert b"Invalid" in client.log.read_bytes(), (key, value)
    print(f"PASS: {len(cases)} invalid settings rejected before connection/allocation", flush=True)


def live_server_scenarios(executable, server_executable, root):
    with listener() as (_, port):
        pass
    directory = root / "server"
    (directory / "Config").mkdir(parents=True)
    (directory / "Config/config.ini").write_text(
        f"[server]\nip=127.0.0.1\nport={port}\nwork_thread_count=4\nsession_max_count=64\n",
        encoding="ascii")
    with (directory / "server.log").open("wb") as output:
        server = subprocess.Popen([str(server_executable)], cwd=directory, stdin=subprocess.PIPE, stdout=output,
                                  stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline = time.monotonic() + 10
            while True:
                assert server.poll() is None, "server startup failed"
                try:
                    with Client(port):
                        break
                except ConnectionRefusedError:
                    assert time.monotonic() < deadline, "server startup timeout"
                    time.sleep(0.05)
            # 같은 계정으로 반복 접속해 이전 실행의 로그인/방 정리까지 검증한다.
            for iteration in range(3):
                with RunningClient(executable, root / f"live-{iteration}", config(port)) as client:
                    client.write("2\n")
                    deadline = time.monotonic() + 10
                    while True:
                        assert client.process.poll() is None, "load client exited early"
                        log = client.log.read_bytes()
                        if any(int(count) >= 16 for count in re.findall(rb"login = 16 send = (\d+)", log)):
                            break
                        assert time.monotonic() < deadline, log.decode("cp949", errors="replace")
                        time.sleep(0.05)
                    if iteration == 1:
                        client.process.stdin.close()
                    else:
                        client.write("/q\n")
                    client.finish()
                with Client(port) as observer:
                    observer.sign_in(f"observer-{iteration}")
                    deadline = time.monotonic() + 5
                    while True:
                        observer.send(packet(1006))
                        rooms = observer.receive(1007)
                        if all(scalar(fields(room), 2) == 0 for room in rooms.get(1, [])):
                            break
                        assert time.monotonic() < deadline, "rooms retained disconnected client"
                        time.sleep(0.01)
                assert server.poll() is None, "server died during client stop"
            print("PASS: real server + 16 bots x 3 runs; /q/EOF exit, account reuse and empty rooms", flush=True)
        finally:
            server.terminate()
            server.wait(timeout=5)


def main():
    executable = pathlib.Path(sys.argv[1]).resolve(strict=True)
    server = pathlib.Path(sys.argv[2]).resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="client-lifecycle-") as directory:
        root = pathlib.Path(directory)
        invalid_configs(executable, root)
        manual_scenario(executable, root)
        bot_failure_scenario(executable, root)
        live_server_scenarios(executable, server, root)
    print("PASS: all client lifecycle scenarios", flush=True)


if __name__ == "__main__":
    main()
