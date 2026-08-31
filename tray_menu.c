#include "tray_menu.h"
#include "utils.h"
#include "network_tx.h"
#include "layout.h"
#include <commctrl.h>
#include <process.h>
#include <stdio.h>

HWND g_hWndDropZone = NULL;
extern HWND g_hWndMain;

// Window procedure for the floating Quick Drop Target
static LRESULT CALLBACK DropZoneWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            // Fill background with standard button face
            FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
            
            // Draw 3D border and inner focus dashed border
            DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT);
            RECT innerRc = rc;
            InflateRect(&innerRc, -3, -3);
            DrawFocusRect(hdc, &innerRc);
            
            // Draw App Icon
            HICON hIcon = (HICON)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCE(101), IMAGE_ICON, 32, 32, LR_SHARED);
            if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);
            if (hIcon) {
                DrawIcon(hdc, 10, (rc.bottom - 32) / 2, hIcon);
            }
            
            // Draw text
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
            HFONT hOldFont = (HFONT)SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
            
            RECT textRc1 = { 48, 8, rc.right - 6, 26 };
            DrawTextA(hdc, APP_NAME " Drop Target", -1, &textRc1, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
            
            RECT textRc2 = { 48, 26, rc.right - 6, rc.bottom - 6 };
            DrawTextA(hdc, g_Lang.trayDropFilesHere, -1, &textRc2, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
            
            SelectObject(hdc, hOldFont);
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_NCHITTEST: {
            LRESULT hit = DefWindowProcA(hWnd, msg, wParam, lParam);
            if (hit == HTCLIENT) return HTCAPTION; // Allow moving by dragging anywhere
            return hit;
        }
        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            ProcessDroppedFiles(hDrop);
            char dropMsg[128];
            _snprintf(dropMsg, sizeof(dropMsg), g_Lang.trayFilesQueued, g_sendQueueCount);
            ShowTrayNotification(g_hWndMain, APP_NAME, dropMsg, NIIF_INFO);
            return 0;
        }
        case WM_RBUTTONUP: {
            ShowTrayContextMenu(g_hWndMain);
            return 0;
        }
        case WM_LBUTTONDBLCLK: {
            ShowWindow(g_hWndMain, SW_RESTORE);
            SetForegroundWindow(g_hWndMain);
            return 0;
        }
        default:
            return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
}

// Creates the floating Quick Drop Target window
void InitDropZoneWindow(HINSTANCE hInstance, HWND hParent) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = DropZoneWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "LocalSendDropZone";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassA(&wc);

    RECT rcWork;
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &rcWork, 0);
    int w = 220, h = 65;
    int x = rcWork.right - w - 20;
    int y = rcWork.bottom - h - 20;

    g_hWndDropZone = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_ACCEPTFILES,
        "LocalSendDropZone",
        "LocalSend Drop Target",
        WS_POPUP | WS_BORDER,
        x, y, w, h,
        NULL, NULL, hInstance, NULL
    );
    DragAcceptFiles(g_hWndDropZone, TRUE);
}

// Shows or hides the Quick Drop Target window
void ShowDropZoneWindow(BOOL bShow) {
    if (g_hWndDropZone) {
        ShowWindow(g_hWndDropZone, bShow ? SW_SHOW : SW_HIDE);
        if (bShow) InvalidateRect(g_hWndDropZone, NULL, TRUE);
    }
}

// Toggles visibility of Quick Drop Target window
void ToggleDropZoneWindow(void) {
    if (g_hWndDropZone) {
        BOOL isVis = IsWindowVisible(g_hWndDropZone);
        ShowDropZoneWindow(!isVis);
    }
}

// Initializes the system tray notification icon
void InitTrayIcon(HWND hWnd, HINSTANCE hInstance) {
    NOTIFYICONDATAA nid;
    memset(&nid, 0, sizeof(NOTIFYICONDATAA));
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON_MSG;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    strncpy(nid.szTip, APP_NAME, sizeof(nid.szTip) - 1);
    Shell_NotifyIconA(NIM_ADD, &nid);
}

// Removes the system tray notification icon
void RemoveTrayIcon(HWND hWnd) {
    NOTIFYICONDATAA nid;
    memset(&nid, 0, sizeof(NOTIFYICONDATAA));
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    Shell_NotifyIconA(NIM_DELETE, &nid);
}

