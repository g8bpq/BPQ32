//
//	Interface to the Baycom Serial Port Modem Kernel Driver
//

//  Kernel Driver simply transmits bits on DTR, and returns time between transitions of CTS
//  This code does all HDLC encoding and decoding. It passes complete frames to Kernal, and receives an
//	array of time periods

//	Version 1.0 June 2012
//

#pragma data_seg("_BPQDATA")

#define WIN32_LEAN_AND_MEAN
#define _USE_32BIT_TIME_T
#define _CRT_SECURE_NO_DEPRECATE 

#include "time.h"

typedef unsigned char byte;


#define IOCTL_BPQ_SET_BAYCOM_MODE	CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x810,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQ_SET_BAYCOM_PARAMS	CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x811,METHOD_BUFFERED,FILE_ANY_ACCESS)

#include "CHeaders.h"
#include "bpq32.h"
#include <winioctl.h> 

_CRT_OBSOLETE(GetVersionEx) errno_t __cdecl _get_winmajor(__out unsigned int * _Value);
_CRT_OBSOLETE(GetVersionEx) errno_t __cdecl _get_winminor(__out unsigned int * _Value);

void GetSemaphore();
void FreeSemaphore();


extern UCHAR BPQProgramDirectory[];


VOID __cdecl Debugprintf(const char * format, ...);
HANDLE _beginthread( void( *start_address )(), unsigned stack_size, VOID * arglist);
static int ExtProc(int fn, int port, unsigned char * buff);
VOID * zalloc(int len);

#define MaxFrameLen 340
#define MinFrameLen 17

#define	TxBufferLen 32768			// Bits to send - includes flags, stuffing and CRC

#define MaxSamples 40960			// Time between transition samples from Kernel Driver. May have up to one per bit.
								// and 1200 bits /sec. Latency should be around 100 mS, but can allow for much more
								// without using excessive memory

LONGLONG SysClock;		 		// Kernel timeer frequency

struct BAYINFO
{
	struct PORTCONTROL *  PortRec;
	HANDLE hDevice;

	OVERLAPPED Overlapped;
	OVERLAPPED OverlappedRead;

	int	BitTime;					// Bit Length in clock ticks - calculated
	int MaxBitTime;
	int MinBitTime;					// Limits of PLL clock
	int PLLBitTime;					// Actual bit length, adjusted for timing errors by DPLL

	UCHAR RxFrame[MaxFrameLen + 10];// Receive Buffer
	int RXFrameLen;
	int BitCount;					// Bits received 
	BOOL RxFrameOK;					// Frame not aborted
	int SamplePhase;				// time to next sampling point
	int SampleLevel;				// low=current data bit, high=prev. bit
	UCHAR RxReg;					// RX Data Shift register
	int RXOneBits;					// Bit count for removing stuffed zeors

	BOOL DCD;						// Current carrier state
	BOOL LastDCD;					// Last carrier state (only do csma if channel was busy and is now free)

	UINT toBPQ_Q;					// Frames from Modem to BPQ32 L2
	UINT fromBPQ_Q;					// Frames from BPQ32 L2 to modem
	UINT ACK_Q;						// Frames waiting to be returned to L2 when they have been sent

	UCHAR TxBuffer[TxBufferLen];
	int TxBlockPtr;

	int TXOneBits;					// Count of consecutive 1 bits (for stuffing)
};

struct BAYINFO * BayList[33];

unsigned short int compute_crc(unsigned char *buf,int len);

static int Init98(HDLCDATA * PORTVEC);
static int Init2K(HDLCDATA * PORTVEC);
VOID EncodePacket(struct BAYINFO * Baycom, UCHAR * Data, int Len);
VOID AddTxByteDirect(struct BAYINFO * Baycom, UCHAR Byte);
VOID AddTxByteStuffed(struct BAYINFO * Baycom, UCHAR Byte);
VOID ProcessSample(struct BAYINFO * Baycom, int Interval);
VOID RxBit(struct BAYINFO * Baycom, UCHAR NewBit);
VOID PrepareNewFrame(struct BAYINFO * Baycom);
VOID SendtoBaycom(struct BAYINFO * Baycom);
VOID CheckTX(struct BAYINFO * Baycom);

