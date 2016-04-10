// Chat Server for BPQ32 Packet Switch
//
//
// Based on MailChat Version 1.4.48.1


#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#include "BPQChat.h"

#ifdef LINBPQ
#include "CHeaders.h"
#endif



#ifndef LINBPQ
#include <new.h>
#endif


#define MaxSockets 64

extern ChatCIRCUIT ChatConnections[MaxSockets+1];
extern int	NumberofChatStreams;

extern struct SEM ChatSemaphore;
extern struct SEM AllocSemaphore;
extern struct SEM ConSemaphore;
extern struct SEM OutputSEM;

extern char OtherNodesList[1000];
extern int MaxChatStreams;

INT_PTR CALLBACK InfoDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int Connected(Stream)
{
	int n;
	ChatCIRCUIT * conn;
	struct UserInfo * user = NULL;
	char callsign[10];
	int port, paclen, maxframe, l4window;
	char ConnectedMsg[] = "*** CONNECTED    ";
	char Msg[100];
	LINK    *link;
	KNOWNNODE *node;

	for (n = 0; n < NumberofChatStreams; n++)
	{
  		conn = &ChatConnections[n];
		
		if (Stream == conn->BPQStream)
		{
			if (conn->Active)
			{
				// Probably an outgoing connect
		
				if (conn->rtcflags == p_linkini)
				{
					conn->paclen = 236;
					nprintf(conn, "c %s\r", conn->u.link->call);
					return 0;
				}
			}
	
			memset(conn, 0, sizeof(ChatCIRCUIT));		// Clear everything
			conn->Active = TRUE;
			conn->BPQStream = Stream;

			conn->Secure_Session = GetConnectionInfo(Stream, callsign,
				&port, &conn->SessType, &paclen, &maxframe, &l4window);

			conn->paclen = paclen;

			strlop(callsign, ' ');		// Remove trailing spaces

			memcpy(conn->Callsign, callsign, 10);

			strlop(callsign, '-');		// Remove any SSID

			user = zalloc(sizeof(struct UserInfo));

			strcpy(user->Call, callsign);

			conn->UserPointer = user;

			n=sprintf_s(Msg, sizeof(Msg), "Incoming Connect from %s", user->Call);
			
			// Send SID and Prompt

			WriteLogLine(conn, '|',Msg, n, LOG_CHAT);
			conn->Flags |= CHATMODE;

			nodeprintf(conn, ChatSID, Ver[0], Ver[1], Ver[2], Ver[3]);

			// See if from a defined node
				
			for (link = link_hd; link; link = link->next)
			{
				if (matchi(conn->Callsign, link->call))
				{
					conn->rtcflags = p_linkwait;
					return 0;						// Wait for *RTL
				}
			}

			// See if from a previously known node

			node = knownnode_find(conn->Callsign);

			if (node)
			{
				// A node is trying to link, but we don't have it defined - close

				Logprintf(LOG_CHAT, conn, '!', "Node %s connected, but is not defined as a Node - closing",
					conn->Callsign);

				nodeprintf(conn, "Node %s does not have %s defined as a node to link to - closing.\r",
					OurNode, conn->Callsign);

				ChatFlush(conn);

				Sleep(500);

				Disconnect(conn->BPQStream);

				return 0;
			}

			if (user->Name[0] == 0)
			{
				char * Name = lookupuser(user->Call);

				if (Name)
				{
					if (strlen(Name) > 17)
						Name[17] = 0;

					strcpy(user->Name, Name);
					free(Name);
				}
				else
				{
					conn->Flags |= GETTINGUSER;
					nputs(conn, NewUserPrompt);
					return TRUE;
				}
			}

			SendWelcomeMsg(Stream, conn, user);
			RefreshMainWindow();
			ChatFlush(conn);
			
			return 0;
		}
	}

	return 0;
}

