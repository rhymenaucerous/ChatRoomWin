/*****************************************************************//**
 * \file   c_messages.c
 * \brief  
 * 
 * \author chris
 * \date   September 2024
 *********************************************************************/
#include "pch.h"
#include "c_messages.h"

VOID
ThreadPrintFailurePacket(HANDLE hStdOut, INT8 wRejectCode)
{
	switch (wRejectCode)
	{
	case REJECT_SRV_BUSY:
		ThreadCustomConsoleWrite(hStdOut, L"Failure: Server Busy\n", 22);
		break;
	case REJECT_SRV_ERR:
		ThreadCustomConsoleWrite(hStdOut, L"Failure: Server Error\n", 23);
		break;
	case REJECT_INVALID_PACKET:
		ThreadCustomConsoleWrite(hStdOut, L"Failure: Invalid packet sent to "
			"server\n", 40);
		break;
	case REJECT_UNAME_LEN:
		ThreadCustomConsoleWrite(hStdOut, L"Failure: Username too long\n", 28);
		break;
	case REJECT_USER_LOGGED:
		ThreadCustomConsoleWrite(hStdOut, L"Failure: User already exists\n", 30);
		break;
	case REJECT_USER_NOT_EXIST:
		ThreadCustomConsoleWrite(hStdOut, L"Failure: User does not exist\n", 30);
		break;
	case REJECT_MSG_LEN:
		ThreadCustomConsoleWrite(hStdOut, L"Failure: Message too long\n", 27);
		break;
	case REJECT_SRV_FULL:
		ThreadCustomConsoleWrite(hStdOut, L"Failure: Server full\n", 22);
		break;
	default:
		ThreadCustomConsoleWrite(hStdOut, L"Failure: Unknown error packet "
			"received from server\n", 52);
		break;
	}
}

VOID
CustomStringPrintTwo(PWSTR pszStrOne, PWSTR pszStrTwo, WORD wLen1, WORD wLen2,
	HANDLE hStdErr, HANDLE hStdOut)
{
	//NOTE: Added len is the extra characters added in StringCchPrintfW().
	WORD wTotalLen = wLen1 + wLen2 + 5;

	PWSTR pszCustomString = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		(wTotalLen + 1) * sizeof(WCHAR));

	if (NULL == pszCustomString)
	{
		ThreadPrintError(hStdErr, (PCHAR)__func__, __LINE__);
		return;
	}

	HRESULT hReturn = S_OK;

	if ((0 == wLen1) && (0 == wLen2))
	{
		ThreadCustomConsoleWrite(hStdOut, L"Received NULL str", 18);
	}
	else if (0 == wLen1)
	{
		hReturn = StringCchPrintfW(pszCustomString, (wLen2 + 6), L"msg> %s\n",
			pszStrTwo);
	}
	else if (0 == wLen2)
	{
		hReturn = StringCchPrintfW(pszCustomString, (wLen1 + 6), L"msg> %s\n",
			pszStrOne);
	}
	else
	{
		hReturn = StringCchPrintfW(pszCustomString, wTotalLen, L"%s> %s\n",
			pszStrOne, pszStrTwo);
	}

	if (S_OK != hReturn)
	{
		ThreadPrintError(hStdErr, (PCHAR)__func__, __LINE__);
		MyHeapFree(GetProcessHeap(), NO_OPTION, pszCustomString,
			wTotalLen * sizeof(WCHAR));
		return;
	}

	//NOTE: We don't need to copy the NULL terminator to the console (-1).
	ThreadCustomConsoleWrite(hStdOut, pszCustomString, wTotalLen);
	MyHeapFree(GetProcessHeap(), NO_OPTION, pszCustomString,
		wTotalLen * sizeof(WCHAR));
}

