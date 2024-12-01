using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Player : MonoBehaviour
{
	private Protocol.PlayerInfo mPlayerInfo;

	protected Rigidbody2D playerRigidbody2D;
	protected Animator animator;
	protected BoxCollider2D collider;
	protected Vector2 movement;
	protected PlayerSensor m_groundSensor;

	[SerializeField] protected  float moveSpeed = 3.0f;
	[SerializeField] protected  float JumpSpeed = 7.5f;
	protected float inputX = 0;

	protected bool isGrounded = false;
	protected bool m_rolling = false;
	protected float m_delayToIdle = 0.0f;


	void Awake()
	{
		

		animator = GetComponent<Animator>();
		m_groundSensor = transform.Find("GroundSensor").GetComponent<PlayerSensor>();
		movement = new Vector2();
	}

    // Update is called once per frame
    void Update()
    {
        
    }


	public void SetPlayerInfo(Protocol.PlayerInfo playerInfo)
	{
		mPlayerInfo = playerInfo;
	}
}
