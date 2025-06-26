/*****************************************************************//**
 * \file   s_worker.c
 * \brief
 *
 * \author chris
 * \date   September 2024
 *********************************************************************/
#include <Windows.h>
#include <stdio.h>
#include <wchar.h>
#include <strsafe.h>

#include "s_worker.h"
#include "s_shared.h"
#include "s_message.h"
#include "s_main.h"

extern volatile BOOL g_bServerState;

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
static HRESULT
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

//NOTE: Calling function will need to release users mutex.
//NOTE: on success, S_OK returned. on failure, SRV_SHUTDOWN_ERR returned -
//if the mutex cannot be locked with an infinite time limit, a fatal error
//has occured.
static HRESULT
UsersTableWriter(PUSER pUser)
{
	HANDLE pUserWriteMutex =
		pUser->m_pUsers->m_haUsersHandles[USERS_WRITE_MUTEX];

	//NOTE: Use same logic cycle as register to access users hash table.
	DWORD dwWaitResult = CustomWaitForSingleObject(pUserWriteMutex, INFINITE);

	if (WAIT_OBJECT_0 != dwWaitResult)
	{
		DEBUG_ERROR("CustomWaitForSingleObject failed");
		return SRV_SHUTDOWN_ERR;
	}

	//NOTE: This comparison means that at least one reader is currently using
	// the semaphore.
	if (0 != pUser->m_pUsers->m_plReaderCount)
	{
		//NOTE: Waiting for a reader to signal that no more readers are using
		// the semaphore.
		dwWaitResult = CustomWaitForSingleObject(
			pUser->m_pUsers->m_haUsersHandles[READERS_DONE_EVENT], INFINITE);

		if (WAIT_OBJECT_0 != dwWaitResult)
		{
			ReleaseMutex(pUserWriteMutex);
			DEBUG_ERROR("CustomWaitForSingleObject failed");
			return SRV_SHUTDOWN_ERR;
		}
	}

	return S_OK;
}

//NOTE: Called when
static HRESULT
WorkerWSARecv(PUSER pUser)
{
	INT iResult = WSARecv(pUser->m_ClientSocket, pUser->m_RecvMsg.m_wsaBuffer,
		THREE_BUFFERS, &(pUser->m_RecvMsg.m_dwBytesMoved), &(pUser->m_RecvMsg.m_dwFlags),
		&(pUser->m_RecvMsg.m_wsaOverlapped), NULL);

	if (SOCKET_ERROR == iResult)
	{
		iResult = WSAGetLastError();
		if (WSA_IO_PENDING != iResult)
		{
			DEBUG_ERROR("WSARecv failed");
			g_bServerState = STOP;
			return CLIENT_REMOVE_ERR;
		}
	}

	return S_OK;
}

//WARNING: Assumes packet header length = 7 and contains correct lengths of
// wide char strings one and two. This does not mean that
static HRESULT
WorkerPartialRecv(PUSER pUser)
{
	//NOTE: WorkerThread() established that BytesSent < BytestoSend.
	PCHATMSG pChatMsg =
		(PCHATMSG)(pUser->m_RecvMsg.m_wsaBuffer[HEADER_INDEX].buf);
	if (pUser->m_RecvMsg.m_dwBytesMovedTotal < HEADER_LEN)
	{
		pUser->m_RecvMsg.m_wsaBuffer[HEADER_INDEX].buf =
			(PCHAR)&(pUser->m_RecvMsg.m_Header) +
			pUser->m_RecvMsg.m_dwBytesMovedTotal;
		pUser->m_RecvMsg.m_wsaBuffer[HEADER_INDEX].len = HEADER_LEN -
			pUser->m_RecvMsg.m_dwBytesMovedTotal;

		return WorkerWSARecv(pUser);
	}

	//NOTE: This is how the TYPE/STYPE/OPCODE will be analyzed as well.
	//WARNING: Don't access the data fields here just yet.
	WORD wLenOne = ntohs(pChatMsg->wLenOne);
	WORD wLenTwo = ntohs(pChatMsg->wLenTwo);
	WORD wBytesOne = wLenOne * sizeof(WCHAR);
	WORD wBytesTwo = wLenTwo * sizeof(WCHAR);

	if (0 == (wBytesOne + wBytesTwo))
	{
		//NOTE: No more data to grab!
		return HEADER_SIZE_PACKET;
	}

	//NOTE: The initial recv only receives the header
	if (HEADER_LEN == pUser->m_RecvMsg.m_dwBytestoMove)
	{
		pUser->m_RecvMsg.m_dwBytestoMove = HEADER_LEN + wBytesOne + wBytesTwo;
		pUser->m_RecvMsg.m_wsaBuffer[HEADER_INDEX].len = 0;
		pUser->m_RecvMsg.m_wsaBuffer[BODY_INDEX_1].len = wBytesOne;
		pUser->m_RecvMsg.m_wsaBuffer[BODY_INDEX_2].len = wBytesTwo;
		//NOTE: Need to return here where packet is actioned.
	}
	else if (pUser->m_RecvMsg.m_dwBytesMovedTotal <
		((DWORD)HEADER_LEN + wBytesOne))
	{
		DWORD dwDataOneBytesRecved = pUser->m_RecvMsg.m_dwBytesMovedTotal -
			HEADER_LEN;
		pUser->m_RecvMsg.m_wsaBuffer[HEADER_INDEX].len = 0;
		pUser->m_RecvMsg.m_wsaBuffer[BODY_INDEX_1].buf =
			(PCHAR)pUser->m_RecvMsg.m_pBodyBufferOne + dwDataOneBytesRecved;
		pUser->m_RecvMsg.m_wsaBuffer[BODY_INDEX_1].len = wBytesOne -
			dwDataOneBytesRecved;
	}
	else
	{
		DWORD dwDataTwoBytesRecved = pUser->m_RecvMsg.m_dwBytesMovedTotal -
			(wBytesOne + HEADER_LEN);
		pUser->m_RecvMsg.m_wsaBuffer[HEADER_INDEX].len = 0;
		pUser->m_RecvMsg.m_wsaBuffer[BODY_INDEX_1].len = 0;
		pUser->m_RecvMsg.m_wsaBuffer[BODY_INDEX_2].buf =
			(PCHAR)pUser->m_RecvMsg.m_pBodyBufferTwo + dwDataTwoBytesRecved;
		pUser->m_RecvMsg.m_wsaBuffer[BODY_INDEX_2].len = wBytesTwo -
			dwDataTwoBytesRecved;
	}

	return WorkerWSARecv(pUser);
}

