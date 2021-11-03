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

// Reset ()�� ȣ�� �� �� ��� �� �� �ð��� ��ȯ�ϸ� �ð谡 ���� �� �ð��� ������� �ʴ´�.
float Timer::TotalTime()const
{
	// Ÿ�̸Ӱ� ���� �����̸�, ������ �������� �帥 �ð��� ������� ���ƾ� �Ѵ�.
	// ����, ������ �̹� �Ͻ� ������ ���� �ִٸ� �ð��� mStopTime - mBaseTime����
	// �Ͻ� ���� ���� �ð��� ���ԵǾ� �ִµ�, �� ���� �ð��� ��ü �ð��� �������� ���ƾ��Ѵ�.
	// �̸� �ٷ���� ����, mStopTime���� �Ͻ� ���� ���� �ð��� ����.
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if (mStopped)
	{
		return (float)(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}

	// �ð��� mCurrTime - mBaseTime���� �Ͻ� ���� ���� �ð��� ���ԵǾ� �ִ�.
	// �̸� ��ü �ð��� �����ϸ� �� �ǹǷ�, �� �ð��� mCurrTime���� ����.
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

	// ����(�Ͻ� ����)�� ���� ���̿� �帥 �ð��� �����Ѵ�.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	// ���� ���¿��� Ÿ�̸Ӹ� �簳�ϴ� ���:
	if (mStopped)
	{
		// �Ͻ� ������ �ð��� �����Ѵ�.
		mPausedTime += (startTime - mStopTime);

		// Ÿ�̸Ӹ� �ٽ� �����ϴ� ���̹Ƿ�, ������ mPrevTime(���� �ð�)��
		// ��ȿ���� �ʴ�.(�Ͻ� ���� ���߿� ���ŵǾ��� ���̹Ƿ�).
		// ���� ���� �ð����� �ٽ� �����Ѵ�.
		mPrevTime = startTime;

		// ������ ���� ���°� �ƴϹǷ� ���� ������� �����Ѵ�.
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

	// ���� �������� �ð��� ���� �������� �ð� ���̸� ���Ѵ�.
	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

	// ���� �������� ���� �غ��Ѵ�.
	mPrevTime = mCurrTime;

	// ������ ���� �ʰ� �Ѵ�. SDK ����ȭ�� CDXUTTimer�� ������,
	// ���μ����� ���� ���� ���ų� ������ �ٸ� ���μ����� 
	// �浹�ϴ� ��� mDeltaTime�� ������ �� �� �ִ�.
	if (mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}