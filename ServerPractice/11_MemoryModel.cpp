/*
	멀티 쓰레드 프로그래밍을 하다보면 부딪히게 되는 난관중 race condition같은 문제가 있고
	이를 해결하기 위해서 원자성을 보장하기 위해 atomic클래스를 사용하기도 한다.
	이 때 atomic클래스는 내가 어떤 클래스를 사용하느냐에 따라서 lock을 쓰냐 안쓰나가 바뀐다.
	그런데 일반적으로 atomic클래스로도 해결하지 못하는 것중 가시성과 코드 재배치라는 문제가 있다고 한다.
	이 때, atomic 클래스에서 값을 저장하거나 꺼내올 때 어떤 메모리 정책을 사용할 것인가를 정해줄 수도 있다.
*/

#include <iostream>
#include <atomic>
#include <thread>

using namespace std;

atomic<bool> ready;
int value;

void Producer()
{
	value = 10;

	ready.store(true,memory_order::memory_order_seq_cst)
}

void Consumer()
{
	while (ready.load(memory_order::memory_order_seq_cst) == false) {}

	cout << value << endl;
}


int main()
{
	atomic<bool> flag = false;
	//flag = true;
	flag.store(true);
	//bool val = flag;
	bool val = flag.load();
	/*
		여기까지는 일반적으로 간단하게 atomic 클래스를 사용하는 방식이었다.
		그런데 store와 load 함수의 파라미터로 memory_order::memory_order_seq_cst가 기본적으로 들어가 있는 상태인 것이었다.
	*/

	//atomic클래스의 기본적인 사용방법

	//flag 값을 prev라는 변수에 넣고 flag값을 수정하고 싶다고 해보자
	{
		//일반적으로는
		bool prev = flag;
		flag = !flag;
		//와 같은 방식으로 하면 될 것이라고 생각하지만 이 역시 원자성이 보장되지 않는다.
		//그렇기 때문에 atomic클래스 내부에서 제공해주는 exchange함수를 이용해야한다.
		//이 함수는 flag내부의 값을 바꿔줌과 동시에 이전 값을 return 해준다.
		prev = flag.exchange(!flag); //이 방식이 맞음!!
	}

	//CAS 알고리즘
	{
		//CAS알고리즘은 비교한 후 조건이 맞으면 값을 수정하는 알고리즘이다.
		//이 알고리즘은 SpinLock을 구현할 때 사용될 수 있다.
		bool expected = false;
		bool desired = true;
		flag.compare_exchange_strong(expected, desired);
		//와 같은 방식으로 사용할 수 있고
		//이를 의사코드로 나타내보면
		if (flag == expected)
		{
			flag = desired; 
			return true;
		}
		else
		{
			return false;
		}
		//와 비슷하다. 단, atomic클래스 내부에서는 이 과정이 한번에 이루어지도록 처리되는 것이다.

		//참고로 atomic 클래스의 내부에는 compare_exchange_weak 라고 하는 함수도 있는데 이 함수는 strong과 거의 유사하다.
		//의사코드도 거의 동일하다고 생각하면 되는데 하드웨어적인 인터럽션이 발생했을 때 true 조건을 만족하더라도 false가 나오게끔 만들어져 있다.
		//strong은 weak에서 좀 더 많은 코드들이 추가되어 성공을 확실하게 보장하는(?) 그런 녀석인것 같다. 그래서 부하가 좀 더 크다.
	}


	/*
		메모리 정책에는 크게 세가지가 있다.
		1. Sequentially Consistent (일반적으로 뒤에 seq_cst가 붙음)
		   가시성 문제와 코드 재배치 문제가 바로 해결된다.
		2. Acquire-Release (consume, acquire, release, acq_rel)
		   딱 중간 정도의 엄격성(?), 사용하기 위해서는 release와 acquire를 한 쌍으로 맞춰줘야 한다.
		   그러고 나면 release이전에 명령어들이 release뒤로 가도록 하는 재배치가 금지된다.
		   그리고 aquire로 같은 변수를 읽는 쓰레드가 있을 때 acquire이후의 명령어들이 이전으로 재배치 되는것을 막을 뿐 아니라
		   release 이전의 변수들의 가시성 또한 확보할 수 있다.
		3. Relaxed (relaxed)
		   컴파일러에게 최대한의 자유를 준것이기 때문에 지 멋대로 코드도 바꾸고 난리가 난다.거의 활용하지 않는다고 함

		위로 올라갈 수록 엄격한 기준을 적용하게 된다.
		엄격해질 수록 컴파일러가 자체적으로 코드를 최적화 시킬 여지가 적기 때문에 프로그래머 입장에서는 직관적인 코드를 볼 수 있다.
		밑으로 내려가면 컴파일러 입장에서 최적화를 할 여지가 많아지기 때문에 코드가 직관적이지 않다는 문제가 있다. 이게 나중에 디버깅에 큰 영향을 끼칠지도?
	*/


	/*
		다음과 같은 코드가 있다고 하자.
		이 때, 1번과 2번 방식은 value값이 무조건 10으로 나온다고 보장할 수 있다. =>코드 재배치가 완전 없거나 코드를 읽어봤을 때 변수와 관련된 재배치가 없기 때문에
		하지만 3번은 코드 재배치가 자유롭게 일어나서 '무조건' 10이 나온다고는 할 수 없다.
	*/
	{
		ready = false;
		value = 0;
		thread t1(Producer);
		thread t2(Consumer);
	}

	/*
		여담으로 인텔이나 암드 CPU같은 경우에는 칩 설계 자체가 Sequentially Consistent로 되어있다고 한다.
		그렇기 때문에 Sequentially Consistent방식을 사용해도 큰 부하가 없지만
		ARM같은 경우(MS의 서피스 2022 같은 제품들에 들어가는 CPU) 꽤 차이가 있다고 한다.
	*/
}