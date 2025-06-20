/*****************************************************************//**
 * \file   s_message.c
 * \brief
 *
 * \author chris
 * \date   September 2024
 *********************************************************************/

#include "s_message.h"
#include "../client_application/Queue.h"

extern volatile BOOL g_bServerState;

VOID
ResetChatRecv(PMSGHOLDER pMsgHolder)
{
	SecureZeroMemory(&pMsgHolder->m_Header, sizeof(CHATMSG));
	SecureZeroMemory(pMsgHolder->m_pBodyBufferOne, (BUFF_SIZE + 1));
	SecureZeroMemory(pMsgHolder->m_pBodyBufferTwo, (BUFF_SIZE + 1));
	pMsgHolder->m_wsaBuffer[HEADER_INDEX].buf = (PCHAR) & (pMsgHolder->m_Header);
	pMsgHolder->m_wsaBuffer[HEADER_INDEX].len = HEADER_LEN;
	pMsgHolder->m_wsaBuffer[BODY_INDEX_1].buf = (PCHAR)pMsgHolder->m_pBodyBufferOne;
	pMsgHolder->m_wsaBuffer[BODY_INDEX_1].len = 0;
	pMsgHolder->m_wsaBuffer[BODY_INDEX_2].buf = (PCHAR)pMsgHolder->m_pBodyBufferTwo;
	pMsgHolder->m_wsaBuffer[BODY_INDEX_2].len = 0;
	pMsgHolder->m_dwBytestoMove = HEADER_LEN;
	pMsgHolder->m_dwBytesMovedTotal = 0;
	pMsgHolder->m_dwBytesMoved = 0;
}

//WARNING: pUser print mutexes must already be created.
//NOTE: Pushes the message onto the queue and returns a pointer to the
// message.
static PMSGHOLDER
CreateMsg(PUSER pUser)
{
	PMSGHOLDER pMsgHolder = HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(MSGHOLDER));
	if (NULL == pMsgHolder)
	{
        DEBUG_ERROR("HeapAlloc()");
		return NULL;
	}

	WORD wResult = QueuePush(pUser->m_SendMsgQueue, pMsgHolder);
	if (SUCCESS != wResult)
    {
        DEBUG_PRINT("QueuePush()");
		return NULL;
	}

	return pMsgHolder;
}

PMSGHOLDER
AddMsgToQueue(PUSER pUser, INT8 iType, INT8 iSubType,
	INT8 iOpcode, WORD wLenOne, WORD wLenTwo, PWSTR pszDataOne,
	PWSTR pszDataTwo)
{
	PMSGHOLDER pMsgHolder = CreateMsg(pUser);
	if (NULL == pMsgHolder)
    {
        DEBUG_PRINT("CreateMsg()");
		return NULL;
	}

	//NOTE: Preparing packet header.
	pMsgHolder->m_Header.iType = iType;
	pMsgHolder->m_Header.iSubType = iSubType;
	pMsgHolder->m_Header.iOpcode = iOpcode;
	pMsgHolder->m_Header.wLenOne = htons(wLenOne);
	pMsgHolder->m_Header.wLenTwo = htons(wLenTwo);

	//NOTE: wcscpy_s requires inclusion of terminating NULL byte in length.
	if (0 < wLenOne)
	{
		errno_t eResult = wcscpy_s(pMsgHolder->m_pBodyBufferOne, (wLenOne + 1),
			pszDataOne);
		if (0 != eResult)
        {
            DEBUG_ERROR_SUPPLIED(eResult, "CreateMsg()");
			return NULL;
		}
	}
	if (0 < wLenTwo)
	{
		errno_t eResult = wcscpy_s(pMsgHolder->m_pBodyBufferTwo, (wLenTwo + 1),
			pszDataTwo);
		if (0 != eResult)
        {
            DEBUG_ERROR_SUPPLIED(eResult, "wcscpy_s()");
			return NULL;
		}
	}

	WstrHostToNet(pMsgHolder->m_pBodyBufferOne, wLenOne);
	WstrHostToNet(pMsgHolder->m_pBodyBufferTwo, wLenTwo);

	//NOTE: Preparing WSABuf struct.
	pMsgHolder->m_wsaBuffer[HEADER_INDEX].len = HEADER_LEN;
	pMsgHolder->m_wsaBuffer[HEADER_INDEX].buf = (PCHAR)&pMsgHolder->m_Header;
	pMsgHolder->m_wsaBuffer[BODY_INDEX_1].len = wLenOne * sizeof(WCHAR);
	pMsgHolder->m_wsaBuffer[BODY_INDEX_1].buf =
		(PCHAR)pMsgHolder->m_pBodyBufferOne;
	pMsgHolder->m_wsaBuffer[BODY_INDEX_2].len = wLenTwo * sizeof(WCHAR);
	pMsgHolder->m_wsaBuffer[BODY_INDEX_2].buf =
		(PCHAR)pMsgHolder->m_pBodyBufferTwo;
	pMsgHolder->m_dwBytestoMove = HEADER_LEN + ((wLenOne + wLenTwo) *
		sizeof(WCHAR));
	pMsgHolder->m_iOperationType = SEND_OP;

	return pMsgHolder;
}

HRESULT
ManageMsgQueueAdd(PUSER pUser, INT8 iType, INT8 iSubType,
	INT8 iOpcode, WORD wLenOne, WORD wLenTwo, PWSTR pszDataOne,
	PWSTR pszDataTwo)
{
	InterlockedIncrement(&pUser->m_plThreadsWaiting);

	DWORD dwWaitResult = WaitForSingleObject(
		pUser->m_haSharedHandles[SEND_MUTEX], INFINITE);
	InterlockedDecrement(&pUser->m_plThreadsWaiting);
	//NOTE: Maybe handle wait abandoned differently than wait failed in the
	// future.
	if (WAIT_OBJECT_0 != dwWaitResult)
	{
        DEBUG_ERROR("WaitForSingleObject()");
		return SRV_SHUTDOWN_ERR;
	}

	PMSGHOLDER pMsgHolder =
        AddMsgToQueue(pUser, iType, iSubType, iOpcode, wLenOne, wLenTwo,
                      pszDataOne, pszDataTwo);
	if (NULL == pMsgHolder)
    {
        DEBUG_PRINT("AddMsgToQueue()");
		return SRV_SHUTDOWN_ERR;
	}

	//NOTE: If the original value wasn't zero, the function will return success
	// - another function is handling the sending of the queue.
	if (0 != InterlockedCompareExchange(&pUser->m_plSendOccuring, 1, 0))
	{
		ReleaseMutex(pUser->m_haSharedHandles[SEND_MUTEX]);
		return S_OK;
	}

	BOOL bResult = ResetEvent(pUser->m_haSharedHandles[SEND_DONE_EVENT]);
	ReleaseMutex(pUser->m_haSharedHandles[SEND_MUTEX]);

	INT iResult = WSASend(pUser->m_ClientSocket, pMsgHolder->m_wsaBuffer,
		THREE_BUFFERS, &pMsgHolder->m_dwBytesMoved, pMsgHolder->m_dwFlags,
		&pMsgHolder->m_wsaOverlapped, NULL);

	if (SOCKET_ERROR == iResult)
	{
		iResult = WSAGetLastError();
		if (WSA_IO_PENDING != iResult)
		{
            DEBUG_ERROR_SUPPLIED(iResult, "WSAGetLastError");
			return NON_FATAL_ERR;
		}
	}

	return S_OK;
}

//End of file
