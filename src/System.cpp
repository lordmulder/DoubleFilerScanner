#include "System.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void initializeConsole(const QString &title)
{
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleTitleW((const wchar_t*) title.utf16());
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
