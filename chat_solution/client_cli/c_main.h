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

HRESULT
CustomConsoleWrite(PWCHAR pszCustomOutput, DWORD dwCustomLen);

//End of file
