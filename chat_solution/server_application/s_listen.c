/*****************************************************************//**
 * \file   s_listen.c
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include "pch.h"
#include "s_listen.h"

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
		PrintError((PCHAR)__func__, __LINE__);
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
		PrintErrorCustom((PCHAR)__func__, __LINE__, "CreatePrintMutexes()");
		return SRV_SHUTDOWN_ERR;
	}
	
	if (EXIT_FAILURE == NetSetUp())
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NetSetUp()");
		return SRV_SHUTDOWN_ERR;
	}

	pServerArgs->m_ListenSocket = NetListen(pServerArgs->m_pszBindIP,
		pServerArgs->m_pszBindPort);

	if (INVALID_SOCKET == pServerArgs->m_ListenSocket)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NetListen()");
		return SRV_SHUTDOWN_ERR;
	}

	//NOTE: The listening socket is now set up, let's create IOCP handle.
	pServerArgs->m_haSharedHandles[IOCP_HANDLE] = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE, NULL, NO_OPTION, pServerArgs->m_dwThreadCount);

	if (NULL == pServerArgs->m_haSharedHandles[IOCP_HANDLE])
	{
		PrintError((PCHAR)__func__, __LINE__);
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
		PrintError((PCHAR)__func__, __LINE__);
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
			PrintError((PCHAR)__func__, __LINE__);\

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
					PrintError((PCHAR)__func__, __LINE__);
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
				PrintErrorCustom((PCHAR)__func__, __LINE__,
					"WaitForMultipleObjects()");
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
			ThreadPrintError(pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__);
			bErrorOccured = TRUE;
			//NOTE: no error return here as all the threads need to close
			//either way.
		}
	}

	//TODO: Determine if any errors will stop function from working.

	DWORD dwResult = WaitForMultipleObjects(pServerArgs->m_dwThreadCount,
		pServerArgs->m_phThreads, TRUE, INFINITE);

	//NOTE: Waiting for threads to close.
	switch (dwResult)
	{
	case WAIT_OBJECT_0:
		break;

	//NOTE: Any option except WAIT_OBJECT_0 means failure to close all threads.
	default:
		ThreadPrintError(pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__);
		bErrorOccured = TRUE;
		break;
	}

	MyHeapFree(GetProcessHeap(), NO_OPTION, pServerArgs->m_phThreads,
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
		PrintError((PCHAR)__func__, __LINE__);
		return NULL;
	}

	pUsers->m_dwMaxClients = pServerArgs->m_dwMaxClients;
	//WARNING: Max clients set to 65535, so conversion to WORD type for entry
	// into the following function does not result in any data loss.
	pUsers->m_pUsersHTable = HashTableInit((WORD)pServerArgs->m_dwMaxClients, NULL);

	if (NULL == pUsers->m_pUsersHTable)
	{
		PrintError((PCHAR)__func__, __LINE__);
		MyHeapFree(GetProcessHeap(), NO_OPTION, pUsers, sizeof(USERS));
		return NULL;
	}


	pUsers->m_pNewUsersTable = HashTableInit((WORD)pServerArgs->m_dwMaxClients, NULL);

	if (NULL == pUsers->m_pNewUsersTable)
	{
		PrintError((PCHAR)__func__, __LINE__);
		MyHeapFree(GetProcessHeap(), NO_OPTION, pUsers, sizeof(USERS));
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
		PrintError((PCHAR)__func__, __LINE__);
		MyHeapFree(GetProcessHeap(), NO_OPTION, pUsers, sizeof(USERS));
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
		ThreadPrintError(pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__);
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
		ThreadPrintError(pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__);

		if (WIN_EXIT_FAILURE == MyHeapFree(GetProcessHeap(), NO_OPTION, pUser,
			sizeof(USER)))
		{
			ThreadPrintErrorCustom(
				pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "MyHeapFree()");
		}

		return NULL;
	}

	pUser->m_SendMsgQueue = QueueInit();

	if (NULL == pUser->m_SendMsgQueue)
	{
		ThreadPrintError(pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__);

		if (WIN_EXIT_FAILURE == MyHeapFree(GetProcessHeap(), NO_OPTION, pUser,
			sizeof(USER)))
		{
			ThreadPrintErrorCustom(
				pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "MyHeapFree()");
		}

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
		PrintErrorCustom((PCHAR)__func__, __LINE__, "ThreadShutDown()");
	}

	if (FALSE == CloseHandle(pServerArgs->m_haSharedHandles[IOCP_HANDLE]))
	{
		PrintError((PCHAR)__func__, __LINE__);
	}

	if (EXIT_FAILURE == HashTableDestroy(pUsers->m_pUsersHTable,
		UserFreeFunction))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "HashTableDestroy()");
	}

	if (EXIT_FAILURE == HashTableDestroy(pUsers->m_pNewUsersTable,
		UserFreeFunction))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "HashTableDestroy()");
	}

	NetCleanup(pServerArgs->m_ListenSocket, INVALID_SOCKET);

	//NOTE: All server processes have now been shutdown, now let's free the
	// memory.
	if (WIN_EXIT_FAILURE == MyHeapFree(GetProcessHeap(), NO_OPTION, pUsers,
		sizeof(USERS)))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "MyHeapFree()");
	}

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
		ThreadPrintErrorCustom(
			pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__, "CreateUsers()");
		ServerShutDown(pServerArgs, pUsers);
		return SRV_SHUTDOWN_ERR;
	}
	
	while(CONTINUE == g_bServerState)
	{
		SOCKET ClientSocket = NetAccept(pServerArgs->m_ListenSocket);

		//NOTE: Errors that are not fatal are handled within NetAccept.
		if (INVALID_SOCKET == ClientSocket)
		{
			//NOTE: The NetAccept fn can return INVALID_SOCKET if the server is
			//shut down on purpose, if it should still be running, we want the
			//diagnotic info printed.
			ThreadPrintErrorWSA(
				pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__);
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
			ThreadPrintErrorCustom(
				pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "CreateUser()");
			ServerShutDown(pServerArgs, pUsers);
			return SRV_SHUTDOWN_ERR;
		}

		//NOTE: Client socket utilized for now as it will always be unique.
		DWORD dwWaitResult = WaitForSingleObject(
			pUsers->m_haUsersHandles[NEW_USERS_MUTEX], INFINITE);

		if (WAIT_OBJECT_0 != dwWaitResult)
		{
			ThreadPrintErrorCustom(
				pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "WaitForSingleObject()");
			ReleaseMutex(pUsers->m_haUsersHandles[NEW_USERS_MUTEX]);
			ServerShutDown(pServerArgs, pUsers);
			return SRV_SHUTDOWN_ERR;
		}

		WORD wResult = HashTableNewEntry(pUsers->m_pNewUsersTable, pUser,
			(PWCHAR)&(pUser->m_ClientSocket),
			(sizeof(SOCKET) / sizeof(WCHAR)));
		ReleaseMutex(pUsers->m_haUsersHandles[NEW_USERS_MUTEX]);
		if (EXIT_FAILURE == wResult)
		{
			ThreadPrintErrorCustom(
				pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "HashTableNewEntry()");
			UserFreeFunction((PVOID)pUser);
			ServerShutDown(pServerArgs, pUsers);
			return SRV_SHUTDOWN_ERR;
		}

		HANDLE hResult = CreateIoCompletionPort((HANDLE)pUser->m_ClientSocket,
			pServerArgs->m_haSharedHandles[IOCP_HANDLE], (ULONG_PTR)pUser,
			NO_OPTION);

		if (NULL == hResult)
		{
			ThreadPrintErrorCustom(
				pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "CreateIoCompletionPort()");
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
				ThreadPrintErrorWSA(pServerArgs->m_haSharedHandles[STD_ERR_MUTEX],
					(PCHAR)__func__, __LINE__);
				ServerShutDown(pServerArgs, pUsers);
				return SRV_SHUTDOWN_ERR;
			}
		}
	}
	
	ThreadCustomConsoleWrite(pServerArgs->m_haSharedHandles[STD_OUT_MUTEX],
		L"Server Shutting Down.\n", 23);
	ServerShutDown(pServerArgs, pUsers);
	return S_OK;
}

//End of file