int Disconnected (Stream)
{
	struct UserInfo * user = NULL;
	ChatCIRCUIT * conn;
	int n;
	char Msg[255];
	int len;
	struct _EXCEPTION_POINTERS exinfo;

	for (n = 0; n <= NumberofChatStreams-1; n++)
	{
		conn=&ChatConnections[n];

		if (Stream == conn->BPQStream)
		{
			if (conn->Active == FALSE)
			{
				return 0;
			}

			ChatClearQueue(conn);

			conn->Active = FALSE;
			RefreshMainWindow();

			if (conn->Flags & CHATMODE)
			{
				if (conn->Flags & CHATLINK)
				{
					len=sprintf_s(Msg, sizeof(Msg), "Chat Node %s Disconnected", conn->u.link->call);
					WriteLogLine(conn, '|',Msg, len, LOG_CHAT);
					__try {link_drop(conn);} My__except_Routine("link_drop");
				}
				else
				{
					len=sprintf_s(Msg, sizeof(Msg), "Chat User %s Disconnected", conn->Callsign);
					WriteLogLine(conn, '|',Msg, len, LOG_CHAT);
					__try
					{
						logout(conn);
					}
					#define EXCEPTMSG "logout"
					#include "StdExcept.c"
					}
				}

				conn->Flags = 0;
				conn->u.link = NULL;
				conn->UserPointer = NULL;	
				return 0;
			}

			return 0;
		}
	}
	return 0;
}

int DoReceivedData(int Stream)
{
	int count, InputLen;
	UINT MsgLen;
	int n;
	ChatCIRCUIT * conn;
	struct UserInfo * user;
	char * ptr, * ptr2;
	char Buffer[10000];
	int Written;

	for (n = 0; n < NumberofChatStreams; n++)
	{
		conn = &ChatConnections[n];

		if (Stream == conn->BPQStream)
		{
			do
			{ 
				// May have several messages per packet, or message split over packets

				if (conn->InputLen + 1000 > 10000)	// Shouldnt have lines longer  than this in text mode
					conn->InputLen = 0;				// discard	
				
				GetMsg(Stream, &conn->InputBuffer[conn->InputLen], &InputLen, &count);

				if (InputLen == 0) return 0;

				if (conn->DebugHandle)				// Receiving a Compressed Message
					WriteFile(conn->DebugHandle, &conn->InputBuffer[conn->InputLen],
						InputLen, &Written, NULL);

				conn->Watchdog = 900;				// 15 Minutes

				conn->InputLen += InputLen;

				{

			loop:

				if (conn->InputLen == 1 && conn->InputBuffer[0] == 0)		// Single Null
				{
					conn->InputLen = 0;
					return 0;
				}

				ptr = memchr(conn->InputBuffer, '\r', conn->InputLen);

				if (ptr)	//  CR in buffer
				{
					user = conn->UserPointer;
				
					ptr2 = &conn->InputBuffer[conn->InputLen];
					
					if (++ptr == ptr2)
					{
						// Usual Case - single meg in buffer

						__try
						{
							if (conn->rtcflags == p_linkini)		// Chat Connect
								ProcessConnecting(conn, conn->InputBuffer, conn->InputLen);
							else
								ProcessLine(conn, user, conn->InputBuffer, conn->InputLen);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							conn->InputBuffer[conn->InputLen] = 0;
							Debugprintf("CHAT *** Program Error Processing input %s ", conn->InputBuffer);
							Disconnect(conn->BPQStream);
							conn->InputLen=0;
							CheckProgramErrors();
							return 0;
						}
						conn->InputLen=0;
					}
					else
					{
						// buffer contains more that 1 message

						MsgLen = conn->InputLen - (ptr2-ptr);

						memcpy(Buffer, conn->InputBuffer, MsgLen);
						__try
						{
							if (conn->rtcflags == p_linkini)
								ProcessConnecting(conn, Buffer, MsgLen);
							else
								ProcessLine(conn, user, Buffer, MsgLen);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							Buffer[MsgLen] = 0;
							Debugprintf("CHAT *** Program Error Processing input %s ", Buffer);
							Disconnect(conn->BPQStream);
							conn->InputLen=0;
							CheckProgramErrors();
							return 0;
						}

						if (*ptr == 0 || *ptr == '\n')
						{
							/// CR LF or CR Null

							ptr++;
							conn->InputLen--;
						}

						memmove(conn->InputBuffer, ptr, conn->InputLen-MsgLen);

						conn->InputLen -= MsgLen;

						goto loop;

					}
				}
				}
			} while (count > 0);

			return 0;
		}
	}

	// Socket not found

	return 0;

}

int ConnectState(Stream)
{
	int state;

	SessionStateNoAck(Stream, &state);
	return state;
}
UCHAR * EncodeCall(UCHAR * Call)
{
	static char axcall[10];

	ConvToAX25(Call, axcall);
	return &axcall[0];

}


