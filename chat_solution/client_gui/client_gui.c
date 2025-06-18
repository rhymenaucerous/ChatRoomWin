// client_gui.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "client_gui.h"
#include <stdio.h>
#include <commctrl.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

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
#define IDM_SEND_MESSAGE        110

// Helper function to append text to display box
void AppendToDisplay(const WCHAR* text) {
    WCHAR displayText[4096];
    GetWindowTextW(hDisplayBox, displayText, 4096);
    wcscat_s(displayText, 4096, text);
    wcscat_s(displayText, 4096, L"\r\n");
    SetWindowTextW(hDisplayBox, displayText);
}

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK InputBoxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// String to display title
const WCHAR szAsciiTitle[] =
L"   ___ _           _     __                      __    __ _       \r\n"
L"  / __\\ |__   __ _| |_  /__\\ ___   ___  _ __ ___/ / /\\ \\ (_)_ __  \r\n"
L" / /  | '_ \\ / _` | __|/ \\/// _ \\ / _ \\| '_ ` _ \\ \\/  \\/ / | '_ \\ \r\n"
L"/ /___| | | | (_| | |_/ _  \\ (_) | (_) | | | | | \\  /\\  /| | | | |\r\n"
L"\\____/|_| |_|\\__,_|\\__\\/ \\_/\\___/ \\___/|_| |_| |_|\\/  \\/ |_|_| |_|\r\n";

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 800, 600, NULL, NULL, hInstance, NULL);

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
                    WCHAR buffer[1024];
                    GetWindowTextW(hInputBox, buffer, 1024);
                    
                    // Append the actual input text if it's not empty
                    if (wcslen(buffer) > 0)
                    {
                        AppendToDisplay(buffer);
                    }
                    
                    // Clear input box
                    SetWindowTextW(hInputBox, L"");

                    // Set focus back to the input box after sending
                    SetFocus(hInputBox);
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
    case WM_KEYDOWN:
        MessageBoxW(hWnd, L"Key was pressed!", L"Debug Alert", MB_OK);
        // Check if the pressed key is Enter (VK_RETURN)
        // And if the input box currently has focus
        if (wParam == VK_RETURN && GetFocus() == hInputBox)
        {
            // Simulate the IDM_SEND_MESSAGE command
            // This reuses the existing logic for sending messages
            SendMessage(hWnd, WM_COMMAND, IDM_SEND_MESSAGE, 0);
            return 0; // Message handled
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
            WCHAR buffer[1024];
            GetWindowTextW(hWnd, buffer, 1024); // Use hWnd of the control itself

            // Append the actual input text if it's not empty
            if (wcslen(buffer) > 0)
            {
                AppendToDisplay(buffer);
            }

            // Clear input box
            SetWindowTextW(hWnd, L"");

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
