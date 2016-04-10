

#define _CRT_SECURE_NO_DEPRECATE 

#include <windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>

#include "..\include\bpq32.h"
#include "BPQAPRSDigi.h"

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

#define BPQICON 100

HINSTANCE hInst; 
char szAppName[] = "BPQAPRSDigi";
char szTitle[80]   = "BPQAPRSRelay" ; // The title bar text


BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int NewLine(HWND hWnd);
int	ProcessBuff(HWND hWnd, UCHAR * readbuff,int len,int stamp);
int TogglePort(HWND hWnd, int Item, int mask);
int CopyScreentoBuffer(char * buff);
VOID SendFrame(UCHAR * buff, int txlen);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int	KissDecode(UCHAR * inbuff, int len);
VOID ResolveThread();

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

SOCKADDR_IN destaddr;

unsigned int ipaddr;

unsigned short port = 10091;
UCHAR BPQPort = 3;

char Host[] = "gm8bpq.no-ip.com";

char Screen[22000];
char readbuff[512];
int baseline=216;
int Stream;

extern short CRCTAB;

LOGFONT LFTTYFONT ;

HFONT hFont ;

SOCKET udpsock;

BOOL MinimizetoTray=FALSE;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[255];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}

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
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = szAppName;

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

int len,count,i;
char msg[20];

HMENU hMenu,hPopMenu1,hPopMenu2,hPopMenu3;		// handle of menu 
	
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	BOOL bcopt=TRUE;
	u_long param=1;
	WSADATA WsaData;            // receives data from WSAStartup

	hInst = hInstance; // Store instance handle in our global variable

	MinimizetoTray=GetMinimizetoTrayFlag();

	hWnd = CreateWindow(szAppName, szTitle,
		WS_OVERLAPPEDWINDOW | WS_VSCROLL,
		CW_USEDEFAULT, 0, 500, 25*16,
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
	

	SetTimer(hWnd,1,1000,NULL);

	memset(Screen, ' ', 20000); 

	for (Stream = 63; Stream > 0; Stream--)
	{
		if (AllocateStream(Stream) == 0)
			break;
	}

	if (Stream == 0)
	{
		MessageBox(NULL,"Couldn'tr find a free stream","BPQMON",MB_OK);
		return (FALSE);
	}

	//
	//	Register message for posting by BPQDLL
	//

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	BPQSetHandle(Stream, hWnd);
	
	SetWindowText(hWnd,szTitle);
		

//	Sets Application Flags and Mask for stream. (BPQHOST function 1)
//	AH = 1	Set application mask to value in DL (or even DX if 16
//		applications are ever to be supported).
//
//		Set application flag(s) to value in CL (or CX).
//		whether user gets connected/disconnected messages issued
//		by the node etc.
//
//		Top bit of mask controlls monitoring

	SetAppl(Stream,0x80,0);
/*
;	hMenu=GetMenu(hWnd);
	hMenu=CreateMenu();
	hPopMenu1=CreatePopupMenu();
	hPopMenu2=CreatePopupMenu();
	hPopMenu3=CreatePopupMenu();
	SetMenu(hWnd,hMenu);


	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu1,"Ports");
	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu2,"Flags");
	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu3,"Edit");

	for (i=1;i <= GetNumberofPorts();i++)
	{
		wsprintf(msg,"Port %d",i);
		AppendMenu(hPopMenu1,
			MF_STRING | MF_CHECKED,BPQBASE + i,msg);
	}

	
	AppendMenu(hPopMenu3,MF_STRING,BPQCOPY,"Copy");

	DrawMenuBar(hWnd);	
*/
	SetScrollRange(hWnd,SB_VERT,0,216,TRUE);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON

		AddTrayMenuItem(hWnd, "BPQAPRSRelay");
	}
	
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	WSAStartup(MAKEWORD(2, 0), &WsaData);

	udpsock = socket(AF_INET,SOCK_DGRAM,0);

	if (udpsock == INVALID_SOCKET)
	{
		int err;
		MessageBox(NULL, (LPSTR) "Failed to create UDP socket",NULL,MB_OK);
		err = WSAGetLastError();
  	 	return FALSE; 
	}

	ioctlsocket (udpsock,FIONBIO,&param);
 
	setsockopt (udpsock,SOL_SOCKET,SO_BROADCAST,(const char FAR *)&bcopt,4);
