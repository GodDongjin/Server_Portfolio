#include "pch.h"
#include "../ServerCore/DB/DBManager.h"
#include "../ServerCore/DB/DBEnum.h"
#include "ServerPacketHandler.h"
#include "PlayerManager.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len)
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

bool Handle_C_LOGIN(SessionRef& session, Protocol::C_LOGIN& pkt)
{
	/*Protocol::S_LOGIN loginPkt;

	uint64 idx = 0;
	wstring userID = stringToWstring(pkt.id());
	wstring userPass = stringToWstring(pkt.pass());

	if (!GDBManager->Select_LoginData(OUT idx, OUT userID, OUT userPass)) {
		loginPkt.set_success(false);
		SEND_PACKET(loginPkt);
		return false;
	}

	session->SetAccountIdx(idx);

	loginPkt.set_idx(idx);
	loginPkt.set_success(true);

	SEND_PACKET(loginPkt);*/

	return true;
}

bool Handle_C_CHAR(SessionRef& session, Protocol::C_CHAR& pkt)
{
	cout << "Session ID : " << session->get_account_idx() << " - " << pkt.message() << endl;

	Protocol::S_CHAR chat_pkt;

	chat_pkt.set_message(pkt.message());

	SEND_PACKET(chat_pkt);
	return true;
}

