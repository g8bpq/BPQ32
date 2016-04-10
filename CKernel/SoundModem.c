//
//	Interface to the Flexnet SoundModem Driver
//

//	Version 1.0 November 2010
//

#pragma data_seg("_BPQDATA")

#define WIN32_LEAN_AND_MEAN
#define _USE_32BIT_TIME_T
#define _CRT_SECURE_NO_DEPRECATE 

//#include <process.h>
//#include <time.h>

#define DllExport	__declspec(dllexport)

typedef unsigned char byte;

#include "CHeaders.h"
#include "bpq32.h"
#include "time.h"
#include <stdlib.h>
 
extern UCHAR BPQProgramDirectory[];

#pragma pack(1)

#define MAXFLEN 400     /* Maximale Laenge eines Frames */
                        /* maximum length of a frame */
typedef signed short    i16;
typedef unsigned char   byte;
typedef unsigned long   u32;


/* Struct f. Treiberkommunikation bei TX und RX */
/* struct for communicating RX and TX packets to the driver */
typedef struct
{
    i16  len;               /* Laenge des Frames - length of the frame */
    byte kanal;             /* Kanalnummer - channel number */
    byte txdelay;           /* RX: Gemessenes TxDelay [*10ms],
                                   0 wenn nicht unterstuetzt
                               TX: Zu sendendes TxDelay */
                            /* RX: measured transmitter keyup delay (TxDelay) in 10ms units,
                                   0 if not supported
                               TX: transmitter keyup delay (TxDelay) that should be sent */
    byte frame[MAXFLEN];    /* L1-Frame (ohne CRC) - L1 frame without CRC */
} L1FRAME;

typedef struct
{
    u32 tx_error;           /* Underrun oder anderes Problem - underrun or some other problem */
    u32 rx_overrun;         /* Wenn Hardware das unterstuetzt - if supported by the hardware */
    u32 rx_bufferoverflow;
    u32 tx_frames;          /* Gesamt gesendete Frames - total number of sent frames */
    u32 rx_frames;          /* Gesamt empfangene Frames - total number of received frames */
    u32 io_error;           /* Reset von IO-Device - number of resets of the IO device */
    u32 reserve[4];         /* f. Erweiterungen, erstmal 0 lassen! - reserved for extensions, leave 0! */
} L1_STATISTICS;

#pragma pack()

// Each Soundcard needs a separate instance of the BPQSoundModem Program

// Each can support more than one bpq port - for instance one 1200 and one 2400 on same radio

// There is one Modem Channel per BPQ Port


struct CHANNELINFO
{
	int Channel;
	struct PORTCONTROL * PORTVEC;
	int BPQPort;
	BOOL RXReady;
};

typedef struct SOUNDTNCINFO
{ 
	int SoundCardNumber;
	HWND hDlg;							// Status Window Handle
	struct CHANNELINFO Channellist[5];	// Max Channels
	int PERSIST;						// CSMA PERSIST
	int TXDELAY;
	int TXQ;
	int State;							// Channel State Flags
	int PID;
	int ProcessMonitor;					// Timer for checking that the modem is running
	BOOL WeStartedTNC;
};

// These must all be in BPQDATA seg so it can be accessed from other processes, must be initialised

struct SOUNDTNCINFO * PortToTNC[33] = {0};
int PortToChannel[33] = {0};

struct SOUNDTNCINFO SoundCardList[11] = {0};
static struct CHANNELINFO * Portlist[33] = {0};

UINT RXQ[33] = {0};				// Frames waiting for BPQ32

typedef UINT (FAR *FARPROCX)();

UINT (FAR *SoundModem)() = NULL;

HINSTANCE ExtDriver=0;
HANDLE hRXThread;

L1FRAME * rxf;


VOID __cdecl Debugprintf(const char * format, ...);
HANDLE _beginthread( void( *start_address )(), unsigned stack_size, int arglist);
static int ExtProc(int fn, int port, unsigned char * buff);
VOID * zalloc(int len);
KillSoundTNC(struct SOUNDTNCINFO * TNC);
RestartSoundTNC(struct SOUNDTNCINFO * TNC);


