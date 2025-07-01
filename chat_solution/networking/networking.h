#pragma once

// Used for WSABUF enable
#include <WinSock2.h>
#include <WS2tcpip.h>

// Always used here
#include <Windows.h>
#include <stdio.h>

#define SRV_BACKLOG 100

// The winsock major and minor version are used to hint at which winsock
// version will be used.
#define WS_MAJ_VER 2
#define WS_MIN_VER 2

#define ACCEPT_TIMEOUT 3000

#define STOP     0
#define CONTINUE 1

// Maximums for strings: IPv4 or IPv6 compatable.
#define HOST_MAX_STRING                                                        \
    40 // Max IP length is (IPv6) 40 ->
       // 7 colons + 32 hexadecimal digits + terminator.

#define PORT_MAX_STRING                                                        \
    6 // Only numeric services allowed - max length of
      // 65535 is 5 + terminator.

// IP length + Port length - 1 (one less terminator) + 2 (: designator in code).
#define ADDR_MAX_STRING (HOST_MAX_STRING + PORT_MAX_STRING + 1)

// Designate message types.
#define SRV_MSG  0
#define CLNT_MSG 1

// Every wait will use the overall shutdown handle. So the number of handles
// will be orginal handle + shutdown handle = 2.
#define NUM_HANDLES_N   2
#define SHUTDOWN_HANDLE 0
#define SOCKET_EVENT    1

// Macros for netcleanup()
#define DONT_CLEAN 0
#define DO_CLEAN   1

// Specific to vardorvis client
#ifndef MAX_MSG_SIZE
#define MAX_MSG_SIZE 1400
#endif

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
#define DEBUG_ERROR_SUPPLIED(fmt, ...)                                                 \
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
                                   PVOID* ppMem,
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

/**
 * @brief Initializes the Winsock library with the required version.
 *
 * This function calls WSAStartup to initialize the Winsock library with the
 * version specified by WS_MAJ_VER and WS_MIN_VER. It verifies that the
 * requested version is supported by the system.
 *
 * @return RETURNTYPE - SUCCESS (0) if initialization successful, ERR_GENERIC
 * (100) otherwise
 */
RETURNTYPE
NetSetUp();

/**
 * @brief Creates a socket and binds it to the specified address and port for
 * listening.
 *
 * @param pszAddress - Pointer to a wide string containing the address to bind
 * to
 * @param pszPort - Pointer to a wide string containing the port to bind to
 * @return SOCKET - The socket file descriptor if successful, INVALID_SOCKET
 * otherwise
 */
SOCKET
NetListen(PWSTR pszAddress, PWSTR pszPort);

/**
 * @brief Accepts an incoming connection on a listening socket.
 *
 * @param ListenSocket - The socket file descriptor of the listening socket
 * @param bServerState - A volatile boolean indicating the server's state
 * (CONTINUE or STOP)
 * @param hShutdown - Handle to an event that signals when to stop accepting
 * connections
 * @return SOCKET - The socket file descriptor of the accepted connection if
 * successful, INVALID_SOCKET otherwise
 */
SOCKET
NetAccept(SOCKET ListenSocket, volatile BOOL bServerState, HANDLE hShutdown);

/**
 * @brief Establishes a connection to a server at the specified address and
 * port.
 *
 * @param pszAddress - Pointer to a wide string containing the server address
 * @param pszPort - Pointer to a wide string containing the server port
 * @return SOCKET - The socket file descriptor if successful, INVALID_SOCKET
 * otherwise
 */
SOCKET
NetConnect(PWSTR pszAddress, PWSTR pszPort);

/**
 * Peeks on the specified socket to see if more data exists.
 *
 * \param TargetSocket specified socket.
 * \return true if more data does exist and false if not.
 */
BOOL IsThereMoreData(SOCKET TargetSocket);

/**
 * @brief Closes socket connections and cleans up Winsock resources.
 *
 * @param SocketFileDescriptor - The socket file descriptor to close
 * @param wClean - Designator to clean up WSA or not
 * @return VOID
 */
VOID NetCleanup(SOCKET SocketFileDescriptor, WORD wClean);

/**
 * @brief Callback function for thread pool work items. Used to support
 * unit testing.
 *
 * @param Instance - Pointer to a TP_CALLBACK_INSTANCE structure
 * @param pParam - Pointer to the parameter passed to the thread pool
 * @param pWork - Pointer to the TP_WORK structure
 * @return VOID
 */
VOID CALLBACK WorkCallback(PTP_CALLBACK_INSTANCE Instance,
                           PVOID                 pParam,
                           PTP_WORK              pWork);

// End of file
