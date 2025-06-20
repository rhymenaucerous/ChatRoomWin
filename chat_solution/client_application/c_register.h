#pragma once

#include <Windows.h>
#include <stdio.h>

HRESULT
HandleRegistration(PWSTR pszClientName, SIZE_T wNameLen,
	PLISTENERARGS pListenerArgs);

//End of file
