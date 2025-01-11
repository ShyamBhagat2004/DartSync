#include <windows.h>
#include <string>
#include <sstream>
#include "BackupManager.h"

// Global handles for UI controls
static HWND hSourceEdit   = nullptr;
static HWND hDestEdit     = nullptr;
static HWND hOutputStatic = nullptr;
static HWND hBackupButton = nullptr;

// Global BackupManager
static BackupManager gBackupManager;

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void OnBackupClicked(HWND hWnd);

// Helper to set text in a Win32 control
void SetControlText(HWND hControl, const std::wstring& text) {
    SetWindowText(hControl, text.c_str());
}

// The GUI entry point instead of main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    // Register the window class
    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = TEXT("DartSyncWindowClass");
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    if(!RegisterClass(&wc)) {
        MessageBox(nullptr, TEXT("RegisterClass failed!"), TEXT("Error"), MB_ICONERROR);
        return 1;
    }

    // Create the main window
    HWND hMainWnd = CreateWindow(
        wc.lpszClassName,
        TEXT("DartSync Backup GUI"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 300,
        nullptr, nullptr, hInstance, nullptr
    );

    if(!hMainWnd) {
        MessageBox(nullptr, TEXT("CreateWindow failed!"), TEXT("Error"), MB_ICONERROR);
        return 1;
    }

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    // Standard Win32 message loop
    MSG msg;
    while(GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Called when the "Start Backup" button is clicked
void OnBackupClicked(HWND hWnd)
{
    // Retrieve user input from the text fields
    const int BUF_SIZE = 1024;
    wchar_t sourceBuf[BUF_SIZE];
    wchar_t destBuf[BUF_SIZE];

    GetWindowText(hSourceEdit, sourceBuf, BUF_SIZE);
    GetWindowText(hDestEdit,   destBuf,   BUF_SIZE);

    // Convert wide string to std::string
    std::wstring wsSource(sourceBuf);
    std::wstring wsDest(destBuf);

    std::string sourcePath(wsSource.begin(), wsSource.end());
    std::string destPath(wsDest.begin(), wsDest.end());

    // Display a "starting" message in the static control
    SetControlText(hOutputStatic, L"Starting backup...");

    // Perform the backup (single-threaded)
    std::vector<std::string> fileTypes; // e.g., empty => all
    std::string keyword;
    size_t maxFileSizeMB = 0;  // no limit

    gBackupManager.backupOnce(sourcePath, destPath, fileTypes, keyword, maxFileSizeMB);

    // After backup is done, update the static text
    SetControlText(hOutputStatic, L"Backup completed.");
}

// Window procedure: create controls and handle events
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        {
            // Create labels, edits, static output, and a button

            CreateWindow(TEXT("STATIC"), TEXT("Source Path:"),
                         WS_VISIBLE | WS_CHILD,
                         20, 20, 90, 20,
                         hWnd, nullptr, nullptr, nullptr);

            hSourceEdit = CreateWindow(TEXT("EDIT"), TEXT(""),
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         120, 20, 400, 20,
                         hWnd, nullptr, nullptr, nullptr);

            CreateWindow(TEXT("STATIC"), TEXT("Destination Path:"),
                         WS_VISIBLE | WS_CHILD,
                         20, 60, 90, 20,
                         hWnd, nullptr, nullptr, nullptr);

            hDestEdit = CreateWindow(TEXT("EDIT"), TEXT(""),
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                         120, 60, 400, 20,
                         hWnd, nullptr, nullptr, nullptr);

            hOutputStatic = CreateWindow(TEXT("STATIC"), TEXT("Output:"),
                         WS_VISIBLE | WS_CHILD,
                         20, 100, 550, 20,
                         hWnd, nullptr, nullptr, nullptr);

            hBackupButton = CreateWindow(TEXT("BUTTON"), TEXT("Start Backup"),
                         WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                         120, 140, 120, 30,
                         hWnd, (HMENU)1, nullptr, nullptr);
        }
        break;
    case WM_COMMAND:
        {
            // If the user clicked the button with ID=1
            if (LOWORD(wParam) == 1) {
                OnBackupClicked(hWnd);
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
