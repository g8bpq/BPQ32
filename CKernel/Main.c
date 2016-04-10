//
//	C replacement for Main.asm
//
#define Kernel

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#pragma data_seg("_BPQDATA")

#include "windows.h"
#include "winerror.h"


#include "time.h"
#include "stdio.h"
#include "io.h"
#include <fcntl.h>					 
//#include "vmm.h"
#include "SHELLAPI.H"

#include "AsmStrucs.h"

#include "SHELLAPI.H"
#include "kernelresource.h"
#include <tlhelp32.h>
#include "BPQTermMDI.h"

//#include "GetVersion.h"

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

//
//	Everything that has to go in shared memory must be initialised

//UINT	FREE_Q = 1;

