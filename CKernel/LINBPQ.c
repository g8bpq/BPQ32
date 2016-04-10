// LINBPQ.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#include "CHeaders.h"

#include <stdlib.h>
#include <time.h>

#define CKernel

BOOL IncludesMail = FALSE;

VOID GetSemaphore();
VOID FreeSemaphore();
VOID TIMERINTERRUPT();

BOOL Start();
VOID INITIALISEPORTS();
Dll BOOL APIENTRY Init_APRS();
VOID APRSClose();
Dll VOID APIENTRY Poll_APRS();
VOID HTTPTimer();
Dll VOID APIENTRY md5 (char *arg, unsigned char * checksum);
#include "Versions.h"

int SemHeldByAPI = 0;
BOOL IGateEnabled = TRUE;
BOOL APRSActive = FALSE;
BOOL ReconfigFlag;

int InitDone;
char pgm[256] = "Dummy";		// Uninitialised so per process

struct SEM Semaphore = {0, 0};

char SESSIONHDDR[80] = "";
int SESSHDDRLEN = 0;

BOOL KEEPGOING = TRUE;

BPQVECSTRUC * BPQHOSTVECPTR;

// Next 3 should be uninitialised so they are local to each process

UCHAR MCOM;
UCHAR MUIONLY;
UCHAR MTX;
ULONG MMASK;


UCHAR AuthorisedProgram;			// Local Variable. Set if Program is on secure list

int SAVEPORT = 0;

VOID SetApplPorts();

char VersionString[50] = Verstring;
char VersionStringWithBuild[50]=Verstring;
int Ver[4] = {Vers};
char TextVerstring[50] = Verstring;

extern UCHAR PWLEN;
extern char PWTEXT[];
extern int ISPort;

#ifdef WIN32

BOOL CtrlHandler(DWORD fdwCtrlType) 
{ 
  switch( fdwCtrlType ) 
  { 
    // Handle the CTRL-C signal. 
    case CTRL_C_EVENT: 
      printf( "Ctrl-C event\n\n" );
	  KEEPGOING = 0;
      Beep( 750, 300 ); 
      return( TRUE );
 
    // CTRL-CLOSE: confirm that the user wants to exit. 
    case CTRL_CLOSE_EVENT: 
      Beep( 600, 200 ); 
      printf( "Ctrl-Close event\n\n" );
      return( TRUE ); 
 
    // Pass other signals to the next handler. 
    case CTRL_BREAK_EVENT: 
      Beep( 900, 200 ); 
      printf( "Ctrl-Break event\n\n" );
      return FALSE; 
 
    case CTRL_LOGOFF_EVENT: 
      Beep( 1000, 200 ); 
      printf( "Ctrl-Logoff event\n\n" );
      return FALSE; 
 
    case CTRL_SHUTDOWN_EVENT: 
      Beep( 750, 500 ); 
      printf( "Ctrl-Shutdown event\n\n" );
      return FALSE; 
 
    default: 
      return FALSE; 
  } 
} 

#else

// Linux Signal Handlers

static void sigterm_handler(int sig)
{
	syslog(LOG_INFO, "terminating on SIGTERM\n");
	KEEPGOING = 0;
}

static void sigusr1_handler(int sig)
{
	signal(SIGUSR1, sigusr1_handler);
	ReconfigFlag = TRUE;
}

#endif


int main(int argc, char * argv[])
{
	int i;
#ifdef WIN32
	WSADATA       WsaData;            // receives data from WSAStartup

	WSAStartup(MAKEWORD(2, 0), &WsaData);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE); 

#else
		openlog("LINBPQ", LOG_PID, LOG_DAEMON);
#endif

	printf("G8BPQ AX25 Packet Switch System Version %s %s\n", TextVerstring, Datestring);
	printf("%s\n", VerCopyright);




#ifdef WIN32
	GetCurrentDirectory(256, BPQDirectory);
#else
	getcwd(BPQDirectory, 256);
