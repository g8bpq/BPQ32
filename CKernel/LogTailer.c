// LogTailer.cpp : Defines the entry point for the console application.
//

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <tchar.h>
#include "time.h"
#include "stdio.h"


int _tmain(int argc, _TCHAR* argv[])
{
	char buff[100];		
	char * ptr1;

	STARTUPINFO  StartupInfo;					// pointer to STARTUPINFO 
    PROCESS_INFORMATION  ProcessInformation; 	// pointer to PROCESS_INFORMATION 

	unsigned char Value[MAX_PATH];
	time_t T;
	struct tm * tm;

	if (argc != 3)
	{
		printf("Invalid Parameter");
		Sleep(3000);
		return 1;
	}

	T = time(NULL);
	tm = gmtime(&T);

	ptr1 = strstr(argv[2], "_.");

	if (ptr1)
	{
		*ptr1++ = 0;

		sprintf(&Value[0], "Tail.exe %s_%04d%02d%02d%s", argv[2],
				tm->tm_year +1900, tm->tm_mon+1, tm->tm_mday, ptr1);

	}

	ptr1 = strstr(argv[2], "__");

	if (ptr1)
	{
		*ptr1++ = 0;

		sprintf(&Value[0], "Tail.exe %s_%02d%02d%02d%s", argv[2],
				tm->tm_year - 100, tm->tm_mon+1, tm->tm_mday, ptr1);

	}
	StartupInfo.cb=sizeof(StartupInfo);
	StartupInfo.lpReserved=NULL; 
	StartupInfo.lpDesktop=NULL; 
	StartupInfo.lpTitle=NULL; 
	StartupInfo.dwFlags=0; 
	StartupInfo.cbReserved2=0; 
  	StartupInfo.lpReserved2=NULL;

	strcpy(buff, "Tail.exe ");
	strcat(buff, argv[2]);

	if (!CreateProcess(argv[1], &Value[0], NULL, NULL, FALSE,
							CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
							NULL,NULL,&StartupInfo,&ProcessInformation))
	{				
		int ret=GetLastError();
		ret++;
		return TRUE;
	}

	return TRUE;
}

