#pragma once
#include "pch.h"
#include "c_shared.h"
#include "c_messages.h"

HRESULT
HandleRegistration(PWSTR pszClientName, SIZE_T wNameLen,
	PLISTENERARGS pListenerArgs);

//End of file
