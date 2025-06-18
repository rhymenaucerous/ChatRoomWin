/*****************************************************************//**
 * \file   pch.c
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include "pch.h"

 //NOTE: Value selected based on FormatMessage's max.
#define CUSTOM_ERROR_MAX 64000

#define ERR_MSG_ONE_LEN 32
#define ERR_MSG_TWO_LEN 31
#define ERR_MSG_THREE_LEN 56
#define ERR_MSG_FOUR_LEN 19

#define MAX_ERR_MSG_TWO_LEN (ERR_MSG_TWO_LEN + FN_NAME_MAX_LEN + \
DWORD_MAX_DIGITS + USIGN_MAX_DIGITS + MAX_FORMAT_MSG_LEN + 1)

#define MAX_ERR_MSG_THREE_LEN (ERR_MSG_THREE_LEN + FN_NAME_MAX_LEN + \
DWORD_MAX_DIGITS + USIGN_MAX_DIGITS + 1)

#define MAX_ERR_MSG_FOUR_LEN (ERR_MSG_FOUR_LEN + FN_NAME_MAX_LEN + \
DWORD_MAX_DIGITS + CUSTOM_ERROR_MAX + 1)

//TODO: Make another error print that handles non-shutdown cases. This is
// needed to use getlasterror and such functionality. in this case,
// writeconsolew is used without checking for success.

/**
 * Uses GetLastError() to print the last error as well as the function name
 * and line number.
 *
 * \param pszFunctionName (PCHAR)__func__ is normal input here but, alternatively,
 * a custom string can be given.
 * \param dwLineNum line number function is called on.
 * \return
 */
VOID
PrintError(PCHAR pszFunctionName, DWORD dwLineNum)
{
	HANDLE hConsoleOutput = GetStdHandle(STD_ERROR_HANDLE);

	if (INVALID_HANDLE_VALUE == hConsoleOutput)
	{
		return;
	}

	//Don't need to NULL check since we'll only be passing the dunder values
	//to this function
	DWORD dwErrorCode = GetLastError();

	if (dwErrorCode == 0)
	{
		return; //No Error
	}

	PVOID pMesageBuffer = NULL;

	//LocalAlloc() specified in FormatMessage with the
	//FORMAT_MESSAGE_ALLOCATE_BUFFER option. LocalFree() used bc of this.
	DWORD dwBufferLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
		dwErrorCode, MAKELANGID(LANG_NEUTRAL,
			SUBLANG_DEFAULT),
		(PWSTR)&pMesageBuffer, NO_OPTION, NULL);

	SIZE_T ulCharsConverted;
	WCHAR pszWideName[FN_NAME_MAX_LEN + 1] = { 0 };
	errno_t erStatus = mbstowcs_s(&ulCharsConverted, pszWideName,
		(FN_NAME_MAX_LEN + 1), pszFunctionName, FN_NAME_MAX_LEN);

	if (EXIT_SUCCESS != erStatus)
	{
		WriteConsoleW(hConsoleOutput, L"PrintError failed on mbstowcs_s\n",
			ERR_MSG_ONE_LEN, NULL, NULL);

		LocalFree(pMesageBuffer);

		return;
	}

	if (dwBufferLen)
	{
		PWSTR pszErrorMsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			MAX_ERR_MSG_TWO_LEN * sizeof(WCHAR));

		if (NULL == pszErrorMsg)
		{
			return;
		}

		HRESULT hrReturn = StringCchPrintfW(pszErrorMsg, MAX_ERR_MSG_TWO_LEN,
			L"%s failed with error %lu at line %lu: %s\n",
			pszWideName, dwErrorCode, dwLineNum, (PWCHAR)pMesageBuffer);

		if (S_OK != hrReturn)
		{
			return;
		}

		WriteConsoleW(hConsoleOutput, pszErrorMsg,
			MAX_ERR_MSG_TWO_LEN, NULL, NULL);

		LocalFree(pMesageBuffer);
	}
	else {
		PWSTR pszErrorMsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			MAX_ERR_MSG_THREE_LEN * sizeof(WCHAR));

		if (NULL == pszErrorMsg)
		{
			return;
		}

		HRESULT hrReturn = StringCchPrintfW(pszErrorMsg, MAX_ERR_MSG_THREE_LEN,
			L"%s failed with error %lu (unable to format message) at line %lu\n",
			pszWideName, dwErrorCode, dwLineNum);

		if (S_OK != hrReturn)
		{
			MyHeapFree(GetProcessHeap(), NO_OPTION, pszErrorMsg,
				MAX_ERR_MSG_THREE_LEN * sizeof(WCHAR));

			return;
		}

		WriteConsoleW(hConsoleOutput, pszErrorMsg,
			MAX_ERR_MSG_THREE_LEN, NULL, NULL);

		MyHeapFree(GetProcessHeap(), NO_OPTION, pszErrorMsg,
			MAX_ERR_MSG_THREE_LEN * sizeof(WCHAR));
	}
}