UINT SoundModemExtInit(struct PORTCONTROL *  PortEntry)
{
	char msg[80];
//	HKEY hKey=0;
//	int ret;
	int Port = PortEntry->PORTNUMBER;
	int Channel = PortEntry->CHANNELNUM - 65;
	int SoundCardNumber = PortEntry->IOBASE;
	struct SOUNDTNCINFO * SoundCard;
	struct CHANNELINFO * Chan;

	sprintf(msg,"Sound Modem Port %d", Port);
	WritetoConsole(msg);

	if (Channel > 4)
	{
		sprintf(msg,"Error - Channel must be A to E");
		WritetoConsole(msg);
		return ((UINT) ExtProc);
	}

	if (SoundCardNumber < 1 || SoundCardNumber > 10)
	{
		sprintf(msg,"Error - IOADDR must be 1 to 10");
		WritetoConsole(msg);
		return ((UINT) ExtProc);
	}

	// See if we already have a port on this soundcard (Address in IOBASE)

	SoundCard = &SoundCardList[SoundCardNumber];
	SoundCard->SoundCardNumber = SoundCardNumber;
	SoundCard->PERSIST = PortEntry->PORTPERSISTANCE;
	SoundCard->TXDELAY = PortEntry->PORTTXDELAY;

	Chan = &SoundCard->Channellist[Channel];

	Chan->PORTVEC = PortEntry;
	Chan->Channel = Channel;
	Chan->BPQPort = Port;

	PortToTNC[Port] = SoundCard;
	PortToChannel[Port] = Channel;

	Portlist[Port] = Chan;
	Debugprintf("Starting TNC %d", SoundCard->WeStartedTNC);

	if (SoundCard->WeStartedTNC == 0)
		SoundCard->WeStartedTNC = RestartSoundTNC(SoundCard);

	return ((UINT) ExtProc);
}


static int ExtProc(int fn, int port, unsigned char * buff)
{
	int len = 0, txlen=0;
	struct CHANNELINFO * PORT = Portlist[port];
	struct PORTCONTROL * PORTVEC = PORT->PORTVEC;
	struct SOUNDTNCINFO * TNC = PortToTNC[port];
	int State = TNC->State;
	UINT * buffptr;

	switch (fn)
	{
	case 1:				// poll

		TNC->ProcessMonitor++;
		
		if (TNC->ProcessMonitor > 300)
		{
			TNC->ProcessMonitor = 0;
			
			if (TNC->PID)
			{
				HANDLE hProc;
				hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, TNC->PID);

				if (hProc)
				{	
					DWORD ExitCode = 0;

					GetExitCodeProcess(hProc, &ExitCode);
					if (ExitCode != STILL_ACTIVE)
						RestartSoundTNC(TNC);

 					CloseHandle(hProc);
				}
			}
			else
				RestartSoundTNC(TNC);
		}
	
		if (State & 0x20)
			PORTVEC->SENDING++;

		if (State & 0x10)
			PORTVEC->ACTIVE++;

		buffptr = Q_REM(&RXQ[port]);

		if (buffptr)
		{
			len = buffptr[1];

			memcpy(&buff[7], &buffptr[2], len);
			len+=7;
			buff[5]=(len & 0xff);
			buff[6]=(len >> 8);

			ReleaseBuffer(buffptr);

			return len;
		}

		return 0;

	case 2: // send

		buffptr = GetBuff();
			
		if (buffptr == 0) return (0);			// No buffers, so ignore

		txlen = (buff[6] <<8 ) + buff[5] - 7;	
		buffptr[1] = txlen;
		buffptr[2] = PortToChannel[port];		// Channel on this Soundcard
		memcpy(buffptr+3, &buff[7], txlen);

		C_Q_ADD(&TNC->TXQ, buffptr);

		return 0;


	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK

	case 4:				// reinit

		return (0);		// OK
			
	case 5:				// Close

		if (TNC->PID)
		{
			KillSoundTNC(TNC);			
			if (AUTOSAVE == 1) SaveNodes();	
			Sleep(1000);
		}

		return (0);

	}

	return 0;

}

KillSoundTNC(struct SOUNDTNCINFO * TNC)
{
	HANDLE hProc;

	if (TNC->PID == 0) return 0;

	hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, TNC->PID);

	if (hProc)
	{
		TerminateProcess(hProc, 0);
		CloseHandle(hProc);
	}

	TNC->PID = 0;			// So we don't try again

	return 0;
}

