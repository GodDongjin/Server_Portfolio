using System.Collections;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Net;
using UnityEngine;
using System;
using System.IO;
using Protocol;
using System.Runtime.InteropServices;
using Unity.VisualScripting;
using System.Text;
using System.Threading;
using static TMPro.SpriteAssetUtilities.TexturePacker_JsonArray;
using UnityEngine.UI;
using UnityEngine.SceneManagement;

public class GameManager : MonoBehaviour
{
	private static GameManager _instance;

	private ScenesManager scenesManager;
	private TextManager textManager;
	private ActiveManager activeManager;
	private ButtonManager buttonManager;

	private NetWork netWork;
	private TcpClient clientSocket = null;
	private Session session = null;
	public ClientPacketHandler handler;
	
	NetworkStream stream;

	private Queue<Action> actions = new Queue<Action>();

	[SerializeField] private string serverIp = "127.0.0.1";
	[SerializeField] private int serverProt = 7777;

	public ScenesManager GetScenesManager() { return scenesManager; }
	public TextManager GetTextManager() { return textManager; }
	public ActiveManager GetActiveManager() { return activeManager; }
	public ButtonManager GetButtonManager() { return buttonManager; }

	public static GameManager Instance
	{
		get
		{
			if (!_instance)
			{
				_instance = FindObjectOfType(typeof(GameManager)) as GameManager;

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
		scenesManager = this.gameObject.GetComponent<ScenesManager>();
		textManager = this.gameObject.GetComponent<TextManager>();
		activeManager = this.gameObject.GetComponent<ActiveManager>();
		buttonManager = this.gameObject.GetComponent<ButtonManager>();
		session = new Session();
		netWork = new NetWork(session);
	}

	void Start()
	{
		ConnectToGameServer();

		if (clientSocket.Connected == true)
		{
			Debug.Log("On client connect");
			netWork.Init();

			//TODO: Loginâ 
			{
				// TEMP
				//Protocol.C_LOGIN pkt = new Protocol.C_LOGIN();
				//pkt.Id = "test1";
				//pkt.Pass = "test12345";
				//SendBuffer sendBuffer = session.packetHandler.MakeSendBuffer(pkt);
				//session.SendPacket(sendBuffer);
			}

			session.RecvPacketStart();
		}
	}

	private void OnDestroy()
	{
		Debug.Log("삭제됨");
	}

	private void Update()
	{
		while(actions.Count > 0)
		{
			actions.Dequeue().Invoke();
		}
	}

	public Session GetSession() { return session; }

	public void Enqueue(Action action)
	{
		actions.Enqueue(action);
	}

	void ConnectToGameServer()
	{
		try
		{
			this.clientSocket = new TcpClient(serverIp, serverProt);
			stream = clientSocket.GetStream();

			session.SetSocket(this.clientSocket);
		}
		catch (Exception e)
		{
			Debug.Log("On client connect exception " + e);
			return;
		}
	}
}
