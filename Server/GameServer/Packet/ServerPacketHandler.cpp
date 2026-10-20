#include "pch.h"
#include "ServerPacketHandler.h"
#include "../ServerCore/NetWork/Service.h"
#include "../ServerCore/NetWork/SessionManager.h"
#include "../ServerCore/Utils/StringUtil.h"

#include "../Main/GameSession.h"
#include "../Utils/GameGlobal.h"
#include "../Login/Login.h"
#include "../Room/RoomManager.h"
#include "../Room/Room.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_REQ_LOGIN(GameSessionRef& session, Protocol::REQ_LOGIN& pkt)
{
	if (session == nullptr) {
		ERROR_LOG("Handle_REQ_LOGIN : session nullptr");
		return false;
	}

	Protocol::ACK_LOGIN loginPkt;

	uint64 idx = 0;
	wstring user_name = L"";
	BYTE result = 0;

	//DB 환경 만들었을 때 사용.
	/*wstring userID = stringToWstring(pkt.id());
	wstring userPass = stringToWstring(pkt.pw());

	if (!GDBManager->Select_LoginData(OUT idx, OUT userID, OUT userPass)) {
		loginPkt.set_success(false);
		SEND_PACKET(loginPkt);
		return false;
	}*/

	// 로그인 및 계정 생성 시 아이디 검증
	if (pkt.id().empty() || pkt.id().size() > 32)
	{
		ERROR_LOG("invalid login id");
		return false;
	}

	// 로그인 및 계정 생성 시 비밀번호 검증
	if (pkt.pw().empty() || pkt.pw().size() > 32)
	{
		ERROR_LOG("invalid login pw");
		return false;
	}

	// 계정 생성 시 닉네임 검증
	if (pkt.is_create() && (pkt.name().empty() || pkt.name().size() > 32))
	{
		ERROR_LOG("invalid login name");
		return false;
	}

	if (session->get_is_login())
	{
		ERROR_LOG("already login session");
		session->disconnect();
		return false;
	}

	if (pkt.is_create()) {
		user_name = StringUtil::Utf8ToWString(pkt.name());

		result = GLogin->create_account(pkt.id(), pkt.pw(), user_name, OUT idx);
	}
	else if (!pkt.is_create()) {
		result = GLogin->login(pkt.id(), pkt.pw(), OUT user_name, OUT idx);
	}

	if (result == Protocol::LOGIN_ERROR::LOGIN_SUCCESS)
	{
		session->set_is_login(true);
		session->set_account_idx(idx);
		session->set_name(user_name);

		auto service = session->get_service().lock();
		if (service) {
			service->get_sessionManager()->add_account_session(idx, session);
			service->get_sessionManager()->add_name_session(user_name, session);
		}

		GServerStats.login++;
	}

	loginPkt.set_idx(idx);
	loginPkt.set_user_name(StringUtil::WStringToUtf8(user_name));
	loginPkt.set_result((Protocol::LOGIN_ERROR)result);

	SEND_PACKET(loginPkt);

	return true;
}

bool Handle_REQ_LOGOUT(GameSessionRef& session, Protocol::REQ_LOGOUT& pkt)
{
	if (session == nullptr) {
		ERROR_LOG("Handle_REQ_LOGIN : session nullptr");
		return false;
	}

	if (session->get_is_login() == false)
	{
		ERROR_LOG("not login session");
		session->disconnect();
		return false;
	}

	Protocol::ACK_LOGOUT logout_pkt;

	// 같은 계정으로 다른 세션이 로그인하기 전에 기존 방 정보를 먼저 정리한다.
	if (session->get_room_id() >= 0)
	{
		RoomManager::get_instance()->exit_room(session->get_room_id(), session->get_account_idx());
		session->set_room_id(-1);
	}

	if (!GLogin->logout(session->get_account_idx()))
	{
		ERROR_LOG("Handle_REQ_LOGIN : account 정보가 없음");
		logout_pkt.set_result(Protocol::LOGOUT_ERROR::LOGOUT_FAILE);
		SEND_PACKET(logout_pkt);

		return true;
	}

	auto service = session->get_service().lock();
	if (service){
		service->get_sessionManager()->release_logout_session(session->get_account_idx(), session);
		service->get_sessionManager()->release_name_session(session->get_name(), session);
	}

	logout_pkt.set_result(Protocol::LOGOUT_ERROR::LOGOUT_SUCCESS);

	// 계정과 이름 등록을 해제한 뒤 초기화해, 다음 로그인에 이전 계정 정보가 남지 않게 한다.
	session->set_account_idx(0);
	session->set_name(L"");
	session->set_is_login(false);

	SEND_PACKET(logout_pkt);

	return true;
}

