#include <windows.h>
#include <shlobj.h>        // For SHBrowseForFolderW
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>          // For std::thread
#include <vector>
#include "BackupManager.h"
#include "ConsoleRedirect.h"

#pragma comment(lib, "shell32.lib") // might be needed for SHBrowseForFolderW

// GUI controls
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

static HWND hFileTypesLabel   = nullptr;
static HWND hFileTypesEdit    = nullptr;  // new
static HWND hMaxSizeLabel     = nullptr;
static HWND hMaxSizeEdit      = nullptr;  // new

static HWND hConsoleLabel     = nullptr;
static HWND hConsoleOutput    = nullptr;

// Global vars
static std::wstring gSourcePath;
static std::wstring gDestPath;
static std::wstring gFrequency = L"once"; // default
static BackupManager gBackupManager;

static EditStreamBuf* gEditBuf = nullptr;

// Fwd declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void PickFolder(std::wstring& outFolder);
void UpdateChosenPathLabel(HWND labelHwnd, const std::wstring& path);
void OnRadioFrequency(HWND radioClicked);
void BuildAndRunCommand();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    CoInitialize(nullptr);

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
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 560,
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

// Browse for folder
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

void UpdateChosenPathLabel(HWND labelHwnd, const std::wstring& path)
{
    std::wstring text = L"Chosen: " + path;
    SetWindowTextW(labelHwnd, text.c_str());
}

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

// Convert wide string with space-separated extensions into vector<string>
std::vector<std::string> parseExtensions(const std::wstring& wext)
{
    std::vector<std::string> exts;
    if (wext.empty()) return exts; // empty => no filter => all files

    // naive split on spaces
    std::wstringstream wss(wext);
    std::wstring token;
    while (wss >> token) {
        // Convert to narrow
        std::string narrow(token.begin(), token.end());
        exts.push_back(narrow);
    }
    return exts;
}

// The function that calls the backup in the background
void BuildAndRunCommand()
{
    if (gSourcePath.empty() || gDestPath.empty()) {
        std::cout << "Error: You must pick both source and destination.\n";
        return;
    }

    // Grab file extension filter from the UI
    wchar_t extBuf[512];
    GetWindowTextW(hFileTypesEdit, extBuf, 512);
    std::wstring wExtStr(extBuf);
    auto fileTypes = parseExtensions(wExtStr);

    // Grab max size
    wchar_t sizeBuf[64];
    GetWindowTextW(hMaxSizeEdit, sizeBuf, 64);
    size_t maxFileSizeMB = 0;
    if (wcslen(sizeBuf) > 0) {
        // Convert to narrow and parse
        std::wstring ws(sizeBuf);
        std::string narrow(ws.begin(), ws.end());
        try {
            maxFileSizeMB = std::stoul(narrow);
        } catch(...) {
            std::cout << "Warning: invalid max size. Using 0 (no limit).\n";
        }
    }

    std::string sourceNarrow(gSourcePath.begin(), gSourcePath.end());
    std::string destNarrow(gDestPath.begin(), gDestPath.end());

    if (gFrequency == L"once") {
        std::cout << "Running one-time backup...\n";
        gBackupManager.backupOnce(sourceNarrow, destNarrow,
                                  fileTypes, "", maxFileSizeMB);
    }
    else if (gFrequency == L"daily") {
        std::cout << "Running daily scheduled backup...\n";
        gBackupManager.backupScheduled(sourceNarrow, destNarrow,
                                       fileTypes, "", maxFileSizeMB,
                                       "daily", 0);
    }
    else if (gFrequency == L"monthly") {
        std::cout << "Running monthly scheduled backup...\n";
        gBackupManager.backupScheduled(sourceNarrow, destNarrow,
                                       fileTypes, "", maxFileSizeMB,
                                       "monthly", 0);
    }
    else {
        std::cout << "No valid frequency selected.\n";
    }
}

// The Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
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
                250, 50, 400, 20,
                hWnd, nullptr, nullptr, nullptr
            );

            // Destination row
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
                250, 90, 400, 20,
                hWnd, nullptr, nullptr, nullptr
            );

            // Backup frequency
            hBackupFreqLabel = CreateWindowW(
                L"STATIC", L"Backup Frequency",
                WS_CHILD | WS_VISIBLE,
                20, 140, 120, 20,
                hWnd, nullptr, nullptr, nullptr
            );

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
            // Default "Once"
            SendMessageW(hOnceRadio, BM_SETCHECK, BST_CHECKED, 0);

            // File Types row
            hFileTypesLabel = CreateWindowW(
                L"STATIC", L"File Extensions (e.g. .dll .txt):",
                WS_CHILD | WS_VISIBLE,
                20, 210, 220, 20,
                hWnd, nullptr, nullptr, nullptr
            );
            hFileTypesEdit = CreateWindowW(
                L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                240, 205, 160, 25,
                hWnd, nullptr, nullptr, nullptr
            );

            // Max Size row
            hMaxSizeLabel = CreateWindowW(
                L"STATIC", L"Max File Size MB (0 = no limit):",
                WS_CHILD | WS_VISIBLE,
                20, 245, 220, 20,
                hWnd, nullptr, nullptr, nullptr
            );
            hMaxSizeEdit = CreateWindowW(
                L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                240, 240, 80, 25,
                hWnd, nullptr, nullptr, nullptr
            );

            // Start Backup button
            CreateWindowW(
                L"BUTTON", L"Start Backup",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                350, 235, 100, 30,
                hWnd, (HMENU)301, nullptr, nullptr
            );

            // Console label
            hConsoleLabel = CreateWindowW(
                L"STATIC", L"Console:",
                WS_CHILD | WS_VISIBLE,
                20, 285, 100, 20,
                hWnd, nullptr, nullptr, nullptr
            );

            // Console multiline
            hConsoleOutput = CreateWindowW(
                L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY |
                WS_VSCROLL | WS_BORDER,
                20, 310, 640, 200,
                hWnd, nullptr, nullptr, nullptr
            );

            // Redirect std::cout
            static EditStreamBuf buf(hConsoleOutput);
            gEditBuf = &buf;
            std::cout.rdbuf(gEditBuf);

            std::cout << "Welcome to DartSyncGUI!\n"
                      << "Pick source/dest, set frequency, optionally set extensions or max file size, then Start.\n";
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
                // Start Backup in background thread
                std::thread worker([](){
                    BuildAndRunCommand();
                });
                worker.detach();
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