#endif
	Consoleprintf("Current Directory is %s\n", BPQDirectory);

	if (!ProcessConfig())
	{
			WritetoConsoleLocal("Configuration File Error\n");
			return (0);
	}
				 
	SESSHDDRLEN = sprintf(SESSIONHDDR, "G8BPQ Network System %s for Linux (", TextVerstring);

	GetSemaphore(&Semaphore);

	if (Start() != 0)
	{
		FreeSemaphore(&Semaphore);
		return (0);
	}

	for (i=0;PWTEXT[i] > 0x20;i++); //Scan for cr or null 

	PWLEN=i;

	SetApplPorts();

	INITIALISEPORTS();

	APRSActive = Init_APRS();

	if (ISPort == 0)
		IGateEnabled = 0;

	RigActive = Rig_Init();
	FreeSemaphore(&Semaphore);

	InitDone = TRUE;

#ifndef WIN32

	if (argc > 1 && stricmp(argv[1], "daemon") == 0)
	{

		// Set up Linux Signal Handlers, and if requested deamonize

		signal(SIGHUP, SIG_IGN);
		signal(SIGUSR1, sigusr1_handler);
		signal(SIGTERM, sigterm_handler);

		// Convert to daemon
		
		pid_t pid, sid;
        
	    /* Fork off the parent process */
		pid = fork();
	   
		if (pid < 0)
	        exit(EXIT_FAILURE);
        
		if (pid > 0)
			exit(EXIT_SUCCESS);

		/* Change the file mode mask */
        
		umask(0);       
        
		/* Create a new SID for the child process */
        
		sid = setsid();
        
		if (sid < 0)
			exit(EXIT_FAILURE);
	    
	    /* Change the current working directory */
        
		if ((chdir("/")) < 0)          
			exit(EXIT_FAILURE);
		
		/* Close out the standard file descriptors */
    
		printf("Entering daemon mode\n");

		close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
	}
#endif

	while (KEEPGOING)
	{
		Sleep(100);
		GetSemaphore(&Semaphore);
		
		if (ReconfigFlag)
		{
			int i;
			BPQVECSTRUC * HOSTVEC;
			PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;

			ReconfigFlag = FALSE;

//			SetupBPQDirectory();

			WritetoConsoleLocal("Reconfiguring ...\n\n");
			OutputDebugString("BPQ32 Reconfiguring ...\n");	


			for (i=0;i<NUMBEROFPORTS;i++)
			{
				if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
				{
					if (PORTVEC->PORT_EXT_ADDR)
					{
//						SaveWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
//						SaveAXIPWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
//						CloseDriverWindow(PORTVEC->PORTCONTROL.PORTNUMBER);
						PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);	// Close External Ports
					}
				}
				PORTVEC->PORTCONTROL.PORTCLOSECODE(&PORTVEC->PORTCONTROL);
				PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
			}

//			IPClose();
			APRSClose();
			Rig_Close();

//			Sleep(2000);

			Consoleprintf("G8BPQ AX25 Packet Switch System Version %s %s", TextVerstring, Datestring);
			Consoleprintf(VerCopyright);

			Start();

			INITIALISEPORTS();
	
			SetApplPorts();

			FreeConfig();

			for (i=1;i<66;i++)			// Include IP Vec
			{
				HOSTVEC=&BPQHOSTVECTOR[i-1];

				HOSTVEC->HOSTTRACEQ=0;

				if (HOSTVEC->HOSTSESSION !=0)
				{
					// Had a connection
					
					HOSTVEC->HOSTSESSION=0;
					HOSTVEC->HOSTFLAGS |=3;	// Disconnected
					
//					PostMessage(HOSTVEC->HOSTHANDLE, BPQMsg, i, 4);
				}
			}

	//		OpenReportingSockets();
		
			WritetoConsoleLocal("\n\nReconfiguration Complete\n");

//			if (IPRequired)	IPActive = Init_IP();
//
			APRSActive = Init_APRS();

			if (ISPort == 0)
				IGateEnabled = 0;

			RigActive = Rig_Init();
			
			OutputDebugString("BPQ32 Reconfiguration Complete\n");	
		}

