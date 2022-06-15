/*
	커널을 중개인으로 활용하는 Event가 참 좋아보이긴 한다,
	하지만 이전 코드에서 sleep을 하는 시간이 길지 않고 빠르게 빠르게 이벤트가 실행된다면 어떨까?
	단순하게 생각해서 이전 코드에서 sleep_for를 제거한다면 데이터를 넣고 이벤트를 실행시키면 데이터를 하나 뺴는 식일것 같지만
	결과는 생각과 다르게 데이터가 엄청나게 쌓이게 된다.
	이는 WaitForSingleObject와 lock사이에 짧은 텀이 있어서 그 사이에 Producer쪽에서 다시 락을 걸어버려서 발생하는 것이다.
	그렇다고 Producer내부의 SetEvent의 위치를 바꿔주는 것 역시 정답이 아니다. Consumer내부에서 lock에 걸려서 대기하는 시간이 생기기 때문이다.
*/

#include <iostream>
#include <mutex>
#include <thread>
#include <queue>

using namespace std;

mutex m;
queue<int> q;
condition_variable cv;
/*
	여기서 새롭게 등장하는 것이 Condition Variable이다.
	Condition Variable은 User Level Object이고 이는 이걸로는 다른 프로그램과 통신할 수 없다는 것을 말한다.
	(커널 오브젝트였던 Event는 다른 프로그램과 통신할 수 있다고 한다.)
	Condition Variable은 mutex와 짝지어서 움직이며 이것이 작동하는 4가지 단계가 있는데
	1. lock을 잡고
	2. 공유변수를 수정한 후
	3. lock을 풀고 (여기까진 mutex와 동일하다)
	4. 조건 변수를 통해서 다른 스레드들에게 알린다.
*/

void Producer() //데이터를 수신하여 큐에 집어 넣고
{
	while (true)
	{
		{
			unique_lock<mutex> lock(m);
			q.push(rand());
		}

		cv.notify_one(); //잠들고 있는 하나의 스레드를 깨운다. one대신 all이 붙는다면 모든 스레드를 깨운다.
	}

}

void Consumer() //데이터를 사용하기 위해 큐에서 꺼낸다.
{
	while (true)
	{
		{
			unique_lock<mutex> lock(m);
			cv.wait(lock, []() {return q.empty() == false; });//cv는 조건을 만족할 때 까지만 대기한다. 이 때 조건은 함수의 주소가 될 수도 있고 람다가 될 수도 있다. 호출 가능한 형태면 다 가능함.
			/*
				이 때, cv는 lock과 함께 2가지 순서의 동작을 한다.
				1. lock을 잡는다. lock이 잡혀 있지 않다면 굳이 lock을 또 하진 않겠지만 일단 시도는 한다.
				2. 조건을 확인해서 조건을 만족하면 아래 코드를 계속 진행하고 만족하지 않으면 lock을 풀고 대기상태에 들어간다.
				만약 notify_one에 의해서 실행되게 되면 위 두개의 단계를 다시 실행한다.

				근데 Producer에서 notify_one을 하면 내가 원하는 조건이 항상 참인건 아닐까? 하는 생각을 할 수도 있다.
				하지만 항상 참일 수는 없는데 그 이유는 Producer에서 unlock을 한 후 notify를 하는 사이에는 약간의 텀이 있고 그 사이에 다른 스레드에서 공용변수를 사용할 수도 있기 때문.
				이런 현상을 Spurious Wakeup 이라 한다.
				그렇기 때문에 확실하게 하기 위해서 추가적인 조건식을 넣어준다.
			*/

			{
				int data = q.front();
				q.pop();
				printf("%d\n", data);
			}
		}
	}
}

int main()
{
	srand(time(0));

	thread t1(Producer);
	thread t2(Consumer);

	t1.join();
	t2.join();

	/*
		Condition Variable은 C++11 의 표준에 들어오면서 윈도우, 리눅스에 관계없이 사용할 수 있다.
		하지만 내가 어떤 OS를 쓸지 확실하게 알고 그것이 변할 가능성이 아예 없다면
		각 운영체제에 최적화된 API가 있는지, 있다면 표준과 비교했을 떄 얼마만큼의 속도 차이가 있는지 등을 생각해볼 필요가 있다.
		윈도우같은 경우는 윈도우API를 쓰면 빠르게끔 MS가 뭔 짓거리를 해놨다더라...
	*/
}