VOID SendWelcomeMsg(int Stream, ChatCIRCUIT * conn, struct UserInfo * user)
{
		if (!rtloginu (conn, TRUE))
		{
			// Already connected - close
			
			ChatFlush(conn);
			Sleep(1000);
			Disconnect(conn->BPQStream);
		}
		return;

}

VOID SendPrompt(ChatCIRCUIT * conn, struct UserInfo * user)
{
	nodeprintf(conn, "de %s>\r", OurNode);
}

VOID ProcessLine(ChatCIRCUIT * conn, struct UserInfo * user, char* Buffer, int len)
{
	char seps[] = " \t\r";
	struct _EXCEPTION_POINTERS exinfo;

	{
		GetSemaphore(&ChatSemaphore, 0);

		__try 
		{
			ProcessChatLine(conn, user, Buffer, len);
		}
			#define EXCEPTMSG "ProcessChatLine"
			#include "StdExcept.c"

			FreeSemaphore(&ChatSemaphore);
	
			if (conn->BPQStream <  0)
				CloseConsole(conn->BPQStream);
			else
				Disconnect(conn->BPQStream);	

			return;
		}
		FreeSemaphore(&ChatSemaphore);
		return;
	}

	//	Send if possible

	ChatFlush(conn);
}


VOID SendUnbuffered(int stream, char * msg, int len)
{
	if (stream < 0)
		WritetoConsoleWindow(stream, msg, len);
	else
		SendMsg(stream, msg, len);
}


void TrytoSend()
{
	// call Flush on any connected streams with queued data

	ChatCIRCUIT * conn;
	struct ConsoleInfo * Cons;

	int n;

	for (n = 0; n < NumberofChatStreams; n++)
	{
		conn = &ChatConnections[n];
		
		if (conn->Active == TRUE)
			ChatFlush(conn);
	}

	for (Cons = ConsHeader[0]; Cons; Cons = Cons->next)
	{
		if (Cons->Console)
			ChatFlush(Cons->Console);
	}
}


void ChatFlush(ChatCIRCUIT * conn)
{
	int tosend, len, sent;
	
	// Try to send data to user. May be stopped by user paging or node flow control

	//	UCHAR * OutputQueue;		// Messages to user
	//	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	//	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	//	BOOL Paging;				// Set if user wants paging
	//	int LinesSent;				// Count when paging
	//	int PageLen;				// Lines per page


	if (conn->OutputQueue == NULL)
	{
		// Nothing to send. If Close after Flush is set, disconnect

		if (conn->CloseAfterFlush)
		{
			conn->CloseAfterFlush--;
			
			if (conn->CloseAfterFlush)
				return;

			Disconnect(conn->BPQStream);
		}

		return;						// Nothing to send
	}
	tosend = conn->OutputQueueLength - conn->OutputGetPointer;

	sent=0;

	while (tosend > 0)
	{
		if (TXCount(conn->BPQStream) > 4)
			return;						// Busy

		if (tosend <= conn->paclen)
			len=tosend;
		else
			len=conn->paclen;

		GetSemaphore(&OutputSEM, 0);

		SendUnbuffered(conn->BPQStream, &conn->OutputQueue[conn->OutputGetPointer], len);

		conn->OutputGetPointer+=len;

		FreeSemaphore(&OutputSEM);

		tosend-=len;	
		sent++;

		if (sent > 4)
			return;
	}

	// All Sent. Free buffers and reset pointers

	ChatClearQueue(conn);
}

VOID ChatClearQueue(ChatCIRCUIT * conn)
{
	GetSemaphore(&OutputSEM, 0);

	conn->OutputGetPointer=0;
	conn->OutputQueueLength=0;

	FreeSemaphore(&OutputSEM);
}

/*
char * FormatDateAndTime(time_t Datim, BOOL DateOnly)
{
	struct tm *tm;
	static char Date[]="xx-xxx hh:mmZ";

	tm = gmtime(&Datim);
	
	if (tm)
		sprintf_s(Date, sizeof(Date), "%02d-%3s %02d:%02dZ",
					tm->tm_mday, month[tm->tm_mon], tm->tm_hour, tm->tm_min);

	if (DateOnly)
	{
		Date[6]=0;
		return Date;
	}
	
	return Date;
}
*/


