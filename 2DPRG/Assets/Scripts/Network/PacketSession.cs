using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PacketSession : MonoBehaviour
{
	public Queue<SendBuffer> sendPacketQueue { get { return sendPacketQueue; } }
	PacketSession()
	{
	
	}   
}