//WARNING: Correct lengths of data one and two must be verified by SendPacket
//caller. Incorrect values could result in undefined behavior.
//NOTE: Lock socket mutex before sending.
//NOTE: We'll create a different function for the server that fits into
//the context of a worker thread using GetQueuedCompletionStatus().
HRESULT
SendPacket(HANDLE hStdErr, SOCKET RecvSock, INT8 iType, INT8 iSubType,
	INT8 iOpcode, WORD wLenOne, WORD wLenTwo, PWSTR pszDataOne,
	PWSTR pszDataTwo)
{
	if ((NULL == hStdErr) || (INVALID_SOCKET == RecvSock))
	{
		ThreadPrintError(hStdErr, (PCHAR)__func__, __LINE__);
		return E_HANDLE;
	}

	CHATMSG ChatMsg = { 0 };
	ChatMsg.iType = iType;
	ChatMsg.iSubType = iSubType;
	ChatMsg.iOpcode = iOpcode;
	ChatMsg.wLenOne = htons(wLenOne);
	ChatMsg.wLenTwo = htons(wLenTwo);

	WstrHostToNet(pszDataOne, wLenOne);
	WstrHostToNet(pszDataTwo, wLenTwo);

	DWORD dwBytesOne = wLenOne * sizeof(WCHAR);
	DWORD dwBytesTwo = wLenTwo * sizeof(WCHAR);
	DWORD dwPacketLen = HEADER_LEN + dwBytesOne + dwBytesTwo;

	//NOTE: wsaBuffer Initialization
	WSABUF wsaBuffer[THREE_BUFFERS] = { 0 };
	wsaBuffer[HEADER_INDEX].buf = (PCHAR)&ChatMsg;
	wsaBuffer[HEADER_INDEX].len = HEADER_LEN;
	wsaBuffer[BODY_INDEX_1].buf = (PCHAR)pszDataOne;
	wsaBuffer[BODY_INDEX_1].len = dwBytesOne;
	wsaBuffer[BODY_INDEX_2].buf = (PCHAR)pszDataTwo;
	wsaBuffer[BODY_INDEX_2].len = dwBytesTwo;

	DWORD dwBytesSent = 0;
	DWORD dwBytesSentTotal = 0;

	DWORD dwSentBytesOne = 0;
	DWORD dwRemainBytesOne = 0;
	DWORD dwSentBytesTwo = 0;
	DWORD dwRemainBytesTwo = 0;

	while (dwBytesSentTotal < dwPacketLen)
	{
		if (SOCKET_ERROR == WSASend(RecvSock, wsaBuffer, THREE_BUFFERS,
			&dwBytesSent, NO_OPTION, NULL, NULL))
		{
			ThreadPrintErrorWSA(hStdErr, (PCHAR)__func__, __LINE__);
			return E_UNEXPECTED;
		}

		dwBytesSentTotal += dwBytesSent;

		//NOTE: Updating WSABuf for partial sends.
		if (dwBytesSentTotal < dwPacketLen)
		{
			if (dwBytesSentTotal <= HEADER_LEN)
			{
				//NOTE: If the bytes sent is equal to HEADER_LEN, the new
				//length will be 0, and the buffer will point to the 
				//terminating 0.
				wsaBuffer[HEADER_INDEX].buf = (PCHAR)&ChatMsg + dwBytesSentTotal;
				wsaBuffer[HEADER_INDEX].len = HEADER_LEN - dwBytesSentTotal;
			}
			else if (dwBytesSentTotal <= (HEADER_LEN + dwBytesOne))
			{
				//NOTE: If the entire header is sent but not all of data1, the
				//buffer position and length of the first data buffer will be
				//updated.
				dwSentBytesOne = dwBytesSentTotal - HEADER_LEN;
				dwRemainBytesOne = dwBytesOne - dwSentBytesOne;

				wsaBuffer[HEADER_INDEX].buf = (PCHAR)&ChatMsg + HEADER_LEN;
				wsaBuffer[HEADER_INDEX].len = 0;

				//NOTE: Position of buffer updated by number of sent bytes,
				//length updated to remaining unsent bytes.
				wsaBuffer[BODY_INDEX_1].buf = (PCHAR)pszDataOne +
					dwSentBytesOne;
				wsaBuffer[BODY_INDEX_1].len = dwRemainBytesOne;
			}
			else //NOTE: Header and data1 have been sent but not all of data2.
			{
				//NOTE:sentbytesone includes sent header here, used to
				//determine all bytes before data two.
				dwSentBytesOne = HEADER_LEN + dwBytesOne;
				dwSentBytesTwo = dwBytesSentTotal - dwSentBytesOne;
				dwRemainBytesTwo = dwPacketLen - dwBytesSentTotal;

				wsaBuffer[HEADER_INDEX].buf = (PCHAR)&ChatMsg + HEADER_LEN;
				wsaBuffer[HEADER_INDEX].len = 0;
				wsaBuffer[BODY_INDEX_1].buf = (PCHAR)pszDataOne +
					(HEADER_LEN + dwBytesOne);
				wsaBuffer[BODY_INDEX_1].len = 0;
				wsaBuffer[BODY_INDEX_2].buf = (PCHAR)pszDataTwo +
					dwSentBytesTwo;
				wsaBuffer[BODY_INDEX_2].len = dwRemainBytesTwo;
			}
		}
	}
	return S_OK;
}

