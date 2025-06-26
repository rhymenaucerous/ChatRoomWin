/*****************************************************************//**
 * \file   s_shared.h
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once

//NOTE: s_shared.h will include the following libraries:
//		They will not need to be added to each server source/heaeder file.
//		The modular libraries are the same as in the client - if any changes
//		are made, they should be made to both. So we'll keep them in the client
//		folder.
#include "../hashtable/hashtable.h"
#include "../linkedlist/linkedlist.h"
#include "../networking/networking.h"
#include "Messages.h"
#include "Queue.h"

#define BUFF_SIZE 1024

//NOTE: Design decision for reconsideration later.
#define MAX_UNAME_LEN 10

//NOTE: This value is chosen based on the default buffer size - 1024.
//BUFF_SIZE - (MAX_UNAME_LEN + max command len + Two spaces + terminating 0)
//1024 - (10 + 5 + 2 + 1)
#define MAX_MSG_LEN_CHAT 1006

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

#ifndef CUSTOM_MACROS
#define CUSTOM_MACROS

#define NO_OPTION   0
#define MAX_MSG_LEN 256

#ifndef SINGLE_BUFFER
#define SINGLE_BUFFER 1
#endif

typedef enum
{
    SUCCESS               = 0,  // Operation successful
    ERR_INVALID_PARAM     = 1,  // Invalid parameter passed
    ERR_MEMORY_ALLOCATION = 2,  // Memory allocation failure
    ERR_FILE_NOT_FOUND    = 3,  // File not found
    ERR_ACCESS_DENIED     = 4,  // Permission denied
    ERR_TIMEOUT           = 5,  // Operation timed out
    ERR_SIGNATURE         = 6,  // invalid signature
    ERR_CRYPTO            = 7,  // ntstatus error desribes issue from bcrypt
    ERR_MEM_FREE          = 8,  // error with heap freeing
    ERR_SEND              = 9,  // error with heap freeing
    ERR_RECV              = 10, // error with heap freeing
    ERR_SURVEY            = 11, // error with heap freeing
    ERR_PERM              = 12, // error with heap freeing
    ERR_GENERIC           = 100 // Generic error
} RETURNTYPE;

#ifdef _DEBUG
#pragma warning(disable : 4996) // Disable warning C4996 (deprecated functions)
#define DEBUG_PRINT(fmt, ...)                                                  \
    do                                                                         \
    {                                                                          \
        fprintf(stderr, "DEBUG: %s(): Line %d: " fmt "\n", __func__, __LINE__, \
                __VA_ARGS__);                                                  \
    } while (0)
#define DEBUG_ERROR(fmt, ...)                                                  \
    do                                                                         \
    {                                                                          \
        DWORD error_code = GetLastError();                                     \
        char  error_message[256];                                              \
        FormatMessageA(                                                        \
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,  \
            error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),             \
            error_message, sizeof(error_message), NULL);                       \
        fprintf(stderr, "DEBUG: %s(): Line %d:\nError %lu: %sNote: " fmt "\n", \
                __func__, __LINE__, error_code, error_message, __VA_ARGS__);   \
    } while (0)
#define DEBUG_ERROR_SUPPLIED(error_code, fmt, ...)                             \
    do                                                                         \
    {                                                                          \
        char error_message[256];                                               \
        FormatMessageA(                                                        \
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,  \
            error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),             \
            error_message, sizeof(error_message), NULL);                       \
        fprintf(stderr, "DEBUG: %s(): Line %d:\nError %lu: %sNote: " fmt "\n", \
                __func__, __LINE__, error_code, error_message, __VA_ARGS__);   \
    } while (0)
#define DEBUG_WSAERROR(fmt, ...)                                               \
    do                                                                         \
    {                                                                          \
        int  wsa_error_code = WSAGetLastError();                               \
        char error_message[256];                                               \
        FormatMessageA(                                                        \
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,  \
            wsa_error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),         \
            error_message, sizeof(error_message), NULL);                       \
        fprintf(stderr, "DEBUG: %s(): Line %d:\nError %d: %sNote: " fmt "\n",  \
                __func__, __LINE__, wsa_error_code, error_message,             \
                __VA_ARGS__);                                                  \
    } while (0)
#define CUSTOM_PRINT(fmt, ...)                                                 \
    do                                                                         \
    {                                                                          \
        fprintf(stderr, "CUSTOM: %s(): Line %d: " fmt "\n", __func__,          \
                __LINE__, __VA_ARGS__);                                        \
    } while (0)
#else
#define DEBUG_PRINT(fmt, ...)                                                  \
    do                                                                         \
    {                                                                          \
    } while (0)
#define DEBUG_ERROR(fmt, ...)                                                  \
    do                                                                         \
    {                                                                          \
    } while (0)
#define DEBUG_WSAERROR(fmt, ...)                                               \
    do                                                                         \
    {                                                                          \
    } while (0)
#define CUSTOM_PRINT(fmt, ...)                                                 \
    do                                                                         \
    {                                                                          \
    } while (0)
#endif

/**
 * @brief Securely frees memory by zeroing it before deallocation.
 *
 * @param hHeap Handle to the heap from which the memory was allocated
 * @param dwFlags Heap free flags
 * @param pMem Pointer to the memory block to be freed
 * @param dwNumBytes Size of the memory block in bytes
 * @return BOOL - TRUE if successful, FALSE if failed
 */
static inline VOID ZeroingHeapFree(HANDLE hHeap,
                                   DWORD  dwFlags,
                                   PVOID *ppMem,
                                   DWORD  dwNumBytes)
{
    PVOID pMem = NULL;

    if (NULL != ppMem)
    {
        pMem = *ppMem;
        SecureZeroMemory(pMem, dwNumBytes);
        HeapFree(hHeap, dwFlags, pMem);
        *ppMem = NULL;
    }
}

#endif // CUSTOM_MACROS

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
