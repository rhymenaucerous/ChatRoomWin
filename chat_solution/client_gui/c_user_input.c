/*****************************************************************//**
 * \file   c_user_input.c
 * \brief
 *
 * \author chris
 * \date   September 2024
 *********************************************************************/
#include "..\networking\networking.h"
#include <Windows.h>
#include <stdio.h>

#include "c_user_input.h"
#include "c_messages.h"
#include "c_shared.h"
#include "c_main.h"

extern volatile BOOL g_bClientState;
extern HANDLE        g_hShutdownEvent;

//NOTE: Following function acts like strtok() but is safer - we have a simple
//use for it.
// ERR_GENERIC or SUCCESS
static INT
DivideString(PWCHAR pszBuffer, PWCHAR pszUser, PWORD pwUserLen, PWCHAR pszMsg,
	PWORD pwMsgLen)
{
	//NOTE: There must be a space between /msg and the username.
	if ((pszBuffer[0] != L' ') || (pszBuffer[0] == L'\0'))
	{
		return ERR_GENERIC;
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
			return ERR_GENERIC;
		}
	}

	//NOTE: This is the case where the username is longer than the max.
	if ((wCounter >= (MAX_UNAME_LEN + wOffset + 1)) ||
		(wCounter == wOffset))
	{
		return ERR_GENERIC;
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
		return ERR_GENERIC;
	}

	return SUCCESS;
}

//NOTE: Converts unix-style newlines (\n) to windows-style (\r\n) for display
// in GUI elements. Returns a heap-allocated string that must be freed by the
// caller.
static PWCHAR
ConvertNewlinesForDisplay(const WCHAR *pInput)
{
	if (!pInput)
	{
		return NULL;
	}

	// Count newlines to determine required buffer size
	size_t nExtraChars = 0;
	for (const WCHAR *p = pInput; *p; p++)
	{
		if (*p == L'\n')
		{
			nExtraChars++;
		}
	}

	if (nExtraChars == 0)
	{
		// No conversion needed, but we still need to return a copy that the caller can free.
		size_t nLen = wcslen(pInput);
		PWCHAR pNoChangeNeeded = HeapAlloc(
			GetProcessHeap(), 0, (nLen + 1) * sizeof(WCHAR));
		if (pNoChangeNeeded)
		{
			wcscpy_s(pNoChangeNeeded, nLen + 1, pInput);
		}
		return pNoChangeNeeded;
	}

	size_t nOriginalLen = wcslen(pInput);
	size_t nNewLen = nOriginalLen + nExtraChars;
	PWCHAR pOutput =
		HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (nNewLen + 1) * sizeof(WCHAR));
	if (!pOutput)
	{
		return NULL;
	}

	// Copy the string, inserting \r before every \n
	const WCHAR *pRead = pInput;
	PWCHAR pWrite = pOutput;
	while (*pRead)
	{
		if (*pRead == L'\n')
		{
			*pWrite++ = L'\r';
		}
		*pWrite++ = *pRead++;
	}
	*pWrite = L'\0';

	return pOutput;
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
        hResult =
            ClientRecvPacket(pListenerArgs->m_ServerSocket, &RecvChat,
                             (WSAEVENT)pListenerArgs->m_hHandles[READ_EVENT]);

		//NOTE: Once server IOCP has been established, client will wait for
		//multiple packets if chat updates come through. the listening thread will
		//not be able to grab the mutex.
		if (S_OK != hResult)
		{
			//NOTE: Calling function will handle shutting down client on fail.
			DEBUG_ERROR("ClientRecvPacket failed");
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
                                 RecvChat.wLenOne, RecvChat.wLenTwo);
		}
		else if ((RecvChat.iType == ExpectedReturn.iType) &&
			(RecvChat.iSubType == ExpectedReturn.iSubType) &&
			(RecvChat.iOpcode == ExpectedReturn.iOpcode)) //NOTE: Expected packet
		{
			if ((RecvChat.iType == TYPE_LIST) &&
				(RecvChat.iSubType == STYPE_EMPTY) &&
				(RecvChat.iOpcode == OPCODE_RES)) //NOTE: Handling list case
			{
				PWCHAR pFormattedList =
					ConvertNewlinesForDisplay(RecvChat.pszDataOne);
				if (pFormattedList)
				{
                    AppendToDisplay(
                        L"List of connected users:");
					AppendToDisplay(pFormattedList);
					HeapFree(GetProcessHeap(), 0, pFormattedList);
				}
				else
				{
					// Fallback to original behavior on allocation failure
					AppendToDisplay(RecvChat.pszDataOne);
				}
			}

			break;
		}
		else if (RecvChat.iType == TYPE_FAILURE) //NOTE: Failure packet
		{
            PrintFailurePacket(RecvChat.iOpcode);
			return E_UNEXPECTED;
		}
		else //unknown packet
		{
			DEBUG_ERROR("Failure: Invalid packet received from server");
			break;
		}
		if (FALSE == PacketHeapFree(&RecvChat))
		{
			DEBUG_ERROR("PacketHeapFree failed");
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
    DWORD  dwWaitObj     = CustomWaitForSingleObject(hSocketHandle, INFINITE);
    ResetEvent(pListenerArgs->m_hHandles[ULISTEN_WAITING]);
    SetEvent(pListenerArgs->m_hHandles[ULISTEN_WAIT_FINISHED]);
	switch(dwWaitObj)
	{
    case WAIT_OBJECT_0:
        if (S_OK != SendPacket(pListenerArgs->m_ServerSocket, TYPE_CHAT,
                               STYPE_EMPTY, OPCODE_REQ, wLenUser, wLenMsg,
                               pszUser, pszMsg))
		{

			ReleaseMutex(hSocketHandle);
			DEBUG_ERROR("SendPacket failed");
			return E_FAIL;
		}
		break;

	default:
		ReleaseMutex(hSocketHandle);
		DEBUG_ERROR("Invalid socket");
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
		DEBUG_ERROR("PacketHeapFree failed");
		return hReturn;
    }

	// If the server ackowledged the message, print it to the display.
	if (E_UNEXPECTED != hReturn)
    {
        WstrNetToHost(pszUser, wLenUser);
        WstrNetToHost(pszMsg, wLenMsg);
        AppendToDisplayNoNewline(L"You to [ ");
        AppendToDisplayNoNewline(pszUser);
        AppendToDisplayNoNewline(L"]> ");
        AppendToDisplay(pszMsg);
	}

	return S_OK;
}

