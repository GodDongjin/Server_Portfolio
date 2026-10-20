#include "ClientPacketHandler.h"
#include "../Utils/StringUtil.h"
#include "../Utils/GlobalData.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_ACK_LOGIN(shared_ptr<TestSession>& session, Protocol::ACK_LOGIN& pkt)
{
	if (session == nullptr) {
		return false;
	}
	switch (pkt.result())
	{
	case Protocol::LOGIN_ERROR::LOGIN_SUCCESS:
		wcout << L"로그인 성공" << endl;
		break;
	case Protocol::LOGIN_ERROR::LOGIN_ID_NONE:
		wcout << L"아이디가 없습니다." << endl;
		break;
	case Protocol::LOGIN_ERROR::LOGIN_PW_NONE:
		wcout << L"비밀번호가 일치하지 않습니다." << endl;
		break;
	case Protocol::LOGIN_ERROR::LOGIN_CREATE_HAS_ACCOUNT:
		wcout << L"이미 계정이 있습니다." << endl;
		break;
	case Protocol::LOGIN_ERROR::LOGIN_ALREADY_LOGIN:
		wcout << L"이미 로그인되어 있습니다." << endl;
		break;
	case Protocol::LOGIN_ERROR::LOGIN_USER_DUPLICATION:
		wcout << L"이미 사용 중인 닉네임입니다. 다른 닉네임을 입력하세요." << endl;
		break;
	default:
		wcout << L"로그인 또는 계정 생성에 실패했습니다." << endl;
		break;
	}
	// IOCP 스레드에서는 결과만 알리고, 재입력은 메인 스레드가 처리한다.
	session->complete_login(pkt.result() == Protocol::LOGIN_ERROR::LOGIN_SUCCESS, pkt.idx());
	return true;
}

bool Handle_ACK_BOT_LOGIN(shared_ptr<TestSession>& session, Protocol::ACK_BOT_LOGIN& pkt)
{
	if (session == nullptr) {
		return false;
	}
	const bool success = pkt.result() == Protocol::LOGIN_ERROR::LOGIN_SUCCESS;
	session->complete_login(success, pkt.idx());
	if (success)
	{
		GServerStats.login_success++;
	}
	else if (pkt.result() == Protocol::LOGIN_ERROR::LOGIN_CREATE_HAS_ACCOUNT)
	{
		// 기존 계정이면 생성 대신 로그인으로 한 번 전환한다.
		session->test_login(session->get_bot_index(), true);
	}
	else
	{
		// 같은 자격 증명으로 실패를 반복하지 않도록 이 봇을 종료한다.
		wcout << L"봇 로그인 실패: Bot_" << session->get_bot_index()
			<< L" / 오류: " << pkt.result() << endl;
		session->disconnect();
	}
	return true;
}

bool Handle_ACK_LOGOUT(shared_ptr<TestSession>& session, Protocol::ACK_LOGOUT& pkt)
{
    if (session == nullptr) {
        cout << "Handle_ACK_CHAT : session nullptr" << endl;
        return false;
    }

    session->disconnect();

    return true;
}

bool Handle_ACK_GET_ROOM_INFO(shared_ptr<TestSession>& session, Protocol::ACK_GET_ROOM_INFO& pkt)
{
    if (session == nullptr) {
        cout << "Handle_ACK_GET_ROOM_INFO : session nullptr" << endl;
        return false;
    }

    if (GClientMode != ClientMode::MANUAL)
        return true;

    wcout << L"========== Room List ==========" << endl;

    for (const Protocol::RoomInfo& room_info : pkt.room_info_list())
    {
        wcout << L"Room ID: " << static_cast<int32>(room_info.room_id())
            << L" / User: " << static_cast<int32>(room_info.cur_enter_user_count())
            << L" / Max: " << static_cast<int32>(room_info.cur_enter_max_count())
            << endl;
    }

    wcout << L"===============================" << endl;

    return true;
}

