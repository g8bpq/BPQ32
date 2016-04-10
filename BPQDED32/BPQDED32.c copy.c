
//	BPQDED32.dll

//	DLL to emulate F6FBB's TFWIN32 - a windows interface to TNCs running WA8DED's Host Mode

//	Allows such programs to use the BPQ32 switch.

//	Suite also includes BPQDED16, which eumlates TFWIN.dll. It simply thunks all calls
//		to BPQDED32


//	Version 0.0 February 2006

//	Version 1.0 October 2006

//		Fix monitoring

//  Version 1.02 November 2007

//		Fix problem with corruption of MONFLAG

#define _CRT_SECURE_NO_DEPRECATE

#include "windows.h"

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )


HINSTANCE ExtDriver=0;

int *BPQAPI=0;

typedef int (FAR *FARPROCX)();

int AllocateStream();
int DeallocateStream(int stream);
void SetMask(int stream,int flags,int mask);

FARPROCX CheckTimer;
FARPROCX GETBPQAPI;

int AttachedProcesses=0;


int RXCOUNT=0;

unsigned char RXBUFFER[512];		//	BUFFER TO PC APPLICATION

unsigned char * RXPOINTER=&RXBUFFER[0];

unsigned char * PUTPTR=&RXBUFFER[0] ;


unsigned char LINEBUFFER[300];		// MSG FROM PC APPL

unsigned char * CURSOR=&LINEBUFFER[0];



unsigned char NODEBUFFER[300];		// MESSAGE FROM NODE
unsigned char WORKAREA[300];		// UNPACK AREA FOR CHAINED BUFFERS

;
char ECHOFLAG=1;		// ECHO ENABLED
char MODE=0;				// INITIALLY TERMINAL MODE
char HOSTSTATE=0;		// HOST STATE MACHINE 
int MSGCOUNT=0;			// LENGTH OF MESSAGE EXPECTED
int MSGLENGTH=0;
char MSGTYPE=0;
char MSGCHANNEL=0;

int APPLMASK=1;
char STREAMBASE=50;

char APPLFLAGS=0x42;		// REQUEST AUTOTIMERS, MSG TO USER
char DEDMODE=' ';			// CLUSTER MODE - DONT ALLOW DUP CONNECTS


#define MAXSTREAMS 32

int HOSTSTREAMS=4;			// Default Streams

unsigned int MMASK=0xFFFF;
unsigned short D10=10;
unsigned short D16=16;

unsigned short TENK[]={10000,1000,100,10};

unsigned short NEWVALUE;
unsigned char MCOM;

unsigned int ecxsave;
char ahsave;
char alsave;

#define CHAN_TXQ	0	//		; FRAMES QUEUED TO NODE


unsigned int CCT[MAXSTREAMS+1];	// Queue headers

int	bpqstream[MAXSTREAMS+1]={0};		// BPQ Stream Numbers  

#define CCTLEN 4

#define BUFFERS	800 //1099
#define BUFFLEN	50

unsigned int FREE_Q=0;

unsigned int QCOUNT=0;

unsigned int MINBUFFCOUNT=BUFFERS;

unsigned int NOBUFFCOUNT=0;

unsigned char BUFFERPOOL[BUFFLEN*BUFFERS];



/*
;
;	BUFFER POOL USES A SYSTEM OF CHAINED SMALL BUFFERS
;
;	FIRST DWORD OF FIRST IS USED FOR CHAINING MESSAGES
;	SECOND DWORD IS LENGTH OF DATA
;	LAST DWORD IS POINTER TO NEXT BUFFER
;
;	ALL BUT LAST FOUR BYTES OF SUBSEQUENT BUFFERS ARE USED FOR DATA
;
;	BUFFERS ARE ONLY USED TO STORE OUTBOUND MESSAGES WHEN SESSION IS
;	BUSY, OR NODE IS CRITICALLY SHORT OF BUFFERS
;

*/

unsigned char PARAMREPLY[]="* 0 0 64 10 4 4 10 100 18000 30 2 0 2\r\n";

#define PARAMPORT PARAMREPLY+2

#define LPARAMREPLY	39

unsigned char BADCMDREPLY[]="\x2" "INVALID COMMAND\x0";

#define LBADCMDREPLY 17 //sizeof BADCMDREPLY


unsigned char  ICMDREPLY[]="\x2" "         \x0";
#define LICMDREPLY 11

unsigned char DATABUSYMSG[]="\x2" "TNC BUSY - LINE IGNORED\x0";
#define LDATABUSY 25

unsigned char BADCONNECT[]="\x2" "INVALID CALLSIGN\x0";
#define LBADCONNECT	18

unsigned char BUSYMSG[]="BUSY fm SWITCH\x0";

unsigned char CONSWITCH[]="\x3" "(1) CONNECTED to 0:          \x0";

#define CONCALL CONSWITCH+20
#define LCONSWITCH	31

unsigned char DISCMSG[]="\x3" "(1) DISCONNECTED fm 0:SWITCH\x0";
#define LDISC 30

unsigned char SWITCH[]="\x1" "0:SWITCH    \x0";
#define CHECKCALL SWITCH+3
#define LSWITCH	14
	
unsigned char NOTCONMSG[]="\x1" "CHANNEL NOT CONNECTED\x0";
#define LNOTCON	23

unsigned char ALREADYCONMSG[]="You are already connected on another port\r";
#define ALREADYLEN	45

unsigned char MONITORDATA[350];			//  RAW FRAME FROM NODE

unsigned char MONBUFFER[258]="\x6";

int MONLENGTH;

unsigned char * MONCURSOR=0;

unsigned char MONHEADER[256];

int MONFLAG=0;


//	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT

#define MSGCHAIN 0		// CHAIN WORD
#define MSGPORT	4		// PORT 
#define MMSGLENGTH WORD PTR 5	// LENGTH

#define MSGDEST	7		// DESTINATION
#define MSGORIGIN 14

//	 MAY BE UP TO 56 BYTES OF DIGIS

#define MSGCONTROL	21		// CONTROL BYTE
#define MSGPID		22		// PROTOCOL IDENTIFIER
#define MSGDATA		23		// START OF LEVEL 2 MESSAGE

unsigned char PORT_NO=0		;	// Received port number 0 - 256
unsigned char FRAME_TYPE=0		;// Frame Type           UI etc in Hex
unsigned char PID=0;			// Protocol ID
unsigned short FRAME_LENGTH=0;	// Length of frame      0 - 65...
unsigned char INFO_FLAG=0;		// Information Packet ? 0 No, 1 Yes
unsigned char OPCODE=0;			//L4 FRAME TYPE
unsigned char FRMRFLAG=0;

unsigned char MCOM=1;
unsigned char MALL=1;
unsigned char HEADERLN=1;

#define CR 0x0D
#define LF 0x0A

static char TO_MSG		[]=" to ";
static char FM_MSG		[]="fm ";

static char MYCCT_MSG[]=" my";
static char CCT_MSG	[]=" cct=";
static char TIM_MSG[]=" t/o=";

char NR_INFO_FLAG;		// Information Packet ? 0 No, 1 Yes

//	HDLC COMMANDS (WITHOUT P/F)

#define SABM	0x2F
#define DISC	0x43
#define DM		0x0F
#define UA		0x63
#define FRMR	0x87

#define PFBIT	0x10

#define UI 3
#define RR 1
#define RNR 5
#define REJ 9

static char CTL_MSG[]=" ctl ";
static char PID_MSG[]=" pid ";
static char SABM_MSG[]="SABM";
static char DISC_MSG[]="DISC";
static char UA_MSG[]="UA";

static char DM_MSG	[]="DM";
static char RR_MSG	[]="RR";
static char RNR_MSG[]="RNR";
static char I_MSG[]="I pid ";
static char UI_MSG[]="UI pid ";
static char FRMR_MSG[]="FRMR";
static char REJ_MSG[]="REJ";

static char AX25CALL[8];	// WORK AREA FOR AX25 <> NORMAL CALL CONVERSION
static char NORMCALL[11];	// CALLSIGN IN NORMAL FORMAT
static int NORMLEN;			// LENGTH OF CALL IN NORMCALL	


BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{

	HANDLE hInstance;

	char Errbuff[100];
	char buff[20];

	hInstance=hInst;
	
	switch( ul_reason_being_called )
	{
		case DLL_PROCESS_ATTACH:
			
			AttachedProcesses++;

			_itoa(AttachedProcesses,buff,10);
			strcpy(Errbuff,	"BPQDED32 V 1.0 Process Attach - total = ");
			strcat(Errbuff,buff);
			OutputDebugString(Errbuff);

	_asm{

;
;	BUILD BUFFER POOL
;
	MOV	EDI,OFFSET BUFFERPOOL
	MOV	ECX,BUFFERS

BUFF000:

	MOV	ESI,OFFSET FREE_Q

	MOV	EAX,[ESI]			; OLD FIRST IN CHAIN
	MOV	[EDI],EAX
	MOV	[ESI],EDI			; PUT NEW ON FRONT

	INC	QCOUNT

	ADD	EDI,BUFFLEN
	LOOP	BUFF000

	MOV	EAX,QCOUNT
	MOV	MINBUFFCOUNT,EAX

	}

            break;

        case DLL_PROCESS_DETACH:

			// Keep track of attaced processes

			AttachedProcesses--;

			_itoa(AttachedProcesses,buff,10);
			strcpy(Errbuff,	"BPQDED32 Process Detach - total = ");
			strcat(Errbuff,buff);
			OutputDebugString(Errbuff);

			break;

         default:

            break;
	}
	return 1;
}

