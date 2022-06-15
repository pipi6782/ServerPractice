/*
	atomic클래스의 연산은 무겁다는 특성때문에 잘 사용하지 않는다.
	그렇기 때문에 일반적인 경우에는 mutex를 이용하는 경우가 많다.
	mutex를 쉽게 말하자면 문이라고 볼 수 있는데
	공통 변수가 있는 방에 들어가 작업을 하기 위해 문을 열고 들어간 후 문을 잠궈버리는 것이다.
	그렇게 되면 다른 스레드들은 문이 잠겨있기 때문에 공통 변수가 있는 방에서 작업을 할 수가 없고 기다려야 한다.
	단, 주의해야 할 것은 문을 잠궜으면 "반드시" 문을 열어야 한다.
	그렇지 않으면 다른 스레드들은 문이 열릴때 까지 무한정 기다릴것이고 이를 Deadlock이라 한다.
*/

#include<iostream>
#include<thread>
#include<mutex>

using namespace std;

mutex m;
int num = 0;

void Add()
{
	for (int i = 0; i < 10000; i++)
	{
		lock_guard<mutex> guard(m);
		num++;
	}
}

void Sub()
{
	for (int i = 0; i < 10000; i++)
	{
		lock_guard<mutex> guard(m);
		num--;
	}
}

/*
	위 함수에서는 lock_guard 를 사용했다.
	mutex를 사용하게 되면 반드시 lock과 unlock의 쌍을 맞춰줘야 하는데
	프로그래머도 사람인지라 lock을 엄청나게 많이 하게 되면 한번정도는 실수로 lock을 해제하지 못한다.
	이는 곧 Deadlock으로 이어지기 때문에 lock을 하면 unlock을 자동으로 할 수 있게 해야한다.
	이를 할 수 있게 해주는게 lock_guard이고 lock과 unlock(소문자 주의)함수만 있다면 어떤 클래스던 사용할 수 있다.
*/

int main()
{
	thread t1(Add);
	thread t2(Sub);

	t1.join();
	t2.join();
	/*
		위 코드의 결과 역시 0이 나오게 된다.
		num에 연산을 할 때 lock을 하기 때문에 연산중에 다른 연산을 시도할 수 없기 때문이다.
		하지만 lock을 사용하게 되면 다른 스레드들은 lock이 해제되기 전 까지는 계속 기다리게 되는데
		이는 서버가 높은 성능을 요구하게 되면 문제가 될 수 있다.
		예산과 성능은 한정돼있기 때문이다.
	*/
}