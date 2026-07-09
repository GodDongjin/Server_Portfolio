# Reliability

이 문서는 서버의 안정성 보강 로직을 정리합니다.

현재 서버는 잘못된 패킷, 비정상 연결 종료, 응답 없는 클라이언트, heartbeat timeout 같은 상황에서 서버 상태가 깨지지 않도록 방어 로직을 둡니다.

## Packet Validation

서버는 패킷을 처리하기 전에 `PacketHeader`와 Protobuf body를 검증합니다.

검증 항목:

- header를 읽을 수 있는 최소 크기인지 확인
- `header.size >= sizeof(PacketHeader)` 확인
- `header.size <= BUFFER_SIZE` 확인
- 현재 수신된 데이터 크기가 `header.size` 이상인지 확인
- packet id에 해당하는 handler 존재 여부 확인
- Protobuf `ParseFromArray()` 성공 여부 확인

목적:

- 잘못된 size로 인한 buffer read 오류 방지
- 등록되지 않은 packet id 처리 방지
- 깨진 Protobuf payload 처리 방지
- 비정상 패킷이 게임 로직까지 들어오는 상황 방지

## Login State Validation

로그인이 필요한 요청은 서버에서 로그인 상태를 확인합니다.

대상 요청:

- room info 조회
- room 입장
- room 퇴장
- 채팅
- 로그아웃

로그인되지 않은 세션이 해당 요청을 보내면 서버는 비정상 요청으로 판단하고 disconnect를 진행합니다.

목적:

- 로그인하지 않은 세션의 게임 로직 접근 방지
- account id가 없는 상태에서 room/chat 로직이 실행되는 상황 방지

## Room State Validation

room 관련 요청은 현재 세션의 room 상태를 기준으로 검증합니다.

검증 항목:

- 이미 room에 입장한 상태에서 다시 입장하려는지 확인
- room에 입장하지 않은 상태에서 퇴장하려는지 확인
- room 채팅 요청의 room_id와 session room_id가 일치하는지 확인

목적:

- 하나의 세션이 여러 room에 동시에 속하는 상태 방지
- 잘못된 room_id로 채팅하는 상황 방지
- disconnect 시 room cleanup을 단순하게 유지

## Chat Validation

채팅 요청은 message, chat_state, target 정보를 검증합니다.

검증 항목:

- message가 비어 있는지 확인
- message 길이가 제한을 넘는지 확인
- whisper의 target_name이 비어 있는지 확인
- whisper 대상 세션이 존재하는지 확인
- 알 수 없는 chat_state인지 확인

목적:

- 과도하게 큰 채팅 payload 방지
- 존재하지 않는 대상에게 귓속말을 보내는 상황 처리
- 잘못된 chat_state로 인한 분기 오류 방지

## Send Queue Limit

각 세션은 send 요청을 `_send_queue`에 쌓습니다.

느린 클라이언트나 비정상 상황에서 send queue가 무한히 증가하면 메모리 사용량이 계속 증가할 수 있습니다. 이를 방지하기 위해 세션별 send queue 누적 수를 제한합니다.

```txt
MAX_SEND_QUEUE_SIZE = 1000
```

동작:

```txt
send 요청
  ↓
_send_queue.size 확인
  ↓
제한 초과 시 disconnect
```

목적:

- 느린 클라이언트로 인한 메모리 증가 방지
- 특정 세션 문제가 전체 서버 문제로 번지는 상황 방지
- 서버 전체 안정성 보호

## Heartbeat Timeout

Heartbeat는 서버 주도 방식입니다.

```txt
Server -> Client : ACK_SEND_CONNECT_PING
Client -> Server : REQ_CONNECT_PONG
```

서버는 ping을 보낸 뒤 `waiting_pong` 상태로 전환합니다. 제한 시간 안에 pong이 오지 않으면 해당 세션을 비정상으로 판단하고 disconnect합니다.

관리 상태:

```txt
_last_ping_tick
_last_pong_tick
_waiting_pong
```

