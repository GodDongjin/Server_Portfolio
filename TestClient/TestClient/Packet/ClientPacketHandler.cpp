#include "ClientPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];


bool Handle_INVALID(shared_ptr<TestSession>& session, BYTE* buffer, int32 len)
{
    return true;
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
           cout << "로그인 실패 " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_ID_NONE:
           cout << "아이디가 없습니다. " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_PW_NONE:
           cout << "패스워드가 없습니다. " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_CREATE_FAILE:
           cout << "계정 생성 실패 " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_CREATE_HAS_ACCOUNT:
           cout << "이미 계정이 있습니다. " << endl;
           session->login();
           break;
       case Protocol::LOGIN_ERROR::LOGIN_SUCCESS:
           cout << "로그인 성공 " << endl;
           session->set_account(pkt.idx());
           //session->login();
           break;
    default:
        break;
    }

    return true;
}

bool Handle_ACK_CHAT(shared_ptr<TestSession>& session, Protocol::ACK_CHAT& pkt)
{
   


    return true;
}
