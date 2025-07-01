// client_gui.cpp : Defines the entry point for the application.
//

#include "c_shared.h"
#include "c_main.h"
#include "framework.h"
#include "client_gui.h"
#include <stdio.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <ws2tcpip.h> // For GetAddrInfoW

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hInputBox;                                 // Input text box
HWND hDisplayBox;                               // Display text box
HWND hStatusBox;                                // Status display box
#define IDC_INPUTBOX 1001                       // Control ID for input box
#define IDC_DISPLAYBOX 1002                     // Control ID for display box
#define IDC_STATUSBOX 1003                      // Control ID for status box
#define IDM_SEND_MESSAGE 110

// Forward declarations of functions included in this code module:
ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ConnectionDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK InputBoxSubclassProc(HWND      hWnd,
                                      UINT      uMsg,
                                      WPARAM    wParam,
                                      LPARAM    lParam,
                                      UINT_PTR  uIdSubclass,
                                      DWORD_PTR dwRefData);

// Used to signal input receivers that input is ready to be processed
HANDLE g_hInputReadyEvent = NULL;

// String to display title
const WCHAR szAsciiTitle[] =
    L"   ___ _           _     __                      __    __ _       \r\n"
    L"  / __\\ |__   __ _| |_  /__\\ ___   ___  _ __ ___/ / /\\ \\ (_)_ __  "
    L"\r\n"
    L" / /  | '_ \\ / _` | __|/ \\/// _ \\ / _ \\| '_ ` _ \\ \\/  \\/ / | '_ "
    L"\\ \r\n"
    L"/ /___| | | | (_| | |_/ _  \\ (_) | (_) | | | | | \\  /\\  /| | | | |\r\n"
    L"\\____/|_| |_|\\__,_|\\__\\/ \\_/\\___/ \\___/|_| |_| |_|\\/  \\/ |_|_| "
    L"|_|\r\n";

extern HANDLE g_hShutdownEvent;

void CreateDebugConsole()
{
    // Allocate a new console for the GUI application
    if (AllocConsole())
    {
        FILE* pCout, * pCerr, * pCin;

        // Redirect stdout to the new console
        freopen_s(&pCout, "CONOUT$", "w", stdout);
        
        // Redirect stderr to the new console
        freopen_s(&pCerr, "CONOUT$", "w", stderr);
        
        // Redirect stdin to the new console
        freopen_s(&pCin, "CONIN$", "r", stdin);

        // Optional: Set the console title
        SetConsoleTitle(L"Debug Console");

        // From this point on, printf, fprintf(stderr, ...), and scanf will work.
        printf("Debug console initialized.\n");
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
#ifdef _DEBUG
    // Create the debug console at the very start of the application
    CreateDebugConsole();
#endif

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    CONNECTION_INFO connInfo = {0};
    BOOL            bProceed = FALSE;

    if (SUCCESS != NetSetUp())
    {
        DEBUG_PRINT("NetSetUp()");
        goto EXIT;
    }

    // Loop that runs until valid connection info is provided
    while (!bProceed)
    {
        INT_PTR nResult = DialogBoxParamW(
            hInstance, MAKEINTRESOURCE(IDD_CONNECTION_DIALOG), NULL,
            ConnectionDlgProc, (LPARAM)&connInfo);

        if (nResult == IDOK)
        {
            bProceed = TRUE;
        }
        else // User pressed Cancel or closed the dialog
        {
            return 0; // Exit application
        }
    }

    // Allocate memory for connection info to pass to the new thread
    PCONNECTION_INFO pThreadConnInfo = (PCONNECTION_INFO)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CONNECTION_INFO));
    if (pThreadConnInfo == NULL)
    {
        return -1; // Allocation failed
    }
    *pThreadConnInfo = connInfo; // Copy the validated data


    g_hInputReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InitializeClient,
                 pThreadConnInfo, 0, NULL);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENTGUI, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENTGUI));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

