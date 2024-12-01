using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerManager : MonoBehaviour
{
	private static PlayerManager _instance;

	[SerializeField] GameObject playerPrefab;
	private GameObject playerObj;
	private Player ownerPlayer;
	private Dictionary<UInt64, Player> otherPlayers;

	public static PlayerManager Instance
	{
		get
		{
			if (!_instance)
			{
				_instance = FindObjectOfType(typeof(PlayerManager)) as PlayerManager;

				if (_instance == null)
					Debug.Log("no Singleton obj");
			}
			return _instance;
		}
	}

	private void Awake()
	{
		if (_instance == null)
		{
			_instance = this;
		}
		else if (_instance != this)
		{
			Destroy(_instance);
		}

		DontDestroyOnLoad(_instance);
	}

	private void Start()
	{
		otherPlayers = new Dictionary<ulong, Player>();
		playerPrefab = Resources.Load<GameObject>("Prefabs/Player");
	}

	public void CreateOwnerPlayer(Protocol.PlayerInfo playerInfo)
	{
		if(playerInfo == null){
			return;
		}

		GameManager.Instance.Enqueue(() => 
		{
			playerObj = Instantiate(playerPrefab);
			ownerPlayer = playerObj.GetComponent<Player>();
			ownerPlayer.SetPlayerInfo(playerInfo);
		}); 
	}

	public void CreateOtherPlayer(Protocol.PlayerInfo playerInfo)
	{
		GameManager.Instance.Enqueue(() =>
		{
			Debug.Log(playerInfo.Idx);
			if (otherPlayers.ContainsKey(playerInfo.Idx)){
				Debug.Log(playerInfo.Idx);
				Debug.Log("CreateOtherPlayer Error");
				return;
			}

			Debug.Log("CreateOtherPlayer Add");
			otherPlayers.Add(playerInfo.Idx, PlayerPool.Instance.GetObject().GetComponent<Player>());
		});
	}
}
