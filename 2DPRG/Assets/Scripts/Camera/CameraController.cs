using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CameraController : MonoBehaviour
{
    Vector3 cameraPos = new Vector3(0, 2, -10);
	Transform playerTransform;
	float cameraMoveSpeed = 4;
	private void Start()
	{
		playerTransform = GameObject.Find("Player").transform;
	}

	private void FixedUpdate()
	{
		transform.position = Vector3.Lerp(transform.position, playerTransform.position + cameraPos, Time.deltaTime * cameraMoveSpeed);
	}
}
