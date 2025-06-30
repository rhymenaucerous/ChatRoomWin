/*****************************************************************//**
 * \file   main.c
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include "..\networking\networking.h"

#include <Windows.h>
#include <stdio.h>
#include <strsafe.h>

#include "c_main.h"
#include "c_shared.h"
#include "c_srv_listen.h"
#include "c_user_input.h"
#include "c_connect.h"
#include "c_register.h"

volatile BOOL g_bClientState   = CONTINUE;
HANDLE        g_hShutdownEvent = NULL;

DWORD CustomWaitForSingleObject(HANDLE hInputEvent, DWORD dwTimeout)
{
    HANDLE hEvents[2] = {hInputEvent, g_hShutdownEvent};

    return WaitForMultipleObjects(2, hEvents, FALSE, dwTimeout);
}

/**
 * .
 *
 * \param pszCustomOutput
 * Null terminated wide char string provided by user.
 *
 * \param dwCustomLen The length of the cutsom string.
 *
 * \return HRESULT: E_HANDLE, E_UNEXPECTED, or S_OK.
 */
HRESULT
CustomConsoleWrite(PWCHAR pszCustomOutput, DWORD dwCustomLen)
{
    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    if (INVALID_HANDLE_VALUE == hConsoleOutput)
    {
        return E_HANDLE;
    }

    if (FALSE ==
        WriteConsoleW(hConsoleOutput, pszCustomOutput, dwCustomLen, NULL, NULL))
    {
        return E_UNEXPECTED;
    }

    return S_OK;
}

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
        SetEvent(g_hShutdownEvent);
		DEBUG_PRINT("Ctrl+C Observed!");
		return TRUE;

	case CTRL_CLOSE_EVENT:
		//NOTE: User closed the console.
        InterlockedExchange((PLONG)&g_bClientState, STOP);
        SetEvent(g_hShutdownEvent);
		return TRUE;

	case CTRL_BREAK_EVENT:
        InterlockedExchange((PLONG)&g_bClientState, STOP);
        SetEvent(g_hShutdownEvent);
		DEBUG_PRINT("Ctrl+break Observed!");
		return TRUE;

	case CTRL_LOGOFF_EVENT:
		//NOTE: User logged off.
        InterlockedExchange((PLONG)&g_bClientState, STOP);
        SetEvent(g_hShutdownEvent);
		return TRUE;

	case CTRL_SHUTDOWN_EVENT:
		//NOTE: System shutdown.
        InterlockedExchange((PLONG)&g_bClientState, STOP);
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

static VOID
PrintHelp()
{
	printf("\nChat Client Usage:\nchat_client.exe <connect_ip> "
		"<connect_port> <unique_client_name>\nExample:chat_client.exe "
		"192.168.0.10 1234 asdf\n\nIP and port must be a valid chat server. "
		"client name must not already be in use on server.\nOnly the first 10 "
		"characters of the given user name will be considered.\n");
}

static INT
CommandLineArgs(INT argc, PTSTR argv[], PCLIENTCHATARGS pChatArgs)
{
	if (4 != argc)
	{
		DEBUG_PRINT("Invalid Number of arguments");
		return ERR_INVALID_PARAM;
	}

	//IP will get checked by using it with NetConnect
	pChatArgs->m_pszConnectIP = argv[1];
	SIZE_T szIPLength = 0;
	if ((S_OK != StringCchLengthW(pChatArgs->m_pszConnectIP, CMD_LINE_MAX,
		&szIPLength)) || (szIPLength > HOST_MAX_STRING))
	{
		DEBUG_PRINT("Invalid IP");
		return ERR_INVALID_PARAM;
	}

	pChatArgs->m_pszConnectPort = argv[2];
	pChatArgs->m_dwConnectPort = wcstoul(argv[2], NULL, BASE_10);
	if (SUCCESS != PortRangeCheck(pChatArgs->m_dwConnectPort))
	{
		DEBUG_PRINT("Invalid Port");
		return ERR_INVALID_PARAM;
	}

	pChatArgs->m_pszClientName = argv[3];

	if ((S_OK != StringCchLengthW(pChatArgs->m_pszClientName, CMD_LINE_MAX,
		&pChatArgs->m_szNameLength)) ||
		(pChatArgs->m_szNameLength > MAX_UNAME_LEN))
	{
		DEBUG_PRINT("Name too long");
		return ERR_INVALID_PARAM;
	}

	return SUCCESS;
}

