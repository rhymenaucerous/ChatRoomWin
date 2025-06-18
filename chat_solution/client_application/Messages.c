/*****************************************************************//**
 * \file   Messages.c
 * \brief  
 * 
 * \author chris
 * \date   September 2024
 *********************************************************************/
#include "pch.h"
#include "Messages.h"

//NOTE: Conversion functions for ntoh and hton for PWSTR types.
static WCHAR
WcharHostToNet(WCHAR HostChar)
{
	return (WCHAR)htons((WORD)HostChar);
}

//WARNING: Can't trust function to know the length of the string, will
//access further data if iLen longer than space allocated to pszString
VOID
WstrHostToNet(PWSTR pszString, INT iLen)
{
	for (INT iCounter = 0; iCounter < iLen; iCounter++)
	{
		pszString[iCounter] = WcharHostToNet(pszString[iCounter]);
	}
} 

static WCHAR
WcharNetToHost(WCHAR HostChar)
{
	return (WCHAR)htons((WORD)HostChar);
}

//WARNING: Can't trust function to know the length of the string, will
//access further data if iLen longer than space allocated to pszString
VOID
WstrNetToHost(PWSTR pszString, INT iLen)
{
	for (INT iCounter = 0; iCounter < iLen; iCounter++)
	{
		pszString[iCounter] = WcharNetToHost(pszString[iCounter]);
	}
}

//End of file
