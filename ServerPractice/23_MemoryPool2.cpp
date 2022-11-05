/*
	이전 코드(22_MemoryPool1)에서 굳이 아쉬운 점을 찾아본다면
	메모리의 할당은 앞으로 자주 일어날 것인데 이런 부분에서 조금이라도 개선을 시킨다면 효과가 매우 좋을 것이다.
	1. 메모리 풀에 Push/Pop을 할 때 다수의 스레드가 경합을 벌여서 락을 한다는 부분
	2. 메모리 풀에서 메모리를 관리할 때 STL을 사용하고 있는데 이 STL이 사용할 데이터를 할당하기 위한 공간이 필요하다는 것
*/

#include <windows.h>
#include <thread>


#define CaseSingleThread 1
#define AfterABAProblem 1
#if CaseSingleThread == 0
template<typename T>
struct Node
{
	//보통 노드기반의 자료구조를 만들게 되면 다음과 같은 형태를 갖게 된다.
	//하지만 이 역시도 조금 아쉬운 점이 있다면 노드 하나를 만드는데 데이터와 다음 노드의 주소까지 총 2개의 데이터가 필요하다는 것이다.
	//이를 좀 더 개선시키면 데이터 내부에 다음 노드와 관련된 정보까지 같이 넣어주는 것이다.
	T data;
	Node* node;
};

struct SListEntry
{
	SListEntry* next;
};

//이런 방식을 취한다면 아예 데이터에서 다음 노드의 위치를 가질 수 있게 된다.
//Data + Node 방식이 아닌 Data(+Node)라고 볼 수 있다.
//따지고 보면 내부의 데이터가 한번 래핑되어있는가 아닌가의 차이라고 볼 수 있을 것 같은데
//결국엔 비슷한 방법이 아닌가 하는 생각이 들기도 한다.
//이 방식의 단점이라 한다면 기존의 잘 만들어진 외부 라이브러리 같은 것들은
//이런 방식을 사용할 수 없다는 것이다.
//하지만 팀이나 프로그래머가 직접 설계해서 만든 데이터라면 이런 방식을 활용할 수 있다.
class Data
{
public:
	SListEntry entry;

	__int32 hp;
	__int32 mp;
};

//LockFree방식으로 스택을 구현해볼 것인데, 기본적인 구조는 이렇다.
//Header가 존재하고 그 Header는 맨 처음 데이터를 가리킨다.
//그리고 그 데이터는 그 다음 데이터를 가리키고 하는 방식이다.
//[Header] -> [List] -> [List] -> [List] 와 같은 구조라 볼 수 있다.
//즉, Header만 알고 있으면 데이터를 넣고 뺴고를 할 수 있다는 것이다.
struct SListHeader
{
	SListEntry* next = nullptr;
};

//늘 그렇듯이 기본적인 세팅은 싱글 스레드를 기반으로 한다.
//그 후 기능들이 완성되면 멀티스레드에 맞게 천천히 바꿔나간다.
void InitializeHead(SListHeader* header)
{
	header->next = nullptr;
}

void PushEntryList(SListHeader* header, SListEntry* entry)
{
	//[Header] [2] [1] 과 같이 있다 할 때
	//Header의 Next는 2인 상황이다.
	//그 상황에서 새로 들어오는 entry(3이 들어온다 하자)의 Next를 2로 바꿔준 후
	//Header의 Next를  3으로 바꿔치기 해주면
	//LIFO방식으로 입력을 할 수 있게 된다.
	entry->next = header->next;
	header->next = entry;
}

SListEntry* PopEntryList(SListHeader* header)
{
	SListEntry* first = header->next;
	if (first != nullptr)
		header->next = first->next;

	return first;
}