UINT BaycomExtInit(struct PORTCONTROL *  PortEntry)
{
	char msg[255];
	char szPort[15];
	COMMTIMEOUTS CommTimeOuts ;
	HANDLE hDevice;
	int Port = PortEntry->PORTNUMBER;
	int Channel = PortEntry->CHANNELNUM - 65;
	HDLCDATA * PORTVEC = (HDLCDATA *)PortEntry;
	UINT Speed;
	struct BAYINFO * Baycom;

	LARGE_INTEGER xx, yy;
	UINT H, L;

	__asm {

		rdtsc 
		mov	H,edx
		mov	L,eax
	}

	xx.HighPart = H;
	xx.LowPart = L;

	Sleep(1000);

	__asm {

		rdtsc 
		mov	H,edx
		mov	L,eax
	}

	yy.HighPart = H;
	yy.LowPart = L;

	yy.QuadPart -= xx.QuadPart;

	SysClock = yy.QuadPart;

	Debugprintf("One sec = %I64d", SysClock);

	sprintf(msg,"Baycom Serial Modem Port %d", Port);
	WritetoConsole(msg);

	Baycom = BayList[Port] = zalloc(sizeof(struct BAYINFO));

	Baycom->PortRec = PortEntry;

	sprintf(szPort, "\\\\.\\COM%d", PortEntry->IOBASE);

  
	Baycom->hDevice = hDevice = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL );
				  
	if (hDevice == (HANDLE) -1 )
	{
		sprintf(msg, "COM%d could not be opened ", PortEntry->IOBASE);
		WritetoConsole(msg);
		Baycom->hDevice = NULL;
	}
	else
	{
		DCB dcb;
		int ret;

		int TXD = PortEntry->PORTTXDELAY;
		int CharTime;

		Speed = PortEntry->BAUDRATE;

		// Kernel wants TXD in character times

		CharTime = 1000000 / (Speed);			// us per char (UART runs at 8 * baud)
		TXD = (TXD * 1000) / CharTime;

		if (TXD == 0)
			TXD = 1;

		// Make sure Serial Port is using Baycom version of serial.sys

		if (DeviceIoControl(hDevice, IOCTL_BPQ_SET_BAYCOM_PARAMS, &TXD, 4, NULL, 0, &ret, NULL) != 1)
		{
			sprintf(msg, " Baycom Driver not found");
			CloseHandle(hDevice);
			Baycom->hDevice = NULL;
			WritetoConsole(msg);
		}
		else
		{
			SetupComm(hDevice, 4096, 32768);			// We send bits to driver, so need a lot
		
			PurgeComm(hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

			// set up for overlapped I/O
	  
			CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
			CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
			CommTimeOuts.ReadTotalTimeoutConstant = 0;
			CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
//		    CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
			CommTimeOuts.WriteTotalTimeoutConstant = 30000;
			SetCommTimeouts(hDevice, &CommTimeOuts );

			dcb.DCBlength = sizeof(DCB);
			GetCommState(hDevice, &dcb ) ;
 
			dcb.BaudRate = Speed * 8;
			dcb.ByteSize = 6;
			dcb.Parity = 0 ;
			dcb.StopBits = 0;		// 1 Stop
			dcb.fDtrControl = 0;
			dcb.fDtrControl = 0;
		
			// other various settings

			dcb.fBinary = TRUE;
			dcb.fParity = FALSE;

			SetCommState(hDevice, &dcb);

			EscapeCommFunction(hDevice,CLRRTS);
			EscapeCommFunction(hDevice,CLRDTR);

			Speed = PortEntry->BAUDRATE;

			Baycom->BitTime = Baycom->PLLBitTime = SysClock / Speed;
			Baycom->MaxBitTime = 1.05 * SysClock / Speed;
			Baycom->MinBitTime = 0.95 * SysClock / Speed;
		}
	}


	PortEntry->KISSFLAGS = 255;			// Flag for Baycom ACK processing

	WritetoConsole("\n");

	return ((UINT) ExtProc);
}

