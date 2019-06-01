#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include "CELLTimestamp.hpp"
using namespace std;

mutex m;
const int tCount = 4;
atomic_int sum = 0;
void  workFun(int index)
{
	for (int n = 0; n < 200000; n++)
	{
		lock_guard<mutex> lg(m);
		//m.lock();
		sum++;
		//m.unlock();
	}
		//cout << index << "Hello, main thread." << n << endl;
}

int main()
{
	thread t[tCount];
	for (int n = 0; n < tCount; n++)
	{
		t[n] = thread(workFun, n);
	}
	CELLTimestamp tTime;
	for (int n = 0; n < tCount; n++)
	{
		t[n].join();
		//t[n].detach();
	}
	//t.detach();
	//t.join();
	cout << tTime.getElapsedTimeInMilliSec() << ",sum=" << sum << endl;
	sum = 0;
	tTime.update();
	for (int n = 0; n < 800000; n++)
	{
		sum++;
	}
	cout << tTime.getElapsedTimeInMilliSec() << ",sum=" << sum << endl;
	cout << "Hello, main thread." << endl;
	getchar();

	return 0;
}