bool Handle_REQ_GET_ROOM_INFO(GameSessionRef& session, Protocol::REQ_GET_ROOM_INFO& pkt)
{
	if (session == nullptr) 
	{
		ERROR_LOG("Handle_REQ_GET_ROOM_INFO : session nullptr");
		return false;
	}

	if (session->get_is_login() == false) 
	{
		ERROR_LOG("not login session");
		session->disconnect();
		return false;
	}

	Protocol::ACK_GET_ROOM_INFO room_info_pkt;

	const map<int32, shared_ptr<Room>>& room_list = RoomManager::get_instance()->get_room_list();

	for (const auto& [room_id, room] : room_list)
	{
		if (room == nullptr)
			continue;

		Protocol::RoomInfo* room_info = room_info_pkt.add_room_info_list();

		room_info->set_room_id(room->get_room_id());
		room_info->set_cur_enter_user_count(room->get_room_cur_count());
		room_info->set_cur_enter_max_count(room->get_room_max_count());
	}

	SEND_PACKET(room_info_pkt);

	return true;
}

bool Handle_REQ_ENTER_ROOM(GameSessionRef& session, Protocol::REQ_ENTER_ROOM& pkt)
{
	if (session == nullptr) {
		ERROR_LOG("Handle_REQ_GET_ROOM_INFO : session nullptr");
		return false;
	}

	if (session->get_is_login() == false)
	{
		ERROR_LOG("not login session");
		session->disconnect();
		return false;
	}

	Protocol::ACK_ENTER_ROOM enter_room_pkt;
	Protocol::ENTER_ROOM_ERROR result = Protocol::ENTER_ROOM_ERROR::ENTER_ROOM_SUCCESS;

	int32 enter_room_id = pkt.room_id();

	if (session->get_room_id() >= 0)
	{
		enter_room_pkt.set_result(Protocol::ENTER_ROOM_ERROR::ENTER_ROOM_FAIL);
		SEND_PACKET(enter_room_pkt);
		return true;
	}

	if (!RoomManager::get_instance()->enter_room(enter_room_id, session))
	{
		result = Protocol::ENTER_ROOM_ERROR::ENTER_ROOM_FAIL;
		enter_room_pkt.set_result(result);
		SEND_PACKET(enter_room_pkt);

		return true;
	}

	session->set_room_id(enter_room_id);
	enter_room_pkt.set_room_id(enter_room_id);
	enter_room_pkt.set_result(result);
	SEND_PACKET(enter_room_pkt);

	return true;
}

bool Handle_REQ_EXIT_ROOM(GameSessionRef& session, Protocol::REQ_EXIT_ROOM& pkt)
{
	if (session == nullptr) {
		ERROR_LOG("Handle_REQ_EXIT_ROOM : session nullptr");
		return false;
	}

	if (session->get_is_login() == false)
	{
		ERROR_LOG("not login session");
		session->disconnect();
		return false;
	}

	Protocol::ACK_EXIT_ROOM exit_room_pkt;
	Protocol::EXIT_ROOM_ERROR result = Protocol::EXIT_ROOM_ERROR::EXIT_ROOM_SUCCESS;

	const int32 exit_room_id = session->get_room_id();

	if (exit_room_id < 0 || !RoomManager::get_instance()->exit_room(exit_room_id, session->get_account_idx()))
	{
		result = Protocol::EXIT_ROOM_ERROR::EXIT_ROOM_FAIL;
		exit_room_pkt.set_result(result);
		SEND_PACKET(exit_room_pkt);
		return true;
	}

	session->set_room_id(-1);
	exit_room_pkt.set_result(result);
	SEND_PACKET(exit_room_pkt);

	return true;
}

