#include <windows.h>
#include <shlobj.h>        // For SHBrowseForFolderW
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "BackupManager.h"
#include "ConsoleRedirect.h"

#pragma comment(lib, "shell32.lib") // Sometimes needed for SHBrowseForFolderW

// GUI Controls
static HWND hTitleStatic      = nullptr;
static HWND hSourceLabel      = nullptr;
static HWND hSourcePickBtn    = nullptr;
static HWND hSourceChosenLbl  = nullptr;
static HWND hDestLabel        = nullptr;
static HWND hDestPickBtn      = nullptr;
static HWND hDestChosenLbl    = nullptr;

static HWND hBackupFreqLabel  = nullptr;
static HWND hOnceRadio        = nullptr;
static HWND hDailyRadio       = nullptr;
static HWND hMonthlyRadio     = nullptr;

static HWND hConsoleLabel     = nullptr;
static HWND hConsoleOutput    = nullptr; // multiline edit

// We'll store user selections in global vars
static std::wstring gSourcePath;
static std::wstring gDestPath;
static std::wstring gFrequency = L"once"; // Default

static EditStreamBuf* gEditBuf = nullptr;
static BackupManager  gBackupManager; // Backup logic

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void PickFolder(std::wstring& outFolder);
void UpdateChosenPathLabel(HWND labelHwnd, const std::wstring& path);
void OnRadioFrequency(HWND radioClicked);
void BuildAndRunCommand();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    CoInitialize(nullptr); // recommended for SHBrowseForFolder
    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"DartSyncWindowClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(nullptr, L"RegisterClass failed!", L"Error", MB_ICONERROR);
        return 1;
    }

    HWND hMainWnd = CreateWindowW(
        wc.lpszClassName,
        L"DartSyncGUI",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 520,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hMainWnd) {
        MessageBoxW(nullptr, L"CreateWindow failed!", L"Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}

// Folder picker with SHBrowseForFolderW
void PickFolder(std::wstring& outFolder)
{
    BROWSEINFOW bi = {0};
    bi.lpszTitle  = L"Select a Folder";
    bi.ulFlags    = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t buf[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, buf)) {
            outFolder = buf;
        }
        CoTaskMemFree(pidl);
    }
}

// Update label to "Chosen: X:\somepath"
void UpdateChosenPathLabel(HWND labelHwnd, const std::wstring& path)
{
    std::wstring text = L"Chosen: " + path;
    SetWindowTextW(labelHwnd, text.c_str());
}

// Radio button logic
void OnRadioFrequency(HWND radioClicked)
{
    if (radioClicked == hOnceRadio) {
        gFrequency = L"once";
    } else if (radioClicked == hDailyRadio) {
        gFrequency = L"daily";
    } else if (radioClicked == hMonthlyRadio) {
        gFrequency = L"monthly";
    }
}

// Build the command from user picks, call BackupManager
void BuildAndRunCommand()
{
    if (gSourcePath.empty() || gDestPath.empty()) {
        std::cout << "Error: You must pick both source and destination.\n";
        return;
    }

    std::string sourceNarrow(gSourcePath.begin(), gSourcePath.end());
    std::string destNarrow(gDestPath.begin(), gDestPath.end());

    if (gFrequency == L"once") {
        std::cout << "Running one-time backup...\n";
        std::vector<std::string> fileTypes;
        gBackupManager.backupOnce(sourceNarrow, destNarrow, fileTypes, "", 0);
    }
    else if (gFrequency == L"daily") {
        std::cout << "Running daily scheduled backup...\n";
        std::vector<std::string> fileTypes;
        gBackupManager.backupScheduled(sourceNarrow, destNarrow, fileTypes, "",
                                       0, "daily", 0);
    }
    else if (gFrequency == L"monthly") {
        std::cout << "Running monthly scheduled backup...\n";
        std::vector<std::string> fileTypes;
        gBackupManager.backupScheduled(sourceNarrow, destNarrow, fileTypes, "",
                                       0, "monthly", 0);
    }
    else {
        std::cout << "No valid frequency selected.\n";
    }
}

