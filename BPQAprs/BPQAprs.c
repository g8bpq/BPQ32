//
// APRS Mapping and Messaging App for BPQ32 Switch.
//

// First Release Jan 2012 Version 1.0.0.0

// Jan 2012 Version 1.0.0.1

// Fix WX message comment

//	Jan 2012

// Allow suppression of stations with 0 Lat/Lon
// Allow suppression of tracks
// Add option to Select track colour

// Feb 2012

// Mods to use MapQuest's jpeg tiles

// Decode Items
// Allows WX to be turned off.
// Add Local Time option
// Add "Set filter to current View" command 
// Implenent "Reply-Acks"

// June 2012 Version 1.1.2.1

// Add option to create a jpeg of the APRS display.
// Implement Web Server

// .2

// Handle Stations that treats message seq as a number, not a text field
// Fix excessive CPU if map is minimized while Create jpeg image is enabled

// .3

//	Add Clear Msgs option

// Auguxt 2012 Version 1.1.3.1	Released

// June 2013

// Fix Loop if Object is Station
// Fix crash if > 1000 stations at a point

// Jan 2014
// Add WX decode to station popup

// Feb 2014 1.1.7.1
// Most processing is now in bpq32.dll (Decode, Station storage and Web interface)
// Only GUI and messaging are handled here.

//  Aug 2014 1.1.8.1
//	Fix sending messages to unknown callsigns

//	Nov 2014 1.1.9.1
//	Add Track Expire Time to config


#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T	// Until the ASM code switches to 64 bit time

#define _WIN32_WINNT 0x0501	

#define MARGIN 0

#define DllImport	__declspec(dllimport)

// standard includes

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "winsock2.h"
#include "WS2tcpip.h"
#include <windows.h>
#include <commctrl.h> 
#include "Commdlg.h"

#include <malloc.h>
#include <memory.h>

#include <setjmp.h>

#define M_PI       3.14159265358979323846

// application includes

#include "png.h"
#include "pngfile.h"
#include "resource.h"

#include "bpq32.h"
#include "compatbits.h"
#include "ASMStrucs.h"
#define APRS
#include "Versions.h"
#include "GetVersion.h"
#include "BPQAPRS.h"

HBRUSH bgBrush;

#define BGCOLOUR RGB(236,233,216)

char APRSCall[10];
char LoppedAPRSCall[10];

char ISFilter[1000] = "m/50 u/APBPQ*"; 

char * StatusMsg;

int RetryCount = 4;
int RetryTimer = 45;
int ExpireTime = 120;
int TrackExpireTime = 1440;
BOOL SuppressNullPosn = FALSE;
BOOL DefaultNoTracks = FALSE;
BOOL LocalTime = TRUE;

char WXFileName[MAX_PATH];
char WXComment[80];
char WXPortList[80];
BOOL SendWX = FALSE;
int WXInterval = 30;

BOOL CreateJPEG = FALSE;
int JPEGInterval = 300;
int JPEGCounter = 0;
char JPEGFileName[MAX_PATH] = "APRSImage.jpg";

struct STATIONRECORD * CurrentPopup;


BOOL WXPort[32];				// Ports to send WX to

int WXCounter;

RECT Rect, MsgRect, StnRect;

HKEY REGTREE = HKEY_LOCAL_MACHINE;		// Default

char Key[80];

int portmask=0xffff;
int mtxparam=1;
int mcomparam=1;

typedef struct tag_dlghdr {

HWND hwndTab; // tab control
HWND hwndDisplay; // current child dialog box
RECT rcDisplay; // display rectangle for the tab control

DLGTEMPLATE *apRes[33];

} DLGHDR;

// Station Name Font

const unsigned char ASCII[][5] = {
//const u08 ASCII[][5]  = {
  {0x00, 0x00, 0x00, 0x00, 0x00} // 20  
  ,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
  ,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
  ,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
  ,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
  ,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
  ,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
  ,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
  ,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
  ,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
  ,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
  ,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
  ,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
  ,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
  ,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
  ,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
  ,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
  ,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
  ,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
  ,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
  ,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
  ,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
  ,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
  ,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
  ,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
  ,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
  ,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
  ,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
  ,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
  ,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
  ,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
  ,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
  ,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
  ,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
  ,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
  ,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
  ,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
  ,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
  ,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
  ,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
  ,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
  ,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
  ,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
  ,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
  ,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
  ,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
  ,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
  ,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
  ,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
  ,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
  ,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
  ,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
  ,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
  ,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
  ,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
  ,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
  ,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
  ,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
  ,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
  ,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
  ,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c 
  ,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
  ,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
  ,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
  ,{0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
  ,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
  ,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
  ,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
  ,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
  ,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
  ,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
  ,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
  ,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
  ,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
  ,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j 
  ,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
  ,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
  ,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
  ,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
  ,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
  ,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
  ,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
  ,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
  ,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
  ,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
  ,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
  ,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
  ,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
  ,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
  ,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
  ,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
  ,{0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
  ,{0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
  ,{0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
  ,{0x10, 0x08, 0x08, 0x10, 0x08} // 7e ~
  ,{0x78, 0x46, 0x41, 0x46, 0x78} // 7f DEL
};

COLORREF Colours[256] = {0, RGB(0,0,255), RGB(0,128,0), RGB(0,128,192), 
		RGB(0,192,0), RGB(0,192,255), RGB(0,255,0), RGB(128,0,128),
		RGB(128,64,0), RGB(128,128,128), RGB(192,0,0), RGB(192,0,255),
		RGB(192,64,128), RGB(192,128,255), RGB(255,0,0), RGB(255,0,255),				// 81
		RGB(255,64,0), RGB(255,64,128), RGB(255,64,192), RGB(255,128,0)};



extern unsigned __int64 IconData[];
extern int IconDataLen;

extern unsigned __int64 DummyData[];
extern int DummyDataLen;

// function prototypes

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PopupWndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK StnWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

VOID RefreshMessages();

BOOL LoadImageFile(HWND hwnd, PTSTR pstrPathName,
        png_byte **ppbImage, int *pxImgSize, int *pyImgSize, int *piChannels,
        png_color *pBkgColor);

BOOL DisplayImage (HWND hwnd, BYTE **ppDib,
        BYTE **ppDiData, int cxWinSize, int cyWinSize,
        BYTE *pbImage, int cxImgSize, int cyImgSize, int cImgChannels,
        BOOL bStretched);

BOOL InitBitmap (BYTE *pDiData, int cxWinSize, int cyWinSize);
BOOL FillBitmap (int x, int y);
VOID LoadImageSet(int Zoom, int startx, int starty);
BOOL RGBToJpegFile(char * fileName, BYTE *dataBuf, UINT widthPix, UINT height, BOOL color, int quality);

// a few global variables

BOOL ReloadMaps;

char APRSDir[MAX_PATH];
char OSMDir[MAX_PATH];
char APRSDir[MAX_PATH];
char Symbols[MAX_PATH];
char DF[MAX_PATH];

static png_color bkgColor = {127, 127, 127};
static BOOL bStretched = FALSE;
static BYTE *pDib = NULL;
static BYTE *pDiData = NULL;

static png_structp png_ptr = NULL;
static png_infop info_ptr = NULL;

// Image chunks are 256 rows of 3 * 256 bytes

// Read 8 * 8 files, and copy to a 2048 * 3 * 2048 array. The display scrolls over this area, and
// it is refreshed when window approaches the edge of the array.

UCHAR Image[2048 * 3 * 2048 + 100];	// Seems past last byte gets corrupt

int SetBaseX = 0;				// Lowest Tiles in currently loaded set
int SetBaseY = 0;

int Zoom = 3;

int MaxZoom = 16;

static int cxWinSize, cyWinSize;
static int cxImgSize, cyImgSize;
static int cImgChannels = 3;

int ScrollX;
int ScrollY;

int MapCentreX = 0;
int MapCentreY = 0;

int MouseX, MouseY;
int PopupX, PopupY;

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

HINSTANCE hInst; 
HWND hMapWnd, hPopupWnd, hSelWnd, hStatus, hwndDlg, hwndDisplay;
HWND hMsgsIn, hStnDlg, hStations, hMsgDlg, hMsgsOut, hInput, hToCall, hToLabel, hTextLabel, hPathLabel, hPath;
WNDPROC wpOrigInputProc; 

BOOL MsgMinimized;

char szAppName[] = "BPQAPRS";
char szTitle[80]   = "BPQAPRS Map" ; // The title bar text

BOOL APRSISOpen = FALSE;

int StationCount = 0;

UCHAR NextSeq = 1;

BOOL ImageChanged;
BOOL NeedRefresh = FALSE;
time_t LastRefresh = 0;

HANDLE hMapFile;

struct STATIONRECORD ** StationRecords = NULL;
struct APRSMESSAGE * Messages = NULL;
struct APRSMESSAGE * OutstandingMsgs = NULL;

struct OSMQUEUE OSMQueue = {NULL,0,0,0};

int OSMQueueCount;

DllImport VOID APIENTRY APRSConnect(char * Call, char * Filter);
DllImport VOID APIENTRY APRSDisconnect();
DllImport BOOL APIENTRY GetAPRSFrame(char * Frame, char * Call);
DllImport BOOL APIENTRY PutAPRSFrame(char * Frame, int Len, int Port);
DllImport BOOL APIENTRY PutAPRSMessage(char * Frame, int Len);
DllImport BOOL APIENTRY GetAPRSLatLon(double * PLat,  double * PLon);
DllImport BOOL APIENTRY GetAPRSLatLonString(char * PLat,  char * PLon);
DllImport VOID APIENTRY APISendBeacon();
DllImport struct STATIONRECORD *  APIENTRY APPLFindStation(char * Call, BOOL AddIfNotFount);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int NewLine(HWND hWnd);
VOID	ProcessBuff(HWND hWnd, MESSAGE * buff,int len,int stamp);
int TogglePort(HWND hWnd, int Item, int mask);
int CopyScreentoBuffer(char * buff);
VOID SendFrame(UCHAR * buff, int txlen);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int	KissDecode(UCHAR * inbuff, int len);
struct STATIONRECORD * FindStation(char * ReportingCall, char * Call, BOOL AddIfNotFound);
//void UpdateStation(char * Call, char * Path, char * Comment, double V_Lat, double V_Lon, double V_SOG, double V_COG, int iconRow, int iconCol);
VOID DrawStation(struct STATIONRECORD * ptr);
VOID FindStationsByPixel(int MouseX, int MouseY);
void RefreshStation(struct STATIONRECORD * ptr);
void RefreshStationList();
void RefreshStationMap();
void DecodeAPRSISMsg(char * msg);
BOOL DecodeLocationString(UCHAR * Payload, struct STATIONRECORD * Station);
VOID DecodeAPRSPayload(char * Payload, struct STATIONRECORD * Station);
VOID Decode_MIC_E_Packet(char * Payload, struct STATIONRECORD * Station);
BOOL GetLocPixels(double Lat, double Lon, int * X, int * Y);
VOID ProcessRFFrame(char * buffer, int len);
DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName);
VOID WINAPI OnSelChanged(HWND hwndDlg);
VOID WINAPI OnChildDialogInit(HWND hwndDlg);
VOID APRSPoll();
VOID OSMThread();
VOID ResolveThread();
VOID RefreshTile(char * FN, int Zoom, int x, int y);
BOOL CreateMessageWindow(char * ClassName, char * WindowTitle, WNDPROC WndProc, LPCSTR MENU);
BOOL CreateStationWindow(char * ClassName, char * WindowTitle, WNDPROC WndProc, LPCSTR MENU);
VOID ProcessMessage(char * Payload, struct STATIONRECORD * Station);
LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
VOID SendAPRSMessage(char * Text, char * ToCall);
VOID SecTimer();
int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
double Distance(double laa, double loa);
double Bearing(double laa, double loa);
VOID CreateStationPopup(struct STATIONRECORD * ptr, int MouseX, int MouseY);
INT_PTR CALLBACK ColourDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CreatePipeThread();

BYTE * JpegFileToRGB(char * fileName, UINT *width, UINT *height);

VOID SendWeatherBeacon();
VOID DecodeWXPortList();
BOOL CreatePopeThresd();

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, void * arglist);

SOCKADDR_IN destaddr = {0};

unsigned int ipaddr = 0;

//char Host[] = "tile.openstreetmap.org";

//char Host[] = "oatile1.mqcdn.com";		//SAT
char Host[] = "otile1.mqcdn.com";

extern short CRCTAB;

LOGFONT LFTTYFONT ;

HFONT hFont ;

BOOL MinimizetoTray=FALSE;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}

VOID CreateIconFile()
{
	HANDLE Handle;
	int Len;

	Handle = CreateFile(Symbols, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (Handle == INVALID_HANDLE_VALUE)
		return;
		
	WriteFile(Handle, (UCHAR *)IconData, IconDataLen, &Len, NULL); 

	SetEndOfFile(Handle);
	CloseHandle(Handle);
}

VOID CreateDummyTile()
{
	HANDLE Handle;
	int Len;

	Handle = CreateFile(DF, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (Handle == INVALID_HANDLE_VALUE)
		return;
		
	WriteFile(Handle, (UCHAR *)DummyData, DummyDataLen, &Len, NULL); 

	SetEndOfFile(Handle);
	CloseHandle(Handle);
}

VOID CreateIconSource()
{
	HANDLE Handle;
	unsigned __int64 File[20000];
	int i, n, Len;
	char Line [256];


	Handle = CreateFile(Symbols, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (Handle == INVALID_HANDLE_VALUE)
		return;
		
	ReadFile(Handle, (UCHAR *)File, 20000, &Len, NULL); 
	CloseHandle(Handle);

//	Handle = CreateFile("d:ARPSIconData.c", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	Handle = CreateFile("d:ARPSDummyData.c", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	n = wsprintf (Line, "int DummyDataLen = %d;\r\n", Len);

	WriteFile(Handle, Line ,n , &n, NULL);

	Len = (Len + 7) /8;

	n = wsprintf (Line, "unsigned __int64 DummyData[%d] = {\r\n", Len);

	WriteFile(Handle, Line ,n , &n, NULL);

	if (Handle != INVALID_HANDLE_VALUE)
	{
		for (i = 0; i < Len; i += 4)
		{
			n = wsprintf (Line, "  %#I64x, %#I64x, %#I64x, %#I64x,  \r\n",
				File[i], File[i+1], File[i+2], File[i+3]);
			WriteFile(Handle, Line ,n , &n, NULL);

		}

		WriteFile(Handle, "};\r\n", 4, &n, NULL); 
		SetEndOfFile(Handle);
	
		CloseHandle(Handle);
	}
}


int long2tilex(double lon, int z) 
{ 
	return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z))); 
}
 
int lat2tiley(double lat, int z)
{ 
	return (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z))); 
}

double long2x(double lon, int z) 
{ 
	return (lon + 180.0) / 360.0 * pow(2.0, z); 
}
 
double lat2y(double lat, int z)
{ 
	return (1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z); 
}


double tilex2long(double x, int z) 
{
	return x / pow(2.0, z) * 360.0 - 180;
}
 
double tiley2lat(double y, int z) 
{
	double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
	return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

void GetMouseLatLon(double * Lat, double * Lon)
{
	int X = ScrollX + MouseX;
	int Y = ScrollY + MouseY;

	*Lat = tiley2lat(SetBaseY + (Y / 256.0), Zoom);
	*Lon = tilex2long(SetBaseX + (X / 256.0), Zoom);
}

void GetCornerLatLon(double * TLLat, double * TLLon, double * BRLat, double * BRLon)
{
	int X = ScrollX;
	int Y = ScrollY;

	*TLLat = tiley2lat(SetBaseY + (Y / 256.0), Zoom);
	*TLLon = tilex2long(SetBaseX + (X / 256.0), Zoom);

	X = ScrollX + cxWinSize;
	Y = ScrollY + cyWinSize;

	*BRLat = tiley2lat(SetBaseY + (Y / 256.0), Zoom);
	*BRLon = tilex2long(SetBaseX + (X / 256.0), Zoom);
}

BOOL CentrePosition(double Lat, double Lon)
{
	// Positions the centre of the map at the specified location

	int X, Y;
	
	
	SetBaseX = long2tilex(Lon, Zoom) - 4;
	SetBaseY = lat2tiley(Lat, Zoom) - 4;				// Set Location at middle

	GetLocPixels(Lat, Lon, &X, &Y);

	ScrollX = X - cxWinSize/2;
	ScrollY = Y - cyWinSize/2;

	while(SetBaseX < 0)
	{
		SetBaseX++;
		ScrollX -= 256;
	}

	while(SetBaseY < 0)
	{
		SetBaseY++;
		ScrollY -= 256;
	}

	ReloadMaps = TRUE;
	InvalidateRect(hMapWnd, NULL, FALSE);

	return TRUE;
}

BOOL CentrePositionToMouse(double Lat, double Lon)
{
	// Positions  specified location at the mouse

	int X, Y;

	SetBaseX = long2tilex(Lon, Zoom) - 4;
	SetBaseY = lat2tiley(Lat, Zoom) - 4;				// Set Location at middle

	if (GetLocPixels(Lat, Lon, &X, &Y) == FALSE)
		return FALSE;							// Off map

	ScrollX = X - cxWinSize/2;
	ScrollY = Y - cyWinSize/2;


//	Map is now centered at loc cursor was at

//  Need to move by distance mouse is from centre

	// if ScrollX, Y are zero, the centre of the map corresponds to 1024, 1024
	
//	ScrollX -= 1024 - X;				// Posn to centre
//	ScrollY -= 1024 - Y;

	ScrollX += cxWinSize/2 - MouseX;
	ScrollY += cyWinSize/2 - MouseY;

	if (ScrollX < 0 || ScrollY < 0)
	{
		// Need to move image

		while(ScrollX < 0)
		{
			SetBaseX--;
			ScrollX += 256;
		}

		while(ScrollY < 0)
		{
			SetBaseY--;
			ScrollY += 256;
		}

		ReloadMaps = TRUE;
	}
	return TRUE;
}

	
BOOL GetLocPixels(double Lat, double Lon, int * X, int * Y)
{
	// Get the pixel offet of supplied location in current image.

	// If location is outside current image, return FAlSE

	int TileX;
	int TileY;
	int OffsetX, OffsetY;
	double FX;
	double FY;

	// if TileX or TileY are outside the window, return null

	FX = long2x(Lon, Zoom);
	TileX = (int)floor(FX);
	OffsetX = TileX - SetBaseX;

	if (OffsetX < 0 || OffsetX > 7)
		return FALSE;

	FY = lat2y(Lat, Zoom);
	TileY = (int)floor(FY);
	OffsetY = TileY - SetBaseY;

	if (OffsetY < 0 || OffsetY > 7)
		return FALSE;

	FX -= TileX;
	FX = FX * 256.0;

	*X = (int)FX + 256 * OffsetX;

	FY -= TileY;
	FY = FY * 256.0;

	*Y = (int)FY + 256 * OffsetY;

	return TRUE;
}




int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG Msg;
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];

	if (!InitApplication(hInstance)) 
			return (FALSE);

	if (!InitInstance(hInstance, nCmdShow))
		return (FALSE);

	// Main message loop:

	while(GetMessage(&Msg, NULL, 0, 0))
	{
		if (Msg.message == WM_KEYDOWN && Msg.wParam == 13)
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		else
		{
			if(!IsDialogMessage(hMsgDlg, &Msg))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
		}
	}

	retCode = RegCreateKeyEx(REGTREE, Key, 0,  0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (hMsgDlg)
	{
		ShowWindow(hMsgDlg, SW_RESTORE);
		GetWindowRect(hMsgDlg, &MsgRect);

		if (MinimizetoTray)
			DeleteTrayMenuItem(hMsgDlg);
	}

	wsprintf(Size,"%d,%d,%d,%d",MsgRect.left,MsgRect.right,MsgRect.top,MsgRect.bottom);
	retCode = RegSetValueEx(hKey,"MsgSize",0,REG_SZ,(BYTE *)&Size, strlen(Size));

	if (hStnDlg)
	{
		ShowWindow(hStnDlg, SW_RESTORE);
		GetWindowRect(hStnDlg, &StnRect);

		if (MinimizetoTray) 
			DeleteTrayMenuItem(hStnDlg);
	}
	
	wsprintf(Size,"%d,%d,%d,%d",StnRect.left,StnRect.right,StnRect.top,StnRect.bottom);
	retCode = RegSetValueEx(hKey,"StnSize",0,REG_SZ,(BYTE *)&Size, strlen(Size));

	if (retCode == ERROR_SUCCESS)
	{
		retCode = RegSetValueEx(hKey, "PortMask", 0, REG_DWORD, (BYTE *)&portmask, 4);
		retCode = RegSetValueEx(hKey, "SetBaseX", 0, REG_DWORD, (BYTE *)&SetBaseX, 4);
		retCode = RegSetValueEx(hKey, "SetBaseY", 0, REG_DWORD, (BYTE *)&SetBaseY, 4);
		retCode = RegSetValueEx(hKey, "ScrollX", 0, REG_DWORD, (BYTE *)&ScrollX, 4);
		retCode = RegSetValueEx(hKey, "ScrollY", 0, REG_DWORD, (BYTE *)&ScrollY, 4);
		retCode = RegSetValueEx(hKey, "Zoom", 0, REG_DWORD, (BYTE *)&Zoom, 4);

		wsprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
		retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));



		RegCloseKey(hKey);
	}

//	if (FloodCalls)
//		free(FloodCalls);

//	if (TraceCalls)
//		free(TraceCalls);

//	if (DigiCalls)
//		free(DigiCalls);

//	KillTimer(NULL, TimerHandle);

	_CrtDumpMemoryLeaks();

	CloseBPQ32();				// Close Ext Drivers if last bpq32 process

	CloseHandle(hMapFile);

	return (Msg.wParam);
}

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

	RegisterClass(&wc);

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MsgWndProc;                                    
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON));
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = "APRSMSGS"; 

	RegisterClass(&wc);

	wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = PopupWndProc;                                    
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON) );
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = "STNPOPUP"; 

	return RegisterClass(&wc);

}

//int len,count,i;
//char msg[20];

HMENU hMenu,hPopMenu1,hPopMenu2,hPopMenu3;		// handle of menu 
HMENU trayMenu = 0;
HMENU trayMenu2 = 0;

char CopyItem[260] = "";			// Area for Copy/Paste

UCHAR * iconImage = NULL;

//UCHAR Icons[100000];


//char Test[] = "2E0AYY>APU25N,TCPIP*,qAC,AHUBSWE2:=5105.18N/00108.19E-Paul in Folkestone Kent {UIV32N}\r\n";

CRITICAL_SECTION Crit, OSMCrit, RefreshCrit;



VOID GetStringVal(HKEY hKey, char * Key, char ** Val)
{
	int Vallen = 0, Type;
	int retCode = RegQueryValueEx(hKey, Key, 0, &Type, (UCHAR *)Val, &Vallen);

	if(Vallen)
	{
		*Val = malloc(Vallen + 1);
		retCode = RegQueryValueEx(hKey, Key, 0, &Type, *Val, &Vallen);
	}
}

int MaxStations = 5000;
	
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int retCode,Type,Vallen;
	HKEY hKey=0;
	char Size[80];
	char FN[MAX_PATH];
	HWND hWnd;
	char * ptr;
	char Error[80];

	BOOL bcopt=TRUE;
	u_long param=1;
	WSADATA WsaData;            // receives data from WSAStartup
	int ImgSizeX, ImgSizeY, ImgChannels;
	png_color bgColor;
	UCHAR * BPQDirectory = GetBPQDirectory();

	UCHAR * APRSStationMemory;

	//	Get pointer to Station Table in BPQ32.dll

	hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security
                 PAGE_READWRITE,          // read/write access
                 0,                       // maximum object size (high-order DWORD)
                 sizeof(struct STATIONRECORD) * (MaxStations + 1), // maximum object size (low-order DWORD)
                 "BPQAPRSStationsMappingObject");                 // name of mapping object

	if (hMapFile == NULL)
	{
		sprintf(Error, "Could not create file mapping object (%d).\n", GetLastError());
		MessageBox(NULL, "Error", "BPQAPRS", MB_ICONERROR);

		return FALSE;
	
	}
	APRSStationMemory = (LPTSTR) MapViewOfFileEx(hMapFile,   // handle to map object
                        FILE_MAP_ALL_ACCESS, // read/write permission
                        0,
                        0,
                        0,//sizeof(struct STATIONRECORD) * MaxStations,
						(LPVOID)0x43000000);

	if (APRSStationMemory == NULL)
	{
		sprintf(Error, "Could not map view of file (%d).\n", GetLastError());
		MessageBox(NULL, "Error", "BPQAPRS", MB_ICONERROR);
		CloseHandle(hMapFile);
		return FALSE;
	}

	StationRecords = (struct STATIONRECORD**)APRSStationMemory;

	REGTREE = GetRegistryKey();
	MinimizetoTray = GetMinimizetoTrayFlag();

	GetVersionInfo(NULL);

	if (BPQDirectory[0] == 0)
		wsprintf(APRSDir, "BPQAPRS");
	else
		wsprintf(APRSDir,"%s/BPQAPRS", BPQDirectory);

	wsprintf(OSMDir, "%s/OSMTiles", APRSDir);

	wsprintf(Symbols, "%s/Symbols.png", APRSDir);
