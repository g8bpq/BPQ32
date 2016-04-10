// Mail and Chat Server for BPQ32 Packet Switch
//
//	Console Window Module

#include "stdafx.h"

char ClassName[]="CONSOLEWINDOW";

char SYSOPCall[50];

struct UserInfo * user;


struct ConsoleInfo CINFO;

struct ConsoleInfo * ConsHeader = &CINFO;


#define InputBoxHeight 25
static LRESULT CALLBACK ConsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY SplitProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static void MoveWindows();


#define BGCOLOUR RGB(236,233,216)

BOOL CreateConsole(int Stream)
{
    WNDCLASS  wc;
	HBRUSH bgBrush;
	HMENU hMenu;

	struct ConsoleInfo * Cinfo = &CINFO;


	if (Cinfo->hConsole)
	{
		ShowWindow(Cinfo->hConsole, SW_SHOWNORMAL);
		SetForegroundWindow(Cinfo->hConsole);
		return FALSE;							// Alreaqy open
	}

	Cinfo->BPQStream = Stream;

/*
		Vallen=80;
		RegQueryValueEx(hKey,"ConsoleSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"Bells",0,			
			(ULONG *)&Type,(UCHAR *)&Bells,(ULONG *)&Vallen);
	
		Vallen=4;
		RegQueryValueEx(hKey,"StripLF",0,			
			(ULONG *)&Type,(UCHAR *)&StripLF,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"CloseWindowOnBye",0,			
			(ULONG *)&Type,(UCHAR *)&CloseWindowOnBye,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"WarnWrap",0,			
			(ULONG *)&Type,(UCHAR *)&WarnWrap,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"WrapInput",0,			
			(ULONG *)&Type,(UCHAR *)&WrapInput,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"FlashOnConnect",0,			
			(ULONG *)&Type,(UCHAR *)&FlashOnConnect,(ULONG *)&Vallen);

*/

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = ConsWndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
    wc.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(IDI_BPQMailChat) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	Cinfo->hConsole=CreateDialog(hInst,ClassName,0,NULL);
	
    if (!Cinfo->hConsole)
        return (FALSE);

	hMenu=GetMenu(Cinfo->hConsole);
	Cinfo->hMenu = hMenu;


	CheckMenuItem(hMenu,BPQBELLS, (Cinfo->Bells) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,BPQStripLF, (Cinfo->StripLF) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_WARNINPUT, (Cinfo->WarnWrap) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_WRAPTEXT, (Cinfo->WrapInput) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_Flash, (Cinfo->FlashOnConnect) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_CLOSEWINDOW, (Cinfo->CloseWindowOnBye) ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(hWnd);	

	// Retrieve the handlse to the edit controls. 

	Cinfo->hwndInput = GetDlgItem(Cinfo->hConsole, 118); 
	Cinfo->hwndOutput = GetDlgItem(Cinfo->hConsole, 117); 
 
	// Set our own WndProcs for the controls. 

	Cinfo->wpOrigInputProc = (WNDPROC) SetWindowLong(Cinfo->hwndInput, GWL_WNDPROC, (LONG) InputProc); 
	Cinfo->wpOrigOutputProc = (WNDPROC)SetWindowLong(Cinfo->hwndOutput, GWL_WNDPROC, (LONG)OutputProc);

	if (cfgMinToTray)
		AddTrayMenuItem(Cinfo->hConsole, "Mail/Chat Console");

	ShowWindow(Cinfo->hConsole, SW_SHOWNORMAL);

	if (Cinfo->ConsoleRect.right < 100 || Cinfo->ConsoleRect.bottom < 100)
	{
		GetWindowRect(Cinfo->hConsole,	&Cinfo->ConsoleRect);
	}

	MoveWindow(Cinfo->hConsole,Cinfo->ConsoleRect.left,Cinfo->ConsoleRect.top, Cinfo->ConsoleRect.right-Cinfo->ConsoleRect.left, Cinfo->ConsoleRect.bottom-Cinfo->ConsoleRect.top, TRUE);

	MoveWindows(Cinfo);

	Cinfo->Console = zalloc(sizeof(CIRCUIT));

	Cinfo->Console->Active = TRUE;
	Cinfo->Console->BPQStream = Stream;

	strcpy(Cinfo->Console->Callsign, SYSOPCall);

	user = LookupCall(SYSOPCall);

	if (user == NULL)
	{
		user = AllocateUserRecord(SYSOPCall);

		if (user == NULL) return 0; //		Cant happen??
	}

	time(&user->TimeLastCOnnected);

	Cinfo->Console->UserPointer = user;
	Cinfo->Console->lastmsg = user->lastmsg;
	Cinfo->Console->paclen=236;
	Cinfo->Console->sysop = TRUE;

	Cinfo->Console->PageLen = user->PageLen;
	Cinfo->Console->Paging = (user->PageLen > 0);

	nodeprintf(Cinfo->Console, BBSSID, Ver[0], Ver[1], Ver[2], Ver[3],
		ALLOWCOMPRESSED ? "B" : "", "", "", "F");

	if (user->Name[0] == 0)
	{
		Cinfo->Console->Flags |= GETTINGUSER;
		SendUnbuffered(-1, NewUserPrompt, strlen(NewUserPrompt));
	}
	else
		SendWelcomeMsg(-1, Cinfo->Console, user);

	return TRUE;

}
VOID CloseConsoleSupport(struct ConsoleInfo * Cinfo);

VOID CloseConsole(int Stream)
{
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->Console)
		{
			if (Cinfo->BPQStream == Stream)
			{
				CloseConsoleSupport(Cinfo);
				return;
			}
		}
	}
}



