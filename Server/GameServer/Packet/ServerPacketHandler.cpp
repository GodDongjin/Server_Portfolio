#include "pch.h"
#include "ServerPacketHandler.h"
#include "../ServerCore/NetWork/Service.h"
#include "../ServerCore/NetWork/SessionManager.h"
#include "../ServerCore/Utils/StringUtil.h"

#include "../Main/GameSession.h"
#include "../Utils/GameGlobal.h"
#include "../Login/Login.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(GameSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	
	return false;
}

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

	Protocol::ACK_LOGOUT logout_pkt;

	if (!GLogin->logout(session->get_account_idx()))
	{
		ERROR_LOG("Handle_REQ_LOGIN : account 정보가 없음");
		logout_pkt.set_result(Protocol::LOGOUT_ERROR::LOGOUT_FAILE);
		SEND_PACKET(logout_pkt);

		return true;
	}

	logout_pkt.set_result(Protocol::LOGOUT_ERROR::LOGOUT_SUCCESS);

	session->set_account_idx(0);
	session->set_is_login(false);

	SEND_PACKET(logout_pkt);

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

	GServerStats.recv_chat++;
	//INFO_LOG(pkt.message());

	// 여기서 귓 or 전체 or 룸 나눠서 적용할 예정 
	if(pkt.chat_state() == Protocol::CHAT_STATE::CHAT_ALL)
	{
			auto service = session->get_service().lock();
			if (service == nullptr)
			{
				ERROR_LOG("Handle_REQ_CHAT : service expired");
				return false;
			}

			Protocol::ACK_SEND_CHAT send_chat_pkt;
			send_chat_pkt.set_user_name(StringUtil::WStringToUtf8(session->_name));
			send_chat_pkt.set_message(pkt.message());

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(send_chat_pkt);
			const int32 targetCount = service->get_sessionManager()->broad_cast(sendBuffer);

			GServerStats.broadcast_packet++;
			GServerStats.broadcast_target += targetCount;

			return true;
	}

	return true;
}