BOOLEAN WINAPI CheckifBPQ32isLoaded()
{
	HANDLE Mutex;
	UCHAR Value[100];

	char bpq[]="BPQ32.exe";
	char *fn=(char *)&bpq;
	HKEY hKey=0;
	int i,ret,Type,Vallen=99;

	char Errbuff[100];
	char buff[20];		

	STARTUPINFO  StartupInfo;					// pointer to STARTUPINFO 
    PROCESS_INFORMATION  ProcessInformation; 	// pointer to PROCESS_INFORMATION 
	
	OutputDebugString("BPQDED32 Check if BPQ32 Loaded Called");

	// See if BPQ32 is running - if we create it in the NTVDM address space by
	// loading bpq32.dll it will not work.

	Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

	if (Mutex == NULL)
	{	
		OutputDebugString("BPQDED32 bpq32.dll is not already loaded - Loading BPQ32.exe");

		// Get address of BPQ Directory

		Value[0]=0;

		ret = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

		if (ret == ERROR_SUCCESS)
		{
			ret = RegQueryValueEx(hKey, "BPQ Directory", 0, &Type,(UCHAR *)&Value, &Vallen);
		
			if (ret == ERROR_SUCCESS)
			{
				if (strlen(Value) == 2 && Value[0] == '"' && Value[1] == '"')
					Value[0]=0;
			}


			if (Value[0] == 0)
			{
		
				// BPQ Directory absent or = "" - "try Config File Location"
			
				ret = RegQueryValueEx(hKey,"Config File Location",0,			
							&Type,(UCHAR *)&Value,&Vallen);

				if (ret == ERROR_SUCCESS)
				{
					if (strlen(Value) == 2 && Value[0] == '"' && Value[1] == '"')
						Value[0]=0;
				}

			}
			RegCloseKey(hKey);
		}
				
		if (Value[0] == 0)
		{
			strcpy(Value,fn);
		}
		else
		{
			strcat(Value,"\\");
			strcat(Value,fn);				
		}

		StartupInfo.cb=sizeof(StartupInfo);
		StartupInfo.lpReserved=NULL; 
		StartupInfo.lpDesktop=NULL; 
		StartupInfo.lpTitle=NULL; 
 		StartupInfo.dwFlags=0; 
 		StartupInfo.cbReserved2=0; 
  		StartupInfo.lpReserved2=NULL; 

		if (!CreateProcess(Value,NULL,NULL,NULL,FALSE,
							CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
							NULL,NULL,&StartupInfo,&ProcessInformation))
		{				
			ret=GetLastError();

			_itoa(ret,buff,10);

			strcpy(Errbuff,	"BPQDED32 Load ");
			strcat(Errbuff,Value);
			strcat(Errbuff," failed ");
			strcat(Errbuff,buff);
			OutputDebugString(Errbuff);

			return FALSE;		
		}

		// Wait up to 20 secs for BPQ32 to start

		for ( i = 0; i < 20; i++ ) 
		{
			Sleep(1000);

			Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

			if (!(Mutex == NULL))
				break;
		}
	
		if (Mutex == NULL)
		{			
			OutputDebugString("BPQDED32 bpq32.exe failed to initialize within 20 seconds");

			return FALSE;
		}

	}

	CloseHandle(Mutex);

	return TRUE;
}

			  
BOOLEAN WINAPI TfOpen(HWND hWnd) 
{
	int i;

	char Errbuff[100];
	char buff[20];

	OutputDebugString("BPQDED32 in TfOpen");

	ExtDriver=LoadLibrary("bpq32.dll");

	if (ExtDriver == NULL)
	{
		OutputDebugString("BPQDED32 Error Loading bpq32.dll");
		return FALSE;
	}

	GETBPQAPI = (FARPROCX)GetProcAddress(ExtDriver,"_GETBPQAPI@0");
	CheckTimer = (FARPROCX)GetProcAddress(ExtDriver,"_CheckTimer@0");
	
	if (GETBPQAPI == NULL || CheckTimer == NULL)
	{
		OutputDebugString("BPQDED32 Error finding BPQ32 API Entry Points");
		return FALSE;
	}

	BPQAPI=(int *)GETBPQAPI();

	for (i=1; i <= HOSTSTREAMS; i++)
	{
		bpqstream[i]=AllocateStream();
			
		if (bpqstream[i]) SetMask(bpqstream[i],APPLFLAGS,APPLMASK);

		strcpy(Errbuff,	"BPQDED32 init Stream ");

		_itoa(i,buff,10);
		strcat(Errbuff,buff);
		strcat(Errbuff," BPQ Stream");

		_itoa(bpqstream[i],buff,10);
		strcat(Errbuff,buff);

		strcat(Errbuff,"\n");

		OutputDebugString(Errbuff);


	}

	bpqstream[0]=bpqstream[1];				// For monitoring


	return TRUE;

}

int AllocateStream()
{
   int retcode;

	_asm {

		;	AH = 13 Allocate/deallocate stream
;		If AL=0, return first free stream and allocate
;		If AL>0, CL=1, Allocate stream. If aleady allocated,
;		   return CX nonzero, else allocate, and return CX=0
;		If AL>0, CL=2, Release stream
;		
	MOV	AH,13
	MOV	AL,0

	call BPQAPI

	mov	retcode,eax
	}
	return retcode;						// Stream was EAX, but keeps compiler happy
}

int DeallocateStream(int stream)
{

	// Clear applmask, close connection, ack any status change,  then deallocate

	_asm {

	mov	eax,stream
	MOV	AH,1			; SET APPL MASK

	MOV	ecx,0
	MOV	edx,0

	call BPQAPI
;
	mov	eax,stream
	MOV	AH,6
	MOV	CX,2			; DISCONNECT
	call BPQAPI

	mov	eax,stream
	MOV	AH,5

	call BPQAPI			; ACK THE STATUS CHANGE

;	AH = 13 Allocate/deallocate stream
;		If AL=0, return first free stream and allocate
;		If AL>0, CL=1, Allocate stream. If aleady allocated,
;		   return CX nonzero, else allocate, and return CX=0
;		If AL>0, CL=2, Release stream
;		
	MOV	eax,stream
	MOV	AH,13
	mov	cl,2

	call BPQAPI

	}
	return 0;
}


void SetMask(stream,flags,mask)
{
	_asm {
		
	mov	eax,stream
	MOV	AH,1			; SET APPL MASK

	MOV	ecx,flags
	MOV	edx,mask

	call BPQAPI
	}

	return;
}

BOOL  WINAPI TfClose(void) 
{
   int i,port;

	char Errbuff[100];
	char buff[20];

	OutputDebugString("BPQDED32 in TfClose");

	for (i=1; i <= HOSTSTREAMS; i++)
	{
		port=bpqstream[i];
			
		if (port)
		{
			DeallocateStream(port);

			strcpy(Errbuff,	"BPQDED Close Stream ");

			_itoa(i,buff,10);
			strcat(Errbuff,buff);
			strcat(Errbuff," BPQ Stream");

			_itoa(bpqstream[i],buff,10);
			strcat(Errbuff,buff);

			strcat(Errbuff,"\n");

			OutputDebugString(Errbuff);

			bpqstream[i]=0;		// Disable DED stream
		}
	}
	return TRUE;
}


void APIENTRY TfChck(void) 
{
_asm {

	MOV	EAX,0			; Nothing Doing
	
	CMP	RXCOUNT,0
	JE	NOCHARS

	MOV	EAX,1
	
NOCHARS:
}
	return;
}


void APIENTRY TfGet(void) 
{
	//	Returns -1 if none available

	_asm {

 	MOV	EAX,-1			; NOTHING DOING 

	CMP	RXCOUNT,0
	JE	NOCHARS

	MOV	EBX,RXPOINTER
	MOVZX	EAX,BYTE PTR[EBX]
	INC	EBX

	CMP	EBX,OFFSET RXBUFFER+512
	JNE	GETRET

	MOV	EBX,OFFSET RXBUFFER
GETRET:
	MOV	RXPOINTER,EBX

	DEC	RXCOUNT

NOCHARS:

}

  return;		 // ret value is already in ax

}

 

