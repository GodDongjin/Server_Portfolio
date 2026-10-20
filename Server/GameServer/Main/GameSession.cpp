#include "pch.h"
#include "../Main/GameSession.h"
#include "../Packet/ServerPacketHandler.h"
#include "../Utils/GameGlobal.h"
#include "../Login/Login.h"
#include "../Room/RoomManager.h"

void GameSession::on_disconnect()
{
	GServerStats.disconnect++;

	// Session::process_disconnect에서 이미 해당 세션의 작업 큐에 등록해 호출한다.
	// 여기서 다시 큐에 넣지 않고 방 퇴장과 로그아웃을 순서대로 처리한다.
	const uint64 account_id = get_account_idx();
	const int32 room_id = get_room_id();

	if (room_id >= 0)
	{
		RoomManager::get_instance()->exit_room(room_id, account_id);
		set_room_id(-1);
	}

	// 방 정리가 끝난 뒤 로그인 점유를 해제해야 같은 계정의 재접속과 충돌하지 않는다.
	if (_is_login.exchange(false))
	{
		GLogin->logout(account_id);
	}
}

void GameSession::on_send(int32 len)
{
	GServerStats.send_complete++;
	GServerStats.send_complete_bytes += len;
}

void GameSession::on_recv_packet(BYTE* buffer, int32 len)
{
	GameSessionRef session = get_game_session();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	if (!ServerPacketHandler::HandlePacket(session, buffer, len))
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		wcout << L"HandlePacket ERROR - ID : " << header->id << endl;
	}
}

void GameSession::send_ping(uint64 now)
{
	set_last_ping_tick(now);
	set_waiting_pong(true);

	Protocol::ACK_SEND_CONNECT_PING pkt;
	pkt.set_server_tick(now);

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	send(sendBuffer);
}


SendBufferRef GameSession::make_server_full_notice(int32 max_session_count)
{
	// 정원 초과 이유와 최대 접속 인원을 클라이언트에 전달한다.
	Protocol::ACK_SERVER_FULL notice;
	notice.set_max_session_count(static_cast<uint32>(max_session_count));
	return ServerPacketHandler::MakeSendBuffer(notice);
}
