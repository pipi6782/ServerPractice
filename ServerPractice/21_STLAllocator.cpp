/*
	Stomp Allocator의 사용으로 유효하지 않은 메모리 사용을 잡아내는 것이 가능해졌다.
	하지만 STL의 경우에는 계속 new/delete를 사용할 것인데 이 역시 버그를 잡기 용이하게 바꿔야 한다.
	STLAllocator는 또 다른 Allocator라기 보단 new/delete 방식으로 작동하는 STL을
	우리가 원하는 방식으로 할당/해제 할 수 있게 하기 위한 것이다.
*/

#include <Windows.h>
#include <vector>

/////////////////////
// Stomp Allocator //
/////////////////////
class StompAllocator
{
public:
	enum { PAGE_SIZE = 0x1000 };

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

template <typename T>
class STLAllocator
{
public:
	using value_type = T;

	STLAllocator() {}
	template<typename Other>
	STLAllocator(const STLAllocator<Other>&) {}

	T* allocate(size_t count)
	{
		const int size = static_cast<int>(count * sizeof(T));
		return static_cast<T*>(StompAllocator::Alloc(size));
	}

	void deallocate(T* ptr, size_t count)
	{
		StompAllocator::Release(ptr);
	}
};

int main()
{
	std::vector<int, STLAllocator<int>> v;
}