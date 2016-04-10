	  							  	     
//	TFTRAP.dll

//	Traps calls to TFWIN32 etc to allow	debugging

 
#include "windows.h"

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )


HINSTANCE ExtDriver=0;


typedef int (FAR  *FARPROCX)();


int AttachedProcesses=0;

HINSTANCE mInst32;  

FARPROCX TfOpen32;
FARPROCX TfClose32;
FARPROCX TfGet32;
FARPROC TfPut32;
FARPROCX TfChck32;
   



BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{

	HANDLE hInstance;

	char Errbuff[100];
	char buff[20];
	int n;
	
				
	hInstance=hInst;
	
	switch( ul_reason_being_called )
	{
	
		case DLL_PROCESS_ATTACH:

			AttachedProcesses++;

			_itoa(AttachedProcesses,buff,10);
			strcpy(Errbuff,	"TFTRAP Process Attach - total = ");
			strcat(Errbuff,buff);
			OutputDebugString(Errbuff);

			mInst32 = LoadLibrary("bpqded32.dll") ;

  			if (mInst32 == 0)
  			{

				MessageBox(NULL,"TFTRAP Load of BPQDED32.dll failed - closing",
				"TFTRAP",MB_ICONSTOP+MB_OK);

				return(FALSE);
			}
	
 
 			TfOpen32=(FARPROCX)GetProcAddress (mInst32, "TfOpen");
 
			if (TfOpen32 == 0)
  			{

			n=GetLastError( );
			_itoa(n,buff,10);
			strcpy(Errbuff,	"TFTRAP GetProcAddress Error = ");
			strcat(Errbuff,buff);
			OutputDebugString(Errbuff);

				MessageBox(NULL,"TFTRAP GetProcAddress of TfOpen failed - closing",
				"TFTRAP",MB_ICONSTOP+MB_OK);

				return(FALSE);
			}
			TfClose32=(FARPROCX)GetProcAddress (mInst32, "TfClose"); 
			TfGet32=(FARPROCX)GetProcAddress (mInst32, "TfGet");
			TfPut32=GetProcAddress (mInst32, "TfPut");
			TfChck32=(FARPROCX)GetProcAddress (mInst32, "TfChck");
      
 
			if (!TfOpen32 || !TfClose32|| !TfGet32 || !TfPut32 || !TfChck32 )

  			{
				MessageBox(NULL,"Get BPQDED32 API Addresses failed - closing",
				   "TFTRAP",MB_ICONSTOP+MB_OK);

				return(FALSE);
    
			}   


            break;

        case DLL_PROCESS_DETACH:

			// Keep track of attaced processes

			AttachedProcesses--;

			_itoa(AttachedProcesses,buff,10);
			strcpy(Errbuff,	"TFTRAP Process Detach - total = ");
			strcat(Errbuff,buff);
			OutputDebugString(Errbuff);
    
		    FreeLibrary(mInst32);

			break;

         default:

            break;

 
	}
 
	return 1;
	
}
			   

			  
BOOL __stdcall TfOpen(HWND hWnd) 
{
 	int x;


	_asm {
	
	pushfd
	cld
	pushad

	mov	eax,hWnd
	push eax

	call TfOpen32

	mov	x,eax	
	
	popad
	popfd

	}

	return (x);

}


BOOL  WINAPI TfClose(void) 
{
 	return TfClose32();
}

 

int WINAPI TfGet(void) 
{
	char Errbuff[100];
	char buff[20];
 	int x;
	
	x=TfGet32();
	
	if (x != -1 ) {
		 
		_itoa(x,buff,10);
		strcpy(Errbuff,	"TFTRAP TfGet =    ");
		strcat(Errbuff,buff);
		Errbuff[15]=x; 
		OutputDebugString(Errbuff);
 	}
	return x;
}


BOOLEAN __stdcall TfPut(cc) 
{
	char Errbuff[100];
	char buff[20];
 	int x;
	
		 
	_itoa(cc,buff,10);
	strcpy(Errbuff,	"TFTRAP TfPut =    ");
	strcat(Errbuff,buff);
	Errbuff[15]=cc; 
	OutputDebugString(Errbuff);

	_asm {
	
	pushfd
	cld
	pushad

	mov	eax,cc
	push eax

	call TfPut32

	mov	x,eax	
	
	popad
	popfd

	}

	return (x);

}


int WINAPI TfChck(void) 
{
	return TfChck();
}

