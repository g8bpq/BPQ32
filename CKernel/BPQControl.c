// Version 1. 0. 0. 1

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>

#include "..\Include\bpq32.h"

VOID APIENTRY RestoreFrameWindow();
VOID APIENTRY CreateNewTrayIcon();
VOID APIENTRY CloseAllPrograms();

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	strcat_s(Mess, 999, "\r\n");
	OutputDebugString(Mess);
	return;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HANDLE Mutex;

	Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

	if (Mutex == NULL)	
		return 0;

	CloseHandle(Mutex);
	
	if (_stricmp(lpCmdLine, "NewICON") == 0)				
	{
		CreateNewTrayIcon();
		return 1;
	}

	if (_stricmp(lpCmdLine, "Restore") == 0)				
	{
		RestoreFrameWindow();
		return 1;
	}

	if (_stricmp(lpCmdLine, "CloseAll") == 0)			
	{
		CloseAllPrograms();
		return 1;
	}

	return 1;
}
