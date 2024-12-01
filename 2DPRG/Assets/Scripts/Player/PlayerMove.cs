using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerMove : Player
{
    bool isJumping = false;
	
	// Start is called before the first frame update
	void Start()
    {
		playerRigidbody2D = GetComponent<Rigidbody2D>();
	}

	private void Update()
	{
		if (!isGrounded && m_groundSensor.State())
		{
			isGrounded = true;
			animator.SetBool("Grounded", isGrounded);
		}

		if (isGrounded && !m_groundSensor.State())
		{
			isGrounded = false;
			animator.SetBool("Grounded", isGrounded);
		}

		animator.SetFloat("AirSpeedY", playerRigidbody2D.velocity.y);

		Move();

		if (Input.GetKeyDown("space") && isGrounded && !m_rolling){
			playerRigidbody2D.AddForce(Vector2.up * JumpSpeed, ForceMode2D.Impulse);
		}
		else if(Mathf.Abs(inputX) > Mathf.Epsilon){
			Run();
		}
		else{
			Idle();                                
		}
	}

    void Move()
    {
		inputX = Input.GetAxis("Horizontal");

		if (inputX > 0)
		{
			GetComponent<SpriteRenderer>().flipX = false;
			//m_facingDirection = 1;
		}

		else if (inputX < 0)
		{
			GetComponent<SpriteRenderer>().flipX = true;
			//m_facingDirection = -1;
		}

		playerRigidbody2D.velocity = new Vector2(inputX * moveSpeed, playerRigidbody2D.velocity.y);
	}

    void Jump()
    {
		animator.SetTrigger("Jump");
		isGrounded = false;
		animator.SetBool("Grounded", isGrounded);
		playerRigidbody2D.velocity = new Vector2(playerRigidbody2D.velocity.x, JumpSpeed);
		m_groundSensor.Disable(0.2f);
	}

	void Idle()
	{
		// Prevents flickering transitions to idle
		m_delayToIdle -= Time.deltaTime;
		if (m_delayToIdle < 0)
			animator.SetInteger("AnimState", 0);
	}

	void Run()
	{
		m_delayToIdle = 0.05f;
		animator.SetInteger("AnimState", 1);
	}
	
}
