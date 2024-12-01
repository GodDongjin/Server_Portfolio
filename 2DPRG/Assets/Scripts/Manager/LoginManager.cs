using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class LoginManager : MonoBehaviour
{
    public Text loginIdText;
	public Text loginPassText;


    public void UserLogin()
    {
        if(loginIdText.ToString() == "" || loginPassText.ToString() == "")
        {
            Debug.Log("���̵� Or ��й�ȣ�� �ùٸ����� �ʽ��ϴ�.");
            return;
        }

        Protocol.C_LOGIN pkt = new Protocol.C_LOGIN();
        pkt.Id = loginIdText.text;
        pkt.Pass = loginPassText.text;
        SendBuffer sendBuffer = GameManager.Instance.GetSession().GetPacketHandler().MakeSendBuffer(pkt);
        GameManager.Instance.GetSession().SendPacket(sendBuffer);
	}

    public void UserAccession()
    {
		Protocol.C_ACCESSION pkt = new Protocol.C_ACCESSION();
		pkt.Id = loginIdText.text;
		pkt.Pass = loginPassText.text;
		SendBuffer sendBuffer = GameManager.Instance.GetSession().GetPacketHandler().MakeSendBuffer(pkt);
		GameManager.Instance.GetSession().SendPacket(sendBuffer);
	}
}
