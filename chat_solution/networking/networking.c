#include "networking.h"

RETURNTYPE NetSetUp()
{
    RETURNTYPE Return = ERR_GENERIC;
    WORD       wVersionRequested;
    WSADATA    wsaData;

    wVersionRequested = MAKEWORD(WS_MAJ_VER, WS_MIN_VER);

    if (SUCCESS != WSAStartup(wVersionRequested, &wsaData))
    {
        DEBUG_WSAERROR("WSAStartup()");
        goto EXIT;
    }

    DEBUG_PRINT("Using winsock V%u.%u", LOBYTE(wsaData.wVersion),
                HIBYTE(wsaData.wVersion));

    if (LOBYTE(wsaData.wVersion) != WS_MAJ_VER ||
        HIBYTE(wsaData.wVersion) != WS_MIN_VER)
    {
        DEBUG_ERROR("Ws2_32.lib doesn't support Winsock 2.2");
        goto WSACLEAN;
    }

    Return = SUCCESS;
    goto EXIT;
WSACLEAN:
    WSACleanup();
EXIT:
    return Return;
}

/**
 * Prints an address to the console. Will specify if NetListen or NetConnect
 * is calling.
 *
 * \param pAddrInfo pointer to the address information structure.
 * \param wMsg message type.
 * \return VOID.
 */
static VOID PrintAddrInfo(const PADDRINFOW pAddrInfo, WORD wMsg)
{
    WCHAR         caString[ADDR_MAX_STRING] = {0};
    PVOID         pAddr                     = NULL;
    INT           iPort                     = 0;
    PSOCKADDR_IN  pIPv4                     = NULL;
    PSOCKADDR_IN6 pIPv6                     = NULL;

    if (NULL == pAddrInfo)
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    if (AF_INET == pAddrInfo->ai_family) // IPv4
    {
        pIPv4 = (PSOCKADDR_IN)pAddrInfo->ai_addr;
        pAddr = &(pIPv4->sin_addr);
        iPort = ntohs(pIPv4->sin_port);
        InetNtopW(AF_INET, pAddr, caString, ADDR_MAX_STRING);
    }
    else if (AF_INET6 == pAddrInfo->ai_family) // IPv6
    {
        pIPv6 = (PSOCKADDR_IN6)pAddrInfo->ai_addr;
        pAddr = &(pIPv6->sin6_addr);
        iPort = ntohs(pIPv6->sin6_port);
        InetNtopW(AF_INET6, pAddr, caString, ADDR_MAX_STRING);
    }
    else
    {
        DEBUG_PRINT("Unknown ai_family");
        goto EXIT;
    }

    if (SRV_MSG == wMsg)
    {
        DEBUG_PRINT("Server bound to: %S:%d", caString, iPort);
    }
    else if (CLNT_MSG == wMsg)
    {
        DEBUG_PRINT("Client connected to: %S:%d", caString, iPort);
    }
    else
    {
        DEBUG_PRINT("Unknown message type");
    }

EXIT:
    return;
}

SOCKET
NetListen(PWSTR pszAddress, PWSTR pszPort)
{
    ADDRINFOW  Hints;
    PADDRINFOW pResult              = NULL;
    PADDRINFOW pTempResult          = NULL;
    SOCKET     SocketFileDescriptor = INVALID_SOCKET;
    INT        iErrorTracker        = 0;
    INT        iOptval              = TRUE;

    if (NULL == pszAddress || NULL == pszPort)
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    ZeroMemory(&Hints, sizeof(Hints));
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    Hints.ai_flags    = AI_NUMERICSERV | AI_PASSIVE;

    if (SUCCESS != GetAddrInfoW(pszAddress, pszPort, &Hints, &pResult))
    {
        DEBUG_WSAERROR("GetAddrInfoW()");
        goto EXIT;
    }

    for (pTempResult = pResult; pTempResult != NULL;
         pTempResult = pTempResult->ai_next)
    {
        SocketFileDescriptor =
            socket(pTempResult->ai_family, pTempResult->ai_socktype,
                   pTempResult->ai_protocol);

        if ((INVALID_SOCKET == SocketFileDescriptor) ||
            (pTempResult->ai_addrlen > INT_MAX))
        {
            continue;
        }

        iErrorTracker = setsockopt(SocketFileDescriptor, SOL_SOCKET,
                                   SO_REUSEADDR, (PCHAR)&iOptval, sizeof(INT));

        if (EXIT_SUCCESS != iErrorTracker)
        {
            DEBUG_WSAERROR("setsockopt()");
            continue;
        }

        // NOTE: converstion to int is safe, given the check on line 101.
        iErrorTracker = bind(SocketFileDescriptor, pTempResult->ai_addr,
                             (INT)pTempResult->ai_addrlen);

        if (EXIT_SUCCESS != iErrorTracker)
        {
            DEBUG_WSAERROR("setsockopt()");
            continue;
        }

        iErrorTracker = listen(SocketFileDescriptor, SRV_BACKLOG);

        if (EXIT_SUCCESS != iErrorTracker)
        {
            DEBUG_WSAERROR("listen()");
            continue;
        }

        break;
    }

    // NOTE:Print information about IP/port bound to console for user.
#ifdef _DEBUG
    PrintAddrInfo(pTempResult, SRV_MSG);
#endif

    FreeAddrInfoW(pResult);
    if (NULL == pTempResult)
    {
        DEBUG_PRINT("No valid sockets");
        SocketFileDescriptor = INVALID_SOCKET;
    }

EXIT:
    return SocketFileDescriptor;
}

