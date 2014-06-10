#include "System.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <csignal>

#pragma intrinsic(_InterlockedExchange)

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
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleTitleW(L"Double File Scanner");
	SetConsoleCtrlHandler(NULL, TRUE);

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