//WARNING: Assumes packet header length = 7 and contains correct lengths of
// wide char strings one and two.
static HRESULT
WorkerPartialSend(PMSGHOLDER pMsgHolder, SOCKET ClientSocket)
{
	//NOTE: WorkerThread() established that BytesSent < BytestoSend.
	CHATMSG pChatMsg = pMsgHolder->m_Header;
	WORD wLenOne = ntohs(pChatMsg.wLenOne);
	WORD wLenTwo = ntohs(pChatMsg.wLenTwo);
	WORD wBytesOne = wLenOne * sizeof(WCHAR);
	WORD wBytesTwo = wLenTwo * sizeof(WCHAR);

	if (pMsgHolder->m_dwBytesMovedTotal < HEADER_LEN)
	{
		pMsgHolder->m_wsaBuffer[HEADER_INDEX].buf =
			(PCHAR)&(pMsgHolder->m_Header) + pMsgHolder->m_dwBytesMovedTotal;
		pMsgHolder->m_wsaBuffer[HEADER_INDEX].len = HEADER_LEN -
			pMsgHolder->m_dwBytesMovedTotal;
	}
	else if (pMsgHolder->m_dwBytesMovedTotal < ((DWORD)HEADER_LEN + wBytesOne))
	{
		DWORD dwDataOneBytesSent = pMsgHolder->m_dwBytesMovedTotal -
			HEADER_LEN;
		pMsgHolder->m_wsaBuffer[HEADER_INDEX].len = 0;
		pMsgHolder->m_wsaBuffer[BODY_INDEX_1].buf =
			(PCHAR)pMsgHolder->m_pBodyBufferOne + dwDataOneBytesSent;
		pMsgHolder->m_wsaBuffer[BODY_INDEX_1].len = wLenOne -
			dwDataOneBytesSent;
	}
	else
	{
		DWORD dwDataTwoBytesSent = pMsgHolder->m_dwBytesMovedTotal -
			(wBytesOne + HEADER_LEN);
		pMsgHolder->m_wsaBuffer[HEADER_INDEX].len = 0;
		pMsgHolder->m_wsaBuffer[BODY_INDEX_1].len = 0;
		pMsgHolder->m_wsaBuffer[BODY_INDEX_2].buf =
			(PCHAR)pMsgHolder->m_pBodyBufferTwo + dwDataTwoBytesSent;
		pMsgHolder->m_wsaBuffer[BODY_INDEX_2].len = wBytesTwo -
			dwDataTwoBytesSent;
	}

	INT iResult = WSASend(ClientSocket, pMsgHolder->m_wsaBuffer,
		THREE_BUFFERS, &pMsgHolder->m_dwBytesMoved, pMsgHolder->m_dwFlags,
		&pMsgHolder->m_wsaOverlapped, NULL);

	if (SOCKET_ERROR == iResult)
	{
		iResult = WSAGetLastError();
		if (WSA_IO_PENDING != iResult)
		{
			DEBUG_WSAERROR("WSASend failed");
			g_bServerState = STOP;
			return CLIENT_REMOVE_ERR;
		}
	}

	return S_OK;
}

static HRESULT
CheckforUser(PUSER pUser, PCHATMSG pChatMsg)
{
	//NOTE: The server has reached max capacity.
	if (pUser->m_pUsers->m_pUsersHTable->m_wSize >=
		pUser->m_pUsers->m_dwMaxClients)
	{
		ReleaseMutex(pUser->m_pUsers->m_haUsersHandles[USERS_WRITE_MUTEX]);
		return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_SRV_FULL, 0, 0, NULL, NULL);
	}

	WORD wResult = HashTableNewEntry(pUser->m_pUsers->m_pUsersHTable, pUser,
                          (PCHAR)pChatMsg->pszDataOne, pChatMsg->wLenOne);
	ReleaseMutex(pUser->m_pUsers->m_haUsersHandles[USERS_WRITE_MUTEX]);

	if (SUCCESS != wResult)
	{
		DEBUG_ERROR("HashTableNewEntry failed");
		return SRV_SHUTDOWN_ERR;
	}

	if (DUPLICATE_KEY == wResult)
	{
		//NOTE: User is already present.
		return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_USER_LOGGED, 0, 0, NULL, NULL);
	}

	pUser->m_wNegotiatedState = NEGOTIATED;
	wcscpy_s(pUser->m_caUsername, (pChatMsg->wLenOne + 1),
		pChatMsg->pszDataOne);
	pUser->m_wUsernameLen = pChatMsg->wLenOne;

	DWORD dwWaitResult = CustomWaitForSingleObject(
		pUser->m_pUsers->m_haUsersHandles[NEW_USERS_MUTEX], INFINITE);

	if (WAIT_OBJECT_0 != dwWaitResult)
	{
		DEBUG_ERROR("CustomWaitForSingleObject failed");
		return SRV_SHUTDOWN_ERR;
	}

	HashTableDestroyEntry(pUser->m_pUsers->m_pNewUsersTable,
		(PCHAR)&(pUser->m_ClientSocket), (sizeof(SOCKET) / sizeof(WCHAR)));
	ReleaseMutex(pUser->m_pUsers->m_haUsersHandles[NEW_USERS_MUTEX]);

	//Successful login.
	return ManageMsgQueueAdd(pUser, TYPE_ACCOUNT, STYPE_LOGIN,
		OPCODE_ACK, 0, 0, NULL, NULL);
}