/**
 * Prints the address of the client connected to the server.
 *
 * \param pSockaddrStorage pointer to the sockaddr_storage structure.
 * \return VOID.
 */
static VOID PrintSockaddrStorage(const PSOCKADDR_STORAGE pSockaddrStorage)
{
    WCHAR         caString[ADDR_MAX_STRING] = {0};
    PVOID         pAddr                     = NULL;
    INT           iPort;
    PSOCKADDR_IN  pIPv4 = NULL;
    PSOCKADDR_IN6 pIPv6 = NULL;

    if (NULL == pSockaddrStorage)
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    if (AF_INET == pSockaddrStorage->ss_family) // IPv4
    {
        pIPv4 = (PSOCKADDR_IN)pSockaddrStorage;
        pAddr = &(pIPv4->sin_addr);
        iPort = ntohs(pIPv4->sin_port);
        InetNtopW(AF_INET, pAddr, caString, ADDR_MAX_STRING);
    }
    else if (AF_INET6 == pSockaddrStorage->ss_family) // IPv6
    {
        pIPv6 = (PSOCKADDR_IN6)pSockaddrStorage;
        pAddr = &(pIPv6->sin6_addr);
        iPort = ntohs(pIPv6->sin6_port);
        InetNtopW(AF_INET6, pAddr, caString, ADDR_MAX_STRING);
    }
    else
    {
        DEBUG_PRINT("Unknown ai_family");
        goto EXIT;
    }

    DEBUG_PRINT("Connection from: %S:%d", caString, iPort);

EXIT:
    return;
}

SOCKET
NetAccept(SOCKET ListenSocket, volatile BOOL bServerState, HANDLE hShutdown)
{
    SOCKADDR_STORAGE ClientAddr            = {0};
    DWORD            dwAddrLen             = sizeof(SOCKADDR_STORAGE);
    SOCKET           ClientFileDescriptor  = INVALID_SOCKET;
    DWORD            dwWaitResult          = WAIT_FAILED;
    INT              iError                = 0;
    HANDLE           haEvents[NUM_HANDLES] = {0};
    WORD             wNumHandles           = NUM_HANDLES;

    if (NULL == hShutdown)
    {
        wNumHandles -= 1;
    }

    WSAEVENT wsaAcceptSocket = WSACreateEvent();
    if (WSA_INVALID_EVENT == wsaAcceptSocket)
    {
        DEBUG_WSAERROR("WSACreateEvent()");
        goto EXIT;
    }

    if (SOCKET_ERROR ==
        WSAEventSelect(ListenSocket, wsaAcceptSocket, FD_ACCEPT))
    {
        DEBUG_WSAERROR("WSAEventSelect()");
        goto EXIT;
    }

    haEvents[0] = wsaAcceptSocket;
    haEvents[1] = hShutdown;

    while (InterlockedCompareExchange(&bServerState, CONTINUE, CONTINUE))
    {
        dwWaitResult = WaitForMultipleObjects(wNumHandles, haEvents, FALSE,
                                              ACCEPT_TIMEOUT);
        if (WAIT_OBJECT_0 == dwWaitResult)
        {
            ClientFileDescriptor =
                WSAAccept(ListenSocket, (struct sockaddr *)(&ClientAddr),
                          (PINT)&dwAddrLen, NO_OPTION, NO_OPTION);

            if (INVALID_SOCKET == ClientFileDescriptor)
            {
                iError = WSAGetLastError();

                // NOTE: Any of these errors don't necessitate server shutdown
                if ((WSAEACCES == iError) || (WSAECONNREFUSED == iError) ||
                    (WSAECONNRESET == iError) || (WSAEINTR == iError) ||
                    (WSAEINPROGRESS == iError) || (WSAEWOULDBLOCK == iError) ||
                    (WSATRY_AGAIN == iError))
                {
                    DEBUG_ERROR_SUPPLIED(iError, "WSAAccept()");
                    continue;
                }
                else
                {
                    DEBUG_ERROR_SUPPLIED(iError, "WSAAccept()");
                    goto EXIT;
                }
            }
            else
            {
                break;
            }
        }
        else if (WAIT_TIMEOUT == dwWaitResult)
        {
            // The function has timed out but will be reset.
            continue;
        }
        else
        {
            DEBUG_ERROR("WaitForSingleObject()");
            break;
        }
        break;
    }

    if (FALSE == WSACloseEvent(wsaAcceptSocket))
    {
        DEBUG_WSAERROR("WSACloseEvent()");
    }

    if (CONTINUE != bServerState)
    {
        goto CLOSE;
    }

#ifdef _DEBUG
    PrintSockaddrStorage(&ClientAddr);
#endif
    goto EXIT;
CLOSE:
    NetCleanup(ClientFileDescriptor, INVALID_SOCKET);
EXIT:
    return ClientFileDescriptor;
}

