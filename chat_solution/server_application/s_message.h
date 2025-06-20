/*****************************************************************//**
 * \file   s_message.h
 * \brief
 *
 * \author chris
 * \date   September 2024
 *********************************************************************/
#pragma once
#include "s_shared.h"

VOID
ResetChatRecv(PMSGHOLDER pMsgHolder);

PMSGHOLDER
AddMsgToQueue(PUSER pUser, INT8 iType, INT8 iSubType,
	INT8 iOpcode, WORD wLenOne, WORD wLenTwo, PWSTR pszDataOne,
	PWSTR pszDataTwo);

HRESULT
ManageMsgQueueAdd(PUSER pUser, INT8 iType, INT8 iSubType,
	INT8 iOpcode, WORD wLenOne, WORD wLenTwo, PWSTR pszDataOne,
	PWSTR pszDataTwo);

VOID
FreeMsg(PVOID pParam);

 //End of file