static HRESULT
LoginBroadcast(PUSER pSendingUser, WORD wMsgLen, PWCHAR pszMsg)
{
	PHASHTABLE pUsersTable = pSendingUser->m_pUsers->m_pUsersHTable;
	for (WORD wCounter = 0; wCounter < pUsersTable->m_wCapacity;
		wCounter++)
	{
		PLINKEDLIST pLinkedList = pUsersTable->m_ppTable[wCounter];

		if (NULL != pLinkedList)
		{
			PLINKEDLISTNODE pTempNode = pLinkedList->m_pTail;

			for (WORD wCounter2 = 0; wCounter2 < pLinkedList->m_wSize;
				wCounter2++)
			{
				pTempNode = pTempNode->m_pNext;
				PHASHTABLEENTRY pTempEntry =
					(PHASHTABLEENTRY)pTempNode->m_pData;
				PUSER pUser = (PUSER)pTempEntry->m_pData;
				if (pUser != pSendingUser)
				{
					HRESULT hResult = ManageMsgQueueAdd(pUser, TYPE_CHAT,
						STYPE_EMPTY, OPCODE_RES,
						pSendingUser->m_wUsernameLen, wMsgLen,
						pSendingUser->m_caUsername, pszMsg);

					if (S_OK != hResult)
					{
						//NOTE: Error information will be printed, but the other
						// client's IOCP packet can handle the failure.
						DEBUG_ERROR("ManageMsgQueueAdd failed");
						return hResult;
					}
				}
			}
		}
	}

	return S_OK;
}

//NOTE: See README for logic explanation.
static HRESULT
HandleClientRegistration(PUSER pUser, PCHATMSG pChatMsg)
{
	if ((TYPE_ACCOUNT != pChatMsg->iType) ||
		(STYPE_LOGIN != pChatMsg->iSubType) ||
		(OPCODE_REQ != pChatMsg->iOpcode))
	{
		//NOTE: Invalid packet.
		return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_INVALID_PACKET, 0, 0, NULL, NULL);
	}

	if (pChatMsg->wLenOne > MAX_UNAME_LEN)
	{
		//NOTE: username too long.
		return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_UNAME_LEN, 0, 0, NULL, NULL);
	}

	//NOTE: Handles writer mutex lock logic.
	HRESULT hResult = UsersTableWriter(pUser);
	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableWriter failed");
		return hResult;
	}

	//NOTE: Write mutex released inside of CheckforUser fn. This ensures mutex
	// unlock prior to message send.
	hResult = CheckforUser(pUser, pChatMsg);
	if (S_OK != hResult)
	{
		DEBUG_ERROR("CheckforUser failed");
		return hResult;
	}

	//NOTE: Broadcasting that the user has logged in.
	return LoginBroadcast(pUser, 19, L"User has logged in.");
}

static HRESULT
SendOtherClientMessage(PUSER pUser, PUSER pTargetUser, PCHATMSG pChatMsg)
{
	HRESULT hResult = ManageMsgQueueAdd(pTargetUser, TYPE_CHAT, STYPE_EMPTY,
		OPCODE_RES, pUser->m_wUsernameLen, pChatMsg->wLenTwo,
		pUser->m_caUsername, pChatMsg->pszDataTwo);

	if (S_OK != hResult)
	{
		DEBUG_ERROR("CustomWaitForSingleObject failed");
		return hResult;
	}

	return ManageMsgQueueAdd(pUser, TYPE_CHAT, STYPE_EMPTY,
		OPCODE_ACK, 0, 0, NULL, NULL);
}

static HRESULT
UsersTableReaderStart(PUSERS pUsers)
{
	DWORD dwWaitResult = CustomWaitForSingleObject(
		pUsers->m_haUsersHandles[USERS_WRITE_MUTEX], INFINITE);

	if (WAIT_OBJECT_0 != dwWaitResult)
	{
		DEBUG_ERROR("CustomWaitForSingleObject failed");
		return SRV_SHUTDOWN_ERR;
	}

	dwWaitResult = CustomWaitForSingleObject(
		pUsers->m_haUsersHandles[USERS_READ_SEMAPHORE], INFINITE);

	if (WAIT_OBJECT_0 != dwWaitResult)
	{
		ReleaseMutex(pUsers->m_haUsersHandles[USERS_WRITE_MUTEX]);
		DEBUG_ERROR("CustomWaitForSingleObject failed");
		return SRV_SHUTDOWN_ERR;
	}

	InterlockedIncrement(&pUsers->m_plReaderCount);

	if (FALSE == ResetEvent(
		pUsers->m_haUsersHandles[READERS_DONE_EVENT]))
	{
		ReleaseMutex(pUsers->m_haUsersHandles[USERS_WRITE_MUTEX]);
		ReleaseSemaphore(
			pUsers->m_haUsersHandles[USERS_READ_SEMAPHORE], 1, NULL);
		DEBUG_ERROR("ResetEvent failed");
		return SRV_SHUTDOWN_ERR;
	}

	ReleaseMutex(pUsers->m_haUsersHandles[USERS_WRITE_MUTEX]);

	return S_OK;
}

static HRESULT
UsersTableReaderFinish(PUSERS pUsers)
{
	//NOTE: If this reader is the last, decrement and send the signal.
	ReleaseSemaphore(
		pUsers->m_haUsersHandles[USERS_READ_SEMAPHORE], 1, NULL);
	if (1 >= InterlockedCompareExchange(&pUsers->m_plReaderCount,
		0, 1))
	{
		if (FALSE == SetEvent(
			pUsers->m_haUsersHandles[READERS_DONE_EVENT]))
		{
			DEBUG_ERROR("SetEvent failed");
			return SRV_SHUTDOWN_ERR;
		}
	}
	else
	{
		InterlockedDecrement(&pUsers->m_plReaderCount);
	}

	return S_OK;
}

