#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "Timer.h"

class Application
{
private:

	Application() = default;
	~Application() = default;

	Application(const Application& _rhs) = delete;
	Application& operator= (const Application& _rhs) = delete;
	Application(Application&& _rhs) = delete;
	Application& operator= (Application&& _rhs) = delete;

public:

	static Application* Instance();

	bool    Initialize(HINSTANCE _hInstance);
	int     Run();

private:

	bool    InitializeWnd();
	void    CalculateFrameStats();

private:

	HWND mhMainWnd = NULL;
	HINSTANCE   hAppInst = NULL;

	int	 mWndWidth = 1920;
	int	 mWndHeight = 1080;

	bool mPaused = false;
	bool mResizing = false;

	Timer mTimer;

	std::wstring mMainWndCaption = L"PSEngine";

};

#endif // !_APPLICATION_H_
