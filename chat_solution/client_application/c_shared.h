/*****************************************************************//**
 * \file   shared.h
 * \brief  
 * 
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once
#include "pch.h"

//NOTE: Shared.h will include the following libraries:
//		They will not need to be added to each client file.
#include "LinkedList.h"
#include "HashTable.h"
#include "Networking.h"
#include "Messages.h"

//NOTE: Change according to needs (units = milliseconds)
//NOTE: Also defined in c_messages.h
#ifndef LISTEN_TIMEOUT
#define LISTEN_TIMEOUT 1000
#endif

//NOTE: Design decision for reconsideration later.
#define MAX_UNAME_LEN 10

//NOTE: This value is chosen based on the default buffer size - 1024.
//BUFF_SIZE - (MAX_UNAME_LEN + max command len + Two spaces + terminating 0)
//1024 - (10 + 5 + 2 + 1)
#define MAX_MSG_LEN 1006

//NOTE: MSG lengths to avoid magic numbers.
#define MSG_1_LEN 28
#define MSG_2_LEN 11
#define MSG_3_LEN 13
#define MSG_4_LEN 14
#define MSG_5_LEN 1
#define MSG_6_LEN 18
#define MSG_7_LEN 28
#define MSG_8_LEN 302
#define MSG_9_LEN 42
#define MSG_10_LEN 296


//Macros for printing client usage info
#define PRINT_HELP 1
#define DONT_PRINT_HELP 0


typedef struct CLIENTCHATARGS {
	PWSTR  m_pszConnectIP;
	DWORD  m_dwConnectPort;
	PWSTR  m_pszConnectPort;
	PWSTR  m_pszClientName;
	SIZE_T m_szNameLength;
} CLIENTCHATARGS, * PCLIENTCHATARGS;

//NOTE: macros defining which handle is which index in the array.
#define NUM_HANDLES 4
#define SOCKET_MUTEX 1
#define READ_EVENT 0

//WARNING: Thread print functions lock and release STDOUT/STDERR custom
//mutexes.
#define STD_OUT_MUTEX 2
#define STD_ERR_MUTEX 3

//NOTE: Used in the c_srv_listen's ListenForChats(). Number of handles that
// waitformultiplehandles() will wait for. Only access the first two handles.
#define NUM_WAIT_HANDLES 2

//NOTE: Lengths of commands
#define CMD_1_LEN 4
#define CMD_2_LEN 5

typedef struct LISTENERARGS {
	SOCKET m_ServerSocket;
	HANDLE m_hHandles[NUM_HANDLES];
}LISTENERARGS, * PLISTENERARGS;

//End of file
