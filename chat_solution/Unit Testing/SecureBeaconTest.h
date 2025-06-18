#pragma once

// Prevent Winsock redefinition
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Define a macro to prevent multiple inclusions of Winsock headers
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include "../implant/BeaconAux.h"
#include "../implant/SecureBeacon.h"
#include "../implant/shared.h"

// Declaration of the TestCheckIn function
RETURNTYPE TestCheckIn();

// End of file
