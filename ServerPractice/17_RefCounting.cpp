/*
	멀티 스레드가 끝났으면 이젠 메모리 관리이다.
	보통 동적으로 new 할당을 하고 나면 반드시 delete를 해야한다.
	어떤 객체가 있는데 더이상 그 객체를 쓸 일이 없어서 delete를 했다고 해보자.
	만약 delete한 객체를 사용하는 곳이 있으면 어떻게 될까?
	프로그램이 터지면 오히려 다행이겠지만 터지지 않으면 문제가 심각해진다.
	이미 할당 해제한 메모리에서 가져오면 안되는 값을 가져오기 때문이다.
	이렇게 메모리를 할당 해제했지만 할당했을 때의 주소가 남아있어 접근할 수 있는 포인터를 허상포인터(댕글링 포인터) 라고 하며
	댕글링 포인터가 존재하는 것은 널 참조로 인해 크래시가 나는 것 보다 더 위험한 것이다.
	하지만 쓸 일이 없다고 무작정 할당을 해제하는 것도 위험할 것이다.
	지금부터는 메모리 관리를 하는 다양한 기법들을 알아볼 것이다.
	그 중 첫번째는 몇개의 객체가 참조를 하고 있는지 알아보는 것이다.
*/

#include <atomic>

/*
	아래 코드는 객체를 참조한 횟수를 기록한 후 아무도 객체를 참조하지 않을 때(refCount 가 0) 객체를 delete해주는 방식이다.
	이 방식이 shared_ptr의 방식과 동일하다고 볼 수 있는데, 이 코드는 싱글 스레드에서는 문제가 없다.
	하지만 멀티스레드로 넘어오는 순간 문제가 발생한다.
	그 이유는 refCount를 연산하는 것이 atomic하지 않다는 것이다.
	그렇기 때문에 refCount를 atomic클래스로 래핑해준다.
*/
class RefCountable
{
public:
	RefCountable() : refCount(1) {}
	virtual ~RefCountable() {}

	int GetRef() { return refCount; }
	int AddRef() { return  ++refCount; }

	int ReleaseRef()
	{
		int count = --refCount;
		if (count == 0)
			delete this;
		return count;
	
	}

protected:
	std::atomic<int> refCount;
};


using MarineRef = TSharedPtr<Marine>;

class Marine : public RefCountable
{
public:
	int hp = 50;
	int posX = 0;
	int posY = 0;
};

class Bullet : public RefCountable
{
public:
	void SetTarget(MarineRef target)
	{
		this->target = target;
		target->AddRef();
	}

	void Update()
	{
		if (target == nullptr) return;

		int targetXPos = target->posX;
		int targetYPos = target->posY;

		if (target->hp == 0)
		{
			target->ReleaseRef();
			target = nullptr;
		}
	}

private:
	MarineRef target = nullptr;
};

/*
	참조횟수를 카운팅 하는 방식이 일반적인 shared_ptr의 방식이지만
	이를 수동으로 하는 것은 상당히 위험하다. 
	수동으로 한다 한들 내가 어떤 행위를 할 때 다른 스레드의 개입을 막을 수 없기 때문이다.
	아래 코드는 shared_ptr의 방식과 비슷하게 작동하게끔 한 방식이다.
	이 코드처럼 어떤 객체를 래핑해서 내부적으로 관리하는 것이 훨씬 안정적이라 볼 수 있다.
	그런데 이런 스마트포인터(?)를 파라미터로 넘겨줄 때 일반적인 변수로 넘겨주게 되면
	복사가 계속 일어나기 때문에 일반 정수연산보다 훨씬 무거운 atomic을 이용한 연산을 계속적으로 하게 된다.
	이것 자체가 부하를 늘리게 되는 것이기 때문에 Call By Reference를 이용해서 넘겨주는 것이 좋다.
	메이플 블로그에서도 나왔던 문제점이 바로 이 문제점이었다.
*/
template<typename T>
class TSharedPtr
{
public:
	TSharedPtr() {}
	TSharedPtr(T* ptr) { Set(ptr); }

	//복사
	TSharedPtr(const TSharedPtr& rhs) { Set(rhs.ptr); }

	//이동
	TSharedPtr(TSharedPtr&& rhs) { ptr = rhs.ptr; rhs.ptr = nullptr; }

	//상속관계 복사
	template<typename U>
	TSharedPtr(const TSharedPtr<U>& rhs) { Set(static_cast<T*>(rhs.ptr)); }

	~TSharedPtr() { Release(); }

public:
	//복사 연산자
	TSharedPtr& operator=(const TSharedPtr& rhs)
	{
		if (ptr != rhs.ptr)
		{
			Release();
			Set(rhs.ptr);
		}
		return *this;
	}

	//이동 연산자
	TSharedPtr& operator=(const TSharedPtr& rhs)
	{
		Release();
		ptr = rhs.ptr;
		rhs.ptr = nullptr;
		return *this;
	}

	bool IsNull() { return ptr == nullptr; }

	bool operator== (const TSharedPtr& rhs) const { return ptr == rhs.ptr; }
	bool operator==(T* ptr) const { return this->ptr == ptr; }
	bool operator!= (const TSharedPtr& rhs) const { return ptr != rhs.ptr; }
	bool operator!=(T* ptr) const { return this->ptr != ptr; }
	bool operator<(const TSharedPtr& rhs) { return ptr < rhs.ptr; }
	T* operator*() const { return ptr; }
	operator T* () const { return ptr; }
	T* operator->() { return ptr; }

private:
	void Set(T* ptr)
	{
		this->ptr = ptr;
		if (ptr)
			ptr->AddRef();
	}

	void Release()
	{
		if (ptr)
		{
			ptr->ReleaseRef();
			ptr = nullptr;
		}
	}

public:
	T* ptr;
};

using BulletRef = TSharedPtr<Bullet>;

/*
int main()
{
	Marine* marine = new Marine();
	Bullet* bullet = new Bullet();

	bullet->SetTarget(marine);
	marine->hp = 0;
	marine->ReleaseRef();

	while (true)
	{
		if (bullet)
			bullet->Update();
	}
}
*/

int main()
{
	MarineRef marine(new Marine());
	BulletRef bullet(new Bullet());

	//이 코드는 지금 이렇게 만든 버전에서만 필요함
	{
		marine->ReleaseRef();
		bullet->ReleaseRef();
	}
}