BOOLEAN APIENTRY TfPut(char character) 
{
	char Errbuff[1000];
	char buff[20];

	HKEY hKey=0;
	int retCode,Type,Vallen=4;
	char * RegValue;


	int i,port;

	_asm {

	xor	eax,eax	
	mov	al,character

	CMP	MODE,1
	JNE	CHARMODE
;
;	HOST MODE
;
	CMP	HOSTSTATE,0
	JE	SETCHANNEL

	CMP	HOSTSTATE,1
	JE	SETMSGTYPE

	CMP	HOSTSTATE,2
	JE	SETLENGTH
;
;	RECEIVING COMMAND/DATA
;
	MOV	EBX,CURSOR
	MOV	[EBX],AL
	INC	EBX
	MOV	CURSOR,EBX
;
	DEC	MSGCOUNT
	JNZ	PUTRETURN				; MORE TO COME

	CALL	PROCESSHOSTPACKET

	MOV	HOSTSTATE,0
	MOV	CURSOR,OFFSET LINEBUFFER

	MOV	AH,0
	JMP SHORT PUTRETURN

SETLENGTH:

	MOV	AH,0
	INC	AX
	
	MOV	MSGCOUNT,EAX			; WORKING FIELD
	MOV	MSGLENGTH,EAX

	JMP SHORT HOSTREST

SETMSGTYPE:

	MOV	MSGTYPE,AL
	JMP SHORT HOSTREST

SETCHANNEL:

	MOV	MSGCHANNEL,AL

HOSTREST:

	INC	HOSTSTATE
	JMP SHORT PUTRETURN

CHARMODE:

	CMP	AL,11H
	JE	IGNORE

	CMP	AL,18H
	JNE	NOTCANLINE
;
;	CANCEL INPUT
; 
	MOV	CURSOR,OFFSET LINEBUFFER

	JMP PUTRETURN

NOTCANLINE:

	MOV	EBX,CURSOR
	MOV	[EBX],AL
	INC	EBX

	CMP	EBX,OFFSET LINEBUFFER+300
	JE	OVERFLOW

	MOV	CURSOR,EBX
;
OVERFLOW:

	CMP	AL,0DH
	JNE	NOTCR
;
;	PROCESS COMMAND (UNLESS HOST MODE)
;
	MOV	EBX,CURSOR
	MOV	[EBX],0

	CALL	DOCOMMAND

NOTCR:
PUTRETURN:


}

	return TRUE;

	_asm {

ATCOMMAND:


	CMP	LINEBUFFER+1,'B'
	JE	BUFFSTAT

	CMP	LINEBUFFER+1,'M'
	JE	BUFFMIN

	CMP	LINEBUFFER+1,'S'
	JE	SETMASK

	JMP	BADCMD

SETMASK:

	MOV	ESI,CURSOR
	MOV	WORD PTR [ESI],2020H	; SPACE ON END

	MOV	ESI,OFFSET LINEBUFFER+2
	CALL	GETVALUE
	JC	MASKBAD

	MOVZX	EAX,NEWVALUE
	MOV	MMASK,EAX

	CALL	GETVALUE
	JC	MASKBAD

	MOV	AX,NEWVALUE
	MOV	MCOM,AL

MASKBAD:

	PUSH	MMASK
	JMP SHORT BUFFCOMM

BUFFMIN:

	PUSH	MINBUFFCOUNT
	JMP SHORT BUFFCOMM
;
BUFFSTAT:

	PUSH	QCOUNT

BUFFCOMM:

	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

	MOV	AL,1
	CALL	PUTCHAR
;
;	GET BUFFER COUNT
;
	POP	EAX
	CALL	CONV_5DIGITS

	MOV	AL,0
	CALL	PUTCHAR

	RET

ICMD:

	}

	// Look for registry key to get appl mask

	LINEBUFFER[MSGLENGTH]=0;

	RegValue=&LINEBUFFER[1];

	if (*RegValue == 0x20) RegValue++;

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\DEDAppl",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{	
		retCode = RegQueryValueEx(hKey,RegValue,0,			
			&Type,(UCHAR *)&APPLMASK,&Vallen);
	
	}
	
	strcpy(Errbuff,	"BPQDED32 ICMD ");

	_itoa(MSGCHANNEL,buff,10);
	strcat(Errbuff,buff);
	strcat(Errbuff," ");

	_itoa(MSGTYPE,buff,10);
	strcat(Errbuff,buff);
	strcat(Errbuff," ");

	strcat(Errbuff,LINEBUFFER);

	strcat(Errbuff," Applmask ");
	_itoa(APPLMASK,buff,10);
	strcat(Errbuff,buff);

	strcat(Errbuff,"\n");

	OutputDebugString(Errbuff);

	_asm {

	MOV	DL,BYTE PTR APPLMASK
	CALL	SETAPPLMASK

	JMP	SENDHOSTOK

	MOV	ESI,OFFSET ICMDREPLY
	MOV	ECX,LICMDREPLY

	JMP	SENDCMDREPLY

PROCESSHOSTPACKET:

	CALL	CheckTimer

	MOVZX	EAX,MSGCHANNEL
	CALL	GETCCT			; POINT BX TO CORRECT ENTRY

	CMP	MSGTYPE,0
	JNE	NOTDATA

	JMP	HOSTDATAPACKET

;HOSTCMDS:
;	DD	'G','I', 'J', 'C', 'D', 'L', '@',      'Y', 'M'
;	DD	POLL,ICMD,JCMD,CCMD,DCMD,LCMD,ATCOMMAND,YCMD,HOSTMON

NOTDATA:

	XOR	EAX,EAX
	MOV	AL,LINEBUFFER
	AND	AL,0FFH-20H		; MASK TO UPPER CASE

	CMP	AL,'G'
	JE	POLL

	CMP	AL,'I'
	JE	ICMD

	CMP	AL,'J'
	JE	JCMD

	CMP	AL,'C'
	JE	CCMD

	CMP	AL,'D'
	JE	DCMD

	CMP	AL,'L'
	JE	LCMD

	CMP	AL,'@'
	JE	ATCOMMAND

	CMP	AL,'Y'
	JE	YCMD

	CMP	AL,'M'

	JNE	DUFFHOSTCMD
	

;HOSTMON:

	CMP	LINEBUFFER+1,'N'
	JE	MONITOROFF

	JMP	ENABLEMONITOR

MONITOROFF:

	JMP	DISABLEMONITOR

DUFFHOSTCMD:

	MOV	ESI,OFFSET LINEBUFFER
	MOV	ECX,MSGLENGTH

DIAG00:

	LODSB
;	CALL	PRINTIT

	LOOP	DIAG00

BADCMD:

	MOV	ESI,OFFSET BADCMDREPLY
	MOV	ECX,LBADCMDREPLY

SENDCMDREPLY:

	jecxz duffxx	
		
	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

CMDREPLYLOOP:


	LODSB
	CALL	PUTCHAR

	LOOP	CMDREPLYLOOP

duffxx:
	RET

ENABLEMONITOR:

	MOV	CL,APPLFLAGS
	OR	CL,80H			; REQUEST MONITORING
	JMP SHORT MONCOM

DISABLEMONITOR:

	MOV	CL,APPLFLAGS		; CANCEL MONITORING

MONCOM:

	MOV	DL,BYTE PTR APPLMASK
	MOV	AL,1			; FIRST PORT
	MOV	AH,1			; SET APPL MASK

	CALL	NODE

	JMP	SENDHOSTOK

CCMD:
;
;	CONNECT REQUEST - WE ONLY ALLOW CONNECT TO SWITCH, BUT DONT CHECK
;
	CMP	MSGCHANNEL,0
	JE	JCOMM00			; SETTING UNPROTO ADDR - JUST ACK IT

	CMP	MSGLENGTH,1
	JNE	REALCALL
;
;	STATUS REQUEST - IF CONNECTED, GET CALL
;
	MOV	EDI,OFFSET CHECKCALL	; FOR ANY RETURNED DATA
	MOV	BYTE PTR [EDI],0		; FIDDLE FOR CONNECTED CHECK

	MOV	AH,8
	MOV	AL,MSGCHANNEL		; GET STATUS, AND CALL IF ANY
	CALL	NODE
;
	CMP	CHECKCALL,0
	JE	NOTCONNECTED

	MOV	ESI,OFFSET SWITCH
	MOV	ECX,LSWITCH

	JMP SHORT SENDCMDREPLY

NOTCONNECTED:

	MOV	ESI,OFFSET NOTCONMSG
	MOV	ECX,LNOTCON

	JMP SENDCMDREPLY

REALCALL:

	MOV	CX,1			; CONNECT
	JMP SHORT CDCOMMAND

DCMD:
;
;	DISCONNECT REQUEST
;
	MOV	CX,2			; DISCONNECT
CDCOMMAND:
	MOV	AL,MSGCHANNEL
	MOV	AH,6
	CALL	NODE
;
;	CONNECT/DISCONNECT WILL BE REPORTED VIA NORMAL STATUS CHANGE
;
	JMP	SENDHOSTOK

JCMD:

	MOV	AL,LINEBUFFER+5
	AND	AL,1
	MOV	MODE,AL			; SET HOST MODE (PROBABLY OFF)
;
	JNZ	JCOMM00			; HOST ON??
;
;	DISABLE SWITCH HOST MODE INTERFACE
;
	MOV	DL,0			; APPL MASK
	CALL	SETAPPLMASK
JCOMM00:
	JMP	SENDHOSTOK

LCMD:

	CALL	CHECKTXQUEUE		; SEE IF ANYTHING TO SEND ON THIS CHAN

	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

	MOV	AL,1
	CALL	PUTCHAR
;
;	GET STATE AND QUEUED BUFFERS
;
	MOV	AL,MSGCHANNEL
	OR	AL,AL
	JNZ	NORM_L
;
;	TO MONITOR CHANNEL 
;
;	SEE IF MONITORED FRAMES AVAILABLE

	MOV ESI,0			; In case NODE call failes
	
	MOV	AH,7
	MOV	AL,1
	CALL	NODE

	MOV	DH,30H			; RETURN TWO ZEROS

	CMP	ESI,0
	JE	MON_L			; NO FRAMES

	MOV	DH,31H			; RETURN 1 FRAME

	JMP SHORT MON_L

NORM_L:

	PUSH	EBX			; SAVE CCT

	MOV ECX,0			; In case NODE call failes
	MOV	AH,4			; SEE IF CONNECTED
	CALL	NODE
;
	MOV	DH,'0'			; AX.25 STATE
	OR	CX,CX
	JZ	NOTCON

	MOV	DH,'4'			; CONNECTED

NOTCON:

	MOV	AL,DL			; STATUS MSGS ON RX Q
	CALL	CONV_DIGITS

	MOV	AL,20H
	CALL	PUTCHAR
;
;	GET OTHER QUEUE COUNTS
;
	PUSH	EDX

	MOV EBX,0			; In case NODE call failes
	MOV	AH,7
	MOV	AL,MSGCHANNEL
	CALL	NODE

	POP	EDX

	MOV	AL,BL			; DATA ON RX Q
	CALL	CONV_DIGITS

	MOV	AL,20H
	CALL	PUTCHAR
;
;	NOT SENT IS NUMBER ON OUR QUEUE, NOT ACKED NUMBER FROM SWITCH
;

	POP	EBX			; CCT
;
;	SEE HOW MANY BUFFERS ATTACHED TO Q HEADER IN BX
;
 	XOR	AL,AL

COUNT_Q_LOOP:

	CMP	DWORD PTR [EBX],0
	JE	COUNT_RET

	INC	AL
	MOV	EBX,[EBX]			; FOLLOW CHAIN
	JMP	COUNT_Q_LOOP

COUNT_RET:

	OR	AL,AL
	JZ	LCOMM05			; NOT BUSY

	MOV	DH,'8'			; BUSY

LCOMM05:

	CALL	CONV_DIGITS		; FRAMES NOT SENT (MAY BE > 10)
;
	MOV	AL,20H
	CALL	PUTCHAR

	MOV	AL,CL
	ADD	AL,30H
	CALL	PUTCHAR			; NOT ACKED

	MOV	AL,20H
	CALL	PUTCHAR

MON_L:

	MOV	AL,30H			; TRIES
	CALL	PUTCHAR

	MOV	AL,20H
	CALL	PUTCHAR

	MOV	AL,DH			; STATE
	CALL	PUTCHAR

	MOV	AL,0
	CALL	PUTCHAR

	RET

HOSTDATAPACKET:

	MOV	ESI,OFFSET LINEBUFFER
	MOV	ECX,MSGLENGTH
;
;	IF WE ALREADY HAVE DATA QUEUED, ADD THIS IT QUEUE
;
	CMP	DWORD PTR CHAN_TXQ[EBX],0
	JE	NOTHINGQUEUED
;
;	COPY MESSAGE TO A CHAIN OF BUFFERS
;
	CMP	QCOUNT,10
	JB	CANTSEND		; NO SPACE - RETURN ERROR (?)

QUEUEFRAME:

	CALL	COPYMSGTOBUFFERS	; RETURNS EDI = FIRST (OR ONLY) FRAGMENT

	LEA	ESI,CHAN_TXQ[EBX]
	CALL	Q_ADD

	JMP	SENDHOSTOK

NOTHINGQUEUED:
;
;	MAKE SURE NODE ISNT BUSY
;
	cmp	MSGCHANNEL,0		; UNPROTO Channel
	je	SendUnproto

	PUSH	ESI
	PUSH	ECX
	PUSH	EBX

	MOV ECX,0			; In case NODE call failes
	MOV	AH,7
	MOV	AL,MSGCHANNEL
	CALL	NODE

	POP	EBX
	CMP	CX,4
	JA	MUSTQUEUEIT		; ALREADY BUSY

	CMP	DX,40			; Buffers left
	JB	MUSTQUEUEIT
;
;	OK TO PASS TO NODE
;
	POP	ECX
	POP	ESI
	
	MOV	AH,2			; SEND DATA

	MOV	AL,MSGCHANNEL
	CALL	NODE
;
	JMP	SENDHOSTOK

SendUnproto:

	MOV	AH,2			; SEND DATA
	MOV	AL,0			; unproto to all ports
	CALL	INTVAL

	JMP	SENDHOSTOK

MUSTQUEUEIT:

	POP	ECX
	POP	ESI

	JMP	QUEUEFRAME

CANTSEND:

	MOV	ESI,OFFSET DATABUSYMSG
	MOV	ECX,LDATABUSY

	JMP	SENDCMDREPLY

POLL:

	CALL	CHECKTXQUEUE		; SEE IF ANYTHING TO SEND
	CALL	PROCESSPOLL
	RET

YCMD:

	mov	ESI,CURSOR
	MOV	WORD PTR [ESI],2020H	; SPACE ON END
}

	LINEBUFFER[MSGLENGTH+2]=0x0;
	LINEBUFFER[MSGLENGTH+3]=0x0a;
	LINEBUFFER[MSGLENGTH+4]=0x0;

	OutputDebugString(LINEBUFFER);

	NEWVALUE=atoi(&LINEBUFFER[1]);
	
	if (NEWVALUE >= 0 && NEWVALUE <= MAXSTREAMS)
	{
		if (NEWVALUE < HOSTSTREAMS)
		{
			// Need to get rid of some streams

			for (i=NEWVALUE+1; i<=HOSTSTREAMS; i++)
			{
				port=bpqstream[i];
			
				if (port)
				{
					DeallocateStream(port);

					strcpy(Errbuff,	"BPQDED YCMD Release Stream ");

					_itoa(i,buff,10);
					strcat(Errbuff,buff);
					strcat(Errbuff," BPQ Stream ");

					_itoa(port,buff,10);
					strcat(Errbuff,buff);

					strcat(Errbuff,"\n");

					OutputDebugString(Errbuff);

					bpqstream[i]=0;		// Disable DED stream
				}
			}
		}
		else
		{
			for (i=HOSTSTREAMS+1; i <= NEWVALUE; i++)
			{
				bpqstream[i]=AllocateStream();
			
				if (bpqstream[i])  SetMask(bpqstream[i],APPLFLAGS,APPLMASK);

				strcpy(Errbuff,	"BPQDED YCMD Stream ");

				_itoa(i,buff,10);
				strcat(Errbuff,buff);
				strcat(Errbuff," BPQ Stream ");

				_itoa(bpqstream[i],buff,10);
				strcat(Errbuff,buff);

				strcat(Errbuff,"\n");

				OutputDebugString(Errbuff);
			}
		}
		HOSTSTREAMS=NEWVALUE;
	}

	 _asm {


SENDHOSTOK:

	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

	MOV	AL,0
	CALL	PUTCHAR			; NOTHING DOING

	RET

PROCESSPOLL:
;
;	ASK SWITCH FOR STATUS CHANGE OR ANY RECEIVED DATA
;
	MOV	CL,0			; POLL TYPE

	CMP	MSGLENGTH,1
	JE	GENERALPOLL
;
;	HE'S BEING AWKWARD, AND USING SPECIFIC DATA/STATUS POLL
;
	CMP	LINEBUFFER+1,'0'
	JE	DATAONLY

	CALL	STATUSPOLL
	JMP SHORT POLLEND

GENERALPOLL:

	CALL	STATUSPOLL
	JNZ	POLLRET

DATAONLY:
	CALL	DATAPOLL

POLLEND:
	JNZ	POLLRET			; GOT DATA

	CALL	SENDHOSTOK		; NOTHING DOING

POLLRET:
	RET

DATAPOLL:

	MOV	EDI,OFFSET NODEBUFFER	; FOR ANY RETURNED DATA
	MOV	AH,3
	MOV	AL,MSGCHANNEL
	OR	AL,AL
	JNZ	NOTMONITOR
;
;	POLL FOR MONITOR DATA
;
	CMP	MONFLAG,0
	JE	NOMONITOR
;
;	HAVE ALREADY GOT DATA PART OF MON FRAME OT SEND
;
	MOV	MONFLAG,0

	MOV	ESI,OFFSET MONBUFFER
	MOV	ECX,MONLENGTH

	jecxz BADMONITOR

	JMP	SENDCMDREPLY

BADMONITOR:

	 }
	 OutputDebugString("BPQDED32 Mondata Flag Set with no data");
	 _asm{

NOMONITOR:
;
;	SEE IF ANYTHING TO MONITOR
;
	MOV	AL,1			; FIRST PORT
	MOV	AH,11
	MOV ECX,0			; In case NODE call failes

	MOV	EDI,OFFSET MONITORDATA
	CALL	NODE

	CMP	ECX,0
	JE	DATAPOLLRET

	MOV	EDI,OFFSET MONITORDATA

	TEST MSGPORT[EDI],80H		; TX?
	JNZ	DATAPOLLRET

	CALL	DISPLAYFRAME		; DISPLAY TRACE FRAME

	CMP	MONCURSOR,OFFSET MONHEADER+1
	JE	DATAPOLLRET		; NOTHING DOING

	MOV	EDI,MONCURSOR
	MOV	BYTE PTR [EDI],0		; NULL TERMINATOR

	MOV	ECX,EDI
	SUB	ECX,OFFSET MONHEADER-1	; LENGTH

	MOV	ESI,OFFSET MONHEADER

	CALL	SENDCMDREPLY

	OR	AL,1			; HAVE SEND SOMETHING
	RET

NOTMONITOR:

	MOV ECX,0			; In case NODE call failes
	CALL	NODE
;
	CMP	ECX,0
	JE	DATAPOLLRET

	cmp ecx,256
	jbe okf

	mov ecxsave,ecx

	pushad

	 }

	_itoa(ecxsave,buff,10);
	strcpy(Errbuff,	"BPQDED32 Corrupt Length = ");
	strcat(Errbuff,buff);
	OutputDebugString(Errbuff);
	
	_asm {

	popad

	XOR	AL,AL			; HAVE NOTHING TO SEND
	RET

okf:
;
;	SEND DATA
;
	MOV	AL,MSGCHANNEL
	CALL	PUTCHAR

	MOV	AL,7
	CALL	PUTCHAR

	DEC	ECX
	MOV	AL,CL
	CALL	PUTCHAR			; LENGTH-1
	INC	CX

	MOV	ESI,OFFSET NODEBUFFER

SENDDATLOOP:

 	LODSB
	CALL	PUTCHAR

	LOOP	SENDDATLOOP

	OR	AL,1			; HAVE SEND SOMETHING
	RET


DATAPOLLRET:

	XOR	AL,AL
	RET

STATUSPOLL:

	MOV	AH,4
	MOV	AL,MSGCHANNEL
	CMP	AL,0
	JE	NOSTATECHANGE		; ?? Channel Zero For Unproto ??

	MOV EDX,0			; in case NODE call fails
	CALL	NODE
;
	CMP	DX,0
	JE	NOSTATECHANGE
;
;	PORT HAS CONNECTED OR DISCONNECTED - SEND STATUS CHANGE TO PC
;
	PUSH	ECX			; SAVE

	MOV	AH,5
	MOV	AL,MSGCHANNEL

	CALL	NODE			; ACK THE STATUS CHANGE

	POP	ECX
	CMP	ECX,0
	JNE	SENDHOSTCON
;
;	DISCONNECTED
;
	MOV	ESI,OFFSET DISCMSG
	MOV	ECX,LDISC

	JMP STATUSPOLLEND

SENDHOSTCON:
;
;	GET CALLSIGN
;
	MOV	EDI,OFFSET CONCALL	; FOR ANY RETURNED DATA
	MOV	AH,8
	MOV	AL,MSGCHANNEL		; GET STATUS, AND CALL IF ANY
	CALL	NODE
;
;	IF IN CLUSTER MODE, DONT ALLOW DUPLICATE CONNECTS
;
	CMP	DEDMODE,'C'
	JNE	DONTCHECK

	MOV	AL,1
	MOV	ECX,HOSTSTREAMS

CHECKLOOP:

	PUSH	ECX
	PUSH	EAX

	CMP	AL,MSGCHANNEL
	JE	CHECKNEXT		; DONT CHECK OUR STREAM!

	MOV	EDI,OFFSET CHECKCALL	; FOR ANY RETURNED DATA
	MOV	AH,8
	CALL	NODE
;
	CMP	AH,0
	JE	CHECKNEXT		; NOT CONNECTED (AH HAS SESSION FLAGS)

	MOV	ESI,OFFSET CHECKCALL
	MOV	EDI,OFFSET CONCALL
	MOV	ECX,10
	REP CMPSB

	JNE	CHECKNEXT
;
;	ALREADY CONNECTED - KILL NEW SESSION
;
	MOV	ECX,ALREADYLEN
	MOV	ESI,OFFSET ALREADYCONMSG
	
	MOV	AH,2			; SEND DATA
	MOV	AL,MSGCHANNEL
	CALL	NODE
;
	MOV	CX,2			; DISCONNECT

	MOV	AL,MSGCHANNEL
	MOV	AH,6
	CALL	NODE
;
	MOV	AH,5
	MOV	AL,MSGCHANNEL
	CALL	NODE			; ACK THE STATUS CHANGE

	POP	EAX
	POP	ECX

	XOR	AL,AL			; SET Z

NOSTATECHANGE:

	RET


CHECKNEXT:

	POP	EAX
	POP	ECX

	INC	AL
	LOOP	CHECKLOOP

DONTCHECK:

	MOV	ESI,OFFSET CONSWITCH
	MOV	ECX,LCONSWITCH

STATUSPOLLEND:

	CALL	SENDCMDREPLY
	OR	AL,1			; SET NZ
	RET

CHECKTXQUEUE:
;
;	IF ANYTHING TO SEND, AND NOT BUSY, SEND IT
;
	CMP	DWORD PTR CHAN_TXQ[EBX],0
	JNZ	SOMETHINGONQUEUE

	RET

SOMETHINGONQUEUE:
;
;	MAKE SURE NODE ISNT BUSY
;
	PUSH	EBX			; SAVE CCT

	MOV	AH,7
	MOV	AL,MSGCHANNEL
	CALL	NODE

	POP	EBX
	CMP	ECX,4
	JA	STILLBUSY		; ALREADY BUSY

	CMP	DX,25
	JB	STILLBUSY
;
;	OK TO PASS TO NODE
;
	LEA	ESI,CHAN_TXQ[EBX]
	CALL	Q_REM

	PUSH	EBX

	MOV	ESI,EDI			; BUFFER CHAIN
	MOV	EDI,OFFSET WORKAREA

	MOV		ECX,4[ESI]		; LENGTH
	PUSH	ECX
	CALL	COPYBUFFERSTOSTRING	; UNPACK CHAIN
	POP		ECX

	MOV	ESI,OFFSET WORKAREA

	MOV	AH,2			; SEND DATA
	MOV	AL,MSGCHANNEL
	CALL	NODE
;
	POP	EBX

STILLBUSY:

	RET

GETCCT:

	MOV	EBX,OFFSET CCT
	SAL	EAX,2			;	* 4
	ADD	EBX,EAX

	RET

SETAPPLMASK:

	MOV	ECX,HOSTSTREAMS		; STREAMS SUPPORTED
	JCXZ noports

	MOV	AH,1			; SET APPL MASK
	MOV	AL,1			; FIRST PORT

ENABLEHOST:

	PUSH	EAX
	PUSH	ECX

	MOV	CL,APPLFLAGS
	CALL	NODE

	POP	ECX
	POP	EAX
	INC	AL			; NEXT PORT

	LOOP	ENABLEHOST

noports:

	RET


DOCOMMAND:
;
;	PROCESS NORMAL MODE COMMAND
;
;

}

		strcpy(Errbuff,	"BPQDED Normal Mode CMD ");
		strcat(Errbuff,LINEBUFFER);
		strcat(Errbuff,"\n");

		OutputDebugString(Errbuff);

