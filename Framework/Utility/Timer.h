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

	float TotalTime()const; // �� ����
	float DeltaTime()const; // �� ����

	void Reset(); // �޽��� ���� ������ ȣ���ؾ� ��.
	void Start(); // Ÿ�̸Ӹ� ���� �Ǵ� �簳�� �� ȣ���ؾ� ��.
	void Stop();  // Ÿ�̸Ӹ� ����(�Ͻ� ����)�� �� ȣ���ؾ� ��.
	void Tick();  // �� ������ ȣ���ؾ� ��.

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