//	wsprintf(Symbols, "%s/OSMTiles/DummyTile.jpg", APRSDir);

//	CreateIconSource();

	// Make sure top level OSM Dirs are there

	if (GetFileAttributes(APRSDir) == INVALID_FILE_ATTRIBUTES)
	{
		Debugprintf("Creating %s", APRSDir);
		CreateDirectory(APRSDir, NULL);
	}

	if (GetFileAttributes(OSMDir) == INVALID_FILE_ATTRIBUTES)
	{
		Debugprintf("Creating %s", OSMDir);
		CreateDirectory(OSMDir, NULL);
	}
	for (Zoom = 0; Zoom < 20; Zoom++)
	{
		wsprintf(FN, "%s/%02d", OSMDir, Zoom);

		if (GetFileAttributes(FN) == INVALID_FILE_ATTRIBUTES)
		{
			Debugprintf("Creating %s", FN);
			CreateDirectory(FN, NULL);
		}
	}

	Zoom = 2;

	wsprintf(DF, "%s/OSMTiles/DummyTile.jpg", APRSDir);

	if (GetFileAttributes(DF) == INVALID_FILE_ATTRIBUTES)
	{
		// Create Dummy Tile

		Debugprintf("Creating %s", DF);
		CreateDummyTile();
	}

	// Get config from Registry 

	wsprintf(Key,"SOFTWARE\\G8BPQ\\BPQ32\\BPQAPRS");

	retCode = RegOpenKeyEx (REGTREE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Zoom",0, (ULONG *)&Type,(UCHAR *)&Zoom,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"SetBaseX",0,			
			(ULONG *)&Type,(UCHAR *)&SetBaseX,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"SetBaseY",0,			
			(ULONG *)&Type,(UCHAR *)&SetBaseY,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"ScrollX",0,			
			(ULONG *)&Type,(UCHAR *)&ScrollX,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"ScrollY",0,			
			(ULONG *)&Type,(UCHAR *)&ScrollY,(ULONG *)&Vallen);

		Vallen=80;
		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);

		Vallen=80;

		retCode = RegQueryValueEx(hKey,"StnSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&StnRect.left,&StnRect.right,&StnRect.top,&StnRect.bottom);

		Vallen=80;

		retCode = RegQueryValueEx(hKey,"MsgSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&MsgRect.left,&MsgRect.right,&MsgRect.top,&MsgRect.bottom);

		GetStringVal(hKey, "StatusMsg", &StatusMsg);

		Vallen=999;
		retCode = RegQueryValueEx(hKey, "ISFilter", 0, &Type, ISFilter, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "RetryCount", 0, &Type,(UCHAR *)&RetryCount, &Vallen);
	
		Vallen=4;
		retCode = RegQueryValueEx(hKey, "RetryTimer", 0, &Type,(UCHAR *)&RetryTimer, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "ExpireTime", 0, &Type,(UCHAR *)&ExpireTime, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "TrackExpireTime", 0, &Type,(UCHAR *)&TrackExpireTime, &Vallen);

		Vallen=250;
		retCode = RegQueryValueEx(hKey, "WXFile", 0, &Type, WXFileName, &Vallen);
		Vallen=79;
		retCode = RegQueryValueEx(hKey, "WXText", 0, &Type, WXComment, &Vallen);
		Vallen=79;
		retCode = RegQueryValueEx(hKey, "WXPorts", 0, &Type, WXPortList, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "WXEnabled", 0, &Type,(UCHAR *)&SendWX, &Vallen);
		Vallen=4;
		retCode = RegQueryValueEx(hKey, "WXInterval", 0, &Type,(UCHAR *)&WXInterval, &Vallen);
		Vallen=4;
		retCode = RegQueryValueEx(hKey, "LocalTime", 0, &Type,(UCHAR *)&LocalTime, &Vallen);
		Vallen=4;
		retCode = RegQueryValueEx(hKey, "SuppressNullPosn", 0, &Type,(UCHAR *)&SuppressNullPosn, &Vallen);
		Vallen=4;
		retCode = RegQueryValueEx(hKey, "DefaultNoTracks", 0, &Type,(UCHAR *)&DefaultNoTracks, &Vallen);

		Vallen=250;
		retCode = RegQueryValueEx(hKey, "WXFile", 0, &Type, WXFileName, &Vallen);
		Vallen=79;
		retCode = RegQueryValueEx(hKey, "WXText", 0, &Type, WXComment, &Vallen);
		Vallen=79;
		retCode = RegQueryValueEx(hKey, "WXPorts", 0, &Type, WXPortList, &Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "CreateJPEG", 0, &Type,(UCHAR *)&CreateJPEG, &Vallen);
		Vallen=4;
		retCode = RegQueryValueEx(hKey, "JPEGInterval", 0, &Type,(UCHAR *)&JPEGInterval, &Vallen);
		Vallen=250;
		retCode = RegQueryValueEx(hKey, "JPEGFile", 0, &Type, JPEGFileName, &Vallen);

		RegCloseKey(hKey);
	}

	DecodeWXPortList();

	InitializeCriticalSection(&Crit); 
	InitializeCriticalSection(&OSMCrit); 
	InitializeCriticalSection(&RefreshCrit); 
	
	LoadImageFile (NULL, Symbols, &iconImage, &ImgSizeX, &ImgSizeY, &ImgChannels, &bgColor);

	if (iconImage == NULL)
	{
		Debugprintf("Creating Icon File %s", Symbols);
		
		CreateIconFile();

		LoadImageFile (NULL, Symbols, &iconImage, &ImgSizeX, &ImgSizeY, &ImgChannels, &bgColor);

		if (iconImage == NULL)
		{
			MessageBox(NULL, "Couldn't open Icon File Symbols.png", "BPQAPRS", MB_OK);
			return (FALSE);
		}
	}

	hInst = hInstance; // Store instance handle in our global variable

	hMapWnd = hWnd = CreateWindow(szAppName, szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 800, 600,
		NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return (FALSE);
	}

	if (Rect.right < 100 || Rect.bottom < 100)
	{
		GetWindowRect(hWnd,	&Rect);
	}

	MoveWindow(hWnd,Rect.left,Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

	 // setup default font information

   LFTTYFONT.lfHeight =			10;
   LFTTYFONT.lfWidth =          8 ;
   LFTTYFONT.lfEscapement =     0 ;
   LFTTYFONT.lfOrientation =    0 ;
   LFTTYFONT.lfWeight =         0 ;
   LFTTYFONT.lfItalic =         0 ;
   LFTTYFONT.lfUnderline =      0 ;
   LFTTYFONT.lfStrikeOut =      0 ;
//   LFTTYFONT.lfCharSet =        OEM_CHARSET ;
   LFTTYFONT.lfOutPrecision =   OUT_DEFAULT_PRECIS ;
   LFTTYFONT.lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
   LFTTYFONT.lfQuality =        DEFAULT_QUALITY ;
   LFTTYFONT.lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
   lstrcpy( LFTTYFONT.lfFaceName, "FIXEDSYS" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;

	CheckTimer();						// Make sure switch is initialised

	SetTimer(hWnd, 1, 1000, NULL);		// Slow Timer
	SetTimer(hWnd, 2, 100, NULL);		// Fast Timer

	SetWindowText(hWnd,szTitle);

	Sleep(3000);

	APRSConnect(APRSCall, ISFilter);					// Request frames from switch

	memcpy(LoppedAPRSCall, APRSCall, 10);

	ptr = strchr(LoppedAPRSCall, ' ');
	if (ptr) *(ptr) = 0;

	hMenu=CreateMenu();
	hPopMenu1=CreatePopupMenu();
	hPopMenu2=CreatePopupMenu();
	hPopMenu3=CreatePopupMenu();
	SetMenu(hWnd,hMenu);


	AppendMenu(hMenu, MF_STRING, IDM_CONFIG, "Basic Setup");
	AppendMenu(hMenu, MF_STRING, IDM_APRSMSGS, "Messages");
	AppendMenu(hMenu, MF_STRING, IDM_APRSSTNS, "Stations");
	AppendMenu(hMenu, MF_STRING, IDM_ZOOMIN,"Zoom In");
	AppendMenu(hMenu, MF_STRING, IDM_ZOOMOUT,"Zoom Out");
	AppendMenu(hMenu, MF_STRING + MF_POPUP, (UINT)hPopMenu1,"Actions");
	AppendMenu(hPopMenu1, MF_STRING, IDM_HOME,"Home");
	AppendMenu(hPopMenu1, MF_STRING, IDM_SENDBEACON,"Send Beacon");
	AppendMenu(hPopMenu1, MF_STRING, IDM_FILTERTOVIEW, "Set Filter to Current View");
	AppendMenu(hMenu, MF_STRING + MF_POPUP, (UINT)hPopMenu3,"Help");
	AppendMenu(hPopMenu3, MF_STRING, IDM_HELP, "Online Help");
	AppendMenu(hPopMenu3, MF_STRING, IDM_ABOUT, "About");

	DrawMenuBar(hWnd);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON

		AddTrayMenuItem(hWnd, "APRS Map");
	}
	
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

//	ScrollX = MapCentreX;
//	ScrollY = MapCentreY;
	
//	SetBaseX = long2tilex(-6.3, Zoom);
//	SetBaseY = lat2tiley(58.5, Zoom);

//	SetBaseX = long2tilex(-7.0, Zoom) - 4;
//	SetBaseY = lat2tiley(59, Zoom) - 4;				// Set Location at middle

	while(SetBaseX < 0)
	{
		SetBaseX++;
		ScrollX -= 256;
	}

	while(SetBaseY < 0)
	{
		SetBaseY++;
		ScrollY -= 256;
	}

	ReloadMaps = TRUE;

	WSAStartup(MAKEWORD(2, 0), &WsaData);
	
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(80);

	_beginthread(ResolveThread,0,0);
	_beginthread(OSMThread,0,0);

	InvalidateRect(hWnd, NULL, FALSE);

	trayMenu = CreatePopupMenu();
	AppendMenu(trayMenu,MF_STRING,40000,"Copy");
	AppendMenu(trayMenu,MF_STRING,40001,"Supress Tracks");
	AppendMenu(trayMenu,MF_STRING,40002,"Set Track Colour");

	trayMenu2 = CreatePopupMenu();
	AppendMenu(trayMenu2,MF_STRING,40000,"Copy");

	CreatePipeThread();		// Open HTTP server pipe if defined

	return (TRUE);
}


INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, ABOUT_VERSION, VersionString);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

int  SaveIntValtoReg(HWND hDlg, int Item, HKEY hKey, char * Key)
{
	int OK;
	int Val = GetDlgItemInt(hDlg, Item, &OK, FALSE);

	RegSetValueEx(hKey, Key, 0, REG_DWORD, (UCHAR *)&Val, 4);

	return Val;
}

VOID SaveFixedStringValtoReg(HWND hDlg, int Item, HKEY hKey, char * Key, char * Val, int MaxLen)
{
	GetDlgItemText(hDlg, Item, Val, MaxLen);
	RegSetValueEx(hKey, Key, 0, REG_SZ, Val, strlen(Val) + 1);
}

VOID SaveStringValtoReg(HWND hDlg, int Item, HKEY hKey, char * Key, char ** Val)
{
	int MsgLen = SendDlgItemMessage(hDlg, Item, WM_GETTEXTLENGTH, 0 ,0);

	if (*Val)
		free(*Val);

	*Val = malloc(MsgLen+1);
	GetDlgItemText(hDlg, Item, *Val, MsgLen+1);

	RegSetValueEx(hKey, Key, 0, REG_SZ, *Val, strlen(*Val) + 1);
}

INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	OPENFILENAME ofn;

	switch (message)
	{
	case WM_INITDIALOG:

		if (StatusMsg)
			SetDlgItemText(hDlg, IDC_STATUSTEXT, StatusMsg);

		SetDlgItemText(hDlg, IDC_FILTER, ISFilter);
		SetDlgItemInt(hDlg, IDC_RETRIES, RetryCount, FALSE);
		SetDlgItemInt(hDlg, IDC_RETRYTIME, RetryTimer, FALSE);
		SetDlgItemInt(hDlg, IDC_EXPIRE, ExpireTime, FALSE);
		SetDlgItemInt(hDlg, IDC_EXPIRETRACKS, TrackExpireTime, FALSE);

		SetDlgItemText(hDlg, IDC_WXFILE, WXFileName);
		SetDlgItemText(hDlg, IDC_WXTEXT, WXComment);
		SetDlgItemText(hDlg, IDC_WXPORTS, WXPortList);
		
		SetDlgItemInt(hDlg, IDC_WXINTERVAL, WXInterval, FALSE);
		CheckDlgButton(hDlg, IDC_SENDWX, SendWX);

		CheckDlgButton(hDlg, IDC_CREATEJPEG, CreateJPEG);
		SetDlgItemText(hDlg, IDC_JPEGFILE, JPEGFileName);
		SetDlgItemInt(hDlg, IDC_JPEGINTERVAL, JPEGInterval, FALSE);

		CheckDlgButton(hDlg, IDC_SUPZERO, SuppressNullPosn);
		CheckDlgButton(hDlg, IDC_NOTRACKS, DefaultNoTracks);
		CheckDlgButton(hDlg, IDC_LOCALTIME, LocalTime);	
					
		EnableWindow(GetDlgItem(hDlg, IDC_WXFILE), SendWX);
		EnableWindow(GetDlgItem(hDlg, IDC_WXTEXT), SendWX);
		EnableWindow(GetDlgItem(hDlg, IDC_WXINTERVAL), SendWX);
		EnableWindow(GetDlgItem(hDlg, IDC_FILE), SendWX);
		EnableWindow(GetDlgItem(hDlg, IDC_WXPORTS), SendWX);

		EnableWindow(GetDlgItem(hDlg, IDC_JPEGFILE), CreateJPEG);
		EnableWindow(GetDlgItem(hDlg, IDC_JPEGINTERVAL), CreateJPEG);
		EnableWindow(GetDlgItem(hDlg, IDC_FILE2), CreateJPEG);


		return (INT_PTR)TRUE;

	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDC_FILE:
		{
			memset(&ofn, 0, sizeof (OPENFILENAME));
			ofn.lStructSize = sizeof (OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = WXFileName;
			ofn.nMaxFile = 250;
			ofn.lpstrTitle = "File to send as beacon";
			ofn.lpstrInitialDir = GetBPQDirectory();

			if (GetOpenFileName(&ofn))
				SetDlgItemText(hDlg, IDC_WXFILE, WXFileName);

			break;
		}

		case IDC_FILE2:
		{
			memset(&ofn, 0, sizeof (OPENFILENAME));
			ofn.lStructSize = sizeof (OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = JPEGFileName;
			ofn.nMaxFile = 250;
			ofn.lpstrTitle = "JPEG of Screen Image File Name";
			ofn.lpstrInitialDir = GetBPQDirectory();

			if (GetOpenFileName(&ofn))
				SetDlgItemText(hDlg, IDC_JPEGFILE, JPEGFileName);

			break;
		}

		case IDC_SENDWX:
		{
			SendWX = IsDlgButtonChecked(hDlg, IDC_SENDWX);
			EnableWindow(GetDlgItem(hDlg, IDC_WXFILE), SendWX);
			EnableWindow(GetDlgItem(hDlg, IDC_WXTEXT), SendWX);
			EnableWindow(GetDlgItem(hDlg, IDC_WXINTERVAL), SendWX);
			EnableWindow(GetDlgItem(hDlg, IDC_FILE), SendWX);
			EnableWindow(GetDlgItem(hDlg, IDC_WXPORTS), SendWX);
			break;
		}

		case IDC_CREATEJPEG:
	
			CreateJPEG = IsDlgButtonChecked(hDlg, IDC_CREATEJPEG);
			EnableWindow(GetDlgItem(hDlg, IDC_JPEGFILE), CreateJPEG);
			EnableWindow(GetDlgItem(hDlg, IDC_JPEGINTERVAL), CreateJPEG);
			EnableWindow(GetDlgItem(hDlg, IDC_FILE2), CreateJPEG);

			break;
		
		case IDC_SUPZERO:
	
			SuppressNullPosn = IsDlgButtonChecked(hDlg, IDC_SUPZERO);
			break;

		case IDC_LOCALTIME:
	
			LocalTime = IsDlgButtonChecked(hDlg, IDC_LOCALTIME);
			break;

		case IDC_NOTRACKS:

			DefaultNoTracks = IsDlgButtonChecked(hDlg, IDC_NOTRACKS);
			break;

		case IDOK:
		{
			// Save Config

			HKEY hKey;
			BOOL OK;

			int retCode = RegOpenKeyEx (REGTREE, "SOFTWARE\\G8BPQ\\BPQ32\\BPQAPRS", 0, KEY_ALL_ACCESS, &hKey);

			SaveStringValtoReg(hDlg, IDC_STATUSTEXT, hKey, "StatusMsg" , &StatusMsg);
			SaveFixedStringValtoReg(hDlg, IDC_FILTER, hKey, "ISFilter", ISFilter, 999);
			SaveFixedStringValtoReg(hDlg, IDC_WXFILE, hKey, "WXFile", WXFileName, 240);
			SaveFixedStringValtoReg(hDlg, IDC_WXTEXT, hKey, "WXText", WXComment, 79);
			SaveFixedStringValtoReg(hDlg, IDC_WXPORTS, hKey, "WXPorts", WXPortList, 79);

			SaveFixedStringValtoReg(hDlg, IDC_JPEGFILE, hKey, "JPEGFile", JPEGFileName, 240);

			RetryCount = GetDlgItemInt(hDlg, IDC_RETRIES, &OK, FALSE);
			RetryTimer = GetDlgItemInt(hDlg, IDC_RETRYTIME, &OK, FALSE);
			ExpireTime = GetDlgItemInt(hDlg, IDC_EXPIRE, &OK, FALSE);
			TrackExpireTime = GetDlgItemInt(hDlg, IDC_EXPIRETRACKS, &OK, FALSE);
			WXInterval = GetDlgItemInt(hDlg, IDC_WXINTERVAL, &OK, FALSE);

			JPEGInterval = GetDlgItemInt(hDlg, IDC_JPEGINTERVAL, &OK, FALSE);

			retCode = RegSetValueEx(hKey, "RetryCount", 0, REG_DWORD, (BYTE *)&RetryCount, 4);
			retCode = RegSetValueEx(hKey, "RetryTimer", 0, REG_DWORD, (BYTE *)&RetryTimer, 4);
			retCode = RegSetValueEx(hKey, "ExpireTime", 0, REG_DWORD, (BYTE *)&ExpireTime, 4);
			retCode = RegSetValueEx(hKey, "TrackExpireTime", 0, REG_DWORD, (BYTE *)&TrackExpireTime, 4);
			retCode = RegSetValueEx(hKey, "WXInterval", 0, REG_DWORD, (BYTE *)&WXInterval, 4);
			retCode = RegSetValueEx(hKey, "WXEnabled", 0, REG_DWORD, (BYTE *)&SendWX, 4);
			retCode = RegSetValueEx(hKey, "SuppressNullPosn", 0, REG_DWORD, (BYTE *)&SuppressNullPosn, 4);
			retCode = RegSetValueEx(hKey, "DefaultNoTracks", 0, REG_DWORD, (BYTE *)&DefaultNoTracks, 4);
			retCode = RegSetValueEx(hKey, "LocalTime", 0, REG_DWORD, (BYTE *)&LocalTime, 4);
			retCode = RegSetValueEx(hKey, "CreateJPEG", 0, REG_DWORD, (BYTE *)&CreateJPEG, 4);

			APRSConnect(APRSCall, ISFilter);			// Will resend Filter

			DecodeWXPortList();

			RegCloseKey(hKey);
		}

		case IDCANCEL:
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
		}
	}
	return (INT_PTR)FALSE;
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

BOOL MouseLeftDown = FALSE;

int DownX;			// Mouse coords when left button was pressed
int DownY;

int Statwidths[] = {150, 200, 250, 300, 350, 800, -1};

LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPNMLISTVIEW pnm = (LPNMLISTVIEW)lParam;
	LPNMLVCUSTOMDRAW lplvcd;

	switch (message)
	{
		case WM_NOTIFY:
		
		switch (pnm->hdr.code)
		{
		case NM_DBLCLK:
		{
			LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
			LVITEM item = {0};
			char Selcall[20];
			int n;

			// Get Text
			
			item.iSubItem = 0;
			item.mask = LVIF_TEXT;
			item.iItem = pnm->iItem;
			item.pszText = Selcall;
			item.cchTextMax = 11;

			ListView_GetItem(pnm->hdr.hwndFrom, &item);

			// Add Call to TO Dropdown list

			// Remove trailing spaces

			n = strlen(Selcall);
			n--;

			while(n > 1)
			{
				if (Selcall[n] == ' ')
					Selcall[n] = 0;				// Remove trailing spaces
				else
					break;
			n--;
			}

			CreateMessageWindow("APRSMSGS", "APRS Messages", MsgWndProc, NULL);

			n = SendMessage(hToCall, CB_FINDSTRINGEXACT, -1, (LPARAM)(LPCTSTR)Selcall);

			if (n == CB_ERR)
				n = SendMessage(hToCall, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)Selcall);

			SendMessage(hToCall, CB_SETCURSEL, n, 0);

			break;
		}

		case NM_RCLICK:
		{
			// Bring up Copy Menu

			LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
			POINT pos;
			LVITEM item = {0};

			// Get Text
			
			item.iSubItem = pnm->iSubItem;
			item.mask = LVIF_TEXT;
			item.iItem = pnm->iItem;
			item.pszText = CopyItem;
			item.cchTextMax = 256;

			ListView_GetItem(pnm->hdr.hwndFrom, &item);

			GetCursorPos(&pos);
			if (CurrentPopup->NoTracks)
				CheckMenuItem(trayMenu, 40001, MF_CHECKED);
			else
				CheckMenuItem(trayMenu, 40001, MF_UNCHECKED);
			
			TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, hWnd, 0);

			break;
		}

		case NM_CUSTOMDRAW:
			
			lplvcd = (LPNMLVCUSTOMDRAW)lParam;

			switch(lplvcd->nmcd.dwDrawStage)
			{

			case CDDS_PREPAINT :
				return CDRF_NOTIFYITEMDRAW;
			
			case CDDS_ITEMPREPAINT:
        
				SelectObject(lplvcd->nmcd.hdc, hFont);
//				lplvcd->clrText = RGB(255,0,0); //GetColorForItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam);
//				lplvcd->clrTextBk = RGB(0,0,0);//GetBkColorForItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam);

			     return CDRF_NEWFONT;
				//  or return CDRF_NOTIFYSUBITEMREDRAW;
			
			case CDDS_SUBITEM | CDDS_ITEMPREPAINT:

				lplvcd->clrText = RGB(255,0,0); //GetColorForSubItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam, lplvcd->iSubItem);
				lplvcd->clrTextBk = RGB(0,255,0);//GetBkColorForSubItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam, lplvcd->iSubItem);

		       return CDRF_NEWFONT;  
			}


		}

		break;

			case WM_SYSCOMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			switch (wmId)
			{ 			
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
			}

		case WM_DESTROY:

			break;

		case WM_CHAR:

			if (wParam == '=' || wParam == '+')
				PostMessage(hMapWnd, WM_CHAR, '=', 0);
			else
			if (wParam == '-')
				PostMessage(hMapWnd, WM_CHAR, '-', 0);

			break;

		case 0x020A:				//WM_MOUSEWHEEL  

			if ((int)wParam > 0)
				PostMessage(hMapWnd, WM_CHAR, '=', 0);
			else
				PostMessage(hMapWnd, WM_CHAR, '-', 0);

			break;



		case WM_COMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

		if (wmId == 40000)
		{
			int len=0;
			HGLOBAL	hMem;
			char * ptr;
			int Len = strlen(CopyItem);

			// Copy Rich Text Selection to Clipboard
		
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, Len + 1);

			if (hMem != 0)
			{
				ptr=GlobalLock(hMem);
				strcpy(ptr, CopyItem);
	
				if (OpenClipboard(hWnd))
				{
					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT, hMem);
					CloseClipboard();
				}
			}
			else
				GlobalFree(hMem);

		}
		if (wmId == 40001)
		{
			CurrentPopup->NoTracks ^= 1;
			return TRUE;
		}

		if (wmId == 40002)
		{
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CHATCOLCONFIG), hWnd, ColourDialogProc);
			break;
		}
		
		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0);
}
	