//NOTE: Alocates space for two strings in packet.
static HRESULT
PacketHeapAlloc(HANDLE hStdErr, PCHATMSG pChatMsg)
{
	if (0 != pChatMsg->wLenOne)
	{
		pChatMsg->pszDataOne = HeapAlloc(GetProcessHeap(),
			HEAP_ZERO_MEMORY,
			//NOTE: Number of bytes not characters
			(pChatMsg->wLenOne + 1) * sizeof(WCHAR));

		if (NULL == pChatMsg->pszDataOne)
		{
			ThreadPrintError(hStdErr, (PCHAR)__func__, __LINE__);
			return E_OUTOFMEMORY;
		}
	}

	if (0 != pChatMsg->wLenTwo)
	{
		pChatMsg->pszDataTwo = HeapAlloc(GetProcessHeap(),
			HEAP_ZERO_MEMORY,
			(pChatMsg->wLenTwo + 1) * sizeof(WCHAR));

		if (NULL == pChatMsg->pszDataTwo)
		{
			ThreadPrintError(hStdErr, (PCHAR)__func__, __LINE__);
			MyHeapFree(GetProcessHeap(), NO_OPTION, pChatMsg->pszDataOne,
				(pChatMsg->wLenOne) * sizeof(WCHAR));
			return E_OUTOFMEMORY;
		}
	}

	return S_OK;
}

//NOTE: false = 0, true = 1, HeapFree will return FALSE on failure.
BOOL
PacketHeapFree(PCHATMSG pChatMsg)
{
	//NOTE: Just freeing the string buffers here.

	BOOL bResult = TRUE;

	if (0 != pChatMsg->wLenOne)
	{
		bResult = MyHeapFree(GetProcessHeap(), NO_OPTION,
			pChatMsg->pszDataOne, (pChatMsg->wLenOne + 1) * sizeof(WCHAR));

		if (FALSE == bResult)
		{
			MyHeapFree(GetProcessHeap(), NO_OPTION,
				pChatMsg->pszDataTwo, (pChatMsg->wLenTwo + 1) * sizeof(WCHAR));
			return bResult;
		}
	}

	if (0 != pChatMsg->wLenTwo)
	{
		bResult = MyHeapFree(GetProcessHeap(), NO_OPTION,
			pChatMsg->pszDataTwo, pChatMsg->wLenTwo * sizeof(WCHAR));
	}

	return bResult;
}

//NOTE: Peeks at the socket to determine if there is data waiting or not.
// 
//Three results:
//No data: E_UNEXPECTED -> handle console print at calling function.
//fail: E_FAIL -> client shutdown
//data: S_OK -> data on the pipe!
HRESULT
SocketPeek(HANDLE hStdErr, SOCKET wsaSocket)
{
	//NOTE: We'll make a buffer of length two (with a terminator), to test if
	//any data is on the pipe.
	CHAR pBuffer[2] = { 0 };
	WSABUF wsaPeekBuff[1] = { 0 };
	wsaPeekBuff[0].buf = pBuffer;
	wsaPeekBuff[0].len = 1;

	//NOTE: Setting the rest of the values for the peek function.
	DWORD dwBytesRecv = 0;
	DWORD dwFlags = MSG_PEEK;
	PDWORD pdwFlags = &dwFlags;

	INT iResult = WSARecv(wsaSocket, wsaPeekBuff, ONE_BUFFER, &dwBytesRecv,
		pdwFlags, NULL, NULL);
	INT iResult2 = 0;
	if (SOCKET_ERROR == iResult)
	{
		iResult2 = WSAGetLastError();
		if (WSAEWOULDBLOCK == iResult2)
		{
			//NOTE: No data on the pipe.
			ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
				"Bad Packet");
			return E_UNEXPECTED;
		}
		ThreadPrintErrorSupplied(hStdErr, (PCHAR)__func__, __LINE__, iResult2);

		return E_FAIL;
	}
	if (0 == dwBytesRecv)
	{
		//NOTE: No data on the pipe.
		ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
			"Bad Packet");
		return E_UNEXPECTED;
	}

	return S_OK;
}