static int ExtProc(int fn, int port, unsigned char * buff)
{
	int len = 0, txlen=0;
	struct BAYINFO * Baycom = BayList[port];
	BOOL       fReadStat = 0;
	int Len = 0, i;
	int Samples[1000];
	UINT * buffptr;

	if (Baycom == NULL)
		return 0;
	
	switch (fn)
	{
	case 1:				// poll

		// See if any samples to process

		// only try to read number of bytes in queue 

		memset(&Baycom->OverlappedRead, 0, sizeof(Baycom->OverlappedRead));		
		fReadStat = ReadFile(Baycom->hDevice, &Samples[0], 4000, &Len, &Baycom->OverlappedRead);

		if (fReadStat == 0)
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
				int Loops = 10;
				while ((HasOverlappedIoCompleted(&Baycom->OverlappedRead) == FALSE) && Loops)
				{
					Sleep(1);
					Debugprintf("RX waiting..");
					Loops--;
				}
				Len = Baycom->OverlappedRead.InternalHigh;
			}
		}

		if (Len)
		{
			Len /= 4;
			Debugprintf("%d Samples", Len);

			for (i = 0; i < Len; i++)
				ProcessSample(Baycom, Samples[i]);
		}

		if (Baycom->toBPQ_Q != 0)
		{
			int datalen;

			buffptr = Q_REM(&Baycom->toBPQ_Q);
			datalen = buffptr[1];

			memcpy(&buff[7], buffptr + 2, datalen);
			datalen += 7;
			buff[5]=(datalen & 0xff);
			buff[6]=(datalen >> 8);
		
			ReleaseBuffer(buffptr);

			return (1);

		}

		CheckTX(Baycom);			// See if frames to send, and if so do CSMA to see if can send them now

		return 0;

	case 2: // send
		
		// Frames are not returned to the switch code until they have been sent (so FRACK starts when sent)
		
		C_Q_ADD(&Baycom->fromBPQ_Q, (UINT *)buff);

		return 0;


	case 3:				// CHECK IF OK TO SEND

		if (Baycom->Overlapped.Internal == 0)	// ? Sending a block of frsames?
			return 0;		// OK
		else
			return 1;
	case 4:				// reinit

		return (0);		// OK
			
	case 5:				// Close

		EscapeCommFunction(Baycom->hDevice,CLRRTS);
		EscapeCommFunction(Baycom->hDevice,CLRDTR);

		CloseHandle(Baycom->hDevice);
		Baycom->hDevice = NULL;

		return (0);
	}

	return 0;

}

VOID CheckTX(struct BAYINFO * Baycom)
{
	if (Baycom->fromBPQ_Q == 0 || Baycom->Overlapped.Internal)	// Nothing to send, or TX busy
		return;

	if (Baycom->PortRec->FULLDUPLEX == 0)
	{
		// Do CSMA

		if (Baycom->DCD)
			return;

		// if DCD has just dropped, do csma. If not, just transmit

		if (Baycom->LastDCD)
		{
			UINT Random = rand();

			if (LOBYTE(Random) > Baycom->PortRec->PORTPERSISTANCE)
				return;
		}
	}

	// ok to send

	// The driver expects all the bits to be sent as a single transmission. Add frames to the TX buffer up to a reasonable 
	// limit

	Baycom->TxBlockPtr = 0;

	while (Baycom->fromBPQ_Q && Baycom->TxBlockPtr < TxBufferLen - 3000)
	{
		UCHAR * buffptr = (UCHAR *)Q_REM(&Baycom->fromBPQ_Q);
		int txlen=(buffptr[6] << 8) + buffptr[5] - 7;	
	
		EncodePacket(Baycom, &buffptr[7], txlen);
		C_Q_ADD(&Baycom->ACK_Q, (UINT *)buffptr);
	}

	// EncodePacket adds a starting flag, but not an ending one, so successive packets can be sent with a single flag 
	// between them. So at end add a teminating flag and a pad to give txtail

	AddTxByteDirect(Baycom, 0x7e);			// End Flag - add without stuffing
	AddTxByteDirect(Baycom, 0xff);			// Tail Padding
	AddTxByteDirect(Baycom, 0xff);			// Tail Padding

	memset(&Baycom->Overlapped, 0, sizeof(Baycom->Overlapped));		
	Baycom->Overlapped.Internal = -1;		// Set Overlapped struc as busy

	_beginthread(SendtoBaycom, 0, (VOID *)Baycom);	// send to driver - use a separate thread so it can block till complete
}

