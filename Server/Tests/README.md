# 세션 작업 큐

`Session::post_logic()`는 해당 세션의 큐에 작업을 넣습니다. 기존 IOCP 작업
스레드가 `ThreadManager::do_global_queue_work()`에서 실행하므로 세션마다
전용 스레드를 만들지 않습니다. 같은 세션의 작업은 동시에 실행되지 않지만,
각 작업을 실행하는 스레드는 달라질 수 있습니다.

- 로그인 상태·계정 ID·이름·방 변경은 해당 세션의 큐에서 처리합니다.
  계정 ID와 이름을 읽을 때도 같은 규칙을 지킵니다.
- 패킷은 수신 버퍼에서 먼저 파싱한 뒤, 파싱 결과와 세션 `shared_ptr`를
  값으로 캡처합니다. `HandlePacket()`의 성공은 파싱·큐 등록 성공을 뜻합니다.
- 연결 종료는 먼저 연결 상태를 false로 바꿉니다. 아직 실행되지 않은 패킷은
  건너뛰고, 방 퇴장·로그아웃·SessionManager 등록 해제는 같은 큐에서 처리합니다.
  이미 실행 중인 핸들러가 끝난 다음 정리가 실행됩니다.
- `post_logic()`는 서버 종료가 시작된 세션의 새 일반 작업을 거절합니다.
  연결 정리는 별도 내부 경로로 반드시 등록하며, 일반 연결 해제 후의 패킷은 실행 시 연결 상태를 검사합니다.
- 다른 세션이나 heartbeat에서 조회하는 방 ID·방 인원·연결 상태 등은 atomic을
  사용합니다. 공유 Room/Login/SessionManager의 기존 잠금은 유지합니다.

## 회귀 테스트

저장소 루트의 Visual Studio 개발자 PowerShell에서 실행합니다.

```powershell
$serverDir = Join-Path (Get-Location) 'Server\'
msbuild Server\GameServer.sln /t:GameServer /p:Configuration=Debug /p:Platform=x64
msbuild Server\Tests\SessionQueueTests.vcxproj /p:Configuration=Debug /p:Platform=x64 "/p:SolutionDir=$serverDir"
& .\Server\Tests\bin\Debug\SessionQueueTests.exe
python Server\Tests\session_lifecycle_test.py Server\Binaries\Debug\GameServer.exe
```

기존 프로젝트와 테스트 프로젝트의 기본 도구 집합은 v143입니다. v145만
설치되어 있으면 두 `msbuild` 명령에 `/p:PlatformToolset=v145`를 추가합니다.

네이티브 테스트는 4개 생산자·4개 작업 스레드에서 32,000개 작업의 세션별
직렬 실행과 FIFO 순서, 다른 세션의 독립적인 진행, 중첩 작업 등록,
세션 수명, 이전 세션의 이름 등록 해제를 검사합니다.

Python 테스트는 표준 라이브러리만 사용합니다. 임시 설정과 임시 loopback
포트로 별도 서버를 실행해 패킷 분할·연속 로그인/로그아웃·중복 로그인 차단·
종료 후 재접속·동시 방 입퇴장을 검사하고, 자신이 실행한 서버를 종료합니다.
평소 사용하는 `config.ini`는 변경하지 않습니다.


## 접속 정원과 거절 안내

`session_max_count`는 로그인 여부와 관계없이 입장한 연결 수를 제한합니다.
정원 확인과 세션 등록은 같은 잠금에서 처리합니다. 초과 연결은 정상 목록에
등록하지 않고 수신도 시작하지 않으며, `ACK_SERVER_FULL`(1017)을 보냅니다.
이 패킷은 기존 번호를 유지하도록 프로토콜 끝에 추가했습니다.

송신 이벤트는 버퍼별 완료 위치를 추적합니다. 일부만 송신되면 남은 구간을
이어서 보내고, 안내 패킷 전체의 송신 완료 후 연결을 끊습니다. 송신 오류가
나면 완료를 더 기다리지 않고 종료합니다. 이는 클라이언트 응답 확인 절차는
아니며, 이미 연결을 끊은 상대에게 안내 수신을 보장하지는 않습니다.

클라이언트는 정원 초과 안내와 최대 인원을 표시하고 연결을 종료합니다.
서버와 클라이언트를 함께 빌드한 후 다음 테스트를 실행할 수 있습니다.

```powershell
msbuild TestClient\TestClient.sln /p:Configuration=Debug /p:Platform=x64
python Server\Tests\server_capacity_test.py Server\Binaries\Debug\GameServer.exe TestClient\x64\Debug\TestClient.exe
```

v145만 설치된 환경에서는 위 빌드 명령에도 `/p:PlatformToolset=v145`를 추가합니다.
네이티브 테스트에는 부분 송신과 버퍼 경계·수명 검증이 포함됩니다.
정원 테스트는 별도 서버의 정원을 2명으로 제한해 안내 패킷 전체 수신 후 EOF,
동시 초과 접속, 거절된 로그인 미실행, 빈자리 재접속, 실제 클라이언트의
한글 안내와 종료를 확인합니다. 임시 설정만 사용하고 실행한 자식 프로세스만 종료합니다.


