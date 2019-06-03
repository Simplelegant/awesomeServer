#include <iostream>
#include <thread>
#include <mutex>
#include "Alloctor.h"
#include <memory>
#include "Alloctor.h"
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
	
}

class ClassA
{
public:
	ClassA()
	{
		printf("ClassA\n");
	}
	~ClassA()
	{
		printf("~ClassA\n");
	}

public:
	int num;
};

shared_ptr<ClassA> fun(shared_ptr<ClassA> pA)
{
	printf("use_count=%d\n", pA.use_count());
	pA->num++;
	return pA;
} 

int main()
{
	//thread t[tCount];
	//for (int n = 0; n < tCount; n++)
	//{
	//	t[n] = thread(workFun, n);
	//}
	//CELLTimestamp tTime;
	//for (int n = 0; n < tCount; n++)
	//{
	//	t[n].join();
	//	//t[n].detach();
	//}
	////t.detach();
	////t.join();
	//cout << tTime.getElapsedTimeInMilliSec()  << endl;
	//
	//cout << "Hello, main thread." << endl;
	//getchar();
	//int *a = new int;
	//*a = 100;
	//printf("a=%d\n", *a);
	//delete a;
	////C++标准库中智能指针的一种
	//shared_ptr<int> b = make_shared<int>();
	//*b = 100;
	//printf("b=%d\n", *b);
	//ClassA* a1 = new ClassA;
	//delete a1;
	shared_ptr<ClassA> b = make_shared<ClassA>();
	printf("use_count=%d\n", b.use_count());
	b->num = 100;
	shared_ptr<ClassA> c =  fun(b);
	printf("use_count=%d\n", b.use_count());
	printf("unum=%d\n", b->num);
	getchar();
	return 0;
}