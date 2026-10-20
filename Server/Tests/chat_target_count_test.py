"""별도 서버에서 실제 채팅 대상 수와 부하 클라이언트 집계를 검증한다.

python chat_target_count_test.py path/to/GameServer.exe [path/to/TestClient.exe]
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
from session_lifecycle_test import Client, field, fields, packet, scalar


def receive(client):
    while True:
        size, kind = struct.unpack("<HH", client.exact(4))
        body = client.exact(size - 4)
        if kind == 1016:
            client.send(packet(1015, body))
            continue
        return kind, fields(body)


def enter(client, room):
    client.send(packet(1008, field(1, room)))
    assert scalar(client.receive(1009), 2) == 2


def leave(client):
    client.send(packet(1010))
    assert scalar(client.receive(1011), 1) == 2


def chat(sender, clients, recipients, state=1, room=1, target="", message="count-test", result=3):
    sender.send(packet(1012, field(1, message) + field(2, state)
                       + field(3, room) + field(4, target)))
    self_deliveries = 0
    while True:
        kind, body = receive(sender)
        if kind == 1013:
            assert scalar(body, 1) == result, body
            assert scalar(body, 2) == len(recipients), (body, len(recipients))
            break
        assert kind == 1014 and scalar(body, 2) == message.encode(), (kind, body)
        self_deliveries += 1
    assert self_deliveries == int(sender in recipients)
    for client in recipients:
        if client is not sender:
            body = client.receive(1014)
            assert scalar(body, 2) == message.encode()
    # 다음 응답 이전에 예상 밖 채팅이 남아 있으면 대상 선택이 잘못된 것이다.
    for client in clients:
        client.send(packet(1006))
        client.receive(1007)


def scenarios(port):
    with contextlib.ExitStack() as stack:
        clients = [stack.enter_context(Client(port)) for _ in range(4)]
        for index, client in enumerate(clients):
            client.sign_in(f"metric-{index}")
        first, second, third, fourth = clients
        enter(first, 1)
        enter(second, 1)
        enter(third, 2)
        chat(first, clients, [first, second])
        leave(second)
        chat(first, clients, [first])
        enter(fourth, 1)
        chat(first, clients, [first, fourth])
        fourth.send(packet(1004))
        assert scalar(fourth.receive(1005), 1) == 6
        active = clients[:3]
        chat(first, active, [first])
        chat(first, active, [first, third], state=0)
        chat(first, active, [second], state=2, target="metric-1")
        chat(first, active, [], state=2, target="missing-user", result=2)
        chat(first, active, [], message="", result=1)
        chat(first, active, [], room=2, result=1)
        print("PASS: room join/leave/logout, all-chat, whisper and rejected requests report exact targets",
              flush=True)

        third.sock.close()
        deadline = time.monotonic() + 5
        while True:
            first.send(packet(1006))
            rooms = first.receive(1007)
            if all(scalar(fields(room), 2) == 0
                   for room in rooms[1] if scalar(fields(room), 1) == 2):
                break
            assert time.monotonic() < deadline, "disconnected recipient was not removed"
            time.sleep(0.01)
        chat(first, active[:2], [first], state=0)
        leave(first)
        chat(first, active[:2], [], state=0)
        print("PASS: disconnect and zero-recipient broadcasts update target count", flush=True)


def actual_client(executable, port, root):
    directory = root / "client"
    (directory / "Config").mkdir(parents=True)
    (directory / "Config/config.ini").write_text(
        f"[server]\nip=127.0.0.1\nport={port}\n"
        "[client]\nsession_count=3\nworker_thread_count=1\nbot_thread_count=1\n"
        "chat_interval_ms=200\nbot_room_count=1\nall_chat_interval=1000000\n"
        "whisper_interval=1000000\nbot_room_target_count=99\n", encoding="ascii")
    log = directory / "client.log"
    with log.open("wb") as output:
        process = subprocess.Popen(
            [str(executable)], cwd=directory, stdin=subprocess.PIPE,
            stdout=output, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            process.stdin.write(b"2\n")
            process.stdin.flush()
            deadline = time.monotonic() + 12
            while True:
                assert process.poll() is None, "load client exited"
                data = log.read_bytes()
                rows = re.findall(
                    rb"login = 3 send = (\d+) recv = (\d+) expected_recv = (\d+)"
                    rb" disconnect = 0 .*?backlog = (\d+)", data)
                if any(int(sent) >= 9 and int(actual) == int(expected) > 0 and int(backlog) == 0
                       for sent, actual, expected, backlog in rows):
                    break
                assert time.monotonic() < deadline, data.decode("cp949", errors="replace")
                time.sleep(0.1)
        finally:
            process.stdin.close()
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=5)
    print("PASS: actual load client counts ACK targets; stale room-size setting 99 is ignored",
          flush=True)


def main():
    server = pathlib.Path(sys.argv[1]).resolve(strict=True)
    client = pathlib.Path(sys.argv[2]).resolve(strict=True) if len(sys.argv) > 2 else None
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    with tempfile.TemporaryDirectory(prefix="chat-target-count-") as directory:
        root = pathlib.Path(directory)
        (root / "Config").mkdir()
        (root / "Config/config.ini").write_text(
            f"[server]\nip=127.0.0.1\nport={port}\n"
            "work_thread_count=4\nsession_max_count=30\n", encoding="ascii")
        with (root / "server.log").open("wb") as output:
            process = subprocess.Popen(
                [str(server)], cwd=root, stdout=output, stderr=subprocess.STDOUT,
                creationflags=subprocess.CREATE_NO_WINDOW)
            try:
                deadline = time.monotonic() + 10
                while True:
                    assert process.poll() is None, "server exited"
                    try:
                        with Client(port):
                            break
                    except ConnectionRefusedError:
                        assert time.monotonic() < deadline, "server startup timeout"
                        time.sleep(0.05)
                scenarios(port)
                if client:
                    actual_client(client, port, root)
                assert process.poll() is None, "server exited during tests"
            finally:
                process.terminate()
                process.wait(timeout=5)
    print("PASS: all chat target count scenarios", flush=True)


if __name__ == "__main__":
    main()