//TODO: Move this fn and helper to s_message.c
//NOTE: handle message to separate user and message rej/ack here.
//NOTE: See README for logic explanation.
static HRESULT
HandleClientMessage(PUSER pUser, PCHATMSG pChatMsg)
{
	if ((TYPE_CHAT != pChatMsg->iType) ||
		(STYPE_EMPTY != pChatMsg->iSubType) ||
		(OPCODE_REQ != pChatMsg->iOpcode))
	{
		//NOTE: Invalid packet.
		return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_INVALID_PACKET, 0, 0, NULL, NULL);
	}

	if (pChatMsg->wLenOne > MAX_UNAME_LEN)
	{
		//NOTE: username too long.
		return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_UNAME_LEN, 0, 0, NULL, NULL);
	}

	if (pChatMsg->wLenTwo > MAX_MSG_LEN_CHAT)
	{
		//NOTE: message too long.
		return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_MSG_LEN, 0, 0, NULL, NULL);
	}

	//NOTE: Performs logic that allows reader to access users hash table.
	// USer required to call UsersTableReaderFinish() to release semaphore and
	// decrement m_plReaderCount.
	HRESULT hResult = UsersTableReaderStart(pUser->m_pUsers);
	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableReaderStart failed");
		return hResult;
	}

	PUSER pTargetUser = HashTableReturnEntry(pUser->m_pUsers->m_pUsersHTable,
                             (PCHAR)pChatMsg->pszDataOne, pChatMsg->wLenOne);

	if (NULL == pTargetUser)
	{
		//NOTE: User does not exist.
		hResult = ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_USER_NOT_EXIST, 0, 0, NULL, NULL);
	}
	else
	{
		hResult = SendOtherClientMessage(pUser, pTargetUser, pChatMsg);
	}

	if (S_OK != hResult)
	{
		DEBUG_ERROR("SendOtherClientMessage failed");

		if (S_OK != UsersTableReaderFinish(pUser->m_pUsers))
		{
			DEBUG_ERROR("UsersTableReaderFinish failed");
			return SRV_SHUTDOWN_ERR;
		}

		return hResult;
	}

	hResult = UsersTableReaderFinish(pUser->m_pUsers);

	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableReaderFinish failed");
	}

	return hResult;
}

//NOTE: Utilized following a logout.
static HRESULT
LogoutBroadcast(PUSERS pUsers, WORD wUserlen, PWCHAR pszUsername, WORD wMsgLen,
	PWCHAR pszMsg)
{
	HRESULT hResult = UsersTableReaderStart(pUsers);
	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableReaderStart failed");
		return hResult;
	}

	PHASHTABLE pUsersTable = pUsers->m_pUsersHTable;
	for (WORD wCounter = 0; wCounter < pUsersTable->m_wCapacity;
		wCounter++)
	{
		PLINKEDLIST pLinkedList = pUsersTable->m_ppTable[wCounter];

		if (NULL != pLinkedList)
		{
			PLINKEDLISTNODE pTempNode = pLinkedList->m_pTail;

			for (WORD wCounter2 = 0; wCounter2 < pLinkedList->m_wSize;
				wCounter2++)
			{
				pTempNode = pTempNode->m_pNext;
				PHASHTABLEENTRY pTempEntry =
					(PHASHTABLEENTRY)pTempNode->m_pData;
				PUSER pUser = (PUSER)pTempEntry->m_pData;
				hResult = ManageMsgQueueAdd(pUser, TYPE_CHAT,
					STYPE_EMPTY, OPCODE_RES, wUserlen, wMsgLen, pszUsername,
					pszMsg);

				if (SRV_SHUTDOWN_ERR == hResult)
				{
					//NOTE: Error information will be printed, but the other
					// client's IOCP packet can handle the failure.
					DEBUG_ERROR("ManageMsgQueueAdd failed");

					if (S_OK != UsersTableReaderFinish(pUser->m_pUsers))
					{
						DEBUG_ERROR("UsersTableReaderFinish failed");
					}

					return SRV_SHUTDOWN_ERR;
				}
			}
		}
	}

	hResult = UsersTableReaderFinish(pUsers);

	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableReaderFinish failed");
	}
	return hResult;
}

static HRESULT
HandleLogout(PUSER pUser)
{
	PUSERS pUsers = pUser->m_pUsers;
	WCHAR caUsername[MAX_UNAME_LEN + 1] = { 0 };
	wcscpy_s(caUsername, (MAX_UNAME_LEN + 1), pUser->m_caUsername);
	WORD wUserlen = pUser->m_wUsernameLen;
	//NOTE: Send user ack packet.
	HRESULT hResult = ManageMsgQueueAdd(pUser, TYPE_ACCOUNT, STYPE_LOGOUT,
		OPCODE_ACK, 0, 0, NULL, NULL);
	if (S_OK != hResult)
	{
		DEBUG_ERROR("ManageMsgQueueAdd failed");
		return hResult;
	}

	DWORD dwWaitResult = 0;
	//NOTE: Waiting for m_plSendOccuring to go to zero.
	if (pUser->m_plSendOccuring != 0)
	{
		dwWaitResult = CustomWaitForSingleObject(
			pUser->m_haSharedHandles[SEND_DONE_EVENT], INFINITE);

		if (WAIT_OBJECT_0 != dwWaitResult)
		{
			DEBUG_ERROR("CustomWaitForSingleObject failed");
			return SRV_SHUTDOWN_ERR;
		}
	}

	//NOTE: Handles writer mutex lock logic.
	hResult = UsersTableWriter(pUser);
	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableWriter failed");
		return hResult;
	}

	PUSER pTempUser = HashTableDestroyEntry(pUser->m_pUsers->m_pUsersHTable,
		(PCHAR)pUser->m_caUsername, pUser->m_wUsernameLen);
	ReleaseMutex(pUser->m_pUsers->m_haUsersHandles[USERS_WRITE_MUTEX]);

	if (NULL == pTempUser)
	{
		UserFreeFunction((PVOID)pUser);
		DEBUG_ERROR("HashTableDestroyEntry failed");
		return SRV_SHUTDOWN_ERR;
	}

	UserFreeFunction((PVOID)pTempUser);
	return LogoutBroadcast(pUsers, wUserlen, caUsername, 25,
		L"User has left the server");
}

