# C++ IOCP Game Server Portfolio

Windows IOCP 기반 비동기 TCP 게임 서버 포트폴리오입니다. `ServerCore`는 네트워크 공통 구조를 담당하고, `GameServer`는 로그인과 전체 채팅 broadcast 같은 게임 서버 기능을 구현합니다. `TestClient`는 수동 테스트와 다중 세션 부하 테스트를 지원합니다.

## Features

- Windows IOCP 기반 비동기 네트워크 처리
- `Session`, `Listener`, `Service`, `SessionManager` 기반 서버 구조
- `RecvBuffer`, `SendBuffer` 기반 패킷 송수신
- Protobuf 기반 패킷 직렬화
- 로그인 / 로그아웃
- 전체 채팅 broadcast
- TestClient 수동 테스트 모드
- TestClient 부하 테스트 모드
- 서버 / 클라이언트 통계 출력
- ini 기반 서버/클라이언트 실행 설정 분리

## Tech Stack

- C++17 / C++20
- Windows IOCP
- Winsock2
- Protobuf
- Visual Studio 2022
- MySQL ODBC, DB 연동 확장용

## Project Structure

```txt
Server_Portfolio
├─ Server
│  ├─ GameServer
│  │  ├─ Config
│  │  ├─ Login
│  │  ├─ Main
│  │  ├─ Packet
│  │  ├─ Protobuf
│  │  ├─ Struct_info
│  │  └─ Utils
│  └─ ServerCore
│     ├─ DB
│     ├─ Job
│     ├─ NetWork
│     ├─ Thread
│     └─ Utils
└─ TestClient
   └─ TestClient
      ├─ Buffer
      ├─ Config
      ├─ Main
      ├─ NetWork
      ├─ Packet
      ├─ Protocol
      ├─ Session
      └─ Utils
```

## Architecture

### ServerCore

`ServerCore`는 서버에서 공통으로 사용하는 네트워크, 스레드, 작업 큐, DB 연결 기반 구조를 담당합니다.

- `IocpCore`: IOCP handle 관리 및 completion dispatch
- `Listener`: client accept 처리
- `Service`: 서버 실행 단위
- `Session`: socket, recv, send, disconnect 처리
- `SessionManager`: 접속 세션 관리 및 broadcast
- `RecvBuffer`: 수신 버퍼 관리
- `SendBuffer`: 송신 버퍼 관리
- `SocketUtils`: socket 생성 및 option 설정
- `JobQueue`, `GlobalQueue`, `JobTimer`: 예약 작업 및 비동기 작업 처리 기반
- `ConfigReader`: ini 설정 파일 로딩

### GameServer

`GameServer`는 실제 게임 서버 로직을 담당합니다.

- `GameSession`: 클라이언트 세션 확장
- `Login`: 계정 생성, 로그인, 로그아웃 관리
- `ServerPacketHandler`: 패킷 분배 및 처리
- `GameGlobal`: 서버 전역 객체 및 통계 관리
- `Protobuf`: 클라이언트/서버 패킷 구조

### TestClient

`TestClient`는 서버 검증을 위한 클라이언트입니다.

- Manual Mode: 직접 로그인 및 채팅 입력
- Load Test Mode: 다수 봇 세션 생성 및 부하 테스트
- 클라이언트 송수신 통계 출력
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
  └─ SessionManager::broad_cast()
        ↓
All connected clients
  └─ ACK_SEND_CHAT
```

## Configuration

실행 설정은 `config.ini`로 분리합니다. 실제 로컬 설정 파일은 Git에 포함하지 않고, 예시 파일만 커밋합니다.

```txt
config.example.ini  # Git에 포함
config.ini          # 로컬 실행용, Git 제외
```

### GameServer Config

`Server/GameServer/Config/config.example.ini`

```ini
[server]
ip=127.0.0.1
port=7777
work_thread_count=4
session_max_count=2000

[database_info]
driver=MySQL ODBC 9.0 Unicode Driver
ip=localhost
database=2drpg
user=CHANGE_ME
password=CHANGE_ME
```

### TestClient Config

`TestClient/TestClient/Config/config.example.ini`

```ini
[client]
session_count=1000
bot_thread_count=4
worker_thread_count=4
chat_interval_ms=2500

[server]
ip=127.0.0.1
port=7777
```

## How to Run

### 1. Clone

```bash
git clone https://github.com/GodDongjin/Server_Portfolio.git
cd Server_Portfolio
```

### 2. Create Local Config Files

예시 설정 파일을 복사해 로컬 실행 설정을 만듭니다.

```bash
copy Server\GameServer\Config\config.example.ini Server\GameServer\Config\config.ini
copy TestClient\TestClient\Config\config.example.ini TestClient\TestClient\Config\config.ini
```

필요하면 `ip`, `port`, `session_count`, `worker_thread_count` 등을 로컬 환경에 맞게 수정합니다.

### 3. Run GameServer

Visual Studio 2022에서 다음 솔루션을 엽니다.

```txt
Server/GameServer.sln
```

`GameServer` 프로젝트를 `Debug x64` 또는 `Release x64`로 실행합니다.

### 4. Run TestClient

Visual Studio 2022에서 다음 솔루션을 엽니다.

```txt
TestClient/TestClient.sln
```

실행 후 테스트 모드를 선택합니다.

```txt
1. Manual Test
2. Load Test
```

Manual Mode 명령:

```txt
/chat message
/q
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

| Clients | Chat Interval | Expected Broadcast Delivery | Result |
|---:|---:|---:|---|
| 1,000 | 5000ms | 200K/sec | Stable |
| 1,000 | 3000ms | 333K/sec | Stable |
| 1,000 | 2500ms | 400K/sec | Stable |
| 1,000 | 2000ms | 500K/sec | Backlog Increasing |

### Example Stats

Client:

```txt
connect = 1,000 login = 1,000 send = 4,321 recv = 4,235,490 disconnect = 0 send/s = 0 recv/s = 465,404 backlog = 0
```

Server:

```txt
[SERVER] connect = 1,000 login = 1,000 recv_chat = 4,321 broadcast_target = 4,321,000 disconnect = 0
```

## Current Limitations

- 현재 채팅은 전체 broadcast 중심입니다.
- Room / Channel 단위 채팅은 아직 구현되어 있지 않습니다.
- 계정 정보는 DB가 아닌 메모리 기반으로 관리됩니다.
- DB 연결 코드는 확장용으로 분리되어 있으며, 현재 로그인 로직에는 연결하지 않았습니다.
- graceful shutdown은 추가 개선이 필요합니다.
- 부하 테스트는 단일 PC 환경 기준입니다.

## Next Improvements

- Room / Channel 기반 채팅 구조
- JobQueue 기반 room broadcast
- DB 기반 계정 저장
- graceful shutdown
- config 검증 강화
- 패킷 검증 강화
- Release 빌드 기준 성능 재측정
