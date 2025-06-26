/*****************************************************************//**
 * \file   s_main.c
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include <Windows.h>
#include <stdio.h>
#include <strsafe.h> // StringCchLengthW
#include <wchar.h>   // wcstoul

#include "s_main.h"
#include "s_shared.h"
#include "s_listen.h"
#include "..\networking\networking.h"

volatile BOOL g_bServerState = CONTINUE;
HANDLE        g_hShutdownEvent = NULL;

DWORD CustomWaitForSingleObject(HANDLE hInputEvent, DWORD dwTimeout)
{
    HANDLE hEvents[2] = {hInputEvent, g_hShutdownEvent};

    return WaitForMultipleObjects(2, hEvents, FALSE, dwTimeout);
}

static BOOL WINAPI
GracefulShutdown(_In_ DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
		//NOTE: This fn's purpose is to handle ctrl+C command. However, any
		//other console event listed here should also result in function shut down.
		//As such, all other cases will have the same effect on g_bServerState.
	case CTRL_C_EVENT:
		g_bServerState = STOP;
        SetEvent(g_hShutdownEvent);
		DEBUG_PRINT("Ctrl+C Observed!");
		return TRUE;

	case CTRL_CLOSE_EVENT:
		//NOTE: User closed the console.
        SetEvent(g_hShutdownEvent);
		g_bServerState = STOP;
		return TRUE;

	case CTRL_BREAK_EVENT:
        g_bServerState = STOP;
        SetEvent(g_hShutdownEvent);
		DEBUG_PRINT("Ctrl+break Observed!");
		return TRUE;

	case CTRL_LOGOFF_EVENT:
		//NOTE: User logged off.
        g_bServerState = STOP;
        SetEvent(g_hShutdownEvent);
		return TRUE;

	case CTRL_SHUTDOWN_EVENT:
		//NOTE: System shutdown.
        g_bServerState = STOP;
        SetEvent(g_hShutdownEvent);
		return TRUE;

	default:
		return FALSE;
	}
}

static INT
PortRangeCheck(DWORD dwPort)
{
	if ((0 >= dwPort) || (65535 < dwPort))
	{
		DEBUG_PRINT("Port out of range");
        return ERR_INVALID_PARAM;
	}

	return SUCCESS;
}

static INT
MaxClientsCheck(DWORD dwMaxClients)
{
	if ((MIN_CLIENTS >= dwMaxClients) || (MAX_CLIENTS < dwMaxClients))
	{
		DEBUG_PRINT("Max clients out of range");
        return ERR_INVALID_PARAM;
	}

	return SUCCESS;
}

static VOID
PrintHelp()
{
	wprintf(L"\nChat Server Usage:\nserver_application.exe <bind_ip"
		"> <bind_port> <max number of clients>\nExample:server_application.exe "
		"192.168.0.10 1234 5.\n");
}

static INT
CommandLineArgs(INT argc, PTSTR argv[], PSERVERCHATARGS pChatArgs)
{
	PWCHAR pcCheck = NULL;

	if (4 != argc)
	{
		DEBUG_PRINT("Invalid Number of arguments");
        return ERR_INVALID_PARAM;
	}

	//IP will get checked by using it with NetConnect
	pChatArgs->m_pszBindIP = argv[1];
	SIZE_T szIPLength = 0;

	if ((S_OK != StringCchLengthW(pChatArgs->m_pszBindIP, CMD_LINE_MAX,
		&szIPLength)) || (szIPLength > HOST_MAX_STRING))
	{
		DEBUG_PRINT("Invalid IP");
        return ERR_INVALID_PARAM;
	}

	pChatArgs->m_pszBindPort = argv[2];
	pChatArgs->m_dwBindPort = wcstoul(argv[2], &pcCheck, BASE_10);

	if ((SUCCESS != PortRangeCheck(pChatArgs->m_dwBindPort)) ||
		((NULL != pcCheck) && (*pcCheck != L'\0')))
	{
		DEBUG_PRINT("Invalid Port");
        return ERR_INVALID_PARAM;
	}

	pChatArgs->m_dwMaxClients = wcstoul(argv[3], &pcCheck, BASE_10);
	if ((SUCCESS != MaxClientsCheck(pChatArgs->m_dwMaxClients)) ||
		((NULL != pcCheck) && (*pcCheck != L'\0')))
	{
		DEBUG_PRINT("Invalid Max Clients");
        return ERR_INVALID_PARAM;
	}


	return SUCCESS;
}

INT
wmain(INT argc, PTSTR argv[])
{
    g_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!SetConsoleCtrlHandler(GracefulShutdown, TRUE))
	{
		DEBUG_ERROR("SetConsoleCtrlHandler failed");
        return ERR_GENERIC;
	}

	//Accepts command line arguments
	DEBUG_PRINT("\n");

	PSERVERCHATARGS pChatArgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(SERVERCHATARGS));
	if (NULL == pChatArgs)
	{
		DEBUG_ERROR("HeapAlloc failed");
        return ERR_MEMORY_ALLOCATION;
	}

	INT iResult = CommandLineArgs(argc, argv, pChatArgs);
	if (SUCCESS != iResult)
	{
		DEBUG_PRINT("CommandLineArgs failed");
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pChatArgs,
			sizeof(SERVERCHATARGS));
		PrintHelp();
        return iResult;
	}

	ThreadCount(pChatArgs);
	if (S_OK != IOCPSetUp(pChatArgs))
	{
		DEBUG_ERROR("IOCPSetUp failed");
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pChatArgs,
			sizeof(SERVERCHATARGS));
		return ERR_GENERIC;
	}

	if (S_OK != ThreadSetUp(pChatArgs))
	{
		DEBUG_PRINT("ThreadSetUp failed");
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pChatArgs,
			sizeof(SERVERCHATARGS));
        return ERR_GENERIC;
	}

	//Threads are set up! Let's accept connections and send em to IOCP.
	iResult = ServerListen(pChatArgs);
	if (SUCCESS != iResult)
	{
		DEBUG_PRINT("ServerListen failed");
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pChatArgs,
			sizeof(SERVERCHATARGS));
        return iResult;
	}

	ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pChatArgs, sizeof(SERVERCHATARGS));

	return SUCCESS;
}

//End of file