VOID EncodePacket(struct BAYINFO * Baycom, UCHAR * Data, int Len)
{
	USHORT CRC = compute_crc(Data, Len);
	UCHAR * Msg = Data;
	UCHAR * Msgend = Data + Len;

	AddTxByteDirect(Baycom, 0x7e);			// Start Flag - add without stuffing

	while (Msg < Msgend)
	{
		AddTxByteStuffed(Baycom, *(Msg++));	// Send byte with stuffing
	}

	CRC ^= 0xffff;

	AddTxByteStuffed(Baycom, LOBYTE(CRC));
	AddTxByteStuffed(Baycom, HIBYTE(CRC));
}

extern UINT TRACE_Q;

VOID SendtoBaycom(struct BAYINFO * Baycom)
{
	int ret, retcode;
	int Ptr = 0;

	Debugprintf("Baycom Writing %d bits", Baycom->TxBlockPtr);

	retcode = WriteFile(Baycom->hDevice, &Baycom->TxBuffer, Baycom->TxBlockPtr, &ret, &Baycom->Overlapped);

	if (retcode == 0)
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			while (HasOverlappedIoCompleted(&Baycom->Overlapped) == FALSE)
			{
				Sleep(100);
//				Debugprintf("TX waiting..");
			}
		}
	}

	Debugprintf("Written = %d Flag = %d ", Baycom->Overlapped.InternalHigh, Baycom->Overlapped.Internal);

	// return transmitted frames to node, after setting FRACK timer

	GetSemaphore(&Semaphore, 70);

	while (Baycom->ACK_Q)
	{
		UINT * 	buffptr = Q_REM(&Baycom->ACK_Q);
		C_Q_ADD(&TRACE_Q, buffptr);
	}

	FreeSemaphore(&Semaphore);
}

VOID ProcessSample(struct BAYINFO * Baycom, int Interval)
{
	// Interval is time since previous input transition

	int BitTime = Baycom->PLLBitTime;
	int ReasonableError = BitTime / 5;
	int Bits = 0;

	if (Interval < BitTime - ReasonableError)		// Allow 5% error here
		return;										// Short sample - assume noise for now
													// at some point try to ignore short spikes

	if (Interval > BitTime * 10)
	{
		Debugprintf("Bittime = %d interval = %d", BitTime, Interval);
		Interval = BitTime * 10;			// Long idles - 9 ones is plenty to abort and reset frame
	}

	while (Interval > BitTime + ReasonableError)		// Allow 5% error here
	{
		RxBit(Baycom, 1);
		Interval -= BitTime;
		Bits++;
	}

	RxBit(Baycom, 0);
	Bits++;

	Interval -= BitTime;

	Interval /= Bits;			// Error per bit

	// Use any residue to converge DPLL, but make sure not spurious transition before updating DPLL

	// Can be between - (BitTime - Reasonable) and Reasonable. Can't be greater, or we would have extraced another 1 bit
	
	if ((Interval + ReasonableError) < 0)
	{
		// Do we try to ignore spurious? Maybe once synced.

		// for now, ignore, but dont use to converge dpll

//		Debugprintf("Bittime = %d Residue interval = %d", BitTime, Interval);

		return;
	}

	if (Interval)
	{
		Baycom->PLLBitTime += Interval /16;
		if (Baycom->PLLBitTime > Baycom->MaxBitTime)
			Baycom->PLLBitTime = Baycom->MaxBitTime;
		else
			if (Baycom->PLLBitTime < Baycom->MinBitTime)
				Baycom->PLLBitTime = Baycom->MinBitTime;
	}
}

