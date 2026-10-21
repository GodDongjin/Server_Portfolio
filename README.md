# C++ IOCP Game Server Portfolio

Windows IOCP 기반 비동기 TCP 게임 서버 포트폴리오입니다.

`ServerCore`는 IOCP, 세션, 송수신 버퍼, 스레드, 설정 로딩 등 서버 공통 구조를 담당합니다.  
`GameServer`는 로그인, 룸 입장/퇴장, 룸 채팅, 전체 채팅, 귓속말, heartbeat 기반 연결 상태 관리를 구현합니다.  
`TestClient`는 수동 테스트와 다중 세션 부하 테스트를 지원합니다.

## 주요 검증 결과

- **1,000명 · 500ms · 10분 부하:** 실제·예상 수신 **207,460,363건 일치**, 최종 미수신 **0건**.
- **접속·복구:** 같은 계정 1,000명 재접속 10회, 2,000명 정원에서 추가 34개 연결 거절, 강제 종료 후 재로그인 검증 통과.
- **정상 종료:** 1,000명 접속 중 서버 종료 시 연결·로그인·종료 각 1,000개 확인.

동일 PC의 Release x64 검증 결과입니다. 측정 조건과 로깅 제거 전후의 검증 시점은
[최근 테스트 결과](#최근-테스트-결과-2026-10-21)에 정리했습니다.

## 목차

[핵심 기능](#핵심-기능) · [기술 스택](#기술-스택) · [프로젝트 구조](#프로젝트-구조) · [구성 요소](#구성-요소)

[패킷 흐름](#패킷-흐름) · [안정성](#안정성) · [설정](#설정) · [실행 방법](#실행-방법) · [패킷 재생성](#패킷-코드-재생성)

[최근 테스트](#최근-테스트-결과-2026-10-21) · [이전 기록](#이전-부하-테스트-기록) · [제한 사항](#현재-제한-사항) · [개선 계획](#다음-개선-사항)

## 핵심 기능

- **비동기 네트워크:** Windows IOCP와 `Session`·`Listener`·`Service`·`SessionManager` 기반 연결 관리
- **패킷 송수신:** Protobuf 직렬화, `RecvBuffer`·`SendBuffer`, 부분 송신 처리
- **게임 기능:** 로그인·로그아웃, 룸 조회·입퇴장, 룸 채팅·전체 채팅·귓속말
- **세션 동시성:** 세션 JobQueue를 통한 패킷 처리와 연결 정리 직렬화
- **안정성:** 패킷 크기·ID 검사, 송신 큐 누적 제한, heartbeat timeout 연결 해제
- **연결 수명:** JobTimer 기반 Accept 재시도, 정원 초과 안내 후 연결 종료, `/q`·EOF 기반 정상 종료
- **테스트·설정:** 수동·다중 세션 부하 테스트, 서버·클라이언트 통계 출력, ini 설정 분리와 값 검증

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

![Server Architecture](docs/images/server-architecture.png)

## 구성 요소

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

## 패킷 흐름

상세 패킷 흐름은 [docs/packet-flow.md](docs/packet-flow.md)에 정리했습니다.

요약:

- Login: `REQ_LOGIN / REQ_BOT_LOGIN` 요청을 검증하고 `ACK_LOGIN / ACK_BOT_LOGIN`으로 결과를 응답합니다.
- Room: 룸 목록 조회, 입장, 퇴장 요청을 `RoomManager`에서 처리합니다.
- Chat: 요청 결과(`ACK_CHAT`)와 실제 메시지 전달(`ACK_SEND_CHAT`)을 분리합니다.
- Heartbeat: 서버가 ping을 보내고 클라이언트 pong 응답으로 연결 생존 여부를 확인합니다.


## 안정성

상세 안정성 보강 내용은 [docs/reliability.md](docs/reliability.md)에 정리했습니다.

주요 방어 로직:

- packet size / packet id 검증
- Protobuf parse 실패 처리
- 로그인 전 요청 제한
- 룸 입장 상태 검증
- 채팅 message 길이 검증
- send queue 누적 제한
- disconnect 중복 처리 방지
- heartbeat timeout disconnect


## 설정

실행 설정은 `config.ini`로 분리합니다. 실제 로컬 설정 파일은 Git에 포함하지 않고, 예시 파일만 커밋합니다.

```txt
config.example.ini  # Git에 포함
config.ini          # 로컬 실행용, Git 제외
```

### GameServer 설정

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

### TestClient 설정

`TestClient/TestClient/Config/config.example.ini`

```ini
[client]
session_count=1000
bot_thread_count=4
worker_thread_count=4
chat_interval_ms=2500
bot_room_count=10
all_chat_interval=10
whisper_interval=15

[server]
ip=127.0.0.1
port=7777
```

## 실행 방법

### 1. 저장소 받기

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

Load Mode에서도 `/q` 또는 입력 종료로 테스트를 끝낼 수 있습니다.
봇 작업을 멈추고 연결을 닫은 뒤, 취소된 송수신의 완료 통지까지 처리하고 IOCP 스레드를 종료합니다.
수동 로그인 입력과 재입력은 메인 스레드에서 처리하므로 입력 대기 중에도 핑에 응답합니다.

클라이언트 설정은 연결 전에 검사합니다. 포트는 1~65,535, 세션 수는 1~100,000,
IOCP·봇 스레드 수는 각각 1~256, 채팅 주기는 1~60,000ms, 봇 방 수는 1~256입니다.
전체 채팅·귓속말 간격은 양의 정수여야 하며 숫자 뒤의 불필요한 문자도 오류로 처리합니다.
`bot_room_count`는 실제 서버의 방 구성에 맞추어 설정해야 합니다.

### 5. 서버 정상 종료

서버 콘솔에 `/q`를 입력하면 새 Accept와 재시도를 중단하고, 접속 중인 세션 및
정원 초과 안내를 송신하던 임시 세션의 소켓을 닫습니다. IOCP 작업 스레드는
취소된 IO 완료 통지와 세션 JobQueue의 방 퇴장·로그아웃·등록 해제를 처리한 뒤 종료합니다.
하트비트와 통계 스레드의 대기는 종료 신호로 깨우며, 최종 누적 통계는 콘솔에 표시합니다.

표준 입력 EOF도 같은 종료 경로를 사용합니다. 자동 실행에서는 입력 파이프를 열어 두고
종료할 때 `/q` 또는 EOF를 전달합니다. 콘솔 창 강제 닫기·프로세스 강제 종료는
이 절차의 대상이 아니며, 종료 직전 채팅의 상대방 수신까지 보장하지 않습니다.

```powershell
python Server/Tests/server_shutdown_test.py `
  Server/Binaries/Release/GameServer.exe `
  --client TestClient/x64/Release/TestClient.exe
```

빈 서버의 `/q`·EOF, 단일 작업 스레드에서 연결 정리, 정원 초과 및 접속 시도와
종료의 경합, 실제 테스트 클라이언트 1,000명 접속 중 종료를 검사합니다.
임시 설정과 별도 loopback 포트를 사용하며 기존 실행 중인 서버에는 접속하지 않습니다.

## 패킷 코드 재생성

일반적인 C++ 빌드에는 Python이 필요하지 않습니다. `.proto` 또는 패킷 핸들러 템플릿을
수정할 때만 Python 3.10 이상과 아래 의존성을 설치합니다. 다음 명령은 저장소 루트 기준입니다.

```powershell
python -m pip install -r Server/Tool/PacketGenerator/requirements.txt
python Server/Tool/PacketGenerator/PacketGenerator.py --target all
```

- `--target server`, `--target client`: 한쪽 코드만 재생성합니다.
- `--check`: 파일을 수정하지 않고 현재 생성 코드와 일치하는지 검사합니다. 다르면 종료 코드 1을 반환합니다.
- 기존 `Server/Common/protoc-21.12-win64/bin/GenPackets.bat`과
  `TestClient/Common/protoc-28.1-win64/bin/protocol.bat`도 같은 Python 도구를 실행합니다.

서버와 클라이언트의 `.proto`는 각각 `Server/Common/protoc-21.12-win64/bin`과
`TestClient/Common/protoc-28.1-win64/bin`에 있습니다. 프로토콜을 변경할 때 두 곳을 함께 수정합니다.
두 폴더의 `protoc.exe`는 모두 21.12 버전이며, 클라이언트 폴더의 `28.1`은 기존 디렉터리 이름입니다.

핸들러 템플릿은 `Server/Tool/PacketGenerator/Templates`에서 관리합니다.
서버 템플릿에는 세션 JobQueue 처리, 클라이언트 템플릿에는 수신 패킷과 송신 크기 검사가 포함됩니다.
생성 대상 전체를 임시 디렉터리에서 먼저 생성하므로, 생성에 실패하면 프로젝트 파일은 갱신하지 않습니다.
도구가 스크립트 위치를 기준으로 경로를 찾기 때문에 실행 디렉터리에 영향을 받지 않습니다.

## 기능 검증

- [서버 테스트](Server/Tests/README.md): 세션 작업 큐, Accept 재시도, 송수신 실패,
  로그인/재접속, 채팅 대상 수, 정원 초과, 정상 종료를 검증합니다.
- [테스트 클라이언트 테스트](TestClient/Tests/README.md): 부분 송신, 패킷 크기 제한,
  설정 검증, 수동/부하 모드 종료와 연결 정리를 검증합니다.

## 최근 테스트 결과 (2026-10-21)

### 부하 및 복구 검증

- **환경:** 동일 PC, Release x64 서버·TestClient, 별도 loopback 포트
- **서버:** IOCP 작업 스레드 4개, Accept 대기 32개, 최대 접속 2,000명
- **클라이언트:** IOCP·봇 작업 스레드 각각 4개, 방 10개

아래 네 가지 검증은 2026-10-21 16:47~17:02에 진행한 결과입니다.
당시에는 상세 로깅 제거 전 코드에서 서버 WARN 및 통계 로그를 활성화했습니다.
이후 로깅 제거와 패킷 생성 도구 정리 후 수행한 검증은 아래에 별도로 기록했습니다.

**네 가지 시나리오 모두 통과했습니다.**

| 테스트 | 조건 |
|---|---|
| 재접속 | 1,000명 × 10회 |
| 채팅 부하 | 1,000명 · 500ms · 10분 |
| 정원 초과 | 2,000명 + 추가 34개 연결 |
| 강제 종료 | 1,000명 종료 후 재접속 |

- **동일 계정 반복 재접속:** 1,000명 × 10회, 회당 약 15초, 채팅 간격 1,000ms. 매회 로그인·정상 종료 성공. 총 송신 142,000건, 실제/예상 수신 26,516,086건 일치.

- **연속 채팅 부하:** 1,000명, 채팅 간격 500ms, 10분. 서버 채팅 수신 1,111,000건. 실제/예상 수신 207,460,363건 일치. 부하 중 연결 종료 0건.

- **최대 접속 수 초과:** 기존 2,000명 접속 중 추가 34개 연결. 프로토콜 클라이언트 33개는 만원 안내 전체 수신 후 EOF 확인. 실제 TestClient 1개도 한글 안내 후 종료. 기존 연결 유지 및 빈자리 재접속 성공.

- **비정상 종료 후 복구:** 1,000명 접속 중 클라이언트 프로세스 강제 종료. 서버 생존 및 1,000개 연결 정리 확인. 같은 계정 1,000개 재로그인 성공. 재접속 후 실제/예상 수신 2,614,262건 일치.

정원 초과 검증은 거절한 TCP 연결도 누적 연결·종료 통계에 포함되도록 테스트 판정을
수정한 뒤 서버 코드 변경 없이 재검증했습니다. 강제 종료는 테스트 소유 프로세스에
`TerminateProcess`를 호출해 정상 종료 절차 없이 연결이 끊기는 상황을 재현했습니다.
정원 초과 시 `SERVER_FULL` 경고 34건, 강제 종료 시 연결 종료 오류 1,000건은 예상된 결과였으며,
그 외 오류는 없었습니다. 반복 재접속과 10분 부하 테스트의 WARN/ERROR는 모두 0건이었습니다.

### 1,000명 · 500ms · 10분 상세 결과

룸 채팅·전체 채팅·귓속말을 함께 사용했으며 `all_chat_interval=10`, `whisper_interval=15`로 설정했습니다.
예상 수신량은 성공한 `ACK_CHAT.target_count`를 누적해 계산했습니다.

| 측정 항목 | 결과 |
|---|---:|
| 클라이언트 채팅 송신 = 서버 채팅 수신 | 1,111,000건 |
| 서버 전달 대상 = 클라이언트 실제 수신 = 예상 수신 | 207,460,363건 |
| 부하 실행 중 연결 종료 | 0건 |
| 최종 미수신 건수 | 0건 |
| 미수신 건수가 남아 있던 통계 표본 | 598개 중 31개 |
| 최대 관측 미수신 건수 | 15,424건 |
| 미수신 건수 발생 후 0 복귀까지 최장 시간 | 2.018초 |
| 로그인 직후 서버 전용 메모리 | 658.11MiB |
| 부하 중 서버 전용 메모리 최대 | 670.91MiB |
| 후반 5분 서버 전용 메모리 범위 | 670.84~670.91MiB |
| 클라이언트 종료·연결 정리 후 서버 전용 메모리 | 37.15MiB |

미수신 건수의 복귀 시간은 약 1초 간격의 통계 샘플 기준이며 개별 패킷의 지연시간 측정값은 아닙니다.
재접속 10회 검증에서도 종료 후 전용 메모리는 최근 5회 기준 41.29~46.27MiB에서 변동했습니다.
위 수치는 해당 시간·횟수에서 관찰한 결과이며, 원격 네트워크 환경이나 수 시간 이상의 장기 부하는 포함하지 않습니다.

### 상세 로깅 제거 후 정상 종료 검증

Release x64 실행 파일로 `/q`와 입력 EOF, 연결 처리 중 종료를 다시 검증했습니다.
아래 모든 시나리오에서 서버 종료 코드 0과 콘솔의 정상 종료 완료 표시를 확인했습니다.

- **빈 서버 종료:** `/q` 및 입력 EOF. 두 경로 모두 정상 종료.

- **단일 작업 스레드 정리:** IOCP 작업 스레드 1개, 로그인·방 입장한 32개 세션. 연결 32개 / 로그인 32개 / 종료 32개, 클라이언트 소켓 종료 확인.

- **신규 접속·정원 초과와 종료 경합:** 정원 4명, 추가 연결·만원 안내·패킷 수신 중 종료. 관측 연결 8개 / 로그인 4개 / 종료 8개, 종료 완료.

- **부하 중 서버 종료:** 실제 TestClient 1,000명, 채팅 간격 500ms. 연결·로그인·종료 각 1,000개. 해당 실행에서 종료 소요 약 0.14초.

정상 종료는 취소된 IO 완료와 세션 작업 큐의 정리를 기다리는지 확인합니다.
종료 직전에 송신한 채팅을 상대방이 모두 수신했다는 의미는 아닙니다.
재실행 방법은 [서버 정상 종료 테스트](Server/Tests/README.md#서버-정상-종료)에 정리했습니다.

### 빌드 및 기능 회귀 검증

같은 날 로깅 제거 후 네이티브·통합 테스트를 통과했습니다.
패킷 생성 도구 정리 후에는 두 솔루션 빌드, 로그인·채팅 통합 테스트와 클라이언트 송수신 테스트를 다시 통과했습니다.

- **서버·클라이언트 솔루션 빌드:** Release x64, 설치된 MSVC v145로 빌드. 양쪽 모두 경고 0개 / 오류 0개.

- **세션 작업 큐:** 생산자 4개·작업 스레드 4개, 작업 32,000개의 세션별 직렬 실행·FIFO·수명 검증 통과.

- **로그인·재접속:** 연속 로그인/로그아웃 패킷 120개, 즉시 재접속 25개 세션, 클라이언트 8개의 총 160회 로그인 주기 검증 통과.

- **채팅 대상 수:** 방 입퇴장·연결 종료·전체 채팅·귓속말·실패 요청·대상 0명 요청의 ACK 대상 수 검증 통과.

- **클라이언트 송수신:** 부분 송신·실패 처리, 실제 Winsock 2MiB 전송의 데이터·순서 일치, 0·1·4개 IOCP 작업 스레드에서 총 15회 종료, 65,535바이트 패킷 경계 검증 통과.

- **클라이언트 실행 흐름:** 잘못된 설정 21개 거절, 로그인 재입력 중 핑 응답, 16개 봇의 3회 접속·종료·계정 재사용 검증 통과.

- **패킷 생성 도구:** 서버·클라이언트 재생성 후 기존 처리 로직과 패킷 ID 유지. 다른 실행 경로·한글/공백 경로, 변경 감지, 생성 실패 시 기존 파일 보존, 반복 실행 검증 통과.

기능 테스트의 실행 명령과 검증 범위는 [서버 테스트](Server/Tests/README.md)와
[테스트 클라이언트 테스트](TestClient/Tests/README.md)에서 확인할 수 있습니다.

## 이전 부하 테스트 기록

아래는 초기 구현에서 기록한 측정값입니다. 위의 2026-10-21 추가 검증과 구분해 보관합니다.

<details>
<summary>이전 Debug·Release 측정값과 통계 보기</summary>

TestClient의 Load Test Mode를 사용해 1,000개 클라이언트 세션을 생성하고, 룸 채팅/전체 채팅/귓속말이 섞인 채팅 송수신 부하를 측정했습니다.

### 테스트 환경

```txt
Build: Release x64
Server: GameServer
Client: TestClient Load Test Mode
Protocol: TCP / IOCP
Serialization: Protobuf
Client Count: 1,000
Chat Interval: 2,500ms
```

### Debug x64 결과

Debug x64 환경에서는 TestClient 1,000개 세션과 수동 접속 1개를 포함해 총 1,001개 세션으로 테스트했습니다

| 항목 | 결과 |
|---|---:|
| 빌드 구성 | Debug x64 |
| 접속 세션 수 | 1,001 |
| 로그인 성공 수 | 1,001 |
| 채팅 간격 | 2,500ms |
| 누적 전송 대상 수 | 3,411,262 |
| 클라이언트 누적 수신량 | 3,411,262 |
| 미수신 건수 | 0으로 회복 / 누적 없음 |
| 연결 종료 | 1001 정상 종료 |

### Release x64 결과

Release x64 환경에서는 TestClient 1,000개 세션으로 테스트했습니다.

| 항목 | 결과 |
|---|---:|
| 접속 세션 수 | 1,000 |
| 로그인 성공 수 | 1,000 |
| 채팅 간격 | 2,500ms |
| 누적 전송 대상 수 | 7,131,488 |
| 클라이언트 누적 수신량 | 7,131,488 |
| 미수신 건수 | 0으로 회복 / 누적 없음 |
| 연결 종료 | 1,000 정상 종료 |

### 부하 테스트 요약

| 클라이언트 수 | 채팅 간격 | 예상 브로드캐스트 처리량 | 결과 |
|---:|---:|---:|---|
| 1,000 | 5000ms | 약 200K/sec | 안정적 |
| 1,000 | 3000ms | 약 333K/sec | 안정적 |
| 1,000 | 2500ms | 약 400K/sec | 안정적 |
| 1,000 | 2000ms | 약 500K/sec | 미수신 건수 증가 |

### 통계 예시

Client:

```txt
[CLIENT] connect = 1,000 login = 1,000 send = 6,321 recv = 6,264,142 disconnect = 0 send/s = 0 recv/s = 466,323 backlog = 0
```

Server:

```txt
[SERVER] connect = 1,000 login = 1,000 recv_chat = 6,321 broadcast_target = 6,321,000 disconnect = 0
```

</details>

## 현재 제한 사항

- 계정 정보는 현재 DB가 아닌 메모리 기반으로 관리됩니다.
- DB 연결 코드는 확장용으로 분리되어 있으며, 현재 로그인 로직에는 연결하지 않았습니다.
- 부하 테스트는 단일 PC 환경 기준입니다.
- Room 생성/삭제 정책은 현재 고정 설정 기반이며, 동적 확장은 향후 개선 항목입니다.

## 다음 개선 사항

- DB 기반 계정 저장
- room 생성/삭제 정책 개선
- JobQueue 기반 room broadcast 최적화
- 운영 로그/에러 로그 정리
- 수 시간 이상의 장기 soak test 및 원격 네트워크 환경 검증
