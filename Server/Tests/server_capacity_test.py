"""별도 서버를 정원 2명으로 실행해 초과 안내, 송신 후 종료, 재접속을 검증한다.

python server_capacity_test.py path/to/GameServer.exe [path/to/TestClient.exe]
"""
import concurrent.futures
import pathlib
import re
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time

sys.dont_write_bytecode = True
from session_lifecycle_test import Client, LOGIN_SUCCESS, fields, login, packet, scalar

SERVER_FULL = 1017
CAPACITY = 2


def receive(client):
    while True:
        size, kind = struct.unpack("<HH", client.exact(4))
        assert size >= 4, size
        body = client.exact(size - 4)
        if kind == 1016:
            client.send(packet(1015, body))
            continue
        return kind, fields(body)


def expect_closed(client):
    # 먼저 안내 패킷 전체를 받은 뒤, 클라이언트가 닫지 않아도 서버가 종료하는지 확인한다.
    assert client.sock.recv(1) == b"", "server sent extra data after SERVER_FULL"


def reject(port, index, send_login=True):
    with Client(port) as client:
        if send_login:
            client.send(login(f"rejected-{index}"))
        kind, body = receive(client)
        assert kind == SERVER_FULL, (kind, body)
        assert scalar(body, 1) == CAPACITY, body
        expect_closed(client)


def admitted(port, name):
    deadline = time.monotonic() + 5
    while True:
        client = Client(port)
        try:
            client.send(login(name))
            kind, body = receive(client)
            if kind == 1001:
                assert scalar(body, 3) == LOGIN_SUCCESS, body
                return client
            assert kind == SERVER_FULL, (kind, body)
            expect_closed(client)
        except BaseException:
            client.sock.close()
            raise
        client.sock.close()
        assert time.monotonic() < deadline, "released slot did not become available"
        time.sleep(0.02)


def client_notice(client_exe, port, root):
    directory = root / "client"
    (directory / "Config").mkdir(parents=True)
    (directory / "Config/config.ini").write_text(
        f"[server]\nip=127.0.0.1\nport={port}\n"
        "[client]\nsession_count=2\nworker_thread_count=1\nbot_thread_count=1\n"
        "chat_interval_ms=1000\nbot_room_count=1\n", encoding="ascii")
    log_path = directory / "client.log"
    with log_path.open("wb") as output:
        process = subprocess.Popen(
            [str(client_exe)], cwd=directory, stdin=subprocess.PIPE, stdout=output,
            stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            process.stdin.write(b"2\n")
            process.stdin.flush()
            deadline = time.monotonic() + 10
            while True:
                assert process.poll() is None, "TestClient exited"
                data = log_path.read_bytes()
                message = "서버 접속 인원이 가득 찼습니다."
                notice_count = max(data.count(message.encode(encoding))
                                   for encoding in ("utf-8", "cp949"))
                if notice_count == 2 and re.search(
                        rb"connect = 2 login = 0 .*disconnect = 2", data):
                    break
                assert time.monotonic() < deadline, data.decode("cp949", errors="replace")
                time.sleep(0.1)
        finally:
            process.stdin.close()
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=5)
    print("PASS: actual TestClient displays Korean notice; login=0, disconnect=2", flush=True)


