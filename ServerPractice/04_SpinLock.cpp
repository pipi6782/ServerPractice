/*
	Lock을 사용한다는건 사용할 수 있는 조건이 될 때까지 기다리는 것이다.
	Spin Lock은 내가 공통 변수를 사용할 수 있는 조건이 될때까지 무작정 기다리는 것이다.
*/

#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

int num = 0;

/*
class SpinLock
{
public:
	void lock()
	{
		while (locked == true){}
		locked = true;
	}

	void unlock()
	{
		locked = false;
	}

private:
	bool locked = false;
};
*/
/*
	위 SpinLock 클래스는 정상적으로 lock과 unlock을 실행하여 정상적인 결과값을 얻어낼 수 있을것 처럼 보인다.
	하지만 위 코드로 실행해보면 정상적인 값을 얻어내지 못하게 되는데 그 이유는 두가지이다.
	1. Debug모드에서는 그렇지 않지만 Release모드로 넘어가는 순간 while(locked == true){} 는 컴파일러 입장에서는 아무런 역할을 하지 않는다고 생각하고
	locked와 관련된 코드들을 최적화 과정에서 생략해버린다. 그렇게 되면 사실상 lock 함수는 locked = true 만 남아있는 셈이다.
	이 문제를 해결하기 위해선 lock 변수를 선언할 때 volatile 키워드를 붙여주는 것인데 이 키워드의 역할은 컴파일 과정에서 위 변수에 대한 최적화를 생략하는 것이다.

	2. locked = false 가 된 찰나의 순간에 다수의 스레드가 접근할 수 있기 때문이다. 그렇게 되면 공통변수에 동시에 작업을 하는 상황이 발생하고 이는 atomic한 연산이 불가능하게 되는 것이다.
	이 문제를 해결하기 위해 사용되는 알고리즘이 CAS(Compare And Swap)알고리즘이다.
*/


class SpinLock
{
public:
	void lock()
	{
		bool expected = false;
		while (locked.compare_exchange_strong(expected,true) == false) {
			expected = false;
			/*이 lock함수가 의미하는 것은 어떤 스레드가 CAS함수를 실행하는 "바로 그 순간에" locked변수의 값이 false면 true로 교체하는 것*/ 
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

}