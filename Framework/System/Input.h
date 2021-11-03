#ifndef _INPUT_H_
#define _INPUT_H_

enum class KEYCODE
{
	ESCAPE       = 0x01,
	ONE          = 0x02,
	TWO          = 0x03,
	THREE        = 0x04,
	FOUR         = 0x05,
	FIVE         = 0x06,
	SIX          = 0x07,
	SEVEN        = 0x08,
	EIGHT        = 0x09,
	NINE         = 0x0A,
	ZERO         = 0x0B,
	MINUS        = 0x0C,
	EQUALS       = 0x0D,
	BACK         = 0x0E,
	TAB          = 0x0F,
	Q            = 0x10,
	W            = 0x11,
	E            = 0x12,
	R            = 0x13,
	T            = 0x14,
	Y            = 0x15,
	U            = 0x16,
	I            = 0x17,
	O            = 0x18,
	P            = 0x19,
	LBRACKET     = 0x1A,
	RBRACKET     = 0x1B,
	RETURN       = 0x1C,
	LCONTROL     = 0x1D,
	A            = 0x1E,
	S            = 0x1F,
	D            = 0x20,
	F            = 0x21,
	G            = 0x22,
	H            = 0x23,
	J            = 0x24,
	K            = 0x25,
	L            = 0x26,
	SEMICOLON    = 0x27,
	APOSTROPHE   = 0x28,
	GRAVE        = 0x29,
	LSHIFT       = 0x2A,
	BACKSLASH    = 0x2B,
	Z            = 0x2C,
	X            = 0x2D,
	C            = 0x2E,
	V            = 0x2F,
	B            = 0x30,
	N            = 0x31,
	M            = 0x32,
	COMMA        = 0x33,
	PERIOD       = 0x34,
	SLASH        = 0x35,
	RSHIFT       = 0x36,
	MULTIPLY     = 0x37,
	LMENU        = 0x38,
	SPACE        = 0x39,
	CAPITAL      = 0x3A,
	F1           = 0x3B,
	F2           = 0x3C,
	F3           = 0x3D,
	F4           = 0x3E,
	F5           = 0x3F,
	F6           = 0x40,
	F7           = 0x41,
	F8           = 0x42,
	F9           = 0x43,
	F10          = 0x44,
	NUMLOCK      = 0x45,
	SCROLL       = 0x46,
	NUMPAD7      = 0x47,
	NUMPAD8      = 0x48,
	NUMPAD9      = 0x49,
	SUBTRACT     = 0x4A,
	NUMPAD4      = 0x4B,
	NUMPAD5      = 0x4C,
	NUMPAD6      = 0x4D,
	ADD          = 0x4E,
	NUMPAD1      = 0x4F,
	NUMPAD2      = 0x50,
	NUMPAD3      = 0x51,
	NUMPAD0      = 0x52,
	DECIMAL      = 0x53,
	OEM_102      = 0x56,
	F11          = 0x57,
	F12          = 0x58,
	F13          = 0x64,
	F14          = 0x65,
	F15          = 0x66,
	KANA         = 0x70,
	ABNT_C1      = 0x73,
	CONVERT      = 0x79,
	NOCONVERT    = 0x7B,
	YEN          = 0x7D,
	ABNT_C2      = 0x7E,
	NUMPADEQUALS = 0x8D,
	PREVTRACK    = 0x90,
	AT           = 0x91,
	COLON        = 0x92,
	UNDERLINE    = 0x93,
	KANJI        = 0x94,
	STOP         = 0x95,
	AX           = 0x96,
	UNLABELED    = 0x97,
	NEXTTRACK    = 0x99,
	NUMPADENTER  = 0x9C,
	RCONTROL     = 0x9D,
	MUTE         = 0xA0,
	CALCULATOR   = 0xA1,
	PLAYPAUSE    = 0xA2,
	MEDIASTOP    = 0xA4,
	VOLUMEDOWN   = 0xAE,
	VOLUMEUP     = 0xB0,
	WEBHOME      = 0xB2,
	NUMPADCOMMA  = 0xB3,
	DIVIDE       = 0xB5,
	SYSRQ        = 0xB7,
	RMENU        = 0xB8,
	PAUSE        = 0xC5,
	HOME         = 0xC7,
	UP           = 0xC8,
	PRIOR        = 0xC9,
	LEFT         = 0xCB,
	RIGHT        = 0xCD,
	END          = 0xCF,
	DOWN         = 0xD0,
	NEXT         = 0xD1,
	INSERT       = 0xD2,
	DEL          = 0xD3,
	LWIN         = 0xDB,
	RWIN         = 0xDC,
	APPS         = 0xDD,
	POWER        = 0xDE,
	SLEEP        = 0xDF,
	WAKE         = 0xE3,
	WEBSEARCH    = 0xE5,
	WEBFAVORITES = 0xE6,
	WEBREFRESH   = 0xE7,
	WEBSTOP      = 0xE8,
	WEBFORWARD   = 0xE9,
	WEBBACK      = 0xEA,
	MYCOMPUTER   = 0xEB,
	MAIL         = 0xEC,
	MEDIASELECT  = 0xED 
};

enum class INPUT_TYPE
{
	NONE, DOWN, PRESSED, UP,
};

// 정적 클래스처럼 활동
class Input final
{
	friend class Application;

private:
	
	Input() = delete;
	~Input() = delete;

	Input(Input& _rhs) = delete;
	Input& operator = (Input& _rhs) = delete;
	Input(Input&& _rhs) = delete;
	Input& operator = (Input&& _rhs) = delete;

public:

	static bool GetKey(KEYCODE _keyCode)
	{
		return m_Keys[(unsigned int)_keyCode] == INPUT_TYPE::PRESSED;
	}

	static bool GetKeyDown(KEYCODE _keyCode)
	{
		return m_Keys[(unsigned int)_keyCode] == INPUT_TYPE::DOWN;
	}

	static bool GetKeyUp(KEYCODE _keyCode)
	{
		return m_Keys[(unsigned int)_keyCode] == INPUT_TYPE::UP;
	}

	static bool GetMouseButton(int _buttonIdx)
	{
		return m_Mice[_buttonIdx] == INPUT_TYPE::PRESSED;
	}

	static bool GetMouseButtonDown(int _buttonIdx)
	{
		return m_Mice[_buttonIdx] == INPUT_TYPE::DOWN;
	}

	static bool GetMouseButtonUp(int _buttonIdx)
	{
		return m_Mice[_buttonIdx] == INPUT_TYPE::UP;
	}

	const static POINT MousePosition()
	{
		return m_MousePosition;
	}

private:

	static void Initialize(HINSTANCE _hInstance, HWND _hwnd, int _width, int _height);
	static void Update();
	static void Destroy();

private:

	static INPUT_TYPE m_Keys[256];
	static INPUT_TYPE m_Mice[3];
	static POINT m_MousePosition;

	// 내부 연산용
	static IDirectInput8* m_DirectInput;
	static IDirectInputDevice8* m_KeyBoard;
	static IDirectInputDevice8* m_Mouse;

	static DIMOUSESTATE m_MouseState;
	static unsigned char m_KeyState[256];

	static int m_ScreenWidth;
	static int m_ScreenHeight;

};
#endif