int main()
{
	//단순히 테스트 목적의 코드이기 때문에 C스타일의 캐스팅을 이용함
	SListHeader header;
	InitializeHead(&header);
	Data* data = new Data();
	data->hp = 10;
	data->mp = 20;
	//이 부분도 원래는 이렇게 하면 안된다.
	//SListEntry가 Data의 부모가 아니기 때문이다.
	//지금 같은 상황이야 SListEntry내부에 포인터밖에 없기 때문에 크기가 맞아서 작동하는 코드지만
	//엄밀히 하려면 &data->entry 로 해야한다.
	PushEntryList(&header,(SListEntry*)data);
	Data* popData = (Data*)PopEntryList(&header);
	
}
#elif CaseSingleThread == 1  & AfterABAProblem == 0
template<typename T>
struct Node
{
	//보통 노드기반의 자료구조를 만들게 되면 다음과 같은 형태를 갖게 된다.
	//하지만 이 역시도 조금 아쉬운 점이 있다면 노드 하나를 만드는데 데이터와 다음 노드의 주소까지 총 2개의 데이터가 필요하다는 것이다.
	//이를 좀 더 개선시키면 데이터 내부에 다음 노드와 관련된 정보까지 같이 넣어주는 것이다.
	T data;
	Node* node;
};

struct SListEntry
{
	SListEntry* next;
};

//이런 방식을 취한다면 아예 데이터에서 다음 노드의 위치를 가질 수 있게 된다.
//Data + Node 방식이 아닌 Data(+Node)라고 볼 수 있다.
//따지고 보면 내부의 데이터가 한번 래핑되어있는가 아닌가의 차이라고 볼 수 있을 것 같은데
//결국엔 비슷한 방법이 아닌가 하는 생각이 들기도 한다.
//이 방식의 단점이라 한다면 기존의 잘 만들어진 외부 라이브러리 같은 것들은
//이런 방식을 사용할 수 없다는 것이다.
//하지만 팀이나 프로그래머가 직접 설계해서 만든 데이터라면 이런 방식을 활용할 수 있다.
class Data
{
public:
	SListEntry entry;

	__int32 hp;
	__int32 mp;
};

//LockFree방식으로 스택을 구현해볼 것인데, 기본적인 구조는 이렇다.
//Header가 존재하고 그 Header는 맨 처음 데이터를 가리킨다.
//그리고 그 데이터는 그 다음 데이터를 가리키고 하는 방식이다.
//[Header] -> [List] -> [List] -> [List] 와 같은 구조라 볼 수 있다.
//즉, Header만 알고 있으면 데이터를 넣고 뺴고를 할 수 있다는 것이다.
struct SListHeader
{
	SListEntry* next = nullptr;
};

//서버는 동시에 들어오는 입력들을 처리해야 하기 때문에 멀티스레드 프로그래밍이 필수적이다.
//그렇기 때문에 기존에 싱글스레드로 만들었던 과정들을 멀티스레드로 교체해본다.
//여기서 CAS알고리즘이 사용된다.
void InitializeHead(SListHeader* header)
{
	//head 초기화는 싱글스레드에서 작동하고 Push/Pop을 멀티스레드로 작동시킨다고 가정하고 진행함
	header->next = nullptr;
}

void PushEntryList(SListHeader* header, SListEntry* entry)
{
	//[Header] [2] [1] 과 같이 있다 할 때
	//Header의 Next는 2인 상황이다.
	//그 상황에서 새로 들어오는 entry(3이 들어온다 하자)의 Next를 2로 바꿔준 후
	//Header의 Next를  3으로 바꿔치기 해주면
	//LIFO방식으로 입력을 할 수 있게 된다.
	entry->next = header->next;
	
	//여기서 각각의 파라미터의 역할은
	//Destination : 변경될 값(메모리)
	//Exchange : 내가 세팅할 값 (Comperand와 Destnation을 비교해서 일치하면 Destnation을 Exchange로 바꿔준다)
	//Compared : 비교할 대상의 값 (Destnation과 비교함)
	//이렇게 되면 InterlockedCompareExchange64에서 경합이 발생하고 경합에서 통과한 스레드만 루프를 탈출하게 된다.
	//아주 잠깐의 순간에 header->next의 값이 바뀌기 때문
	while (::InterlockedCompareExchange64((__int64*)&header->next, (__int64)entry, (__int64)entry->next) == 0)
	{
	}
}