static HRESULT
HandleBroadcast(PLISTENERARGS pListenerArgs, WORD wLenMsg, PWSTR pszMsg)
{
	HANDLE hSocketHandle = pListenerArgs->m_hHandles[SOCKET_MUTEX];
    DWORD  dwWaitObj     = CustomWaitForSingleObject(hSocketHandle, INFINITE);
    ResetEvent(pListenerArgs->m_hHandles[ULISTEN_WAITING]);
    SetEvent(pListenerArgs->m_hHandles[ULISTEN_WAIT_FINISHED]);

	switch (dwWaitObj)
	{
    case WAIT_OBJECT_0:
        if (S_OK != SendPacket(pListenerArgs->m_ServerSocket, TYPE_BROADCAST,
                               STYPE_EMPTY, OPCODE_REQ, wLenMsg, 0, pszMsg,
                               NULL))
		{

			ReleaseMutex(hSocketHandle);
			DEBUG_ERROR("SendPacket failed");
			return E_FAIL;
		}
		break;

	default:
		ReleaseMutex(hSocketHandle);
		DEBUG_ERROR("Invalid socket");
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
		DEBUG_ERROR("PacketHeapFree failed");
		return hReturn;
	}

	return S_OK;
}

static HRESULT
HandleList(PLISTENERARGS pListenerArgs)
{
	HANDLE hSocketHandle = pListenerArgs->m_hHandles[SOCKET_MUTEX];
    DWORD  dwWaitObj     = CustomWaitForSingleObject(hSocketHandle, INFINITE);
    ResetEvent(pListenerArgs->m_hHandles[ULISTEN_WAITING]);
    SetEvent(pListenerArgs->m_hHandles[ULISTEN_WAIT_FINISHED]);

	switch (dwWaitObj)
	{
    case WAIT_OBJECT_0:
        if (S_OK != SendPacket(pListenerArgs->m_ServerSocket, TYPE_LIST,
                               STYPE_EMPTY, OPCODE_REQ, 0, 0, NULL, NULL))
		{

			ReleaseMutex(hSocketHandle);
			DEBUG_ERROR("SendPacket failed");
			return E_FAIL;
		}
		break;

	default:
		ReleaseMutex(hSocketHandle);
		DEBUG_ERROR("Invalid socket");
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
		DEBUG_ERROR("PacketHeapFree failed");
		return hReturn;
	}

	return S_OK;
}

