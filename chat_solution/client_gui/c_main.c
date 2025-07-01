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

void AppendToDisplayNoNewline(const PWCHAR text)
{
    DWORD dwSel = GetWindowTextLengthW(hDisplayBox);
    SendMessageW(hDisplayBox, EM_SETSEL, (WPARAM)dwSel, (LPARAM)dwSel);

    // Append the new text. The ES_AUTOVSCROLL style will handle the scrolling.
    SendMessageW(hDisplayBox, EM_REPLACESEL, FALSE, (LPARAM)text);
}

void AppendToDisplay(const PWCHAR text)
{
    DWORD dwSel = GetWindowTextLengthW(hDisplayBox);
    SendMessageW(hDisplayBox, EM_SETSEL, (WPARAM)dwSel, (LPARAM)dwSel);

    // Append the new text. The ES_AUTOVSCROLL style will handle the scrolling.
    SendMessageW(hDisplayBox, EM_REPLACESEL, FALSE, (LPARAM)text);

    // Append the newline
    SendMessageW(hDisplayBox, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

VOID GetAndAssign(PWCHAR pAssign, PDWORD pdwLen)
{
    DWORD dwWaitRes = CustomWaitForSingleObject(g_hInputReadyEvent, INFINITE);
    if (WAIT_OBJECT_0 != dwWaitRes)
    {
        DEBUG_ERROR("CustomWaitForSingleObject");
        goto FAIL;
    }

    WCHAR buffer[BUFF_SIZE];
    if (NULL != pdwLen)
    {
        *pdwLen = GetWindowTextW(hInputBox, buffer, BUFF_SIZE);
    }
	else
	{
        GetWindowTextW(hInputBox, buffer, BUFF_SIZE);
	}

    // Append the actual input text if it's not empty
    wcscat_s(pAssign, BUFF_SIZE, buffer);

    // Clear input box
    SetWindowTextW(hInputBox, L"");

    // Set focus back to the input box after sending
    SetFocus(hInputBox);

	goto EXIT;
FAIL:
    InterlockedExchange((PLONG)&g_bClientState, STOP);
    SetEvent(g_hShutdownEvent);
EXIT:
    ResetEvent(g_hInputReadyEvent);
}

VOID GetAndAppend()
{
    DWORD dwWaitRes = CustomWaitForSingleObject(g_hInputReadyEvent, INFINITE);
	if (WAIT_OBJECT_0 != dwWaitRes)
	{
        DEBUG_ERROR("CustomWaitForSingleObject");
        goto FAIL;
	}

    WCHAR buffer[BUFF_SIZE];
    GetWindowTextW(hInputBox, buffer, BUFF_SIZE);

    // Append the actual input text if it's not empty
    if (wcslen(buffer) > 0)
    {
        AppendToDisplay(buffer);
    }

    // Clear input box
    SetWindowTextW(hInputBox, L"");

    // Set focus back to the input box after sending
    SetFocus(hInputBox);

    goto EXIT;
FAIL:
    InterlockedExchange((PLONG)&g_bClientState, STOP);
    SetEvent(g_hShutdownEvent);
EXIT:
    ResetEvent(g_hInputReadyEvent);
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

INT PortRangeCheck(DWORD dwPort)
{
	if ((0 >= dwPort) || (65535 < dwPort))
	{
		DEBUG_PRINT("Port out of range");
        return ERR_INVALID_PARAM;
	}

	return SUCCESS;
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
ClientShutdown(PCLIENTCHATARGS pChatArgs, PLISTENERARGS pListenerArgs)
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
}

INT InitializeClient(PVOID pParam)
{
    INT              iReturn         = ERR_GENERIC;
    PCONNECTION_INFO pThreadConnInfo = pParam;
    PCLIENTCHATARGS  pChatArgs       = NULL;
    PLISTENERARGS    pListenerArgs   = NULL;
    SOCKET           ServerSocket    = INVALID_SOCKET;

    if (NULL == pThreadConnInfo)
	{
		DEBUG_ERROR("Invalid connection info");
		return ERR_INVALID_PARAM;
    }

    g_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!SetConsoleCtrlHandler(GracefulShutdown, TRUE))
	{
		DEBUG_ERROR("CreateThread failed");
		return ERR_GENERIC;
    }

    pChatArgs =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLIENTCHATARGS));
	if (NULL == pChatArgs)
	{
		DEBUG_ERROR("CloseHandle failed");
		return ERR_GENERIC;
	}

    // --- Replace command line arg parsing with data from our struct ---
    pChatArgs->m_pszConnectIP   = pThreadConnInfo->szIpAddress;
    pChatArgs->m_pszConnectPort = pThreadConnInfo->szPort;
    pChatArgs->m_pszClientName  = pThreadConnInfo->szUserName;
    pChatArgs->m_szNameLength   = pThreadConnInfo->dwNameLength;

	//NOTE: Connects to the server socket.
	ServerSocket = ChatConnect(pChatArgs);
	if (INVALID_SOCKET == ServerSocket)
	{
        MessageBox(NULL,
                   L"Server not available at specified address",
                   NULL, MB_OK);
		DEBUG_PRINT("ChatConnect failed");
        goto EXIT;
	}

	//NOTE: Creates structures to share data/resources between threads.
	pListenerArgs = ChatCreate(ServerSocket);
	if (NULL == pListenerArgs)
	{
        DEBUG_PRINT("ChatCreate failed");
        goto EXIT;
	}

	if (S_OK != HandleRegistration(pChatArgs->m_pszClientName,
		pChatArgs->m_szNameLength, pListenerArgs))
	{
        DEBUG_PRINT("HandleRegistration failed");
        goto EXIT;
	}

	//NOTE: Creating thread to handle server messages. Will continuously wait
	//for chat type messages from the server.
	HANDLE hListenerThread = CreateThread(NULL, NO_OPTION,
		(LPTHREAD_START_ROUTINE)(PULONG)ListenForChats,
		pListenerArgs, NO_OPTION, NULL);

	if (NULL == hListenerThread)
	{
        DEBUG_ERROR("CreateThread failed");
        goto EXIT;
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

	iReturn = SUCCESS;
EXIT:
	NetCleanup(ServerSocket, INVALID_SOCKET);
    ClientShutdown(pChatArgs, pListenerArgs);
    InterlockedExchange((PLONG)&g_bClientState, STOP);
    if (NULL != g_hShutdownEvent)
	{
		SetEvent(g_hShutdownEvent);
    }
	return iReturn;
}

//End of file
