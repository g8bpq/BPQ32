
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>
#include <htmlhelp.h>

#include "bpqterminal.h"
#include "bpq32.h"	// BPQ32 API Defines
#include "GetVersion.h"

#define BGCOLOUR RGB(236,233,216)

HBRUSH bgBrush;

HINSTANCE hInst; 
char AppName[] = "BPQTerm 32";
char Title[80];

char ClassName[]="BPQMAINWINDOW";

// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
int NewLine();

int	ProcessBuff(char * readbuff,int len);

int EnableConnectMenu(HWND hWnd);
int EnableDisconnectMenu(HWND hWnd);
int	DisableConnectMenu(HWND hWnd);
int DisableDisconnectMenu(HWND hWnd);
int	ToggleAutoConnect(HWND hWnd);
int ToggleAppl(HWND hWnd, int Item, int mask);
int DoReceivedData(HWND hWnd);
int	DoStateChange(HWND hWnd);
int ToggleFlags(HWND hWnd, int Item, int mask);
int CopyScreentoBuffer(char * buff);
int DoMonData(HWND hWnd);
int TogglePort(HWND hWnd, int Item, int mask);
int ToggleMTX(HWND hWnd);
int ToggleMCOM(HWND hWnd);
int ToggleBells(HWND hWnd);
int ToggleChat(HWND hWnd);
void MoveWindows();
void CopyToClipboard(HWND hWnd);


LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;


WNDPROC wpOrigInputProc; 
WNDPROC wpOrigOutputProc; 
WNDPROC wpOrigMonProc; 

HWND hwndInput;
HWND hwndOutput;
HWND hwndMon;


int xsize,ysize;		// Screen size at startup

double Split=0.5;

RECT Rect;

int Height, Width, LastY;

char Key[80];
int portmask=0xffff;
int mtxparam=1;
int mcomparam=1;

char kbbuf[160];
int kbptr=0;
char readbuff[1024];

int ptr=0;

int Stream;
int len,count;

char callsign[10];
int state;
int change;
int applmask = 0;
int applflags = 2;				// Message to Application
int Sessno = 0;

BOOL Bells = FALSE;

HCURSOR DragCursor;
HCURSOR	LastCursor;

CONNECTED=FALSE;
AUTOCONNECT=TRUE;

LOGFONT LFTTYFONT ;

HFONT hFont ;

BOOL MinimizetoTray=FALSE;

HWND MainWnd;

//
//  FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)
//
//  PURPOSE: Entry point for the application.
//
//  COMMENTS:
//
//	This function initializes the application and processes the
//	message loop.
//
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)

{
	MSG msg;
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];

	sscanf(lpCmdLine,"%d %x",&Sessno, &applflags);

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

	// Save Config
	
	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              Key,
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

	if (retCode == ERROR_SUCCESS)
	{
		retCode = RegSetValueEx(hKey,"PortMask",0,REG_DWORD,(BYTE *)&portmask,4);
		retCode = RegSetValueEx(hKey,"Bells",0,REG_DWORD,(BYTE *)&Bells,4);
		retCode = RegSetValueEx(hKey,"ApplMask",0,REG_DWORD,(BYTE *)&applmask,4);
		retCode = RegSetValueEx(hKey,"MTX",0,REG_DWORD,(BYTE *)&mtxparam,4);
		retCode = RegSetValueEx(hKey,"MCOM",0,REG_DWORD,(BYTE *)&mcomparam,4);
		retCode = RegSetValueEx(hKey,"Split",0,REG_BINARY,(BYTE *)&Split,sizeof(Split));
		wsprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
		retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));



		RegCloseKey(hKey);
	}


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

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( BPQICON ) );
    wc.hCursor = NULL; //LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	return (RegisterClass(&wc));

}