VOID FreeList(char ** Hddr)
{
	VOID ** Save;
	
	if (Hddr)
	{
		Save = Hddr;
		while(Hddr[0])
		{
			free(Hddr[0]);
			Hddr++;
		}	
		free(Save);
	}
}

#define LIBCONFIG_STATIC
#include "..\BPQMail\libconfig.h"


config_t cfg;
config_setting_t * group;

extern char ChatWelcomeMsg[1000];

VOID xSaveIntValue(config_setting_t * group, char * name, int value)
{
	config_setting_t *setting;
	
	setting = config_setting_add(group, name, CONFIG_TYPE_INT);
	if(setting)
		config_setting_set_int(setting, value);
}

VOID xSaveStringValue(config_setting_t * group, char * name, char * value)
{
	config_setting_t *setting;

	setting = config_setting_add(group, name, CONFIG_TYPE_STRING);
	if (setting)
		config_setting_set_string(setting, value);

}


VOID SaveNewFormatConfig(char * ConfigName)
{
	config_setting_t *root, *group;

	char Position[81] = "";
	char PopupText[251] = "";
	int Click = 0, Hover = 0;

	GetStringValue("MapPosition", Position, 80);
	GetStringValue("MapPopup", PopupText, 250);
	Click = GetIntValue("PopupMode", 0);


	//	Get rid of old config before saving
	
	config_init(&cfg);

	root = config_root_setting(&cfg);

	group = config_setting_add(root, "Chat", CONFIG_TYPE_GROUP);

	xSaveIntValue(group, "ApplNum", ChatApplNum);
	xSaveIntValue(group, "MaxStreams", MaxChatStreams);
	xSaveStringValue(group, "OtherChatNodes", OtherNodesList);
	xSaveStringValue(group, "ChatWelcomeMsg", ChatWelcomeMsg);

	xSaveStringValue(group, "MapPosition", Position);
	xSaveStringValue(group, "MapPopup", PopupText);
	xSaveIntValue(group, "PopupMode", Click);

	if(! config_write_file(&cfg, ConfigName))
	{
		fprintf(stderr, "Error while writing file.\n");
		config_destroy(&cfg);
		return;
	}
	config_destroy(&cfg);
}



VOID SaveChatConfig(HWND hDlg)
{
#ifdef LINBPQ

#else

	BOOL OK1;
	HKEY hKey=0;
	int OldChatAppl;
	char * ptr1, * ptr2;
	char * Context;

	OldChatAppl = ChatApplNum;
	
	ChatApplNum = GetDlgItemInt(hDlg, ID_CHATAPPL, &OK1, FALSE);
	MaxChatStreams = GetDlgItemInt(hDlg, ID_STREAMS, &OK1, FALSE);

	if (ChatApplNum)	
	{
		ptr1=GetApplCall(ChatApplNum);

		if (ptr1 && (*ptr1 < 0x21))
		{
			MessageBox(NULL, "WARNING - There is no APPLCALL in BPQCFG matching the confgured ChatApplNum. Chat will not work",
				"BPQMailChat", MB_ICONINFORMATION);
		}
	}

	SaveIntValue("ApplNum", ChatApplNum);
	SaveIntValue("MaxStreams", MaxChatStreams);

	GetMultiLineDialog(hDlg, IDC_ChatNodes);

	SaveStringValue("OtherChatNodes", OtherNodesList);
	
	// Show dialog box now - gives time for links to close
	
	// reinitialise other nodes list. rtlink messes with the string so pass copy

	node_close();

	if (ChatApplNum == OldChatAppl)
		wsprintf(InfoBoxText, "Configuration Changes Saved and Applied");
	else
	{
		ChatApplNum = OldChatAppl;
		wsprintf(InfoBoxText, "Warning Program must be restarted to change Chat Appl Num");
	}

#ifndef LINBPQ
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
#endif

	removelinks();
	 
	// Set up other nodes list. rtlink messes with the string so pass copy
	
	ptr2 = ptr1 = strtok_s(_strdup(OtherNodesList), " ,\r", &Context);

	while (ptr1)
	{
		rtlink(ptr1);			
		ptr1 = strtok_s(NULL, " ,\r", &Context);
	}

	free(ptr2);

	if (user_hd)			// Any Users?
		makelinks();		// Bring up links

#endif

}
