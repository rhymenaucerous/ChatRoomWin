/*****************************************************************//**
 * \file   shared.h
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once

#include "..\networking\networking.h"

#include <Windows.h>
#include <stdio.h>

#define BUFF_SIZE 1024

//NOTE: Change according to needs (units = milliseconds)
//NOTE: Also defined in c_messages.h
#ifndef LISTEN_TIMEOUT
#define LISTEN_TIMEOUT 1000
#endif

//NOTE: Design decision for reconsideration later.
#define MAX_UNAME_LEN 10

#define NO_OPTION 0

//Macros for printing client usage info
#define PRINT_HELP 1
#define DONT_PRINT_HELP 0

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

typedef struct CLIENTCHATARGS {
	PWSTR  m_pszConnectIP;
	DWORD  m_dwConnectPort;
	PWSTR  m_pszConnectPort;
	PWSTR  m_pszClientName;
	SIZE_T m_szNameLength;
} CLIENTCHATARGS, * PCLIENTCHATARGS;

//NOTE: macros defining which handle is which index in the array.
#define NUM_HANDLES 5
#define READ_EVENT   0
#define SOCKET_MUTEX 1
#define SHUTDOWN_EVENT 2
// The user listener thread got input
#define ULISTEN_WAIT_FINISHED 3
// The user listerner thread completed actions and is listening again
#define ULISTEN_WAITING 4

//NOTE: Lengths of commands
#define CMD_1_LEN 4
#define CMD_2_LEN 5

typedef struct LISTENERARGS {
	SOCKET m_ServerSocket;
	HANDLE m_hHandles[NUM_HANDLES];
} LISTENERARGS, *PLISTENERARGS;

// CLIENT GUI ADDITIONS

extern HWND   hDisplayBox;
extern HWND   hInputBox;
extern HANDLE g_hInputReadyEvent;

//End of file