SListEntry* PopEntryList(SListHeader* header)
{
	SListEntry* expected = header->next;

	//보통 Pop을 하게 되면 delete를 한다.
	//그런데 멀티스레드 상황에서 Pop을 하게 되면 CAS연산 중 delete해버린 메모리를 참조하는 문제가 발생한다.
	//그렇다면 메모리 삭제를 늦춘다면 문제가 해결될까?
	//그렇다고 할 수 없다. 이런 상황에서 발생하는 문제를 ABA 문제라고 한다.
	//말 그대로 해석하면 A에서 B가 됐다가 다시 A가 됐을 때 발생하는 문제인 것이다.
	//예를 들자면 이렇다
	//[Header]->[A]->[B]->[C] 와 같이 되고 있다 할 때
	//Header->next가 A면 Header->next에 B를 넣겠다고 하자.
	//그런데 다수의 스레드가 일사불란하게 같은 동작을 하고 있을리는 절대 없다.
	//다른 스레드에서 두개의 데이터를 Pop 시키고 또 다른 스레드에서 공교롭게도 Pop시키려 했던 [A]를 Push해서 현재는 [Header]->[A]->[C] 인 상태라고 해보자.
	//그러면 분명히 상태가 바뀌었음에도 로직은 계속해서 실행시키게 된다는 문제가 발생하게 된다.
	//이를 해결하는 방식중 하나는 데이터를 비교할 때 진짜 딱 "데이터"자체만 비교는 것이 아닌
	//내부에 상태를 판별할 수 있는 또 다른 값을 더 넣어줘서 상태 변화를 감지할 수 있게 하는 것이다.
	while (expected != nullptr && ::InterlockedCompareExchange64((__int64*)header->next,(__int64)expected->next,(__int64)expected))
	{}

	return expected;
}
#elif CaseSingleThread == 1  & AfterABAProblem == 1
//위의 방식대로 Push/Pop을 처리하게 되면 상태변화를 감지하지 못하고 동일한 로직을 수행하는 ABA 문제가 발생하게 된다.
//ABA문제를 해결하기 위한 과정이며 MS에서 만든 방식과 유사하게 진행한다.

DECLSPEC_ALIGN(16) //무조건 메모리 정렬을 16바이트 단위로 하라고 하는 것.
struct SListEntry
{
	SListEntry* next;
};

//이런 방식을 취한다면 아예 데이터에서 다음 노드의 위치를 가질 수 있게 된다.
//Data + Node 방식이 아닌 Data(+Node)라고 볼 수 있다.
//따지고 보면 내부의 데이터가 한번 래핑되어있는가 아닌가의 차이라고 볼 수 있을 것 같은데
//결국엔 비슷한 방법이 아닌가 하는 생각이 들기도 한다.
//이 방식의 단점이라 한다면 기존의 잘 만들어진 외부 라이브러리 같은 것들은
//이런 방식을 사용할 수 없다는 것이다.
//하지만 팀이나 프로그래머가 직접 설계해서 만든 데이터라면 이런 방식을 활용할 수 있다.
DECLSPEC_ALIGN(16)
class Data
{
public:
	SListEntry entry;

	__int32 hp;
	__int32 mp;
};

//LockFree방식으로 스택을 구현해볼 것인데, 기본적인 구조는 이렇다.
//Header가 존재하고 그 Header는 맨 처음 데이터를 가리킨다.
//그리고 그 데이터는 그 다음 데이터를 가리키고 하는 방식이다.
//[Header] -> [List] -> [List] -> [List] 와 같은 구조라 볼 수 있다.
//즉, Header만 알고 있으면 데이터를 넣고 뺴고를 할 수 있다는 것이다.
DECLSPEC_ALIGN(16)
struct SListHeader
{
	SListHeader()
	{
		alignment = 0;
		region = 0;
	}
	//union을 사용하면 실질적으로 내부에서는 uint64두개를 갖는 구조체가 존재한다.
	//HeaderX64에서는 각각의 멤버변수가 몇 비트를 사용할 것인지 콜론을 통해서 구분하게 된다.
	union
	{
		struct
		{
			unsigned __int64 alignment;
			unsigned __int64 region;
		}DUMMYSTRUCTNAME;