/**
 * Uses GetLastError() to print the last error as well as the function name
 * and line number.
 *
 * \param pszFunctionName (PCHAR)__func__ is normal input here but, alternatively,
 * a custom string can be given.
 * \param dwLineNum line number function is called on.
 * \param dwErrorCode error code.
 * \return
 */
VOID
PrintErrorSupplied(PCHAR pszFunctionName, DWORD dwLineNum, DWORD dwErrorCode)
{
	HANDLE hConsoleOutput = GetStdHandle(STD_ERROR_HANDLE);

	if (INVALID_HANDLE_VALUE == hConsoleOutput)
	{
		return;
	}

	PVOID pMesageBuffer = NULL;

	//LocalAlloc() specified in FormatMessage with the
	//FORMAT_MESSAGE_ALLOCATE_BUFFER option. LocalFree() used bc of this.
	DWORD dwBufferLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
		dwErrorCode, MAKELANGID(LANG_NEUTRAL,
			SUBLANG_DEFAULT),
		(PWSTR)&pMesageBuffer, NO_OPTION, NULL);

	SIZE_T ulCharsConverted;
	WCHAR pszWideName[FN_NAME_MAX_LEN + 1] = { 0 };
	errno_t erStatus = mbstowcs_s(&ulCharsConverted, pszWideName,
		(FN_NAME_MAX_LEN + 1), pszFunctionName, FN_NAME_MAX_LEN);

	if (EXIT_SUCCESS != erStatus)
	{
		WriteConsoleW(hConsoleOutput, L"PrintError failed on mbstowcs_s\n",
			ERR_MSG_ONE_LEN, NULL, NULL);

		LocalFree(pMesageBuffer);

		return;
	}

	if (dwBufferLen)
	{
		PWSTR pszErrorMsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			MAX_ERR_MSG_TWO_LEN * sizeof(WCHAR));

		if (NULL == pszErrorMsg)
		{
			return;
		}

		HRESULT hrReturn = StringCchPrintfW(pszErrorMsg, MAX_ERR_MSG_TWO_LEN,
			L"%s failed with error %lu at line %lu: %s\n",
			pszWideName, dwErrorCode, dwLineNum, (PWCHAR)pMesageBuffer);

		if (S_OK != hrReturn)
		{
			return;
		}

		WriteConsoleW(hConsoleOutput, pszErrorMsg,
			MAX_ERR_MSG_TWO_LEN, NULL, NULL);

		LocalFree(pMesageBuffer);
	}
	else {
		PWSTR pszErrorMsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			MAX_ERR_MSG_THREE_LEN * sizeof(WCHAR));

		if (NULL == pszErrorMsg)
		{
			return;
		}

		HRESULT hrReturn = StringCchPrintfW(pszErrorMsg, MAX_ERR_MSG_THREE_LEN,
			L"%s failed with error %lu (unable to format message) at line %lu\n",
			pszWideName, dwErrorCode, dwLineNum);

		if (S_OK != hrReturn)
		{
			MyHeapFree(GetProcessHeap(), NO_OPTION, pszErrorMsg,
				MAX_ERR_MSG_THREE_LEN * sizeof(WCHAR));

			return;
		}

		WriteConsoleW(hConsoleOutput, pszErrorMsg,
			MAX_ERR_MSG_THREE_LEN, NULL, NULL);

		MyHeapFree(GetProcessHeap(), NO_OPTION, pszErrorMsg,
			MAX_ERR_MSG_THREE_LEN * sizeof(WCHAR));
	}
}

