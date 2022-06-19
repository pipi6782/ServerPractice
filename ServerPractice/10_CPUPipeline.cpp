﻿#include <iostream>
#include <thread>

int x;
int y;
int r1;
int r2;
volatile bool bReady = false;

int main()
{
	int count = 0;
	while (true)
	{
		count++;
		bReady = false;
		x = y = r1 = r2 = 0;
		std::thread t1([]() {while(bReady == false){} y = 1; r1 = x; });
		std::thread t2([]() {while(bReady == false){} x = 1; r2 = y; });
		bReady = true;
		t1.join();
		t2.join();
		if (r1 == 0 && r2 == 0)
			break;
	}

	std::cout << count << std::endl;

	/*
		위와 같은 코드가 있다고 하자.
		분명 두 스레드에서 r1과 r2를 1로 세팅하게끔 되어있기 때문에 저 코드는 무한반복일 것으로 보인다.
		t1이 아주 살짝 먼저 실행될 때 y에는 1이라는 값이 세팅이 되게 되고 r1에는 x에 1을 넣는 작업이 아직 다 끝나지 않았기 때문에 0이 될것이다.
		그리고 t2는 r2에 y를 넣을 때 아주 간발의 차로 1이 들어갔기 때문에 1이 들어갈 것이다.
		그 반대의 경우에도 그럴것이다.
		그런데 실행해보면 언젠가는 빠져나오게 되어있다.
		그 이유는 가시성과 코드 재배치에 있는데 가시성이란 t1에서 변경한 변수의 값을 t2에서 확실하게 확인할 수 있는가 를 의미한다.
		C#에서는 변수에 volatile 키워드를 붙여주면 해결이 되지만 (물론 그래도 이 코드의 문제는 해결되지 않는다.) C++은 좀 다르다.
		volatile의 역할이 코드 최적화 방지에서 끝나기 때문
		그 다음 문제는 코드 재배치인데 아무리 코드를 이렇게 쓰더라도 컴파일러가 "코드의 순서를 바꿔주면 좀 더 효율적으로 동작하겠구나!"하는 부분이 있다면 그 부분은 컴파일러가 임의로 수정하게 된다.
		컴파일러는 멀티스레드를 생각하지 않기 때문에 함수 내의 결과물이 같다고 판단되면 코드의 위치를 과감하게 바꾸게 된다.(컴파일러가 하지 않더라도 CPU에서 그런 짓을 하게 된다)

	*/
}