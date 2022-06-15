/*
	Lock�� ����Ѵٴ°� ����� �� �ִ� ������ �� ������ ��ٸ��� ���̴�.
	Spin Lock�� ���� ���� ������ ����� �� �ִ� ������ �ɶ����� ������ ��ٸ��� ���̴�.
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
	�� SpinLock Ŭ������ ���������� lock�� unlock�� �����Ͽ� �������� ������� �� �� ������ ó�� ���δ�.
	������ �� �ڵ�� �����غ��� �������� ���� ���� ���ϰ� �Ǵµ� �� ������ �ΰ����̴�.
	1. Debug��忡���� �׷��� ������ Release���� �Ѿ�� ���� while(locked == true){} �� �����Ϸ� ���忡���� �ƹ��� ������ ���� �ʴ´ٰ� �����ϰ�
	locked�� ���õ� �ڵ���� ����ȭ �������� �����ع�����. �׷��� �Ǹ� ��ǻ� lock �Լ��� locked = true �� �����ִ� ���̴�.
	�� ������ �ذ��ϱ� ���ؼ� lock ������ ������ �� volatile Ű���带 �ٿ��ִ� ���ε� �� Ű������ ������ ������ �������� �� ������ ���� ����ȭ�� �����ϴ� ���̴�.

	2. locked = false �� �� ������ ������ �ټ��� �����尡 ������ �� �ֱ� �����̴�. �׷��� �Ǹ� ���뺯���� ���ÿ� �۾��� �ϴ� ��Ȳ�� �߻��ϰ� �̴� atomic�� ������ �Ұ����ϰ� �Ǵ� ���̴�.
	�� ������ �ذ��ϱ� ���� ���Ǵ� �˰����� CAS(Compare And Swap)�˰����̴�.
*/


class SpinLock
{
public:
	void lock()
	{
		bool expected = false;
		while (locked.compare_exchange_strong(expected,true) == false) {
			expected = false;
			/*�� lock�Լ��� �ǹ��ϴ� ���� � �����尡 CAS�Լ��� �����ϴ� "�ٷ� �� ������" locked������ ���� false�� true�� ��ü�ϴ� ��*/ 
		}
	}
	
	void unlock()
	{
		locked = false;
	}

private:
	//atomic Ŭ������ compare_exchange_strong �Լ��� �ǻ��ڵ��� ���� ���ϴ�.
	//�̰Ŵ�� �����غô��� �����δ� �ȵǴ���... �� ���Ÿ��� ���������� �ϳ�����
	bool CAS(bool* current, bool expected, bool desired)
	{
		//current >> ���� ��. ��Ȯ�� ���� ������ ���� �˾ƾ� �ϱ� ������ �����͸� �̿��Ѵ�.(�� ���� �����غ��� ���۷����� �̿��ص� �ɰŰ���)
		//expected >> �������̸� �̷� ���̰���? �ϴ� ���� ����.
		//desired >> ���� current�� ������� �ϴ� ���� ����.

		if (*current == expected) //���� ���� ���� ������ ���� ������
		{
			*current = desired; //���� ������� �ϴ� ������ �ٲ��� �� true�� ���� 
			return true;
		}
		else return false; //�׷��� ������ false�� ����
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