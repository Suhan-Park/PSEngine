#ifndef _TIMER_H_
#define _TIMER_H_

class Timer
{
public:

	Timer();
	~Timer();

	Timer(Timer& _rhs) = delete;
	Timer operator = (Timer& _rhs) = delete;
	Timer(Timer&& _rhs) = delete;
	Timer operator = (Timer&& _rhs) = delete;

public:

	float TotalTime()const; // 초 단위
	float DeltaTime()const; // 초 단위

	void Reset(); // 메시지 루프 이전에 호출해야 함.
	void Start(); // 타이머를 시작 또는 재개할 때 호출해야 함.
	void Stop();  // 타이머를 정지(일시 정지)할 떄 호출해야 함.
	void Tick();  // 매 프레임 호출해야 함.

private:

	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mStopped;
};

#endif // _TIMER_H_