bool Handle_REQ_BOT_LOGIN(GameSessionRef& session, Protocol::REQ_BOT_LOGIN& pkt)
{
	if (session == nullptr) {
		ERROR_LOG("Handle_REQ_LOGIN : session nullptr");
		return false;
	}


	Protocol::ACK_BOT_LOGIN loginPkt;

	uint64 idx = 0;
	wstring user_name = L"";
	BYTE result = 0;

	//DB 환경 만들었을 때 사용.
	/*wstring userID = stringToWstring(pkt.id());
	wstring userPass = stringToWstring(pkt.pw());

	if (!GDBManager->Select_LoginData(OUT idx, OUT userID, OUT userPass)) {
		loginPkt.set_success(false);
		SEND_PACKET(loginPkt);
		return false;
	}*/

	if (pkt.id().empty() || pkt.id().size() > 32)
	{
		ERROR_LOG("invalid login id");
		return false;
	}

	if (pkt.pw().empty() || pkt.pw().size() > 32)
	{
		ERROR_LOG("invalid login pw");
		return false;
	}

	if (pkt.is_create() && (pkt.name().empty() || pkt.name().size() > 32))
	{
		ERROR_LOG("invalid login name");
		return false;
	}

	if (session->get_is_login())
	{
		ERROR_LOG("already login session");
		session->disconnect();
		return false;
	}

	if (pkt.is_create()) {
		user_name = StringUtil::Utf8ToWString(pkt.name());

		result = GLogin->create_account(pkt.id(), pkt.pw(), user_name, OUT idx);
	}
	else if (!pkt.is_create()) {
		result = GLogin->login(pkt.id(), pkt.pw(), OUT user_name, OUT idx);
	}

	if (result == Protocol::LOGIN_ERROR::LOGIN_SUCCESS) {
		session->set_is_login(true);
		session->set_account_idx(idx);
		session->set_name(user_name);

		auto service = session->get_service().lock();
		if (service) {
			service->get_sessionManager()->add_account_session(idx, session);
			service->get_sessionManager()->add_name_session(user_name, session);
		}

		GServerStats.login++;
	}

	loginPkt.set_idx(idx);
	loginPkt.set_user_name(StringUtil::WStringToUtf8(user_name));
	loginPkt.set_result((Protocol::LOGIN_ERROR)result);

	SEND_PACKET(loginPkt);

	return true;
}

