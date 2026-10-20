#pragma once
#include "pch.h"

template<typename T>
class LockQueue
{
public:
	void push(T item)
	{
		WRITE_LOCK;
		_items.push(item);
	}

	T pop()
	{
		WRITE_LOCK;
		if (_items.empty())
			return T();

		T ret = _items.front();
		_items.pop();
		return ret;
	}

	void pop_all(OUT vector<T>& items)
	{
		WRITE_LOCK;
		// 이미 잠금을 잡은 상태에서 pop()을 호출하면 같은 mutex를 다시 잠그며 교착된다.
		// 내부 큐에서 직접 꺼내 한 번의 잠금으로 모든 항목을 옮긴다.
		while (!_items.empty())
		{
			items.push_back(_items.front());
			_items.pop();
		}
	}

	void clear()
	{
		WRITE_LOCK;
		_items = queue<T>();
	}

private:
	USE_LOCK;
	queue<T> _items;
};