//		if (IPActive) Poll_IP();
		if (RigActive) Rig_Poll();
		if (APRSActive) Poll_APRS();
		CheckWL2KReportTimer();

		TIMERINTERRUPT();
		FreeSemaphore(&Semaphore);

		HTTPTimer();
	}

	if (AUTOSAVE)
		SaveNodes();


	return 0;
}


struct TCPINFO;

void ProcessMailHTTPMessage(struct TCPINFO * TCP, char * Method, char * URL, char * input, char * Reply, int * RLen)
{
	*RLen=0;
}

int APIENTRY WritetoConsole(char * buff)
{
	return WritetoConsoleLocal(buff);
}

int WritetoConsoleLocal(char * buff)
{
	return printf("%s", buff);
}

VOID Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}

/*
UINT VCOMExtInit(struct PORTCONTROL *  PortEntry);

UINT SCSExtInit(struct PORTCONTROL *  PortEntry);
UINT AEAExtInit(struct PORTCONTROL *  PortEntry);
UINT KAMExtInit(struct PORTCONTROL *  PortEntry);
UINT HALExtInit(struct PORTCONTROL *  PortEntry);
UINT AGWExtInit(struct PORTCONTROL *  PortEntry);
UINT WinmorExtInit(EXTPORTDATA * PortEntry);
UINT SoundModemExtInit(EXTPORTDATA * PortEntry);
UINT TrackerExtInit(EXTPORTDATA * PortEntry);
UINT TrackerMExtInit(EXTPORTDATA * PortEntry);
UINT V4ExtInit(EXTPORTDATA * PortEntry);
UINT MPSKExtInit(EXTPORTDATA * PortEntry);
UINT BaycomExtInit(EXTPORTDATA * PortEntry);
*/
UINT TelnetExtInit(EXTPORTDATA * PortEntry);
UINT UZ7HOExtInit(EXTPORTDATA * PortEntry);
UINT ETHERExtInit(struct PORTCONTROL *  PortEntry);
UINT AXIPExtInit(struct PORTCONTROL *  PortEntry);

UINT InitializeExtDriver(PEXTPORTDATA PORTVEC)
{
	// Only works with built in drivers

	UCHAR Value[20];
	
	strcpy(Value,PORTVEC->PORT_DLL_NAME);

	_strupr(Value);

	if (strstr(Value, "BPQETHER"))
		return (UINT) ETHERExtInit;

	if (strstr(Value, "BPQAXIP"))
		return (UINT) AXIPExtInit;

/*
	if (strstr(Value, "BPQVKISS"))
		return (UINT) VCOMExtInit;


	if (strstr(Value, "BPQTOAGW"))
		return (UINT) AGWExtInit;

	if (strstr(Value, "AEAPACTOR"))
		return (UINT) AEAExtInit;

	if (strstr(Value, "HALDRIVER"))
		return (UINT) HALExtInit;

	if (strstr(Value, "KAMPACTOR"))
		return (UINT) KAMExtInit;

	if (strstr(Value, "SCSPACTOR"))
		return (UINT) SCSExtInit;

	if (strstr(Value, "WINMOR"))
		return (UINT) WinmorExtInit;
	
	if (strstr(Value, "V4"))
		return (UINT) V4ExtInit;
	
	if (strstr(Value, "SOUNDMODEM"))
		return (UINT) SoundModemExtInit;

	if (strstr(Value, "SCSTRACKER"))
		return (UINT) TrackerExtInit;

	if (strstr(Value, "TRKMULTI"))
		return (UINT) TrackerMExtInit;

	if (strstr(Value, "MULTIPSK"))
		return (UINT) MPSKExtInit;

	if (strstr(Value, "BAYCOM"))
		return (UINT) BaycomExtInit;
*/
	if (strstr(Value, "UZ7HO"))
		return (UINT) UZ7HOExtInit;

	if (strstr(Value, "TELNET"))
		return (UINT) TelnetExtInit;


	return(0);
}

int APIENTRY Restart()
{
	KEEPGOING = 0;
	return TRUE;
}