/**
 * Uses WSAGetLastError() to print the last error as well as the function name
 * and line number.
 *
 * \param pszFunctionName (PCHAR)__func__ is normal input here but, alternatively,
 * a custom string can be given.
 * \param dwLineNum line number function is called on.
 * \return
 */
VOID
PrintErrorWSA(PCHAR pszFunctionName, DWORD dwLineNum)
{
	HANDLE hConsoleOutput = GetStdHandle(STD_ERROR_HANDLE);

	if (INVALID_HANDLE_VALUE == hConsoleOutput)
	{
		return;
	}

	//Don't need to NULL check since we'll only be passing the dunder values
	//to this function
	DWORD dwErrorCode = WSAGetLastError();

	if (dwErrorCode == 0)
	{
		return; //No Error
	}

	PVOID pMesageBuffer = NULL;

	//LocalAlloc() specified in FormatMessage with the
	//FORMAT_MESSAGE_ALLOCATE_BUFFER option. LocalFree() used bc of this.
	DWORD dwBufferLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
		dwErrorCode, MAKELANGID(LANG_NEUTRAL,
			SUBLANG_DEFAULT),
		(PWSTR)&pMesageBuffer, NO_OPTION, NULL);

	SIZE_T ulCharsConverted;
	WCHAR pszWideName[FN_NAME_MAX_LEN + 1] = { 0 };
	errno_t erStatus = mbstowcs_s(&ulCharsConverted, pszWideName,
		(FN_NAME_MAX_LEN + 1), pszFunctionName, FN_NAME_MAX_LEN);

	if (EXIT_SUCCESS != erStatus)
	{
		WriteConsoleW(hConsoleOutput, L"PrintError failed on mbstowcs_s\n",
			ERR_MSG_ONE_LEN, NULL, NULL);

		LocalFree(pMesageBuffer);

		return;
	}

	if (dwBufferLen)
	{
		PWSTR pszErrorMsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			MAX_ERR_MSG_TWO_LEN * sizeof(WCHAR));

		if (NULL == pszErrorMsg)
		{
			return;
		}

		HRESULT hrReturn = StringCchPrintfW(pszErrorMsg, MAX_ERR_MSG_TWO_LEN,
			L"%s failed with error %lu at line %lu: %s\n",
			pszWideName, dwErrorCode, dwLineNum, (PWCHAR)pMesageBuffer);

		if (S_OK != hrReturn)
		{
			return;
		}

		WriteConsoleW(hConsoleOutput, pszErrorMsg,
			MAX_ERR_MSG_TWO_LEN, NULL, NULL);

		LocalFree(pMesageBuffer);
	}
	else {
		PWSTR pszErrorMsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			MAX_ERR_MSG_THREE_LEN * sizeof(WCHAR));

		if (NULL == pszErrorMsg)
		{
			return;
		}

		HRESULT hrReturn = StringCchPrintfW(pszErrorMsg, MAX_ERR_MSG_THREE_LEN,
			L"%s failed with error %lu (unable to format message) at line %lu\n",
			pszWideName, dwErrorCode, dwLineNum);

		if (S_OK != hrReturn)
		{
			MyHeapFree(GetProcessHeap(), NO_OPTION, pszErrorMsg,
				MAX_ERR_MSG_THREE_LEN * sizeof(WCHAR));
			return;
		}

		WriteConsoleW(hConsoleOutput, pszErrorMsg,
			MAX_ERR_MSG_THREE_LEN, NULL, NULL);

		MyHeapFree(GetProcessHeap(), NO_OPTION, pszErrorMsg,
			MAX_ERR_MSG_THREE_LEN * sizeof(WCHAR));
	}
}

