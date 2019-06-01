#include <iostream>
#include <thread>
#include <mutex>

//#include "Alloctor.h"
#include "CELLTimestamp.hpp"
using namespace std;

mutex m;
const int tCount = 8;
const int mCount = 10000;
const int nCount = mCount / tCount;
void  workFun(int index)
{
	char* data[nCount];
	for (size_t i = 0; i < nCount; i++)
		data[i] = new char[rand()%128+1];
	for (size_t i = 0; i < nCount; i++)
		delete[] data[i];
	//for (int n = 0; n < nCount; n++)
	//{
	//	lock_guard<mutex> lg(m);
	//	//m.lock();
	//	//m.unlock();
	//}
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
	cout << tTime.getElapsedTimeInMilliSec()  << endl;
	
	cout << "Hello, main thread." << endl;
	getchar();

	return 0;
}