//NOTE: Sets event and waits until data gets to pipe, reads it!
HRESULT
BlockingRecv(WSAEVENT hReadEvent, HANDLE hStdErr, HANDLE hStdOut,
	SOCKET wsaSocket, LPWSABUF pwsaBuffer, DWORD dwBufferCount,
	PDWORD pdwBytesReceived, PDWORD pdwFlags)
{
	//NOTE: Don't need to release handle here as it will be released at the end
	//of program execution along with the stack.
	DWORD dwSocketWait = WaitForSingleObject(hReadEvent, LISTEN_TIMEOUT);
	switch (dwSocketWait)
	{
	case WAIT_OBJECT_0:
		//NOTE: Wait successful, WSArecv called.
		break;

	case WAIT_TIMEOUT:
		//NOTE: The server could just be very busy. Wait until connection is broken
		//to shutdown client.
		ThreadCustomConsoleWrite(hStdOut, L"Network issue: Server did not ackn"
			"owledge message within timeout period\n", MESSAGE_HDR_1_LEN);
		if (FALSE == WSAResetEvent(hReadEvent))
		{
			ThreadPrintErrorWSA(hStdErr, (PCHAR)__func__, __LINE__);
			return E_FAIL;
		}
		return E_UNEXPECTED;

	case WAIT_ABANDONED_0:
		ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
			"WAIT_ABANDONED_0");
		return E_FAIL;

	case WAIT_FAILED:
		ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
			"WAIT_FAILED");
		return E_FAIL;
	}

	if (FALSE == WSAResetEvent(hReadEvent))
	{
		ThreadPrintErrorWSA(hStdErr, (PCHAR)__func__, __LINE__);
		return E_FAIL;
	}

	if (SOCKET_ERROR == WSARecv(wsaSocket, pwsaBuffer, dwBufferCount,
		pdwBytesReceived, pdwFlags, NULL, NULL))
	{
		ThreadPrintErrorWSA(hStdErr, (PCHAR)__func__, __LINE__);
		return E_FAIL;
	}

	return S_OK;
}

//NOTE: Only receive header at first. Determines total packet length.
//No data: E_UNEXPECTED -> handle console print at calling function.
//fail: E_FAIL -> client shutdown
//data: S_OK -> data on the pipe!
static HRESULT
RecvHeader(HANDLE hStdErr, HANDLE hStdOut, SOCKET RecvSock,
	PCHATMSG pChatMsg, WSAEVENT hReadEvent, PDWORD pdwFlags,
	LPWSABUF pwsaRecvBuffer)
{
	DWORD dwBytesRecv = 0;
	DWORD dwBytesRecvTotal = 0;
	HRESULT hResult = S_OK;

	while (dwBytesRecvTotal < HEADER_LEN)
	{
		hResult = BlockingRecv(hReadEvent, hStdErr, hStdOut, RecvSock,
			pwsaRecvBuffer, ONE_BUFFER, &dwBytesRecv, pdwFlags);
		if (S_OK != hResult)
		{
			//NOTE: Calling function can handle difference between
			// E_FAIL and E_UNEXPECTED.
			if (E_FAIL == hResult)
			{
				ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
					"BlockingRecv()");
			}
			return hResult;
		}

		dwBytesRecvTotal += dwBytesRecv;

		if (dwBytesRecvTotal < HEADER_LEN)
		{
			pwsaRecvBuffer[HEADER_INDEX].buf = (PCHAR)pChatMsg +
				dwBytesRecvTotal;
			pwsaRecvBuffer[HEADER_INDEX].len = HEADER_LEN - dwBytesRecvTotal;

			hResult = SocketPeek(hStdErr, RecvSock);
			if (S_OK != hResult)
			{
				//NOTE: We could have an error here or a bad packet.
				ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
					"SocketPeek()");
				return hResult;
			}
		}
	}

	//NOTE: Get lengths back into host byte order.
	pChatMsg->wLenOne = ntohs(pChatMsg->wLenOne);
	pChatMsg->wLenTwo = ntohs(pChatMsg->wLenTwo);

	return S_OK;
}

