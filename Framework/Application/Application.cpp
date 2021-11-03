#include "Application.h"
#include "D3DApplication.h"
#include "Input.h"

LRESULT CALLBACK WndProc(HWND _hwnd, UINT _msg, WPARAM _wParam, LPARAM _lParam)
{
	switch (_msg)
	{
	case WM_SIZE:
	{
		if (_wParam == SIZE_MINIMIZED) {}
		else {}

		return 0;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	case WM_GETMINMAXINFO:
	{
		((MINMAXINFO*)_lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)_lParam)->ptMinTrackSize.y = 200;
		return 0;
	}
	default:
		return DefWindowProc(_hwnd, _msg, _wParam, _lParam);
	}
}

Application* Application::Instance()
{
	static Application app;
	return &app;
}

bool Application::Initialize(HINSTANCE _hInstance)
{
	hAppInst = _hInstance;

	if (!InitializeWnd())
	{
		return false;
	}

	if (!D3DApplication::Instance()->Initialize(mhMainWnd, mWndWidth, mWndHeight))
	{
		return false;
	}

	D3DApplication::Instance()->OnResize(mhMainWnd, mWndWidth, mWndHeight);
	Input::Initialize(hAppInst, mhMainWnd, mWndWidth, mWndHeight);

	return true;
}

int Application::Run()
{
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		// ������ �޽����� �߻��ϸ� ó���Ѵ�.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// �׷��� ������, ���� ������ ó���Ѵ�.
		else
		{
			mTimer.Tick();
			CalculateFrameStats();

			Input::Update();
			D3DApplication::Instance()->Run(mTimer);

			if (!mPaused)
			{

			}
			else
			{
				//Sleep(100);
			}
		}
	}

	D3DApplication::Instance()->Quit();
	Input::Destroy();

	return (int)msg.wParam;
}

bool Application::InitializeWnd()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, mWndWidth, mWndHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, hAppInst, 0);

	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

void Application::CalculateFrameStats()
{
	// �� �������� �������ϴ� �� �ɸ� �ð��� ��� �� ������ ĸ�� ǥ�� �ٿ� �߰��Ѵ�.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}
