using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using UnityEngine.UIElements;
using Unity.VisualScripting;
using Google.Protobuf;
using Protocol;
using Google.Protobuf.WellKnownTypes;
using System.Linq;
using System.IO;

struct PacketHeader
{
	public UInt16 size;
	public UInt16 id;
}

public enum PacketEnum : UInt16
{
	PKT_C_LOGIN = 1000,
	PKT_S_LOGIN = 1001,
	PKT_C_ACCESSION = 1002,
	PKT_S_ACCESSION = 1003,
	PKT_C_SELECT_ROOM_ENTER = 1004,
	PKT_S_SELECT_ROOM_ENTER = 1005,
	PKT_C_CREATE_PLAYER = 1006,
	PKT_S_CREATE_PLAYER = 1007,
	PKT_C_ENTER_GAME = 1008,
	PKT_S_ENTER_GAME = 1009,
	PKT_C_MOVE = 1010,
	PKT_S_MOVE = 1011,
	PKT_S_SEND_OTHER_USER = 1012,
	PKT_C_LEAVE_GAME = 1013,
	PKT_S_LEAVE_GAME = 1014,
	PKT_S_SPAWN = 1015,
	PKT_S_DESPAWN = 1016,
	PKT_C_CHAT = 1017,
	PKT_S_CHAT = 1018,
	PKT_C_HELLOWORLD = 1019,
};

public class ClientPacketHandler
{
	Dictionary<PacketEnum, Action<Session, IMessage>> packetHandler = new Dictionary<PacketEnum, Action<Session, IMessage>>();

	public SendBuffer MakeSendBuffer(Protocol.C_LOGIN pkt) { return MakeSendBuffer(pkt, (UInt16)PacketEnum.PKT_C_LOGIN); }
	public SendBuffer MakeSendBuffer(Protocol.C_ACCESSION pkt) { return MakeSendBuffer(pkt, (UInt16)PacketEnum.PKT_C_ACCESSION); }
	public SendBuffer MakeSendBuffer(Protocol.C_SELECT_ROOM_ENTER pkt) { return MakeSendBuffer(pkt, (UInt16)PacketEnum.PKT_C_SELECT_ROOM_ENTER); }
	public SendBuffer MakeSendBuffer(Protocol.C_CREATE_PLAYER pkt) { return MakeSendBuffer(pkt, (UInt16)PacketEnum.PKT_C_CREATE_PLAYER); }
	public SendBuffer MakeSendBuffer(Protocol.C_ENTER_GAME pkt) { return MakeSendBuffer(pkt, (UInt16)PacketEnum.PKT_C_ENTER_GAME); }
	public SendBuffer MakeSendBuffer(Protocol.C_MOVE pkt) { return MakeSendBuffer(pkt, (UInt16)PacketEnum.PKT_C_MOVE); }
	
   public void Init()
   {
		packetHandler.Add(PacketEnum.PKT_S_LOGIN, Handle_S_LOGIN);
		packetHandler.Add(PacketEnum.PKT_S_ACCESSION, Handle_S_ACCESSION);
		packetHandler.Add(PacketEnum.PKT_S_SELECT_ROOM_ENTER, Handle_S_SELECT_ROOM_ENTER);
		packetHandler.Add(PacketEnum.PKT_S_CREATE_PLAYER, Handle_S_CREATE_PLAYER);
		packetHandler.Add(PacketEnum.PKT_S_ENTER_GAME, Handle_S_ENTER_GAME);
		packetHandler.Add(PacketEnum.PKT_S_SEND_OTHER_USER, Handle_S_SEND_OTHER_USER);
		packetHandler.Add(PacketEnum.PKT_S_MOVE, Handle_S_MOVE);
		packetHandler.Add(PacketEnum.PKT_S_LEAVE_GAME, Handle_S_LEAVE_GAME);
		packetHandler.Add(PacketEnum.PKT_S_SPAWN, Handle_S_SPAWN);
		packetHandler.Add(PacketEnum.PKT_S_DESPAWN, Handle_S_DESPAWN);
   }

