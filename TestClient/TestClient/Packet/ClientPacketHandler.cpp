#include "ClientPacketHandler.h"
#include "../Utils/StringUtil.h"
#include "../Utils/GlobalData.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_ACK_LOGIN(shared_ptr<TestSession>& session, Protocol::ACK_LOGIN& pkt)
{
    if (session == nullptr) {
        cout << "Handle_ACK_CHAT : session nullptr" << endl;
        return false;
    }

    switch (pkt.result())
    {
       case Protocol::LOGIN_ERROR::LOGIN_FAILE:
           wcout << L"로그인 실패 " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_ID_NONE:
           wcout << L"아이디가 없습니다. " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_PW_NONE:
           wcout << L"패스워드가 없습니다. " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_CREATE_FAILE:
           wcout << L"계정 생성 실패 " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_CREATE_HAS_ACCOUNT:
           wcout << L"이미 계정이 있습니다. " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_ALREADY_LOGIN:
           wcout << L"이미 이미 로그인 되어있습니다. " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_USER_DUPLICATION:
           wcout << L"이미 사용 중인 닉네임입니다. 다른 닉네임을 입력하세요." << endl;
           session->set_account(0);
           session->set_is_login(false);
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_SUCCESS:
           wcout << L"로그인 성공" << endl;
           session->set_account(pkt.idx());
           session->set_is_login(true);
           break;
    default:
        break;
    }

    return true;
}

bool Handle_ACK_BOT_LOGIN(shared_ptr<TestSession>& session, Protocol::ACK_BOT_LOGIN& pkt)
{
    if (session == nullptr) {
        cout << "Handle_ACK_CHAT : session nullptr" << endl;
        return false;
    }

    switch (pkt.result())
    {
    case Protocol::LOGIN_ERROR::LOGIN_FAILE:
        wcout << L"로그인 실패 " << endl;
        session->test_login(session->get_bot_index(), false);
        break;
    case Protocol::LOGIN_ERROR::LOGIN_ID_NONE:
        wcout << L"아이디가 없습니다. " << endl;
        session->test_login(session->get_bot_index(), false);
        break;
    case Protocol::LOGIN_ERROR::LOGIN_PW_NONE:
        wcout << L"패스워드가 없습니다. " << endl;
        session->test_login(session->get_bot_index(), false);
        break;
    case Protocol::LOGIN_ERROR::LOGIN_CREATE_FAILE:
        wcout << L"계정 생성 실패 " << endl;
        session->test_login(session->get_bot_index(), false);
        break;
    case Protocol::LOGIN_ERROR::LOGIN_CREATE_HAS_ACCOUNT:
        wcout << L"이미 계정이 있습니다. " << endl;
        session->test_login(session->get_bot_index(), true);
        break;
    case Protocol::LOGIN_ERROR::LOGIN_ALREADY_LOGIN:
        wcout << L"이미 이미 로그인 되어있습니다. " << endl;
        session->test_login(session->get_bot_index(), false);
        break;
    case Protocol::LOGIN_ERROR::LOGIN_USER_DUPLICATION:
        wcout << L"닉네임 중복으로 봇 로그인 실패: Bot_" << session->get_bot_index() << endl;
        session->set_account(0);
        session->set_is_login(false);
        // 같은 닉네임으로 재시도하면 계속 실패하므로 이 봇의 연결을 종료한다.
        session->disconnect();
        break;
    case Protocol::LOGIN_ERROR::LOGIN_SUCCESS:
        if (GClientMode == ClientMode::MANUAL)
            wcout << L"로그인 성공 Bot_" << session->get_bot_index() << endl;
        session->set_account(pkt.idx());
        session->set_is_login(true);
        GServerStats.login_success++;
        break;
    default:
        break;
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

    if (pkt.result() == Protocol::ENTER_ROOM_ERROR::ENTER_ROOM_SUCCESS)
    {
        
        session->set_room_id(pkt.room_id());
        
        if (GClientMode == ClientMode::MANUAL) {
            wcout << L"방 입장 성공" << endl;
        }
    }
    else
    {
        session->set_room_id(-1);
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

    if (pkt.result() == Protocol::EXIT_ROOM_ERROR::EXIT_ROOM_SUCCESS)
    {
        session->set_room_id(-1);

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
