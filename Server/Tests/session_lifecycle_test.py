"""Python 3 표준 라이브러리로 별도의 GameServer.exe를 실행해 세션 동작을 검사한다.

    python session_lifecycle_test.py path/to/GameServer.exe

임시 설정과 loopback 포트를 사용해 창을 띄우지 않고 자식 프로세스를 실행한다.
기존 서버 설정을 변경하거나 이미 실행 중인 서버에 접속하지 않는다.
"""
import concurrent.futures
import pathlib
import socket
import struct
import subprocess
import sys
import tempfile
import time


LOGIN_SUCCESS = 8


def varint(n):
    out = bytearray()
    while n > 127:
        out.append((n & 127) | 128)
        n >>= 7
    out.append(n)
    return bytes(out)


def field(number, value):
    if isinstance(value, str):
        value = value.encode()
    if isinstance(value, bytes):
        return varint(number * 8 + 2) + varint(len(value)) + value
    return varint(number * 8) + varint(value)


def packet(kind, body=b""):
    return struct.pack("<HH", len(body) + 4, kind) + body


def login(name, create=True, bot=False):
    return packet(1002 if bot else 1000,
                  field(1, int(create)) + field(2, name) + field(3, "test-password")
                  + field(4, name))


def fields(data):
    pos = 0

    def read_varint():
        nonlocal pos
        n, shift = 0, 0
        while True:
            b = data[pos]
            pos += 1
            n |= (b & 127) << shift
            if b < 128:
                return n
            shift += 7

    result = {}
    while pos < len(data):
        tag = read_varint()
        if tag & 7 == 0:
            value = read_varint()
        else:
            assert tag & 7 == 2, f"unexpected wire type: {tag & 7}"
            size = read_varint()
            value = data[pos:pos + size]
            pos += size
        result.setdefault(tag >> 3, []).append(value)
    return result


def scalar(data, number, default=0):
    return data.get(number, [default])[0]


class Client:
    def __init__(self, port):
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=5)

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.sock.close()

    def send(self, data):
        self.sock.sendall(data)

    def exact(self, size):
        out = b""
        while len(out) < size:
            chunk = self.sock.recv(size - len(out))
            if not chunk:
                raise EOFError("connection closed")
            out += chunk
        return out

    def receive(self, expected):
        while True:
            size, kind = struct.unpack("<HH", self.exact(4))
            body = self.exact(size - 4)
            if kind == 1016:
                self.send(packet(1015, body))
                continue
            assert kind == expected, (expected, kind)
            return fields(body)

    def sign_in(self, name, create=True):
        self.send(login(name, create))
        response = self.receive(1001)
        assert scalar(response, 3) == LOGIN_SUCCESS, response
        return scalar(response, 1)


def reconnect(port, name):
    deadline = time.monotonic() + 5
    while True:
        with Client(port) as client:
            client.send(login(name, False))
            result = scalar(client.receive(1001), 3)
            if result == LOGIN_SUCCESS:
                client.send(packet(1004))
                assert scalar(client.receive(1005), 1) == 6
                return
            assert result == 6, result
        assert time.monotonic() < deadline, f"account not released: {name}"
        time.sleep(0.01)