## Accept 지연 재시도

`Listener`는 `JobQueue::do_timer()`로 100ms 뒤 Accept 재등록을 예약합니다.
기존 작업 스레드가 예약 작업을 실행하며, 재귀 호출이나 스레드 대기는 하지 않습니다.
재시도 간격은 `Listener::ACCEPT_RETRY_INTERVAL_MS`에서 조정할 수 있습니다.

- 세션 생성 실패는 지연 재시도합니다. `Service::create_session()`에서도
  세션과 소켓을 확인한 뒤 사용합니다.
- `AcceptEx`가 즉시 실패한 경우 `WSAENOBUFS`, `WSAEMFILE`, `WSAECONNRESET`,
  `WSAECONNABORTED`만 재시도합니다. 그 외 오류는 코드를 기록하고 해당 슬롯을 중단합니다.
- `WSA_IO_PENDING`과 동기 성공은 완료 통지를 기다리며 중복 재등록하지 않습니다.
- 즉시 실패하면 이벤트의 세션과 리스너 참조를 해제합니다. 예약은 약한 참조를
  사용하며, 진행 중인 Accept만 완료 통지까지 강한 참조를 유지합니다.
- `close_socket()`과 Accept 등록·완료 처리는 같은 잠금으로 보호합니다.
  종료 후 실행된 타이머는 다시 Accept를 등록하지 않습니다.

기존 네이티브 테스트에 `AcceptRetryTests.cpp`를 포함했습니다.
실제 소켓과 IOCP를 준비하고 `AcceptEx` 반환값을 주입해, 100ms 이전 재시도 방지,
반복 실패, 정상 대기·동기 성공의 중복 등록 방지, 세션 생성 실패 후 복구,
리스너 종료·소멸 후 예약 취소를 검사합니다. 타이머 검사 시각을 전달하므로
실제 대기 없이 실행하며, 로그는 테스트가 만든 임시 디렉터리에만 저장합니다.


## 수신 등록 실패와 채팅 대상 수

`ServerReceiveTests.cpp`는 실제 `Session` 구현의 WSARecv 반환값만 주입한다.
일반 오류와 연결 종료 오류에서 연결 상태, 이벤트 참조, 계정·이름 등록,
정원 반환 및 한 번만 실행되는 종료 정리를 검사한다.
동기 성공과 `WSA_IO_PENDING`은 완료 전까지 정상 연결로 유지되는지도 확인한다.

`ACK_CHAT.target_count`는 해당 요청을 처리할 때 서버가 선택한 전송 대상 수다.
실제 수신 완료나 전달 보장을 뜻하지 않는다. 클라이언트는 성공 응답의 값만
예상 수신량에 누적하며, 제거한 `bot_room_target_count` 설정은 사용하지 않는다.
ACK와 채팅 도착 순서가 다를 수 있어 실행 중 통계는 순간적으로 차이가 날 수 있다.
여러 부하 클라이언트를 함께 쓰면 각 프로세스의 수신·예상 수신량을 합산해야 한다.

```powershell
python Server\Tests\chat_target_count_test.py Server\Binaries\Debug\GameServer.exe TestClient\x64\Debug\TestClient.exe
```

테스트는 임시 서버와 설정으로 방 입퇴장·로그아웃·연결 종료, 전체 채팅,
귓속말, 실패 요청, 대상 0명인 요청을 확인한다. 실제 부하 클라이언트의
성공 ACK 집계가 수신량과 일치하고 이전 인원 설정값을 무시하는지도 검사한다.
서버와 클라이언트를 새 프로토콜로 함께 빌드한 뒤 실행한다.

## 서버 정상 종료

`server_shutdown_test.py`는 빈 서버의 `/q`와 EOF, 작업 스레드 1개에서 32개 로그인/방 상태 정리,
Accept·정원 초과 안내·패킷 수신과 종료의 경합을 검사합니다. `--client`를 지정하면
실제 TestClient 1,000명이 500ms 간격으로 채팅하는 중 서버를 종료하는 검사도 실행합니다.
프로세스 종료 코드, 연결/종료 누적 통계 일치, 실제 소켓 연결 해제와 콘솔의 종료 완료 표시를 확인합니다.
`--output`에 새 디렉터리를 지정하면 테스트 설정과 콘솔 출력을 보관합니다.

```powershell
python Server/Tests/server_shutdown_test.py Server/Binaries/Release/GameServer.exe --client TestClient/x64/Release/TestClient.exe
```

네이티브 수신 테스트는 DisconnectEx가 아직 완료되지 않은 시점에 종료를 요청하고,
이벤트 소유자가 유지되는지, IO 완료와 JobQueue 정리를 모두 기다리는지,
중복 종료의 정리가 한 번만 실행되는지, 종료 후 새 세션/패킷 작업을 거절하는지도 검사합니다.
기존 Python 테스트는 서버 표준 입력 파이프를 유지해 시작 직후 EOF로 종료되지 않게 합니다.