//NOTE: Only called after recvheader is called.
//No data: E_UNEXPECTED -> handle console print at calling function.
//fail: E_FAIL -> client shutdown
//data: S_OK -> data on the pipe!
static HRESULT
RecvBody(HANDLE hStdErr, HANDLE hStdOut, SOCKET RecvSock,
	PCHATMSG pChatMsg, WSAEVENT hReadEvent, PDWORD pdwFlags,
	LPWSABUF pwsaRecvBuffer, DWORD dwBytesOne, DWORD dwBytesTwo)
{
	DWORD dwBytesLeft = dwBytesOne + dwBytesTwo;
	DWORD dwBytesRecvTotal = 0;
	DWORD dwBytesRecv = 0;
	HRESULT hResult = S_OK;

	pwsaRecvBuffer[BODY_INDEX_1].buf = (PCHAR)pChatMsg->pszDataOne;
	pwsaRecvBuffer[BODY_INDEX_1].len = dwBytesOne;
	pwsaRecvBuffer[BODY_INDEX_2].buf = (PCHAR)pChatMsg->pszDataTwo;
	pwsaRecvBuffer[BODY_INDEX_2].len = dwBytesTwo;


	while (dwBytesRecvTotal < dwBytesLeft)
	{
		//NOTE: +1 on WSA buffer bc we know we've already received the header -
		//now we're receiving the data.
		hResult = BlockingRecv(hReadEvent, hStdErr, hStdOut, RecvSock,
			(pwsaRecvBuffer + 1), TWO_BUFFERS, &dwBytesRecv, pdwFlags);
		if (S_OK != hResult)
		{
			if (E_FAIL == hResult)
			{
				ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
					"BlockingRecv()");
			}
			return hResult;
		}

		dwBytesRecvTotal += dwBytesRecv;

		if (dwBytesRecvTotal < dwBytesLeft)
		{
			if (dwBytesRecvTotal < dwBytesOne)
			{
				pwsaRecvBuffer[BODY_INDEX_1].buf = (PCHAR)pChatMsg->pszDataOne
					+ dwBytesRecvTotal;
				pwsaRecvBuffer[BODY_INDEX_1].len = dwBytesOne -
					dwBytesRecvTotal;
			}
			else
			{
				pwsaRecvBuffer[BODY_INDEX_1].buf = (PCHAR)pChatMsg->pszDataOne
					+ pChatMsg->wLenOne;
				pwsaRecvBuffer[BODY_INDEX_1].len = 0;
				pwsaRecvBuffer[BODY_INDEX_2].buf = (PCHAR)pChatMsg->pszDataTwo +
					(dwBytesRecvTotal - pChatMsg->wLenOne);
				pwsaRecvBuffer[BODY_INDEX_2].len = dwBytesTwo -
					(dwBytesRecvTotal - dwBytesOne);

			}

			hResult = SocketPeek(hStdErr, RecvSock);
			if (S_OK != hResult)
			{
				//NOTE: We could have an error here or a bad packet.
				if (E_FAIL == hResult)
				{
					ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
						"BlockingRecv()");
				}
				return hResult;
			}
		}
	}

	//NOTE: Convert the strings to host byte order.
	WstrNetToHost(pChatMsg->pszDataOne, pChatMsg->wLenOne);
	WstrNetToHost(pChatMsg->pszDataTwo, pChatMsg->wLenTwo);

	return S_OK;
}

