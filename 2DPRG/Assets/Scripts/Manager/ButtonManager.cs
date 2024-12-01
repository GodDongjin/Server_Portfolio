using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ButtonManager : MonoBehaviour
{
	//Login Scenes

	//Select Scenes
	private Button nameObjButton;
	private Button createCharButton;
	private Button inGameStartButton;

	public void SelectScenesSeting()
	{
		nameObjButton = GameObject.Find("CreateButton").gameObject.GetComponent<Button>();
		createCharButton = GameObject.Find("CreateNameButton").gameObject.GetComponent<Button>();
		inGameStartButton = GameObject.Find("SelectButton").gameObject.GetComponent<Button>();
		nameObjButton.onClick.AddListener(() => GameManager.Instance.GetActiveManager().OnCharNameObj());
		createCharButton.onClick.AddListener(() => OnCreatePlayer());
		inGameStartButton.onClick.AddListener(() => InGameStart());
	}

	public void OnCreatePlayer()
	{
		GameManager.Instance.Enqueue(() =>
		{
			if (GameManager.Instance.GetTextManager().GetCreateNameText().ToString() == "")
			{
				Debug.Log("이름이 비어있습니다.");
				return;
			}

			Protocol.C_CREATE_PLAYER pkt = new Protocol.C_CREATE_PLAYER();
			pkt.PlayerName = GameManager.Instance.GetTextManager().GetCreateNameText().text;
			pkt.Idx = GameManager.Instance.GetSession().GetSessionIdx();

			SendBuffer sendBuffer = GameManager.Instance.GetSession().GetPacketHandler().MakeSendBuffer(pkt);
			GameManager.Instance.GetSession().SendPacket(sendBuffer);
		});
	}

	public void OffCreatePlayer()
	{
		GameManager.Instance.Enqueue(() =>
		{
			nameObjButton.gameObject.SetActive(false);
		});
	}

	public void InGameStart()
	{
		if(GameManager.Instance.GetActiveManager().GetSelectPlayerObj().activeSelf == false)
		{
			return;
		}

		GameManager.Instance.GetScenesManager().LoadSceneByIndex(2);
	}
}
