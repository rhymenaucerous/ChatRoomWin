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

/**
 * CustomConsoleWrite
 * 
 * Writes a custom wide char string to the console.
 * 
 * @param pszCustomOutput Null terminated wide char string provided by user.
 * @param dwCustomLen The length of the custom string.
 * 
 * @return HRESULT: E_HANDLE, E_UNEXPECTED, or S_OK.
 */
HRESULT CustomConsoleWrite(PWCHAR pszCustomOutput, DWORD dwCustomLen);

//End of file
