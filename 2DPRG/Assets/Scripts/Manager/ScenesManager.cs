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

		// 로딩이 완료될 때까지 대기
		while (!asyncLoad.isDone)
		{
			yield return null;
		}
		
	}

	void Awake()
	{
		// 이 GameObject가 씬 전환 시에도 삭제되지 않도록 설정
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
		
		// 씬이 로드된 후 특정 작업 수행
	}
}
