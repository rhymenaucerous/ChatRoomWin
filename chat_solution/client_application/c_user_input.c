/*****************************************************************//**
 * \file   c_user_input.c
 * \brief  
 * 
 * \author chris
 * \date   September 2024
 *********************************************************************/
#include "pch.h"
#include "c_user_input.h"

//NOTE: Following function acts like strtok() but is safer - we have a simple
//use for it.
static INT
DivideString(PWCHAR pszBuffer, PWCHAR pszUser, PWORD pwUserLen, PWCHAR pszMsg,
	PWORD pwMsgLen)
{
	//NOTE: There must be a space between /msg and the username.
	if ((pszBuffer[0] != L' ') || (pszBuffer[0] == L'\0'))
	{
		return EXIT_FAILURE;
	}

	//NOTE: Accounts for previously searched buffer space.
	WORD wOffset = 1;
	WORD wCounter = 0;

	//NOTE: The additional one is to account for the case where the username
	//is 10 characters long.
	for (wCounter = wOffset; wCounter < (MAX_UNAME_LEN + wOffset + 1);
		wCounter++)
	{
		pszUser[wCounter - wOffset] = pszBuffer[wCounter];
		
		if (pszBuffer[wCounter] == L' ')
		{
			break;
		}

		if (pszBuffer[wCounter] == L'\0')
		{
			return EXIT_FAILURE;
		}
	}

	//NOTE: This is the case where the username is longer than the max.
	if ((wCounter >= (MAX_UNAME_LEN + wOffset + 1)) ||
		(wCounter == wOffset))
	{
		return EXIT_FAILURE;
	}

	*pwUserLen = wCounter - wOffset;
	
	//NOTE: Two spaces have been seen now. The max that wOffset can be is
	//MAX_UNAME_LEN + 2.
	wOffset = *pwUserLen + 2;

	//NOTE (subject to change): The MAX_MSG_LEN is 1006 and the mas offset
	// is MAX_UNAME_LEN (10) + 2. This means wCounter will only go up to
	// 1018. This is allowed as the buffer is 1024 characters in length.
	// The buffer here is offset by the 4 characters of /msg and has a 
	// terminating zero.
	for (wCounter = wOffset; wCounter < (MAX_MSG_LEN + wOffset);
		wCounter++)
	{
		if ((pszBuffer[wCounter] == L'\0') || (pszBuffer[wCounter] == L'\n')
			|| (pszBuffer[wCounter] == L'\r'))
		{
			break;
		}

		pszMsg[wCounter - wOffset] = pszBuffer[wCounter];
	}

	//NOTE: There is no exit condition where failure occurs here.
	*pwMsgLen = wCounter - wOffset;
	
	//NOTE: The message must be at least one character in length.
	if (0 == *pwMsgLen)
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

HRESULT
HandleSrvReturn(PLISTENERARGS pListenerArgs, CHATMSG ExpectedReturn)
{
	CHATMSG RecvChat = { 0 };
	HRESULT hResult = S_OK;

	while (CONTINUE == InterlockedCompareExchange((PLONG)&g_bClientState,
		CONTINUE, CONTINUE))
	{
		//NOTE: The mutex is still locked because we want to receive the
		// message the corresponds to the one that we just sent. If a chat
		// message is received, we'll still keep the lock, but print that 
		// message in the normal fasion.
		SecureZeroMemory(&RecvChat, sizeof(CHATMSG));
		hResult = ClientRecvPacket(
			pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			pListenerArgs->m_hHandles[STD_OUT_MUTEX],
			pListenerArgs->m_ServerSocket, &RecvChat,
			(WSAEVENT)pListenerArgs->m_hHandles[READ_EVENT]);

		//NOTE: Once server IOCP has been established, client will wait for
		//multiple packets if chat updates come through. the listening thread will
		//not be able to grab the mutex.
		if (S_OK != hResult)
		{
			//NOTE: Calling function will handle shutting down client on fail.
			ThreadPrintError(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__);
			return hResult;
		}

		//wprintf(L"type: %d, stype: %d, opcode: %d, msg lens: %u, %u\n",
		//	RecvChat.iType, RecvChat.iSubType, RecvChat.iOpcode,
		//	RecvChat.wLenOne, RecvChat.wLenTwo);

		//NOTE: Checks for message return packet, packet acknowledge, and
		// failure packets.
		if ((RecvChat.iType == TYPE_CHAT) &&
			(RecvChat.iSubType == STYPE_EMPTY) &&
			(RecvChat.iOpcode == OPCODE_RES)) //Message update packet
		{
			//NOTE: Only on messages will the loop keep going.
			CustomStringPrintTwo(RecvChat.pszDataOne, RecvChat.pszDataTwo,
				RecvChat.wLenOne, RecvChat.wLenTwo,
				pListenerArgs->m_hHandles[STD_ERR_MUTEX], 
				pListenerArgs->m_hHandles[STD_OUT_MUTEX]);
		}
		else if ((RecvChat.iType == ExpectedReturn.iType) &&
			(RecvChat.iSubType == ExpectedReturn.iSubType) &&
			(RecvChat.iOpcode == ExpectedReturn.iOpcode)) //NOTE: Expected packet
		{
			if ((RecvChat.iType == TYPE_LIST) &&
				(RecvChat.iSubType == STYPE_EMPTY) &&
				(RecvChat.iOpcode == OPCODE_RES)) //NOTE: Handling list case
			{
				ThreadCustomConsoleWrite(pListenerArgs->m_hHandles[STD_OUT_MUTEX],
					RecvChat.pszDataOne, RecvChat.wLenOne);
			}
			
			break;
		}
		else if (RecvChat.iType == TYPE_FAILURE) //NOTE: Failure packet
		{
			ThreadPrintFailurePacket(pListenerArgs->m_hHandles[STD_OUT_MUTEX],
				RecvChat.iOpcode);
			break;
		}
		else //unknown packet
		{
			ThreadCustomConsoleWrite(pListenerArgs->m_hHandles[STD_OUT_MUTEX],
				L"Failure: Invalid packet received from server", 45);
			break;
		}

		if (FALSE == PacketHeapFree(&RecvChat))
		{
			ThreadPrintErrorCustom(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "PacketHeapFree()");
			return E_FAIL;
		}
	}

	return S_OK;
}

static HRESULT
HandleMsg(PLISTENERARGS pListenerArgs, WORD wLenUser, PWSTR pszUser,
	WORD wLenMsg, PWSTR pszMsg)
{	
	HANDLE hSocketHandle = pListenerArgs->m_hHandles[SOCKET_MUTEX];
	DWORD dwWaitObj = WaitForSingleObject(hSocketHandle, INFINITE);

	switch(dwWaitObj)
	{
	case WAIT_OBJECT_0:
		if (S_OK != SendPacket(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			pListenerArgs->m_ServerSocket, TYPE_CHAT, STYPE_EMPTY,
			OPCODE_REQ,
			wLenUser, wLenMsg, pszUser, pszMsg))
		{

			ReleaseMutex(hSocketHandle);
			ThreadPrintErrorCustom(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "SendPacket()");
			return E_FAIL;
		}
		break;

	default:
		ReleaseMutex(hSocketHandle);
		ThreadPrintError(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__);
		return E_FAIL;
	}

	CHATMSG ExpectedReturn = { 0 };
	ExpectedReturn.iType = TYPE_CHAT;
	ExpectedReturn.iSubType = STYPE_EMPTY;
	ExpectedReturn.iOpcode = OPCODE_ACK;

	HRESULT hReturn = HandleSrvReturn(pListenerArgs, ExpectedReturn);
	ReleaseMutex(hSocketHandle);

	if (E_FAIL == hReturn)
	{
		ThreadPrintErrorCustom(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__, "PacketHeapFree()");
		return hReturn;
	}

	return S_OK;
}

static HRESULT
HandleBroadcast(PLISTENERARGS pListenerArgs, WORD wLenMsg, PWSTR pszMsg)
{
	HANDLE hSocketHandle = pListenerArgs->m_hHandles[SOCKET_MUTEX];
	DWORD dwWaitObj = WaitForSingleObject(hSocketHandle, INFINITE);

	switch (dwWaitObj)
	{
	case WAIT_OBJECT_0:
		if (S_OK != SendPacket(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			pListenerArgs->m_ServerSocket, TYPE_BROADCAST, STYPE_EMPTY,
			OPCODE_REQ, wLenMsg, 0, pszMsg, NULL))
		{

			ReleaseMutex(hSocketHandle);
			ThreadPrintErrorCustom(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "SendPacket()");
			return E_FAIL;
		}
		break;

	default:
		ReleaseMutex(hSocketHandle);
		ThreadPrintError(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__);
		return E_FAIL;
	}

	CHATMSG ExpectedReturn = { 0 };
	ExpectedReturn.iType = TYPE_BROADCAST;
	ExpectedReturn.iSubType = STYPE_EMPTY;
	ExpectedReturn.iOpcode = OPCODE_ACK;

	HRESULT hReturn = HandleSrvReturn(pListenerArgs, ExpectedReturn);
	ReleaseMutex(hSocketHandle);

	if (E_FAIL == hReturn)
	{
		ThreadPrintErrorCustom(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__, "PacketHeapFree()");
		return hReturn;
	}

	return S_OK;
}

static HRESULT
HandleList(PLISTENERARGS pListenerArgs)
{
	HANDLE hSocketHandle = pListenerArgs->m_hHandles[SOCKET_MUTEX];
	DWORD dwWaitObj = WaitForSingleObject(hSocketHandle, INFINITE);

	switch (dwWaitObj)
	{
	case WAIT_OBJECT_0:
		if (S_OK != SendPacket(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			pListenerArgs->m_ServerSocket, TYPE_LIST, STYPE_EMPTY, OPCODE_REQ,
			0, 0, NULL, NULL))
		{

			ReleaseMutex(hSocketHandle);
			ThreadPrintErrorCustom(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "SendPacket()");
			return E_FAIL;
		}
		break;

	default:
		ReleaseMutex(hSocketHandle);
		ThreadPrintError(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__);
		return E_FAIL;
	}

	CHATMSG ExpectedReturn = { 0 };
	ExpectedReturn.iType = TYPE_LIST;
	ExpectedReturn.iSubType = STYPE_EMPTY;
	ExpectedReturn.iOpcode = OPCODE_RES;

	HRESULT hReturn = HandleSrvReturn(pListenerArgs, ExpectedReturn);
	ReleaseMutex(hSocketHandle);

	if (S_OK != hReturn)
	{
		ThreadPrintErrorCustom(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__, "PacketHeapFree()");
		return hReturn;
	}

	return S_OK;
}

static HRESULT
HandleQuit(PLISTENERARGS pListenerArgs)
{
	HANDLE hSocketHandle = pListenerArgs->m_hHandles[SOCKET_MUTEX];
	DWORD dwWaitObj = WaitForSingleObject(hSocketHandle, INFINITE);
	HRESULT hResult = S_OK;

	switch (dwWaitObj)
	{
	case WAIT_OBJECT_0:
		hResult = SendPacket(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			pListenerArgs->m_ServerSocket, TYPE_ACCOUNT, STYPE_LOGOUT,
			OPCODE_REQ, 0, 0, NULL, NULL);
		if (S_OK != hResult)
		{
			ReleaseMutex(hSocketHandle);
			ThreadPrintErrorCustom(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__, "SendPacket()");
			return E_FAIL;
		}
		break;

	default:
		ReleaseMutex(hSocketHandle);
		ThreadPrintError(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__);
		return E_FAIL;
	}

	CHATMSG ExpectedReturn = { 0 };
	ExpectedReturn.iType = TYPE_ACCOUNT;
	ExpectedReturn.iSubType = STYPE_LOGOUT;
	ExpectedReturn.iOpcode = OPCODE_ACK;

	HRESULT hReturn = HandleSrvReturn(pListenerArgs, ExpectedReturn);
	ReleaseMutex(hSocketHandle);

	if (E_FAIL == hReturn)
	{
		ThreadPrintErrorCustom(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__, "HandleSrvReturn()");
		return hReturn;
	}

	return S_OK;
}

//NOTE: static function for printing help message.
static VOID
PrintHelp(HANDLE hStdErrMutex)
{
	ThreadCustomConsoleWrite(hStdErrMutex,
		L"Options Allowed:\n/msg\n/broadcast\n/list\n/quit\n",
		46);
}

//grab necessary structures and start listening - listener
//creates new threads
HRESULT
UserListen(PLISTENERARGS pListenerArgs)
{
	if (NULL == pListenerArgs)
	{
		//NOTE: Not using thread print bc we can dereference a NULL pointer
		//(the listener args struct).
		PrintErrorCustom((PCHAR)__func__, __LINE__, "Input NULL");
		return E_POINTER;
	}

	WCHAR caUserInputBuffer[BUFF_SIZE] = { 0 };
	DWORD wNumberofCharsRead = 0;
	BOOL bResult = FALSE;
	HANDLE hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	HRESULT hResult = S_OK;

	if (INVALID_HANDLE_VALUE == hStdInput)
	{
		ThreadPrintErrorCustom(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__, "Input NULL");
		InterlockedExchange((PLONG)&g_bClientState, STOP);
		return E_HANDLE;
	}

	while (CONTINUE == InterlockedCompareExchange((PLONG)&g_bClientState,
		CONTINUE, CONTINUE))
	{
		bResult = ReadConsoleW(hStdInput, caUserInputBuffer, (BUFF_SIZE - 1),
			&wNumberofCharsRead, NULL);

		if (FALSE == bResult)
		{
			ThreadPrintError(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
				(PCHAR)__func__, __LINE__);
			InterlockedExchange((PLONG)&g_bClientState, STOP);
			return E_FAIL;
		}

		if ((STOP == InterlockedCompareExchange((PLONG)&g_bClientState,
			STOP, STOP)) || (wNumberofCharsRead <= 0))
		{
			break;
		}

		//TODO: Pass to functions for handling each command-> only two options:
		//msg or list.
		if (STRINGS_EQUAL == wcsncmp(L"/msg", caUserInputBuffer, CMD_1_LEN))
		{
			//NOTE: Preventing buffer overflows is priority #1: a little extra
			// space for the message is allocated on the stack - the max size.
			WCHAR caUser[MAX_UNAME_LEN + 1] = {0};
			WORD wUserLen = 0;

			WCHAR caMsg[MAX_MSG_LEN + 1] = {0};
			WORD wMsgLen = 0;
			
			INT iResult = DivideString((caUserInputBuffer + CMD_1_LEN), caUser,
				&wUserLen, caMsg, &wMsgLen);

			if (iResult != EXIT_SUCCESS)
			{
				PrintHelp(pListenerArgs->m_hHandles[STD_ERR_MUTEX]);
				continue;
			}
			
			hResult = HandleMsg(pListenerArgs, wUserLen, (PWSTR)caUser,
				wMsgLen, (PWSTR)caMsg);
			if (S_OK != hResult)
			{
				ThreadPrintErrorCustom(
					pListenerArgs->m_hHandles[STD_ERR_MUTEX],
					(PCHAR)__func__, __LINE__, "HandleMsg()");
				InterlockedExchange((PLONG)&g_bClientState, STOP);
				return hResult;
			}
		}
		else if (STRINGS_EQUAL == wcsncmp(L"/broadcast ", caUserInputBuffer,
			11))
		{
			//NOTE: Preventing buffer overflows is priority #1: a little extra
			// space for the message is allocated on the stack - the max size.
			//NOTE: Conversion from DWORD to WORD is fine bc max length is 1024.
			hResult = HandleBroadcast(pListenerArgs, (WORD)(wNumberofCharsRead - 12),
				(caUserInputBuffer + 11));
			if (S_OK != hResult)
			{
				ThreadPrintErrorCustom(
					pListenerArgs->m_hHandles[STD_ERR_MUTEX],
					(PCHAR)__func__, __LINE__, "HandleBroadcast()");
				InterlockedExchange((PLONG)&g_bClientState, STOP);
				return hResult;
			}
		}
		else if (STRINGS_EQUAL == wcsncmp(L"/list", caUserInputBuffer,
			CMD_2_LEN))
		{
			hResult = HandleList(pListenerArgs);
			if (S_OK != hResult)
			{
				ThreadPrintErrorCustom(
					pListenerArgs->m_hHandles[STD_ERR_MUTEX],
					(PCHAR)__func__, __LINE__, "HandleMsg()");
				InterlockedExchange((PLONG)&g_bClientState, STOP);
				return hResult;
			}
		}
		else if (STRINGS_EQUAL == wcsncmp(L"/quit", caUserInputBuffer,
			CMD_2_LEN))
		{
			hResult = HandleQuit(pListenerArgs);
			InterlockedExchange((PLONG)&g_bClientState, STOP);
			if (S_OK != hResult)
			{
				ThreadPrintErrorCustom(
					pListenerArgs->m_hHandles[STD_ERR_MUTEX],
					(PCHAR)__func__, __LINE__, "HandleMsg()");
				return hResult;
			}
		}
		else
		{
			PrintHelp(pListenerArgs->m_hHandles[STD_ERR_MUTEX]);
		}
	}

	return S_OK;
}

//End of file
