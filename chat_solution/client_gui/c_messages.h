/*****************************************************************//**
 * \file   c_messages.h
 * \brief
 *
 * \author chris
 * \date   September 2024
 *********************************************************************/
#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>

#include "c_shared.h"
#include "Messages.h"

 //NOTE: Change according to needs (units = milliseconds)
 //NOTE: Also defined in c_shared.h
#ifndef LISTEN_TIMEOUT
#define LISTEN_TIMEOUT 5000
#endif

 //NOTE: Custom message lengths section
#define MESSAGE_HDR_1_LEN 73

VOID PrintFailurePacket(INT8 wRejectCode);

VOID CustomStringPrintTwo(PWSTR pszStrOne,
                          PWSTR pszStrTwo,
                          WORD  wLen1,
                          WORD  wLen2);

HRESULT SendPacket(SOCKET RecvSock,
                   INT8   iType,
                   INT8   iSubType,
                   INT8   iOpcode,
                   WORD   wLenOne,
                   WORD   wLenTwo,
                   PWSTR  pszDataOne,
                   PWSTR  pszDataTwo);

BOOL PacketHeapFree(PCHATMSG pChatMsg);

HRESULT SocketPeek(SOCKET wsaSocket);

HRESULT BlockingRecv(WSAEVENT hReadEvent,
                     SOCKET   wsaSocket,
                     LPWSABUF pwsaBuffer,
                     DWORD    dwBufferCount,
                     PDWORD   pdwBytesReceived,
                     PDWORD   pdwFlags);

HRESULT ClientRecvPacket(SOCKET   RecvSock,
                         PCHATMSG pChatMsg,
                         WSAEVENT hReadEvent);

HRESULT ListenThreadRecvPacket(SOCKET RecvSock, PCHATMSG pChatMsg);

 //End of file
