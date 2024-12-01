using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ActiveManager : MonoBehaviour
{
	//Login Scenes
	GameObject accessionSuccessObj;

	//Select Scenes
	GameObject charSelectPlayerPrefab;
	GameObject charNameObj;

	public GameObject GetSelectPlayerObj() { return charSelectPlayerPrefab; }

	private void OnEnable()
	{
		
	}

	public void LoginScenesSeting()
	{
		accessionSuccessObj = GameObject.Find("AccessionSuccessObj").gameObject;
		accessionSuccessObj.SetActive(false);
	}

	public void SelectScenesSeting()
	{
		charSelectPlayerPrefab = GameObject.Find("Char_Player").gameObject;
		charNameObj = GameObject.Find("NameObj").gameObject;
		charSelectPlayerPrefab.SetActive(false);
		charNameObj.SetActive(false);
	}

	public void OnAccessionSuccessOhj()
	{
		GameManager.Instance.Enqueue(() =>
		{
			accessionSuccessObj.SetActive(true);
		});
	}

	public void OnCreateCharPrefab()
	{
		GameManager.Instance.Enqueue(() =>
		{
			charSelectPlayerPrefab.SetActive(true);
		});
	}

	public void OnCharNameObj()
	{
		GameManager.Instance.Enqueue(() =>
		{
			charNameObj.SetActive(true);
		});
	}

	public void OffCharNameObj()
	{
		GameManager.Instance.Enqueue(() =>
		{
			charNameObj.SetActive(false);
		});
	}
}
