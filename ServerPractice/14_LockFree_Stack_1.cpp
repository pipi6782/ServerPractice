/*
	여태껏 멀티스레드 프로그래밍을 할 땐 동기화를 위해 lock을 걸어주는 방식이 일반적이었다.
	하지만 이번엔 lock을 사용하지 않으면서 동기화를 시켜줄 것이다.
	Lock Free 방식이 lock을 사용하지 않기 때문에 성능이 더 좋아 보이지만
	동기화를 위해서 많은 작업들이 필요하고 그러다보니 막상 성능은
	lock을 쓴 것 만도 못한 상황이 생길때가 있다.
*/

#include <iostream>
#include <thread>
#include <stack>
#include <queue>
#include <atomic>

using namespace std;

template<typename T>
class LockFreeStack
{
	struct Node
	{
		Node(const T& val) : Data(val), Next(nullptr) {}

		T Data;
		Node* Next;
	};

public:
	void Push(const T& value)
	{
		Node* node = new Node(value);
		node->Next = head;
		while (head.compare_exchange_weak(node->Next, node) == false)
		{
			/*
				근데 이건 스핀락과 유사하다고 봐도 되는것 아닌가?
				Lock Free 프로그래밍이 말 그대로 mutex.lock() 을 안써서 Lock Free인가?
			*/
			/*
				인프런 루키스님 답변
			
				Lock-Free는 Lock을 명시적으로 사용하지 않는다는 것은 맞지만
				어차피 쓰레드 경합 방지 차원에서 CAS 류의 코드는 들어가야 합니다.

				LockFreeQueue의 경우 데이터가 충분히 있다면 Push/Pop이 서로 영향을 주지 않으므로
				Lock을 잡는 방식에 비해 조금 이점이 있을 수는 있긴 합니다.
				그러나 서버에서 [우리는 LockFree 구조!!]라고 자랑한다고
				그게 정말 효율적이라는 보장은 없고 오히려 Lock을 잡는게 좋을 수도 있습니다.
				LockFree 외에도 WaitFree라는 용어도 종종 등장하는데
				무한 뺑뺑이를 돌면서 CAS를 하지 않는 상황을 얘기합니다.
				
				마지막으로 Lock을 구현하는 여러가지 방식 중에 스핀락도 포함이 되는 것이라
				[스핀락은 Lock이 아닌가?]는 맞지 않습니다.

			*/
		}
	}

	bool TryPop(T& value)
	{
		popCount++;
		Node* oldHead = head;
		while (oldHead && head.compare_exchange_weak(oldHead, oldHead->Next) == false)
		{

		}

		if (oldHead == nullptr)
		{
			popCount--;
			return false;
		}

		value = oldHead->Data;
		TryDelete(oldHead);
		return true;
	}

	void TryDelete(Node* oldHead)
	{
		//나 외에 누가 있는가?
		if (popCount == 1)
		{
			//사용중인 스레드가 자기 자신뿐

			//혼자인 김에 삭제 예약된 다른 데이터도 삭제
			//1. 데이터를 분리하고
			Node* node = pendingList.exchange(nullptr);
			//2. 카운트를 체크하고
			if (--popCount == 0)
			{
				//중간에 끼어든 스레드가 없다.
				//3. 이제서야 삭제한다.
				DeleteNodes(node);
			}
			else if(node != nullptr)
			{
				//다른 스레드가 껴들었기 때문에 원복
				ChainPendingNodeList(node);
			}

			delete oldHead;
		}
		else
		{
			//다른 스레드도 이 노드를 사용중
			ChainPendingNode(oldHead);
		}
	}

	void ChainPendingNodeList(Node* first, Node* last)
	{
		last->Next = pendingList;
		//멀티스레드이기 때문에 CAS알고리즘으로 계속 비교를 해야한다.
		while (pendingList.compare_exchange_weak(last->Next, first) == false)
		{}
	}

	void ChainPendingNodeList(Node* node)
	{
		Node* lastNode = node;
		while (lastNode->Next)
			lastNode = lastNode->Next;

		ChainPendingNodeList(node, lastNode);
	}

	void ChainPendingNode(Node* node)
	{
		ChainPendingNodeList(node, node);
	}

	void DeleteNodes(Node* node)
	{
		while (node)
		{
			Node* next = node->Next;
			delete node;
			node = next;
		}
	}

private:
	atomic<Node*> head;
	/*
		Pop에서 그냥 멋도 모르고 삭제를 하면 while문에서 CAS알고리즘으로 검사를 할 때
		삭제한 포인터를 쓰려다가 프로그램이 터질 수도 있다.
		lock free는 요즘까지도 계속 연구되고 특허까지 나오고 있는 분야인 만큼 방식들이 계속 나오고 있는데
		그 중 하나는 shared_ptr의 ref count를 비슷하게 따라하는 것 이다.
	*/
	//1. Pop을 실행중인 쓰레드 갯수를 체크
	atomic<int> popCount = 0;	//Pop을 실행중인 쓰레드 갯수
	atomic<Node*> pendingList;	//삭제되어야 할 노드들(제일 첫번째)
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
		int data = 0;
		if (s.TryPop(data))
		{
			printf("%d\n", data);
		}
	}
}

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