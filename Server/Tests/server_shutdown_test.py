"""별도 loopback 서버의 /q·입력 종료와 연결/작업 정리를 검증한다."""
import argparse
import concurrent.futures
import contextlib
import pathlib
import re
import socket
import subprocess
import sys
import tempfile
import threading
import time

sys.dont_write_bytecode = True
from session_lifecycle_test import Client, field, packet, scalar


def wait_for(action, timeout=15):
    deadline = time.monotonic() + timeout
    while True:
        result = action()
        if result:
            return result
        assert time.monotonic() < deadline, "timed out waiting for test condition"
        time.sleep(0.05)


class Server:
    def __init__(self, executable, root, workers=4, capacity=2000):
        self.root = root
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            self.port = reservation.getsockname()[1]
        (root / "Config").mkdir(parents=True)
        (root / "Config/config.ini").write_text(
            f"[server]\nip=127.0.0.1\nport={self.port}\nwork_thread_count={workers}\n"
            f"session_max_count={capacity}\n", encoding="ascii")
        self.output = (root / "console.log").open("wb")
        self.process = subprocess.Popen(
            [str(executable)], cwd=root, stdin=subprocess.PIPE, stdout=self.output,
            stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            def ready():
                assert self.process.poll() is None, "server exited before startup"
                return b"start accept" in (root / "console.log").read_bytes()
            wait_for(ready)
        except BaseException:
            self.close()
            raise

    def quit(self, eof=False):
        started = time.monotonic()
        if eof:
            self.process.stdin.close()
        else:
            self.process.stdin.write(b"/q\n")
            self.process.stdin.flush()
        assert self.process.wait(timeout=15) == 0, "server shutdown failed"
        elapsed = time.monotonic() - started
        text = (self.root / "console.log").read_bytes().decode("cp949", errors="replace")
        assert "서버 정상 종료 완료" in text, text[-3000:]
        rows = re.findall(r"connect = (\d+) login = (\d+).*? disconnect = (\d+)", text.replace(",", ""))
        assert rows and rows[-1][0] == rows[-1][2], (rows[-1:] or text[-1000:])
        assert b"/q" in (self.root / "console.log").read_bytes()
        print(f"PASS: {self.root.name}: exit=0, connect/login/disconnect={rows[-1]}, stop={elapsed:.3f}s", flush=True)
        return text

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


def idle_tests(executable, root):
    for eof in (False, True):
        with Server(executable, root / ("idle_eof" if eof else "idle_q"), workers=1) as server:
            server.quit(eof=eof)


def active_test(executable, root):
    with Server(executable, root / "active_single_worker", workers=1) as server:
        with contextlib.ExitStack() as stack:
            clients = []
            for i in range(32):
                client = stack.enter_context(Client(server.port))
                clients.append(client)
                client.sign_in(f"shutdown-{i}")
                client.send(packet(1008, field(1, i % 10)))
                assert scalar(client.receive(1009), 2) == 2
            text = server.quit()
            assert re.search(r"connect = 32 login = 32 .* disconnect = 32", text)
            # 파일 로그 대신 모든 클라이언트 소켓에서 실제 연결 종료를 확인한다.
            for client in clients:
                try:
                    while client.sock.recv(4096):
                        pass
                except ConnectionResetError:
                    pass


def accept_race_test(executable, root):
    with Server(executable, root / "accept_race", capacity=4) as server:
        with contextlib.ExitStack() as stack:
            clients = [stack.enter_context(Client(server.port)) for _ in range(4)]
            for i, client in enumerate(clients):
                client.sign_in(f"capacity-{i}")
            # 정원 초과 송신과 새 Accept가 계속 발생하는 동안 종료한다.
            connected = threading.Event()
            stopping = threading.Event()

            def connect_until_closed():
                while not stopping.is_set():
                    try:
                        with Client(server.port) as client:
                            assert scalar(client.receive(1017), 1) == 4
                            assert client.sock.recv(1) == b""
                            connected.set()
                    except (OSError, EOFError):
                        return

            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                futures = [pool.submit(connect_until_closed) for _ in range(4)]
                try:
                    assert connected.wait(5), "no capacity rejection exercised"
                    # 로그인 세션의 수신/송신 작업도 종료와 겹치게 한다.
                    for client in clients:
                        client.send(packet(1006) * 100)
                    text = server.quit()
                finally:
                    stopping.set()
                for future in futures:
                    future.result(timeout=5)


def load_test(executable, client_executable, root):
    sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / "TestClient/Tests"))
    from client_lifecycle_test import RunningClient, config
    with Server(executable, root / "load_1000_server") as server:
        with RunningClient(client_executable, root / "load_1000_client", config(
                server.port, session_count=1000, bot_thread_count=4,
                chat_interval_ms=500, bot_room_count=10)) as client:
            client.write("2\n")

            def active():
                assert client.process.poll() is None, "load client exited early"
                return re.search(rb"login = 1000 send = ([1-9]\d*)", client.log.read_bytes().replace(b",", b""))
            wait_for(active, timeout=30)
            text = server.quit()
            assert "login = 1000" in text.replace(",", "")
            client.write("/q\n")
            client.finish()
            assert "disconnect = 1000" in text.replace(",", "")
            assert not list(client.directory.glob("Log/*_Client.log")), "client file logger remains"


def run(args, root):
    idle_tests(args.server, root)
    active_test(args.server, root)
    accept_race_test(args.server, root)
    if args.client:
        load_test(args.server, args.client, root)
    print("PASS: all graceful shutdown scenarios", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("server", type=pathlib.Path)
    parser.add_argument("--client", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()
    args.server = args.server.resolve(strict=True)
    if args.client:
        args.client = args.client.resolve(strict=True)
    if args.output:
        args.output.mkdir(parents=True, exist_ok=False)
        run(args, args.output.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="server-shutdown-") as directory:
            run(args, pathlib.Path(directory))


if __name__ == "__main__":
    main()
