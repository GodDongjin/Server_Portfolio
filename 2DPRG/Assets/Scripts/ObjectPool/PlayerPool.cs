using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerPool : MonoBehaviour
{
	private static PlayerPool _instance;

    private GameObject prefab;
    private int poolSize = 30;

    private Queue<GameObject> pool = new Queue<GameObject>();

	public static PlayerPool Instance
	{
		get
		{
			if (!_instance)
			{
				_instance = FindObjectOfType(typeof(PlayerPool)) as PlayerPool;

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
	}

	void Start()
	{
		prefab = Resources.Load<GameObject>("Prefabs/Player");

		// 초기화: poolSize만큼 오브젝트를 미리 생성하여 풀에 추가
		for (int i = 0; i < poolSize; i++)
		{
			GameObject obj = Instantiate(prefab, this.gameObject.transform);
			obj.SetActive(false);
			pool.Enqueue(obj);
		}
	}

	// 풀에서 오브젝트를 가져오는 함수
	public GameObject GetObject()
	{
		if (pool.Count > 0)
		{
			GameObject obj = pool.Dequeue();
			obj.SetActive(true);
			return obj;
		}
		else
		{
			// 풀에 남아 있는 오브젝트가 없을 경우, 새로 생성하여 반환
			GameObject obj = Instantiate(prefab);
			return obj;
		}
	}

	// 오브젝트를 다시 풀에 반환하는 함수
	public void ReturnObject(GameObject obj)
	{
		obj.SetActive(false);
		pool.Enqueue(obj);
	}
}
