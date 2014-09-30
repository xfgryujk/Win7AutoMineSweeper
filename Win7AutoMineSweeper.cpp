#include "stdafx.h"
#include <tlhelp32.h>
#include <string>
#ifdef _UNICODE
#define tstring wstring
#else
#define tstring string
#endif
#include <algorithm>


// Get the module handle of minesweeper.exe
DWORD GetMineSweeperModuleHandle(DWORD pid)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	MODULEENTRY32 moduleentry;
	moduleentry.dwSize = sizeof(moduleentry);

	BOOL flag = Module32First(snapshot, &moduleentry);
	HANDLE handle = NULL;
	do
	{
		std::tstring moduleName = moduleentry.szModule;
		std::transform(moduleName.begin(), moduleName.end(), moduleName.begin(), tolower);
		if(moduleName == _T("minesweeper.exe"))
		{
			handle = moduleentry.hModule;
			break;
		}
		flag = Module32Next(snapshot, &moduleentry);
	}while(flag);

	CloseHandle(snapshot);
	return (DWORD)handle;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
 	HWND hwnd = FindWindow(_T("Minesweeper"), NULL);
	if(hwnd == NULL)
	{
		MessageBox(NULL, _T("Minesweeper is not found!"), _T("AutoMineSweeper"), MB_ICONSTOP);
		return 1;
	}

	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);

	DWORD minesweeper = GetMineSweeperModuleHandle(pid);
	if(minesweeper == 0)
	{
		MessageBox(NULL, _T("Failed to get module handle!"), _T("AutoMineSweeper"), MB_ICONSTOP);
		return 2;
	}

	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if(handle == NULL)
	{
		MessageBox(NULL, _T("Failed to open process!"), _T("AutoMineSweeper"), MB_ICONSTOP);
		return 3;
	}

	DWORD address;
	ReadProcessMemory(handle, (LPVOID)(minesweeper + 0x868B4), &address, sizeof(DWORD), NULL);

	// height : [minesweeper.exe+0x7E1BC]
	// width  : [[[minesweeper.exe+0x868B4]+0x10]+0x0C]
	int width, height;
	ReadProcessMemory(handle, (LPVOID)(minesweeper + 0x7E1BC), &height, sizeof(int), NULL);
	ReadProcessMemory(handle, (LPVOID)(address + 0x10), &address, sizeof(DWORD), NULL);
	ReadProcessMemory(handle, (LPVOID)(address + 0x0C), &width, sizeof(int), NULL);
	
	// data   : [[[[[[[minesweeper.exe+0x868B4]+0x10]+0x40]+0x0C]+X*4]+0x0C]+Y*4]
	// isMine : [[[[[[[minesweeper.exe+0x868B4]+0x10]+0x44]+0x0C]+X*4]+0x0C]+Y]
	DWORD address2, address3;
	ReadProcessMemory(handle, (LPVOID)(address + 0x40), &address2, sizeof(DWORD), NULL);
	ReadProcessMemory(handle, (LPVOID)(address2 + 0x0C), &address2, sizeof(DWORD), NULL);

	DWORD address4, address5;
	ReadProcessMemory(handle, (LPVOID)(address + 0x44), &address4, sizeof(DWORD), NULL);
	ReadProcessMemory(handle, (LPVOID)(address4 + 0x0C), &address4, sizeof(DWORD), NULL);

	ShowWindow(hwnd, SW_NORMAL);
	SetWindowPos(hwnd, NULL, 0, 0, 76 + width * 18, 121 + height * 18, SWP_NOMOVE);
	SetForegroundWindow(hwnd);
	Sleep(300);
	for(int x = 0; x < width; x++)
	{
		ReadProcessMemory(handle, (LPVOID)(address2 + x * 4), &address3, sizeof(DWORD), NULL);
		ReadProcessMemory(handle, (LPVOID)(address3 + 0x0C), &address3, sizeof(DWORD), NULL);

		ReadProcessMemory(handle, (LPVOID)(address4 + x * 4), &address5, sizeof(DWORD), NULL);
		ReadProcessMemory(handle, (LPVOID)(address5 + 0x0C), &address5, sizeof(DWORD), NULL);
		for(int y = 0; y < height; y++)
		{
			DWORD data;
			ReadProcessMemory(handle, (LPVOID)(address3 + y * 4), &data, sizeof(DWORD), NULL);
			BYTE isMine;
			ReadProcessMemory(handle, (LPVOID)(address5 + y), &isMine, sizeof(BYTE), NULL);

			UINT downMsg = 0, upMsg;
			if(data == 9) // Have not been clicked
			{
				if(isMine == 0)
				{
					downMsg = MOUSEEVENTF_LEFTDOWN;
					upMsg   = MOUSEEVENTF_LEFTUP;
				}
				else
				{
					downMsg = MOUSEEVENTF_RIGHTDOWN;
					upMsg   = MOUSEEVENTF_RIGHTUP;
				}
			}
			if(downMsg != 0)
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				POINT curPos = {rect.left + 40 + x * 18
    			              , rect.top  + 40 + y * 18};
				ClientToScreen(hwnd, &curPos);
				SetCursorPos(curPos.x, curPos.y);
				mouse_event(downMsg, 0, 0, 0, 0);
				Sleep(30);
				mouse_event(upMsg, 0, 0, 0, 0);
			}
		}
	}
	
	CloseHandle(handle);

	return 0;
}