SOCKET
NetConnect(PWSTR pszAddress, PWSTR pszPort)
{
    ADDRINFOW  Hints;
    PADDRINFOW pResult              = NULL;
    PADDRINFOW pTempResult          = NULL;
    SOCKET     SocketFileDescriptor = INVALID_SOCKET;
    INT        iErrorTracker        = 0;
    INT        iStatus              = EAI_AGAIN;

    if (NULL == pszAddress || NULL == pszPort)
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    ZeroMemory(&Hints, sizeof(Hints));
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    Hints.ai_flags    = AI_NUMERICSERV;

    iStatus = GetAddrInfoW(pszAddress, pszPort, &Hints, &pResult);

    if (EXIT_SUCCESS != iStatus)
    {
        DEBUG_ERROR_SUPPLIED((DWORD)iStatus, "GetAddrInfoW()");
        goto EXIT;
    }

    for (pTempResult = pResult; pTempResult != NULL;
         pTempResult = pTempResult->ai_next)
    {
        SocketFileDescriptor =
            socket(pTempResult->ai_family, pTempResult->ai_socktype,
                   pTempResult->ai_protocol);

        if ((INVALID_SOCKET == SocketFileDescriptor) ||
            (pTempResult->ai_addrlen > INT_MAX))
        {
            continue;
        }

        // NOTE: converstion to int is safe, given the check on line 276.
        iErrorTracker =
            WSAConnect(SocketFileDescriptor, pTempResult->ai_addr,
                       (int)pTempResult->ai_addrlen, NULL, NULL, NULL, NULL);

        if (EXIT_SUCCESS != iErrorTracker)
        {
            DEBUG_WSAERROR("WSAConnect()");
            SocketFileDescriptor = INVALID_SOCKET;
            goto EXIT;
        }
        else
        {
            break;
        }

        break;
    }

    // NOTE:Print information about IP/port connected to, to console for user.
#ifdef _DEBUG
    PrintAddrInfo(pTempResult, CLNT_MSG);
#endif

    FreeAddrInfoW(pTempResult);
    if (NULL == pTempResult)
    {
        DEBUG_PRINT("No valid sockets");
        SocketFileDescriptor = INVALID_SOCKET;
    }

EXIT:
    return SocketFileDescriptor;
}

BOOL IsThereMoreData(SOCKET TargetSocket)
{
    BOOL   bReturn                 = FALSE;
    DWORD  dwBytesRecv             = 0;
    WSABUF pTestBuf[SINGLE_BUFFER] = {0};
    CHAR   pBufHolder[1]           = {0};
    DWORD  dwFlags                 = MSG_PEEK;
    pTestBuf->buf                  = pBufHolder;
    pTestBuf->len                  = 1;

    WSARecv(TargetSocket, pTestBuf, SINGLE_BUFFER, &dwBytesRecv, &dwFlags, NULL,
            NULL);

    if (0 == dwBytesRecv)
    {
        goto EXIT;
    }

    bReturn = TRUE;
EXIT:
    return FALSE;
}

VOID NetCleanup(SOCKET SocketFileDescriptor, WORD wClean)
{
    INT iErrorTracker = SOCKET_ERROR;

    if (INVALID_SOCKET != SocketFileDescriptor)
    {
        iErrorTracker = closesocket(SocketFileDescriptor);

        if (SOCKET_ERROR == iErrorTracker)
        {
            DEBUG_WSAERROR("closesocket()");
        }
    }

    if (wClean)
    {
        WSACleanup();
    }
}

VOID CALLBACK WorkCallback(PTP_CALLBACK_INSTANCE Instance,
                           PVOID                 pParam,
                           PTP_WORK              pWork)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(pWork);
    SOCKET       *pClientSocket = pParam;
    SOCKET        ListenSocket  = *pClientSocket; // First SOCKET in memory
    SOCKET        ClientSocket  = INVALID_SOCKET;
    volatile BOOL bState        = CONTINUE; // Used in place of server state

    ClientSocket = NetAccept(ListenSocket, bState, NULL); // Call NetAccept

    *pClientSocket = ClientSocket; // Store result in second SOCKET
}

// End of file