VOID CloseConsoleSupport(struct ConsoleInfo * Cinfo)
{

if (Cinfo->Console->Flags & CHATMODE)
	{
		__try
		{
			logout(Cinfo->Console);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
		}
	
		Cinfo->Console->Flags = 0;
	}


	if (Cinfo->CloseWindowOnBye)
	{
//		PostMessage(hConsole, WM_DESTROY, 0, 0);
		DestroyWindow(Cinfo->hConsole);
	}
/*	
sscanf(Size,"%d,%d,%d,%d",&ConsoleRect.left,&ConsoleRect.right,&ConsoleRect.top,&ConsoleRect.bottom);


		retCode = RegSetValueEx(hKey,"Bells",0,REG_DWORD,(BYTE *)&Bells,4);
		retCode = RegSetValueEx(hKey,"StripLF",0,REG_DWORD,(BYTE *)&StripLF,4);
		retCode = RegSetValueEx(hKey,"WarnWrap",0,REG_DWORD,(BYTE *)&WarnWrap,4);
		retCode = RegSetValueEx(hKey,"WrapInput",0,REG_DWORD,(BYTE *)&WrapInput,4);
		retCode = RegSetValueEx(hKey,"FlashOnConnect",0,REG_DWORD,(BYTE *)&FlashOnConnect,4);
		retCode = RegSetValueEx(hKey,"CloseWindowOnBye",0,REG_DWORD,(BYTE *)&CloseWindowOnBye,4);
*/
}