static VOID
ClientShutdown(PCLIENTCHATARGS pChatArgs, PLISTENERARGS pListenerArgs,
	BOOL bPrintHelp)
{
	if (NULL != pChatArgs)
	{
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pChatArgs,
			sizeof(CLIENTCHATARGS));
	}

	if (NULL != pListenerArgs)
	{
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pListenerArgs,
			sizeof(LISTENERARGS));
	}

	if (bPrintHelp)
	{
		PrintHelp();
	}
}

INT
wmain(INT argc, PTSTR argv[])
{
    g_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!SetConsoleCtrlHandler(GracefulShutdown, TRUE))
	{
		DEBUG_ERROR("CreateThread failed");
		return ERR_GENERIC;
	}

	PCLIENTCHATARGS pChatArgs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(CLIENTCHATARGS));
	if (NULL == pChatArgs)
	{
		DEBUG_ERROR("CloseHandle failed");
		return ERR_GENERIC;
	}

	INT iResult = CommandLineArgs(argc, argv, pChatArgs);
	if (SUCCESS != iResult)
	{
		DEBUG_PRINT("CommandLineArgs failed");
		ClientShutdown(pChatArgs, NULL, PRINT_HELP);
		return ERR_GENERIC;
	}

	//NOTE: Connects to the server socket.
	SOCKET ServerSocket = ChatConnect(pChatArgs);
	if (INVALID_SOCKET == ServerSocket)
	{
		DEBUG_PRINT("ChatConnect failed");
		ClientShutdown(pChatArgs, NULL, PRINT_HELP);
		return ERR_GENERIC;
	}

	//NOTE: Creates structures to share data/resources between threads.
	PLISTENERARGS pListenerArgs = ChatCreate(ServerSocket);
	if (NULL == pListenerArgs)
	{
		DEBUG_PRINT("ChatCreate failed");
		ClientShutdown(pChatArgs, pListenerArgs, DONT_PRINT_HELP);
		return ERR_GENERIC;
	}

	if (S_OK != HandleRegistration(pChatArgs->m_pszClientName,
		pChatArgs->m_szNameLength, pListenerArgs))
	{
		DEBUG_PRINT("HandleRegistration failed");
		ClientShutdown(pChatArgs, pListenerArgs, DONT_PRINT_HELP);
		return ERR_GENERIC;
	}

	//NOTE: Creating thread to handle server messages. Will continuously wait
	//for chat type messages from the server.
	HANDLE hListenerThread = CreateThread(NULL, NO_OPTION,
		(LPTHREAD_START_ROUTINE)(PULONG)ListenForChats,
		pListenerArgs, NO_OPTION, NULL);

	if (NULL == hListenerThread)
	{
		DEBUG_ERROR("CreateThread failed");
		ClientShutdown(pChatArgs, pListenerArgs, DONT_PRINT_HELP);
		return ERR_GENERIC;
	}

	//NOTE: Function for handling user input to the client - sends and recieves
	//any and all built-in commands. Will hold mutex until feedback is recieved
	//from the server.
	if (S_OK != UserListen(pListenerArgs))
	{
		DEBUG_WSAERROR("UserListen failed");
	}
	InterlockedExchange((PLONG)&g_bClientState, STOP);

	//NOTE: Send signal to read event and wait for child thread to finish
	// before shutdown.
	if (FALSE == WSASetEvent(pListenerArgs->m_hHandles[READ_EVENT]))
	{
		DEBUG_WSAERROR("WSASetEvent failed");
	}

	if (WAIT_FAILED == CustomWaitForSingleObject(hListenerThread, INFINITE))
	{
		DEBUG_ERROR("WaitForSingleObject failed");
	}

	if (FALSE == CloseHandle(hListenerThread))
	{
		DEBUG_ERROR("CloseHandle failed");
	}

	//NOTE: Clean function for closing socket and calling WSAClose()
	if (SOCKET_ERROR == shutdown(ServerSocket, SD_BOTH))
	{
		DEBUG_WSAERROR("shutdown failed");
	}

	NetCleanup(ServerSocket, INVALID_SOCKET);
	ClientShutdown(pChatArgs, pListenerArgs, DONT_PRINT_HELP);

	return SUCCESS;
}

//End of file
