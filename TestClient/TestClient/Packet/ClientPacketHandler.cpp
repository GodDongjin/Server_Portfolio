#include "ClientPacketHandler.h"
#include "../Utils/StringUtil.h"
#include "../Utils/GlobalStruct.h"
#include "../Utils/ClientConfig.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];


bool Handle_INVALID(shared_ptr<TestSession>& session, BYTE* buffer, int32 len)
{
    return false;
}

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
       case Protocol::LOGIN_ERROR::LOGIN_SUCCESS:
           wcout << L"로그인 성공 Bot_" << session->get_bot_index() << endl;
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
    case Protocol::LOGIN_ERROR::LOGIN_SUCCESS:
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

bool Handle_ACK_CHAT(shared_ptr<TestSession>& session, Protocol::ACK_CHAT& pkt)
{

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
