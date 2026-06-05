# Server Portfolio

GitHub: [GodDongjin/Server_Portfolio](https://github.com/GodDongjin/Server_Portfolio)

C++ IOCP 기반 게임 서버 포트폴리오 프로젝트입니다. `ServerCore`는 네트워크 공통 로직을 담당하고, `GameServer`는 로그인과 채팅 같은 게임 서버 기능을 구현합니다. `TestClient`는 수동 테스트와 부하 테스트를 지원합니다.

## Features

- IOCP 기반 비동기 네트워크 처리
- `Session`, `Listener`, `Service`, `SessionManager` 구조
- `RecvBuffer`, `SendBuffer` 기반 패킷 송수신
- Protobuf 기반 패킷 직렬화
- 로그인 / 로그아웃
- 전체 채팅 broadcast
- TestClient 수동 테스트 모드
- TestClient 부하 테스트 모드
- 서버 / 클라이언트 통계 출력

## Project Structure

```txt
Server_Portfolio
├─ Server
│  ├─ ServerCore
│  │  ├─ NetWork
│  │  ├─ Utils
│  │  ├─ Thread
│  │  ├─ Job
│  │  └─ DB
│  └─ GameServer
│     ├─ GameSession
│     ├─ Login
│     ├─ PacketHandler
│     └─ Protocol
└─ TestClient
   ├─ NetWork
   ├─ Session
   ├─ Packet
   ├─ Buffer
   ├─ Protocol
   └─ Utils
```

## Architecture

### ServerCore

`ServerCore`는 게임 서버에서 공통으로 사용할 수 있는 네트워크 기반 구조를 담당합니다.

- `IocpCore`: IOCP handle 관리 및 completion dispatch
- `Listener`: client accept 처리
- `Service`: 서버 실행 단위
- `Session`: socket, recv, send, disconnect 처리
- `SessionManager`: 접속 세션 관리 및 broadcast
- `RecvBuffer`: 수신 버퍼 관리
- `SendBuffer`: 송신 버퍼 관리
- `SocketUtils`: socket 생성 및 option 설정

### GameServer

`GameServer`는 실제 게임 서버 로직을 담당합니다.

- `GameSession`: 클라이언트 세션 확장
- `Login`: 계정 생성, 로그인, 로그아웃 관리
- `ServerPacketHandler`: 패킷 분배 및 처리
- `ServerStats`: 서버 통계 수집
- `Protocol`: Protobuf 기반 패킷 구조

### TestClient

`TestClient`는 서버 검증을 위한 클라이언트입니다.

- Manual Mode: 직접 로그인 및 채팅 입력
- Load Test Mode: 다수 봇 세션 생성 및 부하 테스트
- Client 통계 출력
- IOCP 기반 비동기 recv / send 처리

## Packet Flow

### Login

```txt
Client
  └─ REQ_LOGIN / REQ_BOT_LOGIN
        ↓
GameServer
  └─ Login validation
        ↓
Client
  └─ ACK_LOGIN / ACK_BOT_LOGIN
```

### Chat Broadcast

```txt
Client
  └─ REQ_CHAT
        ↓
GameServer
  └─ SessionManager::broadcast()
        ↓
All Clients
  └─ ACK_SEND_CHAT
```

## Load Test

TestClient의 Load Test Mode를 사용해 1,000개 클라이언트 세션을 생성하고, 전체 채팅 broadcast 부하를 측정했습니다.

### Test Environment

```txt
Server: GameServer
Client: TestClient Load Test Mode
Protocol: TCP / IOCP
Serialization: Protobuf
Client Count: 1,000
Chat Type: Full Broadcast
```

### Result

| Clients | Chat Interval | Approx Broadcast/sec | Result |
|---:|---:|---:|---|
| 1,000 | 5000ms | 200K/sec | Stable |
| 1,000 | 3000ms | 333K/sec | Stable |
| 1,000 | 2500ms | 400K/sec | Stable |
| 1,000 | 2000ms | 500K/sec | Backlog Increasing |

### Summary

- 1,000 client sessions connected successfully
- 1,000 login requests completed successfully
- Full broadcast to 1,000 clients was verified
- 2500ms interval test recovered backlog to zero before the next burst
- 2000ms interval test showed increasing backlog

## Example Stats

### Client

```txt
connect = 1,000 login = 1,000 send = 4,321 recv = 4,235,490 disconnect = 0 send/s = 0 recv/s = 465,404 backlog = 0
```

### Server

```txt
[SERVER] connect = 1,000 login = 1,000 recv_chat = 4,321 broadcast_target = 4,321,000 disconnect = 0
```

## How to Run

### Clone

```bash
git clone https://github.com/GodDongjin/Server_Portfolio.git
```

### 1. GameServer 실행

Visual Studio에서 다음 솔루션을 엽니다.

```txt
Server/GameServer.sln
```

`GameServer` 프로젝트를 `Debug x64` 또는 `Release x64`로 실행합니다.

### 2. TestClient 실행

Visual Studio에서 다음 솔루션을 엽니다.

```txt
TestClient/TestClient.sln
```

실행 후 모드를 선택합니다.

```txt
1. Manual Test
2. Load Test
```

### Manual Test Commands

```txt
/chat message
/q
```

### Load Test Config

`LoadTestConfig`에서 부하 테스트 설정을 변경할 수 있습니다.

```cpp
struct LoadTestConfig
{
    int32 session_count = 1000;
    int32 bot_thread_count = 4;
    int32 worker_thread_count = 4;
    int32 chat_interval_ms = 2500;
    wstring ip = L"127.0.0.1";
    uint16 port = 7777;
};
```

## Current Limitations

- 현재 채팅은 전체 broadcast 중심입니다.
- Room / Channel 단위 채팅은 아직 구현되지 않았습니다.
- 계정 정보는 DB가 아닌 메모리 기반으로 관리됩니다.
- 서버 종료 처리와 graceful shutdown은 추가 개선이 필요합니다.
- 부하 테스트는 단일 PC 환경 기준입니다.

## Next Improvements

- Room / Channel 기반 채팅 구조
- DB 기반 계정 저장
- 서버 설정 파일화
- graceful shutdown
- 패킷 검증 강화
- JobQueue 기반 room broadcast
- Release 빌드 기준 성능 재측정

## Tech Stack

- C++17
- Windows IOCP
- Winsock2
- Protobuf
- Visual Studio 2022