HMENU hMenu, hPopMenu1, hPopMenu2, hPopMenu3;		// handle of menu 


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


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	int i;
	char msg[20];
	int retCode,Type,Vallen;
	HKEY hKey=0;
	char Size[80];

	hInst = hInstance; // Store instance handle in our global variable

	MinimizetoTray=GetMinimizetoTrayFlag();

	// Create a dialog box as the main window

	hWnd=CreateDialog(hInst,ClassName,0,NULL);
	
    if (!hWnd)
        return (FALSE);

	MainWnd=hWnd;

	// Retrieve the handlse to the edit controls. 

	hwndInput = GetDlgItem(hWnd, 118); 
	hwndOutput = GetDlgItem(hWnd, 117); 
	hwndMon = GetDlgItem(hWnd, 116); 
 
	// Set our own WndProcs for the controls. 

	wpOrigInputProc = (WNDPROC) SetWindowLong(hwndInput, GWL_WNDPROC, (LONG) InputProc); 
	wpOrigOutputProc = (WNDPROC)SetWindowLong(hwndOutput, GWL_WNDPROC, (LONG)OutputProc);
	wpOrigMonProc = (WNDPROC)SetWindowLong(hwndMon, GWL_WNDPROC, (LONG)MonProc);

	// Get saved config from Registry

	// Get config from Registry 

	wsprintf(Key,"SOFTWARE\\G8BPQ\\BPQ32\\BPQTerminal\\Session%d",Sessno);

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"ApplMask",0,			
			(ULONG *)&Type,(UCHAR *)&applmask,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MTX",0,			
			(ULONG *)&Type,(UCHAR *)&mtxparam,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MCOM",0,			
			(ULONG *)&Type,(UCHAR *)&mcomparam,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"PortMask",0,			
			(ULONG *)&Type,(UCHAR *)&portmask,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Bells",0,			
			(ULONG *)&Type,(UCHAR *)&Bells,(ULONG *)&Vallen);
	
		Vallen=8;
		retCode = RegQueryValueEx(hKey,"Split",0,			
			(ULONG *)&Type,(UCHAR *)&Split,(ULONG *)&Vallen);

		Vallen=80;
		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);
	
	}

	if (Rect.right == 0)
	{
		GetWindowRect(hWnd,	&Rect);
	}

	Height = Rect.bottom-Rect.top;
	Width = Rect.right-Rect.left;

	MoveWindow(hWnd,Rect.left,Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

	MoveWindows();

	Stream=FindFreeStream();
	
	if (Stream == 255)
	{
		MessageBox(NULL,"No free streams available",NULL,MB_OK);
		return (FALSE);
	}

	//
	//	Register message for posting by BPQDLL
	//

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	//
	//	Enable Async notification
	//
	
	BPQSetHandle(Stream, hWnd);

	GetVersionInfo(NULL);

	wsprintf(Title,"BPQTerminal Version %s - using stream %d", VersionString, Stream);

	SetWindowText(hWnd,Title);
		

//	Sets Application Flags and Mask for stream. (BPQHOST function 1)
//	AH = 1	Set application mask to value in DL (or even DX if 16
//		applications are ever to be supported).
//
//		Set application flag(s) to value in CL (or CX).
//		whether user gets connected/disconnected messages issued
//		by the node etc.
//
//		Top bit of flags controls monitoring

	applflags |= 0x80;

	SetAppl(Stream, applflags, applmask);

	SetTraceOptions(portmask,mtxparam,mcomparam);
	
	hMenu=GetMenu(hWnd);

//	if (applmask & 0x01)	CheckMenuItem(hMenu,BPQAPPL1,MF_CHECKED);
	if (applmask & 0x02)	CheckMenuItem(hMenu,BPQCHAT,MF_CHECKED);
//	if (applmask & 0x04)	CheckMenuItem(hMenu,BPQAPPL3,MF_CHECKED);
//	if (applmask & 0x08)	CheckMenuItem(hMenu,BPQAPPL4,MF_CHECKED);
//	if (applmask & 0x10)	CheckMenuItem(hMenu,BPQAPPL5,MF_CHECKED);
//	if (applmask & 0x20)	CheckMenuItem(hMenu,BPQAPPL6,MF_CHECKED);
//	if (applmask & 0x40)	CheckMenuItem(hMenu,BPQAPPL7,MF_CHECKED);
//	if (applmask & 0x80)	CheckMenuItem(hMenu,BPQAPPL8,MF_CHECKED);

// CMD_TO_APPL	EQU	1B		; PASS COMMAND TO APPLICATION
// MSG_TO_USER	EQU	10B		; SEND 'CONNECTED' TO USER
// MSG_TO_APPL	EQU	100B		; SEND 'CONECTED' TO APPL
//		0x40 = Send Keepalives

//	if (applflags & 0x01)	CheckMenuItem(hMenu,BPQFLAGS1,MF_CHECKED);
//	if (applflags & 0x02)	CheckMenuItem(hMenu,BPQFLAGS2,MF_CHECKED);
//	if (applflags & 0x04)	CheckMenuItem(hMenu,BPQFLAGS3,MF_CHECKED);
//	if (applflags & 0x40)	CheckMenuItem(hMenu,BPQFLAGS4,MF_CHECKED);


	hPopMenu1=GetSubMenu(hMenu,1);

	for (i=1;i <= GetNumberofPorts();i++)
	{
		wsprintf(msg,"Port %d",i);
		AppendMenu(hPopMenu1,
			MF_STRING | MF_CHECKED,BPQBASE + i,msg);
	}
	
	if (mtxparam & 1)
		CheckMenuItem(hMenu,BPQMTX,MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQMTX,MF_UNCHECKED);

	if (mcomparam & 1)
		CheckMenuItem(hMenu,BPQMCOM,MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQMCOM,MF_UNCHECKED);
	
	DrawMenuBar(hWnd);	

	DragCursor = LoadCursor(hInstance, "IDC_DragSize");

	GetCallsign(Stream, callsign);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON
	
		wsprintf(Title,"BPQTerminal Stream %d",Stream);
		AddTrayMenuItem(hWnd, Title);
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return (TRUE);
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent, xPos, yPos;
	LPRECT lprc;

	if (message == BPQMsg)
	{
		if (lParam & BPQDataAvail)
			DoReceivedData(hWnd);
				
		if (lParam & BPQMonitorAvail)
			DoMonData(hWnd);
				
		if (lParam & BPQStateChange)
			DoStateChange(hWnd);

		return (0);
	}
	
	switch (message) { 

		case WM_ACTIVATE:

			//fActive = LOWORD(wParam);           // activation flag 
			//fMinimized = (BOOL) HIWORD(wParam); // minimized flag 
			//	hwnd = (HWND) lParam;               // window handle 
 
			SetFocus(hwndInput);
			break;


		case WM_MOUSEMOVE:

			// Used to size split between Monitor and Output Windows

			if (wParam && MK_LBUTTON)
			{
				xPos = LOWORD(lParam);  // horizontal position of cursor 
				yPos = HIWORD(lParam);  // vertical position of cursor 

				if (LastY == 0)
	
					LastY=yPos;

				else
				{
					if (yPos > LastY)
						Split = Split + 0.01;
					if (yPos < LastY)
						Split = Split - 0.01;

					if (Split < .1) Split = .1;
					if (Split > .9) Split = .9;

					LastY=yPos;

					MoveWindows();
				}
				return 0;
			}
			else

				LastY=0;

			break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		//Parse the menu selections:
		
		if (wmId > BPQBASE && wmId < BPQBASE + 17)
		{
			TogglePort(hWnd,wmId,0x1 << (wmId - (BPQBASE + 1)));
			break;
		}
		
		switch (wmId) {
        
		case BPQMTX:
	
			ToggleMTX(hWnd);
			break;

		case BPQMCOM:

			ToggleMCOM(hWnd);
			break;

		case BPQCONNECT:
		
			SessionControl(Stream, 1, 0);
			break;
			        
		case BPQDISCONNECT:
			
			SessionControl(Stream, 2, 0);
			break;
			
		case BPQAUTOCONNECT:

			ToggleAutoConnect(hWnd);
			break;
			
		case BPQAPPL1:

			ToggleAppl(hWnd,BPQAPPL1,0x1);
			break;

		case BPQAPPL2:

			ToggleAppl(hWnd,BPQAPPL2,0x2);
			break;

		case BPQAPPL3:

			ToggleAppl(hWnd,BPQAPPL3,0x4);
			break;

		case BPQAPPL4:

			ToggleAppl(hWnd,BPQAPPL4,0x8);
			break;

		case BPQAPPL5:

			ToggleAppl(hWnd,BPQAPPL5,0x10);
			break;

		case BPQAPPL6:

			ToggleAppl(hWnd,BPQAPPL6,0x20);
			break;

		case BPQAPPL7:

			ToggleAppl(hWnd,BPQAPPL7,0x40);
			break;

		case BPQAPPL8:

			ToggleAppl(hWnd,BPQAPPL8,0x80);
			break;

		case BPQFLAGS1:

			ToggleFlags(hWnd,BPQFLAGS1,0x01);
			break;

		case BPQFLAGS2:

			ToggleFlags(hWnd,BPQFLAGS2,0x02);
			break;

		case BPQFLAGS3:

			ToggleFlags(hWnd,BPQFLAGS3,0x04);
			break;

		case BPQFLAGS4:

			ToggleFlags(hWnd,BPQFLAGS4,0x40);
			break;

		case BPQBELLS:

			ToggleBells(hWnd);
			break;

		case BPQCHAT:

			ToggleChat(hWnd);
			break;

		case BPQCLEARMON:

			SendMessage(hwndMon,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCLEAROUT:

			SendMessage(hwndOutput,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCOPYMON:

			CopyToClipboard(hwndMon);
			break;

		case BPQCOPYOUT:
		
			CopyToClipboard(hwndOutput);
			break;

		case BPQHELP:

			HtmlHelp(hWnd,"BPQTerminal.chm",HH_HELP_FINDER,0);  
			break;

		default:

			return 0;

		}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

		case  SC_MINIMIZE: 

			if (MinimizetoTray)
				return ShowWindow(hWnd, SW_HIDE);		
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_SIZING:

			lprc = (LPRECT) lParam;

			Height = lprc->bottom-lprc->top;
			Width = lprc->right-lprc->left;
			MoveWindows();
			
			return TRUE;


		case WM_DESTROY:
		
			// Remove the subclass from the edit control. 

			GetWindowRect(hWnd,	&Rect);	// For save soutine

            SetWindowLong(hwndInput, GWL_WNDPROC, 
                (LONG) wpOrigInputProc); 
         
			SessionControl(Stream, 2, 0);
			DeallocateStream(Stream);
			PostQuitMessage(0);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);
			
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}

 
LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	int index;

	if (uMsg == WM_CHAR) 
	{
		if (wParam == 13)
		{
			//
			//	if not connected, and autoconnect is
			//	enabled, connect now
			
			if (!CONNECTED && AUTOCONNECT)
				SessionControl(Stream, 1, 0);
			
			kbptr=SendMessage(hwndInput,WM_GETTEXT,159,(LPARAM) (LPCSTR) kbbuf);
	
			// Echo
			
			index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) &kbbuf[0] );		
			SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

			// Replace null with CR, and send to Node

			kbbuf[kbptr]=13;
			SendMsg(Stream, &kbbuf[0], kbptr+1);

			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
		if (wParam == 0x1a)  // Ctrl/Z
		{
			//
			//	if not connected, and autoconnect is
			//	enabled, connect now
			
			if (!CONNECTED && AUTOCONNECT)
				SessionControl(Stream, 1, 0);
	
			kbbuf[0]=0x1a;
			kbbuf[1]=13;

			SendMsg(Stream, &kbbuf[0], 2);

			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

        return 0; 
		}

	}
 
    return CallWindowProc(wpOrigInputProc, hwnd, uMsg, wParam, lParam); 
} 


LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Trap mouse messages, so we cant select stuff in output and mon windows,
	//	otherwise scrolling doesnt work.

	if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) 
        return TRUE; 
 
    return CallWindowProc(wpOrigOutputProc, hwnd, uMsg, wParam, lParam); 
} 

LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) 
        return TRUE; 
 
    return CallWindowProc(wpOrigMonProc, hwnd, uMsg, wParam, lParam); 
} 



DoStateChange(HWND hWnd)
{
	int port, sesstype, paclen, maxframe, l4window;

	//	Get current Session State. Any state changed is ACK'ed
	//	automatically. See BPQHOST functions 4 and 5.
	
	SessionState(Stream, &state, &change);
		
	if (change == 1)
	{
		if (state == 1)
		{
			// Connected
			
			CONNECTED=TRUE;

	//		GetCallsign(Stream, callsign);

			
			GetConnectionInfo(Stream, callsign,
										 &port, &sesstype, &paclen,
										 &maxframe, &l4window);

				
			wsprintf(Title,"BPQTerminal Version %s - using stream %d - Connected to %s",VersionString,Stream,callsign);
			SetWindowText(hWnd,Title);
			DisableConnectMenu(hWnd);
			EnableDisconnectMenu(hWnd);

		}
		else
		{
			CONNECTED=FALSE;
			wsprintf(Title,"BPQTerminal Version %s - using stream %d - Disconnected",VersionString,Stream);
			SetWindowText(hWnd,Title);
			DisableDisconnectMenu(hWnd);
			EnableConnectMenu(hWnd);
		}
	}

	return (0);

}
		
