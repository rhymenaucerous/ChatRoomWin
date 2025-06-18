#pragma once
/*****************************************************************//**
 * \file   Networking.h
 * \brief  
 * 
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once
#include "pch.h"

#define SRV_BACKLOG 100
//The winsock major and minor version are used to hint at which winsock
//version will be used.
#define WS_MAJ_VER 2
#define WS_MIN_VER 2

#define ACCEPT_TIMEOUT 3000

//NOTE: MSG lengths to avoid magic numbers.
#define N_MSG_1_LEN 19
#define N_MSG_2_LEN 17
#define N_MSG_3_LEN 18

volatile BOOL g_bServerState;
volatile BOOL g_bClientState;

WORD
NetSetUp();

SOCKET
NetListen(PWSTR pszAddress, PWSTR pszPort);

//WARNING: g_bServerRun needs to be defined as CONTINUE in main
//		   code. Will need to have system interrupt prcoess defined
//         to handle server shutdown gracefully.
SOCKET
NetAccept(SOCKET ListenSocket);

SOCKET
NetConnect(PWSTR pszAddress, PWSTR pszPort, LPWSABUF lpCallerData,
	LPWSABUF lpCalleeData);

VOID
NetCleanup(SOCKET SocketFileDescriptor, SOCKET ClientFileDescriptor);

//End of file