HWND hwndTrack;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
//	HGLOBAL	hMem;
	int DeltaX, DeltaY;
	double MouseLat, MouseLon;
	char MouseLoc[80];
	int nScrollCode,nPos;
	char NS='N', EW='E';
	char LatString[20], LongString[20];
	int Degrees;
	double Minutes;
	char Msg[80];
	int NewZoom;

	LPNMMOUSE pnm = (LPNMMOUSE) lParam;
	struct STATIONRECORD * Station;


	switch (message)
	{
	case WM_NOTIFY:
		
		switch (pnm->hdr.code)
		{
		case NM_RELEASEDCAPTURE:

			MouseX = cxWinSize/2;
			MouseY = cyWinSize/2;
			GetMouseLatLon(&MouseLat, &MouseLon);

			NewZoom = 18 - SendMessage(hwndTrack, TBM_GETPOS, 0, 0);
			DestroyWindow(hwndTrack);

			if (NewZoom - Zoom > 8)
				NewZoom = Zoom + 8;

			while (Zoom < NewZoom)			// Zooming in
			{
				MouseX = cxWinSize/2;
				MouseY = cyWinSize/2;
				GetMouseLatLon(&MouseLat, &MouseLon);
				Zoom ++;
				CentrePositionToMouse(MouseLat, MouseLon);	
			}

			Zoom = NewZoom;

			CentrePositionToMouse(MouseLat, MouseLon);	

			ReloadMaps = TRUE;

			InvalidateRect(hWnd,NULL,FALSE);

			wsprintf(Msg, "%d", Zoom);
			SendMessage(hStatus, SB_SETTEXT, (WPARAM)(INT) 0 | 3, (LPARAM)Msg);

			return TRUE;

		case NM_CLICK:
		case NM_RCLICK:
			{
			// Right click in Status Bar. If the zoom menu, bring up a zoom selection box

			if (pnm->dwItemSpec == 3)
			{
				hwndTrack = CreateWindowEx( 
					0,							// no extended styles 
					 TRACKBAR_CLASS,			// class name 
					 "",						// title (caption) 
					WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_ENABLESELRANGE | TBS_VERT,              // style 
					260, cyWinSize - 220,		// position 
					40, 200,                    // size 
					hMapWnd,                    // parent window 
					(HMENU)999,		            // control identifier 
					hInst,					    // instance 
					NULL );                     // no WM_CREATE parameter 
 

				   SendMessage(hwndTrack, TBM_SETRANGE, 
						(WPARAM) TRUE,                   // redraw flag 
						(LPARAM) MAKELONG(2, 16));  // min. & max. positions
        
				 SendMessage(hwndTrack, TBM_SETPAGESIZE, 
					   0, (LPARAM) 4);                  // new page size 

				  SendMessage(hwndTrack, TBM_SETSEL, 
			     (WPARAM) FALSE,                  // redraw flag 
				    (LPARAM) MAKELONG(2, 16)); 
        
				  SendMessage(hwndTrack, TBM_SETPOS, 
			      (WPARAM) TRUE,                   // redraw flag 
				     (LPARAM) 18 - Zoom); 

				  SetFocus(hwndTrack); 

				//  return hwndTrack; 

				return TRUE;

			break;
		}
		}
		}

		break;

		case WM_CREATE:
	
			// Initialize common controls
			InitCommonControls();

			// Create status bar
			hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, (HMENU)100, GetModuleHandle(NULL), NULL);

			// Create status bar "compartments" one width 150, other 300, then 400... last -1 means that it fills the rest of the window

			SendMessage(hStatus, SB_SETPARTS, (WPARAM)(sizeof(Statwidths)/sizeof(int)), (LPARAM)Statwidths);
			SendMessage((HWND) hStatus, (UINT) SB_SETTEXT, (WPARAM)(INT) 0 | 0, (LPARAM) (LPSTR) TEXT("Hello"));
		
			break;


		case WM_TIMER:

			if (wParam == 1)	// wParam [in] The timer identifier.
				SecTimer();	
	
			if (wParam == 2)	// wParam [in] The timer identifier.
				APRSPoll();		// Look for messages from switch
			
			break;
		
	
		case WM_CHAR:

			GetMouseLatLon(&MouseLat, &MouseLon);

			if (wParam == '=' || wParam == '+')
				Zoom++;
			if (wParam == '-')
				Zoom--;

			if (Zoom < 2)
			{
				Zoom = 2;
				return TRUE;
			}
			if (Zoom > MaxZoom)
			{
				Zoom = MaxZoom;
				return TRUE;
			}
			
			// Centre on Current Cursor

			CentrePositionToMouse(MouseLat, MouseLon);	

			ReloadMaps = TRUE;

			InvalidateRect(hWnd,NULL,FALSE);

			wsprintf(Msg, "%d", Zoom);
			SendMessage(hStatus, SB_SETTEXT, (WPARAM)(INT) 0 | 3, (LPARAM)Msg);

			break;

		case 0x020A:				//WM_MOUSEWHEEL  

			if ((int)wParam > 0)
				PostMessage(hWnd, WM_CHAR, '=', 0);
			else
				PostMessage(hWnd, WM_CHAR, '-', 0);

			break;

		case WM_MOUSEMOVE:
			
			if (MouseX == LOWORD(lParam) || MouseY == HIWORD(lParam) + 20)
				return TRUE;		// Not Moved

			MouseX = LOWORD(lParam);
			MouseY = HIWORD(lParam) + 20;
			
			if (MouseLeftDown)
			{
				// Dragging

				DeltaX = MouseX - DownX;
				DeltaY = MouseY - DownY;

				ScrollX -= DeltaX;
				ScrollY -= DeltaY;

				if (ScrollX < 0)
				{
					// Load Previous Tile
					
					SetBaseX--;
					
					if (SetBaseX)
					{
						ReloadMaps = TRUE;
						ScrollX += 256;
					}
					else
						ScrollX = 0;

				}
				if (ScrollY < 0)
				{
					// Load Previous Tile
					
					SetBaseY--;
					
					if (SetBaseY)
					{
						ReloadMaps = TRUE;
						ScrollY += 256;
					}
					else
						ScrollY= 0;
				}

				if (ScrollX > 2048 - cxWinSize)
				{
					// Load Next Tile

					SetBaseX++;
					
//					if (SetBaseX)
					{
						ReloadMaps = TRUE;
						ScrollX -= 256;
					}
//					else
//						ScrollX = 2048 - cxWinSize;

				}

				if (ScrollY > 2048 - cyWinSize)
				{
					// Load Next Tile

					SetBaseY++;
					
//					if (SetBaseX)
					{
						ReloadMaps = TRUE;
						ScrollY -= 256;
					}
//					else
//						ScrollX = 2048 - cxWinSize;

				}

				InvalidateRect(hWnd, NULL, FALSE);

				DownX = MouseX;
				DownY = MouseY;

				break;
			}

			// Left not down - see if cursor is over a station Icon

			GetMouseLatLon(&MouseLat, &MouseLon);

			if (MouseLat < 0)
			{
				NS = 'S';
				MouseLat=-MouseLat;
			}
			if (MouseLon < 0)
			{
				EW = 'W';
				MouseLon=-MouseLon;
			}

			#pragma warning(push)
			#pragma warning(disable:4244)

			Degrees = MouseLat;
			Minutes = MouseLat * 60.0 - (60 * Degrees);

			sprintf(LatString,"%2d°%05.2f'%c",Degrees, Minutes, NS);
		
			Degrees = MouseLon;

			#pragma warning(pop)

			Minutes = MouseLon * 60 - 60 * Degrees;

			sprintf(LongString,"%3d°%05.2f'%c",Degrees, Minutes, EW);

			sprintf(MouseLoc, "%s %s", LatString, LongString);
			SendMessage(hStatus, SB_SETTEXT, (WPARAM)(INT) 0 | 0, (LPARAM)MouseLoc);

			FindStationsByPixel(MouseX + ScrollX, MouseY + ScrollY);
			
			break;
			
		case WM_LBUTTONDOWN:

			MouseLeftDown = TRUE;
			DownX = LOWORD(lParam);
			DownY = HIWORD(lParam);
			break;

		case WM_LBUTTONUP:
			
			MouseLeftDown = FALSE;
			MouseX = LOWORD(lParam);
			MouseY = HIWORD(lParam);

			break;

		case WM_VSCROLL:
		
			nScrollCode = (int) LOWORD(wParam); // scroll bar value 
			nPos = (short int) HIWORD(wParam);  // scroll box position 


			return TRUE;

			//hwndScrollBar = (HWND) lParam;      // handle of scroll bar 

				break;
	
		case WM_SIZE:
        
			cxWinSize = LOWORD (lParam);
			cyWinSize = HIWORD (lParam);

			MapCentreX = (2048 - cxWinSize) /2;
			MapCentreY = (2048 - cyWinSize) /2;

			// Auto-resize statusbar (Send WM_SIZE message does just that)

			SendMessage(hStatus, WM_SIZE, 0, 0);

			break;


		case WM_PAINT:

			if (ReloadMaps)
			{
				LoadImageSet(Zoom, SetBaseX, SetBaseY);
				
//				EnterCriticalSection(&RefreshCrit);
				RefreshStationMap();
//				LeaveCriticalSection(&RefreshCrit);

				ReloadMaps = FALSE;
			}

			cImgChannels = 3;			// We've converted it

			hdc = BeginPaint (hWnd, &ps);

	//		RefreshStationMap();
			
			DisplayImage (hWnd, &pDib, &pDiData, cxWinSize, cyWinSize,
                NULL, cxImgSize, cyImgSize, cImgChannels, bStretched);

			if (pDib)
				SetDIBitsToDevice (hdc, 0, 0, cxWinSize, cyWinSize - 20, 0, 0,
					0, cyWinSize, pDiData, (BITMAPINFO *) pDib, DIB_RGB_COLORS);

			
			EndPaint (hWnd, &ps);
			break; 

				
		case WM_SYSCOMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			switch (wmId)
			{ 	
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

			GetWindowRect(hWnd,	&Rect);	// For save soutine

			APRSDisconnect();			// Disconnect from Switch
			DestroyMenu(hMenu);

			PostQuitMessage(0);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);
			
			break;

		case WM_COMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

		if (wmId == 40000)
		{
			int len=0;
			HGLOBAL	hMem;
			char * ptr;
			int Len = strlen(CopyItem);

			// Copy Rich Text Selection to Clipboard
		
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, Len + 1);

			if (hMem != 0)
			{
				ptr=GlobalLock(hMem);
				strcpy(ptr, CopyItem);
	
				if (OpenClipboard(hWnd))
				{
					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT, hMem);
					CloseClipboard();
				}
			}
			else
				GlobalFree(hMem);

		}

		//Parse the menu selections:

			if (lParam == (LPARAM)hSelWnd)
			{
				if (wmEvent == LBN_SELCHANGE)
				{
					int Index = SendMessage(hSelWnd, LB_GETCURSEL, 0, 0);
					int TopIndex = SendMessage(hSelWnd, LB_GETTOPINDEX, 0, 0);
					char Key[20];

					if (Index != -1)
					{
						struct STATIONRECORD * Station;

						SendMessage(hSelWnd, LB_GETTEXT, Index, (LPARAM)Key); 
						Station = FindStation(Key, Key, TRUE);

						if (Station == NULL)
							return TRUE;

						DestroyWindow(hSelWnd);
						hSelWnd = 0;

						CreateStationPopup(Station, PopupX, PopupY + (Index - TopIndex) * 15);
						return TRUE;
					}
				}
			}
		
			if (wmId > BPQBASE && wmId < BPQBASE + 33)
			{
				TogglePort(hWnd,wmId,0x1 << (wmId - (BPQBASE + 1)));
				break;
			}
		
			switch (wmId)
			{

			case IDM_ABOUT:

				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;

			case IDM_HELP:

				ShellExecute(hWnd,"open",
					"http://www.cantab.net/users/john.wiseman/Documents/BPQAPRS.htm", "", NULL, SW_SHOWNORMAL); 
				break;

			case IDM_APRSMSGS:

				CreateMessageWindow("APRSMSGS", "APRS Messages", MsgWndProc, NULL);
				break;

			case IDM_APRSSTNS:

				CreateStationWindow("APRSSTNS", "APRS Stations", StnWndProc, NULL);
				break;

			case IDM_SENDBEACON:

				APISendBeacon();
				break;

			case IDM_FILTERTOVIEW:
			{
				double TLLat, TLLon, BRLat, BRLon;
				char Filter[256];

				GetCornerLatLon(&TLLat, &TLLon, &BRLat, &BRLon);
				//a/50/-130/20/-70 
				sprintf(Filter, "%s a/%.3f/%.3f/%.3f/%.3f", ISFilter, TLLat, TLLon, BRLat, BRLon);

				APRSConnect(APRSCall, Filter);			// Will resend Filter

				break;
			}

			case IDM_HOME:

				Station = *StationRecords;

				while(Station)
				{ 
					if (strcmp(Station->Callsign, LoppedAPRSCall) == 0)
					{
						CentrePosition(Station->Lat, Station->Lon);
						ShowWindow(hMapWnd, SW_SHOWNORMAL);
						SetForegroundWindow(hMapWnd);
				
						return TRUE;
					}
		
					Station = Station->Next;
				}

				return TRUE;

			case IDM_ZOOMIN:

				// Move logical mouse posn to centre of screen

				MouseX = cxWinSize/2;
				MouseY = cyWinSize/2;

				PostMessage(hMapWnd, WM_CHAR, '=', 0);
				break;

			case IDM_ZOOMOUT:

				// Move logical mouse posn to centre of screen

				MouseX = cxWinSize/2;
				MouseY = cyWinSize/2;

				PostMessage(hMapWnd, WM_CHAR, '-', 0);
				break;
	
			case IDM_CONFIG:

				DialogBox(hInst, MAKEINTRESOURCE(IDD_BASICSETUP), hWnd, ConfigWndProc);
				break;
			
/*
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
*/
			}

			default:
				return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0);
}
LRESULT CALLBACK StnWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
//	HGLOBAL	hMem;
	int nScrollCode,nPos;
	RECT rcClient;
	LPNMLISTVIEW  pnm    = (LPNMLISTVIEW)lParam;
	LPNMLVCUSTOMDRAW lplvcd;

	switch (message)
	{

	case WM_NOTIFY:
		
		switch (pnm->hdr.code)
		{
		case LVN_COLUMNCLICK:

			ListView_SortItemsEx(hStations, CompareFunc, (LPARAM) pnm->iSubItem);
			break;

		case NM_DBLCLK:
		{
			LVITEM item = {0};
			char Item1[260] = "AA";
			struct STATIONRECORD * Station;

			item.iSubItem = 0;
			item.mask = LVIF_TEXT;
			item.iItem = pnm->iItem;
			item.pszText = Item1;
			item.cchTextMax = 100;

			ListView_GetItem(hStations, &item);

			Station = FindStation(Item1, Item1, TRUE);

			if (Station)
			{
				CentrePosition(Station->Lat, Station->Lon);
				ShowWindow(hMapWnd, SW_SHOWNORMAL);
				SetForegroundWindow(hMapWnd);
			}

			break;//   lpnmitem = (LPNMITEMACTIVATE) lParam;
		}
		case NM_CUSTOMDRAW:
			
			lplvcd = (LPNMLVCUSTOMDRAW)lParam;

			switch(lplvcd->nmcd.dwDrawStage)
			{
				case CDDS_PREPAINT :
					return CDRF_NOTIFYITEMDRAW;
			
				case CDDS_ITEMPREPAINT:
        
					SelectObject(lplvcd->nmcd.hdc, hFont);
//					lplvcd->clrText = RGB(255,0,0); //GetColorForItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam);
//					lplvcd->clrTextBk = RGB(0,0,0);//GetBkColorForItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam);


					return CDRF_NEWFONT;
					//  or return CDRF_NOTIFYSUBITEMREDRAW;
			
				case CDDS_SUBITEM | CDDS_ITEMPREPAINT:

					SelectObject(lplvcd->nmcd.hdc, hFont);

					lplvcd->clrText = RGB(255,0,0); //GetColorForSubItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam, lplvcd->iSubItem);
					lplvcd->clrTextBk = RGB(0,255,0);//GetBkColorForSubItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam, lplvcd->iSubItem);

// This notification is received only if you are in report mode and
//returned CDRF_NOTIFYSUBITEMREDRAW in the previous step. At
//this point, you can change the background colors for the
//subitem and return CDRF_NEWFONT.
       
					return CDRF_NEWFONT;
			
				default:
					return (DefWindowProc(hWnd, message, wParam, lParam));
			}	

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

		}

		case WM_VSCROLL:
		
			nScrollCode = (int) LOWORD(wParam); // scroll bar value 
			nPos = (short int) HIWORD(wParam);  // scroll box position 

/*			//hwndScrollBar = (HWND) lParam;      // handle of scroll bar 

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
*/
			InvalidateRect(hWnd,NULL,FALSE);
			break;

		case WM_CHAR:

			if (wParam == '=')
				return TRUE;

			break;

		case WM_SIZING:

			GetClientRect (hWnd, &rcClient); 
	
			MoveWindow(hStations, 0, 0, rcClient.right, (rcClient.bottom), TRUE);

			break;
/*		
		case WM_SIZE:
        
			//cxWinSize = LOWORD (lParam);
			//cyWinSize = HIWORD (lParam);

			MapCentreX = (2048 - cxWinSize) /2;
			MapCentreY = (2048 - cyWinSize) /2;

			// Auto-resize statusbar (Send WM_SIZE message does just that)

			SendMessage(hStatus, WM_SIZE, 0, 0);

			break;
*/

				
		case WM_SYSCOMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			switch (wmId)
			{ 	
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

			GetWindowRect(hWnd,	&StnRect);	// For save soutine
		
			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			hStnDlg = hStations = NULL;
			
			break;

			default:
				return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0);
}


LRESULT CALLBACK MsgWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
//	HGLOBAL	hMem;
	int nScrollCode,nPos;
	RECT rcClient;
	LPNMLISTVIEW pnm = (LPNMLISTVIEW)lParam;
	LPNMLVCUSTOMDRAW lplvcd;
	int retCode,Type,Vallen, Val;
	HKEY hKey=0;


	switch (message)
	{	
	case WM_NOTIFY:
		
		switch (pnm->hdr.code)
		{
		case NM_RCLICK:
		{
			LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
			POINT pos;
			LVITEM item = {0};

			// Get Text
			
			item.iSubItem = pnm->iSubItem;
			item.mask = LVIF_TEXT;
			item.iItem = pnm->iItem;
			item.pszText = CopyItem;
			item.cchTextMax = 256;

			ListView_GetItem(pnm->hdr.hwndFrom, &item);

			GetCursorPos(&pos);
			TrackPopupMenu(trayMenu2, 0, pos.x, pos.y, 0, hWnd, 0);

			break;
		}
	
		case NM_DBLCLK:
		{
				LVITEM item = {0};
				char Item1[260] = "AA";
				int i;

				item.iSubItem = pnm->iSubItem;
				item.mask = LVIF_TEXT;
				item.iItem = pnm->iItem;
				item.pszText = Item1;
				item.cchTextMax = 100;

				ListView_GetItem(pnm->hdr.hwndFrom, &item);

				// Add Call to TO Dropdown list

				// Remove trailing spaces

				i = strlen(Item1);
				i--;

				while(i > 1)
				{
					if (Item1[i] == ' ')
						Item1[i] = 0;				// Remove trailing spaces
					else
						break;
				i--;
				}

				i = SendMessage(hToCall, CB_FINDSTRINGEXACT, -1, (LPARAM)(LPCTSTR)&Item1);

				if (i == CB_ERR)
					i = SendMessage(hToCall, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)&Item1);

				SendMessage(hToCall, CB_SETCURSEL, i, 0);
				 
				break;
		}

#ifdef APRS

		case NM_CUSTOMDRAW:
			
			lplvcd = (LPNMLVCUSTOMDRAW)lParam;

			switch(lplvcd->nmcd.dwDrawStage)
			{

			case CDDS_PREPAINT :
				return CDRF_NOTIFYITEMDRAW;
			
			case CDDS_ITEMPREPAINT:
        
				SelectObject(lplvcd->nmcd.hdc, hFont);
//				lplvcd->clrText = RGB(255,0,0); //GetColorForItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam);
//				lplvcd->clrTextBk = RGB(0,0,0);//GetBkColorForItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam);

/* At this point, you can change the background colors for the item
and any subitems and return CDRF_NEWFONT. If the list-view control
is in report mode, you can simply return CDRF_NOTIFYSUBITEMREDRAW
to customize the item's subitems individually */

			     return CDRF_NEWFONT;
				//  or return CDRF_NOTIFYSUBITEMREDRAW;
			
			case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
/*
			SelectObject(lplvcd->nmcd.hdc,
                     GetFontForSubItem(lplvcd->nmcd.dwItemSpec,
                                       lplvcd->nmcd.lItemlParam,
                                       lplvcd->iSubItem));
*/

				lplvcd->clrText = RGB(255,0,0); //GetColorForSubItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam, lplvcd->iSubItem);
				lplvcd->clrTextBk = RGB(0,255,0);//GetBkColorForSubItem(lplvcd->nmcd.dwItemSpec, lplvcd->nmcd.lItemlParam, lplvcd->iSubItem);

/* This notification is received only if you are in report mode and
returned CDRF_NOTIFYSUBITEMREDRAW in the previous step. At
this point, you can change the background colors for the
subitem and return CDRF_NEWFONT.*/
       
        return CDRF_NEWFONT;  
			}
#endif

		}


	case WM_INITDIALOG:

		retCode = RegOpenKeyEx (REGTREE, "SOFTWARE\\G8BPQ\\BPQ32\\BPQAPRS", 0, KEY_QUERY_VALUE, &hKey);

		if (retCode == ERROR_SUCCESS)
		{
			Vallen=4;
			RegQueryValueEx(hKey,"MyMessages", 0, (ULONG *)&Type, (UCHAR *)&Val, (ULONG *)&Vallen);
			CheckDlgButton(hWnd, IDC_MYMSGS, Val);

			Vallen=4;
			RegQueryValueEx(hKey,"MsgBEEP", 0, (ULONG *)&Type, (UCHAR *)&Val, (ULONG *)&Vallen);
			CheckDlgButton(hWnd, IDC_MSGBEEP, Val);

			RegCloseKey(hKey);
		}

		break;


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{
			BOOL Param;
			HKEY hKey;

		case 40000:
		{
			int len=0;
			HGLOBAL	hMem;
			char * ptr;
			int Len = strlen(CopyItem);

			// Copy Rich Text Selection to Clipboard
		
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, Len + 1);

			if (hMem != 0)
			{
				ptr=GlobalLock(hMem);
				strcpy(ptr, CopyItem);
	
				if (OpenClipboard(hWnd))
				{
					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT, hMem);
					CloseClipboard();
				}
			}
			else
				GlobalFree(hMem);

		}
		break;
		
		case IDC_MYMSGS:

			RegOpenKeyEx (REGTREE, "SOFTWARE\\G8BPQ\\BPQ32\\BPQAPRS", 0, KEY_ALL_ACCESS, &hKey);

			Param = IsDlgButtonChecked(hMsgDlg, IDC_MYMSGS);
			RegSetValueEx(hKey, "MyMessages", 0, REG_DWORD, (BYTE *)&Param, 4);

			RefreshMessages();

			break;

			// Drop Through

		case IDC_MSGBEEP:

			RegOpenKeyEx (REGTREE, "SOFTWARE\\G8BPQ\\BPQ32\\BPQAPRS", 0, KEY_ALL_ACCESS, &hKey);
			Param = IsDlgButtonChecked(hMsgDlg, IDC_MSGBEEP);
			RegSetValueEx(hKey, "MsgBEEP", 0, REG_DWORD, (BYTE *)&Param, 4);

			break;

		case IDC_CLEARMSGS:
		{
			struct APRSMESSAGE * ptr = Messages;
			struct APRSMESSAGE * last = Messages;

			while (ptr)
			{
				last = ptr;
				ptr = ptr->Next;
				free(last);
			}

			Messages = NULL;

			ptr = OutstandingMsgs;

			while (ptr)
			{
				last = ptr;
				ptr = ptr->Next;
				free(last);
			}

			OutstandingMsgs = NULL;
			SendMessage(hMsgsOut, LVM_DELETEALLITEMS, (WPARAM)0, (LPARAM) 0);

			RefreshMessages();
			break;

		}
		}
		break;

		case WM_VSCROLL:
	
			nScrollCode = (int) LOWORD(wParam); // scroll bar value 
			nPos = (short int) HIWORD(wParam);  // scroll box position 

			InvalidateRect(hWnd,NULL,FALSE);
			break;

		case WM_SIZING:

			GetClientRect (hWnd, &rcClient); 
	
			MoveWindow(hMsgsIn, 0, 30, rcClient.right, (rcClient.bottom)/2 - 40, TRUE);
			MoveWindow(hMsgsOut, 0, (rcClient.bottom)/2, rcClient.right, (rcClient.bottom)/2 - 60, TRUE);
			MoveWindow(hInput, 200, rcClient.bottom - 35 , 500, 20, TRUE);
			MoveWindow(hToLabel, 9, rcClient.bottom - 33 , 23, 18, TRUE);
			MoveWindow(hToCall, 40, rcClient.bottom - 35 , 120, 20, TRUE);
			MoveWindow(hTextLabel, 163, rcClient.bottom - 33 , 33, 18, TRUE);
			MoveWindow(hPathLabel, 710, rcClient.bottom - 33 , 33, 18, TRUE);
			MoveWindow(hPath, 750, rcClient.bottom - 33 , 150, 18, TRUE);

			break;
				
		case WM_SYSCOMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			switch (wmId)
			{ 	
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

			GetWindowRect(hWnd,	&MsgRect);	// For save soutine
		
			// Remove the subclass from the edit control. 

            SetWindowLong(hInput, GWL_WNDPROC, (LONG) wpOrigInputProc); 

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			hMsgDlg = NULL;
			hToCall = NULL;
			
			break;

			default:
				return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0);
}

