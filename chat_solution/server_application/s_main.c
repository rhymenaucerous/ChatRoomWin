/*****************************************************************//**
 * \file   s_main.c
 * \brief  
 * 
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include "pch.h"
#include "s_main.h"

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
		if (S_OK != CustomConsoleWrite(L"Ctrl+C Observed!\n", MSG_6_LEN))
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "CustomConsoleWrite()");
			return FALSE;
		}
		return TRUE;

	case CTRL_CLOSE_EVENT:
		//NOTE: User closed the console.
		g_bServerState = STOP;
		return TRUE;

	case CTRL_BREAK_EVENT:
		g_bServerState = STOP;
		if (S_OK != CustomConsoleWrite(L"Ctrl+break Observed!\n", MSG_6_LEN))
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "CustomConsoleWrite()");
			return FALSE;
		}
		return TRUE;

	case CTRL_LOGOFF_EVENT:
		//NOTE: User logged off.
		g_bServerState = STOP;
		return TRUE;

	case CTRL_SHUTDOWN_EVENT:
		//NOTE: System shutdown.
		g_bServerState = STOP;
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
		PrintErrorCustom((PCHAR)__func__, __LINE__, "Port out of range\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static INT
MaxClientsCheck(DWORD dwMaxClients)
{
	if (((MIN_CLIENTS - 1) >= dwMaxClients) || (MAX_CLIENTS < dwMaxClients))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "Max clients out of range\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static VOID
PrintHelp()
{
	CustomConsoleWrite(L"\nChat Server Usage:\nserver_application.exe <bind_ip"
		"> <bind_port> <max number of clients>\nExample:server_application.exe "
		"192.168.0.10 1234 5.", MSG_7_LEN);
}

static INT
CommandLineArgs(INT argc, PTSTR argv[], PSERVERCHATARGS pChatArgs)
{
	PWCHAR pcCheck = NULL;
	
	if (4 != argc)
	{
		CustomConsoleWrite(L"Invalid Number of arguments\n", MSG_1_LEN);
		return EXIT_FAILURE;
	}

	//IP will get checked by using it with NetConnect
	pChatArgs->m_pszBindIP = argv[1];
	SIZE_T szIPLength = 0;
	
	if ((S_OK != StringCchLengthW(pChatArgs->m_pszBindIP, CMD_LINE_MAX,
		&szIPLength)) || (szIPLength > HOST_MAX_STRING))
	{
		CustomConsoleWrite(L"Invalid IP\n", MSG_2_LEN);
		return EXIT_FAILURE;
	}

	pChatArgs->m_pszBindPort = argv[2];
	pChatArgs->m_dwBindPort = wcstoul(argv[2], &pcCheck, BASE_10);

	if ((EXIT_SUCCESS != PortRangeCheck(pChatArgs->m_dwBindPort)) ||
		((NULL != pcCheck) && (*pcCheck != L'\0')))
	{
		CustomConsoleWrite(L"Invalid Port\n", MSG_3_LEN);
		return EXIT_FAILURE;
	}

	pChatArgs->m_dwMaxClients = wcstoul(argv[3], &pcCheck, BASE_10);
	if ((EXIT_SUCCESS != MaxClientsCheck(pChatArgs->m_dwMaxClients)) ||
		((NULL != pcCheck) && (*pcCheck != L'\0')))
	{
		CustomConsoleWrite(L"Invalid Max Clients\n", MSG_4_LEN);
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}

INT
wmain(INT argc, PTSTR argv[])
{
	if (!SetConsoleCtrlHandler(GracefulShutdown, TRUE))
	{
		PrintError((PCHAR)__func__, __LINE__);
		return EXIT_FAILURE;
	}

	//Accepts command line arguments
	CustomConsoleWrite(L"\n", MSG_5_LEN);

	PSERVERCHATARGS pChatArgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(SERVERCHATARGS));

	if (NULL == pChatArgs)
	{
		PrintError((PCHAR)__func__, __LINE__);
		return EXIT_FAILURE;
	}

	INT iResult = CommandLineArgs(argc, argv, pChatArgs);

	if (EXIT_FAILURE == iResult)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "CommandLineArgs()");
		MyHeapFree(GetProcessHeap(), NO_OPTION, pChatArgs,
			sizeof(SERVERCHATARGS));
		PrintHelp();
		return EXIT_FAILURE;
	}

	ThreadCount(pChatArgs);
	if (S_OK != IOCPSetUp(pChatArgs))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "IOCPSetUp()");
		MyHeapFree(GetProcessHeap(), NO_OPTION, pChatArgs,
			sizeof(SERVERCHATARGS));
		return EXIT_FAILURE;
	}

	if (S_OK != ThreadSetUp(pChatArgs))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "ThreadSetUp()");
		MyHeapFree(GetProcessHeap(), NO_OPTION, pChatArgs,
			sizeof(SERVERCHATARGS));
		return EXIT_FAILURE;
	}

	//Threads are set up! Let's accept connections and send em to IOCP.
	iResult = ServerListen(pChatArgs);

	if (EXIT_FAILURE == iResult)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "ServerListen()");
		MyHeapFree(GetProcessHeap(), NO_OPTION, pChatArgs,
			sizeof(SERVERCHATARGS));
		return EXIT_FAILURE;
	}

	MyHeapFree(GetProcessHeap(), NO_OPTION, pChatArgs, sizeof(SERVERCHATARGS));

	return EXIT_SUCCESS;
}

//End of file
