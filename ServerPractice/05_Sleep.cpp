/*
	���뺯���� �������� ������̶�� ���� �ڸ��� ���ư��� ���� �� �� �ִ� �ٸ� ���� �ϰ� �ִٰ�
	���� �ð��� ���� �� �ٽ� ����� �õ��غ��� ����̴�.
	�� ����� ��� ������ ������� �ʰ� �ٸ� ���� �ϰ� ���� �� �ִٴ� ������ ������
	���� �ٽ� �� �۾��� �Ϸ� ���ư��ٰ� �ٽ� ����� �õ��ϴ� ���̿� �ٸ� �����尡 ����ϰ� ���� ���� �ִٴ� ������ �ִ�.
	�� ����� �ü���� �����층�� ū ������ �ִ�.
	�����층 : A���μ���(������)�� �ڿ��� �Ҵ��� �� �������� ��� ���μ���(������)�� �ڿ��� �Ҵ��ؾ� �ϴ��� ����. �����층�� �ϴ� ����� �ü������, ��å���� �ٸ���.
	Ŀ�ο��� �� ���μ������� ������ �ð���ŭ �ڿ��� ����� �� �ִ� ������ �ְ� ���μ����� �־��� �ð��� �����ų� �� �̻� �� �۾��� ���ٸ� ������ Ŀ�ο� �����ְ� �ȴ�.
	�� ����� �ڰ��� �����Ҷ� �׻� ������ "�ú��� �ý���"�ε� �ϴ�.
*/

#include <iostream>
#include <thread>
#include <mutex>

using namespace std;

int num = 0;

class SpinLock
{
public:
	void lock()
	{
		bool expected = false;
		while (locked.compare_exchange_strong(expected, true) == false) {
			expected = false;
			/*�� lock�Լ��� �ǹ��ϴ� ���� � �����尡 CAS�Լ��� �����ϴ� "�ٷ� �� ������" locked������ ���� false�� true�� ��ü�ϴ� ��*/
			this_thread::sleep_for(100ms); //Ŀ�� ���� ���ư��� 100ms������ �ٽ� �ð��� �Ҵ���� �ʴ´�. >> 100ms�� ������ ������ ���� �����층������ �Ҵ���� ���Ѵ�.
			//this_thread::yield(); //Ŀ�� ���� ���ư��� ���� �Ҵ���� �ð��� �����Ѵ�. >> ���� �����층������ �ٽ� �Ҵ���� �� �ִ�.
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

	cout << num << endl;
}