RestartSoundTNC(struct SOUNDTNCINFO * TNC)
{
	char cmdLine[80];
	char Prog[256];
	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 

	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
  	SInfo.lpReserved2=NULL; 

	TNC->PID = 0;			// So we don't try again

	sprintf(cmdLine, "BPQSoundModem.exe %d %d %d",
		TNC->SoundCardNumber, TNC->PERSIST, TNC->TXDELAY); 
	sprintf(Prog, "%s\\BPQSoundModem.exe", BPQProgramDirectory);

	return CreateProcess(Prog, cmdLine, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo);

	return 0;
}

enum SMCmds
{
	INIT,
	CHECKCHAN,		// See if channel is configured
	GETBUFFER,
	POLL,
	RXPACKET,
	CLOSING
};

DllExport UINT APIENTRY SoundCardInterface(int CardNo, int Function, UINT Param1, UINT Param2)
{
	UINT * buffptr;
	L1FRAME * rxf;

	switch (Function)
	{
	case INIT:	
		SoundCardList[CardNo].PID = Param1;
		Debugprintf("SoundCard %d PID %d", CardNo, Param1);
		break;

	case CHECKCHAN:

		Debugprintf("CheckChan %d %d = %d", CardNo, Param1,  SoundCardList[CardNo].Channellist[Param1].BPQPort); 

		return SoundCardList[CardNo].Channellist[Param1].BPQPort;	// Nonzero if port is configured

	case GETBUFFER:
		return (UINT)GetBuff();

	case POLL:

		// See if anything on the TXQ

		SoundCardList[CardNo].State = Param1;		// PTT and DCD State bits
		buffptr = Q_REM(&SoundCardList[CardNo].TXQ);

		if (buffptr)
		{
			memcpy((void *)Param2, buffptr, buffptr[1] + 12);
			ReleaseBuffer(buffptr);
			return 1;
		}

		return 0;


	case RXPACKET:

		// A received Packet. Param1 is buffer, Param2 the channel

		buffptr = GetBuff();

		if (buffptr)
		{
			rxf = (L1FRAME *)Param1;

			buffptr[1] = rxf->len;
			memcpy(buffptr+2, rxf->frame, rxf->len);

			C_Q_ADD(&RXQ[SoundCardList[CardNo].Channellist[Param2].BPQPort], buffptr);
		}
		
		break;

	case CLOSING:

		SoundCardList[CardNo].ProcessMonitor = 250;		// Restart in 5 secs
		break;
	}
	return 0;
}



/*
Packet: fm F6AUC-0 to APX194-0 via RS0ISS-4 UI  pid=F0
=4329.20N/00130.45W-PHG2330 Anglet Cote Basque



Packet: fm RS0ISS-11 to F6CDZ-0 RR1+

Packet: fm RS0ISS-11 to F6CDZ-0 DISC+

Packet: fm RS0ISS-11 to F6CDZ-0 DISC+

Packet: fm DB1VQ-0 to APRS-0 via RS0ISS-4 UI  pid=F0
=4919.40N/00705.14E`73' Via Satellite {UISS52}

Packet: fm LA4FPA-6 to APU25N-0 via RS0ISS-4,WIDE2-2 UI^ pid=F0
=6021.00N/00521.51E`Erling ,Bergen, JP20qi

Packet: fm DH4LAR-10 to APEG02-0 via RS0ISS-4 UI  pid=F0
=5453.45N/00817.30E-73' Via Satellite {UISS52}

Packet: fm OW0VIK-0 to V2PPST-0 via WIDE1-0,WIDE2-1 UIv pid=F0
'|JEl .#/]=

Packet: fm DF9EY-0 to CQ-0 via RS0ISS-4 UI^ pid=F0
:DB1VQ    :einen schoenen Morgen OM
.
Packet: fm F6CDZ-0 to APRS-0 via RS0ISS-4 UI  pid=F0
=4912.00N/00609.44E`Maizieres les Metz - Lorraine - Daniel. 73 {UISS52}

Packet: fm F6CDZ-0 to APRS-0 via RS0ISS-4 UI  pid=F0
* F6CDZ Daniel *
.    ___  ___
.   (__ )(__ )
.    / /  (_ \
.   (_/  (___/


*/