/*


MoveSampling:
	  cmp ax,dx             ;compare SamplePhase with period
	  jc PeriodLower        ;jump if Period lower
	  sub ax,dx             ;subtract SamplePhase from period
	  xor ch,cl             ;xor level with previous level
	  call NewRxBit         ;analyze bit in bit 0,ch
	  mov ch,cl             ;previous level = level
	  mov dx,cl_bit_len     ;load SamplePhase with bit length
	  jmp short MoveSampling      ;loop
PeriodLower:
	sub dx,ax               ;subtract period from SamplePhase
	mov SampleLevel,cx      ;save SampleLevel

				;now comes rather primitive DPLL
	mov ax,cl_bit_len_2     ;load half bit period
	sub ax,dx               ;subtract SamplePhase
				;now: dx=SamplePhase, ax=phase error
	cmp ax,walk_step
	jle check_neg
	  add dx,walk_step
	  jmp short save_SamplePhase
check_neg:        
	neg ax
	cmp ax,walk_step
	jle save_SamplePhase
	  sub dx,walk_step

;        add ax,8                ;offset error to compensate
;                                ;for "sar" arithmetic error
;        mov cl,4                ;divide the error by 16
;        sar ax,cl
;        add dx,ax               ;add correction to SamplePhase

save_SamplePhase:        
	mov SamplePhase,dx      ;save SamplePhase
	ret
*/

VOID PrepareNewFrame(struct BAYINFO * Baycom)
{
	Baycom->RXFrameLen = Baycom->BitCount = Baycom->RXOneBits = 0;
	Baycom->RxFrameOK = TRUE;
}


VOID ProcessFlag(struct BAYINFO * Baycom)
{
	// BitCount should have the 7 bits extracted from the (unstuffed) flag if are on a byte boundary

	Debugprintf("BAYCOM Flag RX'ed Framelen %d FrameOK %d BitCount %d",
		Baycom->RXFrameLen, Baycom->RxFrameOK, (Baycom->BitCount & 7));
	
	if (Baycom->RXFrameLen > 16)
	{
		Baycom->RxFrame[Baycom->RXFrameLen] = 0;	// for test display
		Debugprintf("BAYCOM RX Data %s", Baycom->RxFrame);
	}
	
	if (Baycom->RXFrameLen >= MinFrameLen && Baycom->RxFrameOK && (Baycom->BitCount & 7) == 7)
	{
		// Check CRC

		USHORT crc = compute_crc(Baycom->RxFrame, Baycom->RXFrameLen);
		{
			if (crc == 0xf0b8)		// Good CRC
			{
				UINT * buffptr = GetBuff();

				Debugprintf("BAYCOM Good CRC");

				if (buffptr)
				{
					buffptr[1] = Baycom->RXFrameLen - 2;
					memcpy(buffptr+2, Baycom->RxFrame, buffptr[1]);
					C_Q_ADD(&Baycom->toBPQ_Q, buffptr);
				}
			}
			else
			{
				// Bad CRC

				Baycom->PortRec->RXERRORS++;
			}
		}
	}

	// Bad frame, not closing flag no buffers or whatever - set up for next frame
		
	PrepareNewFrame(Baycom);
	return;
}



VOID RxBit(struct BAYINFO * Baycom, UCHAR NewBit)
{
	UCHAR RxShiftReg = Baycom->RxReg >> 1;
	int BitCount;
		
	if (NewBit)				// 1 bit
	{
		RxShiftReg |= 0x80;
		Baycom->RXOneBits++;	// Another 1 bit

		if (Baycom->RXOneBits > 6)		// >6 Ones - an abort
		{
			Baycom->RxFrameOK = FALSE;
			Baycom->PLLBitTime = Baycom->BitTime;	// Reset PLL
			Debugprintf("Baycom - Abort received");
			return;
		}
	}
	else
	{
		// Zero Bit - check for stuffing

		if (Baycom->RXOneBits	== 6)			// 6 Ones followed by zero - a flag
		{
			ProcessFlag(Baycom);
			return;
		}

		if (Baycom->RXOneBits	== 5)
		{
			Baycom->RXOneBits = 0;
			return;						// Drop bit		
		}
				
		Baycom->RXOneBits = 0;

		// No need to or in zero bit
	}
			
	// See if 8 bits received

	BitCount = ++Baycom->BitCount;

	if ((BitCount & 7) == 0)		// 8 bits
	{
		int Len = Baycom->RXFrameLen;

		Baycom->RxFrame[Len++] = RxShiftReg;

		if (Len > MaxFrameLen)
			Baycom->RxFrameOK = FALSE;
		else
			Baycom->RXFrameLen = Len;
	}

	Baycom->RxReg = RxShiftReg;
}

