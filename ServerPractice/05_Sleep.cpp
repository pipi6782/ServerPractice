/*
	공용변수를 누군가가 사용중이라면 나는 자리로 돌아가서 내가 할 수 있는 다른 일을 하고 있다가
	일정 시간이 지난 후 다시 사용을 시도해보는 방식이다.
	이 방식의 경우 무한히 대기하지 않고 다른 일을 하고 있을 수 있다는 장점이 있지만
	내가 다시 내 작업을 하러 돌아갔다가 다시 사용을 시도하는 사이에 다른 스레드가 사용하고 있을 수도 있다는 단점이 있다.
	이 방식은 운영체제의 스케쥴링과 큰 관련이 있다.
	스케쥴링 : A프로세스(스레드)에 자원을 할당한 후 다음에는 어느 프로세스(스레드)에 자원을 할당해야 하는지 결정. 스케쥴링을 하는 방법은 운영체제마다, 정책마다 다르다.
	커널에선 각 프로세스에게 정해진 시간만큼 자원을 사용할 수 있는 권한을 주고 프로세스는 주어진 시간이 끝나거나 더 이상 할 작업이 없다면 권한을 커널에 돌려주게 된다.
	이 방식이 자격증 공부할때 항상 나오던 "시분할 시스템"인듯 하다.
*/

#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

int num = 0;

class SpinLock
{
public:
	void lock()
	{
		bool expected = false;
		while (locked.compare_exchange_strong(expected, true) == false) {
			expected = false;
			/*이 lock함수가 의미하는 것은 어떤 스레드가 CAS함수를 실행하는 "바로 그 순간에" locked변수의 값이 false면 true로 교체하는 것*/
			this_thread::sleep_for(100ms); //커널 모드로 돌아가서 100ms동안은 다시 시간을 할당받지 않는다. >> 100ms가 지나기 전에는 다음 스케쥴링에서도 할당받지 못한다.
			//this_thread::yield(); //커널 모드로 돌아가서 내가 할당받은 시간만 포기한다. >> 다음 스케쥴링에서는 다시 할당받을 수 있다.
		}
	}

	void unlock()
	{
		locked = false;
	}

private:
	//atomic 클래스의 compare_exchange_strong 함수의 의사코드라고 보면 편하다.
	//이거대로 구현해봤더니 실제로는 안되더라... 뭔 짓거리를 내부적으로 하나보다
	bool CAS(bool* current, bool expected, bool desired)
	{
		//current >> 현재 값. 정확한 현재 시점의 값을 알아야 하기 때문에 포인터를 이용한다.(글 쓰고 생각해보니 레퍼런스를 이용해도 될거같다)
		//expected >> 지금쯤이면 이런 값이겠지? 하는 값이 들어간다.
		//desired >> 내가 current에 덮어쓰고자 하는 값이 들어간다.

		if (*current == expected) //현재 값이 내가 예상한 값과 같으면
		{
			*current = desired; //내가 덮어쓰고자 하는 값으로 바꿔준 후 true를 리턴 
			return true;
		}
		else return false; //그렇지 않으면 false를 리턴
	}

private:
	atomic<bool> locked = false;
};

SpinLock spinLock;

void Add()
{
	for (int i = 0; i < 10000; i++)
	{
		lock_guard<SpinLock> guard(spinLock);
		num++;
	}
}

void Sub()
{
	for (int i = 0; i < 10000; i++)
	{
		lock_guard<SpinLock> guard(spinLock);
		num--;
	}
}

int main()
{
	thread t1(Add);
	thread t2(Sub);

	t1.join();
	t2.join();

	cout << num << endl;
}