void MoveWindows(struct ConsoleInfo * Cinfo)
{
	RECT rcClient;
	int ClientHeight, ClientWidth;


	GetClientRect(Cinfo->hConsole, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	MoveWindow(Cinfo->hwndOutput,2, 2, ClientWidth-4, ClientHeight-InputBoxHeight-4, TRUE);
	MoveWindow(Cinfo->hwndInput,2, ClientHeight-InputBoxHeight-2, ClientWidth-4, InputBoxHeight, TRUE);

	GetClientRect(Cinfo->hwndOutput, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;
	
	Cinfo->WarnLen = ClientWidth/8 - 1;
	Cinfo->WrapLen = Cinfo->WarnLen;
	Cinfo->maxlinelen = Cinfo->WarnLen;

}


LRESULT CALLBACK ConsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPRECT lprc;
	int i;
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->hConsole == hWnd)
			break;
	}

	if (Cinfo == NULL)
		 Cinfo = &CINFO;
	
	switch (message) { 

	case WM_ACTIVATE:

		SetFocus(Cinfo->hwndInput);
		break;


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) {

		case BPQBELLS:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->Bells, BPQBELLS);
			break;

		case BPQStripLF:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->StripLF, BPQStripLF);
			break;

		case IDM_WARNINPUT:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->WarnWrap, IDM_WARNINPUT);
			break;


		case IDM_WRAPTEXT:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->WrapInput, IDM_WRAPTEXT);
			break;

		case IDM_Flash:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->FlashOnConnect, IDM_WRAPTEXT);
			break;

		case IDM_CLOSEWINDOW:

			ToggleParam(Cinfo->hMenu, hWnd, &Cinfo->CloseWindowOnBye, IDM_CLOSEWINDOW);
			break;

		case BPQCLEAROUT:

			SendMessage(Cinfo->hwndOutput,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCOPYOUT:
		
			CopyToClipboard(Cinfo->hwndOutput);
			break;



		//case BPQHELP:

		//	HtmlHelp(hWnd,"BPQTerminal.chm",HH_HELP_FINDER,0);  
		//	break;

		default:

			return 0;

		}

	case WM_SYSCOMMAND:


		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

		case  SC_MINIMIZE: 

			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);		
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}


	case WM_SIZING:

		lprc = (LPRECT) lParam;

		Cinfo->Height = lprc->bottom-lprc->top;
		Cinfo->Width = lprc->right-lprc->left;

		MoveWindows(Cinfo);
			
		return TRUE;


	case WM_DESTROY:
		
		// Remove the subclass from the edit control. 

			GetWindowRect(hWnd,	&Cinfo->ConsoleRect);	// For save soutine

            SetWindowLong(Cinfo->hwndInput, GWL_WNDPROC, 
                (LONG) Cinfo->wpOrigInputProc); 
         

			if (cfgMinToTray) 
				DeleteTrayMenuItem(hWnd);


			if (Cinfo->Console && Cinfo->Console->Active)
			{
				ClearQueue(Cinfo->Console);
		
				Cinfo->Console->Active = FALSE;
				RefreshMainWindow();

				if (Cinfo->Console->Flags & CHATMODE)
				{
					logout(Cinfo->Console);
				}
				else
				{
					SendUnbuffered(Cinfo->Console->BPQStream, SignoffMsg, strlen(SignoffMsg));
					user->lastmsg = Cinfo->Console->lastmsg;
				}
			}

			// Free Scrollback

			for (i = 0; i < MAXSTACK ; i++)
			{
				if (Cinfo->KbdStack[i])
				{
					free(Cinfo->KbdStack[i]);
					Cinfo->KbdStack[i] = NULL;
				}
			}

			Sleep(500);

			free(Cinfo->Console);
			Cinfo->Console = 0;
			Cinfo->hConsole = NULL;
			
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}


LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	int i;
	unsigned int TextLen;
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->hwndInput == hWnd)
			 Cinfo = &CINFO;
	}

	if (Cinfo == NULL)
		return TRUE;

 
	if (uMsg == WM_KEYUP)
	{
		unsigned int i;
//		Debugprintf("5%x", LOBYTE(HIWORD(lParam)));

		if (LOBYTE(HIWORD(lParam)) == 0x48)
		{
			// Scroll up

			if (Cinfo->KbdStack[Cinfo->StackIndex] == NULL)
				return TRUE;

			SendMessage(Cinfo->hwndInput, WM_SETTEXT,0,(LPARAM)(LPCSTR) Cinfo->KbdStack[Cinfo->StackIndex]);
			
			for (i = 0; i < strlen(Cinfo->KbdStack[Cinfo->StackIndex]); i++)
			{
				SendMessage(Cinfo->hwndInput, WM_KEYDOWN, VK_RIGHT, 0);
				SendMessage(Cinfo->hwndInput, WM_KEYUP, VK_RIGHT, 0);
			}

			Cinfo->StackIndex++;
			if (Cinfo->StackIndex == 20)
				Cinfo->StackIndex = 19;

			return TRUE;
		}

		if (LOBYTE(HIWORD(lParam)) == 0x50)
		{
			// Scroll up

			Cinfo->StackIndex--;
			if (Cinfo->StackIndex < 0)
				Cinfo->StackIndex = 0;

			if (Cinfo->KbdStack[Cinfo->StackIndex] == NULL)
				return TRUE;
			
			SendMessage(Cinfo->hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) Cinfo->KbdStack[Cinfo->StackIndex]);

			for (i = 0; i < strlen(Cinfo->KbdStack[Cinfo->StackIndex]); i++)
			{
				SendMessage(Cinfo->hwndInput, WM_KEYDOWN, VK_RIGHT, 0);
				SendMessage(Cinfo->hwndInput, WM_KEYUP, VK_RIGHT, 0);
			}
			
			return TRUE;
		}
	}

	if (uMsg == WM_CHAR) 
	{
		if(Cinfo->WarnWrap || Cinfo->WrapInput)
		{
			TextLen = SendMessage(Cinfo->hwndInput,WM_GETTEXTLENGTH, 0, 0);

			if (Cinfo->WarnWrap)
				if (TextLen == Cinfo->WarnLen) Beep(220, 150);
			
			if (Cinfo->WrapInput)
				if ((wParam == 0x20) && (TextLen > Cinfo->WrapLen))
					wParam = 13;		// Replace space with Enter

		}

		if (wParam == 13)
		{
			Cinfo->kbptr=SendMessage(Cinfo->hwndInput,WM_GETTEXT,159,(LPARAM) (LPCSTR) Cinfo->kbbuf);

			Cinfo->StackIndex = 0;

			// Stack it

			if (Cinfo->KbdStack[19])
				free(Cinfo->KbdStack[19]);

			for (i = 18; i >= 0; i--)
			{
				Cinfo->KbdStack[i+1] = Cinfo->KbdStack[i];
			}

			Cinfo->KbdStack[0] = _strdup(Cinfo->kbbuf);

			Cinfo->kbbuf[Cinfo->kbptr]=13;

			// Echo

			WritetoConsoleWindow(Cinfo->BPQStream, Cinfo->kbbuf, Cinfo->kbptr+1);

			ProcessLine(Cinfo->Console, user, &Cinfo->kbbuf[0], Cinfo->kbptr+1);

			SendMessage(Cinfo->hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
		if (wParam == 0x1a)  // Ctrl/Z
		{
	
			Cinfo->kbbuf[0]=0x1a;
			Cinfo->kbbuf[1]=13;

			ProcessLine(Cinfo->Console, user, &Cinfo->kbbuf[0], 2);


			SendMessage(Cinfo->hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
 
	}

    return CallWindowProc(Cinfo->wpOrigInputProc, hwnd, uMsg, wParam, lParam); 
} 

LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->hwndOutput == hWnd)
			break;
	}

	if (Cinfo == NULL)
		 Cinfo = &CINFO;

	
	// Trap mouse messages, so we can't select stuff in output and mon windows,
	//	otherwise scrolling doesnt work.

	if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) 
        return TRUE; 

	return CallWindowProc(Cinfo->wpOrigOutputProc, hwnd, uMsg, wParam, lParam); 
} 

