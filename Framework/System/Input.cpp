#include "Input.h"

INPUT_TYPE           Input::m_Keys[256] = { INPUT_TYPE::NONE, };
INPUT_TYPE           Input::m_Mice[3] = { INPUT_TYPE::NONE, };
POINT                Input::m_MousePosition;

IDirectInput8*       Input::m_DirectInput = nullptr;
IDirectInputDevice8* Input::m_KeyBoard = nullptr;
IDirectInputDevice8* Input::m_Mouse = nullptr;
DIMOUSESTATE         Input::m_MouseState;
unsigned char        Input::m_KeyState[256] = { 0, };
int                  Input::m_ScreenWidth = 0;
int                  Input::m_ScreenHeight = 0;

void Input::Initialize(HINSTANCE _hInstance, HWND _hwnd, int _width, int _height)
{
	HRESULT result;

	// 마우스 커서의 위치 지정에 사용될 화면 크기를 설정합니다.
	m_ScreenWidth = _width;
	m_ScreenHeight = _height;

	// Direct Input 인터페이스를 초기화 합니다.
	result = DirectInput8Create(_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_DirectInput, NULL);
	if (FAILED(result))
	{
		return;
	}

	// 키보드의 Direct Input 인터페이스를 생성합니다
	result = m_DirectInput->CreateDevice(GUID_SysKeyboard, &m_KeyBoard, NULL);
	if (FAILED(result))
	{
		return;
	}

	// 데이터 형식을 설정하십시오. 이 경우 키보드이므로 사전 정의 된 데이터 형식을 사용할 수 있습니다.
	result = m_KeyBoard->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(result))
	{
		return;
	}

	// 다른 프로그램과 공유하지 않도록 키보드의 협조 수준을 설정합니다
	result = m_KeyBoard->SetCooperativeLevel(_hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (FAILED(result))
	{
		return;
	}

	// 키보드를 할당받는다
	result = m_KeyBoard->Acquire();
	if (FAILED(result))
	{
		return;
	}

	// 마우스 Direct Input 인터페이스를 생성합니다.
	result = m_DirectInput->CreateDevice(GUID_SysMouse, &m_Mouse, NULL);
	if (FAILED(result))
	{
		return;
	}

	// 미리 정의 된 마우스 데이터 형식을 사용하여 마우스의 데이터 형식을 설정합니다.
	result = m_Mouse->SetDataFormat(&c_dfDIMouse);
	if (FAILED(result))
	{
		return;
	}

	// 다른 프로그램과 공유 할 수 있도록 마우스의 협력 수준을 설정합니다.
	result = m_Mouse->SetCooperativeLevel(_hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
	{
		return;
	}

	// 마우스를 할당받는다
	result = m_Mouse->Acquire();
	if (FAILED(result))
	{
		return;
	}

}

void Input::Update()
{
	HRESULT result = m_KeyBoard->GetDeviceState(sizeof(m_KeyState), (LPVOID)&m_KeyState);
	if (FAILED(result))
	{
		// 키보드가 포커스를 잃었거나 획득되지 않은 경우 컨트롤을 다시 가져 온다
		if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
		{
			m_KeyBoard->Acquire();
		}
		else
		{
			return;
		}
	}
	
	for (int i = 0; i < 256; i++)
	{
		if (m_KeyState[i] & 0x80 && m_Keys[i] == INPUT_TYPE::NONE)
		{
			m_Keys[i] = INPUT_TYPE::DOWN;
		}
		else if (m_KeyState[i] & 0x80 && m_Keys[i] == INPUT_TYPE::DOWN)
		{
			m_Keys[i] = INPUT_TYPE::PRESSED;
		}
		else if (m_KeyState[i] == 0 && m_Keys[i] == INPUT_TYPE::PRESSED)
		{
			m_Keys[i] = INPUT_TYPE::UP;
		}
		else if (m_KeyState[i] == 0 && m_Keys[i] == INPUT_TYPE::UP)
		{
			m_Keys[i] = INPUT_TYPE::NONE;
		}
	}

	// 마우스 디바이스를 얻는다.
	result = m_Mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&m_MouseState);
	if (FAILED(result))
	{
		// 마우스가 포커스를 잃었거나 획득되지 않은 경우 컨트롤을 다시 가져 온다
		if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
		{
			m_Mouse->Acquire();
		}
		else
		{
			return;
		}
	}

	// 프레임 동안 마우스 위치의 변경을 기반으로 마우스 커서의 위치를 ​​업데이트 합니다.
	m_MousePosition.x += m_MouseState.lX;
	m_MousePosition.y += m_MouseState.lY;

	// 마우스 위치가 화면 너비 또는 높이를 초과하지 않는지 확인한다.
	if (m_MousePosition.x < 0)
	{
		m_MousePosition.x = 0;
	}
	
	if (m_MousePosition.y < 0)
	{
		m_MousePosition.y = 0;
	}

	if (m_MousePosition.x > m_ScreenWidth)
	{
		m_MousePosition.x = m_ScreenWidth;
	}

	if (m_MousePosition.y > m_ScreenHeight)
	{
		m_MousePosition.y = m_ScreenHeight;
	}

	for (int i = 0; i < 3; i++)
	{
		if (i != 0)
		{
			continue;
		}

		if (m_MouseState.rgbButtons[i] & 0x80 && m_Mice[i] == INPUT_TYPE::NONE)
		{
			m_Mice[i] = INPUT_TYPE::DOWN;
		}
		else if (m_MouseState.rgbButtons[i] & 0x80 && m_Mice[i] == INPUT_TYPE::DOWN)
		{
			m_Mice[i] = INPUT_TYPE::PRESSED;
		}
		else if (m_MouseState.rgbButtons[i] == 0 && m_Mice[i] == INPUT_TYPE::PRESSED)
		{
			m_Mice[i] = INPUT_TYPE::UP;
		}
		else if (m_MouseState.rgbButtons[i] == 0 && m_Mice[i] == INPUT_TYPE::UP)
		{
			m_Mice[i] = INPUT_TYPE::NONE;
		}
	}
}

void Input::Destroy()
{
	// 마우스를 반환합니다.
	if (m_Mouse)
	{
		m_Mouse->Unacquire();
		m_Mouse->Release();
		m_Mouse = nullptr;
	}

	// 키보드를 반환합니다.
	if (m_KeyBoard)
	{
		m_KeyBoard->Unacquire();
		m_KeyBoard->Release();
		m_KeyBoard = nullptr;
	}

	// m_directInput 객체를 반환합니다.
	if (m_DirectInput)
	{
		m_DirectInput->Release();
		m_DirectInput = nullptr;
	}
}
