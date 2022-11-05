/*
	보통 메모리를 할당한다 하면 new/delete를 이용하게 된다.
	그런데 만약 필요할 때마다 new를 하고 필요없을 때 마다 delete를 하게 되면
	할 때마다 Context Switching이 발생해서 성능이 떨어지는 결과가 생길 수 있다.
	그렇다면 어짜피 자주 사용하게 될 오브젝트들은 필요없다고 바로 삭제하지 않고 임시로 보관하다가
	필요해지면 정보만 세팅해주고 재활용 하면 될 것이다.
	물론 추가적인 메모리가 필요하다면 더 할당을 해야하지만 말이다.
*/

/*
	그러기 위해서 메모리 풀을 위한 클래스를 구현하는데 이 메모리 풀의 작동방식은 이렇다.
	1. 각각의 메모리풀은 자신이 담당하는 메모리의 크기가 있다.
	(1~32byte)(33byte~64btye) 처럼 말이다.
	그리고 디버깅을 위해서 할당하는 메모리의 앞부분에 헤더를 추가한다.
	이는 실제 new/delete로 메모리를 할당할 떄와 비슷하다고 한다.
	결론적으로 실제 메모리의 구조는 이렇다.
	[Memory Header][Data]
*/

#include <new>
#include <atomic>
#include <queue>
#include <vector>

struct MemoryHeader
{
	MemoryHeader(__int32 size) : allocSize(size) {}

	static void* AttachHeader(MemoryHeader* header, __int32 size)
	{
		new(header)MemoryHeader(size);
		return reinterpret_cast<void*>(++header);
	}

	static MemoryHeader* DetachHeader(void* ptr)
	{
		MemoryHeader* header = reinterpret_cast<MemoryHeader*>(ptr) - 1;
		return header;
	}

	__int32 allocSize;
};

class MemoryPool
{
public:
	MemoryPool(__int32 allocSize) : allocSize(allocSize)
	{

	}
	~MemoryPool()
	{
		while (queue.empty() == false)
		{
			MemoryHeader* header = queue.front();
			queue.pop();
			free(header);
		}
	}


	//메모리가 필요없어서 Pool에 반납할 때는 Push
	void Push(MemoryHeader* ptr)
	{
		//WRITE_LOCK;

		//Pool에 메모리 반납

		queue.push(ptr);
		allocCount.fetch_sub(1);
		
	}

	//메모리가 필요해서 Pool에서 가져올 때는 Pop을 쓴다.
	MemoryHeader* Pop()
	{
		MemoryHeader* header = nullptr;
		{
			//공부용 프로젝트는 이 부분을 주석처리
			//Write하는 부분만 lock을 한다는 의미
			//WRITE_LOCK;

			//Pool에 여분이 있는지 체크
			if (queue.empty() == false)
			{
				header = queue.front();
				queue.pop();
			}
		}

		//만약 여분이 없으면 바로 하나 만들어준다.
		if (header == nullptr)
		{
			header = reinterpret_cast<MemoryHeader*>(malloc(allocSize));
		}
		else
		{
			//혹시나 allocSize가 0이어서 메모리 할당이 의미가 없을 때
			//이 부분 역시 주석처리
			//해당 부분이 true가 아니면 강제로 Crash를 발생시키는 코드
			//ASSERT_CRASH(header->allicSize == 0);
		}

		allocCount.fetch_add(1);

		return header;
	}
private:
	__int32 allocSize = 0;
	std::atomic<__int32> allocCount = 0;

	//이곳에는 원래 USE_LOCK이라는 커스텀 매크로가 들어가야 하지만
	//cpp파일마다 새롭게 정의해야하는 문제가 있어 생략함
	//Lock을 사용한다는 의미임
	std::queue<MemoryHeader*> queue;
};

/*
	StompAllocator를 쓰지 않는 이유
	StompAllocator는 메모리를 사용하지 않으면 운영체제한테까지 가서 무슨 일이 있어도 메모리를 사용하지 못하게 한다.
	하지만 메모리풀은 메모리를 사용하지 않는다 해도 사용가능한 메모리상태를 유지시키기 때문에
	StompAllocator와는 어울리지 않는다.
*/


////////////
// Memory //
////////////
class Memory
{
	enum
	{
		//~1024까지는 32단위
		//~2048까지는 128단위
		//~4096까지는 258단위
		POOL_COUNT = 1024 / 32 + 1024 / 128 + 2048 / 256,
		MAX_ALLOC_SIZE = 4096 //이 사이즈보다 크면 풀을 이용하지 않고 new/delete를 함
	};

public:
	Memory()
	{
		//메모리 할당 사이즈에 따라서 그에 맞는 메모리풀에 맵핑시키게 하기 위한 과정
		
		__int32 size = 0;
		__int32 tableIndex = 0;
		for (size = 32; size <= 1024; size += 32)
		{
			MemoryPool* pool = new MemoryPool(size);
			pools.push_back(pool);
			while (tableIndex <= size)
			{
				poolTable[tableIndex] = pool;
				tableIndex++;
			}
		}

		for (; size <= 2048; size += 128)
		{
			MemoryPool* pool = new MemoryPool(size);
			pools.push_back(pool);
			while (tableIndex <= size)
			{
				poolTable[tableIndex] = pool;
				tableIndex++;
			}
		}

		for (; size <= 4096; size += 256)
		{
			MemoryPool* pool = new MemoryPool(size);
			pools.push_back(pool);
			while (tableIndex <= size)
			{
				poolTable[tableIndex] = pool;
				tableIndex++;
			}
		}
	}

	~Memory()
	{
		for (MemoryPool* pool : pools)
			delete pool;

		pools.clear();
	}

	void* Allocate(__int32 size)
	{
		MemoryHeader* header = nullptr;

		const __int32 allocSize = size + sizeof(MemoryHeader);

		if (allocSize > MAX_ALLOC_SIZE) //메모리풀 최대 크기보다 크면 그냥 일반 할당
		{
			//왜 reinterpret_cast를 쓸까?
			header = reinterpret_cast<MemoryHeader*>(malloc(allocSize));
		}
		else
		{
			//메모리 풀에서 꺼내옴 (메모리 파편화로 인한 단편화 현상 방지 및 성능향상)
			header = poolTable[allocSize]->Pop();
		}

		return MemoryHeader::AttachHeader(header, allocSize);
	}
	void Release(void* ptr)
	{
		MemoryHeader* header = MemoryHeader::DetachHeader(ptr);

		const __int32 allocSize = header->allocSize;
		//ASSERT_CRASH(allocSize > 0); //혹시나 allocSize가 0이면 문제가 있는 것이니 잡아줌(공부용 프로젝트에서는 주석처리)

		//메모리 풀의 최대 크기를 벗어나면 일반적인 할당 해제
		if (allocSize > MAX_ALLOC_SIZE)
			free(header);
		else
		{
			//메모리 풀에 메모리 반납
			poolTable[allocSize]->Push(header);
		}
	}

private:
	std::vector<MemoryPool*> pools;
	MemoryPool* poolTable[MAX_ALLOC_SIZE + 1];
};

int main()
{

}