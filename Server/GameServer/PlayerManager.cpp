#include "pch.h"
#include "PlayerManager.h"
#include "ObjectUtils.h"
#include "Protocol.pb.h"
#include "ServerPacketHandler.h"

PlayerManagerRef GPlayerManager = make_shared<PlayerManager>();

PlayerManager::PlayerManager()
{
	players = make_shared<map<uint64, PlayerRef>>();
}

PlayerManager::~PlayerManager()
{
	DeleteAllPlayer();
}

void PlayerManager::CreatePlayer(uint64 idx, SessionRef session, Protocol::PlayerInfo playerInfo)
{
    WRITE_LOCK;

    // 세션이 유효한지 검사
    if (!session) {
        cerr << "Error: Invalid session. Session is nullptr." << endl;
        return;
    }

    // 세션이 연결 상태인지 검사
    if (!session->IsConnected()) {
        cerr << "Error: Session is not connected." << endl;
        return;
    }

    // 해당 idx가 이미 존재하는지 검사
    if (players->find(idx) != players->end()) {
        cerr << "Error: Player with idx " << idx << " already exists." << endl;
        return;
    }
    // 새로운 플레이어 생성
    try {
        PlayerRef player = make_shared<Player>(session, playerInfo);
		players->insert(make_pair(idx, player));
    }
    catch (const std::exception& e) {
        cerr << "Error: Exception occurred while creating player - " << e.what() << endl;
    }
}

void PlayerManager::DeletePlayer(uint64 playerId)
{
	players->erase(playerId);
}

bool PlayerManager::MovePlayer(Protocol::PlayerPos pos, SessionRef mySession)
{
	WRITE_LOCK;

	if (players->find(pos.idx()) == players->end()) {
		return false;
	}
		
	GetPlayer(pos.idx())->SetPlayerPos(pos);

	Protocol::S_MOVE movePkt;

	Protocol::PlayerPos* playerPos = movePkt.mutable_sessionpos();
	playerPos->CopyFrom(GetPlayer(pos.idx())->GetPos());
	movePkt.set_error(Protocol::MOVE_ERROR::MOVE_SUCCESS);

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
	BoradCast(sendBuffer, pos.idx(), mySession);
	return true;
}

void PlayerManager::UserEnterBoradCast(Protocol::PlayerInfo playerInfo, SessionRef mySession)
{
	std::cout << " UserEnterBoradCast" << endl;

	Protocol::S_SEND_OTHER_USER sendPkt;
	
	Protocol::PlayerInfo* player = sendPkt.mutable_player();
	player->CopyFrom(playerInfo);
	sendPkt.set_error(Protocol::ENTER_GAME_ERROR::ENTER_GAME_SUCCESS);
	cout << player->idx() << endl;
	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(sendPkt);
	BoradCast(sendBuffer, player->idx(), mySession);
}

void PlayerManager::DeleteAllPlayer()
{
	players->clear();
}

void PlayerManager::BoradCast(SendBufferRef sendBuffer, uint64 idx, SessionRef mySession)
{
	std::cout << " BoradCast" << endl;
	for (auto itr = players->begin(); itr != players->end(); itr++)
	{
		if (SessionRef session = itr->second->GetSeeion()) {
			if (mySession == itr->second->GetSeeion()) {
				continue;
			}
			std::cout << " send" << endl;
			session->Send(sendBuffer);
		}
	}
}