int WritetoConsoleWindowSupport(struct ConsoleInfo * Cinfo, char * Msg, int len);

int WritetoConsoleWindow(int Stream, char * Msg, int len)
{
	struct ConsoleInfo * Cinfo;

	for (Cinfo = ConsHeader; Cinfo; Cinfo = Cinfo->next)
	{
		if (Cinfo->Console)
		{
			if (Cinfo->BPQStream == Stream)
			{
				WritetoConsoleWindowSupport(Cinfo, Msg, len);
				return 0;
			}
		}
	}
	return 0;
}

int WritetoConsoleWindowSupport(struct ConsoleInfo * Cinfo, char * Msg, int len)
{
	char * ptr1, * ptr2;
	int index;

	if (len+Cinfo->PartLinePtr > MAXLINE)
		Cinfo->PartLinePtr = 0;

	if (len > MAXLINE)
		len = MAXLINE;

	if (Cinfo->PartLinePtr != 0)
		SendMessage(Cinfo->hwndOutput,LB_DELETESTRING,Cinfo->PartLineIndex,(LPARAM)(LPCTSTR) 0 );		

	memcpy(&Cinfo->readbuff[Cinfo->PartLinePtr], Msg, len);
		
	len=len+Cinfo->PartLinePtr;

	ptr1=&Cinfo->readbuff[0];
	Cinfo->readbuff[len]=0;

	if (Cinfo->Bells)
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

	if (Cinfo->PartLinePtr > MAXLINE)
		Cinfo->PartLinePtr = 0;

	if (len > 0)
	{
		//	copy text to control a line at a time	
					
		ptr2=memchr(ptr1,13,len);

		if (ptr2 == 0)
		{
			// no newline. Move data to start of buffer and Save pointer

			Cinfo->PartLineIndex=SendMessage(Cinfo->hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );
			SendMessage(Cinfo->hwndOutput,LB_SETCARETINDEX,(WPARAM) Cinfo->PartLineIndex, MAKELPARAM(FALSE, 0));

			Cinfo->PartLinePtr=len;
			memmove(Cinfo->readbuff,ptr1,len);

			return (0);

		}

		*(ptr2++)=0;

		// If len is greater that screen with, fold

		if ((ptr2 - ptr1) > Cinfo->maxlinelen)
		{
			char * ptr3;
			char * saveptr1 = ptr1;
			int linelen = ptr2 - ptr1;
			int foldlen;
			char save;
					
		foldloop:

			ptr3 = ptr1 + Cinfo->maxlinelen;
					
			while(*ptr3!= 0x20 && ptr3 > ptr1)
			{
				ptr3--;
			}
						
			foldlen = ptr3 - ptr1 ;

			if (foldlen == 0)
			{
				// No space before, so split at width

				foldlen = Cinfo->maxlinelen;
				ptr3 = ptr1 + Cinfo->maxlinelen;

			}
			else
			{
				ptr3++ ; // Omit space
				linelen--;
			}
			save = ptr1[foldlen];
			ptr1[foldlen] = 0;
			index=SendMessage(Cinfo->hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );					
			ptr1[foldlen] = save;
			linelen -= foldlen;
			ptr1 = ptr3;

			if (linelen > Cinfo->maxlinelen)
				goto foldloop;
						
			if (linelen > 0)
				index=SendMessage(Cinfo->hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );

			ptr1 = saveptr1;
		}
		else
		{
			index=SendMessage(Cinfo->hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );					
		}

		Cinfo->PartLinePtr=0;

		len-=(ptr2-ptr1);

		ptr1=ptr2;

		if ((len > 0) && Cinfo->StripLF)
		{
			if (*ptr1 == 0x0a)					// Line Feed
			{
				ptr1++;
				len--;
			}
		}

		if (index > 1200)
						
		do{

			index=SendMessage(Cinfo->hwndOutput,LB_DELETESTRING, 0, 0);
			
			} while (index > 1000);

		SendMessage(Cinfo->hwndOutput,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

		goto lineloop;
	}


	return (0);
}

int ToggleParam(HMENU hMenu, HWND hWnd, BOOL * Param, int Item)
{
	*Param = !(*Param);

	CheckMenuItem(hMenu,Item, (*Param) ? MF_CHECKED : MF_UNCHECKED);
	
    return (0);
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
/*
#define XBITMAP 80 
#define YBITMAP 20 
 
#define BUFFER MAX_PATH 
 
HBITMAP hbmpPencil, hbmpCrayon, hbmpMarker, hbmpPen, hbmpFork; 
HBITMAP hbmpPicture, hbmpOld; 
 
void AddItem(HWND hwnd, LPSTR lpstr, HBITMAP hbmp) 
{ 
    int nItem; 
 
    nItem = SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)lpstr); 
    SendMessage(hwnd, LB_SETITEMDATA, (WPARAM)nItem, (LPARAM)hbmp); 
} 
 
DWORD APIENTRY DlgDrawProc( 
        HWND hDlg,            // window handle to dialog box 
        UINT message,         // type of message 
        UINT wParam,          // message-specific information 
        LONG lParam) 
{ 
    int nItem; 
    TCHAR tchBuffer[BUFFER]; 
    HBITMAP hbmp; 
    HWND hListBox; 
    TEXTMETRIC tm; 
    int y; 
    HDC hdcMem; 
    LPMEASUREITEMSTRUCT lpmis; 
    LPDRAWITEMSTRUCT lpdis; 
    RECT rcBitmap;
	HRESULT hr; 
	size_t * pcch;
 
    switch (message) 
    { 
 
        case WM_INITDIALOG: 
 
            // Load bitmaps. 
 
            hbmpPencil = LoadBitmap(hinst, MAKEINTRESOURCE(700)); 
            hbmpCrayon = LoadBitmap(hinst, MAKEINTRESOURCE(701)); 
            hbmpMarker = LoadBitmap(hinst, MAKEINTRESOURCE(702)); 
            hbmpPen = LoadBitmap(hinst, MAKEINTRESOURCE(703)); 
            hbmpFork = LoadBitmap(hinst, MAKEINTRESOURCE(704)); 
 
            // Retrieve list box handle. 
 
            hListBox = GetDlgItem(hDlg, IDL_STUFF); 
 
            // Initialize the list box text and associate a bitmap 
            // with each list box item. 
 
            AddItem(hListBox, "pencil", hbmpPencil); 
            AddItem(hListBox, "crayon", hbmpCrayon); 
            AddItem(hListBox, "marker", hbmpMarker); 
            AddItem(hListBox, "pen",    hbmpPen); 
            AddItem(hListBox, "fork",   hbmpFork); 
 
            SetFocus(hListBox); 
            SendMessage(hListBox, LB_SETCURSEL, 0, 0); 
            return TRUE; 
 
        case WM_MEASUREITEM: 
 
            lpmis = (LPMEASUREITEMSTRUCT) lParam; 
 
            // Set the height of the list box items. 
 
            lpmis->itemHeight = 20; 
            return TRUE; 
 
        case WM_DRAWITEM: 
 
            lpdis = (LPDRAWITEMSTRUCT) lParam; 
 
            // If there are no list box items, skip this message. 
 
            if (lpdis->itemID == -1) 
            { 
                break; 
            } 
 
            // Draw the bitmap and text for the list box item. Draw a 
            // rectangle around the bitmap if it is selected. 
 
            switch (lpdis->itemAction) 
            { 
                case ODA_SELECT: 
                case ODA_DRAWENTIRE: 
 
                    // Display the bitmap associated with the item. 
 
                    hbmpPicture =(HBITMAP)SendMessage(lpdis->hwndItem, 
                        LB_GETITEMDATA, lpdis->itemID, (LPARAM) 0); 
 
                    hdcMem = CreateCompatibleDC(lpdis->hDC); 
                    hbmpOld = SelectObject(hdcMem, hbmpPicture); 
 
                    BitBlt(lpdis->hDC, 
                        lpdis->rcItem.left, lpdis->rcItem.top, 
                        lpdis->rcItem.right - lpdis->rcItem.left, 
                        lpdis->rcItem.bottom - lpdis->rcItem.top, 
                        hdcMem, 0, 0, SRCCOPY); 
 
                    // Display the text associated with the item. 
 
                    SendMessage(lpdis->hwndItem, LB_GETTEXT, 
                        lpdis->itemID, (LPARAM) tchBuffer); 
 
                    GetTextMetrics(lpdis->hDC, &tm); 
 
                    y = (lpdis->rcItem.bottom + lpdis->rcItem.top - 
                        tm.tmHeight) / 2;
						
                    hr = StringCchLength(tchBuffer, BUFFER, pcch);
                    if (FAILED(hr))
                    {
                        // TODO: Handle error.
                    }
 
                    TextOut(lpdis->hDC, 
                        XBITMAP + 6, 
                        y, 
                        tchBuffer, 
                        pcch); 						
 
                    SelectObject(hdcMem, hbmpOld); 
                    DeleteDC(hdcMem); 
 
                    // Is the item selected? 
 
                    if (lpdis->itemState & ODS_SELECTED) 
                    { 
                        // Set RECT coordinates to surround only the 
                        // bitmap. 
 
                        rcBitmap.left = lpdis->rcItem.left; 
                        rcBitmap.top = lpdis->rcItem.top; 
                        rcBitmap.right = lpdis->rcItem.left + XBITMAP; 
                        rcBitmap.bottom = lpdis->rcItem.top + YBITMAP; 
 
                        // Draw a rectangle around bitmap to indicate 
                        // the selection. 
 
                        DrawFocusRect(lpdis->hDC, &rcBitmap); 
                    } 
                    break; 
 
                case ODA_FOCUS: 
 
                    // Do not process focus changes. The focus caret 
                    // (outline rectangle) indicates the selection. 
                    // The IDOK button indicates the final 
                    // selection. 
 
                    break; 
            } 
            return TRUE; 
 
        case WM_COMMAND: 
 
            switch (LOWORD(wParam)) 
            { 
                case IDOK: 
                    // Get the selected item's text. 
 
                    nItem = SendMessage(GetDlgItem(hDlg, IDL_STUFF), 
                       LB_GETCURSEL, 0, (LPARAM) 0); 
                       hbmp = SendMessage(GetDlgItem(hDlg, IDL_STUFF), 
                            LB_GETITEMDATA, nItem, 0); 
 
                    // If the item is not the correct answer, tell the 
                    // user to try again. 
                    //
                    // If the item is the correct answer, congratulate 
                    // the user and destroy the dialog box. 
 
                    if (hbmp != hbmpFork) 
                    { 
                        MessageBox(hDlg, "Try again!", "Oops", MB_OK); 
                        return FALSE; 
                    } 
                    else 
                    { 
                        MessageBox(hDlg, "You're right!", 
                            "Congratulations.", MB_OK); 
 
                      // Fall through. 
                    } 
 
                case IDCANCEL: 
 
                    // Destroy the dialog box. 
 
                    EndDialog(hDlg, TRUE); 
                    return TRUE; 
 
                default: 
 
                    return FALSE; 
            } 
 
        case WM_DESTROY: 
 
            // Free any resources used by the bitmaps. 
 
            DeleteObject(hbmpPencil); 
            DeleteObject(hbmpCrayon); 
            DeleteObject(hbmpMarker); 
            DeleteObject(hbmpPen); 
            DeleteObject(hbmpFork); 
 
            return TRUE; 
 
        default: 
            return FALSE; 
 
    } 
    return FALSE; 
} 
*/
