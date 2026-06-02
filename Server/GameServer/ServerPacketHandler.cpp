#include "pch.h"
#include "ServerPacketHandler.h"
#include "GameSession.h"
#include "GameGlobal.h"
#include "Login.h"
#include "../ServerCore/NetWork/Service.h"
#include "../ServerCore/NetWork/SessionManager.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(GameSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	
	return true;
}

std::wstring stringToWstring(const std::string& str) {
	// MultiByteToWideChar를 사용하여 UTF-8 문자열을 UTF-16 문자열로 변환
	int wideCharSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);

	// 변환된 문자열을 저장할 wstring 생성
	std::wstring wideString(wideCharSize, 0);

	// 변환된 내용을 wstring에 저장
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wideString[0], wideCharSize);

	return wideString;
}

std::string wstringToString(const std::wstring& wstr) {
	// WideCharToMultiByte를 사용하여 UTF-16 문자열을 UTF-8 문자열로 변환
	int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);

	// 변환된 문자열을 저장할 string 생성
	std::string utf8String(utf8Size - 1, 0);  // null terminator 제외

	// 변환된 내용을 string에 저장
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8String[0], utf8Size, nullptr, nullptr);

	return utf8String;
}

inline std::string WStringToUtf8(const std::wstring& wstr)
{
	if (wstr.empty())
		return "";

	int len = ::WideCharToMultiByte(
		CP_UTF8, 0,
		wstr.data(), static_cast<int>(wstr.size()),
		nullptr, 0,
		nullptr, nullptr
	);

	std::string result(len, 0);

	::WideCharToMultiByte(
		CP_UTF8, 0,
		wstr.data(), static_cast<int>(wstr.size()),
		result.data(), len,
		nullptr, nullptr
	);

	return result;
}

inline std::wstring Utf8ToWString(const std::string& str)
{
	if (str.empty())
		return L"";

	int len = ::MultiByteToWideChar(
		CP_UTF8, 0,
		str.data(), static_cast<int>(str.size()),
		nullptr, 0
	);

	std::wstring result(len, 0);

	::MultiByteToWideChar(
		CP_UTF8, 0,
		str.data(), static_cast<int>(str.size()),
		result.data(), len
	);

	return result;
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
		user_name = Utf8ToWString(pkt.name());

		result = GLogin->create_account(pkt.id(), pkt.pw(), user_name, OUT idx);
	}
	else if (!pkt.is_create()) {
		result = GLogin->login(pkt.id(), pkt.pw(), OUT user_name, OUT idx);
	}

	if (result == Protocol::LOGIN_ERROR::LOGIN_SUCCESS) {
		session->set_is_login(true);
		session->set_account_idx(idx);
		session->set_name(user_name);
	}

	loginPkt.set_idx(idx);
	loginPkt.set_user_name(WStringToUtf8(user_name));
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
		user_name = Utf8ToWString(pkt.name());

		result = GLogin->create_account(pkt.id(), pkt.pw(), user_name, OUT idx);
	}
	else if (!pkt.is_create()) {
		result = GLogin->login(pkt.id(), pkt.pw(), OUT user_name, OUT idx);
	}

	if (result == Protocol::LOGIN_ERROR::LOGIN_SUCCESS) {
		session->set_is_login(true);
		session->set_account_idx(idx);
		session->set_name(user_name);
	}

	loginPkt.set_idx(idx);
	loginPkt.set_user_name(WStringToUtf8(user_name));
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

	INFO_LOG(pkt.message());

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
			send_chat_pkt.set_user_name(WStringToUtf8(session->_name));
			send_chat_pkt.set_message(pkt.message());

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(send_chat_pkt);
			service->get_sessionManager()->broad_cast(sendBuffer);

			return true;
	}

	//Protocol::ACK_CHAT log
	return true;
}

//bool Handle_C_CHAR(GameSessionRef& session, Protocol::C_CHAR& pkt)
//{
//	cout << "Session ID : " << session->get_account_idx() << " - " << pkt.message() << endl;
//
//	Protocol::S_CHAR chat_pkt;
//
//	chat_pkt.set_message(pkt.message());
//
//	SEND_PACKET(chat_pkt);
//	return true;
//}

