
// Version 1. 0. 0. 1 June 2013

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>
#include <Psapi.h>

#define BPQICON 400

WSADATA WsaData;            // receives data from WSAStartup


HINSTANCE hInst; 
char AppName[] = "WinmorControl";
char Title[80] = "WinmorControl";

// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);

int TimerHandle = 0;

LOGFONT LFTTYFONT ;

HFONT hFont ;

struct sockaddr_in sinx; 
struct sockaddr rx;
SOCKET sock;
int udpport = 8500;
int addrlen = sizeof(struct sockaddr_in);


BOOL MinimizetoTray=FALSE;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}

BOOL KillOldTNC(char * Path)
{
	HANDLE hProc;
	char ExeName[256] = "";
	DWORD Pid = 0;

    DWORD Processes[1024], Needed, Count;
    unsigned int i;

    if (!EnumProcesses(Processes, sizeof(Processes), &Needed))
        return FALSE;
    
    // Calculate how many process identifiers were returned.

    Count = Needed / sizeof(DWORD);

    // Print the name and process identifier for each process.

    for (i = 0; i < Count; i++)
    {
        if (Processes[i] != 0)
        {
			hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Processes[i]);
	
			if (hProc)
			{
				GetModuleFileNameEx(hProc, 0,  ExeName, 255);

				if (_stricmp(ExeName, Path) == 0)
				{
					Debugprintf("Killing Pid %d %s", Processes[i], ExeName);
					TerminateProcess(hProc, 0);
					CloseHandle(hProc);
					return TRUE;
				}
				CloseHandle(hProc);
			}
		}
	}
	return FALSE;
}

KillTNC(int PID)
{
	HANDLE hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);

	Debugprintf("KillTNC Called Pid %d", PID);

	if (hProc)
	{
		TerminateProcess(hProc, 0);
		CloseHandle(hProc);
	}

	return 0;
}

RestartTNC(char * Path)
{
	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 
	int n = 0;

	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
  	SInfo.lpReserved2=NULL; 

	Debugprintf("RestartTNC Called for %s", Path);

	while (KillOldTNC(Path) && n++ < 100)
	{
		Sleep(100);
	}

	if (CreateProcess(Path, NULL, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo))
		Debugprintf("Restart TNC OK");
	else
		Debugprintf("Restart TNC Failed %d ", GetLastError());
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;

	if (!InitApplication(hInstance)) 
		return (FALSE);

	if (!InitInstance(hInstance, nCmdShow))
		return (FALSE);


	// Main message loop:

	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	KillTimer(NULL, TimerHandle);

	return (msg.wParam);
}

//

//
//  FUNCTION: InitApplication(HANDLE)
//
//  PURPOSE: Initializes window data and registers window class 
//
//  COMMENTS:
//
//       In this function, we initialize a window class by filling out a data
//       structure of type WNDCLASS and calling either RegisterClass or 
//       the internal MyRegisterClass.
//
BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;

	// Fill in window class structure with parameters that describe
    // the main window.
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = (WNDPROC)WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

//		wc.lpszMenuName =  MAKEINTRESOURCE(BPQMENU) ;
 
        wc.lpszClassName = AppName;

        // Register the window class and return success/failure code.

		return RegisterClass(&wc);
      
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window 
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