int line=240;
int col=0;

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
}

VOID DecodeWXReport(struct APRSConnectionInfo * sockptr, char * WX)
{
	UCHAR * ptr = strchr(WX, '_');
	char Type;
	int Val;

	if (ptr == 0)
		return;

	sockptr->WindDirn = atoi(++ptr);
	ptr += 4;
	sockptr->WindSpeed = atoi(ptr);
	ptr += 3;
WXLoop:

	Type = *(ptr++);

	if (*ptr =='.')	// Missing Value
	{
		while (*ptr == '.')
			ptr++;

		goto WXLoop;
	}

	Val = atoi(ptr);

	switch (Type)
	{
	case 'c': // = wind direction (in degrees).	
		
		sockptr->WindDirn = Val;
		break;
	
	case 's': // = sustained one-minute wind speed (in mph).
	
		sockptr->WindSpeed = Val;
		break;
	
	case 'g': // = gust (peak wind speed in mph in the last 5 minutes).
	
		sockptr->WindGust = Val;
		break;

	case 't': // = temperature (in degrees Fahrenheit). Temperatures below zero are expressed as -01 to -99.
	
		sockptr->Temp = Val;
		break;

	case 'r': // = rainfall (in hundredths of an inch) in the last hour.
		
		sockptr->RainLastHour = Val;
		break;

	case 'p': // = rainfall (in hundredths of an inch) in the last 24 hours.

		sockptr->RainLastDay = Val;
		break;

	case 'P': // = rainfall (in hundredths of an inch) since midnight.

		sockptr->RainToday = Val;
		break;

	case 'h': // = humidity (in %. 00 = 100%).
	
		sockptr->Humidity = Val;
		break;

	case 'b': // = barometric pressure (in tenths of millibars/tenths of hPascal).

		sockptr->Pressure = Val;
		break;

	default:

		return;
	}
	while(isdigit(*ptr))
	{
		ptr++;
	}

	if (*ptr != ' ')
		goto WXLoop;
}

char HeaderTemplate[] = "Accept: */*\r\nHost: %s\r\nConnection: close\r\nContent-Length: 0\r\nUser-Agent: BPQ32(G8BPQ)\r\n\r\n";
//char Header[] = "Accept: */*\r\nHost: tile.openstreetmap.org\r\nConnection: close\r\nContent-Length: 0\r\nUser-Agent: BPQ32(G8BPQ)\r\n\r\n";

VOID OSMGet(int x, int y, int zoom)
{
	struct OSMQUEUE * OSMRec = malloc(sizeof(struct OSMQUEUE));
	
	EnterCriticalSection(&OSMCrit);

	OSMQueueCount++;

	OSMRec->Next = OSMQueue.Next;
	OSMQueue.Next = OSMRec;
	OSMRec->x = x;
	OSMRec->y = y;
	OSMRec->Zoom = zoom;

	LeaveCriticalSection(&OSMCrit);

}

