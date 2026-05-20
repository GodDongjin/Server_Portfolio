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
		while (T item = pop())
			items.push_back(item);
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
