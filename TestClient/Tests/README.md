# 클라이언트 송신 회귀 테스트

`SendEvent`는 송신 중인 버퍼와 버퍼별 완료 위치를 유지합니다.
`TestSession`은 완료 바이트 수만큼 위치를 이동하고, 남은 구간을 먼저 송신한
뒤 다음 큐 데이터를 처리합니다. 공유된 송신 버퍼의 내용과 길이는 바꾸지 않습니다.

송신 등록·완료 처리와 소켓 종료는 기존 `_send_lock`으로 보호합니다.
종료 시 대기 큐는 비우되 진행 중인 비동기 송신 버퍼는 완료 통지가 올 때까지
유지합니다. 즉시 송신 실패는 그 자리에서 버퍼를 해제하고 연결을 종료합니다.
IOCP 실패 완료도 세션에 전달해 버퍼를 정리하며, 수신 등록 실패로 송신 플래그를
잘못 해제하지 않도록 연결 종료로 처리합니다.

저장소 루트의 Visual Studio 개발자 PowerShell에서 실행합니다.

```powershell
msbuild TestClient\Tests\ClientSendTests.vcxproj /p:Configuration=Debug /p:Platform=x64
& .\TestClient\Tests\bin\Debug\ClientSendTests.exe
```

v145만 설치된 환경에서는 빌드 명령에 `/p:PlatformToolset=v145`를 추가합니다.

테스트는 제품의 `TestSession.cpp`와 `ClientServer.cpp`를 그대로 포함하고,
테스트 실행 파일 안에서만 Winsock 호출 및 IOCP 완료 결과를 주입합니다.
따라서 부분 완료가 우연히 발생하기를 기다리지 않고 다음 동작을 확인합니다.

- 여러 버퍼에 걸친 부분 송신, 정확한 버퍼 경계, 다음 묶음의 위치 초기화
- 미송신 구간을 먼저 처리하고 나중에 등록한 패킷의 순서를 유지하는지
- 0바이트·잘못된 완료 크기·즉시 송신 실패·재송신 중 실패 처리
- 수신 등록 실패 및 연결 종료 시 대기 버퍼와 진행 중인 버퍼의 수명
- 실패한 IOCP 완료가 실제 `ClientServer` 작업 스레드에서 정리되는지
- 동기 성공도 완료 통지를 받은 뒤 버퍼를 해제하는지

마지막 검사는 실제 Winsock과 IOCP 작업 스레드 2개로 loopback에 2MiB를 보내고,
받은 전체 바이트와 순서가 일치하는지 확인합니다. 기존 서버나 설정은 사용하지 않습니다.