VOID OSMThread()
{
	// Request a page from OSM

	char FN[MAX_PATH];
	char Tile[80];
	struct OSMQUEUE * OSMRec;
	int Zoom, x, y;

	SOCKET sock;
	SOCKADDR_IN sinx; 
	int addrlen=sizeof(sinx);
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Request[100];
	char Header[256];
	UCHAR Buffer[200000];
	int Len, InputLen = 0;
	char * ptr;
	int inptr = 0;

	while (TRUE)
	{
	while (OSMQueue.Next)
	{
		EnterCriticalSection(&OSMCrit);

		OSMRec = OSMQueue.Next;
		OSMQueue.Next = OSMRec->Next;

		OSMQueueCount--;

		LeaveCriticalSection(&OSMCrit);

		x = OSMRec->x;
		y = OSMRec->y;
		Zoom = OSMRec->Zoom;

		free(OSMRec);

//		wsprintf(Tile, "/%02d/%d/%d.png", Zoom, x, y);
//		wsprintf(Tile, "/tiles/1.0.0/sat/%02d/%d/%d.jpg", Zoom, x, y);
		wsprintf(Tile, "/tiles/1.0.0/osm/%02d/%d/%d.jpg", Zoom, x, y);

	
		wsprintf(FN, "%s/%02d/%d/%d.jpg", OSMDir, Zoom, x, y);

		if (GetFileAttributes(FN) != INVALID_FILE_ATTRIBUTES)
		{
			Debugprintf(" File %s Exists - skipping", FN);
			continue;
		}

		Len = wsprintf(Request, "GET %s HTTP/1.0\r\n", Tile);

		Debugprintf(FN);

	//   Allocate a Socket entry

		sock=socket(AF_INET,SOCK_STREAM,0);

		if (sock == INVALID_SOCKET)
  		 	return; 
 
		setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);
/*
	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//

		return; 
	}
*/

		if (connect(sock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) != 0)
		{
			err=WSAGetLastError();

			//
			//	Connect failed
			//

			break;
		}

//GET /15/15810/9778.png HTTP/1.0
//Accept: */*
//Host: tile.openstreetmap.org
//Connection: close
//Content-Length: 0
//User-Agent: APRSIS32(G8BPQ)

		InputLen = 0;
		inptr = 0;

		send(sock, Request, Len, 0);
		wsprintf(Header, HeaderTemplate, Host);
		send(sock, Header, strlen(Header), 0);

		while (InputLen != -1)
		{
			InputLen = recv(sock, &Buffer[inptr], 200000 - inptr, 0);

			if (InputLen > 0)
				inptr += InputLen;
			else
			{
				// File Complete??

				if (strstr(Buffer, " 200 OK"))
				{
					ptr = strstr(Buffer, "Content-Length:");

					if (ptr)
					{
						int FileLen = atoi(ptr + 15);
						ptr = strstr(Buffer, "\r\n\r\n");

						if (ptr)
						{
							ptr += 4;
							if (FileLen == inptr - (ptr - Buffer))
							{
								// File is OK

								int cnt;
								HANDLE Handle;

								Handle = CreateFile(FN, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
								if (Handle != INVALID_HANDLE_VALUE)
								{
									WriteFile(Handle ,ptr , FileLen, &cnt, NULL);
									SetEndOfFile(Handle);

									CloseHandle(Handle);
								}
								else
								{
									if (GetLastError() == 3)
									{
										// Invalid Path
									
										char * Dir = _strdup(FN);
										int Len = strlen(Dir);

										while (Len && Dir[Len] != '/' && Dir[Len] != '\\')
										{
											Len --;
										}
										Dir[Len] = 0;
										CreateDirectory(Dir, NULL);
										Handle = CreateFile(FN, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
										if (Handle != INVALID_HANDLE_VALUE)
										{
											WriteFile(Handle ,ptr , FileLen, &cnt, NULL);
											SetEndOfFile(Handle);

											CloseHandle(Handle);
										}
										free(Dir);
									}
								}

								Debugprintf("Tile %s Loaded", FN);
								RefreshTile(FN, Zoom, x, y);
								EnterCriticalSection(&RefreshCrit);
//								RefreshStationMap();
								LeaveCriticalSection(&RefreshCrit);

								InvalidateRect(hMapWnd, NULL, FALSE);

								break;
							}
						}
					}
				}
				Debugprintf("OSM GET Bad Response");
				wsprintf(FN, "%s/DummyTile.jpg", OSMDir);
				RefreshTile(FN, Zoom, x, y);

			
				break;
			}
		}
		closesocket(sock);
		ImageChanged = TRUE;			// DOnt refresh too often		
	}

	// Queue is empty

	Sleep(1000);
}
}


char APRSMsg[300];

VOID LoadImageTile(int Zoom, int startx, int starty, int x, int y)
{
	char FN[100];
	int i, j;
	int StartRow;
	int StartCol;
	char Tile[100];
	UCHAR * pbImage = NULL;
	int ImgChannels;
	BOOL JPG=FALSE;

	int Limit = (int)pow(2, Zoom);

	if (x < 0)
		x = x + 8;
	else
		if (x > 7)
			x = 8 - x;

	if (y < 0)
		y = y + 8;
	else
		if (y > 7)
			y = 8 - y;

	if (x < 0 || x > 8)
		x = 3;

	if (y < 0 || y > 8)
		y = 3;
	
	if ((startx + x) >= Limit || (starty + y) >= Limit || startx + x < 0 || starty + y < 0)
	{
		if (pbImage)
			free(pbImage);
		pbImage = NULL;
		goto NoFile;
	}

	// May have png or jpg tiles

	wsprintf(Tile, "/%02d/%d/%d.png", Zoom, startx + x, starty + y);
	wsprintf(FN, "%s%s", OSMDir, Tile);

	if (GetFileAttributes(FN) != INVALID_FILE_ATTRIBUTES)
		goto gotfile;

	// Try jpg

	wsprintf(Tile, "/%02d/%d/%d.jpg", Zoom, startx + x, starty + y);
	wsprintf(FN, "%s%s", OSMDir, Tile);

	JPG = TRUE;

	if (GetFileAttributes(FN) == INVALID_FILE_ATTRIBUTES)
	{
		// Not got it yet

		OSMGet(startx + x, starty + y, Zoom);
		pbImage = malloc(256 * 3 * 256);
		memset(pbImage, 0x80, 256 * 3 * 256);
		ImgChannels = 3;

		goto NoFile;
	}

gotfile:

		__try
			{
				if (JPG)
				{
					pbImage =  JpegFileToRGB(FN, &cxImgSize, &cyImgSize);
					ImgChannels = 3;
				}
				else
					LoadImageFile (NULL, FN, &pbImage, &cxImgSize, &cyImgSize, &ImgChannels, &bkgColor);

			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				LeaveCriticalSection(&OSMCrit);
				Debugprintf("Corrupt file %s", FN);
				DeleteFile(FN);

				if (pbImage)
				{
					free(pbImage);
					pbImage = NULL;
				}
					
				pbImage = malloc(256 * 3 * 256);
				memset(pbImage, 0xc0, 256 * 3 * 256);
				ImgChannels = 3;

			}

			// Copy to Image Array 3 * 256 * 256

		NoFile:
			
			StartCol = x * 768;
			StartRow = y * 256;

			if (pbImage == NULL)
			{
				pbImage = malloc(256 * 3 * 256);
				memset(pbImage, 0x40, 256 * 3 * 256);
				ImgChannels = 3;
			}
			
			for (i = 0; i < 256; i++)
			{
				for (j = 0; j < 256; j++)
				{
					memcpy(&Image[((StartRow + i) * 2048 * 3) + StartCol + 3 * j], &pbImage[ImgChannels * 256 * i + ImgChannels * j], 3);
				}
			}
			
			free(pbImage);

}

VOID LoadImageSet(int Zoom, int startx, int starty)
{
	int x, y;

	x = MapCentreX - 128 - ScrollX;	
	x = 3 - x /256;				// Posn to centre
	y = MapCentreY - 128 - ScrollY;	
	y = 3 - y /256;

	memset(Image, 0, 2048 * 3 * 2048);

/*
{
		for (x = 0; x < 4; x++)
		{
			for (y = 0; y < 4; y++)
			{
				LoadImageTile(2, 0, 0, x, y);
			}
		}

		return;
	}
*/
	LoadImageTile(Zoom, startx, starty, x+3, y-4);
	LoadImageTile(Zoom, startx, starty, x-3, y-4);
	LoadImageTile(Zoom, startx, starty, x+2, y-4);
	LoadImageTile(Zoom, startx, starty, x-2, y-4);
	LoadImageTile(Zoom, startx, starty, x+1, y-4);
	LoadImageTile(Zoom, startx, starty, x-1, y-4);
	LoadImageTile(Zoom, startx, starty, x, y-4);


	LoadImageTile(Zoom, startx, starty, x-4, y-4);
	LoadImageTile(Zoom, startx, starty, x-4, y+3);
	LoadImageTile(Zoom, startx, starty, x-4, y-3);
	LoadImageTile(Zoom, startx, starty, x-4, y+2);
	LoadImageTile(Zoom, startx, starty, x-4, y-2);
	LoadImageTile(Zoom, startx, starty, x-4, y+1);
	LoadImageTile(Zoom, startx, starty, x-4, y-1);
	LoadImageTile(Zoom, startx, starty, x-4, y);

	LoadImageTile(Zoom, startx, starty, x+2, y+3);
	LoadImageTile(Zoom, startx, starty, x-2, y+3);
	LoadImageTile(Zoom, startx, starty, x+1, y+3);
	LoadImageTile(Zoom, startx, starty, x-1, y+3);
	LoadImageTile(Zoom, startx, starty, x, y+3);


	LoadImageTile(Zoom, startx, starty, x+3, y+3);
	LoadImageTile(Zoom, startx, starty, x+3, y+2);
	LoadImageTile(Zoom, startx, starty, x+3, y-2);
	LoadImageTile(Zoom, startx, starty, x+3, y+1);
	LoadImageTile(Zoom, startx, starty, x+3, y-1);
	LoadImageTile(Zoom, startx, starty, x+3, y);


	LoadImageTile(Zoom, startx, starty, x+3, y-3);
	LoadImageTile(Zoom, startx, starty, x+2, y-3);
	LoadImageTile(Zoom, startx, starty, x-2, y-3);
	LoadImageTile(Zoom, startx, starty, x+1, y-3);
	LoadImageTile(Zoom, startx, starty, x-1, y-3);
	LoadImageTile(Zoom, startx, starty, x, y-3);


	LoadImageTile(Zoom, startx, starty, x-3, y+3);
	LoadImageTile(Zoom, startx, starty, x-3, y-3);
	LoadImageTile(Zoom, startx, starty, x-3, y+2);
	LoadImageTile(Zoom, startx, starty, x-3, y-2);
	LoadImageTile(Zoom, startx, starty, x-3, y+1);
	LoadImageTile(Zoom, startx, starty, x-3, y-1);
	LoadImageTile(Zoom, startx, starty, x-3, y);

	LoadImageTile(Zoom, startx, starty, x-1, y+2);
	LoadImageTile(Zoom, startx, starty, x, y+2);
	LoadImageTile(Zoom, startx, starty, x+1, y+2);

	LoadImageTile(Zoom, startx, starty, x+2, y-1);
	LoadImageTile(Zoom, startx, starty, x+2, y);
	LoadImageTile(Zoom, startx, starty, x+2, y+1);
	LoadImageTile(Zoom, startx, starty, x+2, y+2);

	LoadImageTile(Zoom, startx, starty, x-1, y-2);
	LoadImageTile(Zoom, startx, starty, x, y-2);
	LoadImageTile(Zoom, startx, starty, x+1, y-2);
	LoadImageTile(Zoom, startx, starty, x+2, y-2);

	LoadImageTile(Zoom, startx, starty, x-2, y-2);
	LoadImageTile(Zoom, startx, starty, x-2, y-1);
	LoadImageTile(Zoom, startx, starty, x-2, y);
	LoadImageTile(Zoom, startx, starty, x-2, y+1);
	LoadImageTile(Zoom, startx, starty, x-2, y+2);


	LoadImageTile(Zoom, startx, starty, x+1, y-1);
	LoadImageTile(Zoom, startx, starty, x, y-1);
	LoadImageTile(Zoom, startx, starty, x-1, y + 1);
	LoadImageTile(Zoom, startx, starty, x-1, y);
	LoadImageTile(Zoom, startx, starty, x-1, y-1);
	LoadImageTile(Zoom, startx, starty, x+1, y+1);
	LoadImageTile(Zoom, startx, starty, x, y+1);
	LoadImageTile(Zoom, startx, starty, x+1, y);
	LoadImageTile(Zoom, startx, starty, x, y);

}

VOID RefreshTile(char * FN, int TileZoom, int Tilex, int Tiley)
{
	// Called when a new tile has been diwnloaded from OSM

	int StartRow, StartCol;
	UCHAR * pbImage = NULL;
	int x, y, i, j;
	int ImgChannels;

	if (TileZoom != Zoom)
		return;					// Zoom level has changed

	x = Tilex - SetBaseX;
	y = Tiley -  SetBaseY;

	if (x < 0 || x > 7 || y < 0 || y > 7)	
		return;					// Tile isn't part of current image;

	__try
	{				
//		LoadImageFile (NULL, FN, &pbImage, &cxImgSize, &cyImgSize, &ImgChannels, &bkgColor);
		pbImage =  JpegFileToRGB(FN, &cxImgSize, &cyImgSize);
		ImgChannels = 3;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		LeaveCriticalSection(&OSMCrit);
		Debugprintf("Corrupt file %s", FN);
		DeleteFile(FN);

		if (pbImage)
		{
			free(pbImage);
			pbImage = NULL;
		}

		pbImage = malloc(256 * 3 * 256);
		memset(pbImage, 0x40, 256 * 3 * 256);
		ImgChannels = 3;
	}

	// Copy to Image Array 3 * 256 * 256
	
	StartCol = x * 768;
	StartRow = y * 256;
		
	if (pbImage == NULL)
	{
		pbImage = malloc(256 * 3 * 256);
		memset(pbImage, 0x40, 256 * 3 * 256);
		ImgChannels = 3;
	}

	for (i = 0; i < 256; i++)
	{
		for (j = 0; j < 256; j++)
		{
			memcpy(&Image[((StartRow + i) * 2048 * 3) + StartCol + 3 * j], &pbImage[ImgChannels * 256 * i + ImgChannels * j], 3);
		}
	}

	free(pbImage);

	ImgChannels = 3; // We've changed it if neccessary
}

BOOL LoadImageFile (HWND hwnd, PTSTR pstrPathName,
                png_byte **ppbImage, int *pxImgSize, int *pyImgSize,
                int *piChannels, png_color *pBkgColor)
{
    static TCHAR szTmp [MAX_PATH];

    // if there's an existing PNG, free the memory

    if (*ppbImage)
    {
        free (*ppbImage);
        *ppbImage = NULL;
    }

    // Load the entire PNG into memory

 //   SetCursor (LoadCursor (NULL, IDC_WAIT));
 //   ShowCursor (TRUE);

	EnterCriticalSection(&OSMCrit);

    PngLoadImage (pstrPathName, ppbImage, pxImgSize, pyImgSize, piChannels,
                  pBkgColor);

	LeaveCriticalSection(&OSMCrit);

 //   ShowCursor (FALSE);
 //   SetCursor (LoadCursor (NULL, IDC_ARROW));

    if (*ppbImage != NULL)
    {
  //      sprintf (szTmp, "VisualPng - %s", strrchr(pstrPathName, '\\') + 1);
   //     SetWindowText (hwnd, szTmp);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

//----------------
//  DisplayImage
//----------------

BOOL DisplayImage (HWND hwnd, BYTE **ppDib,
        BYTE **ppDiData, int cxWinSize, int cyWinSize,
        BYTE *pbImage, int cxImgSize, int cyImgSize, int cImgChannels,
        BOOL bStretched)
{
    BYTE                       *pDib = *ppDib;
    BYTE                       *pDiData = *ppDiData;
    // BITMAPFILEHEADER        *pbmfh;
    BITMAPINFOHEADER           *pbmih;
    WORD                        wDIRowBytes;
    png_color                   bkgBlack = {0, 0, 0};
    png_color                   bkgGray  = {127, 127, 127};
    png_color                   bkgWhite = {255, 255, 255};

    // allocate memory for the Device Independant bitmap

    wDIRowBytes = (WORD) ((3 * cxWinSize + 3L) >> 2) << 2;

    if (pDib)
    {
        free (pDib);
        pDib = NULL;
    }

    if (!(pDib = (BYTE *) malloc (sizeof(BITMAPINFOHEADER) +
        wDIRowBytes * cyWinSize)))
    {
        *ppDib = pDib = NULL;
        return FALSE;
    }
    *ppDib = pDib;
    memset (pDib, 0, sizeof(BITMAPINFOHEADER));

    // initialize the dib-structure

    pbmih = (BITMAPINFOHEADER *) pDib;
    pbmih->biSize = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth = cxWinSize;
    pbmih->biHeight = -((long) cyWinSize);
    pbmih->biPlanes = 1;
    pbmih->biBitCount = 24;
    pbmih->biCompression = 0;
    pDiData = pDib + sizeof(BITMAPINFOHEADER);
    *ppDiData = pDiData;

    // first fill bitmap with gray and image border

    InitBitmap (pDiData, cxWinSize, cyWinSize);

    // then fill bitmap with image

//	for (x = 0; x < 8; x++)
//	{
//		for (y = 0; y < 8; y++)
//		{
//			FillBitmap (x,y);
//		}
//	}

	FillBitmap(0,0);

	return TRUE;
}

//--------------
//  InitBitmap
//--------------

BOOL InitBitmap (BYTE *pDiData, int cxWinSize, int cyWinSize)
{
    BYTE *dst;
    int x, y, col;

    // initialize the background with gray

    dst = pDiData;
    for (y = 0; y < cyWinSize; y++)
    {
        col = 0;
        for (x = 0; x < cxWinSize; x++)
        {
            // fill with GRAY
            *dst++ = 127;
            *dst++ = 127;
            *dst++ = 127;
            col += 3;
        }
        // rows start on 4 byte boundaries
        while ((col % 4) != 0)
        {
            dst++;
            col++;
        }
    }

    return TRUE;
}

//--------------
//  FillBitmap
//--------------


BOOL FillBitmap (int cx, int cy)
{
    BYTE *src, *dst;
    BYTE r, g, b, a;
    const int cDIChannels = 3;
    WORD wImgRowBytes;
    WORD wDIRowBytes;
    int xWin, yWin;
    int xImg, yImg;
	int cxImgPos, cyImgPos;
	
	cxImgPos = cx * 256;
	cxImgPos += MARGIN;
	cyImgPos = cy * 256;
	cyImgPos += MARGIN;

       // calculate both row-bytes

        wImgRowBytes = cImgChannels * 2048;
        wDIRowBytes = (WORD) ((cDIChannels * cxWinSize + 3L) >> 2) << 2;

        // copy image to screen

        for (yImg = 0, yWin = cyImgPos; yImg < cyWinSize; yImg++, yWin++)
        {
            if (yWin >= cyWinSize - MARGIN)
                break;

			if (yImg + ScrollY > 2048)
				break;

            src = Image + yImg * wImgRowBytes + ScrollY * 6144 + ScrollX * 3;

			if (src < &Image[0])
				continue;

            dst = pDiData + yWin * wDIRowBytes + cxImgPos * cDIChannels;

            for (xImg = 0, xWin = cxImgPos; xImg < 2048; xImg++, xWin++)
            {
                if (xWin >= cxWinSize - MARGIN)
                    break;
                r = *src++;
                g = *src++;
                b = *src++;
                *dst++ = b; /* note the reverse order */
                *dst++ = g;
                *dst++ = r;
                if (cImgChannels == 4)
                {
                    a = *src++;
                }
				if (src > &Image[2048 * 3 * 2048])
					continue;

            }
        }
    
    return TRUE;
}

Myabort()
{
	int i = 10;
	int j = 0;

	i /= j;				// Force PE to trigger __except
	return 0;
}

//	if (GetLocPixels(58.47583, -6.21151, &X, &Y))


VOID CALLBACK LineDDAProc(int X, int Y, LPARAM lpData)
{
	char * nptr;
	int i, j;
	COLORREF rgb = (COLORREF)lpData;

	nptr = &Image[(Y * 2048 * 3) + (X * 3)];

	for (j = 0; j < 2; j++)
	{
		for (i = 0; i < 2; i++)
		{
			*(nptr++) = GetRValue(rgb);
			*(nptr++) = LOBYTE((rgb >> 8));
			*(nptr++) = LOBYTE((rgb >> 16));
		}
		nptr += 6144 - 6;
	}
}

VOID DrawStation(struct STATIONRECORD * ptr)
{
	int X, Y, Pointer, i, c, index, bit, mask, calllen;
	UINT j;
	char Overlay;
	char * nptr;
	time_t AgeLimit = time(NULL ) - (TrackExpireTime * 60);

	if (ptr->Moved == 0)
		return;				// No need to repaint

	if (SuppressNullPosn && ptr->Lat == 0.0)
		return;

	if (ptr->ObjState == '_')	// Killed Object
		return;

	if (GetLocPixels(ptr->Lat, ptr->Lon, &X, &Y))
	{
		if (X < 8 || Y < 8 || X > 2030 || Y > 2030)
			return;				// Too close to edges

		if (ptr->LatTrack[0] && ptr->NoTracks == FALSE)
		{
			// Draw Track

			int Index = ptr->Trackptr;
			int i, n;
			int X, Y;
			int LastX = 0, LastY = 0;

			__try
			{
			for (n = 0; n < TRACKPOINTS; n++)
			{
				if (ptr->LatTrack[Index] && ptr->TrackTime[Index] > AgeLimit)
				{
					if (GetLocPixels(ptr->LatTrack[Index], ptr->LonTrack[Index], &X, &Y))
					{
						if (LastX)
						{
							if (abs(X - LastX) < 600 && abs(Y - LastY) < 600)
								LineDDA(LastX, LastY, X, Y, LineDDAProc, (LPARAM) Colours[ptr->TrackColour]);
						}

						LastX = X;
						LastY = Y;
								
						nptr = &Image[(Y * 2048 * 3) + (X * 3)];

						for (j = 0; j < 4; j++)
						{
							for (i = 0; i < 12; i++)
							{
								*(nptr++) = 0;
							}
							nptr += 6144 - 12;
						}
					}
				}
				Index++;
				if (Index == TRACKPOINTS)
					Index = 0;
			}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}
		}

		ptr->DispX = X;
		ptr->DispY = Y;					// Save for mouse over checks

		// X and Y are offsets into the pixel data in array Image. Actual Bytes are at Y * 2048 * 3 + (X * 3)

		// Draw Icon

		if (Y < 8) Y = 8;
		if (X < 8) X = 8;
		
		nptr = &Image[((Y -8) * 2048 * 3) + (X * 3) - 24];

		j = ptr->iconRow * 21 * 337 * 3 + ptr->iconCol * 21 * 3 + 9 + 337 * 9;
		for (i = 0; i < 16; i++)
		{
			memcpy(nptr, &iconImage[j], 16 * 3);
			nptr += 6144;
			j += 337 * 3;
		}

		// If an overlay is specified, add it

		Overlay = ptr->IconOverlay;

		if (Overlay)
		{
			Pointer = ((Y - 4) * 2048 * 3) + (X * 3) - 9;
			mask = 1;

			for (index = 0 ; index < 7 ; index++)
			{
				Image[Pointer++] = 255;				// Blank line above chars 
				Image[Pointer++] = 255;
				Image[Pointer++] = 255;
			}
			Pointer += 2041 * 3;

			for (i = 0; i < 7; i++)
			{
				Image[Pointer++] = 255;				// Blank col 
				Image[Pointer++] = 255;
				Image[Pointer++] = 255;

				for (index = 0 ; index < 5 ; index++)
				{
					c = ASCII[Overlay - 0x20][index];	// Font data
					bit = c & mask;


					if (bit)
					{
						Image[Pointer++] = 0;
						Image[Pointer++] = 0;
						Image[Pointer++] = 0;
					}
					else
					{
						Image[Pointer++] = 255;
						Image[Pointer++] = 255;
						Image[Pointer++] = 255;
					}
				}
				Image[Pointer++] = 255;				// Blank col 
				Image[Pointer++] = 255;
				Image[Pointer++] = 255;

				mask <<= 1;
				Pointer += 2041 * 3;
			}
			for (index = 0 ; index < 7 ; index++)
			{
				Image[Pointer++] = 255;				// Blank line above chars 
				Image[Pointer++] = 255;
				Image[Pointer++] = 255;
			}
			Pointer += 2041 * 3;
		}
		
		calllen = strlen(ptr->Callsign) * 6 + 4;
	
		// Draw Callsign Box

		Pointer = ((Y - 7) * 2048 * 3) + (X * 3) + 30;

		for (j = 0; j < 13; j++)
		{
			Image[Pointer++] = 0;
			Image[Pointer + calllen*3] = 0;
			Image[Pointer++] = 0;
			Image[Pointer + calllen*3] = 0;
			Image[Pointer++] = 0;
			Image[Pointer + calllen*3] = 0;
			Pointer += 2047 * 3;
		}

		for (i = 0; i < calllen; i++)
		{
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
		}

		Pointer = ((Y - 7) * 2048 * 3) + (X * 3) + 30;

		for (i = 0; i < calllen; i++)
		{
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
		}

		// Draw Callsign. 

		// Font is 5 bits wide x 8 high. Each byte of font contains one column, so 5 bytes per char

		for (j = 0; j < strlen(ptr->Callsign); j++)
		{

		Pointer = ((Y - 5) * 2048 * 3) + (X * 3) + 36 + (j * 18);

		mask = 1;

		for (i = 0; i < 2; i++)
		{
			for (index = 0 ; index < 6 ; index++)
			{
				Image[Pointer++] = 255;				// Blank lines above chars between chars
				Image[Pointer++] = 255;
				Image[Pointer++] = 255;
			}
			Pointer += 2042 * 3;
		}

	//	Pointer = ((Y - 3) * 2048 * 3) + (X * 3) + 36 + (j * 18);

		for (i = 0; i < 7; i++)
		{
			Image[Pointer++] = 255;				// Blank col between chars
			Image[Pointer++] = 255;
			Image[Pointer++] = 255;

			for (index = 0 ; index < 5 ; index++)
			{
				c = ASCII[ptr->Callsign[j] - 0x20][index];	// Font data
				bit = c & mask;

				if (bit)
				{
					Image[Pointer++] = 0;
					Image[Pointer++] = 0;
					Image[Pointer++] = 0;
				}
				else
				{
					Image[Pointer++] = 255;
					Image[Pointer++] = 255;
					Image[Pointer++] = 255;
				//	Image[Pointer] = Image[Pointer] * 2; //255;
				//	Pointer++;
				//	Image[Pointer] = Image[Pointer] * 2; //255;
				//	Pointer++;
				//	Image[Pointer] = Image[Pointer] * 2; //255;
				//	Pointer++;
				}
			}
			mask <<= 1;
			Pointer += 2042 * 3;
		}
		
	//	Pointer = ((Y - 3) * 2048 * 3) + (X * 3) + 36 + (j * 18);

		mask = 1;

		for (i = 0; i < 2; i++)
		{
			for (index = 0 ; index < 6 ; index++)
			{
				Image[Pointer++] = 255;				// Blank lines below chars between chars
				Image[Pointer++] = 255;
				Image[Pointer++] = 255;
			}
			Pointer += 2042 * 3;
		}

	}
		ImageChanged = TRUE;
	}
	else
	{
		ptr->DispX = 0;
		ptr->DispY = 0;			// Off Screen
	}
}

VOID CreateStationPopup(struct STATIONRECORD * ptr, int X, int Y)
{
	char Msg[80];
	struct tm * TM;
	LV_ITEM Item;
	LV_COLUMN Column;
	HWND hPopupList;
	RECT Rect;
	int Len = 130;

	CurrentPopup = ptr;
		
	if (LocalTime)
		TM = localtime(&ptr->TimeLastUpdated);
	else
		TM = gmtime(&ptr->TimeLastUpdated);

	wsprintf(Msg, "Last Heard: %.2d:%.2d:%.2d on Port %d",
		TM->tm_hour, TM->tm_min, TM->tm_sec, (LPARAM)ptr->LastPort);

	hPopupWnd = CreateDialog(hInst, "STNPOPUP", hMapWnd, NULL);

	hPopupList = GetDlgItem(hPopupWnd, IDC_LIST1);

//		hPopupWnd = CreateWindow(WC_LISTVIEW, "Messages",
  //              WS_CHILD | WS_BORDER | LVS_REPORT |  WS_VSCROLL |LVS_NOCOLUMNHEADER,
    //            X, Y, 400, 150, hMapWnd, NULL, hInst, NULL);

	GetWindowRect(hMapWnd, &Rect);

	Column.cx=1000;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText=" ";
	SendMessage(hPopupList,LVM_INSERTCOLUMN,1,(LPARAM) &Column); 

	Item.mask=LVIF_TEXT;
	Item.iItem=0;
	Item.iSubItem=0;
	Item.pszText = ptr->Callsign;
	ListView_InsertItem(hPopupList, &Item);	 

	Item.iItem++;
	Item.pszText = ptr->Path;
	ListView_InsertItem(hPopupList, &Item);	 

	Item.iItem++;
	Item.pszText = ptr->LastPacket;
	ListView_InsertItem(hPopupList, &Item);	 

	Item.iItem++;
	Item.pszText = ptr->Status;
	ListView_InsertItem(hPopupList, &Item);	 

	Item.iItem++;
	Item.pszText = Msg;
	ListView_InsertItem(hPopupList, &Item);	 

	sprintf(Msg, "Distance %6.1f Bearing %3.0f Course %1.0f Speed %3.1f",
		Distance(ptr->Lat, ptr->Lon),
		Bearing(ptr->Lat, ptr->Lon), ptr->Course, ptr->Speed);

	Item.iItem++;
	ListView_InsertItem(hPopupList, &Item);	

	if (ptr->LastWXPacket[0])
	{
		//display wx info

		struct APRSConnectionInfo temp;

		memset(&temp, 0, sizeof(temp));

		DecodeWXReport(&temp, ptr->LastWXPacket);

		sprintf(Msg, "Wind Speed %d MPH", temp.WindSpeed);
		Item.iItem++;
		ListView_InsertItem(hPopupList, &Item);	

		sprintf(Msg, "Wind Gust %d MPH", temp.WindGust);
		Item.iItem++;
		ListView_InsertItem(hPopupList, &Item);	

		sprintf(Msg, "Wind Direction %d°", temp.WindDirn);
		Item.iItem++;
		ListView_InsertItem(hPopupList, &Item);	

		sprintf(Msg, "Temperature %d°F", temp.Temp);
		Item.iItem++;
		ListView_InsertItem(hPopupList, &Item);	

		sprintf(Msg, "Pressure %05.1f", temp.Pressure /10.0);
		Item.iItem++;
		ListView_InsertItem(hPopupList, &Item);	

		sprintf(Msg, "Humidity %d%%", temp.Humidity);
		Item.iItem++;
		ListView_InsertItem(hPopupList, &Item);	

		Len +=100;

	}

	MoveWindow(hPopupWnd, Rect.left + X, Rect.top + Y + 40, 400, Len, TRUE);
	MoveWindow(hPopupList, 0, 0, 400, Len, TRUE);
	

/*
<td>Rain last hour</td><td>##RAIN_HOUR_IN##"</td></tr>
<tr><td>Rain today</td><td>##RAIN_TODAY_IN##"</td></tr>
<tr><td>Rain last 24 hours</td><td>##RAIN_24_IN##"</td></tr>
</table>
*/

	ShowWindow(hPopupWnd, SW_SHOWNORMAL);
}

VOID FindStationsByPixel(int MouseX, int MouseY)
{
	int j=0;
	struct STATIONRECORD * ptr = *StationRecords;
	struct STATIONRECORD * List[1000];

	while(ptr && j < 999)
	{	
		if (abs((ptr->DispX - MouseX)) < 8 && abs((ptr->DispY - MouseY)) < 8)
			List[j++] = ptr;

		ptr = ptr->Next;
	}

	if (j == 0)
	{
		if (hPopupWnd)
		{
			DestroyWindow(hPopupWnd);
			hPopupWnd = 0;
		}

		if (hSelWnd)
		{
			DestroyWindow(hSelWnd);
			hSelWnd = 0;
		}
		
		return;

	}

	//	If only one, display info popup, else display selection popup 

	if (hPopupWnd || hSelWnd)
		return;						// Already on display


	if (j == 1)
	{
		int PopupLeft = MouseX - ScrollX - 10;
		int PopupTop = MouseY - ScrollY - 30;

		if (PopupLeft + 400 > cxWinSize)
			PopupLeft = cxWinSize - 405;

		if (PopupTop + 150> cyWinSize)
			PopupTop= cyWinSize - 165;

		CreateStationPopup(List[0], PopupLeft, PopupTop);
	}
	else
	{
		PopupX = MouseX - ScrollX - 10;
		PopupY = MouseY - ScrollY - 30;

		if (PopupX + 150 > cxWinSize)
			PopupX = cxWinSize - 155;

		if (PopupY + 150 > cyWinSize)
			PopupY = cyWinSize - 155;
		
		hSelWnd = CreateWindow("LISTBOX", "", WS_CHILD | WS_BORDER | WS_VSCROLL |
			WS_HSCROLL | LBS_NOTIFY,
			PopupX, PopupY, 150, 150, hMapWnd, NULL, hInst, NULL);

		for (; j > 0; j--)
		{	
			SendMessage(hSelWnd, LB_ADDSTRING, 0, (LPARAM)List[j-1]->Callsign);
		}
		ShowWindow(hSelWnd, SW_SHOWNORMAL);

		PopupX = MouseX - ScrollX - 10;

		if (PopupX + 400 > cxWinSize)
			PopupX = cxWinSize - 405;


	}
}

void RefreshStation(struct STATIONRECORD * ptr)
{
	LV_ITEM Item;
	LVFINDINFO Finfo;
	int ret, n;
	double Lat = ptr->Lat;
	double Lon = ptr->Lon;
	char NS='N', EW='E';
	char LatString[20], LongString[20], DistString[20], BearingString[20];
	int Degrees;
	double Minutes;
	char Time[80];
	struct tm * TM;

	if (hStations == 0)
		return;

	if (SuppressNullPosn && ptr->Lat == 0.0)
		return;

	if (ptr->ObjState == '_')	// Killed Object
		return;

	if (LocalTime)
		TM = localtime(&ptr->TimeLastUpdated);
	else
		TM = gmtime(&ptr->TimeLastUpdated);

#ifdef _DEBUG	
	wsprintf(Time, "%.2d:%.2d:%.2d TP %d",
			TM->tm_hour, TM->tm_min, TM->tm_sec, ptr->Trackptr);
#else
	wsprintf(Time, "%.2d:%.2d:%.2d",
			TM->tm_hour, TM->tm_min, TM->tm_sec);
#endif

	Finfo.flags = LVFI_STRING;
	Finfo.psz = ptr->Callsign;
	Finfo.vkDirection = VK_DOWN;
	ret = SendMessage(hStations, LVM_FINDITEM, (WPARAM)-1, (LPARAM) &Finfo);

	if (ret == -1)
	{
		n = ListView_GetItemCount(hStations);
		ptr->Index = n;
	}
	else
		ptr->Index = ret;

	if (Lat < 0)
	{
		NS = 'S';
		Lat=-Lat;
	}
	if (Lon < 0)
	{
		EW = 'W';
		Lon=-Lon;
	}

#pragma warning(push)
#pragma warning(disable:4244)

	Degrees = Lat;
	Minutes = Lat * 60.0 - (60 * Degrees);

	sprintf(LatString,"%2d°%05.2f'%c",Degrees, Minutes, NS);
		
	Degrees = Lon;

#pragma warning(pop)

	Minutes = Lon * 60 - 60 * Degrees;

	n = sprintf(LongString,"%3d°%05.2f'%c",Degrees, Minutes, EW);

	sprintf(DistString, "%6.1f", Distance(ptr->Lat, ptr->Lon));
	sprintf(BearingString, "%3.0f", Bearing(ptr->Lat, ptr->Lon));
	
	if (n > 12)
		n++;

	Item.mask=LVIF_TEXT;
	Item.iItem = ptr->Index;
	Item.iSubItem = 0;
	Item.pszText = ptr->Callsign;

	ret = SendMessage(hStations, LVM_SETITEMTEXT, (WPARAM)ptr->Index, (LPARAM) &Item);

	if (ret == 0)
		ptr->Index = ListView_InsertItem(hStations, &Item);	 
			
	OurSetItemText(hStations, ptr->Index, 1, LatString);
	OurSetItemText(hStations, ptr->Index, 2, LongString);
	OurSetItemText(hStations, ptr->Index, 3, DistString);
	OurSetItemText(hStations, ptr->Index, 4, BearingString);
	OurSetItemText(hStations, ptr->Index, 5, Time);
	OurSetItemText(hStations, ptr->Index, 6, ptr->LastPacket);
}

void RefreshStationList()
{
	struct STATIONRECORD * ptr = *StationRecords;
	struct STATIONRECORD * last = NULL;
	int i = 0;

	SendMessage(hStations, LVM_DELETEALLITEMS, (WPARAM)0, (LPARAM) 0);
	 
	while (ptr)
	{
		RefreshStation(ptr);
		ptr = ptr->Next;
		i++;
	}
	
	SendMessage(hStations, LVM_ENSUREVISIBLE, (WPARAM)i, (LPARAM) 0);
}

void RefreshStationMap()
{
	struct STATIONRECORD * ptr = *StationRecords;
	
	int i = 0;

	while (ptr)
	{
		ptr->Moved = TRUE;				// Refreshing, so need to redraw all stations
		DrawStation(ptr);
		i++;
		ptr = ptr->Next;
	}

	NeedRefresh = FALSE;
	LastRefresh = time(NULL);

//	if (RecsDeleted)
		RefreshStationList();

	StationCount = i;

//	Debugprintf("APRS Refresh - Sation Count = %d", StationCount);
}

struct STATIONRECORD * FindStation(char * ReportingCall, char * Call, BOOL AddIfNotFount)
{
	int i = 0;
	struct STATIONRECORD * find = *StationRecords;
	struct STATIONRECORD * ptr;
	struct STATIONRECORD * last = NULL;
	int sum = 0;

	while(find)
	{ 
	    if (strcmp(find->Callsign, Call) == 0)
			return find;

		last = find;
		find = find->Next;
		i++;
	}
 
	//   Not found - add on end

	//	Cant add, as table is in BPQ32.dll

	return NULL;

	if (AddIfNotFount)
	{
		ptr = malloc(sizeof(struct STATIONRECORD));
		memset(ptr, 0, sizeof(struct STATIONRECORD));

		if (ptr == NULL) return NULL;
	
		EnterCriticalSection(&Crit);

		if (*StationRecords == NULL)
			*StationRecords = ptr;
		else
			last->Next = ptr;

		StationCount++;

		LeaveCriticalSection(&Crit);

		//	Debugprintf("APRS Add Stn %s Station Count = %d", Call, StationCount);
       
		strcpy(ptr->Callsign, Call);
		ptr->TimeAdded = time(NULL);
		ptr->Index = i;
		ptr->NoTracks = DefaultNoTracks;

		for (i = 0; i < 9; i++)
			sum += Call[i];

		sum %= 20;

		ptr->TrackColour = sum;

		return ptr;
	}
	else
		return NULL;
}

/*

INT_PTR CALLBACK ChildDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	This processes messages from controls on the tab subpages
	int Command;

//	int retCode, disp;
//	char Key[80];
//	HKEY hKey;
//	BOOL OK;
//	OPENFILENAME ofn;
//	char Digis[100];

	int Port = PortNum[CurrentPage];

	switch (message)
	{
	case WM_NOTIFY:

        switch (((LPNMHDR)lParam)->code)
        {
		case TCN_SELCHANGE:
			 OnSelChanged(hDlg);
				 return TRUE;
         // More cases on WM_NOTIFY switch.
		case NM_CHAR:
			return TRUE;
        }

       break;
	case WM_INITDIALOG:
		OnChildDialogInit( hDlg);
		return (INT_PTR)TRUE;

	case WM_CTLCOLORDLG:

        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }


	case WM_COMMAND:

		Command = LOWORD(wParam);

		if (Command == 2002)
			return TRUE;

		switch (Command)
		{
/*			case IDC_FILE:

			memset(&ofn, 0, sizeof (OPENFILENAME));
			ofn.lStructSize = sizeof (OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = &FN[Port][0];
			ofn.nMaxFile = 250;
			ofn.lpstrTitle = "File to send as beacon";
			ofn.lpstrInitialDir = GetBPQDirectory();

			if (GetOpenFileName(&ofn))
				SetDlgItemText(hDlg, IDC_FILENAME, &FN[Port][0]);

			break;


		case IDOK:

			GetDlgItemText(hDlg, IDC_UIDEST, &UIDEST[Port][0], 10);

			if (UIDigi[Port])
			{
				free(UIDigi[Port]);
				UIDigi[Port] = NULL;
			}

			if (UIDigiAX[Port])
			{
				free(UIDigiAX[Port]);
				UIDigiAX[Port] = NULL;
			}

			GetDlgItemText(hDlg, IDC_UIDIGIS, Digis, 99); 
		
			UIDigi[Port] = _strdup(Digis);
		
			GetDlgItemText(hDlg, IDC_FILENAME, &FN[Port][0], 255); 
			GetDlgItemText(hDlg, IDC_MESSAGE, &Message[Port][0], 1000); 
	
			Interval[Port] = GetDlgItemInt(hDlg, IDC_INTERVAL, &OK, FALSE); 

			MinCounter[Port] = Interval[Port];

			SendFromFile[Port] = IsDlgButtonChecked(hDlg, IDC_FROMFILE);

			wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", PortNum[CurrentPage]);

			retCode = RegCreateKeyEx(REGTREE,
					Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);
	
			if (retCode == ERROR_SUCCESS)
			{
				retCode = RegSetValueEx(hKey, "UIDEST", 0, REG_SZ,(BYTE *)&UIDEST[Port][0], strlen(&UIDEST[Port][0]));
				retCode = RegSetValueEx(hKey, "FileName", 0, REG_SZ,(BYTE *)&FN[Port][0], strlen(&FN[Port][0]));
				retCode = RegSetValueEx(hKey, "Message", 0, REG_SZ,(BYTE *)&Message[Port][0], strlen(&Message[Port][0]));
				retCode = RegSetValueEx(hKey, "Interval", 0, REG_DWORD,(BYTE *)&Interval[Port], 4);
				retCode = RegSetValueEx(hKey, "SendFromFile", 0, REG_DWORD,(BYTE *)&SendFromFile[Port], 4);
				retCode = RegSetValueEx(hKey, "Enabled", 0, REG_DWORD,(BYTE *)&UIEnabled[Port], 4);
				retCode = RegSetValueEx(hKey, "Digis",0, REG_SZ, Digis, strlen(Digis));

				RegCloseKey(hKey);
			}

			SetupUI(Port);

			return (INT_PTR)TRUE;


		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case ID_TEST:

			SendBeacon(Port);
			return TRUE;




		}
		break;

	}	
	return (INT_PTR)FALSE;
}




VOID WINAPI OnTabbedDialogInit(HWND hDlg)
{
	DLGHDR *pHdr = (DLGHDR *) LocalAlloc(LPTR, sizeof(DLGHDR));
	DWORD dwDlgBase = GetDialogBaseUnits();
	int cxMargin = LOWORD(dwDlgBase) / 4;
	int cyMargin = HIWORD(dwDlgBase) / 8;

	TC_ITEM tie;
	RECT rcTab;

	int i, pos, tab = 0;
	INITCOMMONCONTROLSEX init;

	char PortNo[60];
	struct _EXTPORTDATA * PORTVEC;

	hwndDlg = hDlg;			// Save Window Handle

	// Save a pointer to the DLGHDR structure.

	SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) pHdr);

	// Create the tab control.


	init.dwICC = ICC_STANDARD_CLASSES;
	init.dwSize=sizeof(init);
	i=InitCommonControlsEx(&init);

	pHdr->hwndTab = CreateWindow(WC_TABCONTROL, "", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, 100, 100, hwndDlg, NULL, hInst, NULL);

	if (pHdr->hwndTab == NULL) {

	// handle error

	}

	// Add a tab for each of the child dialog boxes.

	tie.mask = TCIF_TEXT | TCIF_IMAGE;

	tie.iImage = -1;

	for (i = 1; i <= GetNumberofPorts(); i++)
	{
		// Only allow UI on ax.25 ports

		PORTVEC = (struct _EXTPORTDATA * )GetPortTableEntry(i);

		if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
			if (PORTVEC->PORTCONTROL.PROTOCOL == 10)	// Pactor/WINMOR
				continue;

		wsprintf(PortNo, "Port %2d", GetPortNumber(i));
		PortNum[tab] = GetPortNumber(i);

		tie.pszText = PortNo;
		TabCtrl_InsertItem(pHdr->hwndTab, tab, &tie);
	
		pHdr->apRes[tab++] = DoLockDlgRes("PORTPAGE");

	}

	PageCount = tab;

	// Determine the bounding rectangle for all child dialog boxes.

	SetRectEmpty(&rcTab);

	for (i = 0; i < PageCount; i++)
	{
		if (pHdr->apRes[i]->cx > rcTab.right)
			rcTab.right = pHdr->apRes[i]->cx;

		if (pHdr->apRes[i]->cy > rcTab.bottom)
			rcTab.bottom = pHdr->apRes[i]->cy;

	}

	MapDialogRect(hwndDlg, &rcTab);

//	rcTab.right = rcTab.right * LOWORD(dwDlgBase) / 4;

//	rcTab.bottom = rcTab.bottom * HIWORD(dwDlgBase) / 8;

	// Calculate how large to make the tab control, so

	// the display area can accomodate all the child dialog boxes.

	TabCtrl_AdjustRect(pHdr->hwndTab, TRUE, &rcTab);

	OffsetRect(&rcTab, cxMargin - rcTab.left, cyMargin - rcTab.top);

	// Calculate the display rectangle.

	CopyRect(&pHdr->rcDisplay, &rcTab);

	TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay);

	// Set the size and position of the tab control, buttons,

	// and dialog box.

	SetWindowPos(pHdr->hwndTab, NULL, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, SWP_NOZORDER);

	// Move the Buttons to bottom of page

	pos=rcTab.left+cxMargin;

	
	// Size the dialog box.

	SetWindowPos(hwndDlg, NULL, 0, 0, rcTab.right + cyMargin + 2 * GetSystemMetrics(SM_CXDLGFRAME),
		rcTab.bottom  + 2 * cyMargin + 2 * GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION),
		SWP_NOMOVE | SWP_NOZORDER);

	// Simulate selection of the first item.

	OnSelChanged(hwndDlg);

}

// DoLockDlgRes - loads and locks a dialog template resource.

// Returns a pointer to the locked resource.

// lpszResName - name of the resource

DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName)
{
	HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);
	HGLOBAL hglb = LoadResource(hInst, hrsrc);

	return (DLGTEMPLATE *) LockResource(hglb);
}

//The following function processes the TCN_SELCHANGE notification message for the main dialog box. The function destroys the dialog box for the outgoing page, if any. Then it uses the CreateDialogIndirect function to create a modeless dialog box for the incoming page.

// OnSelChanged - processes the TCN_SELCHANGE notification.

// hwndDlg - handle of the parent dialog box

VOID WINAPI OnSelChanged(HWND hwndDlg)
{
	char PortDesc[40];
	int Port;

	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	CurrentPage = TabCtrl_GetCurSel(pHdr->hwndTab);

	// Destroy the current child dialog box, if any.

	if (pHdr->hwndDisplay != NULL)

		DestroyWindow(pHdr->hwndDisplay);

	// Create the new child dialog box.

	pHdr->hwndDisplay = CreateDialogIndirect(hInst, pHdr->apRes[CurrentPage], hwndDlg, ChildDialogProc);

	hwndDisplay = pHdr->hwndDisplay;		// Save

	Port = PortNum[CurrentPage];
	// Fill in the controls

	GetPortDescription(PortNum[CurrentPage], PortDesc);

	SetDlgItemText(hwndDisplay, IDC_PORTNAME, PortDesc);

//	CheckDlgButton(hwndDisplay, IDC_FROMFILE, SendFromFile[Port]);

//	SetDlgItemInt(hwndDisplay, IDC_INTERVAL, Interval[Port], FALSE);

	SetDlgItemText(hwndDisplay, IDC_UIDEST, &UIDEST[Port][0]);
	SetDlgItemText(hwndDisplay, IDC_UIDIGIS, UIDigi[Port]);



//	SetDlgItemText(hwndDisplay, IDC_FILENAME, &FN[Port][0]);
//	SetDlgItemText(hwndDisplay, IDC_MESSAGE, &Message[Port][0]);

	ShowWindow(pHdr->hwndDisplay, SW_SHOWNORMAL);

}

//The following function processes the WM_INITDIALOG message for each of the child dialog boxes. You cannot specify the position of a dialog box created using the CreateDialogIndirect function. This function uses the SetWindowPos function to position the child dialog within the tab control's display area.

// OnChildDialogInit - Positions the child dialog box to fall

// within the display area of the tab control.

VOID WINAPI OnChildDialogInit(HWND hwndDlg)
{
	HWND hwndParent = GetParent(hwndDlg);
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndParent, GWL_USERDATA);

	SetWindowPos(hwndDlg, HWND_TOP, pHdr->rcDisplay.left, pHdr->rcDisplay.top, 0, 0, SWP_NOSIZE);
}


*/
VOID APRSPoll()
{
	char APRSMsg[400];
	char Call[12];
	struct STATIONRECORD * Station;

	CheckTimer();

	while (GetAPRSFrame(APRSMsg, Call))
	{
		Station = FindStation(Call, Call, FALSE);

		if (Station)
			ProcessMessage(APRSMsg, Station);
		
	}

}
void UpdateTXMessageLine(HWND hWnd, int j, struct APRSMESSAGE * Message)
{
	LV_ITEM Item;
	int ret;
	char Retries[10];

	wsprintf(Retries, "%d", Message->Retries);

	Item.mask=LVIF_TEXT;
	Item.iItem=j;
	Item.iSubItem=0;
	Item.pszText = Message->ToCall;

	ret = SendMessage(hWnd, LVM_SETITEMTEXT, (WPARAM)j, (LPARAM) &Item);

	if (ret == 0)
		ret = ListView_InsertItem(hWnd, &Item);	 

	OurSetItemText(hWnd, j, 1, Message->Seq);

	OurSetItemText(hWnd, j, 3, Message->Text);

	SendMessage(hWnd, LVM_ENSUREVISIBLE, (WPARAM)j, (LPARAM) 0);

	if (Message->Acked)
	{
		OurSetItemText(hWnd, j, 2, "A");
		return;
	}

	if (Message->Retries == 0)
	{
		OurSetItemText(hWnd, j, 2, "F");
		return;
	}

	OurSetItemText(hWnd, j, 2, Retries);
}


void UpdateMessageLine(HWND hWnd, int j, struct APRSMESSAGE * Message)
{
	LV_ITEM Item;
	int ret;

	Item.mask=LVIF_TEXT;
	Item.iItem=j;
	Item.iSubItem=0;
	Item.pszText = Message->FromCall;

	ret = SendMessage(hWnd, LVM_SETITEMTEXT, (WPARAM)j, (LPARAM) &Item);

	if (ret == 0)
		ret = ListView_InsertItem(hWnd, &Item);	 

	OurSetItemText(hWnd, j, 1, Message->ToCall);
	OurSetItemText(hWnd, j, 2, Message->Seq);
	OurSetItemText(hWnd, j, 3, Message->Time);
	OurSetItemText(hWnd, j, 4, Message->Text);

	ret = SendMessage(hWnd, LVM_ENSUREVISIBLE, (WPARAM)j, (LPARAM) 0);

}
VOID RefreshMessages()
{
	struct APRSMESSAGE * ptr = Messages;
	int n = 0;
	BOOL OnlyMine = IsDlgButtonChecked(hMsgDlg, IDC_MYMSGS);

	SendMessage(hMsgsIn, LVM_DELETEALLITEMS, (WPARAM)0, (LPARAM) 0);

	if (ptr)
	{
		do
		{				
			if (OnlyMine == 0 ||(strcmp(ptr->ToCall, APRSCall) == 0) || (strcmp(ptr->FromCall, APRSCall) == 0))
			{			
				UpdateMessageLine(hMsgsIn, n, ptr);
				n++;
			}
			ptr = ptr->Next;
		
		} while (ptr);
	}

	ptr = OutstandingMsgs;
	n = 0;

	if (ptr == 0)
		return;

	do
	{				
		UpdateTXMessageLine(hMsgsOut, n, ptr);
		ptr = ptr->Next;
		n++;

	} while (ptr);
	
}


HWND CreateStnListView (HWND hwndParent) 
{
    INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	HWND hList;
	LV_COLUMN Column;
	
	icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Create the list-view window in report view with label editing enabled.
    
	hList = CreateWindow(WC_LISTVIEW, 
                                     "Stations",
                                     WS_CHILD | LVS_REPORT | LVS_EDITLABELS | LVS_SORTASCENDING,
                                     0, 0, 100, 100,
                                     hwndParent,
                                     (HMENU)NULL,
                                     hInst,
                                     NULL); 

	ListView_SetExtendedListViewStyle(hList,LVS_EX_FULLROWSELECT);

	Column.cx=85;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Callsign";

	SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM) &Column); 

	Column.cx=95;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Lat";

	SendMessage(hList,LVM_INSERTCOLUMN,2,(LPARAM) &Column); 
	
	Column.cx=100;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Lon";
	SendMessage(hList,LVM_INSERTCOLUMN,3,(LPARAM) &Column); 

	Column.cx=60;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Range";
	SendMessage(hList,LVM_INSERTCOLUMN,4,(LPARAM) &Column); 

	Column.cx=37;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Bearing";
	SendMessage(hList,LVM_INSERTCOLUMN,5,(LPARAM) &Column); 

	Column.cx=100;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Last Heard";

	SendMessage(hList,LVM_INSERTCOLUMN,6,(LPARAM) &Column); 
	Column.cx=500;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Last Message";

	SendMessage(hList,LVM_INSERTCOLUMN,7,(LPARAM) &Column); 

	ShowWindow(hList, SW_SHOWNORMAL);
	UpdateWindow(hList);

    return (hList);
}


HWND CreateRXListView (HWND hwndParent) 
{
    INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	HWND hList;
	LV_COLUMN Column;
	
	icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Create the list-view window in report view with label editing enabled.
    
/*	hList = CreateWindow(WC_LISTVIEW, 
                                     "Messages",
                                     WS_CHILD | LVS_REPORT | LVS_EDITLABELS,
                                     0, 0, 100, 100,
                                     hwndParent,
                                     (HMENU)NULL,
                                     hInst,
                                     NULL); 

*/
	hList = GetDlgItem(hwndParent, IDC_LIST1);

	ListView_SetExtendedListViewStyle(hList,LVS_EX_FULLROWSELECT);

	Column.cx=85;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="  From";

	SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM) &Column); 

	Column.cx=85;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="   To";

	SendMessage(hList,LVM_INSERTCOLUMN,2,(LPARAM) &Column); 
	Column.cx=50;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Seq";

	SendMessage(hList,LVM_INSERTCOLUMN,3,(LPARAM) &Column); 

	Column.cx=60;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Time";

	SendMessage(hList,LVM_INSERTCOLUMN,4,(LPARAM) &Column); 

	Column.cx=600;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="               Received";

	SendMessage(hList,LVM_INSERTCOLUMN,5,(LPARAM) &Column); 
	ShowWindow(hList, SW_SHOWNORMAL);
	UpdateWindow(hList);

    return (hList);
}