bool Handle_ACK_ENTER_ROOM(shared_ptr<TestSession>& session, Protocol::ACK_ENTER_ROOM& pkt)
{
    if (session == nullptr) {
        cout << "Handle_ACK_ENTER_ROOM : session nullptr" << endl;
        return false;
    }

    session->complete_enter_room(pkt.result() == Protocol::ENTER_ROOM_ERROR::ENTER_ROOM_SUCCESS, pkt.room_id());

    if (pkt.result() == Protocol::ENTER_ROOM_ERROR::ENTER_ROOM_SUCCESS)
    {
        

        
        if (GClientMode == ClientMode::MANUAL) {
            wcout << L"방 입장 성공" << endl;
        }
    }
    else
    {
        wcout << L"방 입장 실패" << endl;
    }

    return true;
}

bool Handle_ACK_EXIT_ROOM(shared_ptr<TestSession>& session, Protocol::ACK_EXIT_ROOM& pkt)
{
    if (session == nullptr) {
        cout << "Handle_ACK_EXIT_ROOM : session nullptr" << endl;
        return false;
    }

    session->complete_exit_room(pkt.result() == Protocol::EXIT_ROOM_ERROR::EXIT_ROOM_SUCCESS);

    if (pkt.result() == Protocol::EXIT_ROOM_ERROR::EXIT_ROOM_SUCCESS)
    {

        if (GClientMode == ClientMode::MANUAL)
            wcout << L"방 퇴장 성공" << endl;
    }
    else if (GClientMode == ClientMode::MANUAL)
    {
        wcout << L"방 퇴장 실패" << endl;
    }

    return true;
}

bool Handle_ACK_CHAT(shared_ptr<TestSession>& session, Protocol::ACK_CHAT& pkt)
{
    if (session == nullptr) {
        cout << "Handle_ACK_CHAT : session nullptr" << endl;
        return false;
    }

    switch (pkt.result())
    {
    case Protocol::CHAT_ERROR_SUCCESS:
        // 방 인원 설정값 대신 서버가 이번 요청에 선택한 실제 대상 수를 누적한다.
        GServerStats.expected_chat_recv += pkt.target_count();
        break;
    case Protocol::CHAT_ERROR_FAIL:
        wcout << L"채팅 전송 실패" << endl;
        break;
    case Protocol::CHAT_ERROR_NOT_TARGET:
        wcout << L"귓속말 상대가 없습니다." << endl;
        break;
    default:
        break;
    }

    return true;
}

bool Handle_ACK_SEND_CHAT(shared_ptr<TestSession>& session, Protocol::ACK_SEND_CHAT& pkt)
{
    if (session == nullptr) {
        cout << "Handle_ACK_CHAT : session nullptr" << endl;
        return false;
    }
 
    if (GClientMode == ClientMode::MANUAL && session->get_is_login() == true)
    {
        wcout << Utf8ToWString(pkt.user_name()) << " :  " << Utf8ToWString(pkt.message()) << endl;
    }

    //wcout << Utf8ToWString(pkt.user_name()) << " :  " << Utf8ToWString(pkt.message()) << endl;

    GServerStats.recv_chat++;
    return true;
}

bool Handle_ACK_SEND_CONNECT_PING(shared_ptr<TestSession>& session, Protocol::ACK_SEND_CONNECT_PING& pkt)
{
    if (session == nullptr) {
        cout << "Handle_ACK_SEND_CONNECT_PING : session nullptr" << endl;
        return false;
    }

    Protocol::REQ_CONNECT_PONG pong;
    pong.set_server_tick(pkt.server_tick());

    shared_ptr<SendBuffer> send_buffer = ClientPacketHandler::MakeSendBuffer(pong);
    session->send(send_buffer);

    return true;
}

bool Handle_ACK_SERVER_FULL(shared_ptr<TestSession>& session, Protocol::ACK_SERVER_FULL& pkt)
{
    if (session == nullptr)
        return false;

    wcout << L"서버 접속 인원이 가득 찼습니다. (최대 "
          << pkt.max_session_count() << L"명)" << endl;
    // 거절된 연결에서는 로그인 재시도를 하지 않고 종료한다.
    session->set_account(0);
    session->set_is_login(false);
    session->disconnect();
    return true;
}
