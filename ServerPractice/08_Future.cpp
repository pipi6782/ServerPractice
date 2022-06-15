/*
	앞에서 Event, Condition Variable과 같은 방법들을 찾아봤는데 이것들만 있어도 나름 쓸모있어 보인다.
	근데 단발성 이벤트를 구현한다면 어떨끼?
	굳이 Event, Condition Variable를 사용해야할까?
	그럴 때 사용할 수 있는 것이 future인데 그렇게 자주 쓸 일은 없다고 한다.
*/

#include <iostream>
#include <mutex>
#include <thread>
#include <future>

using namespace std;

/*
	아래 함수와 같은 아주 간단한 기능이 있다 해보자.
*/
__int64 Calc()
{
	__int64 sum = 0;
	for (int i = 0; i <= 1000000; i++)
		sum += i;

	return sum;
}

void PromiseWorker(std::promise<string>&& promise)
{
	promise.set_value("asdasd");
}

void TaskWorker(std::packaged_task<__int64(void)> task)
{
	task();
}

int main()
{
	/*
		지금까지 이런 방식을 동기방식이라고 불렀다 (A를 하고 다 끝난 후 B를 하는 방식)
		이 방식은 지금 하고있는 작업이 오래 걸릴 경우 이 작업이 다 끝날 때 까지 오오오오오래 기다려야 한다는 단점이 있다.
		세탁기를 빨래를 하고 전기밥솥으로 밥을 지은 다음에 게임을 하는데 빨래가 다 돌때까지 기다렸다가 밥을 지을 필요는 없으니까 ㅇㅇ
		세탁기로 빨래를 돌리고 빨래가 돌아가는 동안 전기밥솥으로 밥을 지으면서 게임을 하면 되는것이다.
		이처럼 내가 지금 하고자 하는 작업이 생각보다 오래 걸릴거같을 때 스레드를 활용하는 비동기 방식을 사용한다.
		그런데 중요도가 그렇게 높지 않은 작업에 굳이 스레드를 쓰는 일이 있는게 좋을까?
		그런 상황을 대비해서 있는 것이 future이다.
	*/

	//비동기 방식으로 알아서 작업을 하게 시킨다.\
	future에 줄 수 있는 옵션은 3가지가 있는데 \
	1. deffered : 지연해서 실행시킨다. (get을 할 때 함수를 실행시키는 것과 다를게 없음. 사장이 결과물 보자고 할 때 잠깐 기다려달라 하고 급하게 만드는 느낌)\
	2. async : 별도의 스레드를 만든 후 작업을 실행시킨다.\
	3. deffered | async : 두 방식중 알아서 선택하게 한다. (니 알아서 해라 느낌)\
	세가지가 있다. \
	근데 deffered 방식이 필요할 때가 있는데 클라이언트가 서버에 간단한 작업을 요청했는데 \
	서버는 더 중요한 작업을 처리중이라 하면 일단 그 작업은 뒤로 미뤄두는 느낌이다.
	std::future<__int64> future = std::async(std::launch::async, Calc);
	{
		//여기서는 내가 해야하는 일을 하고 있다가

		//잠깐 일이 어떻게 진행되고 있나 살펴보고 싶다면
		future_status status = future.wait_for(1ms);

		class MyClass
		{
		public:
			string GetInfo()
			{
				return "Info String";
			}
		};

		//만약 어떤 클래스의 멤버함수를 future로 실행하고 싶다면\
		파라미터에 함수의 주소 뿐만 아니라 어떤 객체의 멤버함수를 실행할 것인지도 넣어줘야 한다.\
		객체들 마다 멤버변수의 값이 전부 다 다를것이기 때문!!
		std::future<string> future2 = std::async(std::launch::async, &MyClass::GetInfo, MyClass());
	}
	//내가 시켰던 일의 결과물이 필요할 때 호출하면 된다. (프리랜서를 고용한 느낌으로 보면 쉽다.)
	__int64 sum = future.get();
	
	{
		//future를 만드는 두번째 방법은 promise를 이용하는 것이다.
		//이 때 promise는 future와 1대1 대응의 관계에 있다고 볼 수도 있다.
		//여러 스레드끼리 통신해야하는 상황에서 굳이 공용변수를 또 만들지 않고 promise를 통해서 데이터를 전달하면
		//그 promise에 딸려있는 future로 데이터를 받아오는 것이다.
		//개인적인 생각으로는 promise를 이용하면 별도의 스레드를 코딩으로 하나 더 만들어야 하는것 같다... 좀 불편하긴 할듯
		std::promise<string> promise;
		std::future<string> future = promise.get_future();

		thread tt(PromiseWorker, std::move(promise));
		string result = future.get();

		tt.join();
	}

	{
		//future를 만드는 마지막 방법은 packaed_task를 이용하는 방법이다.
		//이것을 사용할 때는 타입을 정해줄때 input과 output타입을 전부 다 맞춰줘야 한다.
		//이것 역시 promise와 비슷하게 task와 future이 1대1로 엮여있다고 볼 수 있다.
		std::packaged_task<__int64(void)> task(Calc);
		std::future<__int64> future = task.get_future();

		thread tt(TaskWorker, std::move(task));
		__int64 sum = future.get();

		tt.join();
	}

	/*
		결론적으로 future은 condition_variable이나 mutex를 사용하지 않고
		좀 단순하게 일을 처리할 수 있을 때 사용하면 좋다.
		future의 3가지 방식을 정리하자면
		1. async : 내가 원하는 함수 하나를 비동기적으로 실행시킨다.
		2. promise : promise를 통해 future의 결과물을 받는다.
		3. packaged_task : 원하는 함수의 실행결과를 task로 받아준다.

		정도가 되겠다.
		promise와 task차이가 크게 느껴지지 않는데
		만약 파라미터에 따라서 결과가 달라진다면 그 땐 반드시 task를 써야하지 않을까 싶다.
	*/
}