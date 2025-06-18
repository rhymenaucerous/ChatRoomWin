/*****************************************************************//**
 * \file   s_shared.c
 * \brief  
 * 
 * \author chris
 * \date   October 2024
 *********************************************************************/
#include "pch.h"
#include "s_shared.h"

VOID
FreeMsg(PVOID pParam)
{
	PMSGHOLDER pMsgHolder = (PMSGHOLDER)pParam;

	//NOTE: Not checking return but it should be zero!
	MyHeapFree(GetProcessHeap(), NO_OPTION, pMsgHolder, sizeof(MSGHOLDER));
}

 //NOTE: Mutexes are released on their own.
VOID
UserFreeFunction(PVOID pParam)
{
	if (NULL == pParam)
	{
		return;
	}

	PUSER pTempUser = (PUSER)pParam;

	//TODO: Send shutdown message.

	if (SOCKET_ERROR == shutdown(pTempUser->m_ClientSocket, SD_BOTH))
	{
		ThreadPrintErrorCustom(
			pTempUser->m_haSharedHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__, "shutdown()");
	}

	if (SOCKET_ERROR == closesocket(pTempUser->m_ClientSocket))
	{
		ThreadPrintErrorCustom(
			pTempUser->m_haSharedHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__, "closesocket()");
	}

	if (EXIT_FAILURE == QueueDestroy(pTempUser->m_SendMsgQueue, FreeMsg))
	{
		ThreadPrintErrorCustom(
			pTempUser->m_haSharedHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__, "QueueDestroy()");
	}

	if (WIN_EXIT_FAILURE == MyHeapFree(GetProcessHeap(), NO_OPTION, pTempUser,
		sizeof(USER)))
	{
		ThreadPrintErrorCustom(
			pTempUser->m_haSharedHandles[STD_ERR_MUTEX],
			(PCHAR)__func__, __LINE__, "MyHeapFree()");
	}
}
