/*****************************************************************//**
 * \file   connect.c
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include <Windows.h>
#include <stdio.h>

#include "c_connect.h"


 //NOTE: Creating data structures shared between threads.
 //Need server socket and to create mutex for using that socket.
 //Will need to create mutex for STDOUT handle.
PLISTENERARGS
ChatCreate(SOCKET ServerSocket)
{

	PLISTENERARGS pListenerArgs = HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(LISTENERARGS));
	if (NULL == pListenerArgs)
	{
		DEBUG_ERROR("HeapAlloc failed");
		return NULL;
	}

	pListenerArgs->m_ServerSocket = ServerSocket;

	//NOTE: Mutexes are automatically released on program termination.
	pListenerArgs->m_hHandles[SOCKET_MUTEX] = CreateMutexW(NULL, FALSE, NULL);
	pListenerArgs->m_hHandles[STD_OUT_MUTEX] = CreateMutexW(NULL, FALSE, NULL);
	pListenerArgs->m_hHandles[STD_ERR_MUTEX] = CreateMutexW(NULL, FALSE, NULL);

	if ((NULL == pListenerArgs->m_hHandles[SOCKET_MUTEX])
		|| (NULL == pListenerArgs->m_hHandles[STD_OUT_MUTEX])
		|| (NULL == pListenerArgs->m_hHandles[STD_ERR_MUTEX]))
	{
		DEBUG_ERROR("CreateMutexW()");
		return NULL;
	}

	//NOTE: Create event for FD_READ
	pListenerArgs->m_hHandles[READ_EVENT] = WSACreateEvent();
	if (WSA_INVALID_EVENT == pListenerArgs->m_hHandles[READ_EVENT])
	{
		DEBUG_WSAERROR("WSACreateEvent failed");
		return NULL;
	}

	INT iReturn = WSAEventSelect(ServerSocket,
		pListenerArgs->m_hHandles[READ_EVENT], FD_READ);
	if (SOCKET_ERROR == iReturn)
	{
		DEBUG_WSAERROR("WSAEventSelect failed");
		return NULL;
	}

	return pListenerArgs;
}

//grab necessary structures and start listening - listener
//creates new threads
SOCKET
ChatConnect(PCLIENTCHATARGS pChatArgs)
{
	if (NULL == pChatArgs)
	{
		DEBUG_PRINT("Input NULL");
		return INVALID_SOCKET;
	}

	if (EXIT_FAILURE == NetSetUp())
	{
		DEBUG_PRINT("NetSetUp()");
		return INVALID_SOCKET;
	}

	SOCKET ServerSocket = NetConnect(pChatArgs->m_pszConnectIP,
		pChatArgs->m_pszConnectPort, NULL, NULL);

	if (INVALID_SOCKET == ServerSocket)
	{
		DEBUG_PRINT("NetConnect()");
		return INVALID_SOCKET;
	}

	return ServerSocket;
}

//End of file
