/*****************************************************************//**
 * \file   s_shared.h
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once
#include "pch.h"

//NOTE: s_shared.h will include the following libraries:
//		They will not need to be added to each server source/heaeder file.
//		The modular libraries are the same as in the client - if any changes
//		are made, they should be made to both. So we'll keep them in the client
//		folder.
#include "../client_application/HashTable.h"
#include "../client_application/HashTable.h"
#include "../client_application/Networking.h"
#include "../client_application/Messages.h"
#include "../client_application/Queue.h"

//NOTE: Design decision for reconsideration later.
#define MAX_UNAME_LEN 10

//NOTE: This value is chosen based on the default buffer size - 1024.
//BUFF_SIZE - (MAX_UNAME_LEN + max command len + Two spaces + terminating 0)
//1024 - (10 + 5 + 2 + 1)
#define MAX_MSG_LEN 1006

//NOTE: Used to designated completion key value for shutting down the worker
//threads.
#define IOCP_SHUTDOWN 0

//NOTE: The following couple of lines used to define custom HRESULT values.
// Define custom facility code (codes 0x0000 to 0x01FF are reserved for
// COM-defined codes and 0x0200-0xFFFF are recomended to be used)
//REF:https://learn.microsoft.com/en-us/windows/win32/com/
// codes-in-facility-itf?redirectedfrom=MSDN
#define FACILITY_SERVER 0x200

//NOTE: Defining exit codes for custom HRESULT values.
#define SERVER_SHUTDOWN 0x0001
#define CLIENT_SHUTDOWN 0x0002
#define NON_FATAL_ERR_CODE 0x0003
#define HEADER_SIZE_PACKET_CODE 0x0003

//NOTE: defined according to msdn standards.
#define CUSTOMER_DEFINED_BIT 0x20000000

//NOTE: Custom HRESULT error values.
#define SRV_SHUTDOWN_ERR (MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SERVER, \
SERVER_SHUTDOWN) |  CUSTOMER_DEFINED_BIT)
#define CLIENT_REMOVE_ERR (MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SERVER, \
CLIENT_SHUTDOWN) |  CUSTOMER_DEFINED_BIT)
#define NON_FATAL_ERR (MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SERVER, \
NON_FATAL_ERR_CODE) |  CUSTOMER_DEFINED_BIT)

//NOTE: Custom HRESULT success values.
#define HEADER_SIZE_PACKET (MAKE_HRESULT(SEVERITY_SUCCESS, \
 FACILITY_SERVER, HEADER_SIZE_PACKET_CODE) |  CUSTOMER_DEFINED_BIT)

//NOTE: macros defining which handle is which index in the array.
//WARNING: Thread print functions lock and release STDOUT/STDERR custom
//mutexes.
#define NUM_HANDLES 3
#define NUM_HANDLES_USER 4
#define STD_OUT_MUTEX 0
#define STD_ERR_MUTEX 1
#define SEND_MUTEX 2 //NOTE: Will only be a part of USER struct
#define SEND_DONE_EVENT 3 //NOTE: Will only be a part of USER struct
#define IOCP_HANDLE 2 //NOTE: Will only be a part of SERVERCHATARGS struct

typedef struct SERVERCHATARGS {
	PWSTR   m_pszBindIP;
	DWORD   m_dwBindPort;
	PWSTR   m_pszBindPort;
	DWORD   m_dwMaxClients;
	DWORD   m_dwThreadCount;
	HANDLE	m_haSharedHandles[NUM_HANDLES];
	SOCKET  m_ListenSocket;
	PHANDLE m_phThreads;
} SERVERCHATARGS, * PSERVERCHATARGS;

#define NUM_HANDLES_USERS 6
#define USERS_WRITE_MUTEX 2
#define USERS_READ_SEMAPHORE 3
#define READERS_DONE_EVENT 4
#define NEW_USERS_MUTEX 5

typedef struct USERS {

	PHASHTABLE    m_pNewUsersTable;
	PHASHTABLE    m_pUsersHTable;
	HANDLE	      m_haUsersHandles[NUM_HANDLES_USERS];
	LONG volatile m_plReaderCount;
	DWORD	      m_dwMaxClients; //We'll differentiate users and
							   //clients later, for now it's both.
	//TODO: We'll potentially add the sessionID table later.
	/*PHASHTABLE m_pSessionsTable;
	HANDLE	   m_hSessionHTableMutex;*/
} USERS, * PUSERS;

//NOTE: Macros supporting analysis of completion keys by worker threads.
#define RECV_OP 0
#define SEND_OP 1

//NOTE: states for the client:
#define UN_NEGOTIATED 0
#define NEGOTIATED 1

//NOTE: Destruction states
#define DESTROYING 0
#define NOT_DESTROYING 1

//NOTE: The Msg Holder struct contains state information about packets
// received by the server. Enables the server to handle partial receives and
// partial sends during asychronous operations.
typedef struct MSGHOLDER {
	OVERLAPPED m_wsaOverlapped;
	WSABUF     m_wsaBuffer[THREE_BUFFERS];
	CHATMSG	   m_Header;
	WCHAR	   m_pBodyBufferOne[BUFF_SIZE + 1];
	WCHAR	   m_pBodyBufferTwo[BUFF_SIZE + 1];
	DWORD      m_dwBytestoMove;
	DWORD	   m_dwBytesMovedTotal;
	DWORD	   m_dwBytesMoved;
	DWORD	   m_dwFlags;
	INT8	   m_iOperationType;
} MSGHOLDER, *PMSGHOLDER;

//NOTE: The USER struct will be the IO Completion Key for waiting threads.
//NOTE: Doesn't not include hIOCP bc it will be the worker thread's only arg.
//NOTE: Stucture values all initialized to zero.
typedef struct USER {
	WCHAR          m_caUsername[MAX_UNAME_LEN + 1];
	WORD		   m_wUsernameLen;
	SOCKET	       m_ClientSocket;
	HANDLE         m_haSharedHandles[NUM_HANDLES_USER];
	WORD	       m_wNegotiatedState;
	LONG volatile  m_plSendOccuring;
	LONG volatile  m_plRecvOccuring;
	LONG volatile  m_plThreadsWaiting;
	LONG volatile  m_plBeingDestroyed;
	MSGHOLDER      m_RecvMsg;
	PQUEUE		   m_SendMsgQueue;
	PUSERS	       m_pUsers;
	//TODO: For potential, later, improvement/addition.
	//INT64			m_ilSessionID;
} USER, * PUSER;

//NOTE: Max clients set to 65535 - there are practicaly limits to processing
// and memory for the server's hardware.
// https://serverframework.com/asynchronousevents/2010/10/how-to-support-10000-
// concurrent-tcp-connections.html
#define MAX_CLIENTS 65535
#define MIN_CLIENTS 2

//NOTE: Most low/medium-end servers will have between 8-64 cores. The server's
// maximum and minimum thread counts are based on these values. The number of
// cores will be altered based on the maximum number of clients chosen by the
// user.
// https://community.fs.com/article/what-is-a-server-cpu.html
// https://www.servethehome.com/server-core-counts-going-supernova-by-q1-2025-
// intel-amd-arm-nvidia-ampere/
//NOTE: wmain thread will still be running and one or two other threads might
// pop up as well but matching threads and cores exactly isn't essential to 
// efficieny - sopme threads will probably have down time.
#define MAX_THREADS 64
#define THREADS_32 32
#define THREADS_16 16
#define MIN_THREADS 8

VOID
FreeMsg(PVOID pParam);

VOID
UserFreeFunction(PVOID pParam);

//End of file
