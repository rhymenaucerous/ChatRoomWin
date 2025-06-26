/*****************************************************************//**
 * \file   c_srv_listen.c
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdio.h>

#include "c_srv_listen.h"
#include "c_messages.h"
#include "c_shared.h"
#include "c_main.h"

#include "..\networking\networking.h"

extern volatile BOOL g_bClientState;

 //NOTE: Function that listens for chats and broadcasts and pritns them
 //to the console.
VOID
ListenForChats(PVOID pListenerArgsHolder)
{
	PLISTENERARGS pListenerArgs = (PLISTENERARGS)pListenerArgsHolder;

	if (NULL == pListenerArgs)
	{
		DEBUG_ERROR("Invalid listener args");
		InterlockedExchange((PLONG)&g_bClientState, STOP);
		return;
	}

	WCHAR MyBuff[BUFF_SIZE] = { 0 };
	HRESULT hResult = S_OK;
	INT iResult = 0;
	CHATMSG ChatMsg = { 0 };

	//NOT: Used for testing if message is in pipe.
	WSABUF TestBuf[ONE_BUFFER] = { 0 };
	WCHAR TestBuffer[2] = { 0 };
	TestBuf[0].len = 1;
	TestBuf[0].buf = (PCHAR)TestBuffer;
	DWORD dwFlags = MSG_PEEK;
	PDWORD pdwFlags = &dwFlags;

	//NOTE: While the client is running, we'll continue to listen for data
	//from the server to print to the console.
	while (CONTINUE == InterlockedCompareExchange((PLONG)&g_bClientState,
		CONTINUE, CONTINUE))
	{
		SecureZeroMemory(MyBuff, BUFF_SIZE);

		//NOTE: Ensures that we're reseting each time.
		if (FALSE == WSAResetEvent(pListenerArgs->m_hHandles[READ_EVENT]))
		{
			DEBUG_WSAERROR("WSAResetEvent failed");
			InterlockedExchange((PLONG)&g_bClientState, STOP);
			return;
		}

		//NOTE: waiting for the socket and the event that FD_READ is on the
		//socket.
		DWORD dwSocketWait = WaitForMultipleObjects(NUM_WAIT_HANDLES,
			pListenerArgs->m_hHandles, TRUE, INFINITE);
		if (STOP == InterlockedCompareExchange((PLONG)&g_bClientState,
			STOP, STOP))
		{
			break;
		}

		switch (dwSocketWait)
		{
		case WAIT_OBJECT_0:
			//NOTE: The listener received a message packet.
            hResult =
                ListenThreadRecvPacket(pListenerArgs->m_ServerSocket, &ChatMsg);
			ReleaseMutex(pListenerArgs->m_hHandles[SOCKET_MUTEX]);
			if (S_OK != hResult)
			{
				DEBUG_WSAERROR("WSARecv failed");
				InterlockedExchange((PLONG)&g_bClientState, STOP);
				return;
			}
			break;

		case WAIT_TIMEOUT:
			//NOTE: Checks to see if the connection has been reset.
			dwFlags = MSG_PEEK;
			iResult = WSARecv(pListenerArgs->m_ServerSocket, TestBuf,
				NO_OPTION, NULL, pdwFlags, NULL, NULL);
			ReleaseMutex(pListenerArgs->m_hHandles[SOCKET_MUTEX]);
			if (SOCKET_ERROR == iResult)
			{
				INT wsaLastError = WSAGetLastError();
				if (WSAECONNRESET == wsaLastError)
				{
					DEBUG_ERROR_SUPPLIED(wsaLastError, "WSARecv failed");
					InterlockedExchange((PLONG)&g_bClientState, STOP);
					return;
				}
			}
			//NOTE: Loop would continue here, but there are multiple cases for
			//which the event will be reset.
			break;

		default:
			ReleaseMutex(pListenerArgs->m_hHandles[SOCKET_MUTEX]);
			DEBUG_WSAERROR("WSARecv failed");
			InterlockedExchange((PLONG)&g_bClientState, STOP);
			return;
		}

		if (WAIT_TIMEOUT == dwSocketWait)
		{
			continue;
		}

		if ((ChatMsg.iType == TYPE_CHAT) &&
			(ChatMsg.iSubType == STYPE_EMPTY) &&
			(ChatMsg.iOpcode == OPCODE_RES))
		{
			CustomStringPrintTwo(ChatMsg.pszDataOne, ChatMsg.pszDataTwo,
				ChatMsg.wLenOne, ChatMsg.wLenTwo);

			if (FALSE == PacketHeapFree(&ChatMsg))
			{
				DEBUG_ERROR("PacketHeapFree failed");
				InterlockedExchange((PLONG)&g_bClientState, STOP);
				return;
			}
		}
		else
		{
			CustomConsoleWrite(L"Invalid message packet received from server."
				"\n", 46);
		}
	}
}

//End of file
