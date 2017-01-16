#include <Windows.h>
#include <chrono>

#include "ClantagChanger.h"
#include "Memory.h"

#define IDC_MAIN_BUTTON	101			// Button identifier
#define IDC_MAIN_EDIT	102			// Edit box identifier
#define IDC_MAIN_COMBO 103
#define WINSTYLE (WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX))




constexpr const char* szWindowName = "External Clantag Changer";
constexpr const char* szClassName = "Clantag_Class";

static const char* szAnimationStyles[] = {
	"Static",
	"Scrolling Text",
	"Forming Text"
};

static __forceinline __int64 GetCurrentTimeMS()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string current_clantag = "";
int current_anim_mode = 0;

HWND g_hWnd = 0;
HWND g_hClantagTextBox = 0;
HWND g_hClantagAnimations = 0;

CSGO_Globals Globals;

HINSTANCE g_hProgramHandle = 0;

LRESULT CALLBACK WinProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	g_hProgramHandle = hInstance;
	WNDCLASSEX wClass;
	ZeroMemory(&wClass, sizeof(WNDCLASSEX));
	wClass.cbClsExtra = NULL;
	wClass.cbSize = sizeof(WNDCLASSEX);
	wClass.cbWndExtra = NULL;
	wClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wClass.hIcon = NULL;
	wClass.hIconSm = NULL;
	wClass.hInstance = hInstance;
	wClass.lpfnWndProc = (WNDPROC)WinProc;
	wClass.lpszClassName = szClassName;
	wClass.lpszMenuName = NULL;
	wClass.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wClass))
	{
		MessageBoxA(NULL, 
			"Window Class Registration Failed", 
			"ERROR", 
			MB_ICONERROR);
	}

	g_hWnd = CreateWindowEx(NULL, 
		szClassName,
		szWindowName, 
		WINSTYLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		260, 
		250, 
		NULL,
		NULL,
		hInstance, 
		NULL);

	if (!g_hWnd)
	{
		int error = GetLastError();
		MessageBox(NULL, "Failed to Create Window", "ERROR", MB_ICONERROR);
	}

	ShowWindow(g_hWnd, nCmdShow);


	if (!Globals.CSGOProcess.AttachProcess("csgo.exe"))
	{
		MessageBox(NULL,
			"CSGO is not open!",
			"ERROR",
			MB_ICONERROR);

		return 0;
	}
	else
	{
		Globals.m_dwClientDLL = Globals.CSGOProcess.GetModule("client.dll")->GetImageBase();
		Globals.m_dwEngineDLL = Globals.CSGOProcess.GetModule("engine.dll")->GetImageBase();
	}

	Features::ClantagChanger_Init();

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//Loop
		Features::ClantagChanger_Tick(GetCurrentTimeMS());

	}

	Globals.CSGOProcess.DetachProcess();
	UnregisterClass(szClassName, hInstance);
	return 0;
	
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		g_hClantagTextBox = CreateWindowEx(WS_EX_CLIENTEDGE,
			"EDIT",
			"",
			WS_CHILD | WS_VISIBLE |
			ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
			15,
			100,
			200,
			24,
			hwnd,
			(HMENU)IDC_MAIN_EDIT,
			g_hProgramHandle,
			NULL);
		HGDIOBJ hfDefault = GetStockObject(DEFAULT_GUI_FONT);
		SendMessage(g_hClantagTextBox,
			WM_SETFONT,
			(WPARAM)hfDefault,
			MAKELPARAM(FALSE, 0));
		SendMessage(g_hClantagTextBox,
			WM_SETTEXT,
			NULL,
			(LPARAM)"Clantag goes here...");

		g_hClantagAnimations = CreateWindowEx(NULL,
			"COMBOBOX",
			"",
			CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
			15,
			50,
			200,
			24 * (ARRAYSIZE(szAnimationStyles) + 1),
			hwnd,
			(HMENU)IDC_MAIN_COMBO,
			g_hProgramHandle,
			NULL);

		for (int i = 0; i < ARRAYSIZE(szAnimationStyles); i++)
		{
			const char* szAnimationStyle = szAnimationStyles[i];
			SendMessage(g_hClantagAnimations,
				CB_ADDSTRING,
				0,
				reinterpret_cast<LPARAM>(szAnimationStyle));
		}
		SendMessage(g_hClantagAnimations,
			CB_SETCURSEL,
			(WPARAM)0,
			LPARAM(0));


		HWND hApplyButton = CreateWindowEx(NULL,
			"BUTTON",
			"Apply Clantag Animation",
			WS_TABSTOP | WS_VISIBLE |
			WS_CHILD | BS_DEFPUSHBUTTON,
			15,
			150,
			200,
			24,
			hwnd,
			(HMENU)IDC_MAIN_BUTTON,
			g_hProgramHandle,
			NULL);
		SendMessage(hApplyButton,
			WM_SETFONT,
			(WPARAM)hfDefault,
			MAKELPARAM(FALSE, 0));

		break;
	}
	case WM_COMMAND:
	{
		WORD loword = LOWORD(wparam);
		switch (loword)
		{
		case IDC_MAIN_BUTTON:
		{
			char buffer[256];
			SendMessage(g_hClantagTextBox,
				WM_GETTEXT,
				sizeof(buffer) / sizeof(buffer[0]), 
				reinterpret_cast<LPARAM>(buffer));
			current_clantag = std::string(buffer);
			current_anim_mode = SendMessage(g_hClantagAnimations, 
				CB_GETCURSEL,
				0, 
				0);
			Features::ClantagChanger_SetText(current_clantag);
			Features::ClantagChanger_SetAnimMode(current_anim_mode);
			Features::ClantagChanger_ForceUpdate();
			MessageBox(NULL,
				std::string(std::string("Clantag Applied!\n") + std::string(buffer)).c_str(), 
				"Information",
				MB_ICONINFORMATION);
			break;
		}
		}
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}
