/*****************************************************************//**
 * \file   c_messages.h
 * \brief  
 * 
 * \author chris
 * \date   September 2024
 *********************************************************************/
#pragma once
#include "pch.h"
#include "c_shared.h"

 //NOTE: Change according to needs (units = milliseconds)
 //NOTE: Also defined in c_shared.h
#ifndef LISTEN_TIMEOUT
#define LISTEN_TIMEOUT 5000
#endif

 //NOTE: Custom message lengths section
#define MESSAGE_HDR_1_LEN 73

VOID
ThreadPrintFailurePacket(HANDLE hStdOut, INT8 wRejectCode);

VOID
CustomStringPrintTwo(PWSTR pszStrOne, PWSTR pszStrTwo, WORD wLen1, WORD wLen2,
	HANDLE hStdErr, HANDLE hStdOut);

HRESULT
SendPacket(HANDLE hStdErr, SOCKET RecvSock, INT8 iType, INT8 iSubType,
	INT8 iOpcode, WORD wLenOne, WORD wLenTwo, PWSTR pszDataOne,
	PWSTR pszDataTwo);

BOOL
PacketHeapFree(PCHATMSG pChatMsg);

HRESULT
SocketPeek(HANDLE hStdErr, SOCKET wsaSocket);

HRESULT
BlockingRecv(WSAEVENT hReadEvent, HANDLE hStdErr, HANDLE hStdOut,
	SOCKET wsaSocket, LPWSABUF pwsaBuffer, DWORD dwBufferCount,
	PDWORD pdwBytesReceived, PDWORD pdwFlags);

HRESULT
ClientRecvPacket(HANDLE hStdErr, HANDLE hStdOut, SOCKET RecvSock,
	PCHATMSG pChatMsg, WSAEVENT hReadEvent);

HRESULT
ListenThreadRecvPacket(HANDLE hStdErr, SOCKET RecvSock,
	PCHATMSG pChatMsg);

 //End of file
