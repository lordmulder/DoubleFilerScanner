#include "System.h"

#include "Config.h"
#include "Resource.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <csignal>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>

#pragma intrinsic(_InterlockedExchange)

typedef BOOL (WINAPI *PSetConsoleIcon)(HICON hIcon);

static volatile HANDLE g_hConsole = INVALID_HANDLE_VALUE;



static void my_invalid_param_handler(const wchar_t* exp, const wchar_t* fun, const wchar_t* fil, unsigned int, uintptr_t)
{
	crashHandler("Invalid parameter handler invoked, application will exit!");
}

static void my_signal_handler(int signal_num)
{
	signal(signal_num, my_signal_handler);
	crashHandler("Signal handler invoked unexpectedly, application will exit!");
}

static LONG WINAPI my_exception_handler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	crashHandler("Unhandeled exception handler invoked, application will exit!");
	return LONG_MAX;
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
	static volatile long consoleLock = 0L;

	static const WORD COLORS[3] =
	{
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
		FOREGROUND_RED | FOREGROUND_INTENSITY,
	};

	while(_InterlockedExchange(&consoleLock, 1L) != 0L)
	{
		Sleep(0);
	}

	if((g_hConsole != NULL) && (g_hConsole != INVALID_HANDLE_VALUE))
	{
		if((logLevel >= 0) && (logLevel <= 2))
		{
			SetConsoleTextAttribute(g_hConsole, COLORS[logLevel]);
		}
	
		DWORD written;
		WriteConsoleA(g_hConsole, text, lstrlenA(text), &written, NULL);
		WriteConsoleA(g_hConsole, "\r\n", 2, &written, NULL);
	}

	_InterlockedExchange(&consoleLock, 0L);
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
