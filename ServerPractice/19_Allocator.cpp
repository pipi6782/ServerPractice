/*
	보통 학생의 수준에서 프로그래밍을 하게 되면 new와 delete를 이용해서 메모리 할당을 하게 된다.
	근데 이 방식에는 문제점이 있는데 보통 메모리 할당은 운영체제에서 관리하게 된다.
	그러다보니 커널레벨까지 내려가서 OS가 메모리 할당을 해준 후 다시 유저 레벨로 올라오게 되는데
	이 과정에서 Context Switching이 발생한다.
	그렇게 되면 new/delete가 많아질 수록 Context Switching 횟수가 많아지고 이는 성능과 직결된다.
	그럼 만약에 메모리를 사용하지 않을 때 할당을 해제하는 것이 아니라 임시 보관소에 저장하게 한다면 Context Switching도 한번만 발생할 것이고
	필요한 메모리를 사용할 수 있을 것이다.

	그 다음 문제는 메모리 단편화이다.
	메모리 단편화는 내가 사용할 수 있는 메모리의 크기는 충분히 있지만
	그것이 연속적이지 않아 할당이 불가능한 상황을 말한다.
	예를 들어 내가 30Byte를 할당해야하는데 할당 가능한 연속적인 공간이 각각 25Byte, 20Byte 인 경우
	가지고 있는 공간은 45Byte이지만 연속적이지 않아 할당이 불가능한 것이다.
	
	물론 지금 윈도우에서는 MS에서 잘 수정을 해줬는지 이런 부분을 굳이 생각하지 않는다고 한다.
*/

#include <iostream>

//C++에서는 new 와 delete 역시 연산자이기 때문에 오버로딩이 가능하다.
class MyClass
{
public:
	MyClass() { std::cout << "MyClass()" << std::endl; }
	~MyClass() { std::cout << "~MyClass()" << std::endl; }

	//void* operator new(size_t size)
	//{
	//	std::cout << "MyClass new!" << std::endl;
	//	void* ptr = malloc(size);
	//	return ptr;
	//}
	//
	//void operator delete(void* ptr)
	//{
	//	std::cout << "MyClass delete!" << std::endl;
	//	free(ptr);
	//}
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

/*
	하지만 이 방식도 문제가 있다고 볼 수 있는데
	외부 라이브러리같은 경우에는 이런 방식을 사용했다간 문제가 생길 수도 있다는 것이다.
	그러다보니 메모리 할당을 관리하는 객체를 하나 만드는 방법도 있다.
*/

////////////////////
// Base Allocator //
////////////////////
class BaseAllocator
{
public:
	static void* Alloc(int size)
	{
		return malloc(size);
	}
	static void Release(void* ptr)
	{
		free(ptr);
	}

};

////////////
// Memory //
////////////
/*
	여기서 ...이 붙은게 어떤 의미인가 할 수도 있는데
	...이 의미하는 것은 해당 타입을 가진 파라미터의 갯수가 가변적이라는 뜻이다.
	C++11 이 도입되기 전에는 파라미터의 갯수만큼 템플릿을 만들어줘야 했지만 이 방식을 사용하면
	파라미터가 몇개가 오던 상관이 없다.
*/
template<typename Type, typename... Args>
Type* xnew(Args&&... args)
{
	Type* memory = static_cast<Type*>(BaseAllocator::Alloc(sizeof(Type)));
	
	//이 문법은 placement new 라고 하는 문법으로 기존의 new와는 다르게 일단 메모리는 할당되어 있으니 생성자만 호출하라는 의미가 된다.
	new(memory)Type(std::forward<Args>(args)...);

	return memory;
}

template<typename Type>
void xdelete(Type* obj)
{

	obj->~Type();
	BaseAllocator::Release(obj);
}