HWND CreateTXListView (HWND hwndParent) 
{
    INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	HWND hList;
	LV_COLUMN Column;
	
	icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Create the list-view window in report view with label editing enabled.
    
/*	hList = CreateWindow(WC_LISTVIEW, 
                                     "Messages",
                                     WS_CHILD | LVS_REPORT | LVS_EDITLABELS,
                                     0, 0, 100, 100,
                                     hwndParent,
                                     (HMENU)NULL,
                                     hInst,
                                     NULL); 

*/

	hList = GetDlgItem(hwndParent, IDC_LIST2);


	ListView_SetExtendedListViewStyle(hList,LVS_EX_FULLROWSELECT);

	Column.cx=85;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="   To";

	SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM) &Column); 
	Column.cx=40;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="Seq";

	SendMessage(hList,LVM_INSERTCOLUMN,2,(LPARAM) &Column); 

	Column.cx=37;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="State";

	SendMessage(hList,LVM_INSERTCOLUMN,3,(LPARAM) &Column); 

	Column.cx=600;
	Column.mask=LVCF_WIDTH | LVCF_TEXT;
	Column.pszText="               Sent";

	SendMessage(hList,LVM_INSERTCOLUMN,4,(LPARAM) &Column); 
	ShowWindow(hList, SW_SHOWNORMAL);
	UpdateWindow(hList);

    return (hList);
}

BOOL CreateStationWindow(char * ClassName, char * WindowTitle, WNDPROC WndProc, LPCSTR MENU)
{
    WNDCLASS  wc;
	HANDLE hDlg;
	RECT rcClient;                       // The parent window's client area.
	
	if (hStnDlg)
	{
		ShowWindow(hStnDlg, SW_SHOWNORMAL);
		SetForegroundWindow(hStnDlg);
		return FALSE;							// Already open
	}

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
    wc.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(BPQICON) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = MENU;;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	hStnDlg = hDlg = CreateDialog(hInst, ClassName, 0, NULL);
		
	SetWindowText(hDlg, "APRS Stations");

	if (MinimizetoTray)
		AddTrayMenuItem(hDlg, "APRS Stations");

	if (StnRect.right < 100 || StnRect.bottom < 100)
	{
		GetWindowRect(hDlg,	&StnRect);
	}

	MoveWindow(hDlg, StnRect.left, StnRect.top, StnRect.right - StnRect.left, StnRect.bottom - StnRect.top, TRUE);
	
	GetClientRect (hDlg, &rcClient); 
	
	hStations = CreateStnListView(hDlg);

	MoveWindow(hStations, 0, 0, rcClient.right, rcClient.bottom, TRUE);

	RefreshStationList();

	if (MsgMinimized)
		if (MinimizetoTray)
			ShowWindow(hDlg, SW_HIDE);
		else
			ShowWindow(hDlg, SW_SHOWMINIMIZED);
	else
		ShowWindow(hDlg, SW_RESTORE);

	return TRUE;
}



BOOL CreateMessageWindow(char * ClassName, char * WindowTitle, WNDPROC WndProc, LPCSTR MENU)
{
	HANDLE hDlg;
	RECT rcClient;                       // The parent window's client area.
	
	if (hMsgDlg)
	{
		ShowWindow(hMsgDlg, SW_SHOWNORMAL);
		SetForegroundWindow(hMsgDlg);
		return FALSE;							// Already open
	}

	hMsgDlg = hDlg = CreateDialog(hInst, ClassName, 0, NULL);

	hInput = GetDlgItem(hDlg, IDC_INPUT);
	hToCall = GetDlgItem(hDlg, IDC_TOCALL);
	hToLabel = GetDlgItem(hDlg, IDC_TOLABEL);
	hTextLabel = GetDlgItem(hDlg, IDC_TEXTLABEL);
	hPathLabel = GetDlgItem(hDlg, IDC_TEXTLABEL2);
	hPath = GetDlgItem(hDlg, IDC_PATH);
	
	wpOrigInputProc = (WNDPROC) SetWindowLong(hInput, GWL_WNDPROC, (LONG) InputProc); 

	GetWindowRect(hDlg,	&Rect);
	
	SetWindowText(hDlg, "APRS Messages");

	if (MinimizetoTray)
		AddTrayMenuItem(hDlg, "APRS Messages");

	if (MsgRect.right < 100 || MsgRect.bottom < 100)
	{
		GetWindowRect(hDlg,	&MsgRect);
	}

	MoveWindow(hDlg, MsgRect.left, MsgRect.top, MsgRect.right - MsgRect.left, MsgRect.bottom - MsgRect.top, TRUE);
	
	hMsgsIn = CreateRXListView(hMsgDlg);
	hMsgsOut = CreateTXListView(hMsgDlg);


	GetClientRect (hDlg, &rcClient); 

	MoveWindow(hMsgsIn, 0, 30, rcClient.right, (rcClient.bottom)/2 - 40, TRUE);
	MoveWindow(hMsgsOut, 0, (rcClient.bottom)/2, rcClient.right, (rcClient.bottom)/2 - 60, TRUE);
	MoveWindow(hInput, 200, rcClient.bottom - 35 , 500, 20, TRUE);
	MoveWindow(hToLabel, 9, rcClient.bottom - 33 , 23, 18, TRUE);
	MoveWindow(hToCall, 40, rcClient.bottom - 35 , 120, 20, TRUE);
	MoveWindow(hTextLabel, 163, rcClient.bottom - 33 , 33, 18, TRUE);
	MoveWindow(hPathLabel, 710, rcClient.bottom - 33 , 33, 18, TRUE);
	MoveWindow(hPath, 750, rcClient.bottom - 33 , 150, 18, TRUE);

	RefreshMessages();

	if (MsgMinimized)
		if (MinimizetoTray)
			ShowWindow(hDlg, SW_HIDE);
		else
			ShowWindow(hDlg, SW_SHOWMINIMIZED);
	else
		ShowWindow(hDlg, SW_RESTORE);

	return TRUE;
}

VOID SendAPRSMessage(char * Text, char * ToCall)
{
	struct APRSMESSAGE * Message;
	struct APRSMESSAGE * ptr = OutstandingMsgs;
	int n = 0;
	char Msg[255];

	Message = malloc(sizeof(struct APRSMESSAGE));
	memset(Message, 0, sizeof(struct APRSMESSAGE));
	strcpy(Message->FromCall, APRSCall);
	memset(Message->ToCall, ' ', 9);
	memcpy(Message->ToCall, ToCall, strlen(ToCall));
	Message->ToStation = APPLFindStation(ToCall, TRUE);

	if (Message->ToStation == NULL)
		return;

	if (Message->ToStation->LastRXSeq[0])		// Have we received a Reply-Ack message from him?
		wsprintf(Message->Seq, "%02X}%c%c", NextSeq++, Message->ToStation->LastRXSeq[0], Message->ToStation->LastRXSeq[1]);
	else
	{
		if (Message->ToStation->SimpleNumericSeq)
			wsprintf(Message->Seq, "%d", NextSeq++);
		else
			wsprintf(Message->Seq, "%02X}", NextSeq++);	// Don't know, so assume message-ack capable
	}
	strcpy(Message->Text, Text);
	Message->Retries = RetryCount;
	Message->RetryTimer = RetryTimer;

	if (ptr == NULL)
	{
		OutstandingMsgs = Message;
	}
	else
	{
		n++;
		while(ptr->Next)
		{
			ptr = ptr->Next;
			n++;
		}
		ptr->Next = Message;
	}

	UpdateTXMessageLine(hMsgsOut, n, Message);

	n = wsprintf(Msg, ":%-9s:%s{%s", ToCall, Text, Message->Seq);

	PutAPRSMessage(Msg, n);
	return;
}


VOID ProcessMessage(char * Payload, struct STATIONRECORD * Station)
{
	char MsgDest[10];
	struct APRSMESSAGE * Message;
	struct APRSMESSAGE * ptr = Messages;
	char * TextPtr = &Payload[11];
	char * SeqPtr;
	int n = 0;
	char FromCall[10] = "         ";
	struct tm * TM;
	time_t NOW;

	memcpy(FromCall, Station->Callsign, strlen(Station->Callsign));
	memcpy(MsgDest, &Payload[1], 9);
	MsgDest[9] = 0;

	SeqPtr = strchr(TextPtr, '{');

	if (SeqPtr)
	{
		*(SeqPtr++) = 0;
		if(strlen(SeqPtr) > 6)
			SeqPtr[7] = 0;		
	}

	if (_memicmp(TextPtr, "ack", 3) == 0)
	{
		// Message Ack. See if for one of our messages

		ptr = OutstandingMsgs;

		if (ptr == 0)
			return;

		do
		{
			if (strcmp(ptr->FromCall, MsgDest) == 0
				&& strcmp(ptr->ToCall, FromCall) == 0
				&& strcmp(ptr->Seq, &TextPtr[3]) == 0)
			{
				// Message is acked

				ptr->Retries = 0;
				ptr->Acked = TRUE;
				if (hMsgsOut)
					UpdateTXMessageLine(hMsgsOut, n, ptr);

				return;
			}
			ptr = ptr->Next;
			n++;

		} while (ptr);
	
		return;
	}

	Message = malloc(sizeof(struct APRSMESSAGE));
	memset(Message, 0, sizeof(struct APRSMESSAGE));
	strcpy(Message->FromCall, Station->Callsign);
	strcpy(Message->ToCall, MsgDest);

	if (SeqPtr)
	{
		strcpy(Message->Seq, SeqPtr);

		// If a REPLY-ACK Seg, copy to LastRXSeq, and see if it acks a message

		if (SeqPtr[2] == '}')
		{
			struct APRSMESSAGE * ptr1;
			int nn = 0;

			strcpy(Station->LastRXSeq, SeqPtr);

			ptr1 = OutstandingMsgs;

			while (ptr1)
			{
				if (strcmp(ptr1->FromCall, MsgDest) == 0
					&& strcmp(ptr1->ToCall, FromCall) == 0
					&& memcmp(&ptr1->Seq, &SeqPtr[3], 2) == 0)
				{
					// Message is acked

					ptr1->Acked = TRUE;
					ptr1->Retries = 0;
					if (hMsgsOut)
						UpdateTXMessageLine(hMsgsOut, nn, ptr);
					
					break;
				}
				ptr1 = ptr1->Next;
				nn++;
			}
		}
		else
		{
			// Station is not using reply-ack - set to send simple numeric sequence (workround for bug in APRS Messanges
		
			Station->SimpleNumericSeq = TRUE;
		}
	}

	if (strlen(TextPtr) > 100)
		TextPtr[100] = 0;

	strcpy(Message->Text, TextPtr);
		
	NOW = time(NULL);

	if (LocalTime)
		TM = localtime(&NOW);
	else
		TM = gmtime(&NOW);
					
	wsprintf(Message->Time, "%.2d:%.2d", TM->tm_hour, TM->tm_min);

	if (_stricmp(MsgDest, APRSCall) == 0 && SeqPtr)	// ack it if it has a sequence
	{
		// For us - send an Ack

		char ack[30];

		int n = wsprintf(ack, ":%-9s:ack%s", Message->FromCall, Message->Seq);
		PutAPRSMessage(ack, n);
	}

	if (ptr == NULL)
	{
		Messages = Message;
	}
	else
	{
		n++;
		while(ptr->Next)
		{
			ptr = ptr->Next;
			n++;
		}
		ptr->Next = Message;
	}

	if (hMsgsIn)
	{
		BOOL OnlyMine = IsDlgButtonChecked(hMsgDlg, IDC_MYMSGS);
		RefreshMessages();
	}

	if (strcmp(MsgDest, APRSCall) == 0)			// to me?
	{
		if (IsDlgButtonChecked(hMsgDlg, IDC_MSGBEEP))
		{
			PlaySound("SystemDefault", NULL, SND_ALIAS);
			FlashWindow(hMsgDlg, TRUE);
		}
//			PlaySound ("BPQCHAT_USER_LOGIN", NULL, SND_ALIAS | SND_APPLICATION | SND_ASYNC);
		else
			CreateMessageWindow("APRSMSGS", "APRS Messages", MsgWndProc, NULL); // Will activate if running
	}
}


LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	if (uMsg == WM_CHAR) 
	{
		int i = SendMessage(hInput,WM_GETTEXTLENGTH, 0, 0);

		if (wParam == 13)
		{			
			char kbbuf[80];
			char ToCall[11];

			SendMessage(hInput, WM_GETTEXT, 79, (LPARAM) (LPCSTR) kbbuf);
			kbbuf[i] = 0;
			SendMessage(hToCall, WM_GETTEXT, 10, (LPARAM)(LPCTSTR)&ToCall);

			if (strlen(ToCall) < 2)
			{
				MessageBox(NULL, "Please enter a TO Call", "BPQAPRS", MB_OK);
				return 0;
			}

			// If call not in Listboax, add it

			i = SendMessage(hToCall, CB_FINDSTRINGEXACT, -1, (LPARAM)(LPCTSTR)&ToCall);

			if (i == CB_ERR)
				SendMessage(hToCall, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)&ToCall);

			SendMessage(hInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			SendAPRSMessage(kbbuf, ToCall);

			return 0; 
		}
		else
		{
			// limit to 67

			if (i > 66)
			{
				if (wParam != 8)
				{
					PlaySound("SystemDefault", NULL, SND_ASYNC | SND_ALIAS);
					return 0;
				}
			}
		}

	}
 
    return CallWindowProc(wpOrigInputProc, hwnd, uMsg, wParam, lParam); 
} 

