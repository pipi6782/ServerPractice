/*
	이전의 RefCountable과 같이 직접 스마트포인터의 기능을 만들어서 프로젝트에 이용하는 경우도 있다고 한다.
	하지만 이 방식의 경우엔 문제가 있는데
	1. 이미 만들어진 클래스를 대상으로는 사용할 수 없다.
	이전의 코드를 보면 알겠지만 모든 클래스들이 RefCountable을 상속받는 구조이다.
	하지만 이미 만들어진 클래스같은 경우에는 RefCountable을 상속받지 않는다.
	소스코드를 수정할 수 있다면 그나마 다행이지만 그렇지 못하다면 이 방식은 사용할 수 없다는 문제가 있다.
	2. 순환 참조를 하게 되면 메모리 할당이 해제되지 않는 문제가 발생한다.
	이는 표준 shared_ptr에도 존재하는 문제인데
	A의 멤버변수인 shared_ptr로 B를 참조하고 B역시 같은 방식으로 A를 참조하면 두 스마트 포인터가 사라졌을 때
	RefCount는 0인 상태가 아니기 때문에 내부에 동적으로 할당해준 포인터가 살아있는 문제가 생긴다.
*/

/*
	1. unique_ptr
	일반적인 포인터와 다를게 없다고 볼 수 있다.
	다만 다른 점이 하나 있다면 복사연산을 하는 부분이 아예 막혀있다는 것인데,
	이것이 의미하는 바는 한 객체를 가리키는 포인터는 단 하나만 존재할 수 있고 참조가 불가능 하다는 것이다.
	unique_ptr<MyClass> ptr1 = make_unique<MyClass>();
	unique_ptr<MyClass> ptr2 = ptr1;
	과 같은 코드가 불가능 하다는 것이 된다.
	std::move를 이용하여 소유권을 아예 넘겨주지 않는 이상 대입 연산은 불가능하다.

	2. weak_ptr
	shared_ptr의 순환 참조 문제를 해결할 수 있는 스마트 포인터이다.
	weak_ptr은 shared_ptr을 바로 대입해서도 사용할 수 있는데 사용하기 전에 expired 함수를 이용해서
	지금 가리키고 있는 포인터가 유효한지 확인하게 된다. (일종의 널체크와 비슷하다고 볼 수 있다.)

	3. shared_ptr
	이전 코드에서 직접 만들어본 TSharedPtr과 비슷한기능을 한다.
	차이점이 있다면 TSharedPtr은 어떤 공통된 클래스(RefCountable)의 상속을 받아야 하지만
	shared_ptr은 그러지 않고 또 다른 컴포넌트처럼 사용중이라는 것이다.
	shared_ptr로 객체를 할당하게 되면 실제 데이터가 있는 T* 포인터 부분과 RefCount를 관리하는 부분
	두가지로 메모리가 할당되는데 RefCount를 관리하는 곳을 Control Block이라고 한다.
	Control Block 은 _Ref_count_base 라는 클래스로 구현되어 있는데,
	그 내부에는 _Uses, _Weaks 라는 멤버변수가 있다.

	만약 어떤 클래스에서 Getter 함수로 shared_ptr로 래핑된 자기 자신을 가져오고자 한다면
	shared_ptr<T>(this)를 하면 안된다. 해당 shared_ptr과 연결된 Control Block이 별도로 생성되기 때문이다.
	자기 자신을 shared_ptr로 받고 싶다면 별도의 클래스를 상속받은 뒤 그 클래스에 있는 함수를 사용해야 한다.
	그만큼 자주 사용하지 않는 옵션이라는 것이다.
*/

#include <memory>

class MyClass {};

int main()
{
	//unique_ptr은 일반적인 포인터와 비슷하다. 다만 다른점이 있다면 자동으로 할당이 해제 된다는 점과, 다른 unique_ptr의 참조가 불가능 하다는 것이 다르다.
	std::unique_ptr<MyClass> uPtr1 = std::make_unique<MyClass>();
	//단 std::move 함수나 Reference 를 이용한 대입은 가능하다.
	std::unique_ptr<MyClass> uPtr2 = std::move(uPtr1);
	std::unique_ptr<MyClass>& uPtr3 = uPtr1;

	//shared_ptr은 weak_ptr과 함께 세트로 사용할 수 있는 포인터라고 볼 수 있다.
	//shared_ptr은 내부에 자신을 얼마나 참조했는지 알 수 있는 RefCount가 있고 이 정보를 관리하는 Control Block이 존재한다.
	//여기서 문제가 한가지 있는데 두 객체 A,B가 클래스 내부 멤버변수로 각각을 참조한다면 순환참조로 인해서 RefCount가 0이 되지 않는다.
	//그러다보니 순환참조가 일어나면 메모리 할당이 해제되지 않고 이를 해결하기 위해 나온 것이 weak_ptr이다.
	std::shared_ptr<MyClass> sPtr1 = std::make_shared<MyClass>();
	//weak_ptr은 대입연산자로 바로 shared_ptr을 받을 수 있다.
	std::weak_ptr<MyClass> wPtr1 = sPtr1;
	{
		//그리고 weak_ptr의 lock함수를 통해 shared_ptr을 다시 가져올 수도 있는데
		//이 때 기존 shared_ptr의 RefCount가 0일 수도 있어서 expired함수나 lock으로 가져온 shared_ptr의 널체크는 필수적이다.
		std::shared_ptr<MyClass> sPtr2 = wPtr1.lock();
	}

	return 0;
}