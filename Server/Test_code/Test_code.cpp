#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <array>
#include <algorithm>
#include <thread>
#include <random>
#include <ctime>
#include <chrono>

using namespace std;

void test1(const vector<long>& random_sessions);
void test2(const vector<long>& random_sessions);

int main()
{
	long session_count = 1000000;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<long> distrib(1, session_count);

	vector<long> random_sessions;

	for (int i = 0; i <= 10; i++)
	{
		random_sessions.push_back(distrib(gen));
	}

	thread t1(test1, cref(random_sessions));
	thread t2(test2, cref(random_sessions));

	t1.join();
	t2.join();

	return 0;
}


void test1(const vector<long>& random_sessions)
{
	set<long> sessions;
	long session_count = 1000000;

	for (long i = 0; i < session_count; i++) 
	{
		sessions.insert(i);
	}       

    for (long session_id : random_sessions)
    {
        auto start = chrono::steady_clock::now();

        auto it = sessions.find(session_id);

        auto end = chrono::steady_clock::now();
        chrono::duration<double, milli> elapsed = end - start;

        if (it != sessions.end())
            cout << "[set find] session_id : " << *it << " " << elapsed.count() << "ms" << endl;
    }
}

void test2(const vector<long>& random_sessions)
{
	map<long, long> sessions;
	long session_count = 1000000;

	for (long i = 0; i < session_count; i++)
	{
		sessions.emplace(i, i);
	}

	for (int count = 0; count < random_sessions.size(); count++)
	{
		long session_id = random_sessions[count];

		chrono::steady_clock::time_point start = chrono::steady_clock::now();

		auto it = sessions.find(session_id);

		chrono::steady_clock::time_point end = chrono::steady_clock::now();

		chrono::duration<double, std::milli> milli = end - start;

		if (it != sessions.end())
		{
			cout << "[map find] session_id : " << it->first << "  " << milli.count() << "ms" << endl;
		}

	}
}