VOID SecTimer()
{
	struct STATIONRECORD * sptr = *StationRecords;
	struct APRSMESSAGE * ptr = OutstandingMsgs;
	int n = 0;
	char Msg[20];

	// See if changed flag set on any stations

	while (sptr)
	{
		if (sptr->Moved)
			DrawStation(sptr);
	
		sptr = sptr->Next;
	}

	// Check Message Retries


	JPEGCounter++;

	if (CreateJPEG)
	{
		if (JPEGCounter > JPEGInterval)
		{
			if (cxWinSize > 0 && cyWinSize > 0)
			{
				LoadImageSet(Zoom, SetBaseX, SetBaseY);
				
				EnterCriticalSection(&RefreshCrit);
				RefreshStationMap();
				LeaveCriticalSection(&RefreshCrit);

				if (RGBToJpegFile(JPEGFileName, Image, cxWinSize, cyWinSize, TRUE, 100))
					JPEGCounter = 0;
			}
		}
	}
	if (SendWX)
		SendWeatherBeacon();

	// If any changes to image redraw it

	if (ImageChanged)
	{
		// We have drawn a new Icon. As we only redraw if it has moved, 
		// we need to reload image every now and again to get rid of ghost images

		time_t NOW = time(NULL);
		
		if ((NOW - LastRefresh) > 10)
		{
			LastRefresh = NOW;
			ReloadMaps = TRUE;
		}

		ImageChanged = FALSE;
		InvalidateRect(hMapWnd, NULL, FALSE);
	}

	wsprintf(Msg, "%d", StationCount);
	SendMessage(hStatus, SB_SETTEXT, (WPARAM)(INT) 0 | 1, (LPARAM)Msg);

	wsprintf(Msg, "%d", OSMQueueCount);
	SendMessage(hStatus, SB_SETTEXT, (WPARAM)(INT) 0 | 2, (LPARAM)Msg);

	wsprintf(Msg, "%d", Zoom);
	SendMessage(hStatus, SB_SETTEXT, (WPARAM)(INT) 0 | 3, (LPARAM)Msg);


	if (ptr == 0)
		return;

	do
	{				
		if (ptr->Acked == FALSE)
		{
			if (ptr->Retries)
			{
				ptr->RetryTimer--;
				
				if (ptr->RetryTimer == 0)
				{
					ptr->Retries--;

					if (ptr->Retries)
					{
						// Send Again
						
						char Msg[255];
						int n = wsprintf(Msg, ":%-9s:%s{%s", ptr->ToCall, ptr->Text, ptr->Seq);
						PutAPRSMessage(Msg, n);
						ptr->RetryTimer = RetryTimer;
					}
					UpdateTXMessageLine(hMsgsOut, n, ptr);
				}
			}
		}

		ptr = ptr->Next;
		n++;

	} while (ptr);
}

double radians(double Degrees)
{
    return M_PI * Degrees / 180;
}
double degrees(double Radians)
{
	return Radians * 180 / M_PI;
}


double Distance(double laa, double loa)
{
	double lah, loh;

	GetAPRSLatLon(&lah, &loh);

/*

'Great Circle Calculations.

'dif = longitute home - longitute away


'      (this should be within -180 to +180 degrees)
'      (Hint: This number should be non-zero, programs should check for
'             this and make dif=0.0001 as a minimum)
'lah = latitude of home
'laa = latitude of away

'dis = ArcCOS(Sin(lah) * Sin(laa) + Cos(lah) * Cos(laa) * Cos(dif))
'distance = dis / 180 * pi * ERAD
'angle = ArcCOS((Sin(laa) - Sin(lah) * Cos(dis)) / (Cos(lah) * Sin(dis)))

'p1 = 3.1415926535: P2 = p1 / 180: Rem -- PI, Deg =>= Radians
*/

loh = radians(loh); lah = radians(lah);
loa = radians(loa); laa = radians(laa);

return 60*degrees(acos(sin(lah) * sin(laa) + cos(lah) * cos(laa) * cos(loa-loh))) * 1.15077945;

}

double Bearing(double lat2, double lon2)
{
	double lat1, lon1;
	double dlat, dlon, TC1;

	GetAPRSLatLon(&lat1, &lon1);
 
	lat1 = radians(lat1);
	lat2 = radians(lat2);
	lon1 = radians(lon1);
	lon2 = radians(lon2);

	dlat = lat2 - lat1;
	dlon = lon2 - lon1;

	if (dlat == 0 || dlon == 0) return 0;
	
	TC1 = atan((sin(lon1 - lon2) * cos(lat2)) / (cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon1 - lon2)));
	TC1 = degrees(TC1);
		
	if (fabs(TC1) > 89.5) if (dlon > 0) return 90; else return 270;

	if (dlat > 0)
	{
		if (dlon > 0) return -TC1;
		if (dlon < 0) return 360 - TC1;
		return 0;
	}

	if (dlat < 0)
	{
		if (dlon > 0) return TC1 = 180 - TC1;
		if (dlon < 0) return TC1 = 180 - TC1; // 'ok?
		return 180;
	}

	return 0;

}


int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	LVITEM item = {0};
	char Item1[260] = "AA";
	char Item2[260] = "BB";

	item.iSubItem = lParamSort;
	item.mask = LVIF_TEXT;
	item.iItem = lParam1;
	item.pszText = Item1;
	item.cchTextMax = 100;

	ListView_GetItem(hStations, &item);

	item.iItem = lParam2;
	item.pszText = Item2;

	ListView_GetItem(hStations, &item);

	return strcmp(Item1, Item2);
}


// Weather Data 
	
char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

VOID SendWeatherBeacon()
{
	char Msg[256];
	char DD[3]="";
	char HH[3]="";
	char MM[3]="";
	char Lat[10], Lon[10];
	int i, Len, index;
	HANDLE Handle;
	char WXMessage[1024];
	char * WXptr;
	char * WXend;
	struct tm rtime;
	__time64_t WXTime, TempTime;
	FILETIME LastWriteTime;
	LARGE_INTEGER ft;
	__time64_t now = _time64(NULL);
	SYSTEMTIME SystemTime;

	WXCounter++;

	if (WXCounter < WXInterval * 60)
		return;

	WXCounter = 0;

	Debugprintf("BPQAPRS - Trying to open WX file %s", WXFileName);

	Handle = CreateFile(WXFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (Handle == INVALID_HANDLE_VALUE)
	{
		Debugprintf("BPQAPRS - Open WX file %s failed %d", WXFileName, GetLastError());
		return;
	}

	GetFileTime(Handle, NULL, NULL, &LastWriteTime);

	ft.HighPart = LastWriteTime.dwHighDateTime;
	ft.LowPart = LastWriteTime.dwLowDateTime;

	ft.QuadPart -=  116444736000000000;
	ft.QuadPart /= 10000000;

	WXTime = (now - ft.QuadPart) / 60;		// Minutes 

	// Get DDHHMM from Filetime

	FileTimeToSystemTime(&LastWriteTime, &SystemTime);

	sprintf(DD, "%02d", SystemTime.wDay);
	sprintf(HH, "%02d", SystemTime.wHour);
	sprintf(MM, "%02d", SystemTime.wMinute);

	ReadFile(Handle, WXMessage, 1024, &Len, NULL); 
	CloseHandle(Handle);

	if (Len < 30)
	{
		Debugprintf("BPQAPRS - WX file is too short - %d Chars", Len);
		return;
	}

	WXptr = strchr(WXMessage, 10);

	if (WXptr)
	{
		WXend = strchr(++WXptr, 13);
		if (WXend)
			*WXend = 0;
	}

	if (WXMessage[0] != '*')
	{
		// Try to get time from message
		
		memset(&rtime, 0, sizeof(struct tm));

		rtime.tm_year = atoi(&WXMessage[7]) - 1900;
	
		for (i=0; i < 12; i++)
		{
			if (memcmp(month[i], WXMessage, 3) == 0)
			{
				rtime.tm_mon = i;
				break;
			}
		}

		rtime.tm_mday = atoi(DD);
		rtime.tm_hour = atoi(HH);
		rtime.tm_min = atoi(MM);

		TempTime = _mkgmtime64(&rtime);

		if (TempTime != (__time64_t)-1)
		{
			WXTime = (now - TempTime) / 60;		// Age in Minutes

			// Get DDHHMM from file

			memcpy(DD, &WXMessage[4], 2);
			memcpy(HH, &WXMessage[12], 2);
			memcpy(MM, &WXMessage[15], 2);
		}
	}

	if (WXTime > (3 * WXInterval))
	{
		Debugprintf("APRS Send WX File too old - %d minutes", WXTime);
		return;
	}

	GetAPRSLatLonString(Lat, Lon);

	Len = wsprintf(Msg, "@%s%s%sz%s/%s_%s%s", DD, HH, MM, Lat, Lon, WXptr, WXComment);

	Debugprintf(Msg);

	for (index = 0; index < 32; index++)
		if (WXPort[index])
			PutAPRSFrame(Msg, Len, index);
}


/*
Jan 22 2012 14:10
123/005g011t031r000P000p000h00b10161

/MITWXN Mitchell IN weather Station N9LYA-3 {UIV32} 
< previous

@221452z3844.42N/08628.33W_203/006g007t032r000P000p000h00b10171
Complete Weather Report Format — with Lat/Long position, no Timestamp
! or = Lat   Sym Table ID   Long   Symbol Code _  Wind Directn/ Speed Weather Data APRS Software   WX Unit uuuu
 1      8          1         9          1                 7                 n            1              2-4
Examples
!4903.50N/07201.75W_220/004g005t077r000p000P000h50b09900wRSW
!4903.50N/07201.75W_220/004g005t077r000p000P000h50b.....wRSW

*/


VOID DecodeWXPortList()
{
	char ParamCopy[80];
	char * Context;
	int Port;
	char * ptr;
	int index = 0;

	for (index = 0; index < 32; index++)
		WXPort[index] = FALSE;

	strcpy(ParamCopy, WXPortList);
		
	ptr = strtok_s(ParamCopy, " ,\t\n\r", &Context);

	while (ptr)
	{
		Port = atoi(ptr);				// this gives zero for IS
	
		WXPort[Port] = TRUE;	
			
		ptr = strtok_s(NULL, " ,\t\n\r", &Context);
	}
}

INT_PTR CALLBACK ColourDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TEXTMETRIC tm; 
    int y;  
    LPMEASUREITEMSTRUCT lpmis; 
    LPDRAWITEMSTRUCT lpdis; 


	switch (message)
	{
		int Colour;
		int Sel;
		char ColourString[100];

	case WM_INITDIALOG:
	
		for (Colour = 0; Colour < 20; Colour++)
		{
			SendDlgItemMessage(hDlg, IDC_CHATCOLOURS, CB_ADDSTRING, 0, (LPARAM) Colours [Colour]);
		}

		SendDlgItemMessage(hDlg, IDC_CHATCOLOURS, CB_SETCURSEL, CurrentPopup->TrackColour, 0);
		return TRUE;

		       
	case WM_MEASUREITEM: 
 
            lpmis = (LPMEASUREITEMSTRUCT) lParam; 
 
            // Set the height of the list box items. 
 
            lpmis->itemHeight = 15; 
            return TRUE; 
 
	case WM_DRAWITEM: 
 
            lpdis = (LPDRAWITEMSTRUCT) lParam; 
 
            // If there are no list box items, skip this message. 
 
            if (lpdis->itemID == -1) 
            { 
                break; 
            } 
 
            switch (lpdis->itemAction) 
            { 
				case ODA_SELECT: 
                case ODA_DRAWENTIRE: 
 			
					// if Chat Console, and message has a colour eacape, action it 
 
                    GetTextMetrics(lpdis->hDC, &tm); 
 
                    y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;

						SetTextColor(lpdis->hDC,  Colours[lpdis->itemID]);

//					SetBkColor(lpdis->hDC, 0);

					wsprintf(ColourString, "XXXXXX %06X %d", Colours[lpdis->itemID], lpdis->itemID); 

                    TextOut(lpdis->hDC, 
                        6, 
                        y, 
                        ColourString, 
                        16); 						
 
 //					SetTextColor(lpdis->hDC, OldColour);

                    break; 
			}


	case WM_COMMAND:

		switch LOWORD(wParam)
		{
		case IDOK:
		{
			Sel = SendDlgItemMessage(hDlg, IDC_CHATCOLOURS, CB_GETCURSEL, 0, 0);
			CurrentPopup->TrackColour = Sel; 
		}

		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		
		}
	}
	return FALSE;
}

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
/* #define void char */
/* #define const */
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS	/* we presume a 32-bit flat memory model */
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

/* Define "boolean" as unsigned char, not int, per Windows custom */
#ifndef __RPCNDR_H__		/* don't conflict if rpcndr.h already read */
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN		/* prevent jmorecfg.h from redefining it */

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED

#endif /* JPEG_INTERNALS */

#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED		/* BMP image file format */
#define GIF_SUPPORTED		/* GIF image file format */
#define PPM_SUPPORTED		/* PBMPLUS PPM/PGM image file format */
#undef RLE_SUPPORTED		/* Utah RLE image file format */
#define TARGA_SUPPORTED		/* Targa image file format */

#define TWO_FILE_COMMANDLINE	/* optional */
#define USE_SETMODE		/* Microsoft has setmode() */
#undef NEED_SIGNAL_CATCHER
#undef DONT_USE_B_MODE
#undef PROGRESS_REPORT		/* optional */

#endif /* JPEG_CJPEG_DJPEG */


#include "jpeglib.h"

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

void my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	char buffer[JMSG_LENGTH_MAX];

	/* Create the message */
	(*cinfo->err->format_message) (cinfo, buffer);

	/* Always display the message. */
	MessageBox(NULL,buffer,"JPEG Fatal Error",MB_ICONSTOP);


	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}


void j_putRGBScanline(BYTE *jpegline, 
					 int widthPix,
					 BYTE *outBuf,
					 int row)
{
	int offset = row * widthPix * 3;
	int count;
	for (count=0;count<widthPix;count++) 
	{
		*(outBuf + offset + count * 3 + 0) = *(jpegline + count * 3 + 0);
		*(outBuf + offset + count * 3 + 1) = *(jpegline + count * 3 + 1);
		*(outBuf + offset + count * 3 + 2) = *(jpegline + count * 3 + 2);
	}
}

//
//	stash a gray scanline
//

void j_putGrayScanlineToRGB(BYTE *jpegline, 
							 int widthPix,
							 BYTE *outBuf,
							 int row)
{
	int offset = row * widthPix * 3;
	int count;
	for (count=0;count<widthPix;count++) {

		BYTE iGray;

		// get our grayscale value
		iGray = *(jpegline + count);

		*(outBuf + offset + count * 3 + 0) = iGray;
		*(outBuf + offset + count * 3 + 1) = iGray;
		*(outBuf + offset + count * 3 + 2) = iGray;
	}
}


//
//	read a JPEG file
//

BYTE * JpegFileToRGB(char * fileName, UINT *width, UINT *height)

{
	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct my_error_mgr jerr;
	/* More stuff */
	FILE * infile=NULL;		/* source file */

	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */
	char buf[250];
	BYTE *dataBuf;

	// basic code from IJG Jpeg Code v6 example.c

	*width=0;
	*height=0;

	/* In this example we want to open the input file before doing anything else,
	* so that the setjmp() error recovery below can assume the file is open.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to read binary files.
	*/

	if ((infile = fopen(fileName, "rb")) == NULL) {
		sprintf(buf, "JPEG :\nCan't open %s\n", fileName);
		MessageBox(NULL, buf, "BPQAPRS", MB_ICONSTOP);
		return NULL;
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;


	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */

		jpeg_destroy_decompress(&cinfo);

		if (infile!=NULL)
			fclose(infile);
		return NULL;
	}

	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void) jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	*   (a) suspension is not possible with the stdio data source, and
	*   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	* See libjpeg.doc for more info.
	*/

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	* jpeg_read_header(), so we do nothing here.
	*/

	/* Step 5: Start decompressor */

	(void) jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* We may need to do some setup of our own at this point before reading
	* the data.  After jpeg_start_decompress() we have the correct scaled
	* output image dimensions available, as well as the output colormap
	* if we asked for color quantization.
	* In this example, we need to make an output work buffer of the right size.
	*/ 

	// get our buffer set to hold data

	////////////////////////////////////////////////////////////
	// alloc and open our new buffer
	dataBuf = malloc(cinfo.output_width * 3 * cinfo.output_height);
	if (dataBuf==NULL) {

		MessageBox(NULL, "JpegFile :\nOut of memory", "BPQAPRS", MB_ICONSTOP);

		jpeg_destroy_decompress(&cinfo);
		
		fclose(infile);

		return NULL;
	}

	// how big is this thing gonna be?
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */

		// asuumer all 3-components are RGBs
		if (cinfo.out_color_components==3) {
			
			j_putRGBScanline(buffer[0], 
								*width,
								dataBuf,
								cinfo.output_scanline-1);

		} else if (cinfo.out_color_components==1) {

			// assume all single component images are grayscale
			j_putGrayScanlineToRGB(buffer[0], 
								*width,
								dataBuf,
								cinfo.output_scanline-1);

		}

	}

	/* Step 7: Finish decompression */

	(void) jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	* Here we postpone it until after no more JPEG errors are possible,
	* so as to simplify the setjmp error logic above.  (Actually, I don't
	* think that jpeg_destroy can do an error exit, but why assume anything...)
	*/
	fclose(infile);

	/* At this point you may want to check to see whether any corrupt-data
	* warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	*/

	return dataBuf;
}
// store a scanline to our data buffer

void j_putRGBScanline(BYTE *jpegline, 
						 int widthPix,
						 BYTE *outBuf,
						 int row);

void j_putGrayScanlineToRGB(BYTE *jpegline, 
						 int widthPix,
						 BYTE *outBuf,
						 int row);


/*
BYTE RGBFromDWORDAligned(BYTE *inBuf, UINT widthPix, UINT widthBytes, UINT height)
{
	BYTE *tmp;
	UINT row;

	if (inBuf==NULL)
		return 0;

	tmp= malloc(height * widthPix * 3);
	
	if (tmp==NULL)
		return NULL;


	for (row=0;row<height;row++) {
		memcpy((tmp+row * widthPix * 3), 
				(inBuf + row * widthBytes), 
				widthPix * 3);
	}

	return tmp;
}
*/
//
//
//

#include <share.h>

BOOL RGBToJpegFile(char * fileName, BYTE *dataBuf, UINT widthPix, UINT height, BOOL color, int quality)
{
	struct jpeg_compress_struct cinfo;
	int row_stride;			/* physical row widthPix in image buffer */
	struct my_error_mgr jerr;
	FILE * outfile=NULL;			/* target file */

	if (dataBuf==NULL)
		return FALSE;
	if (widthPix==0)
		return FALSE;
	if (height==0)
		return FALSE;

	/* More stuff */
	/* Step 1: allocate and initialize JPEG compression object */
	
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	/* Establish the setjmp return context for my_error_exit to use. */
	
	if (setjmp(jerr.setjmp_buffer))
	{
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */

		jpeg_destroy_compress(&cinfo);

		if (outfile!=NULL)
			fclose(outfile);

		return FALSE;
	}

	/* Now we can initialize the JPEG compression object. */

	jpeg_create_compress(&cinfo);

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */

	if ((outfile = _fsopen(fileName, "wb", _SH_DENYWR)) == NULL)
	{
//		char buf[250];
//		sprintf(buf, "JpegFile :\nCan't open %s\n", fileName);
//		MessageBox(NULL, buf, "", 0);
		return FALSE;
	}

	jpeg_stdio_dest(&cinfo, outfile);

	/* Step 3: set parameters for compression */
												    
	/* First we supply a description of the input image.
	* Four fields of the cinfo struct must be filled in:
	*/
	cinfo.image_width = widthPix; 	/* image widthPix and height, in pixels */
	cinfo.image_height = height;

	cinfo.input_components = 3;		/* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
 
/* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */

  jpeg_set_defaults(&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = widthPix * 3;	/* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height)
  {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
	LPBYTE outRow;

	outRow = dataBuf + ScrollY * 6144 + ScrollX * 3 + (cinfo.next_scanline * 2048 * 3);

//	outRow = dataBuf + (cinfo.next_scanline * widthPix * 3);

    (void) jpeg_write_scanlines(&cinfo, &outRow, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);

  /* After finish_compress, we can close the output file. */
  fclose(outfile);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);

  /* And we're done! */

  return TRUE;
}

//	Web Server Code

//	The actual HTTP socket code is in bpq32.dll. Any requests for APRS data are passed in 
//	using a Named Pipe. The request looks exactly like one from a local socket, and the respone is
//	a fully pormatted HTTP packet


#define InputBufferLen 1000


#define MaxSessions 100


HANDLE PipeHandle;

char * LoginMsg = NULL;

int HTTPPort = 80;
BOOL IPV6 = TRUE;

#define MAX_PENDING_CONNECTS 5

BOOL OpenSockets6();

char HTDocs[MAX_PATH] = "HTML";
char SpecialDocs[MAX_PATH] = "Special Pages";

char SymbolText[192][20] = {

"Police Stn", "No Symbol", "Digi", "Phone", "DX Cluster", "HF Gateway", "Plane sm", "Mob Sat Stn",
"WheelChair", "Snowmobile", "Red Cross", "Boy Scout", "Home", "X", "Red Dot", "Circle (0)", 
"Circle (1)", "Circle (2)", "Circle (3)", "Circle (4)", "Circle (5)", "Circle (6)", "Circle (7)", "Circle (8)", 
"Circle (9)", "Fire", "Campground", "Motorcycle", "Rail Eng.", "Car", "File svr", "HC Future", 

"Aid Stn", "BBS", "Canoe", "No Symbol", "Eyeball", "Tractor", "Grid Squ.", "Hotel", 
"Tcp/ip", "No Symbol", "School", "Usr Log-ON", "MacAPRS", "NTS Stn", "Balloon", "Police", 
"TBD", "Rec Veh'le", "Shuttle", "SSTV", "Bus", "ATV", "WX Service", "Helo", 
"Yacht", "WinAPRS", "Jogger", "Triangle", "PBBS", "Plane lrge", "WX Station", "Dish Ant.", 

"Ambulance", "Bike", "ICP", "Fire Station", "Horse", "Fire Truck", "Glider", "Hospital", 
"IOTA", "Jeep", "Truck", "Laptop", "Mic-E Rptr", "Node", "EOC", "Rover", 
"Grid squ.", "Antenna", "Power Boat", "Truck Stop", "Truck 18wh", "Van", "Water Stn", "XAPRS", 
"Yagi", "Shelter", "No Symbol", "No Symbol", "No Symbol", "No Symbol", "", "",

"Emergency", "No Symbol", "No. Digi", "Bank", "No Symbol", "No. Diam'd", "Crash site", "Cloudy", 
"MEO", "Snow", "Church", "Girl Scout", "Home (HF)", "UnknownPos", "Destination", "No. Circle", 
"No Symbol", "No Symbol", "No Symbol", "No Symbol", "No Symbol", "No Symbol", "No Symbol", "No Symbol", 
"Petrol Stn", "Hail", "Park", "Gale Fl", "No Symbol", "No. Car", "Info Kiosk", "Hurricane", 

"No. Box", "Snow blwng", "Coast G'rd", "Drizzle", "Smoke", "Fr'ze Rain", "Snow Shwr", "Haze", 
"Rain Shwr", "Lightning", "Kenwood", "Lighthouse", "No Symbol", "Nav Buoy", "Rocket", "Parking  ", 
"Quake", "Restaurant", "Sat/Pacsat", "T'storm", "Sunny", "VORTAC", "No. WXS", "Pharmacy", 
"No Symbol", "No Symbol", "Wall Cloud", "No Symbol", "No Symbol", "No. Plane", "No. WX Stn", "Rain",

"No. Diamond", "Dust blwng", "No. CivDef", "DX Spot", "Sleet", "Funnel Cld", "Gale", "HAM store",
"No. Blk Box", "WorkZone", "SUV", "Area Locns", "Milepost", "No. Triang", "Circle sm", "Part Cloud",
"No Symbol", "Restrooms", "No. Boat", "Tornado", "No. Truck", "No. Van", "Flooding", "No Symbol",
"Sky Warn", "No Symbol", "Fog", "No Symbol", "No Symbol", "No Symbol", "", ""};

// All Calls (8 per line)

//<td><a href="find.cgi?call=EI7IG-1">EI7IG-1</a></td>
//<td><a href="find.cgi?call=G7TKK-1">G7TKK-1</a></td>
//<td><a href="find.cgi?call=GB7GL-B">GB7GL-B</a></td>
//<td><a href="find.cgi?call=GM1TCN">GM1TCN</a></td>
//<td><a href="find.cgi?call=GM8BPQ">GM8BPQ</a></td>
//<td><a href="find.cgi?call=GM8BPQ-14">GM8BPQ-14</a></td>
//<td><a href="find.cgi?call=LA2VPA-9">LA2VPA-9</a></td>
//<td><a href="find.cgi?call=LA3FIA-10">LA3FIA-10</a></td></tr><tr>
//<td><a href="find.cgi?call=LA6JF-2">LA6JF-2</a></td><td><a href="find.cgi?call=LD4ST">LD4ST</a></td><td><a href="find.cgi?call=M0CHK-7">M0CHK-7</a></td><td><a href="find.cgi?call=M0OZH-7">M0OZH-7</a></td><td><a href="find.cgi?call=MB7UFO-1">MB7UFO-1</a></td><td><a href="find.cgi?call=MB7UN">MB7UN</a></td><td><a href="find.cgi?call=MM0DXE-15">MM0DXE-15</a></td><td><a href="find.cgi?call=PA2AYX-9">PA2AYX-9</a></td></tr><tr>
//<td><a href="find.cgi?call=PA3AQW-5">PA3AQW-5</a></td><td><a href="find.cgi?call=PD1C">PD1C</a></td><td><a href="find.cgi?call=PD5LWD-2">PD5LWD-2</a></td><td><a href="find.cgi?call=PI1ECO">PI1ECO</a></td></tr>