//NOTE: Gets a list of all users in the hash table.
//NOTE: Calling function is responsible for freeing allocated space.
//TODO: May need adjustment if table gets to big (updaing a list instead of
// generating a new one each time.
static PWCHAR
CreateList(PHASHTABLE pUsersTable)
{
	//NOTE: Space allocated for:
	// NULL terminator + ((Max username len + newline) * (number of users) *
	// (size of WCHAR))
	PWCHAR pUserList = HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		(1 + ((MAX_UNAME_LEN + 1) * pUsersTable->m_wSize * sizeof(WCHAR))));

	if (NULL == pUserList)
	{
		DEBUG_ERROR("HeapAlloc failed");
		return NULL;
	}

	PWCHAR pUserListTracker = pUserList;
	rsize_t rsLengthLeft = (1 + ((MAX_UNAME_LEN + 1) * pUsersTable->m_wSize *
		sizeof(WCHAR)));
	for (WORD wCounter = 0; wCounter < pUsersTable->m_wCapacity;
		wCounter++)
	{
		PLINKEDLIST pLinkedList = pUsersTable->m_ppTable[wCounter];

		if (NULL != pLinkedList)
		{
			PLINKEDLISTNODE pTempNode = pLinkedList->m_pTail;

			for (WORD wCounter2 = 0; wCounter2 < pLinkedList->m_wSize;
				wCounter2++)
			{
				pTempNode = pTempNode->m_pNext;
				PHASHTABLEENTRY pTempEntry =
					(PHASHTABLEENTRY)pTempNode->m_pData;
				PUSER pUser = (PUSER)pTempEntry->m_pData;
				if (0 != wcscpy_s(pUserListTracker,
					rsLengthLeft,
					pUser->m_caUsername))
				{
					DEBUG_ERROR("wcscpy_s failed");
					ZeroingHeapFree(GetProcessHeap(), NO_OPTION, (PVOID)&pUserList,
						(1 + ((MAX_UNAME_LEN + 1) * pUsersTable->m_wSize *
							sizeof(WCHAR))));
					return NULL;
				}
				pUserListTracker += pUser->m_wUsernameLen;
				pUserListTracker[0] = L'\n';
				pUserListTracker += 1;
				rsLengthLeft -= (pUser->m_wUsernameLen + 1);
			}
		}
	}

	return pUserList;
}

static HRESULT
HandleList(PUSER pUser, PCHATMSG pChatMsg)
{
	if ((TYPE_LIST != pChatMsg->iType) ||
		(STYPE_EMPTY != pChatMsg->iSubType) ||
		(OPCODE_REQ != pChatMsg->iOpcode))
	{
		//NOTE: Invalid packet.
		return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_INVALID_PACKET, 0, 0, NULL, NULL);
	}

	//reader for hash table mutex
	HRESULT hResult = UsersTableReaderStart(pUser->m_pUsers);
	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableReaderStart failed");
		return hResult;
	}

	PWCHAR pUserList = CreateList(pUser->m_pUsers->m_pUsersHTable);

	if (NULL == pUserList)
	{
		DEBUG_ERROR("CreateList failed");

		if (S_OK != UsersTableReaderFinish(pUser->m_pUsers))
		{
			DEBUG_ERROR("UsersTableReaderFinish failed");
		}
		return SRV_SHUTDOWN_ERR;
	}

	SIZE_T stUserListLen = 0;
	if (FAILED(StringCchLengthW(pUserList, (1 + ((MAX_UNAME_LEN + 1) *
		pUser->m_pUsers->m_pUsersHTable->m_wSize * sizeof(WCHAR))),
		&stUserListLen)))
	{
		DEBUG_ERROR("StringCchLengthW failed");
		ZeroingHeapFree(GetProcessHeap(), NO_OPTION, (PVOID)&pUserList, (1 +
			((MAX_UNAME_LEN + 1) * pUser->m_pUsers->m_pUsersHTable->m_wSize *
				sizeof(WCHAR))));

		if (S_OK != UsersTableReaderFinish(pUser->m_pUsers))
		{
			DEBUG_ERROR("UsersTableReaderFinish failed");
		}
		return SRV_SHUTDOWN_ERR;
	}

//NOTE: Current limitation is that packet data lens are max 65535 chars in len,
// more design decisions would need to be made prior to increasing that size.
// This means that even though the set limit of users is 65535, it's possible
// that list will not list all of them.
#pragma warning(push)
#pragma warning(disable : 4244)
	hResult = ManageMsgQueueAdd(pUser, TYPE_LIST, STYPE_EMPTY, OPCODE_RES,
		stUserListLen, 0, pUserList, NULL);
#pragma warning(pop)
	ZeroingHeapFree(GetProcessHeap(), NO_OPTION, (PVOID)&pUserList, (1 +
		((MAX_UNAME_LEN + 1) * pUser->m_pUsers->m_pUsersHTable->m_wSize *
			sizeof(WCHAR))));

	if (S_OK != hResult)
	{
		DEBUG_ERROR("ManageMsgQueueAdd failed");

		if (S_OK != UsersTableReaderFinish(pUser->m_pUsers))
		{
			DEBUG_ERROR("UsersTableReaderFinish failed");
			return SRV_SHUTDOWN_ERR;
		}

		return hResult;
	}

	hResult = UsersTableReaderFinish(pUser->m_pUsers);

	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableReaderFinish failed");
	}

	return hResult;
}

static VOID
CreateBroadcast(PUSER pSendingUser, WORD wMsgLen, PWCHAR pszMsg)
{
	PHASHTABLE pUsersTable = pSendingUser->m_pUsers->m_pUsersHTable;
	for (WORD wCounter = 0; wCounter < pUsersTable->m_wCapacity;
		wCounter++)
	{
		PLINKEDLIST pLinkedList = pUsersTable->m_ppTable[wCounter];

		if (NULL != pLinkedList)
		{
			PLINKEDLISTNODE pTempNode = pLinkedList->m_pTail;

			for (WORD wCounter2 = 0; wCounter2 < pLinkedList->m_wSize;
				wCounter2++)
			{
				pTempNode = pTempNode->m_pNext;
				PHASHTABLEENTRY pTempEntry =
					(PHASHTABLEENTRY)pTempNode->m_pData;
				PUSER pUser = (PUSER)pTempEntry->m_pData;
				HRESULT hResult = ManageMsgQueueAdd(pUser, TYPE_CHAT,
					STYPE_EMPTY, OPCODE_RES,
					pSendingUser->m_wUsernameLen, wMsgLen,
					pSendingUser->m_caUsername, pszMsg);

				if (S_OK != hResult)
				{
					//NOTE: Error information will be printed, but the other
					// client's IOCP packet can handle the failure.
					DEBUG_ERROR("ManageMsgQueueAdd failed");
				}
			}
		}
	}
}

static HRESULT
HandleBroadcast(PUSER pUser, PCHATMSG pChatMsg)
{
	if ((TYPE_BROADCAST != pChatMsg->iType) ||
		(STYPE_EMPTY != pChatMsg->iSubType) ||
		(OPCODE_REQ != pChatMsg->iOpcode))
	{
		//NOTE: Invalid packet.
		return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_INVALID_PACKET, 0, 0, NULL, NULL);
	}

	//NOTE: Need to call readers finish.
	HRESULT hResult = UsersTableReaderStart(pUser->m_pUsers);
	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableReaderStart failed");
		return hResult;
	}

	CreateBroadcast(pUser, pChatMsg->wLenOne, pChatMsg->pszDataOne);

	hResult = UsersTableReaderFinish(pUser->m_pUsers);

	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableReaderFinish failed");
		return hResult;
	}

	return ManageMsgQueueAdd(pUser, TYPE_BROADCAST,
		STYPE_EMPTY, OPCODE_ACK, 0, 0, NULL, NULL);
}

