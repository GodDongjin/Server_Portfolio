# C++ IOCP Game Server Portfolio

Windows IOCP 기반 비동기 TCP 게임 서버 포트폴리오입니다.

`ServerCore`는 IOCP, 세션, 송수신 버퍼, 스레드, 설정 로딩 등 서버 공통 구조를 담당합니다.  
`GameServer`는 로그인, 룸 입장/퇴장, 룸 채팅, 전체 채팅, 귓속말, heartbeat 기반 연결 상태 관리를 구현합니다.  
`TestClient`는 수동 테스트와 다중 세션 부하 테스트를 지원합니다.

## 특징

- Windows IOCP 기반 비동기 네트워크 처리
- `Session`, `Listener`, `Service`, `SessionManager` 기반 서버 구조
- `RecvBuffer`, `SendBuffer` 기반 패킷 송수신
- Protobuf 기반 패킷 직렬화
- 로그인 / 로그아웃
- 룸 정보 조회 / 룸 입장 / 룸 퇴장
- 룸 채팅 / 전체 채팅 / 귓속말
- Heartbeat ping-pong 기반 timeout disconnect
- Send queue overflow 방어
- Packet size / packet id 검증
- TestClient 수동 테스트 모드
- TestClient 다중 세션 부하 테스트 모드
- 서버 / 클라이언트 통계 출력
- ini 기반 서버/클라이언트 실행 설정 분리

## 기술 스택

- C++17 / C++20
- Windows IOCP
- Winsock2
- Protobuf
- Visual Studio 2022
- MySQL ODBC, DB 연동 확장용

## 프로젝트 구조

```txt
Server_Portfolio
├─ Server
│  ├─ GameServer
│  │  ├─ Config
│  │  ├─ Login
│  │  ├─ Main
│  │  ├─ Packet
│  │  ├─ Protobuf
│  │  ├─ Room
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

## 구조

### ServerCore

`ServerCore`는 서버에서 공통으로 사용하는 네트워크, 스레드, 작업 큐, 설정 로딩, DB 연결 기반 구조를 담당합니다.

- `IocpCore`: IOCP handle 관리 및 completion dispatch
- `Listener`: client accept 처리
- `Service`: 서버 실행 단위
- `Session`: socket, recv, send, disconnect 처리
- `SessionManager`: 접속 세션, 로그인 세션, 닉네임 세션 관리
- `RecvBuffer`: 수신 버퍼 관리
- `SendBuffer`: 송신 버퍼 관리
- `SocketUtils`: socket 생성 및 option 설정
- `JobQueue`, `GlobalQueue`, `JobTimer`: 예약 작업 및 비동기 작업 처리 기반
- `ConfigReader`: ini 설정 파일 로딩

### GameServer

`GameServer`는 실제 게임 서버 로직을 담당합니다.

- `GameSession`: 클라이언트 세션 확장
- `Login`: 계정 생성, 로그인, 로그아웃 관리
- `Room`, `RoomManager` : 룸 생성, 입장, 퇴장, 룸 유저 관리
- `ServerPacketHandler`: 패킷 분배 및 처리
- `GameGlobal`: 서버 전역 객체 및 통계 관리
- `Protobuf`: 클라이언트/서버 패킷 구조

### TestClient

`TestClient`는 서버 검증을 위한 클라이언트입니다.

- `Manual Mode`: 직접 로그인, 룸 조회/입장/퇴장, 채팅 입력
- `Load Test Mode`: 다수 봇 세션 생성 및 부하 테스트
- 클라이언트 송수신 통계 출력
- IOCP 기반 비동기 recv / send 처리

## Packet 흐름

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

### Room

```txt
Client
  └─ REQ_GET_ROOM_INFO / REQ_ENTER_ROOM / REQ_EXIT_ROOM
        ↓
GameServer
  └─ RoomManager
        ↓
Client
  └─ ACK_GET_ROOM_INFO / ACK_ENTER_ROOM / ACK_EXIT_ROOM
```

### Chat

```txt
Client
  └─ REQ_CHAT
        ↓
GameServer
  └─ chat_state에 따라 처리
        ├─ CHAT_NORMAL  -> SessionManager::normal_chat()    // 일반 채팅
        ├─ CHAT_ALL     -> SessionManager::all_chat()       // 전채 채팅
        └─ CHAT_WHISPER -> SessionManager::whisper_chat()   // 귓속말 채팅
        ↓
Target clients
  └─ ACK_SEND_CHAT
```

### Heartbeat

```txt
GameServer
  └─ ACK_SEND_CONNECT_PING(server_tick)
        ↓
Client
  └─ REQ_CONNECT_PONG(server_tick)
        ↓
GameServer
  └─ RTT 기록 및 timeout 상태 갱신