bool Handle_REQ_CHAT(GameSessionRef& session, Protocol::REQ_CHAT& pkt)
{
	if (session == nullptr) {
		ERROR_LOG("Handle_REQ_LOGIN : session nullptr");
		return false;
	}

	if (session->get_is_login() == false)
	{
		ERROR_LOG("not login session");
		session->disconnect();
		return false;
	}
	
	// 실패 응답의 대상 수는 기본값 0이며, 성공한 전송 분기에서만 설정한다.
	Protocol::ACK_CHAT ack_chat;
	Protocol::CHAT_ERROR result = Protocol::CHAT_ERROR::CHAT_ERROR_SUCCESS;

	if (pkt.message().empty() || pkt.message().size() > 256)
	{
		ack_chat.set_result(Protocol::CHAT_ERROR::CHAT_ERROR_FAIL);
		SEND_PACKET(ack_chat);
		return true;
	}

	// 여기서 귓 or 전체 or 룸 나눠서 적용할 예정 
	if(pkt.chat_state() == Protocol::CHAT_STATE::CHAT_NORMAL)
	{
			auto service = session->get_service().lock();
			if (service == nullptr)
			{
				ERROR_LOG("Handle_REQ_CHAT : service expired");
				result = Protocol::CHAT_ERROR::CHAT_ERROR_FAIL;
				ack_chat.set_result(result);
				SEND_PACKET(ack_chat);
				return true;
			}

			if (session->get_room_id() < 0)
			{
				ERROR_LOG("chat error room");
				result = Protocol::CHAT_ERROR::CHAT_ERROR_FAIL;
				ack_chat.set_result(result);
				SEND_PACKET(ack_chat);
				return true;
			}

			if (session->get_room_id() != pkt.room_id())
			{
				ERROR_LOG("room_id error");
				result = Protocol::CHAT_ERROR::CHAT_ERROR_FAIL;
				ack_chat.set_result(result);
				SEND_PACKET(ack_chat);
				return true;
			}

			Protocol::ACK_SEND_CHAT send_chat_pkt;
			send_chat_pkt.set_user_name(StringUtil::WStringToUtf8(session->get_name()));
			send_chat_pkt.set_message(pkt.message());

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(send_chat_pkt);
			const int32 targetCount = service->get_sessionManager()->normal_chat(pkt.room_id(), sendBuffer);
			GServerStats.broadcast_packet++;
			GServerStats.broadcast_target += targetCount;
			ack_chat.set_target_count(static_cast<uint32>(targetCount));
	}

	else if (pkt.chat_state() == Protocol::CHAT_STATE::CHAT_ALL)
	{
		auto service = session->get_service().lock();
		if (service == nullptr)
		{
			ERROR_LOG("Handle_REQ_CHAT : service expired");
			result = Protocol::CHAT_ERROR::CHAT_ERROR_FAIL;
			ack_chat.set_result(result);
			SEND_PACKET(ack_chat);
			return true;
		}

		Protocol::ACK_SEND_CHAT send_chat_pkt;
		send_chat_pkt.set_user_name(StringUtil::WStringToUtf8(session->get_name()));
		send_chat_pkt.set_message(pkt.message());

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(send_chat_pkt);
		const int32 targetCount = service->get_sessionManager()->all_chat(sendBuffer);
		GServerStats.broadcast_packet++;
		GServerStats.broadcast_target += targetCount;
		ack_chat.set_target_count(static_cast<uint32>(targetCount));
	}
	else if (pkt.chat_state() == Protocol::CHAT_STATE::CHAT_WHISPER)
	{
		auto service = session->get_service().lock();
		if (service == nullptr)
		{
			ERROR_LOG("Handle_REQ_CHAT : service expired");
			result = Protocol::CHAT_ERROR::CHAT_ERROR_FAIL;
			ack_chat.set_result(result);
			SEND_PACKET(ack_chat);
			return true;
		}

		if (pkt.target_name().empty() || pkt.target_name().size() > 32)
		{
			result = Protocol::CHAT_ERROR::CHAT_ERROR_FAIL;
			ack_chat.set_result(result);
			SEND_PACKET(ack_chat);
			return true;
		}

		if (!service->get_sessionManager()->is_whisper_target_find(StringUtil::Utf8ToWString(pkt.target_name()))) {
			result = Protocol::CHAT_ERROR::CHAT_ERROR_NOT_TARGET;
			ack_chat.set_result(result);
			SEND_PACKET(ack_chat);
			return true;
		}

		Protocol::ACK_SEND_CHAT send_chat_pkt;
		send_chat_pkt.set_user_name(StringUtil::WStringToUtf8(session->get_name()));
		send_chat_pkt.set_message(pkt.message());

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(send_chat_pkt);
		const int32 targetCount = service->get_sessionManager()->whisper_chat(StringUtil::Utf8ToWString(pkt.target_name()), sendBuffer);
		GServerStats.broadcast_packet++;
		GServerStats.broadcast_target += targetCount;
		ack_chat.set_target_count(static_cast<uint32>(targetCount));
	}
	else
	{
		result = Protocol::CHAT_ERROR::CHAT_ERROR_FAIL;
		ack_chat.set_result(result);
		SEND_PACKET(ack_chat);
		return true;
	}

	GServerStats.recv_chat++;

	ack_chat.set_result(result);
	SEND_PACKET(ack_chat);
	return true;
}

bool Handle_REQ_CONNECT_PONG(GameSessionRef& session, Protocol::REQ_CONNECT_PONG& pkt)
{
	if (session == nullptr)
	{
		ERROR_LOG("Handle_REQ_CONNECT_PONG : session nullptr");
		return false;
	}

	const uint64 now = ::GetTickCount64();

	session->set_last_pong_tick(now);
	session->set_waiting_pong(false);

	return true;
}