/*
TxCountDown     dw 0    ;Down-counter to measure Tx bits within a byte.
TxState         db 0    ;0 = idle
			;1 = sending head
			;2 = sending usefull data
			;3 = sending tail


sense_DataTrans:
	xor ax,ax               ;at least one data transition
	cmp ax,DataTransCount   ;since previous time slot ?
	ret                     ;carry=1 if so

sense_data:
	mov bx,DataTransCount
	cmp bx,2                ;more than 1 transitions counted ?
	jc few_trans            ;jump if not
	mov cx,dx               ;save dx
	mov ax,PeriodDevSum     ;compute sum/count that is the average
	mov dx,PeriodDevSum+2
	div bx                  ;ax=periodDevSum div DataTransCount
	cmp ax,cl_dcd_thres     ;is the average bigger than dcd_threshold ?
	mov dx,cx               ;restore dx
	ret                     ;carry=0 (no carrier) if so
few_trans:
	clc             ;say carrier=false if there were only few transitions
	ret

		even
DataTransCount  dw 0            ;count input signal transitions
PeriodDevSum    dw 0,0          ;sums period deviations from round bit lenghts
prev_period     dw 0

ClearDataStat:                  ;clear statistics for DCD
	xor ax,ax
	mov DataTransCount,ax
	mov PeriodDevSum,ax
	mov PeriodDevSum+2,ax
	ret
UpdateDataStat:                 ;input: ax=period
	inc DataTransCount      ;increment data transition counter
	mov bl,carrier_sense    ;check carrier mode
	cmp bl,3                ;execute the rest only if carrier mode is 3
	jnz UpdateDataStat_ret
	push dx
	xor dx,dx
	xchg ax,prev_period     ;prev_period=period;
	add ax,prev_period      ;ax=period+prev_period
	add ax,cl_bit_len_2     ;period+=cl_bit_len/2
	mov bx,cl_bit_len
	div bx                  ;dx=(period+cl_bit_len_2) div cl_bit_len
	shr bx,1                ;bx=cl_bit_len/2
	sub dx,bx               ;dx-=cl_bit_len/2
	jnc UpdateDevSum        ;if result negative
	  neg dx                ;then negate it (we need absolute value)
UpdateDevSum:
	xor ax,ax               ;add dx to PeriodDevSum
	add PeriodDevSum,dx     ;PeriodDevSum sums deviation of periods
	adc PeriodDevSum+2,ax   ;from multiple bit lengths
				;for DCD decision
	pop dx
UpdateDataStat_ret:
	ret             ;ax,bx modified

	*/

VOID AddTXBit(struct BAYINFO * Baycom, UCHAR Byte)
{
	if (Byte)
		Baycom->TXOneBits++;
	else
		Baycom->TXOneBits = 0;
	
	Baycom->TxBuffer[Baycom->TxBlockPtr++] = Byte;

	// *********** Testing *******
	// RxBit(Baycom, Byte);
}

VOID AddTxByteDirect(struct BAYINFO * Baycom, UCHAR Byte)				// Add unstuffed byte to output
{
	int i;
	UCHAR Data = Byte;

	for (i = 0; i < 8; i++)
	{
		AddTXBit(Baycom, Byte & 1);
		Byte >>= 1;
	}
}

VOID AddTxByteStuffed(struct BAYINFO * Baycom, UCHAR Byte)				// Add unstuffed byte to output
{
	int i;
	UCHAR Data = Byte;

	for (i = 0; i < 8; i++)
	{
		AddTXBit(Baycom, Byte & 1);
		Byte >>= 1;

		if (Baycom->TXOneBits == 5)
			AddTXBit(Baycom, 0);
	}
}