int APIENTRY Reboot()
{
	// Run shutdown -r -f
#ifndef LINBPQ
	STARTUPINFO  SInfo;
    PROCESS_INFORMATION PInfo;
	char Cmd[] = "shutdown -r -f";


	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
  	SInfo.lpReserved2=NULL; 

	return CreateProcess(NULL, Cmd, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo);
#endif
	return 0;
}

int APIENTRY Reconfig()
{
	if (!ProcessConfig())
	{
		return (0);
	}
	SaveNodes();
	WritetoConsoleLocal("Nodes Saved\n");
	ReconfigFlag=TRUE;	
	WritetoConsoleLocal("Reconfig requested ... Waiting for Timer Poll\n");
	return 1;
}

VOID MonitorAPRSIS(char * Msg, int MsgLen, BOOL TX)
{
}

struct TNCINFO * TNC;

#ifndef WIN32
int GetTickCount()
{
    struct timespec start;
 
    if (clock_gettime(CLOCK_REALTIME, &start) == -1 ) {
      perror( "clock gettime" );
      return 0;
    }

	return (start.tv_sec * 1000 + start.tv_nsec /1000000);
}
#endif

BOOL MySetWindowText(HWND hWnd, char * lpString)
{
	return 0;
};

BOOL MySetDlgItemText(HWND hWnd, char * lpString)
{
	return 0;
};

VOID Check_Timer()
{
}

VOID POSTDATAAVAIL(){};

COLORREF Colours[256] = {0,
		RGB(0,0,0), RGB(0,0,128), RGB(0,0,192), RGB(0,0,255),				// 1 - 4
		RGB(0,64,0), RGB(0,64,128), RGB(0,64,192), RGB(0,64,255),			// 5 - 8
		RGB(0,128,0), RGB(0,128,128), RGB(0,128,192), RGB(0,128,255),		// 9 - 12
		RGB(0,192,0), RGB(0,192,128), RGB(0,192,192), RGB(0,192,255),		// 13 - 16
		RGB(0,255,0), RGB(0,255,128), RGB(0,255,192), RGB(0,255,255),		// 17 - 20

		RGB(64,0,0), RGB(64,0,128), RGB(64,0,192), RGB(0,0,255),				// 21 
		RGB(64,64,0), RGB(64,64,128), RGB(64,64,192), RGB(64,64,255),
		RGB(64,128,0), RGB(64,128,128), RGB(64,128,192), RGB(64,128,255),
		RGB(64,192,0), RGB(64,192,128), RGB(64,192,192), RGB(64,192,255),
		RGB(64,255,0), RGB(64,255,128), RGB(64,255,192), RGB(64,255,255),

		RGB(128,0,0), RGB(128,0,128), RGB(128,0,192), RGB(128,0,255),				// 41
		RGB(128,64,0), RGB(128,64,128), RGB(128,64,192), RGB(128,64,255),
		RGB(128,128,0), RGB(128,128,128), RGB(128,128,192), RGB(128,128,255),
		RGB(128,192,0), RGB(128,192,128), RGB(128,192,192), RGB(128,192,255),
		RGB(128,255,0), RGB(128,255,128), RGB(128,255,192), RGB(128,255,255),

		RGB(192,0,0), RGB(192,0,128), RGB(192,0,192), RGB(192,0,255),				// 61
		RGB(192,64,0), RGB(192,64,128), RGB(192,64,192), RGB(192,64,255),
		RGB(192,128,0), RGB(192,128,128), RGB(192,128,192), RGB(192,128,255),
		RGB(192,192,0), RGB(192,192,128), RGB(192,192,192), RGB(192,192,255),
		RGB(192,255,0), RGB(192,255,128), RGB(192,255,192), RGB(192,255,255),

		RGB(255,0,0), RGB(255,0,128), RGB(255,0,192), RGB(255,0,255),				// 81
		RGB(255,64,0), RGB(255,64,128), RGB(255,64,192), RGB(255,64,255),
		RGB(255,128,0), RGB(255,128,128), RGB(255,128,192), RGB(255,128,255),
		RGB(255,192,0), RGB(255,192,128), RGB(255,192,192), RGB(255,192,255),
		RGB(255,255,0), RGB(255,255,128), RGB(255,255,192), RGB(255,255,255)
};

