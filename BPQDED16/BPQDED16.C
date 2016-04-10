#include <windows.h>
#include <windowsx.h>
#include <toolhelp.h>
#include "c:\msdev\include\wownt16.h"

/*

    THIS DLL ALLOWS WINDOWS APPLICATIONS EXPECTING TO USE TFWIN TO USE BP32
    
    IT THUNKS ALL CALLS TO THE 32 BIT EQUIVALENT, BPQDED32

*/    
  

  
DWORD mInst32;
DWORD hKERNEL32;  

DWORD TfOpen32;
DWORD TfClose32;
DWORD TfGet32;
DWORD TfPut32;
DWORD TfChck32;
DWORD CheckBPQ32;
DWORD Sleep;   


int FAR PASCAL LibMain(HINSTANCE hinst, UINT wDS, UINT cbHeap, DWORD unused)
//
//	Called when the DLL is first initialised. Check that BPQDED32 is available,
//	and get entry points 
//
{  

                        
	OutputDebugString("BPQDED16 V 1.2 Dll Loaded\n");
	
     
    return TRUE;
}

int FAR PASCAL WEP(BOOL fSystemExit)
{
    //
    // DDL Unload routine - Release BPQ32.DLL.
    // 
    
    OutputDebugString("BPQDED16 WEP called\n");
    FreeLibrary32W(mInst32);
    return TRUE;

}

			  
BOOL WINAPI TfOpen(HWND hWnd) 
{
  	OutputDebugString("BPQDED16 TfOpen called\n");
  	
  	mInst32 = LoadLibraryEx32W( "bpqded32.dll", NULL, 0 ) ;

  	if (mInst32 == 0)
  	{
		MessageBox(NULL,"BPQDED16 Load of BPQDED32.dll failed - closing",
		   "BPQDED16",MB_ICONSTOP+MB_OK);

		return(FALSE);
	}
	
  	hKERNEL32 = LoadLibraryEx32W( "kernel32.dll", NULL, 0 ) ;

  	if (hKERNEL32 == 0)
  	{
		MessageBox(NULL,"BPQDED16 Load of KERNEL32.dll failed - closing",
		   "BPQDED16",MB_ICONSTOP+MB_OK);

		return(FALSE);
	}

  
 	TfOpen32=GetProcAddress32W (mInst32, "TfOpen");
    TfClose32=GetProcAddress32W (mInst32, "TfClose"); 
    TfGet32=GetProcAddress32W (mInst32, "TfGet");
    TfPut32=GetProcAddress32W (mInst32, "TfPut");
    TfChck32=GetProcAddress32W (mInst32, "TfChck");
    CheckBPQ32=GetProcAddress32W (mInst32, "CheckifBPQ32isLoaded");
      
    Sleep=GetProcAddress32W (hKERNEL32, "Sleep");
 
 
	if (!TfOpen32 || !TfClose32|| !TfGet32 || !TfPut32 || !TfChck32 )

  	{
		MessageBox(NULL,"Get BPQDED32 API Addresses failed - closing",
		   "BPQDED16",MB_ICONSTOP+MB_OK);

		return(FALSE);
    
    } 
    
    if (!Sleep)

  	{
		MessageBox(NULL,"Get Sleep API Addresses failed - closing",
		   "BPQDED16",MB_ICONSTOP+MB_OK);

		return(FALSE);
    
    } 
    
    // Get DEDHOST32 to load BPQ32.exe if BPQ32.dll is not already loaded
    
    if (CheckBPQ32) CallProcEx32W(0,0,CheckBPQ32);


	return LOWORD (CallProcEx32W(1,0,TfOpen32,(DWORD)hWnd));
}


BOOL  WINAPI TfClose(void) 
{
 	OutputDebugString("BPQDED16 TfClose called\n"); 
 	CallProcEx32W(0,0,TfClose32)     ;
 	FreeLibrary32W(mInst32);

	return 1;
}

 

int WINAPI TfGet(void) 
{
	int retval;
	
	retval = LOWORD (CallProcEx32W(0,0,TfGet32));
	
 	if (retval == -1)
 	{
 	 	Yield();
 	 	CallProcEx32W(1,0,Sleep,(DWORD)2);
 	}
 	  
 	return LOWORD (retval);
}


BOOL WINAPI TfPut(char character) 
{
 	return LOWORD (CallProcEx32W(1,0,TfPut32,(DWORD)character));
}


int WINAPI TfChck(void) 
{
 	return LOWORD (CallProcEx32W(0,0,TfChck32));
}

