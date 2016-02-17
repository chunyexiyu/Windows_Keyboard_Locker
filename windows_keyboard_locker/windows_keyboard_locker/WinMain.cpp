#define _WIN32_IE 0x0500

#define NOTI_LOCKED        L"键盘已锁定"
#define NOTI_UNLOCKED      L"键盘已解锁"
#define NOTI_LOCK_FAILED   L"键盘锁定失败"
#define NOTI_UNLOCK_FAILED L"键盘解锁失败"

#include <Windows.h>
#include <string>
#include <strsafe.h>
#include "resource.h"
using namespace std;
enum
{
	WM_TRAYICON = WM_USER + 100, //自定义托盘消息
	ID_LOCK,
	ID_UNLOCK,
	ID_EXIT
};


HHOOK KBhook = NULL;//钩子句柄
HINSTANCE hInst;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);//键盘钩子回调函数

void TrayMessage(HWND hwnd, int nFlag);//显示托盘气泡信息
void DestroyTrayIcon(HWND hwnd);//删除托盘图标
void CreateTrayMenu(HWND hwnd);//建立托盘菜单
BOOL isValidMessage(WORD message);          //验证信息的合法性

/*void PopUpMenu(ClipBoard clipboard, POINT point);   */      //弹出选择菜单
VOID APIENTRY DisplayContextMenu(HWND hwnd, POINT pt);

int EnableKeyboardCapture();//激活键盘钩子
int DisableKeyboardCapture();//解除键盘钩子

void OnLock(HWND hwnd);
void OnUnlock(HWND hwnd);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	HWND hwnd;
	MSG msg;
	NOTIFYICONDATA IconData;
	WNDCLASS wndclass;
	static TCHAR szAppName[] = { L"Keyboard Lock" };
	hInst = hInstance;
	//初始化窗口
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hCursor = LoadCursor(NULL, IDI_APPLICATION);
	wndclass.hIcon = LoadIcon(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.hInstance = hInstance;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	RegisterClass(&wndclass);

	hwnd = CreateWindow(szAppName, szAppName, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, 0, 0, hInstance, NULL);

	//这里之后不需要显示窗口，新建托盘图标，操作全部在托盘图标上完成
	//Warning: HWND 和 HINSTANCE 不对的话，就会有问题
	IconData.cbSize = sizeof(NOTIFYICONDATA);
	IconData.hWnd = hwnd;
	IconData.uID = (UINT)hInstance;
	//IconData.hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_ICON1));
	IconData.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	IconData.uFlags = NIF_MESSAGE + NIF_ICON + NIF_TIP;
	IconData.uCallbackMessage = WM_TRAYICON;
	StringCchCopy(IconData.szTip, ARRAYSIZE(IconData.szTip), L"点击设置");
	//strcpy(IconData.szTip, "键盘锁");

	Shell_NotifyIcon(NIM_ADD, &IconData);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == 0)
		{
			//菜单动作的消息
			switch (LOWORD(wParam))
			{
			case ID_LOCK:
				OnLock(hwnd);
				break;

			case ID_UNLOCK:
				OnUnlock(hwnd);
				break;

			case ID_EXIT:
				::DestroyWindow(hwnd);
				break;
			}
		}
		break;

	case WM_TRAYICON:
		switch (lParam)
		{
		case WM_LBUTTONDOWN:
			CreateTrayMenu(hwnd);
			break;
		}
		break;

	case WM_DESTROY:
		DestroyTrayIcon(hwnd);
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}


LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	//是键盘的动作全部忽略
	if (nCode == HC_ACTION)
	{
		return 1;
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}


BOOL isValidMessage(WORD message)
{
	return TRUE;
}

string copyToCustomeClipBoard()
{
	char * buffer = NULL;
	//打开剪贴板
	string fromClipboard;
	if (OpenClipboard(NULL))
	{
		HANDLE hData = GetClipboardData(CF_TEXT);
		char * buffer = (char*)GlobalLock(hData);
		fromClipboard = buffer;
		GlobalUnlock(hData);
		CloseClipboard();

	}

	return fromClipboard;
}



