/*****************************************************************//**
 * \file   Networking.c
 * \brief  
 * 
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include "pch.h"
#include "Networking.h"

volatile BOOL g_bServerState = CONTINUE;
volatile BOOL g_bClientState = CONTINUE;

WORD
NetSetUp()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	INT iError;

	wVersionRequested = MAKEWORD(WS_MAJ_VER, WS_MIN_VER);

	iError = WSAStartup(wVersionRequested, &wsaData);

	if (EXIT_SUCCESS != iError)
	{
		PrintErrorSupplied((PCHAR)__func__, __LINE__, (DWORD)iError);
		return EXIT_FAILURE;
	}

	PWSTR pszVersionMsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		(ADDR_MAX_STRING + N_MSG_1_LEN) * sizeof(WCHAR));

	if (NULL == pszVersionMsg)
	{
		PrintError((PCHAR)__func__, __LINE__);
		WSACleanup();
		return EXIT_FAILURE;
	}

	HRESULT hrReturn = StringCchPrintfW(pszVersionMsg, (USIGN_MAX_DIGITS * 2
		+ N_MSG_2_LEN),
		L"Using winsock V%u.%u\n", LOBYTE(wsaData.wVersion),
		HIBYTE(wsaData.wVersion));

	if (S_OK != hrReturn)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "StringCchPrintfW()");
		MyHeapFree(GetProcessHeap(), NO_OPTION, pszVersionMsg,
			(USIGN_MAX_DIGITS * 2 + N_MSG_2_LEN) * sizeof(WCHAR));
		WSACleanup();
		return EXIT_FAILURE;
	}

	//NOTE: We don't need to copy the NULL terminator to the console (-1).
	HRESULT hResult = CustomConsoleWrite(pszVersionMsg, (USIGN_MAX_DIGITS * 2
		+ N_MSG_2_LEN));

	MyHeapFree(GetProcessHeap(), NO_OPTION, pszVersionMsg,
		(USIGN_MAX_DIGITS * 2 + N_MSG_2_LEN) * sizeof(WCHAR));

	if (S_OK != hResult)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "CustomConsoleWrite()");
		WSACleanup();
		return EXIT_FAILURE;
	}

	if (LOBYTE(wsaData.wVersion) != WS_MAJ_VER ||
		HIBYTE(wsaData.wVersion) != WS_MIN_VER)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "Ws2_32.lib doesn't support "
			"Winsock 2.2");
		WSACleanup();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static VOID
PrintAddrInfo(const PADDRINFOW pAddrInfo)
{
	if (NULL == pAddrInfo)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "input NULL");
		return;
	}
	
	WCHAR caString[ADDR_MAX_STRING] = { 0 };
	PVOID pAddr = NULL;
	INT iPort;

	if (AF_INET == pAddrInfo->ai_family)//IPv4
	{
		PSOCKADDR_IN pIPv4 = (PSOCKADDR_IN)(pAddrInfo->ai_addr);
		pAddr = &(pIPv4->sin_addr);
		iPort = ntohs(pIPv4->sin_port);
		InetNtopW(AF_INET, pAddr, caString, ADDR_MAX_STRING);
	}
	else if (AF_INET6 == pAddrInfo->ai_family)//IPv6
	{
		PSOCKADDR_IN6 pIPv6 = (PSOCKADDR_IN6)pAddrInfo->ai_addr;
		pAddr = &(pIPv6->sin6_addr);
		iPort = ntohs(pIPv6->sin6_port);
		InetNtopW(AF_INET6, pAddr, caString, ADDR_MAX_STRING);
	}
	else
	{
		CustomConsoleWrite(L"PrintAddrInfo(): Error printing bound socket.\n",
			46);
		return;
	}

	PWSTR pszAddrMsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		(ADDR_MAX_STRING + N_MSG_1_LEN) * sizeof(WCHAR));

	if (NULL == pszAddrMsg)
	{
		return;
	}

	HRESULT hrReturn = StringCchPrintfW(pszAddrMsg, (ADDR_MAX_STRING + N_MSG_1_LEN),
		L"Server bound to: %s:%d\n", caString, iPort);

	if (S_OK != hrReturn)
	{
		MyHeapFree(GetProcessHeap(), NO_OPTION, pszAddrMsg,
			(ADDR_MAX_STRING + N_MSG_1_LEN) * sizeof(WCHAR));
		return;
	}

	//NOTE: We don't need to copy the NULL terminator to the console (-1).
	CustomConsoleWrite(pszAddrMsg, (ADDR_MAX_STRING + N_MSG_3_LEN));
}

SOCKET
NetListen(PWSTR pszAddress, PWSTR pszPort)
{
	if (NULL == pszAddress || NULL == pszPort)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
		WSACleanup();
		return INVALID_SOCKET;
	}

	ADDRINFOW Hints;
	PADDRINFOW pResult = NULL;
	PADDRINFOW pTempResult = NULL;
	SOCKET SocketFileDescriptor = INVALID_SOCKET;
	INT iErrorTracker = 0;

	ZeroMemory(&Hints, sizeof(Hints));
	Hints.ai_socktype = SOCK_STREAM;
	Hints.ai_family = AF_INET;
	Hints.ai_protocol = IPPROTO_TCP;
	Hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;

	INT iStatus = GetAddrInfoW(pszAddress, pszPort, &Hints, &pResult);

	if (EXIT_SUCCESS != iStatus)
	{
		PrintErrorSupplied((PCHAR)__func__, __LINE__, (DWORD)iStatus);
		WSACleanup();
		return INVALID_SOCKET;
	}

	INT iOptval = TRUE;

	for (pTempResult = pResult; pTempResult != NULL;
		pTempResult = pTempResult->ai_next)
	{
		SocketFileDescriptor = socket(pTempResult->ai_family,
			pTempResult->ai_socktype, pTempResult->ai_protocol);
		
		if ((INVALID_SOCKET == SocketFileDescriptor) ||
			(pTempResult->ai_addrlen > INT_MAX))
		{
			continue;
		}

		iErrorTracker = setsockopt(SocketFileDescriptor, SOL_SOCKET,
			SO_REUSEADDR, (PCHAR)&iOptval, sizeof(INT));

		if (EXIT_SUCCESS != iErrorTracker)
		{
			PrintErrorWSA((PCHAR)__func__, __LINE__);
			continue;
		}

		//NOTE: converstion to int is safe, given the check on line 101.
		iErrorTracker = bind(SocketFileDescriptor, pTempResult->ai_addr,
			(INT)pTempResult->ai_addrlen);

		if (EXIT_SUCCESS != iErrorTracker)
		{
			PrintErrorWSA((PCHAR)__func__, __LINE__);
			continue;
		}

		iErrorTracker = listen(SocketFileDescriptor, SRV_BACKLOG);

		if (EXIT_SUCCESS != iErrorTracker)
		{
			PrintErrorWSA((PCHAR)__func__, __LINE__);
			continue;
		}

		break;
	}

	//NOTE:Print information about IP/port bound to console for user.
	PrintAddrInfo(pTempResult);

	FreeAddrInfoW(pResult);

	if (NULL == pTempResult)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "No valid sockets");
		return INVALID_SOCKET;
	}

	return SocketFileDescriptor;
}

static VOID
PrintSockaddrStorage(const PSOCKADDR_STORAGE pSockaddrStorage)
{
	WCHAR caString[ADDR_MAX_STRING] = { 0 };
	PVOID pAddr = NULL;
	INT iPort;

	if (AF_INET == pSockaddrStorage->ss_family)//IPv4
	{
		PSOCKADDR_IN pIPv4 = (PSOCKADDR_IN)pSockaddrStorage;
		pAddr = &(pIPv4->sin_addr);
		iPort = ntohs(pIPv4->sin_port);
		InetNtopW(AF_INET, pAddr, caString, ADDR_MAX_STRING);
	}
	else if (AF_INET6 == pSockaddrStorage->ss_family)//IPv6
	{
		PSOCKADDR_IN6 pIPv6 = (PSOCKADDR_IN6)pSockaddrStorage;
		pAddr = &(pIPv6->sin6_addr);
		iPort = ntohs(pIPv6->sin6_port);
		InetNtopW(AF_INET6, pAddr, caString, ADDR_MAX_STRING);
	}
	else
	{
		CustomConsoleWrite(L"PrintAddrInfo(): Error printing bound socket.\n",
			46);
		return;
	}

	PWSTR pszAddrMsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		(ADDR_MAX_STRING + N_MSG_1_LEN) * sizeof(WCHAR));

	if (NULL == pszAddrMsg)
	{
		return;
	}

	HRESULT hrReturn = StringCchPrintfW(pszAddrMsg, (ADDR_MAX_STRING + N_MSG_1_LEN),
		L"Connection from: %s:%d\n", caString, iPort);

	if (S_OK != hrReturn)
	{
		MyHeapFree(GetProcessHeap(), NO_OPTION, pszAddrMsg,
			(ADDR_MAX_STRING + N_MSG_1_LEN) * sizeof(WCHAR));
		return;
	}

	//NOTE: We don't need to copy the NULL terminator to the console (-1).
	CustomConsoleWrite(pszAddrMsg, (ADDR_MAX_STRING + N_MSG_3_LEN));
}

SOCKET
NetAccept(SOCKET ListenSocket)
{
	SOCKADDR_STORAGE ClientAddr = { 0 };
	DWORD dwAddrLen = sizeof(SOCKADDR_STORAGE);
	SOCKET ClientFileDescriptor = INVALID_SOCKET;

	WSAEVENT wsaAcceptSocket = WSACreateEvent();

	if (WSA_INVALID_EVENT == wsaAcceptSocket)
	{
		PrintErrorWSA((PCHAR)__func__, __LINE__);
		return INVALID_SOCKET;
	}

	INT iResult = WSAEventSelect(ListenSocket, wsaAcceptSocket, FD_ACCEPT);

	if (SOCKET_ERROR == iResult)
	{
		PrintErrorWSA((PCHAR)__func__, __LINE__);
		return INVALID_SOCKET;
	}

	DWORD dwWaitResult = WaitForSingleObject(wsaAcceptSocket, ACCEPT_TIMEOUT);

	while (CONTINUE == g_bServerState)
	{
		if (WAIT_OBJECT_0 == dwWaitResult)
		{
			ClientFileDescriptor = WSAAccept(ListenSocket,
				(struct sockaddr*)(&ClientAddr), (PINT)&dwAddrLen,
				NO_OPTION, NO_OPTION);

			if (INVALID_SOCKET == ClientFileDescriptor)
			{
				INT iError = WSAGetLastError();

				//NOTE: Any of these errors don't necessitate server shutdown
				if ((WSAEACCES == iError) || (WSAECONNREFUSED == iError) ||
					(WSAECONNRESET == iError) || (WSAEINTR == iError) ||
					(WSAEINPROGRESS == iError) || (WSAEWOULDBLOCK == iError)
					|| (WSATRY_AGAIN == iError))
				{
					PrintErrorSupplied((PCHAR)__func__, __LINE__, (DWORD)iError);
					continue;
				}
				else
				{
					PrintErrorSupplied((PCHAR)__func__, __LINE__, (DWORD)iError);
					return INVALID_SOCKET;
				}
			}
			else
			{
				break;
			}
		}
		else if (WAIT_TIMEOUT == dwWaitResult)
		{
			//The function has timed out but will be reset.
			dwWaitResult = WaitForSingleObject(wsaAcceptSocket, ACCEPT_TIMEOUT);
			continue;
		}
		else
		{
			PrintError((PCHAR)__func__, __LINE__);
			break;
		}
		break;
	}

	if (FALSE == WSACloseEvent(wsaAcceptSocket))
	{
		PrintErrorWSA((PCHAR)__func__, __LINE__);
		return INVALID_SOCKET;
	}

	if (CONTINUE != g_bServerState)
	{
		return INVALID_SOCKET;
	}

	PrintSockaddrStorage(&ClientAddr);

	return ClientFileDescriptor;
}


SOCKET
NetConnect(PWSTR pszAddress, PWSTR pszPort, LPWSABUF lpCallerData,
	LPWSABUF lpCalleeData)
{
	if (NULL == pszAddress || NULL == pszPort)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
		WSACleanup();
		return INVALID_SOCKET;
	}

	ADDRINFOW Hints;
	PADDRINFOW pResult = NULL;
	PADDRINFOW pTempResult = NULL;
	SOCKET SocketFileDescriptor = INVALID_SOCKET;
	INT iErrorTracker = 0;

	ZeroMemory(&Hints, sizeof(Hints));
	Hints.ai_socktype = SOCK_STREAM;
	Hints.ai_family = AF_INET;
	Hints.ai_protocol = IPPROTO_TCP;
	Hints.ai_flags = AI_NUMERICSERV;

	INT iStatus = GetAddrInfoW(pszAddress, pszPort, &Hints, &pResult);

	if (EXIT_SUCCESS != iStatus)
	{
		PrintErrorSupplied((PCHAR)__func__, __LINE__, (DWORD)iStatus);
		WSACleanup();
		return INVALID_SOCKET;
	}

	for (pTempResult = pResult; pTempResult != NULL;
		pTempResult = pTempResult->ai_next)
	{
		SocketFileDescriptor = socket(pTempResult->ai_family,
			pTempResult->ai_socktype, pTempResult->ai_protocol);

		if ((INVALID_SOCKET == SocketFileDescriptor) ||
			(pTempResult->ai_addrlen > INT_MAX))
		{
			continue;
		}

		//NOTE: converstion to int is safe, given the check on line 276.
		iErrorTracker = WSAConnect(SocketFileDescriptor, pTempResult->ai_addr,
			(int)pTempResult->ai_addrlen, lpCallerData, lpCalleeData, NULL, NULL);

		if (EXIT_SUCCESS != iErrorTracker)
		{
			PrintErrorWSA((PCHAR)__func__, __LINE__);
			return INVALID_SOCKET;
		}
		else
		{
			break;
		}

		break;
	}

	FreeAddrInfoW(pTempResult);

	if (NULL == pTempResult)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "No valid sockets");
		return INVALID_SOCKET;
	}

	return SocketFileDescriptor;
}

VOID
NetCleanup(SOCKET SocketFileDescriptor, SOCKET ClientFileDescriptor)
{
	INT iErrorTracker = SOCKET_ERROR;

	if (INVALID_SOCKET != ClientFileDescriptor)
	{
		iErrorTracker = closesocket(ClientFileDescriptor);

		if (SOCKET_ERROR == iErrorTracker)
		{
			PrintErrorWSA((PCHAR)__func__, __LINE__);
		}
	}

	if (INVALID_SOCKET != SocketFileDescriptor)
	{
		iErrorTracker = closesocket(SocketFileDescriptor);

		if (SOCKET_ERROR == iErrorTracker)
		{
			PrintErrorWSA((PCHAR)__func__, __LINE__);
		}
	}

	WSACleanup();
}

//End of file
