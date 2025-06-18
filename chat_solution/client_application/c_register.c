#include "pch.h"
#include "c_register.h"

//NOTE: listener args has the server socket and the read event.
HRESULT
HandleRegistration(PWSTR pszClientName, SIZE_T stNameLen,
	PLISTENERARGS pListenerArgs)
{
	WCHAR caUserName[MAX_UNAME_LEN + 1] = { 0 };
	wcscpy_s(caUserName, MAX_UNAME_LEN, pszClientName);

	HANDLE hStdInput = GetStdHandle(STD_INPUT_HANDLE);

//NOTE: Safe conversion from size_t to DWORD is possible due to check in main -
// stNameLen must be 10 or less.
#pragma warning(push)
#pragma warning(disable : 4244)
	DWORD dwNumberofCharsRead = stNameLen;
#pragma warning(push)

	if (INVALID_HANDLE_VALUE == hStdInput)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "GetStdHandle");
		return E_HANDLE;
	}

	while (CONTINUE == g_bClientState)
	{
//NOTE: Safe conversion from DWORD to WORD is possible due to check in loop -
// dwNumberofCharsRead must be 10 or less.
#pragma warning(push)
#pragma warning(disable : 4244)
		WORD wNumberofCharsRead = dwNumberofCharsRead;
#pragma warning(push)

		HRESULT hResult = SendPacket(pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			pListenerArgs->m_ServerSocket, TYPE_ACCOUNT, STYPE_LOGIN,
			OPCODE_REQ, wNumberofCharsRead, 0, caUserName, NULL);
		if (S_OK != hResult)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "SendPacket()");
			return hResult;
		}

		CHATMSG RecvChat = { 0 };
		hResult = ClientRecvPacket(
			pListenerArgs->m_hHandles[STD_ERR_MUTEX],
			pListenerArgs->m_hHandles[STD_OUT_MUTEX],
			pListenerArgs->m_ServerSocket, &RecvChat,
			(WSAEVENT)pListenerArgs->m_hHandles[READ_EVENT]);

		if (S_OK != hResult)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "ClientRecvPacket()");
			return hResult;
		}

		if ((RecvChat.iType != TYPE_ACCOUNT) ||
			(RecvChat.iSubType != STYPE_LOGIN) ||
			(RecvChat.iOpcode != OPCODE_ACK))
		{
			//NOTE: Server didn't ack. Let's analyze packet.
			if ((RecvChat.iType == TYPE_FAILURE) &&
				((RecvChat.iOpcode == REJECT_UNAME_LEN) ||
				(RecvChat.iOpcode == REJECT_USER_LOGGED) ||
				(RecvChat.iOpcode == REJECT_INVALID_PACKET) ||
				(RecvChat.iOpcode == REJECT_SRV_FULL)))
			{
				ThreadPrintFailurePacket(
					pListenerArgs->m_hHandles[STD_ERR_MUTEX],
					RecvChat.iOpcode);
				CustomConsoleWrite(L"If you'd like to try again, type in your"
					" username and press enter.\nOtherwise, press ctrl+C.\n",
					93);
			}
			else
			{
				CustomConsoleWrite(L"Invalid packet received from server - "
					"potential versioning issues.\nTry again?\n", 79);
			}

			SecureZeroMemory(caUserName, MAX_UNAME_LEN);
			BOOL bResult = ReadConsoleW(hStdInput, caUserName, (MAX_UNAME_LEN),
				&dwNumberofCharsRead, NULL);

			if ((FALSE == bResult) || (0 == dwNumberofCharsRead))
			{
				PrintErrorCustom((PCHAR)__func__, __LINE__, "ReadConsoleW()");
				return E_FAIL;
			}

			dwNumberofCharsRead -= 2;

			continue;
		}

		CustomConsoleWrite(L"Username registered with server.\n", 34);
		return S_OK;
	}

	return E_FAIL;
}

//End of file