//WARNING: Assumes that data1 and data2 are PWSTR type.
//WARNING: If using with threads, lock socket mutex and wait for data
//on pipe to use.
HRESULT
ClientRecvPacket(HANDLE hStdErr, HANDLE hStdOut, SOCKET RecvSock,
	PCHATMSG pChatMsg, WSAEVENT hReadEvent)
{
	if ((NULL == hStdErr) || (INVALID_SOCKET == RecvSock))
	{
		ThreadPrintError(hStdErr, (PCHAR)__func__, __LINE__);
		return E_HANDLE;
	}

	//NOTE: the second two indexes will be set after the first is recevied.
	WSABUF wsaRecvBuffer[THREE_BUFFERS] = { 0 };
	wsaRecvBuffer[HEADER_INDEX].buf = (PCHAR)pChatMsg;
	wsaRecvBuffer[HEADER_INDEX].len = HEADER_LEN;

	DWORD dwFlags = NO_OPTION;
	PDWORD pdwFlags = &dwFlags;

	HRESULT hResult = RecvHeader(hStdErr, hStdOut, RecvSock, pChatMsg,
		hReadEvent, pdwFlags, wsaRecvBuffer);

	if (S_OK != hResult)
	{
		//NOTE: Calling function can handle difference between
		// E_FAIL and E_UNEXPECTED.
		if (E_FAIL == hResult)
		{
			ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
				"RecvHeader()");
		}
		return hResult;
	}

	DWORD dwBytesOne = pChatMsg->wLenOne * sizeof(WCHAR);
	DWORD dwBytesTwo = pChatMsg->wLenTwo * sizeof(WCHAR);

	hResult = PacketHeapAlloc(hStdErr, pChatMsg);
	if (S_OK != hResult)
	{
		ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
			"PacketHeapAlloc()");
		return hResult;
	}

	hResult = RecvBody(hStdErr, hStdOut, RecvSock, pChatMsg, hReadEvent,
		pdwFlags, wsaRecvBuffer, dwBytesOne, dwBytesTwo);
	if (S_OK != hResult)
	{
		//NOTE: Calling function can handle difference between
		// E_FAIL and E_UNEXPECTED.
		if (E_FAIL == hResult)
		{
			ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
				"RecvBody()");
		}
		//NOTE: Calling function only responsible for freeing memory if
		// S_OK returned.
		if (FALSE == PacketHeapFree(pChatMsg))
		{
			ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
				"PacketHeapFree()");
			return E_FAIL;
		}
		return hResult;
	}

	return S_OK;
}

static HRESULT
ListenRecvHeader(HANDLE hStdErr, SOCKET RecvSock,
	PCHATMSG pChatMsg, PDWORD pdwFlags, LPWSABUF pwsaRecvBuffer)
{
	DWORD dwBytesRecv = 0;
	DWORD dwBytesRecvTotal = 0;
	HRESULT hResult = S_OK;

	while (dwBytesRecvTotal < HEADER_LEN)
	{
		if (SOCKET_ERROR == WSARecv(RecvSock, pwsaRecvBuffer, ONE_BUFFER,
			&dwBytesRecv, pdwFlags, NULL, NULL))
		{
			ThreadPrintErrorWSA(hStdErr, (PCHAR)__func__, __LINE__);
			return E_FAIL;
		}

		dwBytesRecvTotal += dwBytesRecv;

		if (dwBytesRecvTotal < HEADER_LEN)
		{
			pwsaRecvBuffer[HEADER_INDEX].buf = (PCHAR)pChatMsg +
				dwBytesRecvTotal;
			pwsaRecvBuffer[HEADER_INDEX].len = HEADER_LEN - dwBytesRecvTotal;

			hResult = SocketPeek(hStdErr, RecvSock);
			if (S_OK != hResult)
			{
				//NOTE: We could have an error here or a bad packet.
				ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
					"SocketPeek()");
				return hResult;
			}
		}
	}

	//NOTE: Get lengths back into host byte order.
	pChatMsg->wLenOne = ntohs(pChatMsg->wLenOne);
	pChatMsg->wLenTwo = ntohs(pChatMsg->wLenTwo);

	return S_OK;
}

