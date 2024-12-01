using System.Collections;
using System.Collections.Generic;
using System.Numerics;
using UnityEngine;
using UnityEngine.SceneManagement;

public class ScenesManager : MonoBehaviour
{
    public void LoadScene(string SceneName)
    {
		GameManager.Instance.Enqueue(() =>
		{
			SceneManager.LoadScene(SceneName);
		});
		
    }

    public void LoadSceneByIndex(int sceneIndex)
    {
		GameManager.Instance.Enqueue(() =>
		{
			SceneManager.LoadScene(sceneIndex);
		});
		
    }

	IEnumerator LoadYourAsyncScene(string sceneName)
	{
		
		AsyncOperation asyncLoad = SceneManager.LoadSceneAsync(sceneName);

		// �ε��� �Ϸ�� ������ ���
		while (!asyncLoad.isDone)
		{
			yield return null;
		}
		
	}

	void Awake()
	{
		// �� GameObject�� �� ��ȯ �ÿ��� �������� �ʵ��� ����
		DontDestroyOnLoad(gameObject);
		SceneManager.sceneLoaded += OnSceneLoaded;
	}

	void OnSceneLoaded(Scene scene, LoadSceneMode mode)
	{
		if(scene.name == "Login")
		{
			GameManager.Instance.GetActiveManager().LoginScenesSeting();
		}

		if(scene.name == "CharSelect")
		{
			GameManager.Instance.GetTextManager().Seting();
			GameManager.Instance.GetButtonManager().SelectScenesSeting();
			GameManager.Instance.GetActiveManager().SelectScenesSeting();
		
			Protocol.C_SELECT_ROOM_ENTER pkt = new Protocol.C_SELECT_ROOM_ENTER();
			pkt.Idx = GameManager.Instance.GetSession().GetSessionIdx();

			SendBuffer sendBuffer = GameManager.Instance.GetSession().GetPacketHandler().MakeSendBuffer(pkt);
			GameManager.Instance.GetSession().SendPacket(sendBuffer);
			
		}

		if(scene.name == "GameScene")
		{
			Protocol.C_ENTER_GAME pkt = new Protocol.C_ENTER_GAME();
			pkt.Idx = GameManager.Instance.GetSession().GetSessionIdx();

			SendBuffer sendBuffer = GameManager.Instance.GetSession().GetPacketHandler().MakeSendBuffer(pkt);
			GameManager.Instance.GetSession().SendPacket(sendBuffer);
		}
		
		// ���� �ε�� �� Ư�� �۾� ����
	}
}