static HRESULT
HandleQuit(PLISTENERARGS pListenerArgs)
{
	HANDLE hSocketHandle = pListenerArgs->m_hHandles[SOCKET_MUTEX];
    DWORD  dwWaitObj     = CustomWaitForSingleObject(hSocketHandle, INFINITE);
    ResetEvent(pListenerArgs->m_hHandles[ULISTEN_WAITING]);
    SetEvent(pListenerArgs->m_hHandles[ULISTEN_WAIT_FINISHED]);
	HRESULT hResult = S_OK;

	switch (dwWaitObj)
	{
    case WAIT_OBJECT_0:
        hResult = SendPacket(pListenerArgs->m_ServerSocket, TYPE_ACCOUNT,
                             STYPE_LOGOUT, OPCODE_REQ, 0, 0, NULL, NULL);
		if (S_OK != hResult)
		{
			ReleaseMutex(hSocketHandle);
			DEBUG_ERROR("SendPacket failed");
			return E_FAIL;
		}
		break;

	default:
		ReleaseMutex(hSocketHandle);
		DEBUG_ERROR("Invalid socket");
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
		DEBUG_ERROR("HandleSrvReturn failed");
		return hReturn;
	}

	return S_OK;
}

//NOTE: static function for printing help message.
static VOID
PrintHelp()
{
    AppendToDisplay(
        L"Options Allowed:\r\n/msg\r\n/broadcast\r\n/list\r\n/quit\r\n");
}

//grab necessary structures and start listening - listener
//creates new threads
HRESULT
UserListen(PLISTENERARGS pListenerArgs)
{
    HRESULT hResult = E_FAIL;
	if (NULL == pListenerArgs)
	{
		//NOTE: Not using thread print bc we can dereference a NULL pointer
		//(the listener args struct).
		DEBUG_ERROR("Input NULL");
		return E_POINTER;
	}

	WCHAR caUserInputBuffer[BUFF_SIZE] = { 0 };
	DWORD dwNumberofCharsRead = 0;
	BOOL bResult = FALSE;

	while (CONTINUE == InterlockedCompareExchange((PLONG)&g_bClientState,
		CONTINUE, CONTINUE))
    {
        SecureZeroMemory(caUserInputBuffer, BUFF_SIZE * sizeof(WCHAR));
        ResetEvent(pListenerArgs->m_hHandles[ULISTEN_WAIT_FINISHED]);
        SetEvent(pListenerArgs->m_hHandles[ULISTEN_WAITING]);
        GetAndAssign(caUserInputBuffer, &dwNumberofCharsRead);

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

			if (SUCCESS != iResult)
			{
				PrintHelp();
				continue;
			}

			hResult = HandleMsg(pListenerArgs, wUserLen, (PWSTR)caUser,
				wMsgLen, (PWSTR)caMsg);
			if (S_OK != hResult)
			{
                DEBUG_ERROR("HandleMsg failed");
                goto EXIT;
			}
		}
		else if (STRINGS_EQUAL == wcsncmp(L"/broadcast ", caUserInputBuffer,
			11))
		{
			//NOTE: Preventing buffer overflows is priority #1: a little extra
			// space for the message is allocated on the stack - the max size.
			//NOTE: Conversion from DWORD to WORD is fine bc max length is 1024.
            hResult =
                HandleBroadcast(pListenerArgs, (WORD)(dwNumberofCharsRead - 11),
				(caUserInputBuffer + 11));
			if (S_OK != hResult)
			{
                DEBUG_ERROR("HandleBroadcast failed");
                goto EXIT;
			}
		}
		else if (STRINGS_EQUAL == wcsncmp(L"/list", caUserInputBuffer,
			5))
		{
			hResult = HandleList(pListenerArgs);
			if (S_OK != hResult)
			{
				DEBUG_ERROR("HandleList failed");
				goto EXIT;
			}
		}
		else if (STRINGS_EQUAL == wcsncmp(L"/quit", caUserInputBuffer,
			5))
		{
			hResult = HandleQuit(pListenerArgs);
			if (S_OK != hResult)
			{
				DEBUG_ERROR("HandleQuit failed");
			}
            goto EXIT;
		}
		else
		{
			PrintHelp();
		}
	}

	hResult =  S_OK;
EXIT:
    InterlockedExchange((PLONG)&g_bClientState, STOP);
    SetEvent(g_hShutdownEvent);
    return hResult;
}

//End of file