// Displays a balloon notification popup above the system tray icon
void ShowTrayNotification(HWND hWnd, const char* title, const char* message, DWORD infoFlags) {
    NOTIFYICONDATAA nid;
    memset(&nid, 0, sizeof(NOTIFYICONDATAA));
    nid.cbSize = sizeof(NOTIFYICONDATAA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO;
    strncpy(nid.szInfo, message ? message : "", sizeof(nid.szInfo) - 1);
    strncpy(nid.szInfoTitle, title ? title : APP_NAME, sizeof(nid.szInfoTitle) - 1);
    nid.dwInfoFlags = infoFlags;
    Shell_NotifyIconA(NIM_MODIFY, &nid);
}

// Displays the rich right-click context menu for the system tray icon
void ShowTrayContextMenu(HWND hWnd) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    // 1. Quick Add Actions
    AppendMenuA(hMenu, MF_STRING, IDM_TRAY_ADD_FILES, g_Lang.trayAddFiles);
    AppendMenuA(hMenu, MF_STRING, IDM_TRAY_ADD_FOLDER, g_Lang.trayAddFolder);
    AppendMenuA(hMenu, MF_STRING, IDM_TRAY_PASTE, g_Lang.trayPaste);

    // 2. Quick Drop Target toggle
    UINT dropZoneFlags = MF_STRING;
    if (g_hWndDropZone && IsWindowVisible(g_hWndDropZone)) dropZoneFlags |= MF_CHECKED;
    AppendMenuA(hMenu, dropZoneFlags, IDM_TRAY_DROP_ZONE, g_Lang.trayDropZone);

    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);

    // 3. Queued files submenu (if any)
    if (g_sendQueueCount > 0) {
        HMENU hSubFiles = CreatePopupMenu();
        char filesTitle[128];
        _snprintf(filesTitle, sizeof(filesTitle), g_Lang.trayFilesCount, g_sendQueueCount);

        for (int i = 0; i < g_sendQueueCount && i < MAX_SEND_FILES; i++) {
            char sizeStr[32] = {0};
            FormatByteSizeString(g_sendQueue[i].fileSize, sizeStr, sizeof(sizeStr));
            char fileLabel[320];
            _snprintf(fileLabel, sizeof(fileLabel), "%d. %s (%s)", i + 1, g_sendQueue[i].fileName, sizeStr);
            AppendMenuA(hSubFiles, MF_STRING | MF_GRAYED, 0, fileLabel);
        }
        AppendMenuA(hSubFiles, MF_SEPARATOR, 0, NULL);
        AppendMenuA(hSubFiles, MF_STRING, IDM_TRAY_CLEAR_FILES, g_Lang.trayClearFiles);
        AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSubFiles, filesTitle);
        AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    }

    // 4. Send to device submenu listing all discovered peers
    HMENU hSubDevices = CreatePopupMenu();
    int devCount = hWndListDevices ? ListView_GetItemCount(hWndListDevices) : 0;
    if (devCount == 0) {
        AppendMenuA(hSubDevices, MF_STRING | MF_GRAYED, 0, g_Lang.trayNoDevices);
    } else {
        for (int i = 0; i < devCount && i < (IDM_TRAY_DEVICE_MAX - IDM_TRAY_DEVICE_START); i++) {
            LVITEMA lvi = {0};
            lvi.iItem = i;
            lvi.mask = LVIF_PARAM;
            ListView_GetItem(hWndListDevices, &lvi);
            RemoteDevice* dev = (RemoteDevice*)lvi.lParam;
            if (dev) {
                const char* lastDot = strrchr(dev->ipAddress, '.');
                const char* devHashtag = lastDot ? (lastDot + 1) : dev->ipAddress;
                const char* locType = GetLocalizedDeviceType(dev->deviceType);
                char devLabel[256];
                _snprintf(devLabel, sizeof(devLabel), "%s (#%s, %s)", dev->alias, devHashtag, locType);
                AppendMenuA(hSubDevices, MF_STRING, IDM_TRAY_DEVICE_START + i, devLabel);
            }
        }
    }
    AppendMenuA(hSubDevices, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hSubDevices, MF_STRING, IDM_TRAY_SCAN_AGAIN, g_Lang.searchAgain);
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSubDevices, g_Lang.traySendToDevice);

    // Separator before window actions
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);

    // 5. Window management options: Open, Settings, Exit
    AppendMenuA(hMenu, MF_STRING, IDM_TRAY_OPEN, g_Lang.trayOpen);
    AppendMenuA(hMenu, MF_STRING, IDM_TRAY_SETTINGS, g_Lang.traySettings);
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, IDM_TRAY_EXIT, g_Lang.trayExit);

    // Display context menu at current cursor position
    SetForegroundWindow(hWnd);
    POINT pt;
    GetCursorPos(&pt);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);

    if (cmd > 0) {
        HandleTrayCommand(hWnd, cmd);
    }
}