int PartLinePtr=0;

DoReceivedData(HWND hWnd)
{
	char * ptr1, * ptr2;
	int index;

	if (RXCount(Stream) > 0)
	{
		do {
		
			GetMsg(Stream, &readbuff[PartLinePtr],&len,&count);
		
			len=len+PartLinePtr;

			ptr1=&readbuff[0];

			if (Bells)
			{
				do {

					ptr2=memchr(ptr1,7,len);
					
					if (ptr2)
					{
						*(ptr2)=32;
						Beep(440,250);
					}
	
				} while (ptr2);

			}

		lineloop:

			if (len > 0)
			{
				//	copy text to control a line at a time	

					
				ptr2=memchr(ptr1,13,len);
				
				if (ptr2 == 0)
				{
					// no newline. Move data to start of buffer and Save pointer

					PartLinePtr=len;

					memmove(readbuff,ptr1,len);

					return (0);

				}
				else
				{
					*(ptr2++)=0;

					index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );
					
					PartLinePtr=0;

					len-=(ptr2-ptr1);

					ptr1=ptr2;

					if (index > 1200)
						
					do{

						index=SendMessage(hwndOutput,LB_DELETESTRING, 0, 0);
					
						} while (index > 1000);

					SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

					goto lineloop;
				}
			}

		} while (count > 0);
	}
	return (0);
}

