/*****************************************************************//**
 * \file   s_listen.h
 * \brief  
 * 
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once
#include "pch.h"
#include "s_shared.h"
#include "s_worker.h"
#include "s_message.h"

//WARNING: Include thread safety in description. Some use regular console 
// prints for errors. Potentially, make functions static.

VOID
ThreadCount(PSERVERCHATARGS pServerArgs);

HRESULT
IOCPSetUp(PSERVERCHATARGS pChatArgs);

HRESULT
ThreadSetUp(PSERVERCHATARGS pServerArgs);

static HRESULT
ThreadShutDown(PSERVERCHATARGS pServerArgs);

//NOTE: Potentially, add next three functions to their own file: s_users.c/h
PUSERS
CreateUsers(PSERVERCHATARGS pServerArgs);

PUSER
CreateUser(PSERVERCHATARGS pServerArgs, PUSERS pUsers, SOCKET ClientSocket);

VOID
UserFreeFunction(PVOID pParam);

static VOID
ServerShutDown(PSERVERCHATARGS pServerArgs, PUSERS pUsers);

HRESULT
ServerListen(PSERVERCHATARGS pServerArgs);

//End of file