/*
	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = htons(udpport[i]);

	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(0);

	if (bind(udpsock[i], (LPSOCKADDR) &sinx, sizeof(sinx)) != 0 )
	{
		//
		//	Bind Failed
		//
		err = WSAGetLastError();
		wsprintf(Msg, "Bind Failed for UDP socket - error code = %d", err);
		MessageBox(NULL,Msg,NULL, MB_OK);
		return;
	}
*/
	
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(0);

	_beginthread(ResolveThread,0,0);

	return (TRUE);
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  MESSAGES:
//
//	WM_COMMAND - process the application menu
//	WM_PAINT - Paint the main window
//	WM_DESTROY - post a quit message and return
//    WM_DISPLAYCHANGE - message sent to Plug & Play systems when the display changes
//    WM_RBUTTONDOWN - Right mouse click -- put up context menu here if appropriate
//    WM_NCRBUTTONUP - User has clicked the right button on the application's system menu
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
    HFONT    hOldFont ;
	HGLOBAL	hMem;


	int i,stamp;
	int nScrollCode,nPos;

	if (message == BPQMsg)
	{
		if (MONCount(Stream) > 0)
		{
			do {
		
			stamp=GetRaw(Stream, readbuff,&len,&count);
			
			if (len > 0)
			{
				ProcessBuff(hWnd,readbuff,len,stamp);
				if (count == 0)
					InvalidateRect(hWnd,NULL,FALSE);
			}

			} while (count > 0);

		}
	}

	switch (message) { 

		case WM_VSCROLL:
		
			nScrollCode = (int) LOWORD(wParam); // scroll bar value 
			nPos = (short int) HIWORD(wParam);  // scroll box position 

			//hwndScrollBar = (HWND) lParam;      // handle of scroll bar 

			if (nScrollCode == 0)
			{
				baseline--;
				if (baseline <0)
					baseline=0;
			}
			if (nScrollCode == 1)
			{

				baseline++;
				if (baseline > 216)
					baseline = 216;
			}

			SetScrollPos(hWnd,SB_VERT,baseline,TRUE);

			InvalidateRect(hWnd,NULL,FALSE);
			break;


		case WM_PAINT:

			hdc = BeginPaint (hWnd, &ps);
			
			hOldFont = SelectObject( hdc, hFont) ;
			
	
			for (i=0; i<25; i++)
			{
				TextOut(hdc,0,i*14,&Screen[(baseline+i)*80],80);
			}
				
			SelectObject( hdc, hOldFont ) ;
			EndPaint (hWnd, &ps);
			break; 
			
				
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




		case WM_DESTROY:
			
			DeallocateStream(Stream);
			DestroyMenu(hMenu);

			PostQuitMessage(0);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			

			
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
        	
		case BPQCOPY:
		
			//
			//	Copy buffer to clipboard
			//
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,sizeof(Screen));
		
			if (hMem != 0)
			{
				if (OpenClipboard(hWnd))
				{
					CopyScreentoBuffer(GlobalLock(hMem));
					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT,hMem);
					CloseClipboard();
				}
				else
				{
					GlobalFree(hMem);
				}

			}

			break;

		}

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}


unsigned short int compute_crc(unsigned char *buf,int len)
{
	int fcs;

	_asm{

	mov	esi,buf
	mov	ecx,len
	mov	edx,-1		; initial value

crcloop:

	lodsb

	XOR	DL,AL		; OLD FCS .XOR. CHAR
	MOVZX EBX,DL		; CALC INDEX INTO TABLE FROM BOTTOM 8 BITS
	ADD	EBX,EBX
	MOV	DL,DH		; SHIFT DOWN 8 BITS
	XOR	DH,DH		; AND CLEAR TOP BITS
	XOR	DX,CRCTAB[EBX]	; XOR WITH TABLE ENTRY
	
	loop	crcloop

	mov	fcs,EDX

	}	

	return (fcs);

  }


int line=240;
int col=0;

int	ProcessBuff(HWND hWnd, UCHAR * buff,int buflen, int timestamp)
{
	unsigned short int crc;
	int txlen=(buff[6]<<8) + buff[5] - 5;			// Len includes buffer header (7) but we add crc

	buff[4] &=  127	;			// Maks TX bit
								
	if (buff[4] != BPQPort)
		return 0;									// Only Port 1

	crc=compute_crc(&buff[7], txlen - 2);
	crc ^= 0xffff;

	buff[txlen+5]=(crc&0xff);
	buff[txlen+6]=(crc>>8);

	// Pass to g8bpq.org.uk port 10091
	
	destaddr.sin_port = htons(port);

	sendto(udpsock, &buff[7], txlen, 0,(LPSOCKADDR)&destaddr, sizeof(destaddr));		

	return (0);
}
int xNewLine()
{
	while (col <80)
		Screen[line*80+col++] = ' ';
	
	col=0;
	line++;
	if (line >= 250)
	{
		line=0;
	}

	baseline=line-25;

	if (baseline<0)
		baseline=baseline+249;

	return (0);
}

int NewLine(HWND hWnd)
{

	col=0;
	line++;
	if (line > 240)
	{
		memmove(Screen,Screen+80,19200);
		memset(Screen+19200,' ',80);
		line=240;
		baseline=216;
	}

		SetScrollPos(hWnd,SB_VERT,baseline,TRUE);

	return (0);
}

int portmask=0xffff;
int mtxparam=1;
int mcomparam=1;

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

int CopyScreentoBuffer(char * buff)
{
	int i,j;
	char line [82];
	
	for (i=0;i != 241; i++)
	{
		memcpy(line,&Screen[i*80],80);
		
		//
		//	scan line backwards, and replace last space with crlf
		//

		for (j=79;j != -1; j--)
		{
			if (line[j] == ' ')
				continue;

			if (j == -1)
				break;			// Ignore blank lines

			j++;
			line[j++] = '\r';
			line[j++] = '\n';
		
			memcpy(buff,line,j);
			buff+=j;

			break;
		}
	}

	return (0);
}

VOID ResolveThread()
{
	struct hostent * HostEnt;
	int err;

	while (TRUE)
	{

	// Resolve Name if needed

	HostEnt = gethostbyname(Host);
		 
	if (!HostEnt)
	{
		err = WSAGetLastError();

		Debugprintf("Resolve Failed for %s %d %x", Host, err, err);
	}
	else
	{
		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
	}


	Sleep(60 * 15 * 1000);
	}

	return;
}

