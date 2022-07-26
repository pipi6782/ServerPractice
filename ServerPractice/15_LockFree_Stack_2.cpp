#include <iostream>
#include <thread>
#include <stack>
#include <queue>
#include <atomic>

using namespace std;

//template<typename T>
//class LockFreeStack
//{
//	struct Node
//	{
//		Node(const T& val) : Data(make_shared<T>(val)), Next(nullptr) {}
//
//		shared_ptr<T> Data;
//		shared_ptr<Node> Next;
//	};
//
//public:
//	void Push(const T& value)
//	{
//		shared_ptr<Node> node = make_shared<Node>(value);
//		node->Next = head;
//
//		//이제 head가 스마트 포인터로 바꼈다. 그럼 이제 CAS를 쓰지 못하는건가? 싶을 수도 있다.
//		//하지만 사용 가능하다. atomic_compare_exchange_weak 와 같은 함수를 사용하면 된다.
//		while (atomic_compare_exchange_weak(&head, &node->Next, node) == false)
//		{
//
//		}
//	}
//
//	shared_ptr<T> TryPop(T& value)
//	{
//		//shared_ptr을 그냥 바로 가져오는 것 역시 원자적이지 않다.
//		//reference count가 영향을 받을 수 있기 때문.
//		//이 역시 atomic_load 함수를 이용해서 받아온다.
//		shared_ptr<Node> oldHead = atomic_load(&head);
//
//		while (oldHead && atomic_compare_exchange_weak(&head, &oldHead, oldHead->Next) == false)
//		{
//		}
//
//		if (oldHead == nullptr)
//			return shared_ptr<T>();
//
//		return oldHead->Data;
//	}
//
//
//
//private:
//	/*
//		Pop에서 그냥 멋도 모르고 삭제를 하면 while문에서 CAS알고리즘으로 검사를 할 때
//		삭제한 포인터를 쓰려다가 프로그램이 터질 수도 있다.
//		lock free는 요즘까지도 계속 연구되고 특허까지 나오고 있는 분야인 만큼 방식들이 계속 나오고 있는데
//		이번 방법은 C#,JAVA의 GC의 느낌처럼 스마트포인터를 사용하는 것이다.
//	*/
//	shared_ptr<Node> head;
//};

/*
	이 코드의 아주 치명적인 문제가 있다.
	문제는 바로 이 코드가 lock free 방식이 아니라는 것이다.
	그 이유는 shared_ptr 이 lock free로 동작하지 않기 때문
	지금부터는 그런 문제들을 해결한 코드를 새로 만든다.
	shared_ptr의 레퍼런스 카운팅을 직접 구현하는 방식이다.
*/

//물론 이 방식이 lock free라고 단정지을 수는 없다.
//사용자의 pc 사양에 따라서 lock free여부가 바뀌기 때문.
//일단은 "이건 lock free방식이다." 라는 전제하에 진행한다.
template<typename T>
class LockFreeStack
{
	struct CountedNodePtr;

	struct Node
	{
		Node(const T& val) : Data(make_shared<T>(val)) {}

		shared_ptr<T> Data;
		CountedNodePtr Next;
		atomic<int> internalCount = 0;
	};

	struct CountedNodePtr
	{
		int externalCount = 0;
		Node* node = 0;
	};

public:
	void Push(const T& value)
	{
		CountedNodePtr node;
		node.node = new Node(value);
		node.externalCount = 1;
		node.node->Next = head;
		//여기까지는 아무 문제 없다.
		//스택 영역에서만 다루기 때문.
		while (head.compare_exchange_weak(node.node->Next, node) == false)	//경합을 통해서 Push를 하겠다.
		{}

	}

	shared_ptr<T> TryPop()
	{
		//기존 헤드를 가져온다. 근데 이 상태로는 온전하게 사용할 수 있다는 보장이 없다.
		CountedNodePtr oldHead = head;
		while (true)
		{
			//참조할 수 있는 권한을 획득한다. externalCount가 현 시점에서 1만큼 커진 값을 가진 애가 이긴다.
			//Increase만 하고 Decrease는 하지 않는다.
			//은행에서 번호표를 뽑는 느낌
			IncreaseHeadCount(oldHead);
			//지금 상태는 externalCount가 최소 2이상일꺼니 삭제하지 않는다.

			Node* ptr = oldHead.node;
			if (ptr == nullptr)
				return shared_ptr<T>();

			//소유권 획득 (ptr->Next 로 head를 바꾼 애가 이긴다.)
			if (head.compare_exchange_strong(oldHead, ptr->Next))
			{
				//이곳에 들어왔단 것은 head와 oldHead가 완전히 일치한다는 것을 의미함
				//여기 들어온 순간 head의 값은 바뀌기 때문에 다른 스레드가 이 조건문 전 까지 왔다 하더라도 head와 oldHead는 다른 상황임
				shared_ptr<T> result;
				result.swap(ptr->Data);

				//external은 늘어나기만 하고 internal은 줄이기만 할거다.

				//나 말고 다른 스레드가 쓰고 있는지 확인
				const int countIncrease = oldHead.externalCount - 2;
				if (ptr->internalCount.fetch_add(countIncrease) == -countIncrease)
					delete ptr;

				return result;
			}
			else if(ptr->internalCount.fetch_sub(1) == 1)
			{
				//참조는 할 수 있지만 소유는 할 수 없다.
				//fetch_sub로 얻은 빼기 이전의 값이 1이면
				//나만 남은 것이기 때문에 내가 삭제한다.
				delete ptr;
			}
		}
	}

private:
	void IncreaseHeadCount(CountedNodePtr& oldCounter)
	{
		while (true)
		{
			//newCounter은 우리가 원하는 externalCount가 딱 1만 늘어난 상태를 의미함
			CountedNodePtr newCounter = oldCounter;
			newCounter.externalCount++;
			//그 상태에서 계속 CAS알고리즘으로 비교를 한 후 조건에 부합하면 반복문 탈출
			if (head.compare_exchange_strong(oldCounter, newCounter))
			{
				//반복문 조건에 부합하니 지금은 안전하게 Count를 수정할 수 있다.
				oldCounter.externalCount = newCounter.externalCount;
				break;
			}
		}
	}



private:
	atomic<CountedNodePtr> head;
};

LockFreeStack<int> s;

void Push()
{
	while (true)
	{
		int data = rand() * rand();
		s.Push(data);
		this_thread::sleep_for(1ms);
	}
}


void Pop()
{
	while (true)
	{
		auto data = s.TryPop();
		if(data != nullptr)
			printf("%d\n", *data);
		
	}
}

/*
	lock free 프로그래밍은 상당히 어려운 분야다.
	생각대로 해서 절대 안되고 로직에 구멍이있는지도 철저하게 확인해야한다.
*/


int main()
{
	thread t1(Push);
	thread t2(Push);
	thread t3(Pop);
	thread t4(Pop);

	t1.join();
	t2.join();
	t3.join();
	t4.join();
}