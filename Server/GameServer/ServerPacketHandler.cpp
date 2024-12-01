#include "pch.h"
#include "ServerPacketHandler.h"
#include "DBManager.h"
#include "PlayerManager.h"
#include "DBEnum.h"

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
	Protocol::S_LOGIN loginPkt;

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

	SEND_PACKET(loginPkt);

	return true;
}

bool Handle_C_ACCESSION(SessionRef& session, Protocol::C_ACCESSION& pkt)
{
	Protocol::S_ACCESSION accessionPkt;

	wstring testID = stringToWstring(pkt.id());
	wstring testpass = stringToWstring(pkt.pass());

	if (!GDBManager->Insert_Accession(testID, testpass)) {
		accessionPkt.set_success(false);
		SEND_PACKET(accessionPkt);
		return false;
	}

	accessionPkt.set_success(true);
	SEND_PACKET(accessionPkt);

	return true;
}


bool Handle_C_SELECT_ROOM_ENTER(SessionRef& session, Protocol::C_SELECT_ROOM_ENTER& pkt)
{
	if (session->GetAccountIdx() != pkt.idx() || session->IsConnected() == false) {
		return false;
	}

	Protocol::S_SELECT_ROOM_ENTER selectPlayerPkt;

	wstring outName;
	uint32 outLv = 0;
	float outHp = 0;
	float outAtk = 0;
	float outDf = 0;

	Protocol::SELECT_PLAYER_ERROR error = Protocol::SELECT_PLAYER_ERROR::SELECT_PLAYER_NONE;
	if (!GDBManager->Select_Player(OUT(SELECT_PLAYER_ERROR&)error, OUT outName, OUT outLv, OUT outHp, OUT outAtk, OUT outDf, pkt.idx()))
	{
		selectPlayerPkt.set_error(error);
		SEND_PACKET(selectPlayerPkt);
		return false;
	}

	Protocol::PlayerInfo* playerInfo = new Protocol::PlayerInfo();
	playerInfo->set_x(0);
	playerInfo->set_y(0);
	playerInfo->set_z(0);
	playerInfo->set_playername(wstringToString(outName));
	playerInfo->set_lv(outLv);
	playerInfo->set_hp(outHp);
	playerInfo->set_atk(outAtk);
	playerInfo->set_df(outDf);
	playerInfo->set_idx(pkt.idx());

	GPlayerManager->CreatePlayer(pkt.idx(), session, *playerInfo);

	selectPlayerPkt.set_allocated_player(playerInfo);
	selectPlayerPkt.set_error(error);
	SEND_PACKET(selectPlayerPkt);

	return true;
}

bool Handle_C_CREATE_PLAYER(SessionRef& session, Protocol::C_CREATE_PLAYER& pkt)
{
	if (session->GetAccountIdx() != pkt.idx() || session->IsConnected() == false || pkt.playername() == "") {
		return false;
	}

	Protocol::S_CREATE_PLAYER createPlayerPkt;
	Protocol::PlayerInfo* playerInfo = new Protocol::PlayerInfo();
	playerInfo->set_x(0);
	playerInfo->set_y(0);
	playerInfo->set_z(0);
	playerInfo->set_playername(pkt.playername());
	playerInfo->set_lv(1);
	playerInfo->set_hp(100);
	playerInfo->set_atk(10);
	playerInfo->set_df(3);
	playerInfo->set_idx(pkt.idx());
	//playerInfo->set_idx

	wstring playerName = stringToWstring(pkt.playername());

	Protocol::CREATE_PLAYER_ERROR error = Protocol::CREATE_PLAYER_ERROR::CREATE_PLAYER_NONE;
	if (!GDBManager->Insert_Player(OUT (CREATE_PLAYER_ERROR&)error, pkt.idx(), playerName, playerInfo->lv(), playerInfo->hp(), playerInfo->atk(), playerInfo->df()))
	{
		createPlayerPkt.set_error(error);
		SEND_PACKET(createPlayerPkt);
		return false;
	}

	GPlayerManager->CreatePlayer(pkt.idx(), session, *playerInfo);

	createPlayerPkt.set_allocated_player(playerInfo);
	createPlayerPkt.set_error(error);
	SEND_PACKET(createPlayerPkt);
	return true;
}

bool Handle_C_ENTER_GAME(SessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
	Protocol::S_ENTER_GAME enterPkt;
	Protocol::S_SEND_OTHER_USER otherUserPkt;
	std::mutex playersMutex;
	USE_LOCK;

	if (session->GetAccountIdx() != pkt.idx() || session->IsConnected() == false) {
		enterPkt.set_error(Protocol::ENTER_GAME_ERROR::ENTER_GAME_FAIL);
		SEND_PACKET(enterPkt);
		return false;
	}

	if (!GPlayerManager->FindPlayer(pkt.idx())) {
		enterPkt.set_error(Protocol::ENTER_GAME_ERROR::ENTER_GAME_FAIL);
		SEND_PACKET(enterPkt);
		return false;
	}

	Protocol::PlayerInfo* sessionPlayer = new Protocol::PlayerInfo();
	sessionPlayer->CopyFrom(GPlayerManager->GetPlayerInfo(pkt.idx()));
	enterPkt.set_allocated_sessionplayer(sessionPlayer);

	if (!GPlayerManager->GetPlayerEmpty()) {
		{
			WRITE_LOCK;
			for (auto itr = GPlayerManager->GetPlayersMap()->begin(); itr != GPlayerManager->GetPlayersMap()->end(); itr++)
			{
				if (!itr->second->GetInGameState() || itr->first == pkt.idx()) {
					continue;
				}

				Protocol::PlayerInfo* playerInfo = enterPkt.add_players();
				playerInfo->CopyFrom(itr->second->GetPlayerInfo());
			}
		}
	}
	

	enterPkt.set_error(Protocol::ENTER_GAME_ERROR::ENTER_GAME_SUCCESS);
	GPlayerManager->GetPlayer(pkt.idx())->PlayerEnterGame();
	GPlayerManager->UserEnterBoradCast(GPlayerManager->GetPlayerInfo(pkt.idx()), session);

	SEND_PACKET(enterPkt);
	return true;
}

bool Handle_C_MOVE(SessionRef& session, Protocol::C_MOVE& pkt)
{
	Protocol::S_MOVE movePkt;

	if (session->GetAccountIdx() != pkt.sessionpos().idx() || session->IsConnected() == false) {
		movePkt.set_error(Protocol::MOVE_ERROR::MOVE_FAIL);
		SEND_PACKET(movePkt);
		return false;
	}

	if (!GPlayerManager->MovePlayer(pkt.sessionpos(), session)) {
		movePkt.set_error(Protocol::MOVE_ERROR::MOVE_FAIL);
		SEND_PACKET(movePkt);
		return false;
	}

	return true;
}

bool Handle_C_LEAVE_GAME(SessionRef& session, Protocol::C_LEAVE_GAME& pkt)
{
	return true;
}

bool Handle_C_CHAT(SessionRef& session, Protocol::C_CHAT& pkt)
{
	return true;
}

bool Handle_C_HELLOWORLD(SessionRef& session, Protocol::C_HELLOWORLD& pkt)
{
	return true;
}
