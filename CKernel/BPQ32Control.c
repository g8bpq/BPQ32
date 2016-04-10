// Version 1. 0. 0. 1

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>

#include "..\Include\bpq32.h"

VOID APIENTRY RestoreFrameWindow(HWND hWnd);
VOID APIENTRY RecreateIcon(HWND hWnd);
VOID APIENTRY CloseAllBPQ32Windows(HWND hWnd);

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
	MSG msg;
	BOOL bRet;

	Debugprintf("BPQControl %s Entered", lpCmdLine);

	if (_stricmp(lpCmdLine, "NewICON") == 0)				// If AutoRestart then Delay 5 Secs				
	{
		RecreateIcon();
		return 1;
	}

	if (_stricmp(lpCmdLine, "Restore") == 0)				// If AutoRestart then Delay 5 Secs				
	{
		RestoreFrameWindow();
		return 1;
	}

	if (_stricmp(lpCmdLine, "CloseAll") == 0)				// If AutoRestart then Delay 5 Secs				
	{
		CloseAllBPQ32Windows();
		return 1;
	}

	return 1;
}
