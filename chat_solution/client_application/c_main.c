/*****************************************************************//**
 * \file   main.c
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include "pch.h"
#include "c_main.h"

//NOTE: Per the example given within the msdn documentation - it's alright to
//have simple print statements.
BOOL WINAPI
GracefulShutdown(_In_ DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
		//NOTE: This fn's purpose is to handle ctrl+C command. However, any
		//other console event listed here should also result in function shut
		// down. As such, all other cases will have the same effect on
		// g_bServerState.
	case CTRL_C_EVENT:
		InterlockedExchange((PLONG)&g_bClientState, STOP);
		if (S_OK != CustomConsoleWrite(L"Ctrl+C Observed!\n", MSG_6_LEN))
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "CustomConsoleWrite()");
			return FALSE;
		}
		return TRUE;

	case CTRL_CLOSE_EVENT:
		//NOTE: User closed the console.
		InterlockedExchange((PLONG)&g_bClientState, STOP);
		return TRUE;

	case CTRL_BREAK_EVENT:
		InterlockedExchange((PLONG)&g_bClientState, STOP);
		if (S_OK != CustomConsoleWrite(L"Ctrl+break Observed!\n", MSG_6_LEN))
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "CustomConsoleWrite()");
			return FALSE;
		}
		return TRUE;

	case CTRL_LOGOFF_EVENT:
		//NOTE: User logged off.
		InterlockedExchange((PLONG)&g_bClientState, STOP);
		return TRUE;

	case CTRL_SHUTDOWN_EVENT:
		//NOTE: System shutdown.
		InterlockedExchange((PLONG)&g_bClientState, STOP);
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

static VOID
PrintHelp()
{
	CustomConsoleWrite(L"\nChat Client Usage:\nchat_client.exe <connect_ip> "
		"<connect_port> <unique_client_name>\nExample:chat_client.exe "
		"192.168.0.10 1234 asdf\n\nIP and port must be a valid chat server. "
		"client name must not already be in use on server.\nOnly the first 10 "
		"characters of the given user name will be considered.", MSG_10_LEN);
}

static INT
CommandLineArgs(INT argc, PTSTR argv[], PCLIENTCHATARGS pChatArgs)
{
	if (4 != argc)
	{
		CustomConsoleWrite(L"Invalid Number of arguments\n", MSG_1_LEN);
		return EXIT_FAILURE;
	}

	//IP will get checked by using it with NetConnect
	pChatArgs->m_pszConnectIP = argv[1];
	SIZE_T szIPLength = 0;

	if ((S_OK != StringCchLengthW(pChatArgs->m_pszConnectIP, CMD_LINE_MAX,
		&szIPLength)) || (szIPLength > HOST_MAX_STRING))
	{
		CustomConsoleWrite(L"Invalid IP\n", MSG_2_LEN);
		return EXIT_FAILURE;
	}

	pChatArgs->m_pszConnectPort = argv[2];
	pChatArgs->m_dwConnectPort = wcstoul(argv[2], NULL, BASE_10);

	if (EXIT_SUCCESS != PortRangeCheck(pChatArgs->m_dwConnectPort))
	{
		CustomConsoleWrite(L"Invalid Port\n", MSG_3_LEN);
		return EXIT_FAILURE;
	}

	pChatArgs->m_pszClientName = argv[3];

	if ((S_OK != StringCchLengthW(pChatArgs->m_pszClientName, CMD_LINE_MAX,
		&pChatArgs->m_szNameLength)) ||
		(pChatArgs->m_szNameLength > MAX_UNAME_LEN))
	{
		CustomConsoleWrite(L"Name too long\n", MSG_4_LEN);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static VOID
ClientShutdown(PCLIENTCHATARGS pChatArgs, PLISTENERARGS pListenerArgs,
	BOOL bPrintHelp)
{
	if (NULL != pChatArgs)
	{
		MyHeapFree(GetProcessHeap(), NO_OPTION, pChatArgs,
			sizeof(CLIENTCHATARGS));
	}

	if (NULL != pListenerArgs)
	{
		MyHeapFree(GetProcessHeap(), NO_OPTION, pListenerArgs,
			sizeof(LISTENERARGS));
	}

	if (bPrintHelp)
	{
		PrintHelp();
	}
}

INT
wmain(INT argc, PTSTR argv[]) {

	if (!SetConsoleCtrlHandler(GracefulShutdown, TRUE))
	{
		PrintError((PCHAR)__func__, __LINE__);
		return EXIT_FAILURE;
	}
	
	//Accepts command line arguments
	CustomConsoleWrite(L"\n", MSG_5_LEN);

	PCLIENTCHATARGS pChatArgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(CLIENTCHATARGS));

	if (NULL == pChatArgs)
	{
		PrintError((PCHAR)__func__, __LINE__);
		return EXIT_FAILURE;
	}

	INT iResult = CommandLineArgs(argc, argv, pChatArgs);

	if (EXIT_FAILURE == iResult)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "CommandLineArgs()");
		ClientShutdown(pChatArgs, NULL, PRINT_HELP);
		return EXIT_FAILURE;
	}

	//NOTE: Connects to the server socket.
	SOCKET ServerSocket = ChatConnect(pChatArgs);
	if (INVALID_SOCKET == ServerSocket)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "ChatConnect()");
		ClientShutdown(pChatArgs, NULL, PRINT_HELP);
		return EXIT_FAILURE;
	}

	//NOTE: Creates structures to share data/resources between threads.
	PLISTENERARGS pListenerArgs = ChatCreate(ServerSocket);
	if (NULL == pListenerArgs)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "ChatCreate()");
		ClientShutdown(pChatArgs, pListenerArgs, DONT_PRINT_HELP);
		return EXIT_FAILURE;
	}

	if (S_OK != HandleRegistration(pChatArgs->m_pszClientName,
		pChatArgs->m_szNameLength, pListenerArgs))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "HandleRegistration()");
		ClientShutdown(pChatArgs, pListenerArgs, DONT_PRINT_HELP);
		return EXIT_FAILURE;
	}

	//NOTE: Creating thread to handle server messages. Will continuously wait
	//for chat type messages from the server.
	HANDLE hListenerThread = CreateThread(NULL, NO_OPTION,
		(LPTHREAD_START_ROUTINE)(PULONG)ListenForChats,
		pListenerArgs, NO_OPTION, NULL);

	if (NULL == hListenerThread)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "CreateThread()");
		ClientShutdown(pChatArgs, pListenerArgs, DONT_PRINT_HELP);
		return EXIT_FAILURE;
	}

	//NOTE: Function for handling user input to the client - sends and recieves
	//any and all built-in commands. Will hold mutex until feedback is recieved
	//from the server.
	if (S_OK != UserListen(pListenerArgs))
	{
		ThreadPrintErrorWSA(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__);
		ThreadCustomConsoleWrite(pListenerArgs->m_hHandles[STD_OUT_MUTEX],
			L"Looks like the server may have shut ""down, maybe try again "
			"later?", 65);
	}
	InterlockedExchange((PLONG)&g_bClientState, STOP);

	//NOTE: Send signal to read event and wait for child thread to finish
	// before shutdown.
	if (FALSE == WSASetEvent(pListenerArgs->m_hHandles[READ_EVENT]))
	{
		ThreadPrintErrorWSA(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__);
	}
	
	if (WAIT_OBJECT_0 != WaitForSingleObject(hListenerThread, INFINITE))
	{
		PrintError((PCHAR)__func__, __LINE__);
	}

	if (WIN_EXIT_FAILURE == CloseHandle(hListenerThread))
	{
		PrintError((PCHAR)__func__, __LINE__);
	}

	//NOTE: Clean function for closing socket and calling WSAClose()
	if (SOCKET_ERROR == shutdown(ServerSocket, SD_BOTH))
	{
		PrintErrorWSA((PCHAR)__func__, __LINE__);
	}

	NetCleanup(ServerSocket, INVALID_SOCKET);
	ClientShutdown(pChatArgs, pListenerArgs, DONT_PRINT_HELP);

	return EXIT_SUCCESS;
}

//End of file