static HRESULT
HandleClientPacket(PUSER pUser, PCHATMSG pChatMsg)
{
	switch (pChatMsg->iType)
	{
	case TYPE_ACCOUNT: //NOTE: Logout
		if ((pChatMsg->iSubType == STYPE_LOGOUT)
			&& (pChatMsg->iOpcode == OPCODE_REQ))
		{
			return HandleLogout(pUser);
		}
		else
		{
			return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
				REJECT_INVALID_PACKET, 0, 0, NULL, NULL);
		}

	case TYPE_CHAT:
		return HandleClientMessage(pUser, pChatMsg);

	case TYPE_LIST:
		return HandleList(pUser, pChatMsg);

	case TYPE_BROADCAST:
		return HandleBroadcast(pUser, pChatMsg);

	default:
		//NOTE: Sending failure packet if packet invalid.
		return ManageMsgQueueAdd(pUser, TYPE_FAILURE, STYPE_EMPTY,
			REJECT_INVALID_PACKET, 0, 0, NULL, NULL);
	}
}

static HRESULT
WorkerRecvOP(PUSER pUser, DWORD dwBytesTransferred)
{
	//NOTE: The socket just received bytes into the WSABuffer within
	//the completion key.
	HRESULT hResult = S_OK;
	pUser->m_RecvMsg.m_dwBytesMovedTotal += dwBytesTransferred;
	if ((pUser->m_RecvMsg.m_dwBytesMovedTotal == HEADER_LEN)
		|| (pUser->m_RecvMsg.m_dwBytesMovedTotal <
			pUser->m_RecvMsg.m_dwBytestoMove))
	{
		//NOTE: Partial recv. Test with just recv.
		hResult = WorkerPartialRecv(pUser);
		if (HEADER_SIZE_PACKET != hResult)
		{
			return hResult; //NOTE: This could be success or failure. Simply
							// means that the packet has data lens of 0.
		}
	}

	//NOTE: Get the message copied onto the stack for handling, reset receiver.
	CHATMSG ChatMsgCopy = { 0 };
	WCHAR pszDataOne[BUFF_SIZE + 1] = { 0 };
	WCHAR pszDataTwo[BUFF_SIZE + 1] = { 0 };
	memcpy_s((PVOID)&ChatMsgCopy, sizeof(CHATMSG),
		(PVOID)&pUser->m_RecvMsg.m_Header, sizeof(CHATMSG));
	ChatMsgCopy.wLenOne = ntohs(ChatMsgCopy.wLenOne);
	ChatMsgCopy.wLenTwo = ntohs(ChatMsgCopy.wLenTwo);
	ChatMsgCopy.pszDataOne = pszDataOne;
	ChatMsgCopy.pszDataTwo = pszDataTwo;

	if ((0 < ChatMsgCopy.wLenOne) && (ChatMsgCopy.wLenOne < MAX_MSG_LEN_CHAT))
	{
		errno_t eResult = wmemcpy_s(pszDataOne, (BUFF_SIZE + 1),
			pUser->m_RecvMsg.m_pBodyBufferOne, ChatMsgCopy.wLenOne);
		if (0 != eResult)
		{
            DEBUG_ERROR_SUPPLIED(eResult, "wmemcpy_s failed");
			return SRV_SHUTDOWN_ERR;
		}
		WstrNetToHost(pszDataOne, ChatMsgCopy.wLenOne);
		/*wprintf(L"string one:%s\n", pszDataOne);*/
	}

	if ((0 < ChatMsgCopy.wLenTwo) && (ChatMsgCopy.wLenTwo < MAX_MSG_LEN_CHAT))
	{
		errno_t eResult = wmemcpy_s(pszDataTwo, (BUFF_SIZE + 1),
			pUser->m_RecvMsg.m_pBodyBufferTwo, ChatMsgCopy.wLenTwo);
		if (0 != eResult)
		{
            DEBUG_ERROR_SUPPLIED(eResult, "wmemcpy_s failed");
			return SRV_SHUTDOWN_ERR;
		}
		WstrNetToHost(pszDataTwo, ChatMsgCopy.wLenTwo);
		/*wprintf(L"string two:%s\n", pszDataTwo);*/
	}

	ResetChatRecv(&pUser->m_RecvMsg);
	//wprintf(L"type: %d, stype: %d, opcode: %d, msg lens: %u, %u\n",
	//	ChatMsgCopy.iType, ChatMsgCopy.iSubType, ChatMsgCopy.iOpcode,
	//	ChatMsgCopy.wLenOne, ChatMsgCopy.wLenTwo);

	//NOTE: Updating volatile value so that other receiver can prepare. Only
	// updated if the packet is not a logout packet. In that case, we don't
	// want another receiver started.
	if (!((ChatMsgCopy.iType == TYPE_ACCOUNT) &&
		(ChatMsgCopy.iSubType == STYPE_LOGOUT) &&
		(ChatMsgCopy.iOpcode == OPCODE_REQ) &&
		(NEGOTIATED == pUser->m_wNegotiatedState)))
	{
		InterlockedDecrement(&pUser->m_plRecvOccuring);
	}

	if (UN_NEGOTIATED == pUser->m_wNegotiatedState)
	{
		hResult = HandleClientRegistration(pUser, &ChatMsgCopy);
		if (S_OK != hResult)
		{
			DEBUG_ERROR("CheckforUser failed");
			return hResult;
		}
	}
	else
	{
		hResult = HandleClientPacket(pUser, &ChatMsgCopy);
		if (S_OK != hResult)
		{
			DEBUG_ERROR("CheckforUser failed");
			return hResult;
		}
	}

	return S_OK;
}