```

## Reliability

서버 안정성을 위해 다음 방어 로직을 추가했습니다.

- packet size 검증
- packet id 검증
- Protobuf parse 실패 처리
- 로그인 전 요청 제한
- 룸 입장 상태 검증
- 채팅 message 길이 검증
- send queue 누적 제한
- disconnect 중복 처리 방지
- heartbeat timeout disconnect

## Config

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
bot_room_count=10
bot_room_target_count=10
all_chat_interval=10
whisper_interval=15

[server]
ip=127.0.0.1
port=7777
```

## 작동 방식

### 1. Clone

```bash
git clone https://github.com/GodDongjin/Server_Portfolio.git
cd Server_Portfolio
```

### 2. 로컬 설정 파일 생성

예시 설정 파일을 복사해 로컬 실행 설정을 만듭니다.

```bash
copy Server\GameServer\Config\config.example.ini Server\GameServer\Config\config.ini
copy TestClient\TestClient\Config\config.example.ini TestClient\TestClient\Config\config.ini
```

필요하면 `ip`, `port`, `session_count`, `worker_thread_count` 등을 로컬 환경에 맞게 수정합니다.

### 3. GameServer 실행

Visual Studio 2022에서 다음 솔루션을 엽니다.

```txt
Server/GameServer.sln
```

`GameServer` 프로젝트를 `Debug x64` 또는 `Release x64`로 실행합니다.

### 4. TestClient 실행

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
/rooms                      방 목록 조회
/enter [room_id]            방 입장
/exit                       방 퇴장
/chat [message]             현재 입장한 방에 채팅 전송
/all [message]              전체 채팅 전송
/whisper [name] [message]   귓속말 전송
/q                          로그아웃 후 종료
```

## 부하 테스트

TestClient의 Load Test Mode를 사용해 1,000개 클라이언트 세션을 생성하고, 룸 채팅/전체 채팅/귓속말이 섞인 채팅 송수신 부하를 측정했습니다.

### Test 환경

```txt
Build: Release x64
Server: GameServer
Client: TestClient Load Test Mode
Protocol: TCP / IOCP
Serialization: Protobuf
Client Count: 1,000
Chat Interval: 2,500ms
```

### 결과

### Release x64 결과

Debug x64 환경에서는 TestClient 1,000개 세션과 수동 접속 1개를 포함해 총 1,001개 세션으로 테스트했습니다

| Item | Result |
|---|---:|
| Build | Debug x64 |
| Connected Sessions | 1,001 |
| Login Success | 1,001 |
| Chat Interval | 2,500ms |
| Total Broadcast Target | 3,411,262 |
| Client Total Recv | 3,411,262 |
| Backlog | 0으로 회복 / 누적 없음 |
| Disconnect | 1001 정상 종료 |

### Release x64 결과

Release x64 환경에서는 TestClient 1,000개 세션으로 테스트했습니다.

| Item | Result |
|---|---:|
| Connected Sessions | 1,000 |
| Login Success | 1,000 |
| Chat Interval | 2,500ms |
| Total Broadcast Target | 7,131,488 |
| Client Total Recv | 7,131,488 |
| Backlog | 0으로 회복 / 누적 없음 |
| Disconnect | 1,000 정상 종료 |

### 부하 테스트 요약

| Clients | Chat Interval | Expected Broadcast Delivery | Result |
|---:|---:|---:|---|
| 1,000 | 5000ms | 약 200K/sec | Stable |
| 1,000 | 3000ms | 약 333K/sec | Stable |
| 1,000 | 2500ms | 약 400K/sec | Stable |
| 1,000 | 2000ms | 약 500K/sec | Backlog Increasing |

### 통계 예시

Client:

```txt
[CLIENT] connect = 1,000 login = 1,000 send = 6,321 recv = 6,264,142 disconnect = 0 send/s = 0 recv/s = 466,323 backlog = 0
```

Server:

```txt
[SERVER] connect = 1,000 login = 1,000 recv_chat = 6,321 broadcast_target = 6,321,000 disconnect = 0
```

## 현재 제한 사항

- 계정 정보는 현재 DB가 아닌 메모리 기반으로 관리됩니다.
- DB 연결 코드는 확장용으로 분리되어 있으며, 현재 로그인 로직에는 연결하지 않았습니다.
- 부하 테스트는 단일 PC 환경 기준입니다.
- graceful shutdown은 추가 개선이 필요합니다.
- Room 생성/삭제 정책은 현재 고정 설정 기반이며, 동적 확장은 향후 개선 항목입니다.

## 다음 개선 사항

- DB 기반 계정 저장
- graceful shutdown
- room 생성/삭제 정책 개선
- JobQueue 기반 room broadcast 최적화
- config 검증 강화
- 운영 로그/에러 로그 정리
- 장시간 soak test 추가
