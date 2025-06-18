#pragma once

#include <Windows.h>
#include <stdio.h>

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
static inline BOOL ZeroingHeapFree(HANDLE hHeap,
                                   DWORD  dwFlags,
                                   PVOID  pMem,
                                   DWORD  dwNumBytes)
{
    BOOL bReturn = FALSE;

    SecureZeroMemory(pMem, dwNumBytes);

    bReturn = HeapFree(hHeap, dwFlags, pMem);
    if (0 == bReturn)
    {
        DEBUG_ERROR("Failed to free memory");
    }

    return bReturn;
}

#endif // CUSTOM_MACROS

typedef struct LINKEDLISTNODE
{
    PVOID                  m_pData;
    struct LINKEDLISTNODE *m_pNext;
} LINKEDLISTNODE, *PLINKEDLISTNODE;

typedef struct LINKEDLIST
{
    struct LINKEDLISTNODE *m_pHead;
    struct LINKEDLISTNODE *m_pTail;
    WORD                   m_wSize; // Max size is 65535.
} LINKEDLIST, *PLINKEDLIST, **PPLINKEDLIST;

RETURNTYPE
LinkedListInit(PPLINKEDLIST ppLinkedList);

RETURNTYPE
LinkedListInsert(PLINKEDLIST pLinkedList, PVOID pData, WORD wIndex);

PVOID
LinkedListRemove(PLINKEDLIST pLinkedList,
                 WORD        wIndex,
                 VOID (*pfnFreeFunction)(PVOID));

PVOID
LinkedListReturn(PLINKEDLIST pLinkedList, WORD wIndex);

RETURNTYPE
LinkedListDestroy(PLINKEDLIST pLinkedList, VOID (*pfnFreeFunction)(PVOID));

// NOTE: Temporary function for testing.
VOID PrintList(PLINKEDLIST pLinkedList);

// End of file