static HRESULT
CheckSendQueue(PUSER pUser)
{
	PMSGHOLDER pMsgHolder = QueuePeek(pUser->m_SendMsgQueue);

	if (NULL == pMsgHolder)
	{
		DEBUG_ERROR("CustomWaitForSingleObject failed");
		return SRV_SHUTDOWN_ERR;
	}

	INT iResult = WSASend(pUser->m_ClientSocket, pMsgHolder->m_wsaBuffer,
		THREE_BUFFERS, &pMsgHolder->m_dwBytesMoved, pMsgHolder->m_dwFlags,
		&pMsgHolder->m_wsaOverlapped, NULL);

	if (SOCKET_ERROR == iResult)
	{
		iResult = WSAGetLastError();
		if (WSA_IO_PENDING != iResult)
		{
			DEBUG_WSAERROR("WSASend failed");
			return CLIENT_REMOVE_ERR;
		}
	}

	return S_OK;
}

static HRESULT
ManageSendQueue(PUSER pUser)
{
	InterlockedIncrement(&pUser->m_plThreadsWaiting);

	DWORD dwWaitResult = CustomWaitForSingleObject(
		pUser->m_haSharedHandles[SEND_MUTEX], INFINITE);
	InterlockedDecrement(&pUser->m_plThreadsWaiting);

	//NOTE: Maybe handle wait abandoned differently than wait failed in the
	// future.
	if (WAIT_OBJECT_0 != dwWaitResult)
	{
		DEBUG_ERROR("CustomWaitForSingleObject failed");
		return SRV_SHUTDOWN_ERR;
	}

	//NOTE: The full send was successful. Remove memory allocated for this send.
	if (SUCCESS != QueuePopRemove(pUser->m_SendMsgQueue, FreeMsg))
	{
		ReleaseMutex(pUser->m_haSharedHandles[SEND_MUTEX]);
		DEBUG_ERROR("QueuePopRemove failed");
		return SRV_SHUTDOWN_ERR;
	}

	if (0 == pUser->m_SendMsgQueue->m_iSize)
	{
		//NOTE: plSendOccuring only updated when sends are initiated: only
		// initiated by the recvOP fn.
		InterlockedDecrement(&pUser->m_plSendOccuring);
		BOOL bResult = SetEvent(pUser->m_haSharedHandles[SEND_DONE_EVENT]);
		ReleaseMutex(pUser->m_haSharedHandles[SEND_MUTEX]);
		if (FALSE == bResult)
		{
			DEBUG_ERROR("SetEvent failed");
			return SRV_SHUTDOWN_ERR;
		}
		return S_OK;
	}

	HRESULT hResult = CheckSendQueue(pUser);
	ReleaseMutex(pUser->m_haSharedHandles[SEND_MUTEX]);

	if (S_OK != hResult)
	{
		DEBUG_WSAERROR("WSASend failed");
		return hResult;
	}

	return S_OK;
}

//NOTE: The operation that just completed was a send operation. Now we need to
// check if the operation requires a partial send.
static HRESULT
WorkerSendOP(PUSER pUser, DWORD dwBytesTransferred)
{
	HRESULT hResult = S_OK;
	PMSGHOLDER pSendHolder = QueuePeek(pUser->m_SendMsgQueue);
	if (NULL == pSendHolder)
	{
		DEBUG_ERROR("QueuePeek failed");
		return SRV_SHUTDOWN_ERR;
	}

	pSendHolder->m_dwBytesMovedTotal += dwBytesTransferred;
	if (pSendHolder->m_dwBytesMovedTotal < pSendHolder->m_dwBytestoMove)
	{
		hResult = WorkerPartialSend(pSendHolder, pUser->m_ClientSocket);
		if (S_OK != hResult)
		{
			DEBUG_ERROR("WorkerPartialSend failed");
			return hResult;
		}

		return S_OK;
	}

	//NOTE: Send was completed, lets check queue for more sends.
	hResult = ManageSendQueue(pUser);
	if (S_OK != hResult)
	{
		DEBUG_ERROR("ManageSendQueue failed");

		//TODO: should we keep this?
		//NOTE: If the previous function failed and didn't decrement
		// m_plSendOccuring, it is done here.
		InterlockedExchange(&pUser->m_plSendOccuring, 0);

		return hResult;
	}

	//NOTE: Receiver already reset after recv operation. Checking if the value
	// of the volatile LONG was zero. If it was, the value is changed to 1 and
	// another WSARecv is started. If not, no operation is started.
	if (0 != InterlockedCompareExchange(&pUser->m_plRecvOccuring, 1, 0))
	{
		return S_OK;
	}

	INT iResult = WSARecv(pUser->m_ClientSocket, pUser->m_RecvMsg.m_wsaBuffer,
		THREE_BUFFERS, &(pUser->m_RecvMsg.m_dwBytesMoved),
		&(pUser->m_RecvMsg.m_dwFlags), &(pUser->m_RecvMsg.m_wsaOverlapped),
		NULL);

	if (SOCKET_ERROR == iResult)
	{
		iResult = WSAGetLastError();
		if (WSA_IO_PENDING != iResult)
		{
			DEBUG_WSAERROR("WSARecv failed");
			g_bServerState = STOP;
			return CLIENT_REMOVE_ERR;
		}
	}

	return S_OK;
}

static HRESULT
ClientShutdown(PUSER pUser)
{
	//NOTE: Handles writer mutex lock logic.
	HRESULT hResult = UsersTableWriter(pUser);
	if (S_OK != hResult)
	{
		DEBUG_ERROR("UsersTableWriter failed");
		return hResult;
	}

	//NOTE: Waiting for m_plSendOccuring to go to zero.
	if (pUser->m_plSendOccuring != 0)
	{
		DWORD dwWaitResult = CustomWaitForSingleObject(
			pUser->m_haSharedHandles[SEND_DONE_EVENT], INFINITE);

		if (WAIT_OBJECT_0 != dwWaitResult)
		{
			DEBUG_ERROR("CustomWaitForSingleObject failed");
			return SRV_SHUTDOWN_ERR;
		}
	}

	PUSER pTempUser = HashTableDestroyEntry(pUser->m_pUsers->m_pUsersHTable,
                                            (PCHAR)pUser->m_caUsername,
                                            pUser->m_wUsernameLen);
	ReleaseMutex(pUser->m_pUsers->m_haUsersHandles[USERS_WRITE_MUTEX]);

	if (NULL == pTempUser)
	{
		DEBUG_ERROR("HashTableDestroyEntry failed");
		UserFreeFunction((PVOID)pUser);
		return SRV_SHUTDOWN_ERR;
	}

	UserFreeFunction((PVOID)pTempUser);
	return S_OK;
}

