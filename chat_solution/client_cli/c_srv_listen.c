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
extern HANDLE        g_hShutdownEvent;

 //NOTE: Function that listens for chats and broadcasts and pritns them
 //to the console.
VOID
ListenForChats(PVOID pListenerArgsHolder)
{
	PLISTENERARGS pListenerArgs = (PLISTENERARGS)pListenerArgsHolder;
	if (NULL == pListenerArgs)
	{
		DEBUG_ERROR("Invalid listener args");
        goto EXIT;
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

	// Event handles for listening to server messages.
    HANDLE haReadHandles[3]      = {0};
    haReadHandles[SUCCESS_EVENT] = pListenerArgs->m_hHandles[READ_EVENT];
    haReadHandles[SYNC_EVENT] =
        pListenerArgs->m_hHandles[ULISTEN_WAIT_FINISHED];
    haReadHandles[SHUTDOWN_EVENT] = g_hShutdownEvent;

    // Event handles for getting socket mutex.
    HANDLE haSocketHandles[3]      = {0};
    haSocketHandles[SUCCESS_EVENT] = pListenerArgs->m_hHandles[SOCKET_MUTEX];
    haSocketHandles[SYNC_EVENT] =
        pListenerArgs->m_hHandles[ULISTEN_WAIT_FINISHED];
    haSocketHandles[SHUTDOWN_EVENT] = g_hShutdownEvent;


	//NOTE: While the client is running, we'll continue to listen for data
	//from the server to print to the console.
	while (CONTINUE == InterlockedCompareExchange((PLONG)&g_bClientState,
		CONTINUE, CONTINUE))
	{
		SecureZeroMemory(MyBuff, BUFF_SIZE);

		DWORD dwWaitObject = CustomWaitForSingleObject(
            pListenerArgs->m_hHandles[ULISTEN_WAITING], INFINITE);
        if (WAIT_OBJECT_0 != dwWaitObject)
        {
            if (WAIT_FAILED == dwWaitObject)
            {
                DEBUG_WSAERROR(
                    "CustomWaitForSingleObject failed: ULISTEN_WAITING");
            }
            goto FAIL;
        }

		// Wait for the read event to be signaled.
		DWORD dwReadReady = WaitForMultipleObjects(3, haReadHandles, FALSE, INFINITE);
        WSAResetEvent(pListenerArgs->m_hHandles[READ_EVENT]);
		if (WAIT_OBJECT_0 == dwReadReady)
        {
            // Wait for the socket mutex to be available.
            DWORD dwSocketWait =
                WaitForMultipleObjects(3, haSocketHandles, FALSE, INFINITE);
			if (WAIT_OBJECT_0 == dwSocketWait)
            {

                // NOTE: The listener received a message packet.
                hResult = ListenThreadRecvPacket(pListenerArgs->m_ServerSocket,
                                                 &ChatMsg);
                ReleaseMutex(pListenerArgs->m_hHandles[SOCKET_MUTEX]);
                WSAResetEvent(pListenerArgs->m_hHandles[READ_EVENT]);
                if (S_OK != hResult)
                {
                    DEBUG_PRINT("ListenThreadRecvPacket failed");
                    goto FAIL;
                }
			}
            else if (WAIT_OBJECT_0 + SYNC_EVENT == dwReadReady)
            {
                continue;
            }
            else
			{
				DEBUG_ERROR("WaitForSingleObject failed");
                goto FAIL;
            }
		}
        else if (WAIT_OBJECT_0 + SYNC_EVENT == dwReadReady)
        {
            continue;
		}
		else
        {
            DEBUG_WSAERROR("WaitForSingleObject failed: ReadEvent");
            goto FAIL;
		}

		// Message properly received from server
		if ((ChatMsg.iType == TYPE_CHAT) &&
			(ChatMsg.iSubType == STYPE_EMPTY) &&
			(ChatMsg.iOpcode == OPCODE_RES))
		{
			CustomStringPrintTwo(ChatMsg.pszDataOne, ChatMsg.pszDataTwo,
				ChatMsg.wLenOne, ChatMsg.wLenTwo);

			if (FALSE == PacketHeapFree(&ChatMsg))
			{
				DEBUG_ERROR("PacketHeapFree failed");
                goto FAIL;
			}
		}
		else
		{
			CustomConsoleWrite(L"Invalid message packet received from server."
				"\n", 46);
		}
	}

	goto EXIT;
FAIL:
    InterlockedExchange((PLONG)&g_bClientState, STOP);
    SetEvent(g_hShutdownEvent);
EXIT:
    return;
}

//End of file