_asm{

 	CMP	LINEBUFFER,1BH
	JNE	NOTCOMMAND		; DATA IN NORMAL MODE - IGNORE
;
;	IF ECHO ENABLED, ECHO IT
;
;	ECHO IF ENABLED
;
	CMP	ECHOFLAG,1
	JNE	NO_ECHO

	MOV	ESI,OFFSET LINEBUFFER

ECHOLOOP:

	LODSB
	CMP	AL,1BH
	JNE	NORMECHO

	MOV	AL,':'		; Echo esc as :

NORMECHO:

	CALL	PUTCHAR
	CMP	AL,0DH
	JNE	ECHOLOOP
;
NO_ECHO:

 	MOV	AL,LINEBUFFER+1
	AND	AL,0FFH-20H		; MASK TO LOWER CASE

	CMP	AL,'E'
	JE	ECHOCMD

	CMP	AL,'I'
	JE	SETCALLCMD

	CMP	AL,'J'
	JE	HOSTxx

	CMP	AL,'P'
	JNE	IGNORECOMMAND		; IGNORE OTHERS
;
;	PARAMS COMMAND - RETURN FIXED STRING
;
	MOV	AL,LINEBUFFER+2
	MOV	PARAMPORT,AL

	MOV	ESI,OFFSET PARAMREPLY
	MOV	ECX,LPARAMREPLY

PARAMLOOP:

	LODSB
	CALL	PUTCHAR

	LOOP	PARAMLOOP

	JMP	SHORT ENDCOMMAND

HOSTxx:

	MOV	AL,LINEBUFFER+6
	CMP	AL,0DH
	JNE	HOSTxxx

	MOV	AL,0			; DEFAULT TO OFF
HOSTxxx:
	AND	AL,1
	MOV	MODE,AL			; SET HOST MODE (PROBABLY OFF)

	cmp	 AL,0
	JE	ENDCOMMAND		; 	 
;
;	ENABLE SWITCH HOST MODE INTERFACE
;
	MOV	DL,BYTE PTR APPLMASK
	CALL	SETAPPLMASK

;	send host mode ack

	MOV	AL,0
	CALL	PUTCHAR
	
	MOV	AL,0
	CALL	PUTCHAR

	JMP	SHORT ENDCOMMAND

ECHOCMD:

	MOV	AL,LINEBUFFER+2
	AND	AL,1
	MOV	ECHOFLAG,AL

SETCALLCMD:
ENDCOMMAND:
NOTCOMMAND:
IGNORECOMMAND:

	MOV	CURSOR,OFFSET LINEBUFFER
	RET

PUTCHAR:

	PUSH	EBX

	MOV	EBX,PUTPTR
	MOV	[EBX],AL
	INC	EBX
	CMP	EBX,OFFSET RXBUFFER+512
	JNE	PUTRET

	MOV	EBX,OFFSET RXBUFFER
PUTRET:
	MOV	PUTPTR,EBX

	INC	RXCOUNT

	POP	EBX
	RET


GETVALUE:
;
;	EXTRACT NUMBER (HEX OR DECIMAL) FROM INPUT STRING
;
	MOV	NEWVALUE,0

	LODSB
	CMP	AL,'$'			; HEX?
	JE	DECODEHEX
	DEC	ESI
VALLOOP:
	LODSB
	CMP	AL,' '
	JE	ENDVALUE
	CMP	AL,0DH
	JE	ENDVALUE
	CMP	AL,','
	JE	ENDVALUE
;
;	ANOTHER DIGIT - MULTIPLY BY 10
;
	MOV	AX,NEWVALUE
	MUL	D10
	MOV	NEWVALUE,AX

	DEC	ESI
	LODSB
	SUB	al,'0'
	JC	DUFFVALUE
	CMP	AL,10
	JNC	DUFFVALUE

	MOV	AH,0
	ADD	NEWVALUE,AX

	JC	DUFFVALUE
	JMP	VALLOOP

DECODEHEX:
HEXLOOP:
	LODSB
	CMP	AL,' '
	JE	ENDVALUE
	CMP	AL,0DH
	JE	ENDVALUE
	CMP	AL,','
	JE	ENDVALUE
;
;	ANOTHER DIGIT - MULTIPLY BY 16
;
	MOV	AX,NEWVALUE
	MUL	D16
	MOV	NEWVALUE,AX

	DEC	ESI
	LODSB
	SUB	al,'0'
	JC	DUFFVALUE
	CMP	AL,10
	JC	HEXOK
	SUB	AL,7
	CMP	AL,10
	JC	DUFFVALUE

	CMP	AL,16
	JNC	DUFFVALUE
HEXOK:
	MOV	AH,0
	ADD	NEWVALUE,AX
	JMP	HEXLOOP

ENDVALUE:
	CLC
	RET

DUFFVALUE:
	STC
	RET

NODE:

	MOV	alsave,AL
	MOV	ahsave,AH
	
	push	eax
	and	eax,0xff

	MOV	EBX,OFFSET bpqstream
	SAL	EAX,2			;	* 4
	ADD	EBX,EAX

	pop	eax

	mov	AL,[ebx]

	or	al,al
	jnz xx

	pushad

	 }

	_itoa(alsave,buff,10);
	strcpy(Errbuff,	"BPQDED32 NODE called with invalid stream ");
	strcat(Errbuff,buff);
	_itoa(ahsave,buff,10);
	strcat(Errbuff," Function ");
	strcat(Errbuff,buff);
	OutputDebugString(Errbuff);
	
	_asm {

	popad
		
	ret						; ignore

xx:


INTVAL:

	nop
	call BPQAPI

	RET

RELBUFF:

	MOV	ESI,OFFSET FREE_Q
	CALL	Q_ADDF			; RETURN BUFFER TO FREE QUEUE
	RET

COPYMSGTOBUFFERS:
;
;	COPIES MESSAGE IN ESI, LENGTH ECX TO A CHAIN OF BUFFERS
;
;	RETURNS EDI = FIRST (OR ONLY) FRAGMENT
;
	PUSH	ESI			; SAVE MSG

	CALL	GETBUFF			; GET FIRST
	JNZ	BUFFEROK1		; NONE - SHOULD NEVER HAPPEN

	POP		ESI
	MOV	EDI,0			; INDICATE NO BUFFER
	
	RET

BUFFEROK1:

	POP	ESI			; RECOVER DATA

	MOV	EDX,EDI			; SAVE FIRST BUFFER

	MOV	4[EDI],ECX		; SAVE LENGTH
	ADD	EDI,8

	CMP	ECX,BUFFLEN-12	; MAX DATA IN FIRST
	JA	NEEDCHAIN
;
;	IT WILL ALL FIT IN ONE BUFFER
;
COPYLASTBIT:

	REP MOVSB

	MOV	EDI,EDX			; FIRST BUFFER
	
	RET

NEEDCHAIN:

	PUSH	ECX
	MOV		ECX,BUFFLEN-12
	REP MOVSB			; COPY FIRST CHUNK
	POP	ECX
	SUB	ECX,BUFFLEN-12
;
;	EDI NOW POINTS TO CHAIN WORD OF BUFFER
;
COPYMSGLOOP:
;
;	GET ANOTHER BUFFER
;
	PUSH	ESI			; MESSAGE 
	PUSH	EDI			; FIRST BUFFER CHAIN WORD

	CALL	GETBUFF
	JNZ	BUFFEROK2		; NONE - SHOULD NEVER HAPPEN

	POP		ESI			; PREVIOUS BUFFER
	POP		ESI
	MOV	EDI,0			; INDICATE NO BUFFER
	
	RET

BUFFEROK2:

	POP	ESI			; PREVIOUS BUFFER
	MOV	[ESI],EDI			; CHAIN NEW BUFFER

	POP	ESI			; MESSAGE

	CMP	ECX,BUFFLEN-4		; MAX DATA IN REST
	JBE	COPYLASTBIT

	PUSH	ECX
	MOV	ECX,BUFFLEN-4
	REP MOVSB			; COPY FIRST CHUNK
	POP	ECX
	SUB	ECX,BUFFLEN-4
;
	JMP	COPYMSGLOOP

COPYBUFFERSTOSTRING:
;
;	UNPACKS CHAIN OF BUFFERS IN SI TO DI
;
;
	MOV	ECX,4[ESI]		; LENGTH

	MOV	EDX,ESI

	ADD	ESI,8

	CMP	ECX,BUFFLEN-12
	JA	MORETHANONE
;
;	ITS ALL IN ONE BUFFER
;
	REP MOVSB

	MOV	EDI,EDX			; BUFFER
	CALL	RELBUFF

	RET

MORETHANONE:

	PUSH	ECX
	MOV	ECX,BUFFLEN-12
	REP MOVSB

	POP	ECX
	SUB	ECX,BUFFLEN-12

UNCHAINLOOP:

	PUSH	EDI			; SAVE TARGET

	MOV	EDI,EDX			; OLD BUFFER

	MOV	EDX,[ESI]			; NEXT BUFFER

	CALL	RELBUFF

	POP	EDI
	MOV	ESI,EDX

	CMP	ECX,BUFFLEN-4
	JBE	LASTONE


	PUSH	ECX
	MOV	ECX,BUFFLEN-4
	REP MOVSB
	POP	ECX
	SUB	ECX,BUFFLEN-4

	JMP	UNCHAINLOOP

LASTONE:

	REP MOVSB

	MOV	EDI,EDX
	CALL	RELBUFF

	RET

Q_REM:

	MOV	EDI,DWORD PTR[ESI]		; GET ADDR OF FIRST BUFFER
	CMP	EDI,0
	JE	Q_RET					; EMPTY

	MOV	EAX,DWORD PTR [EDI]			; CHAIN FROM BUFFER
	MOV	[ESI],EAX			; STORE IN HEADER

Q_RET:
	RET

;               
Q_ADD:
Q_ADD05:
	CMP	DWORD PTR [ESI],0		; END OF CHAIN
	JE	Q_ADD10

	MOV	ESI,DWORD PTR [ESI]		; NEXT IN CHAIN
	JMP	Q_ADD05
Q_ADD10:
	MOV	DWORD PTR [EDI],0		; CLEAR CHAIN ON NEW BUFFER
	MOV	[ESI],EDI			; CHAIN ON NEW BUFFER

	RET

;
;	ADD TO FRONT OF QUEUE - MUST ONLY BE USED FOR FREE QUEUE
;
Q_ADDF:

	MOV	EAX,DWORD PTR[ESI]	; OLD FIRST IN CHAIN
	MOV	[EDI],EAX
	MOV	[ESI],EDI			; PUT NEW ON FRONT

	INC	QCOUNT
	RET

GETBUFF:

	MOV	ESI,OFFSET FREE_Q
	CALL	Q_REM
;
	JZ	NOBUFFS

	DEC	QCOUNT
	MOV	EAX,QCOUNT
	CMP	EAX,MINBUFFCOUNT
	JA	GETBUFFRET
	MOV	MINBUFFCOUNT,EAX

GETBUFFRET:

	OR	AL,1			; SET NZ
 	RET

NOBUFFS:

	INC	NOBUFFCOUNT
	XOR	AL,AL
	RET


CONV_DIGITS:

	MOV	AH,0

CONV_5DIGITS:

	PUSH	EDX
	PUSH	EBX

	MOV	EBX,OFFSET TENK		; 10000

	CMP	AX,10
	JB	UNITS			; SHORT CUT AND TO STOP LOOP

START_LOOP:

	cmp	AX,WORD PTR [EBX]
	JAE	STARTCONV

	ADD	EBX,2

	JMP SHORT START_LOOP
;
STARTCONV:

	MOV	DX,0
	DIV	WORD PTR [EBX]		; 
	ADD	AL,30H			; MUST BE LESS THAN 10
	CALL	PUTCHAR
;
	MOV	AX,DX			; REMAINDER

	ADD	EBX,2
	CMP	EBX,OFFSET TENK+8	; JUST DIVIDED BY 10?
	JNE	STARTCONV		; NO, SO ANOTHER TO DO
;
;	REST MUST BE UNITS
;
UNITS:

	add	AL,30H
	CALL	PUTCHAR

	POP	EBX
	POP	EDX

	ret


MONPUTCHAR:

	PUSH	EDI
	MOV	EDI,MONCURSOR
	STOSB
	MOV	MONCURSOR,EDI
	POP	EDI

	RET

DISPLAYFRAME:

/*

    From DEDHOST Documentation
	
	  
Codes of 4 and 5 both signify a monitor header.  This  is  a  null-terminated
format message containing the

    fm {call} to {call} via {digipeaters} ctl {name} pid {hex}

string  that  forms  a monitor header.  The monitor header is also identical to
the monitor header displayed in user mode.  If the code was  4,  the  monitored
frame contained no information field, so the monitor header is all you get.  If
you monitor KB6C responding to a connect request from me and then poll  channel
0, you'll get:

    0004666D204B42364320746F204B42354D552063746C2055612070494420463000
    ! ! f m   K B 6 C   t o   K B 5 M U   c t l   U A   p i d   F 0 !
    ! !                                                             !
    ! +---- Code = 4 (Monitor, no info)        Null termination ----+
    +------- Channel = 0 (Monitor info is always on channel 0)

  If  the code was 5, the monitored frame did contain an information field.  In
this case, another G command to channel 0 will return the monitored information
with  a code of 6.  Since data transmissions must be transparent, the monitored
information is passed as a byte-count format transmission.    That  is,  it  is
preceded  by a count byte (one less than the number of bytes in the field).  No
null terminator is used in this case.  Since codes  4,  5,  and  6  pertain  to
monitored  information,  they will be seen only on channel 0.  If you hear KB6C
say "Hi" to NK6K, and then poll channel 0, you'll get:

    0005666D204B42364320746F204E4B364B2063746C204930302070494420463000
    ! ! f m   K B 6 C   t o   N K 6 K   c t l   I 0 0   p i d   F 0 !
    ! !                                           ! !               !
    ! !                           or whatever ----+-+               !
    ! !                                                             !
    ! +---- Code = 5 (Monitor, info follows)   Null termination ----+
    +------ Channel = 0 (Monitor info is always on channel 0)

and then the very next poll to channel 0 will get:

         00 06 02 48 69 0D
         !  !  !  H  i  CR
         !  !  !        !
         !  !  !        +----    (this is a data byte)
         !  !  +---- Count = 2   (three bytes of data)
         !  +------- Code = 6    (monitored information)
         +---------- Channel = 0 (Monitor info is always on channel 0)

*/


	MOV	MONHEADER,4		; NO DATA FOLLOWS
	MOV	MONCURSOR,OFFSET MONHEADER+1
;
;	GET THE CONTROL BYTE, TO SEE IF THIS FRAME IS TO BE DISPLAYED
;
	PUSH	EDI
	MOV	ECX,8			; MAX DIGIS
CTRLLOOP:
	TEST	BYTE PTR (MSGCONTROL-1)[EDI],1
	JNZ	CTRLFOUND

	ADD	EDI,7
	LOOP	CTRLLOOP
;
;	INVALID FRAME
;
	POP	EDI
	RET

CTRLFOUND:
	MOV	AL,MSGCONTROL[EDI]

	and	AL,NOT PFBIT		; Remove P/F bit
	mov	FRAME_TYPE,AL

	
	POP	EDI
;
	TEST	AL,1			; I FRAME
	JZ	IFRAME

	CMP	AL,3			; UI
	JE	OKTOTRACE		; ALWAYS DO UI

	CMP	AL,FRMR
	JE	OKTOTRACE		; ALWAYS DO FRMR
;
;	USEQ/CONTROL - TRACE IF MCOM ON
;
	CMP	MCOM,0
	JNE	OKTOTRACE

	RET

;-----------------------------------------------------------------------------;
;       Check for MALL                                                        ;
;-----------------------------------------------------------------------------;

IFRAME:
	cmp	MALL,0
	jne	OKTOTRACE

	ret

OKTOTRACE:
;
;-----------------------------------------------------------------------------;
;       Get the port number of the received frame                             ;
;-----------------------------------------------------------------------------;
;
;	CHECK FOR PORT SELECTIVE MONITORING
;
	MOV	CL,MSGPORT[EDI]
	mov	PORT_NO,CL

	DEC	CL
	MOV	EAX,1
	SHL	EAX,CL			; SHIFT BIT UP

	TEST	MMASK,EAX
	JNZ	TRACEOK1

	RET

TRACEOK1:

	MOV	FRMRFLAG,0
	push	EDI
	mov	AH,MSGDEST+6[EDI]
	mov	AL,MSGORIGIN+6[EDI]

;
;       Display Origin Callsign                                               ;
;

;    0004666D204B42364320746F204B42354D552063746C2055612070494420463000
;    ! ! f m   K B 6 C   t o   K B 5 M U   c t l   U A   p i d   F 0 !
;    ! !                                                             !
;    ! +---- Code = 4 (Monitor, no info)        Null termination ----+
 ;   +------- Channel = 0 (Monitor info is always on channel 0)

	mov	ESI,OFFSET FM_MSG
	call	NORMSTR

	lea	ESI,MSGORIGIN[EDI]
	call	CONVFROMAX25
	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	pop	EDI
	push	EDI

	mov	ESI,OFFSET TO_MSG
	call	NORMSTR

;
;       Display Destination Callsign                                          ;
;

	lea	ESI,MSGDEST[EDI]
	call	CONVFROMAX25
	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	pop	EDI
	push	EDI

	mov	AX,MMSGLENGTH[EDI]
	mov	FRAME_LENGTH,AX
	mov	ECX,8			; Max number of digi-peaters

;
;       Display any Digi-Peaters                                              ;
;

NEXT_DIGI:
	test	MSGORIGIN+6[EDI],1
	jnz	NO_MORE_DIGIS

	add	EDI,7
	sub	FRAME_LENGTH,7		; Reduce length

	push	EDI
	push	ECX
	lea	ESI,MSGORIGIN[EDI]
	call	CONVFROMAX25		; Convert to call

	push	EAX			; Last byte is in AH

	mov	AL,','
	call	MONPUTCHAR

	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	pop	EAX

	test	AH,80H
	jz	NOT_REPEATED

	mov	AL,'*'
	call	MONPUTCHAR

NOT_REPEATED:
	pop	ECX
	pop	EDI
	loop	NEXT_DIGI

NO_MORE_DIGIS:	

;----------------------------------------------------------------------------;
;       Display ctl                                    ;
;----------------------------------------------------------------------------;

	mov	ESI,OFFSET CTL_MSG
	call	NORMSTR

;-----------------------------------------------------------------------------;
;       Start displaying the frame information                                ;
;-----------------------------------------------------------------------------;


	mov	INFO_FLAG,0

	mov	AL,FRAME_TYPE

	test	AL,1
	jne	NOT_I_FRAME

;-----------------------------------------------------------------------------;
;       Information frame                                                     ;
;-----------------------------------------------------------------------------;

	mov	AL,'I'
	call	MONPUTCHAR
	mov	INFO_FLAG,1

	mov	ESI,OFFSET I_MSG
	call	NORMSTR

	lea	ESI,MSGPID[EDI]
	lodsb

	call BYTE_TO_HEX
	

	jmp	END_OF_TYPE

NOT_I_FRAME:

;-----------------------------------------------------------------------------;
;       Un-numbered Information Frame                                         ;
;-----------------------------------------------------------------------------;

	cmp	AL,UI
	jne	NOT_UI_FRAME

	mov	ESI,OFFSET UI_MSG
	call	NORMSTR

	lea	ESI,MSGPID[EDI]
	lodsb

	call BYTE_TO_HEX
	
	mov	INFO_FLAG,1
	jmp	END_OF_TYPE

NOT_UI_FRAME:
	test	AL,10B
	jne	NOT_R_FRAME

;-----------------------------------------------------------------------------;
;       Process supervisory frames                                            ;
;-----------------------------------------------------------------------------;


	and	AL,0FH			; Mask the interesting bits
	cmp	AL,RR	
	jne	NOT_RR_FRAME

	mov	ESI,OFFSET RR_MSG
	call	NORMSTR
	jmp	END_OF_TYPE

NOT_RR_FRAME:
	cmp	AL,RNR
	jne	NOT_RNR_FRAME

	mov	ESI,OFFSET RNR_MSG
	call	NORMSTR
	jmp END_OF_TYPE

NOT_RNR_FRAME:
	cmp	AL,REJ
	jne	NOT_REJ_FRAME

	mov	ESI,OFFSET REJ_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_REJ_FRAME:
	mov	AL,'?'			; Print "?"
	call	MONPUTCHAR
	jmp	SHORT END_OF_TYPE

;
;       Process all other frame types                                         ;
;

NOT_R_FRAME:
	cmp	AL,UA
	jne	NOT_UA_FRAME

	mov	ESI,OFFSET UA_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_UA_FRAME:
	cmp	AL,DM
	jne	NOT_DM_FRAME

	mov	ESI,OFFSET DM_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_DM_FRAME:
	cmp	AL,SABM
	jne	NOT_SABM_FRAME

	mov	ESI,OFFSET SABM_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_SABM_FRAME:
	cmp	AL,DISC
	jne	NOT_DISC_FRAME

	mov	ESI,OFFSET DISC_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_DISC_FRAME:
	cmp	AL,FRMR
	jne	NOT_FRMR_FRAME

	mov	ESI,OFFSET FRMR_MSG
	call	NORMSTR
	MOV	FRMRFLAG,1
	jmp	SHORT END_OF_TYPE

NOT_FRMR_FRAME:
	mov	AL,'?'
	call	MONPUTCHAR

END_OF_TYPE:

	CMP	FRMRFLAG,0
	JE	NOTFRMR
;
;	DISPLAY FRMR BYTES
;
	lea	ESI,MSGPID[EDI]
	MOV	CX,3			; TESTING
FRMRLOOP:
	lodsb
	CALL	BYTE_TO_HEX

	LOOP	FRMRLOOP

	JMP	NO_INFO

NOTFRMR:

	MOVZX	ECX,FRAME_LENGTH


	cmp	INFO_FLAG,1		; Is it an information packet ?
	jne	NO_INFO


	XOR	AL,AL			; IN CASE EMPTY

	sub	ECX,23
	JCXZ	NO_INFO			; EMPTY I FRAME
;
;	PUT DATA IN MONBUFFER, LENGTH IN MONLENGTH
;

	MOV	MONFLAG,1
	MOV	MONHEADER,5		; DATA FOLLOWS

	cmp	ECX,257
	jb	LENGTH_OK
;
	mov	ECX,256
;
LENGTH_OK:
;
	MOV	MONBUFFER+1,CL
	DEC	MONBUFFER+1

	MOV	MONLENGTH,ECX
	ADD	MONLENGTH,2

	MOV	EDI,OFFSET MONBUFFER+2

MONCOPY:
	LODSB
	CMP	AL,7			; REMOVE BELL
	JNE	MONC00

	MOV	AL,20H
MONC00:
	STOSB

	LOOP	MONCOPY

	POP	EDI
	RET

NO_INFO:
;
;	ADD CR UNLESS DATA ALREADY HAS ONE
;
	CMP	AL,CR
	JE	NOTANOTHER

	mov	AL,CR
	call	MONPUTCHAR

NOTANOTHER:
;
	pop	EDI
	ret

;----------------------------------------------------------------------------;
;       Display ASCIIZ strings                                               ;
;----------------------------------------------------------------------------;

NORMSTR:
	lodsb
	cmp	AL,0		; End of String ?
	je	NORMSTR_RET	; Yes
	call	MONPUTCHAR
	jmp	SHORT NORMSTR

NORMSTR_RET:
	ret

;-----------------------------------------------------------------------------;
;       Display Callsign pointed to by SI                                     ;
;-----------------------------------------------------------------------------;

DISPADDR:
	jcxz	DISPADDR_RET

	lodsb
	call	MONPUTCHAR

	loop	DISPADDR

DISPADDR_RET:
	ret


;-----------------------------------------------------------------------------;
;       Convert byte in AL to nn format                                       ;
;-----------------------------------------------------------------------------;

DISPLAY_BYTE_2:
	cmp	AL,100
	jb	TENS_2

	sub	AL,100
	jmp	SHORT DISPLAY_BYTE_2

TENS_2:
	mov	AH,0

TENS_LOOP_2:
	cmp	AL,10
	jb	TENS_LOOP_END_2

	sub	AL,10
	inc	AH
	jmp	SHORT TENS_LOOP_2

TENS_LOOP_END_2:
	push	EAX
	mov	AL,AH
	add	AL,30H
	call	MONPUTCHAR
	pop	EAX

	add	AL,30H
	call	MONPUTCHAR

	ret

;-----------------------------------------------------------------------------;
;       Convert byte in AL to Hex display                                     ;
;-----------------------------------------------------------------------------;

BYTE_TO_HEX:
	push	EAX
	shr	AL,1
	shr	AL,1
	shr	AL,1
	shr	AL,1
	call	NIBBLE_TO_HEX
	pop	EAX
	call	NIBBLE_TO_HEX
	ret

NIBBLE_TO_HEX:
	and	AL,0FH
	cmp	AL,10

	jb	LESS_THAN_10
	add	AL,7

LESS_THAN_10:
	add	AL,30H
	call	MONPUTCHAR
	ret



CONVFROMAX25:
;
;	CONVERT AX25 FORMAT CALL IN [SI] TO NORMAL FORMAT IN NORMCALL
;	   RETURNS LENGTH IN CX AND NZ IF LAST ADDRESS BIT IS SET
;
	PUSH	ESI			; SAVE
	MOV	EDI,OFFSET NORMCALL
	MOV	ECX,10			; MAX ALPHANUMERICS
	MOV	AL,20H
	REP STOSB			; CLEAR IN CASE SHORT CALL
	MOV	EDI,OFFSET NORMCALL
	MOV	CL,6
CONVAX50:
	LODSB
	CMP	AL,40H
	JE	CONVAX60		; END IF CALL - DO SSID

	SHR	AL,1
	STOSB
	LOOP	CONVAX50
CONVAX60:
	POP	ESI
	ADD	ESI,6			; TO SSID
	LODSB
	MOV	AH,AL			; SAVE FOR LAST BIT TEST
	SHR	AL,1
	AND	AL,0FH
	JZ	CONVAX90		; NO SSID - FINISHED
;
	MOV	BYTE PTR [EDI],'-'
	INC	EDI
	CMP	AL,10
	JB	CONVAX70
	SUB	AL,10
	MOV	BYTE PTR [EDI],'1'
	INC	EDI
CONVAX70:
	ADD	AL,30H			; CONVERT TO DIGIT
	STOSB
CONVAX90:
	MOV	ECX,EDI
	SUB	ECX,OFFSET NORMCALL
	MOV	NORMLEN,ECX		; SIGNIFICANT LENGTH

	TEST	AH,1			; LAST BIT SET?
	RET

	}
}

