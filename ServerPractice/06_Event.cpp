/*
	이벤트를 이용하는 방식은 별도의 중재자를 통하여 나의 차례가 왔는지 확인하는 방식이다.
	중재자는 커널이고 커널은 스레드에게 정해진 순서가 오면 순서를 알려주는 원리이다.
	이벤트는 boolean처럼 간단하게 생각해볼 수도 있는데 "내 차례다." or "내 차례가 아니다." 두가지만으로
	스레드의 실행 여부를 결정한다고 볼 수 있다.(아주 아주 아주 간단하게 생각해보면 이렇다)
	이벤트는 Auto Reset Event, Manual Reset Event의 두가지로 구분할 수 있고 C#,JAVA 수준에서는 알아서 구분되지만
	C++수준에서는 내가 인자를 어떻게 주냐에 따라서 바뀔 수 있다.
	작동원리를 간단하게 풀어보면 두 스레드 A,B가 있고 공유자원을 A가 사용중일 때 B역시 공유자원을 사용하고 싶어한다.
	1. 이 ?? B는 커널에게 달려가서 "쟤 쓰던거 다 쓰면 나 좀 불러줘." 하는 부탁을 하게 된다.
	2. 시간이 지난 후 볼일을 다 본 A는 커널에게 다 썼다는 신호를 준다.
	3. 그 신호를 받은 커널은 B에게 달려가 "아까 쓰던 놈 다 썼으니까 빨리 가서 써."하는 신호를 준다.
	Event 방식의 장점은 스핀락처럼 무식하게 계속 기다리지도 않고 Sleep처럼 운에 맡기면서 쉬지도 않으니 효율적인 처리가 가능하다는 점이다.
	하지만 단점이라고 한다면 또 다른 중재자가 필요한 셈이니 추가적인 리소스를 사용하게 된다는 것이다. 그렇기 때문에 아무때나 쓸 수 있는 것은 아니고 꼭 필요할 때만 써야 한다.
*/

#include <iostream>
#include <thread>
#include <mutex>
#include <Windows.h> //윈도우 api를 이용하여 이벤트를 구현한다.
#include <queue> //공용 데이터를 관리할 큐를 만든다.

using namespace std;

mutex m;
queue<int> q;
HANDLE handle;

void Producer() //데이터를 수신하여 큐에 집어 넣고
{
	/*while (true)
	{
		{
			unique_lock<mutex> lock(m);
			q.push(rand());
		}
		this_thread::sleep_for(50ms);
	}
	이 방식은 50ms 마다 값이 주기적으로 들어온다는 것이 보장된 상황이다.
	이런 상황이라면 Consumer에서 무한루프를 계속 돌면서 체크하고
	값을 꺼내오는 것도 괜찮을 수도 있다.
	하지만 데이터가 정말 어쩌다가 한번씩 들어온다면 어떨까>
	그때도 무한루프를 도는 의미없는 행동을 할 건가?
	*/

	while (true)
	{
		{
			unique_lock<mutex> lock(m);
			q.push(rand());
		}

		//데이터를 넣어서 뭔가를 처리할 수 있는 상황이 되면 이벤트를 실행시킨다(?).
		if(handle != NULL)
			SetEvent(handle);


		this_thread::sleep_for(500ms);
	}

}

void Consumer() //데이터를 사용하기 위해 큐에서 꺼낸다.
{
	while (true)
	{
		//handle에 해당하는 이벤트가 발생하기를 무한정 기다린다. >>무한 루프를 도는것은 아니기 때문에, cpu자원을 소모하는 상태는 아님
		WaitForSingleObject(handle, INFINITE);
		//esetEvent(handle); 이벤트를 생성할 때 자동으로 설정했기 때문에 이 코드는 필요가 없다. \
		하지만 이 코드는 이벤트를 수동으로 설정했을 때 Signal 상태의 이벤트를 Non-Signal 상태로 전환시켜주는 역할을 한다.
		{
			unique_lock<mutex> lock(m);
			if (q.empty() == false)
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
	
	/*
		이벤트 = 커널에서 사용하는 오브젝트.
		커널 오브젝트들은 공통적으로
		Usage-Count : 오브젝트를 얼마나 많이 사용하고 있는가(자주 사용하냐는 의미가 아님),
		signal or non-signal을 구분하는 데이터,
		Auto / Manual 을 구분하는 값 이 있다.
	*/ 
	handle = CreateEvent(
		NULL,//첫번째 파라미터는 보안속성과 관련된 파라미터다. 지금은 필요없으니 null로 설정한다.
		false,//리셋은 자동? 수동?
		false,//이벤트의 초기 상태 = true면 쓸 수 있고 false면 쓸 수 없다.
		NULL //이벤트의 이름, 지금은 그냥 null
	);
	/*
		HANDLE은 그냥 void 포인터다.
		typedef로 이름을 붙여줘서 어떤 역할을 한다는 느낌을 줬을 뿐이라고 생각하는데
		그 역할은 일종의 번호표 같은 것이다.
		내가 자원을 사용할 수 있는 상황이 오면 이 번호표를 보여주고 자원을 사용하는 것이다.
	*/
	thread t1(Producer);
	thread t2(Consumer);

	t1.join();
	t2.join();

	if(handle != NULL)
		CloseHandle(handle);
}