DoMonData(HWND hWnd)
{
	char * ptr1, * ptr2;
	int index, stamp;
	int len;
	char buffer[1024], monbuff[512];

	if (MONCount(Stream) > 0)
	{
		do {
		
			stamp=GetRaw(Stream, monbuff,&len,&count);
			len=DecodeFrame(monbuff,buffer,stamp);
	
			ptr1=&buffer[0];

		lineloop:

			if (len > 0)
			{
				//	copy text to control a line at a time	

				ptr2=memchr(ptr1,13,len);
				
				if (ptr2 == 0)
				{
					// no newline. Move data to start of buffer and Save pointer

					memmove(buffer,ptr1,len);

					return (0);

				}
				else
				{
					*(ptr2++)=0;

					index=SendMessage(hwndMon,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );	

					if (index > 1200)

					do{

						index=SendMessage(hwndMon,LB_DELETESTRING, 0, 0);
					
						} while (index > 1000);

					SendMessage(hwndMon,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

					len-=(ptr2-ptr1);

					ptr1=ptr2;
					
					goto lineloop;
				}
			}
			
		} while (count > 0);
	}
	return (0);
}





int DisableConnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQCONNECT,MF_GRAYED);

	return (0);
}	
int DisableDisconnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQDISCONNECT,MF_GRAYED);
	return (0);
}	