HFONT FAR PASCAL MyCreateFont( void ) 
{ 
    CHOOSEFONT cf; 
    LOGFONT lf; 
    HFONT hfont; 
 
    // Initialize members of the CHOOSEFONT structure.  
 
    cf.lStructSize = sizeof(CHOOSEFONT); 
    cf.hwndOwner = (HWND)NULL; 
    cf.hDC = (HDC)NULL; 
    cf.lpLogFont = &lf; 
    cf.iPointSize = 0; 
    cf.Flags = CF_SCREENFONTS | CF_FIXEDPITCHONLY; 
    cf.rgbColors = RGB(0,0,0); 
    cf.lCustData = 0L; 
    cf.lpfnHook = (LPCFHOOKPROC)NULL; 
    cf.lpTemplateName = (LPSTR)NULL; 
    cf.hInstance = (HINSTANCE) NULL; 
    cf.lpszStyle = (LPSTR)NULL; 
    cf.nFontType = SCREEN_FONTTYPE; 
    cf.nSizeMin = 0; 
    cf.nSizeMax = 0; 
 
    // Display the CHOOSEFONT common-dialog box.  
 
    ChooseFont(&cf); 
 
    // Create a logical font based on the user's  
    // selection and return a handle identifying  
    // that font.  
 
    hfont = CreateFontIndirect(cf.lpLogFont); 
    return (hfont); 
} 



BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Msg[255];
	int ret, err;


	hInst = hInstance; // Store instance handle in our global variable

	WSAStartup(MAKEWORD(2, 0), &WsaData);

	hWnd = CreateWindow(AppName, Title, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 350,50,
		NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return (FALSE);
	}
		
	// setup default font information

   LFTTYFONT.lfHeight =			12;
   LFTTYFONT.lfWidth =          8 ;
   LFTTYFONT.lfEscapement =     0 ;
   LFTTYFONT.lfOrientation =    0 ;
   LFTTYFONT.lfWeight =         0 ;
   LFTTYFONT.lfItalic =         0 ;
   LFTTYFONT.lfUnderline =      0 ;
   LFTTYFONT.lfStrikeOut =      0 ;
   LFTTYFONT.lfCharSet =        OEM_CHARSET ;
   LFTTYFONT.lfOutPrecision =   OUT_DEFAULT_PRECIS ;
   LFTTYFONT.lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
   LFTTYFONT.lfQuality =        DEFAULT_QUALITY ;
   LFTTYFONT.lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
   lstrcpy( LFTTYFONT.lfFaceName, "Fixedsys" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;
//	hFont = MyCreateFont();

	SetWindowText(hWnd,Title);

	ShowWindow(hWnd, nCmdShow);

	sock = socket(AF_INET,SOCK_DGRAM,0);

	if (sock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		sprintf(Msg, "Failed to create UDP socket - error code = %d", err);
		MessageBox(NULL, Msg, "WinmorCOntrol", MB_OK);
		return FALSE; 
	}

	ioctlsocket (sock, FIONBIO, &param);
 
	setsockopt (sock,SOL_SOCKET,SO_BROADCAST,(const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	
	sinx.sin_port = htons(udpport);

	ret = bind(sock, (struct sockaddr *) &sinx, sizeof(sinx));

	if (ret != 0)
	{
		//	Bind Failed

		err = WSAGetLastError();
		sprintf(Msg, "Bind Failed for UDP socket - error code = %d", err);
		MessageBox(NULL, Msg, "WinmorCOntrol", MB_OK);

		return FALSE;
	}

	TimerHandle = SetTimer(hWnd, 1, 1000, NULL);		// Slow Timer

	return (TRUE);
}

VOID Poll()
{
	char Msg[256];
	int len;

	len = recvfrom(sock, Msg, 256, 0, &rx, &addrlen);

	if (len <= 0)
		return;

	Msg[len] = 0;

	if (_memicmp(Msg, "REMOTE:", 7) == 0)
		RestartTNC(&Msg[7]);

	if (_memicmp(Msg, "KILL ", 5) == 0)
		KillTNC(atoi(&Msg[5]));
	
}
	
//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) { 

		case WM_TIMER:

			Poll();

			return 0;


		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

	
		case  SC_MINIMIZE: 

			if (MinimizetoTray)

				return ShowWindow(hWnd, SW_HIDE);
			else
				return (DefWindowProc(hWnd, message, wParam, lParam));
				
				
			break;
		
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}


	case WM_CLOSE:
		
			PostQuitMessage(0);
			
			break;


		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}

	return (0);
}