	SendBuffer MakeSendBuffer<T>(T pkt, UInt16 pktId) where T : IMessage<T>
	{
		UInt16 dataSize = (UInt16)pkt.CalculateSize();
		UInt16 packetSize = (UInt16)(dataSize + Marshal.SizeOf(new PacketHeader()));

		PacketHeader header = new PacketHeader
		{
			id = (UInt16)pktId,
			size = packetSize
		};

		byte[] packet = new byte[packetSize];
		IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(header));

		try
		{
			Marshal.StructureToPtr(header, ptr, true);
			Marshal.Copy(ptr, packet, 0, Marshal.SizeOf(header));
		}
		finally
		{
			Marshal.FreeHGlobal(ptr);
		}

		byte[] pktData = pkt.ToByteArray(); // Assuming pkt implements IMessage<T>
		Array.Copy(pktData, 0, packet, Marshal.SizeOf(header), pktData.Length);

		SendBuffer sendBuffer = new SendBuffer(packetSize);
		Array.Copy(packet, sendBuffer._buffer, packetSize);
		sendBuffer.Close(packetSize);

		return sendBuffer;
	}

	public void OnRecvPacket(Session session, ArraySegment<byte> buffer)
	{
		UInt16 size = BitConverter.ToUInt16(buffer.Array, 0);
		UInt16 id = BitConverter.ToUInt16(buffer.Array, 2);

		if (packetHandler.TryGetValue((PacketEnum)id, out var action))
		{
			// Deserialize the packet based on the id
			IMessage packet = DeserializePacket(id, buffer);
			action.Invoke(session, packet);
		}
		else
		{
			Debug.LogWarning($"Unknown packet ID: {id}");
		}
	}

	private IMessage DeserializePacket(UInt16 id, ArraySegment<byte> buffer)
	{
		if (buffer.Count < 4)
		{
			Debug.LogWarning("Buffer too short to contain packet header.");
			return null;
		}
		Debug.Log((PacketEnum)id);
		// Deserialize the packet based on the packet ID
		switch ((PacketEnum)id)
		{
			case PacketEnum.PKT_S_LOGIN:
				return Protocol.S_LOGIN.Parser.ParseFrom(buffer.Array, 4, buffer.Count - 4);
			case PacketEnum.PKT_S_ACCESSION:
				return Protocol.S_ACCESSION.Parser.ParseFrom(buffer.Array, 4, buffer.Count - 4);
			case PacketEnum.PKT_S_SELECT_ROOM_ENTER:
				return Protocol.S_SELECT_ROOM_ENTER.Parser.ParseFrom(buffer.Array, 4, buffer.Count - 4);
			case PacketEnum.PKT_S_CREATE_PLAYER:
				return Protocol.S_CREATE_PLAYER.Parser.ParseFrom(buffer.Array, 4, buffer.Count - 4);
			case PacketEnum.PKT_S_ENTER_GAME:
				return Protocol.S_ENTER_GAME.Parser.ParseFrom(buffer.Array, 4, buffer.Count - 4);
			case PacketEnum.PKT_S_SEND_OTHER_USER:
				return Protocol.S_SEND_OTHER_USER.Parser.ParseFrom(buffer.Array, 4, buffer.Count - 4);
			case PacketEnum.PKT_S_MOVE:
				return Protocol.S_MOVE.Parser.ParseFrom(buffer.Array, 4, buffer.Count - 4);
			case PacketEnum.PKT_S_LEAVE_GAME:
				return Protocol.S_LEAVE_GAME.Parser.ParseFrom(buffer.Array, 4, buffer.Count - 4);
			case PacketEnum.PKT_S_SPAWN:
				return Protocol.S_SPAWN.Parser.ParseFrom(buffer.Array, 4, buffer.Count - 4);
			case PacketEnum.PKT_S_DESPAWN:
				return Protocol.S_DESPAWN.Parser.ParseFrom(buffer.Array, 4, buffer.Count - 4);
			default:
				return null;
		}
	}

	private void Handle_S_LOGIN(Session session, IMessage packet)
	{
		var loginPacket = (Protocol.S_LOGIN)packet;

		if (loginPacket == null){
			return;
		}

		session.SetSessionIdx(loginPacket.Idx); 

		if (loginPacket.Success == true)
		{
			GameManager.Instance.GetScenesManager().LoadSceneByIndex(1);
		}
		else
		{
			Debug.Log("로그인 실패");
		}
	}

	private void Handle_S_ACCESSION(Session session, IMessage packet)
	{
		var accessionPacket = (Protocol.S_ACCESSION)packet;

		if(accessionPacket.Success == true)
		{
			GameManager.Instance.GetActiveManager().OnAccessionSuccessOhj();
		}
		else
		{
			Debug.Log("로그인 실패");
		}
	}

	private void Handle_S_SELECT_ROOM_ENTER(Session session, IMessage packet)
	{
		var selectPlayerPkt = (Protocol.S_SELECT_ROOM_ENTER)packet;
		if (selectPlayerPkt == null)
		{
			return;
		}

		if(selectPlayerPkt.Error == SELECT_PLAYER_ERROR.SelectPlayerFail)
		{
			Debug.Log("이상함");
		}
		if (selectPlayerPkt.Error == SELECT_PLAYER_ERROR.SelectPlayerSuccess)
		{
			GameManager.Instance.GetActiveManager().OnCreateCharPrefab();
			GameManager.Instance.GetTextManager().SetPlayerInfo(selectPlayerPkt.Player);
			GameManager.Instance.GetButtonManager().OffCreatePlayer();
		}
	}

	private void Handle_S_CREATE_PLAYER(Session session, IMessage packet)
	{
		var createPlayerPkt = (Protocol.S_CREATE_PLAYER)packet;
		if (createPlayerPkt == null){
			return;
		}

		if(createPlayerPkt.Error == CREATE_PLAYER_ERROR.CreatePlayerDuplication)
		{
			GameManager.Instance.GetTextManager().OnDuplication();
		}
		if (createPlayerPkt.Error == CREATE_PLAYER_ERROR.CreatePlayerSuccess)
		{
			GameManager.Instance.GetActiveManager().OnCreateCharPrefab();
			GameManager.Instance.GetActiveManager().OffCharNameObj();
			GameManager.Instance.GetTextManager().SetPlayerInfo(createPlayerPkt.Player);
		}
	}

	private void Handle_S_ENTER_GAME(Session session, IMessage packet)
	{
		var enterGamePkt = (Protocol.S_ENTER_GAME)packet;
		if (enterGamePkt == null){
			return;
		}

		if(enterGamePkt.Error == ENTER_GAME_ERROR.EnterGameFail){
			return;
		}

		if(enterGamePkt.Error == ENTER_GAME_ERROR.EnterGameSuccess)
		{
			PlayerManager.Instance.CreateOwnerPlayer(enterGamePkt.SessionPlayer);
			
			var playerList = enterGamePkt.Players;
			foreach(var player in playerList)
			{
				PlayerManager.Instance.CreateOtherPlayer(player);
			}
		}
	}

	private void Handle_S_SEND_OTHER_USER(Session session, IMessage packet)
	{
		Debug.Log("Handle_S_SEND_OTHER_USER");

		var sendGamePkt = (Protocol.S_SEND_OTHER_USER)packet;
		if (sendGamePkt == null)
		{
			Debug.Log("NULL");
			return;
		}

		if (sendGamePkt.Error == ENTER_GAME_ERROR.EnterGameFail)
		{
			Debug.Log("EnterGameFail");
			return;
		}

		if (sendGamePkt.Error == ENTER_GAME_ERROR.EnterGameSuccess)
		{

			Debug.Log(sendGamePkt.Player.Idx);
			PlayerManager.Instance.CreateOtherPlayer(sendGamePkt.Player);
		}
	}

	private void Handle_S_MOVE(Session session, IMessage packet)
	{
		var movePkt = (Protocol.S_MOVE)packet;
		if(movePkt == null){
			return;
		}

		if(movePkt.Error == MOVE_ERROR.MoveFail){
			return;
		}
	}
	private void Handle_S_LEAVE_GAME(Session session, IMessage packet)
	{
		Debug.Log("Handle_S_LEAVE_GAME");
	}

	private void Handle_S_SPAWN(Session session, IMessage packet)
	{
		Debug.Log("Handle_S_SPAWN");
	}

	private void Handle_S_DESPAWN(Session session, IMessage packet)
	{
		Debug.Log("Handle_S_DESPAWN");
	}

}
