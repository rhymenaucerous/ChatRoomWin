#include "..\networking\networking.h"

#include <Windows.h>
#include <stdio.h>

#include "c_register.h"
#include "c_messages.h"
#include "c_shared.h"
#include "c_main.h"


extern volatile BOOL g_bClientState;

//NOTE: listener args has the server socket and the read event.
HRESULT
HandleRegistration(PWSTR pszClientName, SIZE_T stNameLen,
	PLISTENERARGS pListenerArgs)
{
	WCHAR caUserName[MAX_UNAME_LEN + 1] = { 0 };
	wcscpy_s(caUserName, MAX_UNAME_LEN, pszClientName);

//NOTE: Safe conversion from size_t to DWORD is possible due to check in main -
// stNameLen must be 10 or less.
#pragma warning(push)
#pragma warning(disable : 4244)
	DWORD dwNumberofCharsRead = stNameLen;
#pragma warning(push)

	while (CONTINUE == g_bClientState)
	{
//NOTE: Safe conversion from DWORD to WORD is possible due to check in loop -
// dwNumberofCharsRead must be 10 or less.
#pragma warning(push)
#pragma warning(disable : 4244)
		WORD wNumberofCharsRead = dwNumberofCharsRead;
#pragma warning(push)

        HRESULT hResult =
            SendPacket(pListenerArgs->m_ServerSocket, TYPE_ACCOUNT, STYPE_LOGIN,
                       OPCODE_REQ, wNumberofCharsRead, 0, caUserName, NULL);
		if (S_OK != hResult)
		{
			DEBUG_ERROR("SendPacket failed");
			return hResult;
		}

		CHATMSG RecvChat = {0};
        hResult =
            ClientRecvPacket(pListenerArgs->m_ServerSocket, &RecvChat,
                             (WSAEVENT)pListenerArgs->m_hHandles[READ_EVENT]);
		if (S_OK != hResult)
		{
			DEBUG_ERROR("ClientRecvPacket failed");
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
                DEBUG_PRINT("Opcode recieved: %d", RecvChat.iOpcode);
                AppendToDisplay(
                    L"If you'd like to try again, type in your"
					" username and press enter.\nOtherwise, press ctrl+C.\n");
			}
			else
			{
                AppendToDisplay(
                    L"Invalid packet received from server - "
					"potential versioning issues.\nTry again?\n");
			}

			SecureZeroMemory(caUserName, MAX_UNAME_LEN);
            GetAndAssign(caUserName, NULL);

			dwNumberofCharsRead -= 2;

			continue;
		}

		AppendToDisplay(L"Username registered with server.\n");
		return S_OK;
	}

	return E_FAIL;
}

//End of file