		struct
		{
			unsigned __int64 depth : 16;
			unsigned __int64 sequence : 48;
			unsigned __int64 reserved : 4;
			unsigned __int64 next : 60;
		}HeaderX64;
	};
};

void InitializeHead(SListHeader* header)
{
	header->alignment = 0;
	header->region = 0;
}

void PushEntryList(SListHeader* header, SListEntry* entry)
{
	SListHeader expected = { };
	SListHeader desired = { };

	//데이터를 16바이트 단위로 정렬을 했다는 말은 하위 4바이트는 무조건 0000이라는 말이 된다.
	//64비트 개발 환경에서의 최신 컴파일러라면 할당받은 주소가 16으로 나누어 떨어지게 된다.
	//그렇게해서 시프트연산을 하고 남게 되는 상위 4비트는 다른 정보를 저장하는데 사용할 수 있다는 뜻이다.
	desired.HeaderX64.next = ((unsigned __int64)entry) >> 4;
	while (true)
	{
		expected = *header;

		//이 연산을 하는 사이에 다른 스레드에서 데이터를 변경할 수도 있음.
		//지금은 스레드 내부에 있는 스택영역에 있는 변수를 활용하기 때문에 별도의 CAS연산은 필요하지 않음
		entry->next = (SListEntry*)(((unsigned __int64)expected.HeaderX64.next) << 4);
		desired.HeaderX64.depth = expected.HeaderX64.depth + 1;
		desired.HeaderX64.sequence = expected.HeaderX64.sequence + 1;

		//현재 사용하는 Header의 크기가 128비트이기 때문에 128버전을 사용함
		//상위 64비트는 region, 하위64비트는 alignment를 넣어줌
		if (::InterlockedCompareExchange128((__int64*)header, desired.region, desired.alignment, (__int64*)&expected) == 1)
			break;
	}
}

SListEntry* PopEntryList(SListHeader* header)
{
	SListHeader expected = { };
	SListHeader desired = { };
	SListEntry* entry = nullptr;

	while (true)
	{
		expected = *header;

		entry = (SListEntry*)(((unsigned __int64)expected.HeaderX64.next) << 4);
		if (entry == nullptr)
			break;

		//depth는 Push했을 때는 1증가하고 Pop했을 때는 1감소한다.
		//sequence는 Push나 Pop을 할 때 마다 1씩 증가한다.
		//그러면 언젠가 다시 겹치는 상황이 발생할 수도 있지 않을까? 싶겠지만
		//그럴 확률은 굉장히 낮다.
		//sequence가 한바퀴를 다 돌아야 가능한데 48비트를 다 돌고나서 정확하게 일치한다는 것이 현실적으로 불가능하기 때문.
		//그렇기 때문에 ABA문제를 "우회"할 수 있다고 볼 수 있다. 강의에서는 해결했다고 하는데, 나는 이걸 해결이라고 보진 않는다.
		//정말 가끔이라도 언젠가는 발생할 수 있는 가능성이 있기 때문이다.
		//이렇게 ABA문제를 우회했다 해도 다른 문제가 있을 수 있는데 댕글링 포인터와 관련된 문제다.
		//entry의 널체크를 통과하고 desired.HeaderX64.next = ((unsigned __int64)entry->next) >> 4 로 주소를 가져오려는 순간에
		//다른 스레드에서 entry에 해당하는 메모리를 해제해버릴 수도 있다.
		//Pop을 하는 스레드가 하나뿐이라면 멀티스레드 환경에서 안전하다고 볼 수 있지만
		//다수의 스레드가 Pop을 하게 된다면 댕글링 포인터와 관련된 문제는 여전히 존재한다고 볼 수 있다.
		desired.HeaderX64.next = ((unsigned __int64)entry->next) >> 4;
		desired.HeaderX64.depth = expected.HeaderX64.depth - 1;
		desired.HeaderX64.sequence= expected.HeaderX64.sequence + 1;

		if (::InterlockedCompareExchange128((__int64*)header, desired.region, desired.alignment, (__int64*)&expected) == 1)
			break;
	}


	return entry;
}

int main()
{
	return 0;
}
#endif

