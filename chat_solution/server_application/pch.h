/*****************************************************************//**
 * \file   pch.h
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once

#include <Windows.h>
#include <stdio.h>

 //NOTE: Needed for MyError library - stringcchprintfw()
#include <strsafe.h>

//NOTE: Networking library requires windows socekts header.
#include <Winsock2.h>

//NOTE: Definition of INT_MAX.
#include <limits.h>

//NOTE: included for strtoul
#include <stdlib.h>

//NOTE: For getaddrinfo()
#include <ws2tcpip.h>

//NOTE: For time() function.
#include <time.h>

//NOTE: Included for wcsncmp()
#include <string.h>
#define STRINGS_EQUAL 0

//NOTE: Wide chars more widely utilized.
#ifndef _UNICODE
#define _UNICODE
#endif

//macros enabling clear returns from some functions.
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define EXIT_FAILURE_NEGATIVE -1

//macros enabling clear returns from some windows functions.
#define WIN_EXIT_SUCCESS 1
#define WIN_EXIT_FAILURE 0

//macros for starting/stopping server/client run
#define CONTINUE 1
#define STOP 0

//filenames should not exceed 50 characters - this will enable some paths.
#define FILE_NAME_MAX_LEN 50

//Maximums for strings: IPv4 or IPv6 compatable.
#define HOST_MAX_STRING 40 //Max IP length is (IPv6) 40 -> 
                           //7 colons + 32 hexadecimal digits + terminator.

#define PORT_MAX_STRING 6 //Only numeric services allowed - max length of
                          //65535 is 5 + terminator.

//IP length + Port length - 1 (one less terminator) + 2 (: designator in code).
#define ADDR_MAX_STRING (HOST_MAX_STRING + PORT_MAX_STRING + 1)

//Macros to set option value to zero.
#define NO_OPTION 0
#define NO_MAX_SIZE 0

//Quick functions
#define GREATER_THAN(a, b) ((a) > (b))
#define LESS_THAN(a, b) ((a) < (b))


//Per BARR C, the max function/variable name length is 31.
#define FN_NAME_MAX_LEN 31
#define DWORD_MAX_DIGITS 10
#define USIGN_MAX_DIGITS 4
#define MAX_FORMAT_MSG_LEN 64000 //chosen bc FormatMessage has max
                                 //output buffer size of 64K bytes.

//NOTE: Covers magic number in code.
#define BASE_10 10

//NOTE: Generic buffers will stick to a size of 1024.
#define BUFF_SIZE 1024

//NOTE: Command line max reference:
// learn.microsoft.com/en-us/troubleshoot/windows-client/shell-experience/
// command-line-string-limitation
#define CMD_LINE_MAX 8191

VOID
PrintError(PCHAR pszFunctionName, DWORD dwLineNum);

VOID
PrintErrorWSA(PCHAR pszFunctionName, DWORD dwLineNum);

VOID
PrintErrorSupplied(PCHAR pszFunctionName, DWORD dwLineNum, DWORD dwErrorCode);

VOID
PrintErrorCustom(PCHAR pszFunctionName, DWORD dwLineNum,
    PCHAR pszCustomError);

HRESULT
CustomConsoleWrite(PWCHAR pszCustomOutput, DWORD dwCustomLen);

BOOL
MyHeapFree(HANDLE hHeap, DWORD dwFlags, PVOID lpMem, SIZE_T ulCount);

VOID
ThreadPrintError(HANDLE pStdErrMutex, PCHAR pszFunctionName, DWORD dwLineNum);

VOID
ThreadPrintErrorWSA(HANDLE pStdErrMutex, PCHAR pszFunctionName,
    DWORD dwLineNum);

VOID
ThreadPrintErrorSupplied(HANDLE pStdErrMutex, PCHAR pszFunctionName,
    DWORD dwLineNum, DWORD dwErrorCode);

VOID
ThreadPrintErrorCustom(HANDLE pStdErrMutex, PCHAR pszFunctionName,
    DWORD dwLineNum, PCHAR pszCustomError);

VOID
ThreadCustomConsoleWrite(HANDLE pStdOutMutex, PWCHAR pszCustomOutput,
    DWORD dwCustomLen);

//End of file