//the function to display shortcut menu
VOID APIENTRY DisplayContextMenu(HWND hwnd, POINT pt)
{
	HMENU hmenu;            // top-level menu 
	HMENU hmenuTrackPopup;  // shortcut menu 

	// Load the menu resource. 

	if ((hmenu = LoadMenu(NULL, L"ShortcutExample")) == NULL)
		return;

	// TrackPopupMenu cannot display the menu bar so get 
	// a handle to the first shortcut menu. 

	hmenuTrackPopup = GetSubMenu(hmenu, 0);

	// Display the shortcut menu. Track the right mouse 
	// button. 

	TrackPopupMenu(hmenuTrackPopup,
		TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		pt.x, pt.y, 0, hwnd, NULL);

	// Destroy the menu. 

	DestroyMenu(hmenu);
}

int EnableKeyboardCapture()
{
	// 13 表示使用低级键盘钩子，和 WM_KEYBOARD 不同
	KBhook = !KBhook ? SetWindowsHookEx(13, (HOOKPROC)KeyboardHookProc, (HINSTANCE)GetModuleHandle(NULL), 0) : KBhook;

	return (KBhook ? 1 : -1);
}


int DisableKeyboardCapture()
{
	if (KBhook == NULL)
	{
		return 0;
	}
	else
	{
		BOOL flag;
		flag = UnhookWindowsHookEx(KBhook);
		KBhook = NULL;
		return (flag ? 1 : -1);
	}
}


void TrayMessage(HWND hwnd, int nFlag)
{
	NOTIFYICONDATA nib = {};
	nib.cbSize = sizeof(NOTIFYICONDATA);
	nib.hWnd = hwnd;
	nib.uID = (UINT)GetModuleHandle(NULL);
	nib.uFlags = NIF_INFO | NIF_ICON;
	nib.dwInfoFlags = NIIF_INFO;
	nib.uTimeout = 1000;
	nib.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	//StringCchCopy(nid.szTip, ARRAYSIZE(nid.szTip), L"Windows KeyBoard Locker");
	//wsprintf(nid.szInfo, szText);
	switch (nFlag)
	{
	case 0:
		StringCchCopy(nib.szInfo, ARRAYSIZE(nib.szInfo), NOTI_UNLOCKED);
		break;
	case 1:
		StringCchCopy(nib.szInfo, ARRAYSIZE(nib.szInfo), NOTI_LOCKED);
		break;
	case 2:
		StringCchCopy(nib.szInfo, ARRAYSIZE(nib.szInfo), NOTI_UNLOCK_FAILED);
		break;
	case 3:
		StringCchCopy(nib.szInfo, ARRAYSIZE(nib.szInfo), NOTI_LOCK_FAILED);
		break;
	default:
		break;
	}
	/*strcpy(nid.szInfoTitle, (TCHAR )"键盘锁");*/
	Shell_NotifyIcon(NIM_MODIFY, &nib);
}


void DestroyTrayIcon(HWND hwnd)
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(nid);
	nid.uID = (UINT)GetModuleHandle(NULL);
	nid.hWnd = hwnd;
	nid.uFlags = 0;
	Shell_NotifyIcon(NIM_DELETE, &nid);
}


void CreateTrayMenu(HWND hwnd)
{
	HMENU hMenu;
	hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_LOCK, L"锁定键盘");
	AppendMenu(hMenu, MF_STRING, ID_UNLOCK, L"解锁键盘");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, ID_EXIT, L"退出");

	POINT pt;
	GetCursorPos(&pt);

	TrackPopupMenu(hMenu, TPM_RIGHTALIGN, pt.x, pt.y, 0, hwnd, NULL);
}


void OnUnlock(HWND hwnd)
{
	switch (DisableKeyboardCapture())
	{
	case 0:
		TrayMessage(hwnd, 3);
		break;
	case 1:
		TrayMessage(hwnd, 0);
		break;
	case -1:
		TrayMessage(hwnd, 2);
		break;
	}
}


void OnLock(HWND hwnd)
{
	switch (EnableKeyboardCapture())
	{
	case 1:
		TrayMessage(hwnd, 1);  //KeyBoard lock Success
		break;

	case -1:
		TrayMessage(hwnd, 3);  //KeyBoard lock failed
		break;
	}
}