int	EnableConnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQCONNECT,MF_ENABLED);
	return (0);
}	

int EnableDisconnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQDISCONNECT,MF_ENABLED);

    return (0);
}

int ToggleAutoConnect(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	
	AUTOCONNECT = !AUTOCONNECT;
	
	if (AUTOCONNECT)

		CheckMenuItem(hMenu,BPQAUTOCONNECT,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQAUTOCONNECT,MF_UNCHECKED);

    return (0);
  
}

int ToggleAppl(HWND hWnd, int Item, int mask)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	
	applmask = applmask ^ mask;
	
	if (applmask & mask)

		CheckMenuItem(hMenu,Item,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

	SetAppl(Stream,applflags,applmask);

    return (0);
  
}

int ToggleFlags(HWND hWnd, int Item, int mask)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	
	applflags = applflags ^ mask;
	
	if (applflags & mask)

		CheckMenuItem(hMenu,Item,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

	SetAppl(Stream,applflags,applmask);

    return (0);
  
}

CopyScreentoBuffer(char * buff)
{

	return (0);
}
	
int TogglePort(HWND hWnd, int Item, int mask)
{
	portmask = portmask ^ mask;
	
	if (portmask & mask)

		CheckMenuItem(hMenu,Item,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

	SetTraceOptions(portmask,mtxparam,mcomparam);

    return (0);
  
}
int ToggleMTX(HWND hWnd)
{
	mtxparam = mtxparam ^ 1;
	
	if (mtxparam & 1)

		CheckMenuItem(hMenu,BPQMTX,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQMTX,MF_UNCHECKED);

	SetTraceOptions(portmask,mtxparam,mcomparam);

    return (0);
  
}
int ToggleMCOM(HWND hWnd)
{
	mcomparam = mcomparam ^ 1;
	
	if (mcomparam & 1)

		CheckMenuItem(hMenu,BPQMCOM,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQMCOM,MF_UNCHECKED);

	SetTraceOptions(portmask,mtxparam,mcomparam);

    return (0);
  
}
int ToggleBells(HWND hWnd)
{
	Bells = Bells ^ 1;
	
	if (Bells & 1)

		CheckMenuItem(hMenu,BPQBELLS,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQBELLS,MF_UNCHECKED);

    return (0);
  
}

int ToggleChat(HWND hWnd)
{	
	applmask = applmask ^ 02;
	
	if (applmask & 02)

		CheckMenuItem(hMenu,BPQCHAT,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQCHAT,MF_UNCHECKED);

	SetAppl(Stream,applflags,applmask);

    return (0);
  
}
void MoveWindows()
{
	MoveWindow(hwndMon,3, 3, Width-13, Height*(Split)-45, TRUE);
	MoveWindow(hwndOutput,3, Height*Split-35, Width-13, Height*(1-Split)-45, TRUE);
	MoveWindow(hwndInput,3, Height-80, Width-13, 25, TRUE);
}

void CopyToClipboard(HWND hWnd)
{
	int i,n, len=0;
	HGLOBAL	hMem;
	char * ptr;
	//
	//	Copy List Box to clipboard
	//
	
	n = SendMessage(hWnd, LB_GETCOUNT, 0, 0);		
	
	for (i=0; i<n; i++)
	{
		len+=SendMessage(hWnd, LB_GETTEXTLEN, i, 0);
	}

	hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len+n+n+1);
	

	if (hMem != 0)
	{
		ptr=GlobalLock(hMem);
	
		if (OpenClipboard(MainWnd))
		{
			//			CopyScreentoBuffer(GlobalLock(hMem));
			
			for (i=0; i<n; i++)
			{
				ptr+=SendMessage(hWnd, LB_GETTEXT, i, (LPARAM) ptr);
				*(ptr++)=13;
				*(ptr++)=10;
			}

			*(ptr)=0;					// end of data

			GlobalUnlock(hMem);
			EmptyClipboard();
			SetClipboardData(CF_TEXT,hMem);
			CloseClipboard();
		}
			else
				GlobalFree(hMem);		
	}
}

