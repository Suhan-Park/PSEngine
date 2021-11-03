#include "Application.h"

#ifdef UNICODE
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#else
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

// Visual Studio 2019에서 Win32 프로젝트 생성하기
// https://docs.microsoft.com/ko-kr/cpp/windows/walkthrough-creating-windows-desktop-applications-cpp?view=msvc-160

// https://docs.microsoft.com/ko-kr/windows/win32/direct3d12/direct3d-12-graphics

int WINAPI wWinMain(_In_ HINSTANCE _hInstance,
	_In_opt_ HINSTANCE _hPrevInstance,
	_In_ LPWSTR    _lpCmdLine,
	_In_ int       _nCmdShow)
{
	UNREFERENCED_PARAMETER(_hPrevInstance);
	UNREFERENCED_PARAMETER(_lpCmdLine);

	if (Application::Instance()->Initialize(_hInstance))
	{
		Application::Instance()->Run();
	}
	return 0;
}