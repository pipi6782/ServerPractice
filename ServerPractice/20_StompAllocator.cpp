/*
	C++에서 제일 골치아픈 버그중 하나는 댕글링 포인터로 인한 메모리 오염이라고 볼 수 있다.

	오염된 메모리에 접근하는 경우는 다양하다.
	1. 할당을 해제한 메모리에 접근한다.
	보통 메모리는 할당을 해제하고 나면 그 구역의 메모리는 사용하지 않는것이 원칙이다.
	더이상 사용할 일이 없기 때문에 할당을 해제하기 때문이다.
	하지만 그 주소는 계속 남아있어서 해당 메모리에 접근은 할 수 있다.
	유효하지 않은 메모리에 접근하기 때문에 터지지 않을까 하는 생각이 들 수도 있지만
	예상과는 달리 터지지않고 메모리에 접근하게 된다.
	이런 문제를 댕글링 포인터 혹은 Use-After-Free 라고 부른다.
	그렇다면 delete를 하고 난 후에 널 포인터로 바꿔주면 될 수도 있겠지만
	그렇게 되면 그 포인터를 참조하는 모든 포인터들 역시 전부 다 널포인터로 바꿔줘야 한다.
	이런 문제를 막기 위해 나온 것이 스마트 포인터이고 스마트 포인터는 이런 문제를 해결할 수 있다.

	2. STL을 잘못 사용한 경우
	어떤 오브젝트들을 관리하는 STL이 있고 어떤 로직에서 하나의 엘리먼트라도 특정 조건을 만족한다면 STL내부를 싹 비워주는 코드를 만든다고 해보자.
	그런데 그 조건을 만족하여 STL내부를 다 날려버리고 break를 하지 않았다고 해보자.
	그럼 로직 내의 반복문이 있다면 그 반복문은 계속 돌 것이다.
	STL에서 알아서 터지지 않을까? 싶을 수도 있지만 STL이 언제나 항상 터져줄 것 이라는 보장은 없다.

	3. 캐스팅을 잘못 한 경우
	어떤 클래스 A가 있고 그 클래스를 상속받는 클래스 B가 있다고 해보자.
	이 때, A타입의 포인터에서 B의 기능을 사용하기 위해 캐스팅을 사용한다면 문제가 발생할 수 있다.
	보통 static_cast를 사용하는데 이 경우에는 타입체크를 하지 않고 캐스팅을 하기 때문에
	A타입의 포인터가 딱 A만큼의 메모리를 할당받았을 경우엔 B로 static_cast가 됐다고 하더라도 B에 해당하는 구역의 메모리는 할당받지 않은 상태다.
	이런 상황에서 B에 해당하는 데이터를 사용 하는것 역시 오염된 메모리를 사용하는 경우라고 볼 수 있다.
	그럼 dynamic_cast를 사용하면 되지 않을까? 하는 생각을 할 수도 있다.
	하지만 dynamic_cast는 가상함수 테이블 내에 있는 타입의 정보를 통해 타입을 비교한 후 캐스팅을 하기 때문에
	그냥 곧바로 캐스팅을 하는 static_cast 보다는 많이 느리다는 문제가 있다.
	그러다보니 dynamic_cast는 잘 사용하지 않고, 타입을 정확하게 알 수 있다는 조건 하에 캐스팅을 한다고 한다.
	
	오염된 메모리로 인해 생긴 버그는 버그가 어디서 발생하는지 찾기도 힘들 뿐더러 널포인터도 아니기 때문에 터지지도 않는다.
	그런 현상과 관련된 버그들을 잡을 수 있는 메모리 관련 정책중 하나로 Stomp Allocator가 있다.
*/


#include <iostream>
#include <Windows.h>

class MyClass
{
public:
	MyClass() { std::cout << "MyClass()" << std::endl; }
	~MyClass() { std::cout << "~MyClass()" << std::endl; }
};

void* operator new(size_t size)
{
	std::cout << "new!" << std::endl;
	void* ptr = malloc(size);
	return ptr;
}

void operator delete(void* ptr)
{
	std::cout << "delete!" << std::endl;
	free(ptr);
}

void* operator new[](size_t size)
{
	std::cout << "new![]" << std::endl;
	void* ptr = malloc(size);
	return ptr;
}

void operator delete[](void* ptr)
{
	std::cout << "delete![]" << std::endl;
	free(ptr);
}


/////////////////////
// Stomp Allocator //
/////////////////////
class StompAllocator
{
public:
	enum {PAGE_SIZE = 0x1000 };

