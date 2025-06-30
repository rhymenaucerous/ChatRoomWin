/*****************************************************************//**
 * \file   main.h
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once

#include <Windows.h>
#include <stdio.h>

#define CMD_LINE_MAX 8191
#define BASE_10      10

DWORD CustomWaitForSingleObject(HANDLE hInputEvent, DWORD dwTimeout);

void AppendToDisplayNoNewline(const PWCHAR text);

void AppendToDisplay(const PWCHAR text);

VOID GetAndAssign(PWCHAR pAssign, PDWORD pdwLen);

VOID GetAndAppend();

INT InitializeClient();

//End of file
