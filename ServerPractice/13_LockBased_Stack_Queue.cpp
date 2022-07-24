/*
	지금까지 했던 멀티스레드를 다시 한번 되짚어보는 느낌이다.
	자료구조에 멀티스레드를 접목시켜보는 건데 이번엔 lock을 사용하는 자료구조이다.
*/

#include <iostream>
#include <thread>
#include <mutex>
#include <stack>
#include <queue>

using namespace std;

template<typename T>
class LockStack
{
public:
	LockStack() = default;
	LockStack(const LockStack&) = delete;
	LockStack& operator=(const LockStack&) = delete;

	void Push(T val)
	{
		std::lock_guard<std::mutex> lock(mutex);
		stack.push(move(val));
		cv.notify_one();
	}

	bool TryPop(T& val)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (stack.empty())
			return false;

		val = move(stack.top());

		stack.pop();
		return true;
	}

	void WaitPop(T& val)
	{
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock, [] {return stack.empty() == false; });
		val = std::move(stack.top());
		stack.pop();
	}
private:
	stack<T> stack;
	mutex mutex;
	condition_variable cv;
};

template<typename T>
class LockQueue
{
public:
	LockQueue() = default;
	LockQueue(const LockQueue&) = delete;
	LockQueue& operator = (const LockQueue&) = delete;

	void Push(T& val)
	{
		std::lock_guard<std::mutex> lock(mutex);

		queue.push(val);

		cv.notify_one();
	}

	bool TryPop(T& val)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (queue.empty())
			return false;

		val = std::move(queue.front());

		queue.pop();
		return true;
	}

	void WaitPop(T& val)
	{
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock, [] {return queue.empty() == false; });
		val = std::move(queue.top());
		queue.pop();
	}

private:
	std::queue<T> queue;
	std::mutex mutex;
	std::condition_variable cv;
};


LockStack<int> s;
LockQueue<int> q;

void Push()
{
	while (true)
	{
		int data = rand()*rand();
		q.Push(data);
		this_thread::sleep_for(5ms);
	}
}


void Pop()
{
	while (true)
	{
		int data = 0;
		if (q.TryPop(data))
		{
			printf("%d\n", data);
		}
	}
}

int main()
{
	thread t1(Push);
	thread t2(Pop);

	t1.join();
	t2.join();
}
