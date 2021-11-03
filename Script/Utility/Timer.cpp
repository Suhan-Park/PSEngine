#include "Timer.h"

Timer::Timer()
	: mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0),
	mPausedTime(0), mStopTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

Timer::~Timer()
{
}

// Reset ()이 호출 된 후 경과 된 총 시간을 반환하며 시계가 중지 된 시간은 계산하지 않는다.
float Timer::TotalTime()const
{
	// 타이머가 정지 상태이면, 정지된 시점부터 흐른 시간은 계산하지 말아야 한다.
	// 또한, 이전에 이미 일시 정지된 적이 있다면 시간차 mStopTime - mBaseTime에는
	// 일시 정지 누적 시간이 포함되어 있는데, 그 누적 시간은 전체 시간에 포함하지 말아야한다.
	// 이를 바로잡기 위해, mStopTime에서 일시 정지 누적 시간을 뺀다.
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if (mStopped)
	{
		return (float)(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}

	// 시간차 mCurrTime - mBaseTime에는 일시 정지 누적 시간이 포함되어 있다.
	// 이를 전체 시간에 포함하면 안 되므로, 그 시간을 mCurrTime에서 뺀다.
	//
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime

	else
	{
		return (float)(((mCurrTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}
}

float Timer::DeltaTime()const
{
	return (float)mDeltaTime;
}

void Timer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

void Timer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	// 정지(일시 정지)와 시작 사이에 흐른 시간을 누적한다.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	// 정시 상태에서 타이머를 재개하는 경우:
	if (mStopped)
	{
		// 일시 정지된 시간을 누적한다.
		mPausedTime += (startTime - mStopTime);

		// 타이머를 다시 시작하느 것이므로, 현재의 mPrevTime(이전 시간)은
		// 유효하지 않다.(일시 정지 도중에 갱신되었을 것이므로).
		// 따라서 현재 시간으로 다시 설정한다.
		mPrevTime = startTime;

		// 이제는 정지 상태가 아니므로 관련 멤버들을 갱신한다.
		mStopTime = 0;
		mStopped = false;
	}
}

void Timer::Stop()
{
	if (!mStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		mStopTime = currTime;
		mStopped = true;
	}
}

void Timer::Tick()
{
	if (mStopped)
	{
		mDeltaTime = 0.0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// 현재 프레임의 시간과 이전 프레임의 시간 차이를 구한다.
	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

	// 다음 프레임을 위해 준비한다.
	mPrevTime = mCurrTime;

	// 음수가 되지 않게 한다. SDK 문서화의 CDXUTTimer에 따르면,
	// 프로세서가 절전 모드로 들어가거나 실행이 다른 프로세서와 
	// 충돌하는 경우 mDeltaTime이 음수가 될 수 있다.
	if (mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}