static HRESULT
ListenRecvBody(HANDLE hStdErr, SOCKET RecvSock, PCHATMSG pChatMsg,
	PDWORD pdwFlags, LPWSABUF pwsaRecvBuffer, DWORD dwBytesOne,
	DWORD dwBytesTwo)
{
	DWORD dwBytesLeft = dwBytesOne + dwBytesTwo;
	DWORD dwBytesRecvTotal = 0;
	DWORD dwBytesRecv = 0;
	HRESULT hResult = S_OK;

	pwsaRecvBuffer[BODY_INDEX_1].buf = (PCHAR)pChatMsg->pszDataOne;
	pwsaRecvBuffer[BODY_INDEX_1].len = dwBytesOne;
	pwsaRecvBuffer[BODY_INDEX_2].buf = (PCHAR)pChatMsg->pszDataTwo;
	pwsaRecvBuffer[BODY_INDEX_2].len = dwBytesTwo;

	while (dwBytesRecvTotal < dwBytesLeft)
	{
		if (SOCKET_ERROR == WSARecv(RecvSock, (pwsaRecvBuffer + 1),
			TWO_BUFFERS, &dwBytesRecv, pdwFlags, NULL, NULL))
		{
			ThreadPrintErrorWSA(hStdErr, (PCHAR)__func__, __LINE__);
			return E_FAIL;
		}

		dwBytesRecvTotal += dwBytesRecv;

		if (dwBytesRecvTotal < dwBytesLeft)
		{
			if (dwBytesRecvTotal < dwBytesOne)
			{
				pwsaRecvBuffer[BODY_INDEX_1].buf = (PCHAR)pChatMsg->pszDataOne
					+ dwBytesRecvTotal;
				pwsaRecvBuffer[BODY_INDEX_1].len = dwBytesOne -
					dwBytesRecvTotal;
			}
			else
			{
				pwsaRecvBuffer[BODY_INDEX_1].buf = (PCHAR)pChatMsg->pszDataOne
					+ pChatMsg->wLenOne;
				pwsaRecvBuffer[BODY_INDEX_1].len = 0;
				pwsaRecvBuffer[BODY_INDEX_2].buf = (PCHAR)pChatMsg->pszDataTwo +
					(dwBytesRecvTotal - pChatMsg->wLenOne);
				pwsaRecvBuffer[BODY_INDEX_2].len = dwBytesTwo -
					(dwBytesRecvTotal - dwBytesOne);

			}

			hResult = SocketPeek(hStdErr, RecvSock);
			if (S_OK != hResult)
			{
				//NOTE: We could have an error here or a bad packet.
				if (E_FAIL == hResult)
				{
					ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
						"SocketPeek()");
				}
				return hResult;
			}
		}
	}

	//NOTE: Convert the strings to host byte order.
	WstrNetToHost(pChatMsg->pszDataOne, pChatMsg->wLenOne);
	WstrNetToHost(pChatMsg->pszDataTwo, pChatMsg->wLenTwo);

	return S_OK;
}

HRESULT
ListenThreadRecvPacket(HANDLE hStdErr, SOCKET RecvSock,
	PCHATMSG pChatMsg)
{
	if ((NULL == hStdErr) || (INVALID_SOCKET == RecvSock))
	{
		ThreadPrintError(hStdErr, (PCHAR)__func__, __LINE__);
		return E_HANDLE;
	}

	//NOTE: the second two indexes will be set after the first is recevied.
	WSABUF wsaRecvBuffer[THREE_BUFFERS] = { 0 };
	wsaRecvBuffer[HEADER_INDEX].buf = (PCHAR)pChatMsg;
	wsaRecvBuffer[HEADER_INDEX].len = HEADER_LEN;

	DWORD dwFlags = NO_OPTION;
	PDWORD pdwFlags = &dwFlags;

	HRESULT hResult = ListenRecvHeader(hStdErr, RecvSock, pChatMsg,
		pdwFlags, wsaRecvBuffer);

	if (S_OK != hResult)
	{
		//NOTE: Calling function can handle difference between
		// E_FAIL and E_UNEXPECTED.
		if (E_FAIL == hResult)
		{
			ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
				"ListenRecvHeader()");
		}
		return hResult;
	}

	DWORD dwBytesOne = pChatMsg->wLenOne * sizeof(WCHAR);
	DWORD dwBytesTwo = pChatMsg->wLenTwo * sizeof(WCHAR);

	hResult = PacketHeapAlloc(hStdErr, pChatMsg);
	if (S_OK != hResult)
	{
		ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
			"PacketHeapAlloc()");
		return hResult;
	}

	hResult = ListenRecvBody(hStdErr, RecvSock, pChatMsg,
		pdwFlags, wsaRecvBuffer, dwBytesOne, dwBytesTwo);
	if (S_OK != hResult)
	{
		//NOTE: Calling function can handle difference between
		// E_FAIL and E_UNEXPECTED.
		if (E_FAIL == hResult)
		{
			ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
				"RecvBody()");
		}
		//NOTE: Calling function only responsible for freeing memory if
		// S_OK returned.
		if (FALSE == PacketHeapFree(pChatMsg))
		{
			ThreadPrintErrorCustom(hStdErr, (PCHAR)__func__, __LINE__,
				"PacketHeapFree()");
			return E_FAIL;
		}
		return hResult;
	}

	return S_OK;
}

//End of file