// Executes actions triggered from the system tray context menu
void HandleTrayCommand(HWND hWnd, int cmdId) {
    if (cmdId == IDM_TRAY_OPEN) {
        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
    } else if (cmdId == IDM_TRAY_SETTINGS) {
        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
        if (hWndTab) {
            TabCtrl_SetCurSel(hWndTab, 2);
            UpdatePageVisibility();
        }
    } else if (cmdId == IDM_TRAY_EXIT) {
        DestroyWindow(hWnd);
    } else if (cmdId == IDM_TRAY_CLEAR_FILES) {
        ClearSendQueue();
    } else if (cmdId == IDM_TRAY_SCAN_AGAIN) {
        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_BTN_SEARCH_AGAIN, 0), 0);
    } else if (cmdId == IDM_TRAY_ADD_FILES) {
        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_BTN_SEND_FILE, 0), 0);
    } else if (cmdId == IDM_TRAY_ADD_FOLDER) {
        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_BTN_SEND_FOLDER, 0), 0);
    } else if (cmdId == IDM_TRAY_PASTE) {
        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_BTN_SEND_PASTE, 0), 0);
    } else if (cmdId == IDM_TRAY_DROP_ZONE) {
        ToggleDropZoneWindow();
    } else if (cmdId >= IDM_TRAY_DEVICE_START && cmdId <= IDM_TRAY_DEVICE_MAX) {
        int devIdx = cmdId - IDM_TRAY_DEVICE_START;
        int devCount = hWndListDevices ? ListView_GetItemCount(hWndListDevices) : 0;
        if (devIdx < devCount) {
            LVITEMA lvi = {0};
            lvi.iItem = devIdx;
            lvi.mask = LVIF_PARAM;
            ListView_GetItem(hWndListDevices, &lvi);
            RemoteDevice* dev = (RemoteDevice*)lvi.lParam;

            if (dev) {
                if (g_sendQueueCount == 0) {
                    // Open to Send tab and inform user to drop files
                    ShowWindow(hWnd, SW_RESTORE);
                    SetForegroundWindow(hWnd);
                    if (hWndTab) {
                        TabCtrl_SetCurSel(hWndTab, 1);
                        UpdatePageVisibility();
                    }
                    ListView_SetCheckState(hWndListDevices, devIdx, TRUE);
                    ShowTrayNotification(hWnd, APP_NAME, g_Lang.trayDropFilesHere, NIIF_INFO);
                } else {
                    // Initiate transfer directly from tray
                    SendSessionContext* txCtx = (SendSessionContext*)malloc(sizeof(SendSessionContext));
                    if (txCtx) {
                        memset(txCtx, 0, sizeof(SendSessionContext));
                        strncpy(txCtx->targets[0].ipAddress, dev->ipAddress, 15);
                        txCtx->targets[0].ipAddress[15] = '\0';
                        txCtx->targets[0].port = dev->port;
                        strncpy(txCtx->targets[0].alias, dev->alias, sizeof(txCtx->targets[0].alias) - 1);
                        txCtx->targets[0].isHttps = dev->isHttps;
                        txCtx->targetCount = 1;
                        txCtx->fileCount = g_sendQueueCount;
                        memcpy(txCtx->files, g_sendQueue, sizeof(FileToSend) * g_sendQueueCount);

                        char notifMsg[256];
                        _snprintf(notifMsg, sizeof(notifMsg), g_Lang.traySendingNotification, g_sendQueueCount, dev->alias);
                        ShowTrayNotification(hWnd, APP_NAME, notifMsg, NIIF_INFO);

                        char* copyMsg = (char*)malloc(256);
                        if (copyMsg) {
                            _snprintf(copyMsg, 256, g_Lang.msgConnectingTo, dev->alias);
                            PostMessage(hWnd, WM_SEND_STATUS_UPDATE, 1, (LPARAM)copyMsg);
                        }

                        DWORD thId = 0;
                        HANDLE hTxThread = CreateThread(NULL, 0, StartSendSessionThread, (LPVOID)txCtx, 0, &thId);
                        if (hTxThread) CloseHandle(hTxThread);
                    }
                }
            }
        }
    }
}