// The window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        {
            // Title
            hTitleStatic = CreateWindowW(
                L"STATIC", L"DartSyncGUI",
                WS_CHILD | WS_VISIBLE,
                20, 10, 200, 20,
                hWnd, nullptr, nullptr, nullptr
            );

            // Source row
            hSourceLabel = CreateWindowW(
                L"STATIC", L"Select Source Folder",
                WS_CHILD | WS_VISIBLE,
                20, 50, 150, 20,
                hWnd, nullptr, nullptr, nullptr
            );
            hSourcePickBtn = CreateWindowW(
                L"BUTTON", L"pick",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                180, 45, 60, 30,
                hWnd, (HMENU)101, nullptr, nullptr
            );
            hSourceChosenLbl = CreateWindowW(
                L"STATIC", L"Chosen: ",
                WS_CHILD | WS_VISIBLE,
                250, 50, 300, 20,
                hWnd, nullptr, nullptr, nullptr
            );

            // Dest row
            hDestLabel = CreateWindowW(
                L"STATIC", L"Select Destination Folder",
                WS_CHILD | WS_VISIBLE,
                20, 90, 150, 20,
                hWnd, nullptr, nullptr, nullptr
            );
            hDestPickBtn = CreateWindowW(
                L"BUTTON", L"pick",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                180, 85, 60, 30,
                hWnd, (HMENU)102, nullptr, nullptr
            );
            hDestChosenLbl = CreateWindowW(
                L"STATIC", L"Chosen: ",
                WS_CHILD | WS_VISIBLE,
                250, 90, 300, 20,
                hWnd, nullptr, nullptr, nullptr
            );

            // Backup frequency
            hBackupFreqLabel = CreateWindowW(
                L"STATIC", L"Backup Frequency",
                WS_CHILD | WS_VISIBLE,
                20, 140, 120, 20,
                hWnd, nullptr, nullptr, nullptr
            );

            // Radio: once, daily, monthly
            hOnceRadio = CreateWindowW(
                L"BUTTON", L"Once",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                20, 170, 80, 20,
                hWnd, (HMENU)201, nullptr, nullptr
            );
            hDailyRadio = CreateWindowW(
                L"BUTTON", L"Daily",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                120, 170, 80, 20,
                hWnd, (HMENU)202, nullptr, nullptr
            );
            hMonthlyRadio = CreateWindowW(
                L"BUTTON", L"Monthly",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                220, 170, 80, 20,
                hWnd, (HMENU)203, nullptr, nullptr
            );

            // "Once" is default
            SendMessageW(hOnceRadio, BM_SETCHECK, BST_CHECKED, 0);

            // Start backup button
            CreateWindowW(
                L"BUTTON", L"Start Backup",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                320, 165, 100, 30,
                hWnd, (HMENU)301, nullptr, nullptr
            );

            // Console label
            hConsoleLabel = CreateWindowW(
                L"STATIC", L"Console output:",
                WS_CHILD | WS_VISIBLE,
                20, 210, 100, 20,
                hWnd, nullptr, nullptr, nullptr
            );

            // Console multiline
            hConsoleOutput = CreateWindowW(
                L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL | WS_BORDER,
                20, 240, 560, 200,
                hWnd, nullptr, nullptr, nullptr
            );

            // Redirect std::cout
            static EditStreamBuf buf(hConsoleOutput);
            gEditBuf = &buf;
            std::cout.rdbuf(gEditBuf);

            std::cout << "Welcome to DartSyncGUI!\n"
                      << "Pick your source & destination, choose frequency, then Start Backup.\n";
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            if (wmId == 101) {
                // Source pick
                PickFolder(gSourcePath);
                UpdateChosenPathLabel(hSourceChosenLbl, gSourcePath);
            }
            else if (wmId == 102) {
                // Dest pick
                PickFolder(gDestPath);
                UpdateChosenPathLabel(hDestChosenLbl, gDestPath);
            }
            else if (wmId == 201 || wmId == 202 || wmId == 203) {
                OnRadioFrequency((HWND)lParam);
            }
            else if (wmId == 301) {
                // Start backup
                BuildAndRunCommand();
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}