EXIT:
    ZeroingHeapFree(GetProcessHeap(), 0, (PVOID)&pThreadConnInfo,
                    sizeof(CONNECTION_INFO));

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENTGUI));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = CreateSolidBrush(RGB(32, 32, 32));  // Dark background
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CLIENTGUI);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   // Get screen dimensions to center the window
   int screenWidth = GetSystemMetrics(SM_CXSCREEN);
   int screenHeight = GetSystemMetrics(SM_CYSCREEN);
   int windowWidth = 800;
   int windowHeight = 600;
   int x = (screenWidth - windowWidth) / 2;
   int y = (screenHeight - windowHeight) / 2;

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      x, y, windowWidth, windowHeight, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   // Dark mode
   BOOL dark = TRUE;
   DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

   // monospace font for the title
   HFONT hTitleFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

   // Title Section
   hStatusBox = CreateWindowW(
       L"STATIC", szAsciiTitle,
       WS_CHILD | WS_VISIBLE | SS_CENTER,
       10, 10, 760, 90,
       hWnd, (HMENU)IDC_STATUSBOX, hInstance, NULL);

   // Set the custom font for the status box
   if (hTitleFont)
   {
       SendMessage(hStatusBox, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
   }

   // Output box (read-only)
   hDisplayBox = CreateWindowW(
       L"EDIT", L"",
       WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
       10, 100, 760, 350,
       hWnd, (HMENU)IDC_DISPLAYBOX, hInstance, NULL);

   // Create the input box
   hInputBox = CreateWindowW(
       L"EDIT", L"",
       WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
       10, 460, 585, 50,
       hWnd, (HMENU)IDC_INPUTBOX, hInstance, NULL);

   // Subclass the input box to handle the Enter key.
   SetWindowSubclass(hInputBox, InputBoxSubclassProc, 0, 0);

   // Create the Send button
   HWND hButton = CreateWindowW(
       L"BUTTON", L"Send", // Button class and text
       WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
       600, 460, 160, 50, // Position and size
       hWnd, (HMENU)IDM_SEND_MESSAGE, hInstance, NULL);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HBRUSH hbrStatusBkgnd = NULL; // Add this line

    if (WAIT_OBJECT_0 == WaitForSingleObject(g_hShutdownEvent, 0))
    {
#ifdef _DEBUG
        printf("Shutdown event signaled, closing client GUI.");
#endif
        DestroyWindow(hWnd);
        goto EXIT;

    }

    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_SEND_MESSAGE:
                {
                    SetEvent(g_hInputReadyEvent);
                }
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        // Clean up the brush we created
        if (hbrStatusBkgnd != NULL)
        {
            DeleteObject(hbrStatusBkgnd);
        }
        PostQuitMessage(0);
        break;
    case WM_CTLCOLORSTATIC:
        {
            // Check if the message is from our status box
            if ((HWND)lParam == hStatusBox)
            {
                HDC hdcStatic = (HDC)wParam;
                SetTextColor(hdcStatic, RGB(255, 255, 0)); // Yellow
                SetBkMode(hdcStatic, TRANSPARENT);

                // Create a brush for the background (only once)
                if (hbrStatusBkgnd == NULL)
                {
                    hbrStatusBkgnd = CreateSolidBrush(RGB(32, 32, 32));
                }
                return (INT_PTR)hbrStatusBkgnd;
            }
            // For other static controls, use default handling
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

EXIT:
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Subclass procedure for the input text box
LRESULT CALLBACK InputBoxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_RETURN)
        {
            SetEvent(g_hInputReadyEvent);

            // We've handled the message, so we don't pass it on.
            return 0; 
        }
        break; // Let other key presses be handled by the default proc
    
    case WM_NCDESTROY:
        // When the control is being destroyed, we must remove the subclass.
        RemoveWindowSubclass(hWnd, InputBoxSubclassProc, 0);
        break;
    }

    // For all other messages, call the default procedure.
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK ConnectionDlgProc(HWND hDlg, UINT message, WPARAM wParam,
                                   LPARAM lParam)
{
    static PCONNECTION_INFO pConnInfo;

    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Center the dialog on the screen
            RECT rcDialog;
            GetWindowRect(hDlg, &rcDialog);
            int iScreenWidth = GetSystemMetrics(SM_CXSCREEN);
            int iScreenHeight = GetSystemMetrics(SM_CYSCREEN);

            int iDlgWidth = rcDialog.right - rcDialog.left;
            int iDlgHeight = rcDialog.bottom - rcDialog.top;

            int x = (iScreenWidth - iDlgWidth) / 2;
            int y = (iScreenHeight - iDlgHeight) / 2;
        
            SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            pConnInfo = (PCONNECTION_INFO)lParam;
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            WCHAR szIp[MAX_ADDR_LEN + 1];
            WCHAR szPort[MAX_PORT_LEN + 1];
            WCHAR szName[MAX_UNAME_LEN + 1];

            GetDlgItemTextW(hDlg, IDC_IP_EDIT, szIp, MAX_ADDR_LEN + 1);
            GetDlgItemTextW(hDlg, IDC_PORT_EDIT, szPort, MAX_PORT_LEN + 1);
            GetDlgItemTextW(hDlg, IDC_NAME_EDIT, szName, MAX_UNAME_LEN + 1);

            // --- Validation ---

            int port = _wtoi(szPort);
            if (port <= 0 || port > 65535)
            {
                MessageBoxW(hDlg,
                            L"Please enter a valid port number (1-65535).",
                            L"Input Error", MB_OK | MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            ADDRINFOW hints = {0};
            SecureZeroMemory(&hints, sizeof(hints));
            hints.ai_socktype  = SOCK_STREAM;
            hints.ai_protocol  = IPPROTO_TCP;
            hints.ai_family = AF_UNSPEC;

            PADDRINFOW pResult = NULL;

            if (GetAddrInfoW(szIp, NULL, &hints, &pResult) != 0)
            {
                MessageBoxW(hDlg,
                            L"Please enter a valid IPv4, IPv6 address, or "
                            L"hostname (like 'localhost').",
                            L"Input Error", MB_OK | MB_ICONERROR);
                return (INT_PTR)TRUE;
            }
            FreeAddrInfoW(pResult); // We only needed this for validation
            pConnInfo->dwNameLength = wcslen(szName);
            if (pConnInfo->dwNameLength == 0 ||
                pConnInfo->dwNameLength > MAX_UNAME_LEN)
            {
                MessageBoxW(hDlg, L"User name must be between 1 and 10 characters.",
                            L"Input Error", MB_OK | MB_ICONERROR);
                return (INT_PTR)TRUE;
            }

            // --- Validation passed, copy data ---
            wcscpy_s(pConnInfo->szIpAddress, MAX_ADDR_LEN + 1, szIp);
            wcscpy_s(pConnInfo->szPort, MAX_PORT_LEN + 1, szPort);
            wcscpy_s(pConnInfo->szUserName, MAX_UNAME_LEN + 1, szName);

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
