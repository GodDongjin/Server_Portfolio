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

		// �ʱ�ȭ: poolSize��ŭ ������Ʈ�� �̸� �����Ͽ� Ǯ�� �߰�
		for (int i = 0; i < poolSize; i++)
		{
			GameObject obj = Instantiate(prefab, this.gameObject.transform);
			obj.SetActive(false);
			pool.Enqueue(obj);
		}
	}

	// Ǯ���� ������Ʈ�� �������� �Լ�
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
			// Ǯ�� ���� �ִ� ������Ʈ�� ���� ���, ���� �����Ͽ� ��ȯ
			GameObject obj = Instantiate(prefab);
			return obj;
		}
	}

	// ������Ʈ�� �ٽ� Ǯ�� ��ȯ�ϴ� �Լ�
	public void ReturnObject(GameObject obj)
	{
		obj.SetActive(false);
		pool.Enqueue(obj);
	}
}