def scenarios(port):
    with Client(port) as client:
        wire = login("fragmented")
        client.send(wire[:2])
        time.sleep(0.01)
        client.send(wire[2:])
        assert scalar(client.receive(1001), 3) == LOGIN_SUCCESS
        client.send(packet(1004))
        assert scalar(client.receive(1005), 1) == 6
    print("PASS: fragmented receive and logout", flush=True)

    # 여러 패킷이 같은 수신 버퍼를 사용해도 버퍼 재사용으로 큐 작업의 데이터가 바뀌면 안 된다.
    with Client(port) as client:
        burst = b"".join(login(f"pipeline-{i}") + packet(1004) for i in range(60))
        client.send(burst)
        ids = set()
        for i in range(60):
            response = client.receive(1001)
            assert scalar(response, 3) == LOGIN_SUCCESS
            assert scalar(response, 2) == f"pipeline-{i}".encode()
            ids.add(scalar(response, 1))
            assert scalar(client.receive(1005), 1) == 6
        assert len(ids) == 60
    print("PASS: 120 pipelined login/logout packets preserve data and order", flush=True)

    # 일반 로그인과 봇 로그인 모두 이미 로그인한 세션의 재요청을 연결 종료로 막아야 한다.
    for bot in (False, True):
        name = f"duplicate-{int(bot)}"
        with Client(port) as client:
            client.sign_in(name)
            client.send(login("must-not-create", bot=bot) + login("also-not-created"))
            try:
                client.receive(1003 if bot else 1001)
                raise AssertionError("duplicate login did not disconnect")
            except (EOFError, ConnectionResetError):
                pass
        reconnect(port, name)
    with Client(port) as client:
        for name in ("must-not-create", "also-not-created"):
            client.send(login(name, False))
            assert scalar(client.receive(1001), 3) == 2
    print("PASS: duplicate normal/bot login disconnects; pending requests skipped", flush=True)

    # 요청이 대기 중인 상태에서 연결을 끊고, 새 소켓에서 같은 계정으로 다시 로그인한다.
    for i in range(25):
        name = f"disconnect-{i}"
        with Client(port) as client:
            client.sign_in(name)
            client.send(packet(1008, field(1, 1)))
            assert scalar(client.receive(1009), 2) == 2
            client.send(packet(1006) * 100)
        reconnect(port, name)
    print("PASS: disconnect cleanup and immediate reconnect (25 sessions)", flush=True)

    def parallel_client(index):
        with Client(port) as client:
            for iteration in range(10):
                name = f"parallel-{index}-{iteration}"
                client.send(login(name) + packet(1008, field(1, 1)) + packet(1004))
                assert scalar(client.receive(1001), 3) == LOGIN_SUCCESS
                assert scalar(client.receive(1009), 2) == 2
                assert scalar(client.receive(1005), 1) == 6
                # 명시적으로 로그아웃했다면 같은 세션에서 다시 로그인할 수 있어야 한다.
                client.send(login(name, False) + packet(1008, field(1, 1))
                            + packet(1010) + packet(1004))
                assert scalar(client.receive(1001), 3) == LOGIN_SUCCESS
                assert scalar(client.receive(1009), 2) == 2
                assert scalar(client.receive(1011), 1) == 2
                assert scalar(client.receive(1005), 1) == 6

    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        list(pool.map(parallel_client, range(8)))
    with Client(port) as client:
        client.sign_in("room-observer")
        client.send(packet(1006))
        rooms = client.receive(1007)
        assert len(rooms[1]) == 10
        for room in rooms[1]:
            assert scalar(fields(room), 2) == 0, fields(room)
    print("PASS: 8 parallel clients / 160 login cycles; all room counts return to zero",
          flush=True)


def main():
    executable = pathlib.Path(sys.argv[1]).resolve(strict=True)
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    with tempfile.TemporaryDirectory(prefix="session-queue-") as directory:
        root = pathlib.Path(directory)
        (root / "Config").mkdir()
        (root / "Config" / "config.ini").write_text(
            f"[server]\nip=127.0.0.1\nport={port}\nwork_thread_count=4\n"
            "session_max_count=2000\n", encoding="ascii")
        with (root / "server.log").open("wb") as output:
            process = subprocess.Popen([str(executable)], cwd=root, stdin=subprocess.PIPE, stdout=output,
                                       stderr=subprocess.STDOUT,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
            try:
                deadline = time.monotonic() + 10
                while True:
                    assert process.poll() is None, "server exited before accepting clients"
                    try:
                        with Client(port):
                            break
                    except ConnectionRefusedError:
                        assert time.monotonic() < deadline, "server startup timeout"
                        time.sleep(0.05)
                scenarios(port)
                assert process.poll() is None, "server exited during tests"
            finally:
                process.terminate()
                process.wait(timeout=5)
    print("PASS: all lifecycle scenarios", flush=True)


if __name__ == "__main__":
    main()