def scenarios(port, first, client_exe, root):
    held = [first]
    try:
        first.sign_in("capacity-first")
        held.append(admitted(port, "capacity-second"))

        # 로그인 없이 기다리는 연결에도 안내가 와야 하며, 바로 로그인해도 처리하지 않는다.
        reject(port, 0, send_login=False)
        with concurrent.futures.ThreadPoolExecutor(max_workers=12) as pool:
            list(pool.map(lambda i: reject(port, i), range(1, 25)))
        reject(port, 25)
        print("PASS: 26 full-server connections receive complete notice then EOF", flush=True)

        # 거절 세션의 정리가 정상 세션 수를 감소시키지 않는지 반복 접속으로 검증한다.
        for client in held:
            client.send(packet(1006))
            assert len(client.receive(1007)[1]) == 10

        if client_exe:
            client_notice(client_exe, port, root)

        # 송신 전에 상대가 강제 종료한 경우에도 다음 연결을 계속 처리해야 한다.
        for _ in range(10):
            with socket.create_connection(("127.0.0.1", port), timeout=5) as sock:
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("HH", 1, 0))
        reject(port, 26)
        print("PASS: rejection cleanup and peer reset preserve occupied slots", flush=True)

        held.pop().sock.close()
        replacement = admitted(port, "capacity-replacement")
        held.append(replacement)
        # 초과 상태에서 보낸 계정 생성 요청이 실행되지 않았는지 확인한다.
        replacement.send(packet(1004))
        assert scalar(replacement.receive(1005), 1) == 6
        replacement.send(login("rejected-1", create=False))
        assert scalar(replacement.receive(1001), 3) == 2
        replacement.sign_in("capacity-replacement", create=False)
        reject(port, 27)
        print("PASS: released slot admits a new client; rejected login was never executed", flush=True)

        # 한 자리가 비었을 때 동시에 접속해도 한 연결만 입장해야 한다.
        held.pop().sock.close()
        for attempt in range(10):
            barrier = threading.Barrier(12)

            def compete(index):
                barrier.wait(timeout=5)
                client = Client(port)
                try:
                    client.send(login(f"race-{attempt}-{index}"))
                    kind, body = receive(client)
                    if kind == 1001:
                        assert scalar(body, 3) == LOGIN_SUCCESS, body
                        return client
                    assert kind == SERVER_FULL and scalar(body, 1) == CAPACITY, (kind, body)
                    expect_closed(client)
                except BaseException:
                    client.sock.close()
                    raise
                client.sock.close()
                return None

            with concurrent.futures.ThreadPoolExecutor(max_workers=12) as pool:
                results = list(pool.map(compete, range(12)))
            winners = [client for client in results if client is not None]
            held.extend(winners)
            assert len(winners) <= 1, f"capacity exceeded: {len(winners) + 1}"
            if winners:
                break
            time.sleep(0.02)
        assert len(held) == CAPACITY, "no connection admitted after cleanup"
        reject(port, 28)
        print("PASS: 12 concurrent connections compete for one slot; exactly one admitted", flush=True)
    finally:
        for client in held:
            client.sock.close()


def main():
    server_exe = pathlib.Path(sys.argv[1]).resolve(strict=True)
    client_exe = pathlib.Path(sys.argv[2]).resolve(strict=True) if len(sys.argv) > 2 else None
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    with tempfile.TemporaryDirectory(prefix="server-capacity-") as directory:
        root = pathlib.Path(directory)
        (root / "Config").mkdir()
        (root / "Config/config.ini").write_text(
            f"[server]\nip=127.0.0.1\nport={port}\nwork_thread_count=4\n"
            f"session_max_count={CAPACITY}\n", encoding="ascii")
        with (root / "server.log").open("wb") as output:
            process = subprocess.Popen(
                [str(server_exe)], cwd=root, stdin=subprocess.PIPE, stdout=output, stderr=subprocess.STDOUT,
                creationflags=subprocess.CREATE_NO_WINDOW)
            try:
                deadline = time.monotonic() + 10
                while True:
                    assert process.poll() is None, "server exited before accepting clients"
                    try:
                        first = Client(port)
                        break
                    except ConnectionRefusedError:
                        assert time.monotonic() < deadline, "server startup timeout"
                        time.sleep(0.05)
                scenarios(port, first, client_exe, root)
                assert process.poll() is None, "server exited during tests"
            except BaseException:
                print((root / "server.log").read_bytes()[-4000:].decode("cp949", errors="replace"))
                raise
            finally:
                if process.poll() is None:
                    process.terminate()
                process.wait(timeout=5)
    print("PASS: all capacity scenarios", flush=True)


if __name__ == "__main__":
    main()
