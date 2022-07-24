/*
	TLS는 Thread Local Storage 의 약자로 스레드마다 갖고 있는 로컬 저장소라고 볼 수 있다.
	그런데 생각해보면 멀티스레드에서 각 스레드는 힙,데이터,코드영역을 공유하고 각각 별도의 스택영역을 갖는다.
	그럼 스택영역과 TLS는 뭐가 다른걸까?
	스택영역은 함수나 특정한 범위를 위한 공간이란것이 일반적이다.
	그렇기 때문에 스택영역은 범위가 끝나면 스택프레임이 정리되면서 일정 부분은 사용할 수 없게 되는 불안정한 영역이라는 특징이 있다.
	하지만 TLS는 스레드별로 독립적으로 주어지는 저장소로 전역공간처럼 쓸 수도 있다. (나만 쓸 수 있는 전역공간)
*/

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

using namespace std;

//thread_local 키워드를 붙여주면 TLS구역에 변수가 할당된다.
//기존 전역변수라면 다른 스레드가 값을 덮어 쓰는 순간 기존 스레드도 영향을 받지만
//이 키워드가 있으면 영향을 받지 않는다.
//C++ 11 부터는 thread_local 키워드로 사용하지만
//이 전에는 __declspec(thread) 를 사용했다 했고 운영체제마다 방법이 달랐다 한다.

thread_local int LThreadID = 0;

void ThreadMain(int threadID)
{
	LThreadID = threadID;

	while (true)
	{
		printf("스레드 %d\n",LThreadID);
		this_thread::sleep_for(0.5s);
	}
}

int main()
{
	vector<thread> threads;
	for (int i = 0; i < 10; i++)
	{
		int threadID = i + 1;
		threads.push_back(thread(ThreadMain, threadID));
	}

	for (thread& t : threads)
	{
		t.join();
	}
}