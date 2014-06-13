///////////////////////////////////////////////////////////////////////////////
// Double File Scanner
// Copyright (C) 2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#include "System.h"

#include "Config.h"
#include "Resource.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShellAPI.h>

#include <csignal>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <cstring>

#include <QWidget>
#include <QIcon>

#pragma intrinsic(_InterlockedExchange)

typedef BOOL (WINAPI *PSetConsoleIcon)(HICON hIcon);

static volatile HANDLE g_hConsole = INVALID_HANDLE_VALUE;
static volatile long g_consoleLock = 0L;

static void my_invalid_param_handler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
	crashHandler("Invalid parameter handler invoked, application will exit!");
}

static void my_signal_handler(int signal_num)
{
	signal(signal_num, my_signal_handler);
	crashHandler("Signal handler invoked unexpectedly, application will exit!");
}

static LONG WINAPI my_exception_handler(struct _EXCEPTION_POINTERS*)
{
	crashHandler("Unhandeled exception handler invoked, application will exit!");
	//return LONG_MAX;
}

void initErrorHandlers()
{
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	SetUnhandledExceptionFilter(my_exception_handler);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	_set_invalid_parameter_handler(my_invalid_param_handler);
	
	static const int signal_num[6] = { SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM };

	for(size_t i = 0; i < 6; i++)
	{
		signal(signal_num[i], my_signal_handler);
	}
}

void initConsole(void)
{
	while(_InterlockedExchange(&g_consoleLock, 1L) != 0L)
	{
		Sleep(0);
	}

	if((g_hConsole != NULL) && (g_hConsole != INVALID_HANDLE_VALUE))
	{
		_InterlockedExchange(&g_consoleLock, 0L);
		return;
	}

	if(AllocConsole())
	{
		g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		SetConsoleOutputCP(CP_UTF8);
		SetConsoleTitleW(L"Double File Scanner");
		SetConsoleCtrlHandler(NULL, TRUE);

		const int flags = _O_WRONLY | _O_U8TEXT;
		int hCrtStdOut = _open_osfhandle((intptr_t) GetStdHandle(STD_OUTPUT_HANDLE), flags);
		int hCrtStdErr = _open_osfhandle((intptr_t) GetStdHandle(STD_ERROR_HANDLE),  flags);
		FILE *hfStdOut = (hCrtStdOut >= 0) ? _fdopen(hCrtStdOut, "wb") : NULL;
		FILE *hfStdErr = (hCrtStdErr >= 0) ? _fdopen(hCrtStdErr, "wb") : NULL;
		if(hfStdOut) { *stdout = *hfStdOut; std::cout.rdbuf(new std::filebuf(hfStdOut)); }
		if(hfStdErr) { *stderr = *hfStdErr; std::cerr.rdbuf(new std::filebuf(hfStdErr)); }

		HWND hwndConsole = GetConsoleWindow();
		if((hwndConsole != NULL) && (hwndConsole != INVALID_HANDLE_VALUE))
		{
			HMENU hMenu = GetSystemMenu(hwndConsole, 0);
			EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
			RemoveMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);

			SetWindowPos(hwndConsole, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
			SetWindowLong(hwndConsole, GWL_STYLE, GetWindowLong(hwndConsole, GWL_STYLE) & (~WS_MAXIMIZEBOX) & (~WS_MINIMIZEBOX));
			SetWindowPos(hwndConsole, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
		}

		if(HMODULE kernel32 = GetModuleHandleA("kernel32.dll"))
		{
			if(PSetConsoleIcon setConsoleIcon = (PSetConsoleIcon) GetProcAddress(kernel32, "SetConsoleIcon"))
			{
				if(HICON hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON1)))
				{
					setConsoleIcon(hIcon);
				}
			}
		}
	}
}

void printConsole(const char* text, const int &logLevel)
{
	static const WORD COLORS[3] =
	{
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
		FOREGROUND_RED | FOREGROUND_INTENSITY,
	};

	while(_InterlockedExchange(&g_consoleLock, 1L) != 0L)
	{
		Sleep(0);
	}

	if((g_hConsole == NULL) || (g_hConsole == INVALID_HANDLE_VALUE))
	{
		_InterlockedExchange(&g_consoleLock, 0L);
		return;
	}

	if((logLevel >= 0) && (logLevel <= 2))
	{
		SetConsoleTextAttribute(g_hConsole, COLORS[logLevel]);
	}
	
	char buffer[256];
	strncpy_s(buffer, 256, text, _TRUNCATE);
	const size_t len = lstrlenA(buffer);
	buffer[len] = '\n'; //replace \0 with \n

	DWORD written;
	WriteConsoleA(g_hConsole, buffer, len+1, &written, NULL);

	_InterlockedExchange(&g_consoleLock, 0L);
}

void crashHandler(const char *message)
{
	static volatile long bFatalFlag = 0L;
	
	if(_InterlockedExchange(&bFatalFlag, 1L) == 0L)
	{
		MessageBoxA(NULL, message, "GURU MEDITATION", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND);
	}

	for(;;)
	{
		TerminateProcess(GetCurrentProcess(), 666);
	}
}

quint32 getCurrentThread(void)
{
	return GetCurrentThreadId();
}

void shellExplore(const wchar_t *path)
{
	wchar_t commandLine[512];
	_snwprintf_s(commandLine, 512, _TRUNCATE, L"explorer.exe /select,\"%s\"", path);

	STARTUPINFOW startupInfo;
	memset(&startupInfo, 0, sizeof(STARTUPINFOW));
	startupInfo.cb = sizeof(STARTUPINFOW);

	PROCESS_INFORMATION processInfo;
	memset(&processInfo, 0, sizeof(PROCESS_INFORMATION));

	if(CreateProcessW(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
	{
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
}

static HICON qicon2hicon(const QIcon &icon, const int w, const int h)
{
	if(!icon.isNull())
	{
		QPixmap pixmap = icon.pixmap(w, h);
		if(!pixmap.isNull())
		{
			return pixmap.toWinHICON();
		}
	}
	return NULL;
}

void changeWindowIcon(QWidget *window, const QIcon &icon, const bool bIsBigIcon)
{
	if(!icon.isNull())
	{
		const int extend = (bIsBigIcon ? 32 : 16);
		if(HICON hIcon = qicon2hicon(icon, extend, extend))
		{
			SendMessage(window->winId(), WM_SETICON, (bIsBigIcon ? ICON_BIG : ICON_SMALL), LPARAM(hIcon));
		}
	}
}
