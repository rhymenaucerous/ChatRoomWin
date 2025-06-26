/*****************************************************************//**
 * \file   s_listen.c
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include <Windows.h>
#include <stdio.h>

#include "s_shared.h"
#include "s_listen.h"
#include "s_worker.h"
#include "s_message.h"
#include "s_main.h"
#include "Queue.h"

extern volatile BOOL g_bServerState;
extern HANDLE        g_hShutdownEvent;

VOID
ThreadCount(PSERVERCHATARGS pServerArgs)
{
	pServerArgs->m_dwThreadCount = 0;
	if (pServerArgs->m_dwMaxClients <= MIN_THREADS)
	{
		pServerArgs->m_dwThreadCount = MIN_THREADS;
	}
	else if (pServerArgs->m_dwMaxClients <= THREADS_16)
	{
		pServerArgs->m_dwThreadCount = THREADS_16;
	}
	else if (pServerArgs->m_dwMaxClients <= THREADS_32)
	{
		pServerArgs->m_dwThreadCount = THREADS_32;
	}
	else
	{
		pServerArgs->m_dwThreadCount = MAX_THREADS;
	}
}

static HRESULT
CreatePrintMutexes(PSERVERCHATARGS pServerArgs)
{
	pServerArgs->m_haSharedHandles[STD_OUT_MUTEX] = CreateMutexW(NULL, FALSE,
		NULL);
	pServerArgs->m_haSharedHandles[STD_ERR_MUTEX] = CreateMutexW(NULL, FALSE,
		NULL);
	if ((NULL == pServerArgs->m_haSharedHandles[STD_OUT_MUTEX]) ||
		(NULL == pServerArgs->m_haSharedHandles[STD_ERR_MUTEX]))
	{
		DEBUG_ERROR("CreateMutexW failed");
		return SRV_SHUTDOWN_ERR;
	}

	return S_OK;
}

//NOTE: Set up listening socket and IOCP handle.
HRESULT
IOCPSetUp(PSERVERCHATARGS pServerArgs)
{
	if (S_OK != CreatePrintMutexes(pServerArgs))
	{
		DEBUG_PRINT("CreatePrintMutexes failed");
		return SRV_SHUTDOWN_ERR;
	}

	if (SUCCESS != NetSetUp())
	{
		DEBUG_PRINT("NetSetUp failed");
		return SRV_SHUTDOWN_ERR;
	}

	pServerArgs->m_ListenSocket = NetListen(pServerArgs->m_pszBindIP,
		pServerArgs->m_pszBindPort);
	if (INVALID_SOCKET == pServerArgs->m_ListenSocket)
	{
		DEBUG_PRINT("NetListen failed");
		return SRV_SHUTDOWN_ERR;
	}

	//NOTE: The listening socket is now set up, let's create IOCP handle.
	pServerArgs->m_haSharedHandles[IOCP_HANDLE] = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE, NULL, NO_OPTION, pServerArgs->m_dwThreadCount);
	if (NULL == pServerArgs->m_haSharedHandles[IOCP_HANDLE])
	{
		DEBUG_PRINT("CreateIoCompletionPort failed");
		NetCleanup(pServerArgs->m_ListenSocket, INVALID_SOCKET);
		return SRV_SHUTDOWN_ERR;
	}

	return S_OK;
}

//NOTE: Output will point to an array of handles.
//NOTE: We know that max clients will be within 2-65535
HRESULT
ThreadSetUp(PSERVERCHATARGS pServerArgs)
{
	pServerArgs->m_phThreads = HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY, pServerArgs->m_dwThreadCount * sizeof(HANDLE));
	if (NULL == pServerArgs->m_phThreads)
	{
		DEBUG_ERROR("HeapAlloc failed");
		return E_OUTOFMEMORY;
	}

	//NOTE: Threads created and handles stored.
	for (DWORD iCounter = 0; iCounter < pServerArgs->m_dwThreadCount;
		iCounter++)
	{
		pServerArgs->m_phThreads[iCounter] = CreateThread(NULL, NO_OPTION,
			WorkerThread, (PVOID)pServerArgs->m_haSharedHandles[IOCP_HANDLE],
			NO_OPTION, NULL);
		//NOTE: If any of the CreateThread() iterations fail, we need to ensure
		//the termination of those other threads.
		if (NULL == pServerArgs->m_phThreads[iCounter])
		{
			g_bServerState = STOP;
			DEBUG_ERROR("CreateThread failed");

			for (DWORD iCounter2 = 0; iCounter2 < iCounter; iCounter2++)
			{
				//NOTE: The worker thread will close following the reception
				//of this completion key.
				BOOL bResult = PostQueuedCompletionStatus(
					pServerArgs->m_haSharedHandles[IOCP_HANDLE], 0,
					IOCP_SHUTDOWN, NULL);
				if (FALSE == bResult)
				{
					//NOTE: Simply print error info, try to shutdown other
					// threads.
					DEBUG_ERROR("PostQueuedCompletionStatus failed");
				}
			}

			DWORD dwResult = WaitForMultipleObjects(iCounter,
				pServerArgs->m_phThreads, TRUE, INFINITE);
			//NOTE: Waiting for threads to close.
			switch (dwResult)
			{
			case WAIT_OBJECT_0:
				return SRV_SHUTDOWN_ERR;

			default:
				DEBUG_ERROR("WaitForMultipleObjects failed");
				return SRV_SHUTDOWN_ERR;
			}
		}
	}

	return S_OK;
}

static HRESULT
ThreadShutDown(PSERVERCHATARGS pServerArgs)
{
	BOOL bErrorOccured = FALSE;
	for (DWORD iCounter = 0; iCounter < pServerArgs->m_dwThreadCount;
		iCounter++)
	{
		BOOL bResult = PostQueuedCompletionStatus(
			pServerArgs->m_haSharedHandles[IOCP_HANDLE], 0, IOCP_SHUTDOWN,
			NULL);
		if (FALSE == bResult)
		{
			DEBUG_ERROR("PostQueuedCompletionStatus failed");
			bErrorOccured = TRUE;
			//NOTE: no error return here as all the threads need to close
			//either way.
		}
	}

	DWORD dwResult = WaitForMultipleObjects(pServerArgs->m_dwThreadCount,
		pServerArgs->m_phThreads, TRUE, INFINITE);

	//NOTE: Waiting for threads to close.
	switch (dwResult)
	{
	case WAIT_OBJECT_0:
		break;

	//NOTE: Any option except WAIT_OBJECT_0 means failure to close all threads.
	default:
		DEBUG_ERROR("WaitForMultipleObjects failed");
		bErrorOccured = TRUE;
		break;
	}

	ZeroingHeapFree(GetProcessHeap(), NO_OPTION, (PVOID)&pServerArgs->m_phThreads,
		pServerArgs->m_dwThreadCount * sizeof(HANDLE));

	if (bErrorOccured == FALSE)
	{
		return S_OK;
	}
	else
	{
		return SRV_SHUTDOWN_ERR;
	}
}

PUSERS
CreateUsers(PSERVERCHATARGS pServerArgs)
{
	PUSERS pUsers = HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY, sizeof(USERS));
	if (NULL == pUsers)
	{
		DEBUG_ERROR("HeapAlloc failed");
		return NULL;
	}

	pUsers->m_dwMaxClients = pServerArgs->m_dwMaxClients;
	//WARNING: Max clients set to 65535, so conversion to WORD type for entry
	// into the following function does not result in any data loss.
    if (SUCCESS != HashTableInit(&pUsers->m_pUsersHTable,
                                 (WORD)pServerArgs->m_dwMaxClients, NULL))
	{
		DEBUG_PRINT("HashTableInit failed");
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pUsers, sizeof(USERS));
		return NULL;
	}


    if (SUCCESS != HashTableInit(&pUsers->m_pNewUsersTable,
                                 (WORD)pServerArgs->m_dwMaxClients, NULL))
	{
		DEBUG_PRINT("HashTableInit failed");
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pUsers, sizeof(USERS));
		HashTableDestroy(pUsers->m_pUsersHTable, NULL);
		return NULL;
	}
	pUsers->m_haUsersHandles[STD_OUT_MUTEX] =
		pServerArgs->m_haSharedHandles[STD_OUT_MUTEX];
	pUsers->m_haUsersHandles[STD_ERR_MUTEX] =
		pServerArgs->m_haSharedHandles[STD_ERR_MUTEX];
	pUsers->m_haUsersHandles[USERS_WRITE_MUTEX] = CreateMutexW(NULL, FALSE,
		NULL);
	pUsers->m_haUsersHandles[NEW_USERS_MUTEX] = CreateMutexW(NULL, FALSE,
		NULL);
	//NOTE: The initial count on the semaphore is zero because no threads will
	//be reading initially.
	pUsers->m_haUsersHandles[USERS_READ_SEMAPHORE] = CreateSemaphoreW(NULL,
		pServerArgs->m_dwMaxClients, pServerArgs->m_dwMaxClients, NULL);
	//NOTE: Event will auto reset after releasing a thread
	pUsers->m_haUsersHandles[READERS_DONE_EVENT] = CreateEventW(NULL, FALSE,
		FALSE, NULL);

	if ((NULL == pUsers->m_haUsersHandles[USERS_WRITE_MUTEX]) ||
		(NULL == pUsers->m_haUsersHandles[NEW_USERS_MUTEX]) ||
		(NULL == pUsers->m_haUsersHandles[USERS_READ_SEMAPHORE]) ||
		(NULL == pUsers->m_haUsersHandles[READERS_DONE_EVENT]))
	{
		DEBUG_ERROR("CreateMutexW failed");
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pUsers, sizeof(USERS));
		HashTableDestroy(pUsers->m_pUsersHTable, NULL);
		HashTableDestroy(pUsers->m_pNewUsersTable, NULL);
		return NULL;
	}

	return pUsers;
}

//NOTE: Username must still be set during  login negotiation process.
//NOTE: WSABUF will be set in listening loop/worker threads.
//NOTE: m_dwBytestoRecv/m_dwBytesRecved must be set in server listening loop.
PUSER
CreateUser(PSERVERCHATARGS pServerArgs, PUSERS pUsers, SOCKET ClientSocket)
{
	PUSER pUser = HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY, sizeof(USER));
	if (NULL == pUser)
	{
		DEBUG_ERROR("HeapAlloc failed");
		return NULL;
	}


	pUser->m_haSharedHandles[SEND_MUTEX] = CreateMutexW(NULL, FALSE,
		NULL);
	//NOTE: Event is manual reset and the initial state is signaled.
	pUser->m_haSharedHandles[SEND_DONE_EVENT] = CreateEventW(NULL, TRUE,
		TRUE, NULL);

	if ((NULL == pUser->m_haSharedHandles[SEND_DONE_EVENT])  ||
		(NULL == pUser->m_haSharedHandles[SEND_MUTEX]))
	{
		DEBUG_ERROR("CreateMutexW failed");
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pUser,
			sizeof(USER));

		return NULL;
	}

	pUser->m_SendMsgQueue = QueueInit();

	if (NULL == pUser->m_SendMsgQueue)
	{
		DEBUG_ERROR("QueueInit failed");

		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pUser,
			sizeof(USER));

		return NULL;
	}

	pUser->m_haSharedHandles[STD_OUT_MUTEX] =
		pServerArgs->m_haSharedHandles[STD_OUT_MUTEX];
	pUser->m_haSharedHandles[STD_ERR_MUTEX] =
		pServerArgs->m_haSharedHandles[STD_ERR_MUTEX];
	pUser->m_ClientSocket = ClientSocket;
	pUser->m_pUsers = pUsers;
	pUser->m_plBeingDestroyed = NOT_DESTROYING;

	//NOTE: Setting conditions for asycronous recv.
	ResetChatRecv(&pUser->m_RecvMsg);

	return pUser;
}

static VOID
ServerShutDown(PSERVERCHATARGS pServerArgs, PUSERS pUsers)
{
	g_bServerState = STOP;

	if (S_OK != ThreadShutDown(pServerArgs))
	{
		DEBUG_PRINT("ThreadShutDown failed");
	}

	if (FALSE == CloseHandle(pServerArgs->m_haSharedHandles[IOCP_HANDLE]))
	{
		DEBUG_PRINT("CloseHandle failed");
	}

	if (SUCCESS != HashTableDestroy(pUsers->m_pUsersHTable,
		UserFreeFunction))
	{
		DEBUG_PRINT("HashTableDestroy failed");
	}

	if (SUCCESS != HashTableDestroy(pUsers->m_pNewUsersTable,
		UserFreeFunction))
	{
		DEBUG_PRINT("HashTableDestroy failed");
	}

	NetCleanup(pServerArgs->m_ListenSocket, INVALID_SOCKET);

	//NOTE: All server processes have now been shutdown, now let's free the
	// memory.
	ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pUsers,
		sizeof(USERS));

	//NOTE: pServerArgs is freed in wmain.
}


//grab necessary structures and start listening - listener
//creates new threads
HRESULT
ServerListen(PSERVERCHATARGS pServerArgs)
{
	PUSERS pUsers = CreateUsers(pServerArgs);
	if (NULL == pUsers)
	{
		DEBUG_PRINT("CreateUsers failed");
		ServerShutDown(pServerArgs, pUsers);
		return SRV_SHUTDOWN_ERR;
	}

	while(CONTINUE == g_bServerState)
	{
        SOCKET ClientSocket =
            NetAccept(pServerArgs->m_ListenSocket, g_bServerState, g_hShutdownEvent);

		//NOTE: Errors that are not fatal are handled within NetAccept.
		if (INVALID_SOCKET == ClientSocket)
		{
			//NOTE: The NetAccept fn can return INVALID_SOCKET if the server is
			//shut down on purpose, if it should still be running, we want the
			//diagnotic info printed.
			DEBUG_WSAERROR("NetAccept failed");
			ServerShutDown(pServerArgs, pUsers);
			if (CONTINUE == g_bServerState)
			{
				return SRV_SHUTDOWN_ERR;
			}
			return S_OK;
		}

		PUSER pUser = CreateUser(pServerArgs, pUsers, ClientSocket);
		if (NULL == pUser)
		{
			DEBUG_PRINT("CreateUser failed");
			ServerShutDown(pServerArgs, pUsers);
			return SRV_SHUTDOWN_ERR;
		}

		//NOTE: Client socket utilized for now as it will always be unique.
        DWORD dwWaitResult = CustomWaitForSingleObject(
			pUsers->m_haUsersHandles[NEW_USERS_MUTEX], INFINITE);
		if (WAIT_OBJECT_0 != dwWaitResult)
		{
			DEBUG_ERROR("CustomWaitForSingleObject failed");
			ReleaseMutex(pUsers->m_haUsersHandles[NEW_USERS_MUTEX]);
			ServerShutDown(pServerArgs, pUsers);
			return SRV_SHUTDOWN_ERR;
		}

		WORD wResult = HashTableNewEntry(pUsers->m_pNewUsersTable, pUser,
			(PCHAR)&(pUser->m_ClientSocket),
			(sizeof(SOCKET) / sizeof(WCHAR)));
		ReleaseMutex(pUsers->m_haUsersHandles[NEW_USERS_MUTEX]);
		if (SUCCESS != wResult)
		{
			DEBUG_PRINT("HashTableNewEntry failed");
			UserFreeFunction((PVOID)pUser);
			ServerShutDown(pServerArgs, pUsers);
			return SRV_SHUTDOWN_ERR;
		}

		HANDLE hResult = CreateIoCompletionPort((HANDLE)pUser->m_ClientSocket,
			pServerArgs->m_haSharedHandles[IOCP_HANDLE], (ULONG_PTR)pUser,
			NO_OPTION);

		if (NULL == hResult)
		{
			DEBUG_ERROR("CreateIoCompletionPort failed");
			ServerShutDown(pServerArgs, pUsers);
			return SRV_SHUTDOWN_ERR;
		}

		MSGHOLDER RecvHolder = pUser->m_RecvMsg;
		InterlockedIncrement(&pUser->m_plRecvOccuring);
		INT iResult = WSARecv(pUser->m_ClientSocket, RecvHolder.m_wsaBuffer,
			ONE_BUFFER, &(RecvHolder.m_dwBytesMoved),
			&(RecvHolder.m_dwFlags), &RecvHolder.m_wsaOverlapped, NULL);

		//NOTE: Further testing required to determine if errors that shouldn't shut
		//down the server can here.
		if (SOCKET_ERROR == iResult)
		{
			iResult = WSAGetLastError();
			if (WSA_IO_PENDING != iResult)
			{
				DEBUG_WSAERROR("WSARecv failed");
				ServerShutDown(pServerArgs, pUsers);
				return SRV_SHUTDOWN_ERR;
			}
		}
	}

	DEBUG_PRINT("Server Shutting Down.");
	ServerShutDown(pServerArgs, pUsers);
	return S_OK;
}

//End of file
