/*****************************************************************//**
 * \file   c_srv_listen.h
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once

#include <Windows.h>
#include <stdio.h>

// MACROs for array index of events in handle array.
#define SUCCESS_EVENT 0
#define SYNC_EVENT    1


VOID
ListenForChats(PVOID pListenerArgsHolder);

//End of file