static HRESULT
HandleClientShutdown(PUSER pUser, PMSGHOLDER pMsgHolder)
{
	//NOTE: Utilized for sending logout broadcast.
	PUSERS pUsers = pUser->m_pUsers;
	WCHAR caUsername[MAX_UNAME_LEN + 1] = { 0 };
	wcscpy_s(caUsername, (MAX_UNAME_LEN + 1), pUser->m_caUsername);
	WORD wUserlen = pUser->m_wUsernameLen;

	if (NEGOTIATED == pUser->m_wNegotiatedState)
	{
		//NOTE: Fatal client error, call for deletion.
		if (RECV_OP == pMsgHolder->m_iOperationType)
		{
			//NOTE: There will always be at least one receive that will come
			// through. Function waits for this IOCP return to conduct shutdown.
			if (NOT_DESTROYING == InterlockedCompareExchange(
				&pUser->m_plBeingDestroyed, DESTROYING, NOT_DESTROYING))
			{
				CustomConsoleWrite(L"WorkerThread(): Removing client due to: socket "
					"failure.\n", 76);
				//NOTE: The previous value was zero, so we'll commence shutdown
				//here.
				//NOTE: possible values: server shutdown, s_ok
				if (SRV_SHUTDOWN_ERR == ClientShutdown(pUser))
				{
					g_bServerState = STOP;
				}
				return LogoutBroadcast(pUsers, wUserlen, caUsername, 25,
					L"User has left the server");
			}
		}
		else
		{
			if (1 == InterlockedCompareExchange(&pUser->m_plSendOccuring, 0, 1))
			{
				//NOTE: If the send operation didn't transfer any bytes, we'll
				// decrement that send isn't occuring anymore.
				if (0 == SetEvent(
					pUser->m_haSharedHandles[SEND_DONE_EVENT]))
				{
					DEBUG_ERROR("SetEvent failed");
					return SRV_SHUTDOWN_ERR;
				}

				return S_OK;
			}
		}
	}
	else
	{
		//NOTE: This will be the only thread, user removed from new users hash
		// table.
		DWORD dwWaitResult = CustomWaitForSingleObject(
			pUser->m_pUsers->m_haUsersHandles[NEW_USERS_MUTEX], INFINITE);

		if (WAIT_OBJECT_0 != dwWaitResult)
		{
			DEBUG_ERROR("CustomWaitForSingleObject failed");
			return SRV_SHUTDOWN_ERR;
		}

		PUSER pTempUser = HashTableDestroyEntry(
			pUser->m_pUsers->m_pNewUsersTable,
			(PCHAR)&(pUser->m_ClientSocket),
			(sizeof(SOCKET) / sizeof(WCHAR)));
		ReleaseMutex(pUser->m_pUsers->m_haUsersHandles[NEW_USERS_MUTEX]);

		if (NULL == pTempUser)
		{
			DEBUG_ERROR("HashTableDestroyEntry failed");
			UserFreeFunction((PVOID)pUser);
			return SRV_SHUTDOWN_ERR;
		}

		UserFreeFunction((PVOID)pTempUser);
		return LogoutBroadcast(pUsers, wUserlen, caUsername, 25,
			L"User has left the server");
	}

	return S_OK;
}

DWORD
WorkerThread(PVOID pParam)
{
	//NOTE: Setting up and waiting for IOCP.
	HANDLE hIOCP = (HANDLE)pParam;
	DWORD dwBytesTransferred;
	ULONG_PTR pulUserHolder = 0;
	OVERLAPPED OverLapped = { 0 };
	LPOVERLAPPED lpOverLapped = &OverLapped;
	HRESULT hResult = S_OK;
	while (CONTINUE == g_bServerState)
	{
		dwBytesTransferred = 0;
		BOOL bResult = GetQueuedCompletionStatus(hIOCP, &dwBytesTransferred,
			&pulUserHolder, &lpOverLapped, INFINITE);
		if (IOCP_SHUTDOWN == pulUserHolder)
		{
			//NOTE: The server has issued shutdown packets to the IOCP handle.
			return SUCCESS;
		}

		PUSER pUser = (PUSER)pulUserHolder;
		//NOTE: lpOverLapped is the first member of our MSGHOLDER struct.
		PMSGHOLDER pMsgHolder = (PMSGHOLDER)lpOverLapped;

		if ((FALSE == bResult) || (0 == dwBytesTransferred))
		{
			if (SRV_SHUTDOWN_ERR == HandleClientShutdown(pUser, pMsgHolder))
			{
				//NOTE: Thread print dereference could cause errors.
				DEBUG_ERROR("GetQueuedCompletionStatus failed");
				return ERR_GENERIC;
			}
			continue;
		}

		//NOTE: RecvOP and SendOP contain most of server functionality.
		if (RECV_OP == pMsgHolder->m_iOperationType)
		{
			hResult = WorkerRecvOP(pUser, dwBytesTransferred);
		}
		else
		{
			hResult = WorkerSendOP(pUser, dwBytesTransferred);
		}

		//NOTE: Error handling for worker thread done here.
		switch (hResult)
		{
		case S_OK:
			break;

		case NON_FATAL_ERR:
			//NOTE: Error occured but does not effect run.
			break;

		case CLIENT_REMOVE_ERR:
			//NOTE: Error that requires client shutdown but not server shutdown.
			CustomConsoleWrite(L"WorkerThread(): Removing client due to: CLIENT_REMOVE_ERR",
				58);
			if (SRV_SHUTDOWN_ERR == HandleClientShutdown(pUser, pMsgHolder))
			{
				DEBUG_ERROR("HandleClientShutdown failed");
				return ERR_GENERIC;
			}
			break;

		case SRV_SHUTDOWN_ERR:
			//NOTE: Error that requires server shutdown.
			DEBUG_PRINT("WorkerThread(): Server shutting "
				"down");
			g_bServerState = STOP;
			break;

		default:
			DEBUG_PRINT("WorkerThread(): Unknown error, server shutting down");
			g_bServerState = STOP;
			break;
		}
	}
	return SUCCESS;
}

//End of file