	static void* Alloc(int size)
	{
		//4096당 1페이지씩 할당되도록 함
		const __int64 pageCount = (size + PAGE_SIZE - 1) / PAGE_SIZE;
		const __int64 dataOffset = pageCount * PAGE_SIZE - size;

		void* baseAddress = VirtualAlloc(NULL, pageCount * PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		return static_cast<void*>(static_cast<__int8*>(baseAddress) + dataOffset);
	}

	static void Release(void* ptr)
	{
		const __int64 address = reinterpret_cast<__int64>(ptr);
		const __int64 baseAddress = address - (address % PAGE_SIZE);
		VirtualFree(reinterpret_cast<void*>(baseAddress), 0, MEM_RELEASE);
	}

};

////////////
// Memory //
////////////

template<typename Type, typename... Args>
Type* xnew(Args&&... args)
{
	Type* memory = static_cast<Type*>(StompAllocator::Alloc(sizeof(Type)));
	
	new(memory)Type(std::forward<Args>(args)...);

	return memory;
}

template<typename Type>
void xdelete(Type* obj)
{
	obj->~Type();
	StompAllocator::Release(obj);
}

int main()
{
/*
	Stomp Allocator에 대해서 알려면 운영체제의 메모리 관리 기법에 대해서 알아야 한다.
	일단 가상메모리에 대해서 알아야 한다.
	다음과 같은 코드가 있다고 해보자.
*/

	/*
	{
		int* pNum = new int;
		*pNum = 100;

		int addr = reinterpret_cast<long long>(pNum);
		
		//이렇게 하면 동적으로 할당받은 pNum의 주소를 알 수 있게 된다.
		//그런 다음 다음 코드가 있다고 해보자.
		
		{
			//이 코드는 다른 독립적인 프로그램에서 실행한다고 가정한다.
			int* num = reinterpret_cast<int*>(addr);
			*num = 200;
			
			//이런 코드가 있다 하면 이 코드는 작동하지 않는다.
			//유저 레벨에서의 독립적인 프로그램은 서로 간섭을 할 수 없기 때문이다.
			//그 이유는 가상메모리에 있는데, 포인터를 통해서 확인하는 주소값은 램에 실제로 존재하는 물리적인 주소가 아니다.
			//어떤 부분은 디스크로 맵핑 될 수도 있고, 어떤 부분은 램에 맵핑 될 수도 있는 가상 주소를 받게 되는 것이고 이를 가상 메모리라 한다.
			//같은 0x1 이라 해도 실제로는 같은 곳을 가리키는 것이 아닌 것이다.

			//실질적으로 운영체제는 메모리를 페이지 단위로 관리하게 된다.
			//그래서 1번 페이지는 READ만 가능하게, 2번 페이지는 WRITE만 가능하게, 3번 페이지는 R/W다 가능하게 와 같은 설정을 할 수가 있다는 것이다.

			//C++의 new는 그 자체로 메모리를 할당하는 것이 아니라 각 운영체제에 맞는 방식의 메모리 할당을 하게 할 뿐이다.
			//그 중 윈도우에서는 VirtualAlloc 이라는 함수를 사용하는데 이 함수로 메모리 사용을 예약만 할 것인지 아니면 사용까지 할 것인지 부터 해서
			//읽기만 할 것인지, 쓰기만 할 것인지, 둘 다 가능하게 할 것인지 등 다양한 정책을 세울 수 있다.	
		}
	}
	*/
	
	//이렇게 쓰면 운영체제가 알아서 정해주는 위치에(NULL), 4바이트 만큼 메모리를 예약과 동시에 사용하고 (MEM_RESERVE | MEM_COMMIT), 이 구역은 READ와 WRITE 둘 다 가능하게 한다는 말이 된다.
	int* test = (int*)::VirtualAlloc(NULL, 4, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	*test = 100;
	//메모리를 해제하고 싶으면 이런 식으로 사용한다. test라는 메모리를 0바이트로 밀어버리고 그 구역의 메모리는 사용하지 않는다 라고 생각하면 쉽다.
	//이렇게 하면 운영체제한테까지 가서 이 메모리를 날렸기 때문에 진짜로 사용할 수 없는 메모리가 된다.(쓰려 한다면 크래시가 난다.)
	::VirtualFree(test, 0, MEM_RELEASE);

	/* 
		Stomp Allocator는 단점이 있다 하면 필요없는 메모리를 더 할당받게 될 수도 있다는 것이다.
		1바이트만 할당 받는다 하더라도 1페이지에 해당하는 만큼의 메모리를 할당받는 상황이 생길 수 있다.
		나는 0~100까지만 필요한데 0~4096만큼 할당받는 상황이 생길 수 있는 것이다.
		이런 현상은 곧 사람의 눈으로 봤을 땐 유효하지 않은 메모리에 접근해서 임의로 데이터를 수정할 수 있다는 문제가 있다.
		이런 메모리 오버플로우 문제를 방지하기 위해서 할 수 있는 방법으로 내가 필요한 영역의 끝점을 할당받은 메모리의 끝과 똑같이 맞추는 방법이 있다.
		그러면 언더플로우 문제가 발생할 수 있지 않냐 생각할 수 있을텐데, 메모리 언더플로우 문제는 생각보다 발생할 수 있는 케이스가 적다고 한다.
	*/
}