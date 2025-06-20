#pragma once

#include "../linkedlist/linkedlist.h"

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
 * @return VOID
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

#define KEY_LENGTH   50
#define MIN_CAPACITY 7 // WARNING: Do not set to 0 or library will divide by 0.

// Once the load factor is above this set
// value, the hash table will be re-hashed.
// This allows for maximum efficiency when
// adding new values. Per wikipedia, optimal load
// factor for separate chaining hash tables is
// between 1 and 3.
#define LOAD_FACTOR_UPDATE 1.5

#define MAX_PRIME_IN_RANGE 65521 // Largest prime in unsigned 16-bit int range
#define DUPLICATE_KEY      2

typedef struct HASHTABLEENTRY
{
    PVOID m_pData;
    CHAR  m_caKey[KEY_LENGTH + 1];
    WORD  m_wKeyLen;
    VOID (*m_pfnFreeFunction)(PVOID);
} HASHTABLEENTRY, *PHASHTABLEENTRY;

typedef struct HASHTABLE
{
    WORD m_wSize;     // Max size is 65535.
    WORD m_wCapacity; // Number of rows in hash table
    DWORD (*m_pfnHashFunction)(PVOID);
    PPLINKEDLIST m_ppTable;
} HASHTABLE, *PHASHTABLE, **PPHASHTABLE;

BOOL IsPrime(WORD wValue);

WORD NextPrime(WORD wValue);

RETURNTYPE
HashTableInit(PPHASHTABLE ppHashTable,
              WORD        wCapacity,
              DWORD (*pfnHashFunction)(PVOID));

RETURNTYPE
HashTableNewEntry(PHASHTABLE pHashTable,
                  PVOID      pData,
                  PCHAR      pszKey,
                  WORD       wKeyLen);

PVOID
HashTableReturnEntry(PHASHTABLE pHashTable, PCHAR pszKey, WORD wKeyLen);

PVOID
HashTableDestroyEntry(PHASHTABLE pHashTable, PCHAR pszKey, WORD wKeyLen);

RETURNTYPE
HashTableDestroy(PHASHTABLE pHashTable, VOID (*pfnFreeFunction)(PVOID));

// End of file