char * DoSummaryLine(struct STATIONRECORD * ptr, int n, int Width)
{
	static char Line2[80];
	int x;

	wsprintf(Line2, "<td><a href=""find.cgi?call=%s"">%s</a></td>",
		ptr->Callsign, ptr->Callsign);

	x = ++n/Width;
	x = x * Width;

	if (x == n)
		strcat(Line2, "</tr><tr>");

	return Line2;
}

char * DoDetailLine(struct STATIONRECORD * ptr)
{
	static char Line[512];
	double Lat = ptr->Lat;
	double Lon = ptr->Lon;
	char NS='N', EW='E';

	char LatString[20], LongString[20], DistString[20], BearingString[20];
	int Degrees;
	double Minutes;
	char Time[80];
	struct tm * TM;

	
//	if (ptr->ObjState == '_')	// Killed Object
//		return;

	TM = gmtime(&ptr->TimeLastUpdated);

	wsprintf(Time, "%.2d:%.2d:%.2d", TM->tm_hour, TM->tm_min, TM->tm_sec);

	if (ptr->Lat < 0)
	{
		NS = 'S';
		Lat=-Lat;
	}
	if (Lon < 0)
	{
		EW = 'W';
		Lon=-Lon;
	}

#pragma warning(push)
#pragma warning(disable:4244)

	Degrees = Lat;
	Minutes = Lat * 60.0 - (60 * Degrees);

	sprintf(LatString,"%2d°%05.2f'%c", Degrees, Minutes, NS);
		
	Degrees = Lon;

#pragma warning(pop)

	Minutes = Lon * 60 - 60 * Degrees;

	sprintf(LongString, "%3d°%05.2f'%c",Degrees, Minutes, EW);

	sprintf(DistString, "%6.1f", Distance(ptr->Lat, ptr->Lon));
	sprintf(BearingString, "%3.0f", Bearing(ptr->Lat, ptr->Lon));
	
	wsprintf(Line, "<tr><td align=""left""><a href=""find.cgi?call=%s"">&nbsp;%s%s</a></td><td align=""left"">%s</td><td align=""center"">%s  %s</td><td align=""right"">%s</td><td align=""right"">%s</td><td align=""left"">%s</td></tr>",
			ptr->Callsign, ptr->Callsign, 
			(strchr(ptr->Path, '*'))?  "*": "", &SymbolText[ptr->iconRow << 4 | ptr->iconCol][0], LatString, LongString, DistString, BearingString, Time);

	return Line;
}

 
int CompareFN(const void *a, const void *b) 
{ 
	struct STATIONRECORD * x;
	struct STATIONRECORD * y;

	_asm 
	{
		push eax
		mov eax, a
		mov eax, [eax]
		mov	x, eax
		mov eax, b
		mov eax, [eax]
		mov	y, eax
		pop eax
}

	return strcmp(x->Callsign, y->Callsign);
	/* strcmp functions works exactly as expected from
	comparison function */ 
} 



char * CreateStationList(BOOL RFOnly, BOOL WX, BOOL Mobile, char Objects, int * Count, char * Param)
{
	char Line[100000] = "";	
	struct STATIONRECORD * ptr = *StationRecords;
	int n = 0, i;
	struct STATIONRECORD * List[1000];
	int TableWidth = 8;

	if (Param[0])
	{
		char * Key, *Context;

		Key = strtok_s(Param, "=", &Context);

		TableWidth = atoi(Context);

		if (TableWidth == 0)
			TableWidth = 8;
	}

	// Build list of calls

	while (ptr)
	{
		if (ptr->ObjState == Objects && ptr->Lat != 0.0 && ptr->Lon != 0.0)
		{
			if ((WX && (ptr->LastWXPacket[0] == 0)) || (RFOnly && (ptr->LastPort == 0)) ||
				(Mobile && ((ptr->Speed < 0.1) || ptr->LastWXPacket[0] != 0)))
			{
				ptr = ptr->Next;
				continue;
			}

			List[n++] = ptr;

			if (n > 999)
				break;

		}
		ptr = ptr->Next;		
	}

	if (n >  1)
		qsort(List, n, 4, CompareFN);

	for (i = 0; i < n; i++)
	{
		if (RFOnly)
			strcat(Line, DoDetailLine(List[i]));
		else
			strcat(Line, DoSummaryLine(List[i], i, TableWidth));
	}	
		
	*Count = n;

	return _strdup(Line);
}

char * LookupKey(struct APRSConnectionInfo * sockptr, char * Key)
{
	struct STATIONRECORD * stn = sockptr->SelCall;

	if (strcmp(Key, "##MY_CALLSIGN##") == 0)
		return _strdup(LoppedAPRSCall);

	if (strcmp(Key, "##CALLSIGN##") == 0)
		return _strdup(sockptr->Callsign);

	if (strcmp(Key, "##CALLSIGN_NOSSID##") == 0)
	{
		char * Call = _strdup(sockptr->Callsign);
		char * ptr = strchr(Call, '-');
		if (ptr)
			*ptr = 0;
		return Call;
	}

	if (strcmp(Key, "##MY_WX_CALLSIGN##") == 0)
		return _strdup(LoppedAPRSCall);

	if (strcmp(Key, "##MY_BEACON_COMMENT##") == 0)
		return _strdup(StatusMsg);

	if (strcmp(Key, "##MY_WX_BEACON_COMMENT##") == 0)
		return _strdup(WXComment);

	if (strcmp(Key, "##MILES_KM##") == 0)
		return _strdup("Miles");

	if (strcmp(Key, "##EXPIRE_TIME##") == 0)
	{
		char val[80];
		wsprintf(val, "%d", ExpireTime);
		return _strdup(val);
	}

	if (strcmp(Key, "##LOCATION##") == 0)
	{
		char val[80];
		double Lat = sockptr->SelCall->Lat;
		double Lon = sockptr->SelCall->Lon;
		char NS='N', EW='E';
		char LatString[20];
		int Degrees;
		double Minutes;
	
		if (Lat < 0)
		{
			NS = 'S';
			Lat=-Lat;
		}
		if (Lon < 0)
		{
			EW = 'W';
			Lon=-Lon;
		}

#pragma warning(push)
#pragma warning(disable:4244)

		Degrees = Lat;
		Minutes = Lat * 60.0 - (60 * Degrees);

		sprintf(LatString,"%2d°%05.2f'%c",Degrees, Minutes, NS);
		
		Degrees = Lon;

#pragma warning(pop)

		Minutes = Lon * 60 - 60 * Degrees;

		sprintf(val,"%s %3d°%05.2f'%c", LatString, Degrees, Minutes, EW);

		return _strdup(val);
	}

	if (strcmp(Key, "##LOCDDMMSS##") == 0)
	{
		char val[80];
		double Lat = sockptr->SelCall->Lat;
		double Lon = sockptr->SelCall->Lon;
		char NS='N', EW='E';
		char LatString[20];
		int Degrees;
		double Minutes;

		// 48.45.18N, 002.18.37E
			
		if (Lat < 0)
		{
			NS = 'S';
			Lat=-Lat;
		}
		if (Lon < 0)
		{
			EW = 'W';
			Lon=-Lon;
		}

#pragma warning(push)
#pragma warning(disable:4244)

		Degrees = Lat;
		Minutes = Lat * 60.0 - (60 * Degrees);
//		IntMins = Minutes;
//		Seconds = Minutes * 60.0 - (60 * IntMins);

		sprintf(LatString,"%2d.%05.2f%c",Degrees, Minutes, NS);
		
		Degrees = Lon;
		Minutes = Lon * 60.0 - 60 * Degrees;
//		IntMins = Minutes;
//		Seconds = Minutes * 60.0 - (60 * IntMins);

#pragma warning(pop)

		sprintf(val,"%s, %03d.%05.2f%c", LatString, Degrees, Minutes, EW);

		return _strdup(val);
	}
	if (strcmp(Key, "##STATUS_TEXT##") == 0)
		return _strdup(stn->Status);
	
	if (strcmp(Key, "##LASTPACKET##") == 0)
		return _strdup(stn->LastPacket);


	if (strcmp(Key, "##LAST_HEARD##") == 0)
	{
		char Time[80];
		struct tm * TM;
		time_t Age = time(NULL) - stn->TimeLastUpdated;

		TM = gmtime(&Age);

		wsprintf(Time, "%.2d:%.2d:%.2d", TM->tm_hour, TM->tm_min, TM->tm_sec);

		return _strdup(Time);
	}

	if (strcmp(Key, "##FRAME_HEADER##") == 0)
		return _strdup(stn->Path);

	if (strcmp(Key, "##FRAME_INFO##") == 0)
		return _strdup(stn->LastWXPacket);
	
	if (strcmp(Key, "##BEARING##") == 0)
	{
		char val[80];

		sprintf(val, "%03.0f", Bearing(sockptr->SelCall->Lat, sockptr->SelCall->Lon));
		return _strdup(val);
	}

	if (strcmp(Key, "##COURSE##") == 0)
	{
		char val[80];

		sprintf(val, "%03.0f", stn->Course);
		return _strdup(val);
	}

	if (strcmp(Key, "##SPEED_MPH##") == 0)
	{
		char val[80];

		sprintf(val, "%5.1f", stn->Speed);
		return _strdup(val);
	}

	if (strcmp(Key, "##DISTANCE##") == 0)
	{
		char val[80];

		sprintf(val, "%5.1f", Distance(sockptr->SelCall->Lat, sockptr->SelCall->Lon));
		return _strdup(val);
	}



	if (strcmp(Key, "##WIND_DIRECTION##") == 0)
	{
		char val[80];

		sprintf(val, "%03d", sockptr->WindDirn);
		return _strdup(val);
	}

	if (strcmp(Key, "##WIND_SPEED_MPH##") == 0)
	{
		char val[80];

		sprintf(val, "%d", sockptr->WindSpeed);
		return _strdup(val);
	}

	if (strcmp(Key, "##WIND_GUST_MPH##") == 0)
	{
		char val[80];

		sprintf(val, "%d", sockptr->WindGust);
		return _strdup(val);
	}

	if (strcmp(Key, "##TEMPERATURE_F##") == 0)
	{
		char val[80];

		sprintf(val, "%d", sockptr->Temp);
		return _strdup(val);
	}

	if (strcmp(Key, "##HUMIDITY##") == 0)
	{
		char val[80];

		sprintf(val, "%d", sockptr->Humidity);
		return _strdup(val);
	}

	if (strcmp(Key, "##PRESSURE_HPA##") == 0)
	{
		char val[80];

		sprintf(val, "%05.1f", sockptr->Pressure /10.0);
		return _strdup(val);
	}

	if (strcmp(Key, "##RAIN_TODAY_IN##") == 0)
	{
		char val[80];

		sprintf(val, "%5.2f", sockptr->RainToday /100.0);
		return _strdup(val);
	}


	if (strcmp(Key, "##RAIN_24_IN##") == 0)
	{
		char val[80];

		sprintf(val, "%5.2f", sockptr->RainLastDay /100.0);
		return _strdup(val);
	}


	if (strcmp(Key, "##RAIN_HOUR_IN##") == 0)
	{
		char val[80];

		sprintf(val, "%5.2f", sockptr->RainLastHour /100.0);
		return _strdup(val);
	}

	if (strcmp(Key, "##MAP_LAT_LON##") == 0)
	{
		char val[256];

		sprintf(val, "%f,%f", stn->Lat, stn->Lon);
		return _strdup(val);
	}

	if (strcmp(Key, "##SYMBOL_DESCRIPTION##") == 0)
		return _strdup(&SymbolText[stn->iconRow << 4 | stn->iconCol][0]);


/*
##WIND_SPEED_MS## - wind speed metres/sec
##WIND_SPEED_KMH## - wind speed km/hour
##WIND_GUST_MPH## - wind gust miles/hour
##WIND_GUST_MS## - wind gust metres/sec
##WIND_GUST_KMH## - wind gust km/hour
##WIND_CHILL_F## - wind chill F
##WIND_CHILL_C## - wind chill C
##TEMPERATURE_C## - temperature C
##DEWPOINT_F## - dew point temperature F
##DEWPOINT_C## - dew point temperature C
##PRESSURE_IN## - pressure inches of mercury
##PRESSURE_HPA## - pressure hPa (mb)
##RAIN_HOUR_MM## - rain in last hour mm
##RAIN_TODAY_MM## - rain today mm
##RAIN_24_MM## - rain in last 24 hours mm
##FRAME_HEADER## - frame header of the last posit heard from the station
##FRAME_INFO## - information field of the last posit heard from the station
##MAP_LARGE_SCALE##" - URL of a suitable large scale map on www.vicinity.com
##MEDIUM_LARGE_SCALE##" - URL of a suitable medium scale map on www.vicinity.com
##MAP_SMALL_SCALE##" - URL of a suitable small scale map on www.vicinity.com
##MY_LOCATION## - 'Latitude', 'Longitude' in 'Station Setup'
##MY_STATUS_TEXT## - status text configured in 'Status Text'
##MY_SYMBOL_DESCRIPTION## - 'Symbol' that would be shown for our station in 'Station List'
##HIT_COUNTER## - The number of times the page has been accessed
##DOCUMENT_LAST_CHANGED## - The date/time the page was last edited

##FRAME_HEADER## - frame header of the last posit heard from the station
##FRAME_INFO## - information field of the last posit heard from the station

*/
	return NULL;
}

VOID ProcessSpecialPage(struct APRSConnectionInfo * sockptr, char * Buffer, int FileSize, char * StationTable, int Count, BOOL WX)
{
	// replaces ##xxx### constructs with the requested data

	char * NewMessage = malloc(250000);
	char * ptr1 = Buffer, * ptr2, * ptr3, * ptr4, * NewPtr = NewMessage;
	int PrevLen;
	int BytesLeft = FileSize;
	int NewFileSize = FileSize;
	char * StripPtr = ptr1;
	int HeaderLen;
	char Header[256];
	int cbWritten;

	if (WX && sockptr->SelCall && sockptr->SelCall->LastWXPacket)
	{
		DecodeWXReport(sockptr, sockptr->SelCall->LastWXPacket);
	}

	// strip comments blocks

	while (ptr4 = strstr(ptr1, "<!--"))
	{
		ptr2 = strstr(ptr4, "-->");
		if (ptr2)
		{
			PrevLen = (ptr4 - ptr1);
			memcpy(StripPtr, ptr1, PrevLen);
			StripPtr += PrevLen;
			ptr1 = ptr2 + 3;
			BytesLeft = FileSize - (ptr1 - Buffer);
		}
	}


	memcpy(StripPtr, ptr1, BytesLeft);
	StripPtr += BytesLeft;

	BytesLeft = StripPtr - Buffer;

	FileSize = BytesLeft;
	NewFileSize = FileSize;
	ptr1 = Buffer;
	ptr1[FileSize] = 0;

loop:
	ptr2 = strstr(ptr1, "##");

	if (ptr2)
	{
		PrevLen = (ptr2 - ptr1);			// Bytes before special text
		
		ptr3 = strstr(ptr2+2, "##");

		if (ptr3)
		{
			char Key[80] = "";
			int KeyLen;
			char * NewText;
			int NewTextLen;

			ptr3 += 2;
			KeyLen = ptr3 - ptr2;

			if (KeyLen < 80)
				memcpy(Key, ptr2, KeyLen);

			if (strcmp(Key, "##STATION_TABLE##") == 0)
			{
				NewText = _strdup(StationTable);
			}
			else
			{
				if (strcmp(Key, "##TABLE_COUNT##") == 0)
				{
					char val[80];
					wsprintf(val, "%d", Count);
					NewText = _strdup(val);
				}
				else
					NewText = LookupKey(sockptr, Key);
			}
			
			if (NewText)
			{
				NewTextLen = strlen(NewText);
				NewFileSize = NewFileSize + NewTextLen - KeyLen;					
			//	NewMessage = realloc(NewMessage, NewFileSize);

				memcpy(NewPtr, ptr1, PrevLen);
				NewPtr += PrevLen;
				memcpy(NewPtr, NewText, NewTextLen);
				NewPtr += NewTextLen;

				free(NewText);
				NewText = NULL;
			}
			else
			{
				// Key not found, so just leave

				memcpy(NewPtr, ptr1, PrevLen + KeyLen);
				NewPtr += (PrevLen + KeyLen);
			}

			ptr1 = ptr3;			// Continue scan from here
			BytesLeft = Buffer + FileSize - ptr3;
		}
		else		// Unmatched ##
		{
			memcpy(NewPtr, ptr1, PrevLen + 2);
			NewPtr += (PrevLen + 2);
			ptr1 = ptr2 + 2;
		}
		goto loop;
	}

	// Copy Rest

	memcpy(NewPtr, ptr1, BytesLeft);
	
	HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", NewFileSize);
	WriteFile(sockptr->hPipe, Header, HeaderLen, &cbWritten, NULL); 
	WriteFile(sockptr->hPipe, NewMessage, NewFileSize, &cbWritten, NULL); 

	FlushFileBuffers(sockptr->hPipe); 

	free (NewMessage);
	free(StationTable);
	
	return;
}

VOID SendMessageFile(struct APRSConnectionInfo * sockptr, char * FN)
{
	int FileSize;
	char * MsgBytes;
	char MsgFile[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	int ReadLen;
	BOOL Special = FALSE;
	int HeaderLen;
	char Header[256];
	int cbWritten;
	char * Param;

	FN = strtok_s(FN, "?", &Param);

	if (strcmp(FN, "/") == 0)
		sprintf_s(MsgFile, sizeof(MsgFile), "%s\\%s\\index.html", APRSDir, SpecialDocs);
	else
		sprintf_s(MsgFile, sizeof(MsgFile), "%s\\%s%s", APRSDir, SpecialDocs, &FN[5]);
	
	hFile = CreateFile(MsgFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		// Try normal pages

		if (strcmp(FN, "/") == 0)
			sprintf_s(MsgFile, sizeof(MsgFile), "%s\\%s\\index.html", APRSDir, HTDocs);
		else
			sprintf_s(MsgFile, sizeof(MsgFile), "%s\\%s%s", APRSDir, HTDocs, &FN[5]);
	
		hFile = CreateFile(MsgFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			HeaderLen = sprintf(Header, "HTTP/1.0 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
			WriteFile(sockptr->hPipe, Header, HeaderLen, &cbWritten, NULL); 
			return;

		}
	}
	else
		Special = TRUE;

	FileSize = GetFileSize(hFile, NULL);

	MsgBytes=malloc(FileSize+1);

	ReadFile(hFile, MsgBytes, FileSize, &ReadLen, NULL); 

	CloseHandle(hFile);

	// if HTML file, look for ##...## substitutions

	if ((strstr(FN, "htm" ) || strstr(FN, "HTM")) &&  strstr(MsgBytes, "##" ))
	{
		// Build Station list, depending on URL
	
		int Count = 0;
		BOOL RFOnly = (BOOL)strstr(_strlwr(FN), "rf");		// Leaves FN in lower case
		BOOL WX = (BOOL)strstr(FN, "wx");
		BOOL Mobile = (BOOL)strstr(FN, "mobile");
		char Objects = (strstr(FN, "obj"))? '*' :0;
		char * StationList = CreateStationList(RFOnly, WX, Mobile, Objects, &Count, Param);

		ProcessSpecialPage(sockptr, MsgBytes, FileSize, StationList, Count, WX); 
		free (MsgBytes);
		return;			// ProcessSpecial has sent the reply
	}

	HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", FileSize);
	WriteFile(sockptr->hPipe, Header, HeaderLen, &cbWritten, NULL); 
	WriteFile(sockptr->hPipe, MsgBytes, FileSize, &cbWritten, NULL); 

	FlushFileBuffers(sockptr->hPipe); 
	free (MsgBytes);
}

char PipeFileName[] = "\\\\.\\pipe\\BPQAPRSWebPipe";

DWORD WINAPI InstanceThread(LPVOID lpvParam)

// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{ 
   DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0; 
   BOOL fSuccess = FALSE;
   HANDLE hPipe  = NULL;
   char Buffer[4096];
   char OutBuffer[100000];
   char * MsgPtr;
   int InputLen = 0;
   int OutputLen = 0;
   	char * URL;
	char * ptr;
	struct APRSConnectionInfo CI;
	struct APRSConnectionInfo * sockptr = &CI;
	char Key[12];

	__try
	{
//	Debugprintf("InstanceThread created, receiving and processing messages.");

// The thread's parameter is a handle to a pipe object instance. 
 
   sockptr->hPipe = hPipe = (HANDLE) lpvParam; 

   // Read client requests from the pipe. This simplistic code only allows messages
   // up to BUFSIZE characters in length.
 
   fSuccess = ReadFile( 
         hPipe,        // handle to pipe 
         Buffer,    // buffer to receive data 
         4096, // size of buffer 
         &InputLen, // number of bytes read 
         NULL);        // not overlapped I/O 

	if (!fSuccess || InputLen == 0)
	{   
		if (GetLastError() == ERROR_BROKEN_PIPE)
			Debugprintf("InstanceThread: client disconnected.", GetLastError()); 
		else
			Debugprintf("InstanceThread ReadFile failed, GLE=%d.", GetLastError()); 
	}
	else
	{
		Buffer[InputLen] = 0;

		MsgPtr = &Buffer[0];

		if (memcmp(MsgPtr, "GET" , 3) != 0)
		{
			Debugprintf(MsgPtr);
			OutputLen = sprintf(OutBuffer, "HTTP/1.0 501 Not Supported\r\n\r\n");
			WriteFile(sockptr->hPipe, OutBuffer, OutputLen, &cbWritten, NULL); 

			FlushFileBuffers(sockptr->hPipe); 
			DisconnectNamedPipe(hPipe); 
			CloseHandle(hPipe);
			return 1;

		}

		URL = &MsgPtr[4];

		ptr = strstr(URL, " HTTP");

		if (ptr)
			*ptr = 0;

//		Debugprintf("Processing %s", URL);

		if (_memicmp(URL, "/aprs/find.cgi?call=", 20) == 0)
		{
			// return Station details

			char * Call = &URL[20];
			BOOL RFOnly, WX, Mobile, Object = FALSE;
			struct STATIONRECORD * stn;
			char * Referrer = strstr(ptr + 1, "Referer:");

			strcpy(Key, Call);

			if (Referrer)
			{
				ptr = strchr(Referrer, 13);
				if (ptr)
				{
					*ptr = 0;
					RFOnly = (BOOL)strstr(Referrer, "rf");
					WX = (BOOL)strstr(Referrer, "wx");
					Mobile = (BOOL)strstr(Referrer, "mobile");
					Object = (BOOL)strstr(Referrer, "obj");

					if (WX)
						strcpy(URL, "/aprs/infowx_call.html");
					else if (Mobile)
						strcpy(URL, "/aprs/infomobile_call.html");
					else if (Object)
						strcpy(URL, "/aprs/infoobj_call.html");
					else
						strcpy(URL, "/aprs/info_call.html");
				}
			}

			if (Object)
			{
				// Name is space padded, and could have embedded spaces
				
				int Keylen = strlen(Key);
				
				if (Keylen < 9)
					memset(&Key[Keylen], 32, 9 - Keylen);
			}			
			
			stn = FindStation(Key, Key, FALSE);

			if (stn == NULL)
				strcpy(URL, "/aprs/noinfo.html");
			else
				sockptr->SelCall = stn;
		}

		strcpy(sockptr->Callsign, Key);

		SendMessageFile(sockptr, URL);
	}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Debugprintf("Program Error processing request");
	}

	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe); 
	return 1;
}

DWORD WINAPI PipeThreadProc(LPVOID lpvParam)
{
	BOOL   fConnected = FALSE; 
	DWORD  dwThreadId = 0; 
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL; 
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and this loop is free to wait for the
// next client connect request. It is an infinite loop.
 
	for (;;) 
	{ 
      hPipe = CreateNamedPipe( 
          PipeFileName,             // pipe name 
          PIPE_ACCESS_DUPLEX,       // read/write access 
          PIPE_TYPE_BYTE |       // message type pipe 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          4096,                  // output buffer size 
          4096,                  // input buffer size 
          0,                        // client time-out 
          NULL);                    // default security attribute 

      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          Debugprintf("CreateNamedPipe failed, GLE=%d.\n", GetLastError()); 
          return -1;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
      if (fConnected) 
	  {
         // Create a thread for this client. 
   
		 hThread = CreateThread( 
            NULL,              // no security attribute 
            0,                 // default stack size 
            InstanceThread,    // thread proc
            (LPVOID) hPipe,    // thread parameter 
            0,                 // not suspended 
            &dwThreadId);      // returns thread ID 

         if (hThread == NULL) 
         {
            Debugprintf("CreateThread failed, GLE=%d.\n", GetLastError()); 
            return -1;
         }
         else CloseHandle(hThread); 
       } 
      else 
        // The client could not connect, so close the pipe. 
         CloseHandle(hPipe); 
   } 

   return 0; 
} 

BOOL CreatePipeThread()
{
	DWORD ThreadId;
	CreateThread(NULL, 0, PipeThreadProc, 0, 0, &ThreadId);
	return TRUE;
}

