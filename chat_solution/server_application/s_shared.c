/*****************************************************************//**
 * \file   s_shared.c
 * \brief
 *
 * \author chris
 * \date   October 2024
 *********************************************************************/

#include "s_shared.h"
#include "Queue.h"

extern volatile BOOL g_bServerState;

VOID
FreeMsg(PVOID pParam)
{
	PMSGHOLDER pMsgHolder = (PMSGHOLDER)pParam;
	ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pMsgHolder, sizeof(MSGHOLDER));
}

// NOTE: Mutexes are released on their own.
VOID
UserFreeFunction(PVOID pParam)
{
	if (NULL == pParam)
	{
		return;
	}

	PUSER pTempUser = (PUSER)pParam;

	if (SOCKET_ERROR == shutdown(pTempUser->m_ClientSocket, SD_BOTH))
	{
        DEBUG_PRINT("Shutdown");
	}

	if (SOCKET_ERROR == closesocket(pTempUser->m_ClientSocket))
    {
        DEBUG_PRINT("closesocket()");
	}

	if (SUCCESS != QueueDestroy(pTempUser->m_SendMsgQueue, FreeMsg))
    {
        DEBUG_PRINT("QueueDestroy()");
    }

    ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pTempUser, sizeof(USER));
}
