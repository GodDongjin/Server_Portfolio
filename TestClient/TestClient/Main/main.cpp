#pragma once

#include <iostream>

#include "../Session/TestSession.h"

#pragma comment(lib, "Ws2_32.lib")

int main()
{
	TestSession test_session;

	for (int i = 0; i < 1000; i++)
	{
		if (!test_session.connect(L"127.0.0.1", 7777))
		{
			cout << "connect failed" << endl;
			return 0;
		}
	}

	

	while (true)
	{
		::Sleep(1000);
	}

	return 0;
}