동작:

```txt
waiting_pong == true
  ↓
now - last_ping_tick > timeout
  ↓
heartbeat timeout
  ↓
disconnect
```

목적:

- 비정상 클라이언트 감지
- 끊어진 연결 정리
- 응답 없는 연결 정리

## Disconnect Trigger

disconnect는 여러 상황에서 시작될 수 있습니다.

대표 트리거:

- 클라이언트 연결 종료
- recv 0 byte 수신
- heartbeat timeout
- send queue overflow
- socket error

이 트리거들은 모두 최종적으로 session cleanup으로 이어집니다.

## Disconnect Duplicate Guard

멀티스레드 환경에서는 여러 경로에서 동시에 disconnect가 호출될 수 있습니다.

예를 들어 recv 0 byte 처리와 heartbeat timeout이 거의 동시에 발생하면 cleanup이 중복 실행될 위험이 있습니다.

이를 방지하기 위해 atomic flag를 사용합니다.

```txt
_is_connect
_is_disconnect
```

동작:

```txt
disconnect()
  ↓
_is_connect.exchange(false)
  ↓
이미 false면 return
  ↓
process_disconnect()
  ↓
_is_disconnect.exchange(true)
  ↓
이미 true면 return
```

목적:

- 중복 disconnect 방지
- room/login/session cleanup 중복 실행 방지
- 멀티스레드 환경에서 상태 일관성 유지

## Disconnect Cleanup

disconnect가 확정되면 `GameSession::on_disconnect()`와 `SessionManager::on_disconnected()`를 통해 세션 상태를 정리합니다.

정리 대상:

- room 입장 상태
- login 상태
- account session map
- name session map
- 전체 session set
- send 상태

흐름:

```txt
Session::disconnect()
  ↓
Session::process_disconnect()
  ↓
GameSession::on_disconnect()
  ↓
RoomManager::exit_room()
  ↓
Login::logout()
  ↓
SessionManager::on_disconnected()
  ↓
release_session()
```

목적:

- 연결 종료 후 room에 유저가 남는 상황 방지
- 로그인 중복 상태 방지
- 귓속말 name map에 끊어진 세션이 남는 상황 방지

## SessionManager Snapshot Broadcast

전체 채팅이나 broadcast 계열 작업은 세션 목록을 순회합니다.

세션 목록을 lock 상태로 오래 순회하면 다른 세션 추가/삭제 작업이 지연될 수 있습니다. 이를 줄이기 위해 현재 세션 목록을 임시 vector로 복사한 뒤 lock을 해제하고 send를 수행합니다.

```txt
lock
  ↓
sessions snapshot 생성
  ↓
unlock
  ↓
snapshot 순회하며 send
```

목적:

- lock hold time 감소
- broadcast 중 세션 추가/삭제 지연 완화
- disconnect와 broadcast 간 충돌 가능성 감소

## Reliability Summary

| 상황 | 대응 |
|---|---|
| 잘못된 packet size | 패킷 처리 중단 또는 disconnect |
| 등록되지 않은 packet id | handler 없음 처리 |
| Protobuf parse 실패 | 패킷 처리 실패 |
| 로그인 전 요청 | disconnect |
| 잘못된 room 상태 | 실패 ACK 응답 |
| 존재하지 않는 whisper 대상 | `CHAT_ERROR_NOT_TARGET` 응답 |
| send queue 과다 누적 | disconnect |
| heartbeat timeout | disconnect |
| 중복 disconnect | atomic flag로 방지 |

## Notes

이 안정성 로직은 서버가 모든 장애를 완전히 복구한다는 의미는 아닙니다.

현재 목적은 다음을 보장하는 것입니다.

- 잘못된 요청이 서버 상태를 망가뜨리지 않게 한다.
- 비정상 세션을 정리한다.
- disconnect cleanup을 중복 실행하지 않는다.
- 부하 상황에서 특정 세션의 문제가 전체 서버로 번지지 않게 한다.
