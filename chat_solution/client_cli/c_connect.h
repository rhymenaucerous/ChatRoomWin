/*****************************************************************//**
 * \file   connect.h
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once

#include <Windows.h>
#include <stdio.h>

#include "c_shared.h"

PLISTENERARGS
ChatCreate(SOCKET ServerSocket);

SOCKET
ChatConnect(PCLIENTCHATARGS pChatArgs);

 //End of file