/**
 * Prints a custom error message that can be up to 64K bytes in length.
 * Additionally, prints the function name and line number supplied.
 *
 * \param pszFunctionName
 * (PCHAR)__func__ is normal input here but, alternatively,
 * a custom string can be given.
 *
 * \param dwLineNum
 * line number function is called on.
 *
 * \param pszCustomError
 * custom error string supplied by user.
 *
 * \return
 */
VOID
PrintErrorCustom(PCHAR pszFunctionName, DWORD dwLineNum,
	PCHAR pszCustomError)
{
	HANDLE hConsoleOutput = GetStdHandle(STD_ERROR_HANDLE);

	if (INVALID_HANDLE_VALUE == hConsoleOutput)
	{
		return;
	}

	//Don't need to NULL check since we'll only be passing the dunder values
	//to this function
	SIZE_T ulCharsConverted;
	SIZE_T ulCharsConverted2;
	WCHAR pszFunctionNameWide[FN_NAME_MAX_LEN + 1] = { 0 };

	PWSTR pszCustomErrorWide = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		(CUSTOM_ERROR_MAX + 1) * sizeof(WCHAR));

	if (NULL == pszCustomErrorWide)
	{
		return;
	}

	errno_t Status = mbstowcs_s(&ulCharsConverted, pszFunctionNameWide,
		(FN_NAME_MAX_LEN + 1), pszFunctionName, FN_NAME_MAX_LEN);
	errno_t Status2 = mbstowcs_s(&ulCharsConverted2, pszCustomErrorWide,
		(CUSTOM_ERROR_MAX + 1), pszCustomError, CUSTOM_ERROR_MAX);

	if ((EXIT_SUCCESS != Status) || (EXIT_SUCCESS != Status2))
	{
		WriteConsoleW(hConsoleOutput, L"PrintError failed on mbstowcs_s\n",
			ERR_MSG_ONE_LEN, NULL, NULL);
		MyHeapFree(GetProcessHeap(), NO_OPTION, pszCustomErrorWide,
			(CUSTOM_ERROR_MAX + 1) * sizeof(WCHAR));
		return;
	}

	PWSTR pszErrorMsg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		MAX_ERR_MSG_FOUR_LEN * sizeof(WCHAR));

	if (NULL == pszErrorMsg)
	{
		MyHeapFree(GetProcessHeap(), NO_OPTION, pszCustomErrorWide,
			(CUSTOM_ERROR_MAX + 1) * sizeof(WCHAR));
		return;
	}

	HRESULT hrReturn = StringCchPrintfW(pszErrorMsg, MAX_ERR_MSG_FOUR_LEN,
		L"%s failed at line %lu: %s\n",
		pszFunctionNameWide, dwLineNum, pszCustomErrorWide);


	MyHeapFree(GetProcessHeap(), NO_OPTION, pszCustomErrorWide,
		(CUSTOM_ERROR_MAX + 1) * sizeof(WCHAR));

	if (S_OK != hrReturn)
	{
		MyHeapFree(GetProcessHeap(), NO_OPTION, pszErrorMsg,
			MAX_ERR_MSG_FOUR_LEN * sizeof(WCHAR));
		return;
	}

	WriteConsoleW(hConsoleOutput, pszErrorMsg,
		MAX_ERR_MSG_FOUR_LEN, NULL, NULL);

	MyHeapFree(GetProcessHeap(), NO_OPTION, pszErrorMsg,
		MAX_ERR_MSG_FOUR_LEN * sizeof(WCHAR));
}

/**
 * .
 *
 * \param pszCustomOutput
 * Null terminated wide char string provided by user.
 *
 * \param dwCustomLen The length of the cutsom string.
 *
 * \return HRESULT: E_HANDLE, E_UNEXPECTED, or S_OK.
 */
HRESULT
CustomConsoleWrite(PWCHAR pszCustomOutput, DWORD dwCustomLen)
{
	HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	if (INVALID_HANDLE_VALUE == hConsoleOutput)
	{
		return E_HANDLE;
	}

	if (WIN_EXIT_FAILURE == WriteConsoleW(hConsoleOutput, pszCustomOutput,
		dwCustomLen, NULL, NULL))
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

BOOL
MyHeapFree(HANDLE hHeap, DWORD dwFlags, PVOID lpMem, SIZE_T ulCount)
{
	SecureZeroMemory(lpMem, ulCount);

	return HeapFree(hHeap, dwFlags, lpMem);
}

//NOTE: Functions below are created to enable printing inside a thread.
VOID
ThreadPrintError(HANDLE pStdErrMutex, PCHAR pszFunctionName, DWORD dwLineNum)
{
	DWORD dsWaitTimeMS = INFINITE; //NOTE: alter value to allow for timeout.

	DWORD dwResult = WaitForSingleObject(pStdErrMutex, dsWaitTimeMS);

	if (WAIT_OBJECT_0 == dwResult)
	{
		PrintError(pszFunctionName, dwLineNum);
	}

	ReleaseMutex(pStdErrMutex);
}

VOID
ThreadPrintErrorWSA(HANDLE pStdErrMutex, PCHAR pszFunctionName,
	DWORD dwLineNum)
{
	DWORD dsWaitTimeMS = INFINITE; //NOTE: alter value to allow for timeout.

	DWORD dwResult = WaitForSingleObject(pStdErrMutex, dsWaitTimeMS);

	if (WAIT_OBJECT_0 == dwResult)
	{
		PrintErrorWSA(pszFunctionName, dwLineNum);
	}

	ReleaseMutex(pStdErrMutex);
}

VOID
ThreadPrintErrorSupplied(HANDLE pStdErrMutex, PCHAR pszFunctionName,
	DWORD dwLineNum, DWORD dwErrorCode)
{
	DWORD dsWaitTimeMS = INFINITE; //NOTE: alter value to allow for timeout.

	DWORD dwResult = WaitForSingleObject(pStdErrMutex, dsWaitTimeMS);

	if (WAIT_OBJECT_0 == dwResult)
	{
		PrintErrorSupplied(pszFunctionName, dwLineNum, dwErrorCode);
	}

	ReleaseMutex(pStdErrMutex);
}

VOID
ThreadPrintErrorCustom(HANDLE pStdErrMutex, PCHAR pszFunctionName,
	DWORD dwLineNum, PCHAR pszCustomError)
{
	DWORD dsWaitTimeMS = INFINITE; //NOTE: alter value to allow for timeout.

	DWORD dwResult = WaitForSingleObject(pStdErrMutex, dsWaitTimeMS);

	if (WAIT_OBJECT_0 == dwResult)
	{
		PrintErrorCustom(pszFunctionName, dwLineNum, pszCustomError);
	}

	ReleaseMutex(pStdErrMutex);
}


VOID
ThreadCustomConsoleWrite(HANDLE pStdOutMutex, PWCHAR pszCustomOutput,
	DWORD dwCustomLen)
{
	DWORD dsWaitTimeMS = INFINITE; //NOTE: alter value to allow for timeout.

	DWORD dwResult = WaitForSingleObject(pStdOutMutex, dsWaitTimeMS);

	if (WAIT_OBJECT_0 == dwResult)
	{
		CustomConsoleWrite(pszCustomOutput, dwCustomLen);
	}

	ReleaseMutex(pStdOutMutex);
}


//End of file
