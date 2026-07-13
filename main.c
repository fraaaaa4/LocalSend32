#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <shlwapi.h>

#ifndef SHACF_FILESYS_DIRS
#define SHACF_FILESYS_DIRS 0x00000020
#endif

#include "utils.h"
#include "network_udp.h"
#include "network_tcp.h"
#include "network_tx.h"
#include "cert.h"
#include "tls_layer.h"
#include "dialogs.h"
#include "settings.h"
#include "ui_creator.h"




__declspec(dllimport) UINT WINAPI PrivateExtractIconsA(LPCSTR szFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT *piconid, UINT nIcons, UINT flags);


void ApplyWindowFont(HWND hWndChild);
void ApplyLargeFont(HWND hWndChild);
void ApplyStatusFont(HWND hWndChild);
void UpdateStatusIcon(StatusIconType type);
void UpdateSendStatusIcon(StatusIconType type);
void AddTooltip(HWND hCtrl, char* text);
void UpdateTooltipText(HWND hCtrl, char* text);
void GetDeviceNetworkInfo(char* outBuffer, size_t maxLen);
int GetSystemIconIndex(const char* fileName);
void ShowSettingsSubPage(int subTab);
void UpdatePageVisibility(void);
void UpdateUITexts(void);
void ResizeControls(HWND hWnd, int width, int height);
int GetDeviceIconIndex(const char* deviceType);
void AddFileToSendQueue(const char* filePath, const char* fileName, long long fileSize);
void UpdateSendButtonsState(void);


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void sendDiscoveryShout(SOCKET mySocket);

HWND g_hWndMain = NULL;
HWND hWndTab = NULL, hWndSettingsTab = NULL;
HWND hWndStatus = NULL, hWndGroupBox = NULL, hWndInfoText = NULL, hWndListView = NULL;
HWND hWndStatusIcon = NULL, hWndBtnCloseStatus = NULL;

HFONT hStatusFont = NULL, hNormalFont = NULL, hLargeFont = NULL;
HIMAGELIST hSystemImageList = NULL, hDeviceImageList = NULL;

FileToSend g_sendQueue[MAX_SEND_FILES];
int g_sendQueueCount = 0;

char g_IniPath[MAX_PATH] = {0};
HWND hWndDeviceNameTitle = NULL, hWndDeviceName = NULL, hWndBtnEditDevice = NULL;
HWND hWndEditDeviceBox = NULL, hWndBtnSaveDevice = NULL, hWndBtnCancelDevice = NULL;
HWND hWndBtnSndFile = NULL, hWndBtnSndFolder = NULL, hWndBtnSndText = NULL, hWndBtnSndPaste = NULL;
HWND hWndLblSendFiles = NULL, hWndListSendFiles = NULL, hWndBtnCleanFiles = NULL, hWndBtnSendFiles = NULL;
HWND hWndLblDevices = NULL, hWndListDevices = NULL, hWndBtnSearchAgain = NULL, hWndBtnSendManual = NULL;
HWND hWndToolTip = NULL, hWndSearchLoading = NULL;
HWND hWndSendProgress = NULL, hWndSendStatusTxt = NULL, hWndSendStatusIcon = NULL;

HWND hWndCheckSavePos = NULL, hWndCheckMinClose = NULL, hWndCheckTopmost = NULL;
HWND hWndCheckQuickSave = NULL;
HWND hWndLblLanguage = NULL, hWndComboLanguage = NULL;
HWND hWndRecvLbl = NULL, hWndRecvRadApp = NULL, hWndRecvRadDl = NULL, hWndRecvRadCustom = NULL, hWndRecvTxtPath = NULL, hWndRecvBtnBrowse = NULL;
HWND hWndBtnGithub = NULL, hWndBtnSupport = NULL, hWndLabelAbout = NULL, hWndIconStatic = NULL;

HWND hWndCheckReqPin = NULL; HWND hWndEditPinCode = NULL;
HWND hWndEditDiscTimeout = NULL; HWND hWndEditMulticast = NULL;
HWND hWndCheckEncryption = NULL; HWND hWndComboDevType = NULL;
HWND hWndEditDevModel = NULL; HWND hWndEditPort = NULL;

HWND hWndLblReqPin = NULL; HWND hWndLblPinCode = NULL;
HWND hWndLblDiscTimeout = NULL; HWND hWndLblMulticast = NULL;
HWND hWndLblDevType = NULL; HWND hWndLblDevModel = NULL; HWND hWndLblPort = NULL;

SOCKET g_mySocket = INVALID_SOCKET;
BOOL g_bEditingDeviceName = FALSE;
char g_MyDeviceName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
NOTIFYICONDATAA nid = {0};
LoggedTransfer g_transfers[100];
int g_transfersCount = 0, g_SplitterPos = 360;
bool g_bDraggingSplitter = false;




// Assigns standard UI font to the given child control
void ApplyWindowFont(HWND hWndChild) {
    if (hNormalFont == NULL) {
        NONCLIENTMETRICSA ncm;
        memset(&ncm, 0, sizeof(NONCLIENTMETRICSA));
        ncm.cbSize = sizeof(NONCLIENTMETRICSA);
        if (!SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), &ncm, 0)) {
            // Try Windows XP/RT-compatible size (excluding iPaddedBorderWidth, which is a 4-byte integer)
            ncm.cbSize = sizeof(NONCLIENTMETRICSA) - sizeof(int);
            if (!SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0)) {
                hNormalFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            }
        }
        if (hNormalFont == NULL) {
            hNormalFont = CreateFontIndirectA(&ncm.lfMessageFont);
        }
        if (hNormalFont == NULL) {
            hNormalFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
    }
    SendMessage(hWndChild, WM_SETFONT, (WPARAM)hNormalFont, TRUE);
}

// Applies a larger text font to highlighted labels
void ApplyLargeFont(HWND hWndChild) {
    if (hLargeFont == NULL) {
        NONCLIENTMETRICSA ncm;
        memset(&ncm, 0, sizeof(NONCLIENTMETRICSA));
        ncm.cbSize = sizeof(NONCLIENTMETRICSA);
        BOOL ok = SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), &ncm, 0);
        if (!ok) {
            ncm.cbSize = sizeof(NONCLIENTMETRICSA) - sizeof(int);
            ok = SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
        }

        int height = -16;
        const char* faceName = "MS Shell Dlg";
        if (ok) {
            height = ncm.lfMessageFont.lfHeight * 1.5;
            faceName = ncm.lfMessageFont.lfFaceName;
        }

        hLargeFont = CreateFontA(height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, faceName);
        if (hLargeFont == NULL) {
            hLargeFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
    }
    SendMessage(hWndChild, WM_SETFONT, (WPARAM)hLargeFont, TRUE);
}

// Sets status-specific fonts with custom weights on controls
void ApplyStatusFont(HWND hWndChild) {
    if (hStatusFont == NULL) {
        NONCLIENTMETRICSA ncm;
        memset(&ncm, 0, sizeof(NONCLIENTMETRICSA));
        ncm.cbSize = sizeof(NONCLIENTMETRICSA);
        BOOL ok = SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), &ncm, 0);
        if (!ok) {
            ncm.cbSize = sizeof(NONCLIENTMETRICSA) - sizeof(int);
            ok = SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
        }

        int height = -10;
        if (ok) {
            height = ncm.lfMessageFont.lfHeight - 2;
        }

        hStatusFont = CreateFontA(height, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        if (hStatusFont == NULL) {
            hStatusFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
    }
    SendMessage(hWndChild, WM_SETFONT, (WPARAM)hStatusFont, TRUE);
}

// Swaps the visual status icon on the receive progress page
void UpdateStatusIcon(StatusIconType type) {
    char sysPath[MAX_PATH]; GetSystemDirectoryA(sysPath, MAX_PATH); strcat(sysPath, "\\shell32.dll");
    int iconIndex = (type == ICON_WAITING) ? 54 : (type == ICON_TRANSFERRING) ? 89 : (type == ICON_SUCCESS) ? 301 : 131;
    HICON hIcon = NULL; UINT iconId = 0;
    if (PrivateExtractIconsA(sysPath, iconIndex, 16, 16, &hIcon, &iconId, 1, 0) > 0 && hIcon != NULL) {
        HICON hOldIcon = (HICON)SendMessage(hWndStatusIcon, STM_SETICON, (WPARAM)hIcon, 0); if (hOldIcon) DestroyIcon(hOldIcon);
    }
}

// Swaps the status icon on the sending page
void UpdateSendStatusIcon(StatusIconType type) {
    char sysPath[MAX_PATH]; GetSystemDirectoryA(sysPath, MAX_PATH); strcat(sysPath, "\\shell32.dll");
    int iconIndex = (type == ICON_WAITING) ? 54 : (type == ICON_TRANSFERRING) ? 89 : (type == ICON_SUCCESS) ? 301 : 131;
    HICON hIcon = NULL; UINT iconId = 0;
    if (PrivateExtractIconsA(sysPath, iconIndex, 16, 16, &hIcon, &iconId, 1, 0) > 0 && hIcon != NULL) {
        HICON hOldIcon = (HICON)SendMessage(hWndSendStatusIcon, STM_SETICON, (WPARAM)hIcon, 0); if (hOldIcon) DestroyIcon(hOldIcon);
    }
}

// Associates a tooltip helper with a control
void AddTooltip(HWND hCtrl, char* text) {
    if (!hWndToolTip) {
        hWndToolTip = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, g_hWndMain, NULL, GetModuleHandle(NULL), NULL);
        SetWindowPos(hWndToolTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    TOOLINFOA ti = {0}; ti.cbSize = sizeof(TOOLINFOA); ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND; ti.hwnd = g_hWndMain; ti.uId = (UINT_PTR)hCtrl; ti.lpszText = text;
    SendMessage(hWndToolTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
}

// Dynamically changes tooltip text for localized strings
void UpdateTooltipText(HWND hCtrl, char* text) {
    if (!hWndToolTip) return;
    TOOLINFOA ti = {0}; ti.cbSize = sizeof(TOOLINFOA); ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND; ti.hwnd = g_hWndMain; ti.uId = (UINT_PTR)hCtrl; ti.lpszText = text;
    SendMessage(hWndToolTip, TTM_UPDATETIPTEXTA, 0, (LPARAM)&ti);
}

// Resolves device local hostname and IP address info
void GetDeviceNetworkInfo(char* outBuffer, size_t maxLen) {
    char ipStr[32] = "127.0.0.1"; char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) == 0) {
        struct hostent* phe = gethostbyname(hostName);
        if (phe && phe->h_addr_list[0]) {
            struct in_addr addr; memcpy(&addr, phe->h_addr_list[0], sizeof(struct in_addr)); strcpy(ipStr, inet_ntoa(addr));
        }
    }
    const char* lastDot = strrchr(ipStr, '.');
    const char* myHashtag = lastDot ? (lastDot + 1) : ipStr;
    _snprintf(outBuffer, maxLen, "Status: Ready to receive\r\nHashtag: #%s\r\nIP Address: %s\r\nActive Port: %d", myHashtag, ipStr, 53317);
}

// Finds and registers file icon from shell registry
int GetSystemIconIndex(const char* fileName) {
    SHFILEINFOA sfi = {0};
    HIMAGELIST hInstIL = (HIMAGELIST)SHGetFileInfoA(fileName, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    if (hSystemImageList == NULL && hInstIL != NULL) { hSystemImageList = hInstIL; ListView_SetImageList(hWndListView, hSystemImageList, LVSIL_SMALL); ListView_SetImageList(hWndListSendFiles, hSystemImageList, LVSIL_SMALL); }
    return sfi.iIcon;
}

// Toggles visibility of settings subpages based on sub-tab index
void ShowSettingsSubPage(int subTab) {
    int hide = SW_HIDE;
    ShowWindow(hWndCheckSavePos, hide); ShowWindow(hWndCheckMinClose, hide); ShowWindow(hWndCheckTopmost, hide);
    ShowWindow(hWndLblLanguage, hide); ShowWindow(hWndComboLanguage, hide);
    ShowWindow(hWndRecvLbl, hide); ShowWindow(hWndRecvRadApp, hide); ShowWindow(hWndRecvRadDl, hide); ShowWindow(hWndRecvRadCustom, hide); ShowWindow(hWndRecvTxtPath, hide); ShowWindow(hWndRecvBtnBrowse, hide); ShowWindow(hWndCheckQuickSave, hide);
    
    ShowWindow(hWndCheckReqPin, hide); ShowWindow(hWndEditPinCode, hide);
    ShowWindow(hWndEditDiscTimeout, hide); ShowWindow(hWndEditMulticast, hide);
    ShowWindow(hWndCheckEncryption, hide); ShowWindow(hWndComboDevType, hide);
    ShowWindow(hWndEditDevModel, hide); ShowWindow(hWndEditPort, hide);
    ShowWindow(hWndLblReqPin, hide); ShowWindow(hWndLblPinCode, hide);
    ShowWindow(hWndLblDiscTimeout, hide); ShowWindow(hWndLblMulticast, hide);
    ShowWindow(hWndLblDevType, hide); ShowWindow(hWndLblDevModel, hide); ShowWindow(hWndLblPort, hide);

    ShowWindow(hWndIconStatic, hide); ShowWindow(hWndLabelAbout, hide); ShowWindow(hWndBtnGithub, hide); ShowWindow(hWndBtnSupport, hide);

    if (TabCtrl_GetCurSel(hWndTab) != 2) return;

    if (subTab == 0) {
        ShowWindow(hWndCheckSavePos, SW_SHOW); ShowWindow(hWndCheckMinClose, SW_SHOW); ShowWindow(hWndCheckTopmost, SW_SHOW);
        ShowWindow(hWndLblLanguage, SW_SHOW); ShowWindow(hWndComboLanguage, SW_SHOW);
    } else if (subTab == 1) {
        ShowWindow(hWndRecvLbl, SW_SHOW); ShowWindow(hWndRecvRadApp, SW_SHOW); ShowWindow(hWndRecvRadDl, SW_SHOW); ShowWindow(hWndRecvRadCustom, SW_SHOW);
        ShowWindow(hWndRecvTxtPath, SW_SHOW); ShowWindow(hWndRecvBtnBrowse, SW_SHOW); ShowWindow(hWndCheckQuickSave, SW_SHOW);
    } else if (subTab == 2) {
        ShowWindow(hWndCheckReqPin, SW_SHOW); ShowWindow(hWndEditPinCode, SW_SHOW);
        ShowWindow(hWndEditDiscTimeout, SW_SHOW); ShowWindow(hWndEditMulticast, SW_SHOW);
        ShowWindow(hWndCheckEncryption, SW_SHOW); ShowWindow(hWndComboDevType, SW_SHOW);
        ShowWindow(hWndEditDevModel, SW_SHOW); ShowWindow(hWndEditPort, SW_SHOW);
        ShowWindow(hWndLblPinCode, SW_SHOW); ShowWindow(hWndLblDiscTimeout, SW_SHOW); 
        ShowWindow(hWndLblMulticast, SW_SHOW); ShowWindow(hWndLblDevType, SW_SHOW); 
        ShowWindow(hWndLblDevModel, SW_SHOW); ShowWindow(hWndLblPort, SW_SHOW);
    } else if (subTab == 3) {
        ShowWindow(hWndIconStatic, SW_SHOW); ShowWindow(hWndLabelAbout, SW_SHOW); ShowWindow(hWndBtnGithub, SW_SHOW); ShowWindow(hWndBtnSupport, SW_SHOW);
    }
}

// Translates all visual labels and controls to the selected language
void UpdateUITexts() {
    if (!hWndTab) return;

    TCITEMA tie; tie.mask = TCIF_TEXT;
    tie.pszText = g_Lang.tabReceive; TabCtrl_SetItem(hWndTab, 0, &tie);
    tie.pszText = g_Lang.tabSend; TabCtrl_SetItem(hWndTab, 1, &tie);
    tie.pszText = g_Lang.tabSettings; TabCtrl_SetItem(hWndTab, 2, &tie);

    if (hWndSettingsTab) {
        tie.pszText = g_Lang.settingsGeneral; TabCtrl_SetItem(hWndSettingsTab, 0, &tie);
        tie.pszText = g_Lang.settingsReceive; TabCtrl_SetItem(hWndSettingsTab, 1, &tie);
        tie.pszText = g_Lang.settingsNetwork; TabCtrl_SetItem(hWndSettingsTab, 2, &tie);
        tie.pszText = g_Lang.settingsOther; TabCtrl_SetItem(hWndSettingsTab, 3, &tie);
    }

    if (hWndCheckSavePos) SetWindowTextA(hWndCheckSavePos, g_Lang.saveWindowPos);
    if (hWndCheckMinClose) SetWindowTextA(hWndCheckMinClose, g_Lang.minimizeToTray);
    if (hWndCheckTopmost) SetWindowTextA(hWndCheckTopmost, g_Lang.alwaysOnTop);
    if (hWndLblLanguage) SetWindowTextA(hWndLblLanguage, g_Lang.language);

    if (hWndDeviceNameTitle) SetWindowTextA(hWndDeviceNameTitle, g_Lang.deviceName);
    if (hWndBtnEditDevice) SetWindowTextA(hWndBtnEditDevice, g_Lang.editBtn);
    if (hWndBtnSaveDevice) SetWindowTextA(hWndBtnSaveDevice, g_Lang.saveBtn);
    if (hWndBtnCancelDevice) SetWindowTextA(hWndBtnCancelDevice, g_Lang.cancelBtn);
    if (hWndGroupBox) SetWindowTextA(hWndGroupBox, g_Lang.networkInfo);
    if (hWndStatus && !IsWindowVisible(hWndBtnCloseStatus)) SetWindowTextA(hWndStatus, g_Lang.waitingForFiles);
    if (hWndBtnCloseStatus) SetWindowTextA(hWndBtnCloseStatus, g_Lang.closeBtn);

    if (hWndRecvLbl) SetWindowTextA(hWndRecvLbl, g_Lang.whenIReceive);
    if (hWndRecvRadApp) SetWindowTextA(hWndRecvRadApp, g_Lang.saveAppPath);
    if (hWndRecvRadDl) SetWindowTextA(hWndRecvRadDl, g_Lang.saveDownloads);
    if (hWndRecvRadCustom) SetWindowTextA(hWndRecvRadCustom, g_Lang.saveCustomPath);
    if (hWndRecvBtnBrowse) SetWindowTextA(hWndRecvBtnBrowse, g_Lang.browse);
    if (hWndCheckQuickSave) SetWindowTextA(hWndCheckQuickSave, g_Lang.quickSave);

    if (hWndCheckReqPin) SetWindowTextA(hWndCheckReqPin, g_Lang.requirePin);
    if (hWndLblPinCode) SetWindowTextA(hWndLblPinCode, g_Lang.pinCode);
    if (hWndLblPort) SetWindowTextA(hWndLblPort, g_Lang.port);

    if (hWndLabelAbout) {
        char aboutTxt[512]; _snprintf(aboutTxt, sizeof(aboutTxt), "%s\r\nVersion: 1.1.0\r\nPublisher: Fratta\r\n\r\n%s", APP_NAME, APP_ABOUT);
        SetWindowTextA(hWndLabelAbout, aboutTxt);
    }
    if (hWndBtnGithub) SetWindowTextA(hWndBtnGithub, g_Lang.aboutBtn);
    if (hWndBtnSupport) SetWindowTextA(hWndBtnSupport, g_Lang.supportBtn);

    if (hWndBtnCleanFiles) SetWindowTextA(hWndBtnCleanFiles, g_Lang.cleanFiles);
    if (hWndBtnSendFiles) SetWindowTextA(hWndBtnSendFiles, g_Lang.sendFiles);
    if (hWndLblDevices) SetWindowTextA(hWndLblDevices, g_Lang.nearbyDevices);
    if (hWndBtnSearchAgain) SetWindowTextA(hWndBtnSearchAgain, g_Lang.searchAgain);
    if (hWndBtnSendManual) SetWindowTextA(hWndBtnSendManual, g_Lang.sendManually);

    // Update list view columns
    LVCOLUMNA lvc; lvc.mask = LVCF_TEXT;
    if (hWndListView) {
        lvc.pszText = g_Lang.colFile; ListView_SetColumn(hWndListView, LV_COL_NAME, &lvc);
        lvc.pszText = g_Lang.colSize; ListView_SetColumn(hWndListView, LV_COL_SIZE, &lvc);
        lvc.pszText = g_Lang.colDate; ListView_SetColumn(hWndListView, LV_COL_DATE, &lvc);
        lvc.pszText = g_Lang.colProgress; ListView_SetColumn(hWndListView, LV_COL_PROGRESS, &lvc);

        LVGROUP lvg = {0}; lvg.cbSize = sizeof(LVGROUP); lvg.mask = LVGF_HEADER;
        wchar_t wToday[64]; MultiByteToWideChar(CP_UTF8, 0, g_Lang.grpToday, -1, wToday, 64);
        wchar_t wOlder[64]; MultiByteToWideChar(CP_UTF8, 0, g_Lang.grpOlder, -1, wOlder, 64);
        lvg.pszHeader = wToday; ListView_SetGroupInfo(hWndListView, 1, &lvg);
        lvg.pszHeader = wOlder; ListView_SetGroupInfo(hWndListView, 2, &lvg);
    }
    if (hWndListSendFiles) {
        lvc.pszText = g_Lang.colFile; ListView_SetColumn(hWndListSendFiles, 0, &lvc);
        lvc.pszText = g_Lang.colSize; ListView_SetColumn(hWndListSendFiles, 1, &lvc);
    }
    if (hWndListDevices) {
        lvc.pszText = g_Lang.colDevice; ListView_SetColumn(hWndListDevices, 0, &lvc);
        lvc.pszText = g_Lang.colInfo; ListView_SetColumn(hWndListDevices, 1, &lvc);
    }

    // Update Action buttons
    if (hWndBtnSndFile) SetWindowTextA(hWndBtnSndFile, g_Lang.btnFile);
    if (hWndBtnSndFolder) SetWindowTextA(hWndBtnSndFolder, g_Lang.btnFolder);
    if (hWndBtnSndText) SetWindowTextA(hWndBtnSndText, g_Lang.btnText);
    if (hWndBtnSndPaste) SetWindowTextA(hWndBtnSndPaste, g_Lang.btnPaste);
    if (hWndLblSendFiles) SetWindowTextA(hWndLblSendFiles, g_Lang.filesToSend);

    // Update Tooltips
    if (hWndBtnSndFile) UpdateTooltipText(hWndBtnSndFile, g_Lang.addFilesTooltip);
    if (hWndBtnSndFolder) UpdateTooltipText(hWndBtnSndFolder, g_Lang.addFolderTooltip);
    if (hWndBtnSndText) UpdateTooltipText(hWndBtnSndText, g_Lang.sendTextTooltip);
    if (hWndBtnSndPaste) UpdateTooltipText(hWndBtnSndPaste, g_Lang.pasteTooltip);
}

// Enables or disables the Send files buttons based on queue and device selections
void UpdateSendButtonsState() {
    BOOL hasFiles = (g_sendQueueCount > 0);
    int selectedDevice = ListView_GetNextItem(hWndListDevices, -1, LVNI_SELECTED);
    EnableWindow(hWndBtnCleanFiles, hasFiles); EnableWindow(hWndBtnSendFiles, hasFiles && (selectedDevice != -1));
}

// Shows or hides page layouts depending on current main tab select
void UpdatePageVisibility() {
    int mainTab = TabCtrl_GetCurSel(hWndTab);
    int showRecv = (mainTab == 0) ? SW_SHOW : SW_HIDE;

    ShowWindow(hWndGroupBox, showRecv); ShowWindow(hWndInfoText, showRecv); ShowWindow(hWndStatus, showRecv); ShowWindow(hWndListView, showRecv); ShowWindow(hWndStatusIcon, showRecv); ShowWindow(hWndDeviceNameTitle, showRecv);

    if (mainTab == 0) {
        if (IsWindowVisible(hWndBtnCloseStatus)) ShowWindow(hWndBtnCloseStatus, SW_SHOW);
        if (g_bEditingDeviceName) { ShowWindow(hWndDeviceName, SW_HIDE); ShowWindow(hWndBtnEditDevice, SW_HIDE); ShowWindow(hWndEditDeviceBox, SW_SHOW); ShowWindow(hWndBtnSaveDevice, SW_SHOW); ShowWindow(hWndBtnCancelDevice, SW_SHOW); }
        else { ShowWindow(hWndDeviceName, SW_SHOW); ShowWindow(hWndBtnEditDevice, SW_SHOW); ShowWindow(hWndEditDeviceBox, SW_HIDE); ShowWindow(hWndBtnSaveDevice, SW_HIDE); ShowWindow(hWndBtnCancelDevice, SW_HIDE); }
    } else { ShowWindow(hWndBtnCloseStatus, SW_HIDE); ShowWindow(hWndDeviceName, SW_HIDE); ShowWindow(hWndBtnEditDevice, SW_HIDE); ShowWindow(hWndEditDeviceBox, SW_HIDE); ShowWindow(hWndBtnSaveDevice, SW_HIDE); ShowWindow(hWndBtnCancelDevice, SW_HIDE); }

    int showSend = (mainTab == 1) ? SW_SHOW : SW_HIDE;
    ShowWindow(hWndBtnSndFile, showSend); ShowWindow(hWndBtnSndFolder, showSend); ShowWindow(hWndBtnSndText, showSend); ShowWindow(hWndBtnSndPaste, showSend); ShowWindow(hWndLblSendFiles, showSend); ShowWindow(hWndListSendFiles, showSend); ShowWindow(hWndBtnCleanFiles, showSend); ShowWindow(hWndBtnSendFiles, showSend); ShowWindow(hWndLblDevices, showSend); ShowWindow(hWndListDevices, showSend); ShowWindow(hWndBtnSearchAgain, showSend); ShowWindow(hWndBtnSendManual, showSend);

    if (showSend == SW_SHOW) { UpdateSendButtonsState(); } else { ShowWindow(hWndSearchLoading, SW_HIDE); ShowWindow(hWndSendProgress, SW_HIDE); ShowWindow(hWndSendStatusTxt, SW_HIDE); ShowWindow(hWndSendStatusIcon, SW_HIDE); }
    if (mainTab == 2) { ShowWindow(hWndSettingsTab, SW_SHOW); ShowSettingsSubPage(TabCtrl_GetCurSel(hWndSettingsTab)); } else { ShowWindow(hWndSettingsTab, SW_HIDE); ShowSettingsSubPage(-1); }
}

// Handles positioning and layout reflow during window resizing
void ResizeControls(HWND hWnd, int width, int height) {
    if (!hWndTab) return;
    MoveWindow(hWndTab, 5, 5, width - 10, height - 10, TRUE);
    int mainTab = TabCtrl_GetCurSel(hWndTab);
    int sxWidth = g_SplitterPos; int dxLeft = g_SplitterPos + 10; int dxWidth = (width - 20) - dxLeft;

    if (mainTab == 0) {
        MoveWindow(hWndDeviceNameTitle, 20, 38, sxWidth - 20, 15, TRUE);
        if (!g_bEditingDeviceName) { MoveWindow(hWndDeviceName, 20, 54, sxWidth - 95, 30, TRUE); MoveWindow(hWndBtnEditDevice, sxWidth - 70, 53, 60, 24, TRUE); }
        else { MoveWindow(hWndEditDeviceBox, 20, 54, sxWidth - 155, 24, TRUE); MoveWindow(hWndBtnSaveDevice, sxWidth - 130, 53, 55, 24, TRUE); MoveWindow(hWndBtnCancelDevice, sxWidth - 70, 53, 60, 24, TRUE); }
        MoveWindow(hWndGroupBox, 15, 95, sxWidth - 10, 105, TRUE); MoveWindow(hWndInfoText, 30, 115, sxWidth - 40, 75, TRUE);
        MoveWindow(hWndStatusIcon, 15, height - 35, 16, 16, TRUE);
        if (IsWindowVisible(hWndBtnCloseStatus)) { MoveWindow(hWndStatus, 38, height - 37, sxWidth - 115, 22, TRUE); MoveWindow(hWndBtnCloseStatus, sxWidth - 75, height - 39, 65, 24, TRUE); }
        else { MoveWindow(hWndStatus, 38, height - 37, sxWidth - 40, 22, TRUE); }
        MoveWindow(hWndListView, dxLeft, 35, dxWidth, height - 50, TRUE);
    }
    else if (mainTab == 1) {
        int btnWidth = (sxWidth - 25) / 4;
        MoveWindow(hWndBtnSndFile, 15, 35, btnWidth, 35, TRUE); MoveWindow(hWndBtnSndFolder, 15 + btnWidth + 5, 35, btnWidth, 35, TRUE);
        MoveWindow(hWndBtnSndText, 15 + btnWidth*2 + 10, 35, btnWidth, 35, TRUE); MoveWindow(hWndBtnSndPaste, 15 + btnWidth*3 + 15, 35, btnWidth, 35, TRUE);
        MoveWindow(hWndLblSendFiles, 15, 80, sxWidth - 10, 15, TRUE); MoveWindow(hWndListSendFiles, 15, 100, sxWidth - 10, height - 195, TRUE);
        MoveWindow(hWndSendProgress, 15, height - 88, sxWidth - 10, 14, TRUE); MoveWindow(hWndSendStatusIcon, 15, height - 66, 16, 16, TRUE); MoveWindow(hWndSendStatusTxt, 38, height - 68, sxWidth - 33, 32, TRUE);
        int botBtnWidthL = (sxWidth - 15) / 2;
        MoveWindow(hWndBtnCleanFiles, 15, height - 35, botBtnWidthL, 24, TRUE); MoveWindow(hWndBtnSendFiles, 15 + botBtnWidthL + 5, height - 35, botBtnWidthL, 24, TRUE);
        MoveWindow(hWndLblDevices, dxLeft, 35, dxWidth, 15, TRUE); MoveWindow(hWndListDevices, dxLeft, 55, dxWidth, height - 100, TRUE);
        int botBtnWidthR = (dxWidth - 5) / 2;
        if (IsWindowVisible(hWndSearchLoading)) { MoveWindow(hWndSearchLoading, dxLeft, height - 30, 16, 16, TRUE); MoveWindow(hWndBtnSearchAgain, dxLeft + 22, height - 35, botBtnWidthR - 22, 24, TRUE); }
        else { MoveWindow(hWndBtnSearchAgain, dxLeft, height - 35, botBtnWidthR, 24, TRUE); }
        MoveWindow(hWndBtnSendManual, dxLeft + botBtnWidthR + 5, height - 35, botBtnWidthR, 24, TRUE);
    }
    else if (mainTab == 2) {
        RECT rcTab; GetClientRect(hWndTab, &rcTab); TabCtrl_AdjustRect(hWndTab, FALSE, &rcTab);
        MoveWindow(hWndSettingsTab, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, TRUE);

        RECT rcSubTab; rcSubTab.left = 0; rcSubTab.top = 0; rcSubTab.right = rcTab.right - rcTab.left; rcSubTab.bottom = rcTab.bottom - rcTab.top;
        TabCtrl_AdjustRect(hWndSettingsTab, FALSE, &rcSubTab);

        int baseX = rcTab.left + rcSubTab.left; int baseY = rcTab.top + rcSubTab.top; int subW = rcSubTab.right - rcSubTab.left;

        MoveWindow(hWndCheckSavePos, baseX + 15, baseY + 15, subW - 30, 20, TRUE); MoveWindow(hWndCheckMinClose, baseX + 15, baseY + 45, subW - 30, 20, TRUE); MoveWindow(hWndCheckTopmost, baseX + 15, baseY + 75, subW - 30, 20, TRUE);
        MoveWindow(hWndLblLanguage, baseX + 15, baseY + 115, 100, 20, TRUE); MoveWindow(hWndComboLanguage, baseX + 120, baseY + 112, 150, 150, TRUE);
        
        MoveWindow(hWndRecvLbl, baseX + 15, baseY + 15, subW - 30, 20, TRUE); MoveWindow(hWndRecvRadApp, baseX + 15, baseY + 40, subW - 30, 20, TRUE); MoveWindow(hWndRecvRadDl, baseX + 15, baseY + 65, subW - 30, 20, TRUE); MoveWindow(hWndRecvRadCustom, baseX + 15, baseY + 90, subW - 30, 20, TRUE);
        MoveWindow(hWndRecvTxtPath, baseX + 35, baseY + 115, subW - 120, 24, TRUE); MoveWindow(hWndRecvBtnBrowse, baseX + subW - 80, baseY + 114, 65, 26, TRUE);
        MoveWindow(hWndCheckQuickSave, baseX + 15, baseY + 150, subW - 30, 20, TRUE);
        
        // Network UI layout
        MoveWindow(hWndCheckReqPin, baseX + 15, baseY + 15, 120, 20, TRUE);
        MoveWindow(hWndLblPinCode, baseX + 145, baseY + 17, 70, 20, TRUE);
        MoveWindow(hWndEditPinCode, baseX + 215, baseY + 15, 80, 22, TRUE);

        MoveWindow(hWndLblDiscTimeout, baseX + 15, baseY + 47, 160, 20, TRUE);
        MoveWindow(hWndEditDiscTimeout, baseX + 180, baseY + 45, 60, 22, TRUE);

        MoveWindow(hWndLblMulticast, baseX + 15, baseY + 77, 160, 20, TRUE);
        MoveWindow(hWndEditMulticast, baseX + 180, baseY + 75, 150, 22, TRUE);

        MoveWindow(hWndCheckEncryption, baseX + 15, baseY + 105, 300, 20, TRUE);

        MoveWindow(hWndLblDevType, baseX + 15, baseY + 137, 160, 20, TRUE);
        MoveWindow(hWndComboDevType, baseX + 180, baseY + 135, 120, 150, TRUE);

        MoveWindow(hWndLblDevModel, baseX + 15, baseY + 167, 160, 20, TRUE);
        MoveWindow(hWndEditDevModel, baseX + 180, baseY + 165, 150, 22, TRUE);

        MoveWindow(hWndLblPort, baseX + 15, baseY + 197, 160, 20, TRUE);
        MoveWindow(hWndEditPort, baseX + 180, baseY + 195, 80, 22, TRUE);

        MoveWindow(hWndIconStatic, baseX + 15, baseY + 15, 48, 48, TRUE); MoveWindow(hWndLabelAbout, baseX + 80, baseY + 15, subW - 95, 100, TRUE); MoveWindow(hWndBtnGithub, baseX + 15, baseY + 125, 160, 30, TRUE); MoveWindow(hWndBtnSupport, baseX + 190, baseY + 125, 160, 30, TRUE);
    }
    if (hWndTab) SetWindowPos(hWndTab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    if (hWndSettingsTab) SetWindowPos(hWndSettingsTab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

// Handles command messages (clicks, menu selections) sent to the main window
BOOL HandleWndCommand(HWND hWnd, int wmId, int wmEvent, HWND hWndCtrl) {
    if (wmId == 7001) {
        int selectedIdx = ListView_GetNextItem(hWndListSendFiles, -1, LVNI_SELECTED);
        if (selectedIdx != -1) {
            LVITEMA lvi = {0}; lvi.iItem = selectedIdx; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListSendFiles, &lvi);
            int queueIdx = (int)lvi.lParam;
            for (int i = queueIdx; i < g_sendQueueCount - 1; i++) {
                g_sendQueue[i] = g_sendQueue[i + 1];
            }
            g_sendQueueCount--;
            ListView_DeleteItem(hWndListSendFiles, selectedIdx);
            for (int i = 0; i < ListView_GetItemCount(hWndListSendFiles); i++) {
                LVITEMA itemMod = {0}; itemMod.iItem = i; itemMod.mask = LVIF_PARAM; ListView_GetItem(hWndListSendFiles, &itemMod);
                if ((int)itemMod.lParam > queueIdx) {
                    itemMod.lParam = (LPARAM)((int)itemMod.lParam - 1);
                    ListView_SetItem(hWndListSendFiles, &itemMod);
                }
            }
            UpdateSendButtonsState();
        }
        return TRUE;
    }
    if (wmId == 7002) {
        int selectedIdx = ListView_GetNextItem(hWndListSendFiles, -1, LVNI_SELECTED);
        if (selectedIdx != -1) {
            LVITEMA lvi = {0}; lvi.iItem = selectedIdx; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListSendFiles, &lvi);
            int queueIdx = (int)lvi.lParam;
            if (queueIdx >= 0 && queueIdx < g_sendQueueCount) {
                SHELLEXECUTEINFOA sei = {0}; sei.cbSize = sizeof(SHELLEXECUTEINFOA); sei.fMask = SEE_MASK_INVOKEIDLIST; sei.lpVerb = "properties"; sei.lpFile = g_sendQueue[queueIdx].filePath; ShellExecuteExA(&sei);
            }
        }
        return TRUE;
    }

    if (wmId == IDC_SET_BTN_GH) { ShellExecuteA(NULL, "open", "https://localsend.org", NULL, NULL, SW_SHOWNORMAL); return TRUE; }
    if (wmId == IDC_SET_BTN_SUPP) { ShellExecuteA(NULL, "open", APP_SUPPORT_URL, NULL, NULL, SW_SHOWNORMAL); return TRUE; }

    if (wmId == IDC_RECV_RAD_APP || wmId == IDC_RECV_RAD_DL || wmId == IDC_RECV_RAD_CUSTOM) {
        g_SaveMode = (wmId == IDC_RECV_RAD_APP) ? 0 : (wmId == IDC_RECV_RAD_DL) ? 1 : 2;
        BOOL isCustom = (g_SaveMode == 2); EnableWindow(hWndRecvTxtPath, isCustom); EnableWindow(hWndRecvBtnBrowse, isCustom); SaveSettings(); return TRUE;
    }
    if (wmId == IDC_RECV_BTN_BROWSE) {
        BROWSEINFOA bi = {0}; bi.hwndOwner = hWnd; bi.lpszTitle = g_Lang.selectSaveFolder; bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI; LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        if (pidl != NULL) { char folderPath[MAX_PATH]; if (SHGetPathFromIDListA(pidl, folderPath)) { SetWindowTextA(hWndRecvTxtPath, folderPath); strcpy(g_CustomPath, folderPath); g_SaveMode = 2; SendMessage(hWndRecvRadApp, BM_SETCHECK, BST_UNCHECKED, 0); SendMessage(hWndRecvRadDl, BM_SETCHECK, BST_UNCHECKED, 0); SendMessage(hWndRecvRadCustom, BM_SETCHECK, BST_CHECKED, 0); EnableWindow(hWndRecvTxtPath, TRUE); EnableWindow(hWndRecvBtnBrowse, TRUE); SaveSettings(); } CoTaskMemFree(pidl); } return TRUE;
    }
    if (wmId == IDC_RECV_TXT_PATH && wmEvent == EN_KILLFOCUS) { GetWindowTextA(hWndRecvTxtPath, g_CustomPath, MAX_PATH); SaveSettings(); return TRUE; }

    if (wmId == IDC_BTN_SEND_FILE) {
        char fileBuffer[4096] = {0}; OPENFILENAMEA ofn = {0}; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hWnd; ofn.lpstrFile = fileBuffer; ofn.nMaxFile = sizeof(fileBuffer); ofn.lpstrFilter = "Tutti i file\0*.*\0"; ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;
        if (GetOpenFileNameA(&ofn)) {
            char* p = fileBuffer; char dir[512]; strncpy(dir, p, 511); p += strlen(p) + 1;
            if (*p == '\0') { char* fileName = strrchr(dir, '\\'); if (fileName) { fileName++; HANDLE hFile = CreateFileA(dir, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); if (hFile != INVALID_HANDLE_VALUE) { LARGE_INTEGER fs; GetFileSizeEx(hFile, &fs); CloseHandle(hFile); AddFileToSendQueue(dir, fileName, fs.QuadPart); } } }
            else { while (*p) { char fullPath[512]; sprintf(fullPath, "%s\\%s", dir, p); HANDLE hFile = CreateFileA(fullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); if (hFile != INVALID_HANDLE_VALUE) { LARGE_INTEGER fs; GetFileSizeEx(hFile, &fs); CloseHandle(hFile); AddFileToSendQueue(fullPath, p, fs.QuadPart); } p += strlen(p) + 1; } }
        }
        return TRUE;
    }

    if (wmId == IDC_BTN_SEND_FOLDER) {
        BROWSEINFOA bi = {0}; bi.hwndOwner = hWnd; bi.lpszTitle = g_Lang.selectFolderToSend; bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI; LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        if (pidl != NULL) { char folderPath[MAX_PATH]; if (SHGetPathFromIDListA(pidl, folderPath)) { char searchPath[512]; sprintf(searchPath, "%s\\*.*", folderPath); WIN32_FIND_DATAA ffd; HANDLE hFind = FindFirstFileA(searchPath, &ffd); if (hFind != INVALID_HANDLE_VALUE) { do { if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) { char fullFilePath[512]; sprintf(fullFilePath, "%s\\%s", folderPath, ffd.cFileName); LARGE_INTEGER fs; fs.LowPart = ffd.nFileSizeLow; fs.HighPart = ffd.nFileSizeHigh; AddFileToSendQueue(fullFilePath, ffd.cFileName, fs.QuadPart); } } while (ffd.cFileName[0] && FindNextFileA(hFind, &ffd)); FindClose(hFind); } } CoTaskMemFree(pidl); }
        return TRUE;
    }

    if (wmId == IDC_BTN_SEND_TEXT) {
        WORD* pDlgMem = (WORD*)malloc(1024); memset(pDlgMem, 0, 1024); LPDLGTEMPLATEA lpd = (LPDLGTEMPLATEA)pDlgMem; lpd->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER; lpd->cx = 240; lpd->cy = 180;
        char* textResult = (char*)DialogBoxIndirectParamA(GetModuleHandle(NULL), lpd, hWnd, TextDialogProc, 0); free(pDlgMem);
        if (textResult) { char tempPath[MAX_PATH], tempFile[MAX_PATH]; GetTempPathA(MAX_PATH, tempPath); GetTempFileNameA(tempPath, "ls", 0, tempFile); char finalTxtPath[MAX_PATH]; sprintf(finalTxtPath, "%s.txt", tempFile); FILE* tf = fopen(finalTxtPath, "w"); if (tf) { fputs(textResult, tf); fclose(tf); AddFileToSendQueue(finalTxtPath, "text.txt", strlen(textResult)); } free(textResult); }
        return TRUE;
    }

    if (wmId == IDC_BTN_SEND_PASTE) {
        if (OpenClipboard(hWnd)) {
            if (IsClipboardFormatAvailable(CF_TEXT)) { HGLOBAL hMem = GetClipboardData(CF_TEXT); if (hMem) { char* pText = (char*)GlobalLock(hMem); if (pText) { char tempPath[MAX_PATH], finalTxtPath[MAX_PATH]; GetTempPathA(MAX_PATH, tempPath); sprintf(finalTxtPath, "%s\\clip_%ld.txt", tempPath, GetTickCount()); FILE* tf = fopen(finalTxtPath, "w"); if (tf) { fputs(pText, tf); fclose(tf); AddFileToSendQueue(finalTxtPath, "text.txt", strlen(pText)); } GlobalUnlock(hMem); } } }
            else if (IsClipboardFormatAvailable(CF_HDROP)) { HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP); if (hDrop) { int fileCount = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0); for (int i = 0; i < fileCount; i++) { char filePath[MAX_PATH]; DragQueryFileA(hDrop, i, filePath, MAX_PATH); char* fileName = strrchr(filePath, '\\'); fileName = fileName ? fileName + 1 : filePath; HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); if (hFile != INVALID_HANDLE_VALUE) { LARGE_INTEGER fs; GetFileSizeEx(hFile, &fs); CloseHandle(hFile); AddFileToSendQueue(filePath, fileName, fs.QuadPart); } } } }
            else if (IsClipboardFormatAvailable(CF_BITMAP)) { HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP); if (hBitmap) { char tempPath[MAX_PATH], bmpPath[MAX_PATH]; GetTempPathA(MAX_PATH, tempPath); sprintf(bmpPath, "%s\\clip_%ld.bmp", tempPath, GetTickCount()); BITMAP bmp; HDC hdc = GetDC(hWnd); GetObject(hBitmap, sizeof(BITMAP), &bmp); BITMAPFILEHEADER bfh = {0}; bfh.bfType = 0x4D42; bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); bfh.bfSize = bfh.bfOffBits + bmp.bmWidthBytes * bmp.bmHeight; BITMAPINFOHEADER bih = {0}; bih.biSize = sizeof(BITMAPINFOHEADER); bih.biWidth = bmp.bmWidth; bih.biHeight = bmp.bmHeight; bih.biPlanes = 1; bih.biBitCount = bmp.bmBitsPixel; bih.biCompression = BI_RGB; FILE* f = fopen(bmpPath, "wb"); if (f) { fwrite(&bfh, 1, sizeof(bfh), f); fwrite(&bih, 1, sizeof(bih), f); char* pBits = (char*)malloc(bmp.bmWidthBytes * bmp.bmHeight); GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, pBits, (BITMAPINFO*)&bih, DIB_RGB_COLORS); fwrite(pBits, 1, bmp.bmWidthBytes * bmp.bmHeight, f); free(pBits); fclose(f); AddFileToSendQueue(bmpPath, "immagine_appunti.bmp", bfh.bfSize); } ReleaseDC(hWnd, hdc); } }
            CloseClipboard();
        }
        return TRUE;
    }

    if (wmId == IDC_BTN_CLEAN_FILES) {
        ListView_DeleteAllItems(hWndListSendFiles); g_sendQueueCount = 0; memset(g_sendQueue, 0, sizeof(g_sendQueue)); UpdateSendButtonsState();
        return TRUE;
    }

    if (wmId == IDC_BTN_SEND_FILES) {
        int selectedCount = 0; int listCount = ListView_GetItemCount(hWndListDevices);
        for (int i = 0; i < listCount; i++) {
            if (ListView_GetCheckState(hWndListDevices, i)) {
                selectedCount++;
            }
        }
        if (selectedCount == 0) {
            MessageBoxA(hWnd, "Please check or select a device to send files to.", "No selection", MB_ICONWARNING);
            return TRUE;
        }

        SendSessionContext* txCtx = (SendSessionContext*)malloc(sizeof(SendSessionContext));
        if (txCtx) {
            memset(txCtx, 0, sizeof(SendSessionContext));
            int tgtIdx = 0;
            for (int i = 0; i < listCount && tgtIdx < 16; i++) {
                if (ListView_GetCheckState(hWndListDevices, i)) {
                    LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListDevices, &lvi); RemoteDevice* dev = (RemoteDevice*)lvi.lParam;
                    if (dev) {
                        strcpy(txCtx->targets[tgtIdx].ipAddress, dev->ipAddress);
                        txCtx->targets[tgtIdx].port = dev->port;
                        strcpy(txCtx->targets[tgtIdx].alias, dev->alias);
                        tgtIdx++;
                    }
                }
            }
            txCtx->targetCount = tgtIdx;
            txCtx->fileCount = g_sendQueueCount;
            memcpy(txCtx->files, g_sendQueue, sizeof(FileToSend) * g_sendQueueCount);
            ShowWindow(hWndSendProgress, SW_SHOW); SendMessage(hWndSendProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); SendMessage(hWndSendProgress, PBM_SETPOS, 0, 0);
            char* copyMsg = (char*)malloc(128); if (copyMsg) { strcpy(copyMsg, "Connecting to remote peer..."); PostMessage(hWnd, WM_SEND_STATUS_UPDATE, ICON_TRANSFERRING, (LPARAM)copyMsg); }
            HANDLE hTxThread = CreateThread(NULL, 0, StartSendSessionThread, (LPVOID)txCtx, 0, NULL); if (hTxThread) CloseHandle(hTxThread);
        }
        return TRUE;
    }

    if (wmId == IDC_BTN_SEND_MANUAL) {
        WORD* pDlgMem = (WORD*)malloc(1024); memset(pDlgMem, 0, 1024); LPDLGTEMPLATEA lpd = (LPDLGTEMPLATEA)pDlgMem; lpd->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER; lpd->cx = 210; lpd->cy = 90;
        char* textResult = (char*)DialogBoxIndirectParamA(GetModuleHandle(NULL), lpd, hWnd, ManualSendDialogProc, 0); free(pDlgMem);
        if (textResult) {
            char targetIp[64] = {0}; int targetPort = 53317; BOOL found = FALSE;
            if (textResult[0] == '#') {
                char targetHash[64]; strcpy(targetHash, textResult + 1);
                int devCount = ListView_GetItemCount(hWndListDevices);
                for (int i = 0; i < devCount; i++) {
                    LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListDevices, &lvi); RemoteDevice* dev = (RemoteDevice*)lvi.lParam;
                    if (dev) {
                        char cleanAlias[64] = {0}, cleanFingerprint[64] = {0};
                        cleanQuotes(cleanAlias, dev->alias, 63); cleanQuotes(cleanFingerprint, dev->fingerprint, 63);
                        const char* lastDot = strrchr(dev->ipAddress, '.');
                        const char* devHashtag = lastDot ? (lastDot + 1) : dev->ipAddress;

                        if (strcmp(cleanAlias, targetHash) == 0 || strcmp(cleanFingerprint, targetHash) == 0 || strcmp(devHashtag, targetHash) == 0) {
                            strcpy(targetIp, dev->ipAddress); targetPort = dev->port; found = TRUE; break;
                        }
                    }
                }
                if (!found) { MessageBoxA(hWnd, "Hashtag not found in nearby list. Please scan again.", "Not found", MB_ICONERROR); free(textResult); return TRUE; }
            } else { strcpy(targetIp, textResult); found = TRUE; }
            free(textResult);

            if (found) {
                SendSessionContext* txCtx = (SendSessionContext*)malloc(sizeof(SendSessionContext));
                if (txCtx) {
                    memset(txCtx, 0, sizeof(SendSessionContext));
                    strncpy(txCtx->targets[0].ipAddress, targetIp, 15);
                    txCtx->targets[0].ipAddress[15] = '\0';
                    txCtx->targets[0].port = targetPort;
                    strcpy(txCtx->targets[0].alias, "Manual Target");
                    txCtx->targetCount = 1;
                    txCtx->fileCount = g_sendQueueCount;
                    memcpy(txCtx->files, g_sendQueue, sizeof(FileToSend) * g_sendQueueCount);
                    ShowWindow(hWndSendProgress, SW_SHOW); SendMessage(hWndSendProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); SendMessage(hWndSendProgress, PBM_SETPOS, 0, 0);
                    char* copyMsg = (char*)malloc(128);
                    if (copyMsg) {
                        strcpy(copyMsg, "Connecting to remote peer...");
                        PostMessage(hWnd, WM_SEND_STATUS_UPDATE, ICON_TRANSFERRING, (LPARAM)copyMsg);
                    }
                    HANDLE hTxThread = CreateThread(NULL, 0, StartSendSessionThread, (LPVOID)txCtx, 0, NULL); if (hTxThread) CloseHandle(hTxThread);
                }
            }
        }
        return TRUE;
    }

    if (wmId == IDC_BTN_SEARCH_AGAIN) {
        int devCount = ListView_GetItemCount(hWndListDevices);
        for (int i = 0; i < devCount; i++) {
            LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListDevices, &lvi);
            if (lvi.lParam) free((void*)lvi.lParam);
        }
        ListView_DeleteAllItems(hWndListDevices);
        ShowWindow(hWndSearchLoading, SW_SHOW); EnableWindow(hWndBtnSearchAgain, FALSE); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); if (g_mySocket != INVALID_SOCKET) { sendDiscoveryShout(g_mySocket); } SetTimer(hWnd, 999, 3000, NULL); UpdateSendButtonsState();
        return TRUE;
    }
    if (wmId == IDC_BTN_CLOSE_STATUS) { int totalItems = ListView_GetItemCount(hWndListView); for (int i = 0; i < totalItems; i++) { LVITEMA itemMod = {0}; itemMod.iItem = i; itemMod.mask = LVIF_GROUPID; ListView_GetItem(hWndListView, &itemMod); if (itemMod.iGroupId == 1) { itemMod.iGroupId = 2; ListView_SetItem(hWndListView, &itemMod); } } SetWindowTextA(hWndStatus, "Waiting for files..."); UpdateStatusIcon(ICON_WAITING); ShowWindow(hWndBtnCloseStatus, SW_HIDE); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); return TRUE; }
    if (wmId == IDC_BTN_EDIT_DEVICE) { g_bEditingDeviceName = TRUE; SetWindowTextA(hWndEditDeviceBox, g_MyDeviceName); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); UpdatePageVisibility(); SetFocus(hWndEditDeviceBox); return TRUE; }
    if (wmId == IDC_BTN_SAVE_DEVICE) { GetWindowTextA(hWndEditDeviceBox, g_MyDeviceName, sizeof(g_MyDeviceName)); SetWindowTextA(hWndDeviceName, g_MyDeviceName); g_bEditingDeviceName = FALSE; RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); UpdatePageVisibility(); SaveSettings(); return TRUE; }
    if (wmId == IDC_BTN_CANCEL_DEVICE) { g_bEditingDeviceName = FALSE; RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); UpdatePageVisibility(); return TRUE; }

    if (wmId == IDC_SET_TOPMOST && wmEvent == BN_CLICKED) { LRESULT lChecked = SendMessage(hWndCheckTopmost, BM_GETCHECK, 0, 0); if (lChecked == BST_CHECKED) { SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); } else { SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); } SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_MIN_CLOSE && wmEvent == BN_CLICKED) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_SAVE_POS && wmEvent == BN_CLICKED) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_QUICK_SAVE && wmEvent == BN_CLICKED) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_REQ_PIN && wmEvent == BN_CLICKED) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_PIN_CODE && wmEvent == EN_KILLFOCUS) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_DISC_TIMEOUT && wmEvent == EN_KILLFOCUS) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_MULTICAST && wmEvent == EN_KILLFOCUS) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_ENCRYPTION && wmEvent == BN_CLICKED) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_DEV_TYPE && wmEvent == CBN_SELCHANGE) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_DEV_MODEL && wmEvent == EN_KILLFOCUS) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_LANG_COMBO && wmEvent == CBN_SELCHANGE) {
        int sel = (int)SendMessage(hWndComboLanguage, CB_GETCURSEL, 0, 0);
        if (sel >= 0 && sel < LANG_COUNT) {
            g_Language = sel;
            memcpy(&g_Lang, &g_Languages[g_Language], sizeof(LanguageStrings));
            UpdateUITexts();
            SaveSettings();
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
        }
        return TRUE;
    }
    if (wmId == IDC_SET_PORT && wmEvent == EN_KILLFOCUS) { SaveSettings(); return TRUE; }

    int selectedIdx = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
    if (selectedIdx != -1) {
        LVITEMA lvi = {0}; lvi.iItem = selectedIdx; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListView, &lvi); LoggedTransfer* transfer = (LoggedTransfer*)lvi.lParam;
        char filePath[MAX_PATH] = {0}; GetConfiguredSavePath(filePath, MAX_PATH, transfer->fileName);

        if (wmId == IDM_OPEN_FILE) { ShellExecuteA(NULL, "open", filePath, NULL, NULL, SW_SHOWNORMAL); return TRUE; }
        else if (wmId == IDM_DELETE_FILE) {
            if (DeleteFileA(filePath) || GetLastError() == ERROR_FILE_NOT_FOUND) { char fileToPurge[256]; strcpy(fileToPurge, transfer->fileName); int totalItems = ListView_GetItemCount(hWndListView); for (int i = totalItems - 1; i >= 0; i--) { LVITEMA itemLoop = {0}; itemLoop.iItem = i; itemLoop.mask = LVIF_PARAM; ListView_GetItem(hWndListView, &itemLoop); LoggedTransfer* tLoop = (LoggedTransfer*)itemLoop.lParam; if (tLoop && strcmp(tLoop->fileName, fileToPurge) == 0) { ListView_DeleteItem(hWndListView, i); } } } else { MessageBoxA(hWnd, "Unable to delete file.", "Error", MB_ICONERROR); }
            return TRUE;
        }
        else if (wmId == IDM_PROP_FILE) { SHELLEXECUTEINFOA sei = {0}; sei.cbSize = sizeof(SHELLEXECUTEINFOA); sei.fMask = SEE_MASK_INVOKEIDLIST; sei.lpVerb = "properties"; sei.lpFile = filePath; ShellExecuteExA(&sei); return TRUE; }
        else if (wmId == IDM_COPY_FILE) { if (OpenClipboard(NULL)) { EmptyClipboard(); HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strlen(filePath) + 1); memcpy(GlobalLock(hMem), filePath, strlen(filePath) + 1); GlobalUnlock(hMem); SetClipboardData(CF_TEXT, hMem); CloseClipboard(); } return TRUE; }
    }
    return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            CreateMainControls(hWnd, GetModuleHandle(NULL));
            break;
        }

        case WM_COMMAND: {
            if (HandleWndCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam)) {
                return 0;
            }
            break;
        }

        case WM_SEND_STATUS_UPDATE: {
            StatusIconType iconType = (StatusIconType)wParam; char* msg = (char*)lParam;
            if (msg) { UpdateSendStatusIcon(iconType); SetWindowTextA(hWndSendStatusTxt, msg); ShowWindow(hWndSendStatusIcon, SW_SHOW); ShowWindow(hWndSendStatusTxt, SW_SHOW); free(msg); } break;
        }
        case WM_SEND_DONE: {
            BOOL isSuccess = (BOOL)wParam; ShowWindow(hWndSendProgress, SW_HIDE);
            if (isSuccess) { UpdateSendStatusIcon(ICON_SUCCESS); SetWindowTextA(hWndSendStatusTxt, "All files transferred successfully."); ShowWindow(hWndSendStatusIcon, SW_SHOW); ShowWindow(hWndSendStatusTxt, SW_SHOW); SetTimer(hWnd, 998, 5000, NULL); }
            UpdateSendButtonsState(); break;
        }
        case WM_TIMER: {
            if (wParam == 999) { KillTimer(hWnd, 999); ShowWindow(hWndSearchLoading, SW_HIDE); EnableWindow(hWndBtnSearchAgain, TRUE); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); }
            if (wParam == 998) { KillTimer(hWnd, 998); ShowWindow(hWndSendStatusIcon, SW_HIDE); ShowWindow(hWndSendStatusTxt, SW_HIDE); } break;
        }

        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam; RECT rect; GetClientRect(hWnd, &rect); FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1)); int mainTab = TabCtrl_GetCurSel(hWndTab);
            if (mainTab == 0 || mainTab == 1) { HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW)); HPEN hOldPen = (HPEN)SelectObject(hdc, hPen); MoveToEx(hdc, g_SplitterPos + 5, 30, NULL); LineTo(hdc, g_SplitterPos + 5, rect.bottom - 10); SelectObject(hdc, hOldPen); DeleteObject(hPen); hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT)); hOldPen = (HPEN)SelectObject(hdc, hPen); MoveToEx(hdc, g_SplitterPos + 6, 30, NULL); LineTo(hdc, g_SplitterPos + 6, rect.bottom - 10); SelectObject(hdc, hOldPen); DeleteObject(hPen); }
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW + 1);
        }
        case WM_GETMINMAXINFO: { LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam; lpMMI->ptMinTrackSize.x = 600; lpMMI->ptMinTrackSize.y = 340; return 0; }
        case WM_SIZE: { ResizeControls(hWnd, LOWORD(lParam), HIWORD(lParam)); break; }
        case WM_LBUTTONDOWN: { int xPos = LOWORD(lParam); int mainTab = TabCtrl_GetCurSel(hWndTab); if ((mainTab == 0 || mainTab == 1) && xPos >= g_SplitterPos && xPos <= g_SplitterPos + 10) { g_bDraggingSplitter = true; SetCapture(hWnd); SetCursor(LoadCursor(NULL, IDC_SIZEWE)); } break; }
        case WM_MOUSEMOVE: { int mainTab = TabCtrl_GetCurSel(hWndTab); if (mainTab != 0 && mainTab != 1) break; int xPos = LOWORD(lParam); if (g_bDraggingSplitter) { if (xPos > 200 && xPos < 600) { g_SplitterPos = xPos; RECT rect; GetClientRect(hWnd, &rect); InvalidateRect(hWnd, NULL, TRUE); ResizeControls(hWnd, rect.right, rect.bottom); } } else if (xPos >= g_SplitterPos && xPos <= g_SplitterPos + 10) { SetCursor(LoadCursor(NULL, IDC_SIZEWE)); } break; }
        case WM_LBUTTONUP: { if (g_bDraggingSplitter) { g_bDraggingSplitter = false; ReleaseCapture(); } break; }
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetBkMode(hdcStatic, TRANSPARENT);
            BOOL classic = TRUE;
            HMODULE hTheme = LoadLibraryA("uxtheme.dll");
            if (hTheme) {
                typedef BOOL(WINAPI* pfnIsThemeActive)();
                pfnIsThemeActive fnIsThemeActive = (pfnIsThemeActive)GetProcAddress(hTheme, "IsThemeActive");
                typedef BOOL(WINAPI* pfnIsAppThemed)();
                pfnIsAppThemed fnIsAppThemed = (pfnIsAppThemed)GetProcAddress(hTheme, "IsAppThemed");
                if (fnIsThemeActive && fnIsAppThemed) {
                    if (fnIsThemeActive() && fnIsAppThemed()) {
                        classic = FALSE;
                    }
                }
                FreeLibrary(hTheme);
            }
            return (LRESULT)GetSysColorBrush(classic ? COLOR_BTNFACE : COLOR_WINDOW);
        }

        case WM_CONTEXTMENU: {
            HWND hTrigger = (HWND)wParam;
            if (hTrigger == hWndListView) { int selectedIdx = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED); if (selectedIdx != -1) { HMENU hMenu = CreatePopupMenu(); AppendMenuA(hMenu, MF_STRING, IDM_OPEN_FILE, "Open file"); AppendMenuA(hMenu, MF_STRING, IDM_COPY_FILE, "Copy path"); AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL); AppendMenuA(hMenu, MF_STRING, IDM_DELETE_FILE, "Delete"); AppendMenuA(hMenu, MF_STRING, IDM_PROP_FILE, "Properties"); TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, NULL); DestroyMenu(hMenu); } }
            break;
        }

        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            if (nmhdr->code == TCN_SELCHANGE) { UpdatePageVisibility(); RECT rect; GetClientRect(hWnd, &rect); ResizeControls(hWnd, rect.right, rect.bottom); RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN); }
            if (nmhdr->idFrom == IDC_LIST_DEVICES && (nmhdr->code == LVN_ITEMCHANGED)) { UpdateSendButtonsState(); }
            if (nmhdr->idFrom == IDC_FILE_LISTVIEW && nmhdr->code == NM_DBLCLK) {
                int selectedIdx = ((LPNMITEMACTIVATE)lParam)->iItem;
                if (selectedIdx != -1) { LVITEMA lvi = {0}; lvi.iItem = selectedIdx; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListView, &lvi); LoggedTransfer* transfer = (LoggedTransfer*)lvi.lParam; char filePath[MAX_PATH] = {0}; GetConfiguredSavePath(filePath, MAX_PATH, transfer->fileName); ShellExecuteA(NULL, "open", filePath, NULL, NULL, SW_SHOWNORMAL); }
            }
            if (nmhdr->idFrom == IDC_LIST_SEND_FILES && nmhdr->code == NM_DBLCLK) {
                int selectedIdx = ((LPNMITEMACTIVATE)lParam)->iItem;
                if (selectedIdx != -1) {
                    LVITEMA lvi = {0}; lvi.iItem = selectedIdx; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListSendFiles, &lvi);
                    int queueIdx = (int)lvi.lParam;
                    if (queueIdx >= 0 && queueIdx < g_sendQueueCount) {
                        ShellExecuteA(NULL, "open", g_sendQueue[queueIdx].filePath, NULL, NULL, SW_SHOWNORMAL);
                    }
                }
            }
            if (nmhdr->idFrom == IDC_LIST_SEND_FILES && nmhdr->code == NM_RCLICK) {
                int selectedIdx = ListView_GetNextItem(hWndListSendFiles, -1, LVNI_SELECTED);
                if (selectedIdx != -1) {
                    HMENU hMenu = CreatePopupMenu();
                    AppendMenuA(hMenu, MF_STRING, 7001, "Delete");
                    AppendMenuA(hMenu, MF_STRING, 7002, "Properties");
                    POINT pt; GetCursorPos(&pt);
                    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
                    DestroyMenu(hMenu);
                }
            }
            if (nmhdr->idFrom == IDC_FILE_LISTVIEW && nmhdr->code == NM_CUSTOMDRAW) {
                LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
                switch (lplvcd->nmcd.dwDrawStage) {
                    case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW; case CDDS_ITEMPREPAINT: return CDRF_NOTIFYSUBITEMDRAW;
                    case CDDS_SUBITEMPREPAINT: {
                        if (lplvcd->iSubItem == LV_COL_PROGRESS) {
                            LoggedTransfer* transfer = (LoggedTransfer*)lplvcd->nmcd.lItemlParam; int pct = transfer ? transfer->percentuale : 0;
                            RECT rect; ListView_GetSubItemRect(hWndListView, lplvcd->nmcd.dwItemSpec, LV_COL_PROGRESS, LVIR_BOUNDS, &rect); FillRect(lplvcd->nmcd.hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
                            char pctText[32]; if (transfer && transfer->stato == STATE_CANCELED) { sprintf(pctText, "  Canceled (%d%%)", pct); } else if (transfer && transfer->stato == STATE_COMPLETED) { sprintf(pctText, "  Completed"); } else { sprintf(pctText, "  %d%%", pct); }
                            SetTextColor(lplvcd->nmcd.hdc, RGB(60, 60, 60)); DrawTextA(lplvcd->nmcd.hdc, pctText, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                            RECT progressRect = rect; progressRect.top = rect.bottom - 6; progressRect.left += 6; progressRect.right -= 6; FillRect(lplvcd->nmcd.hdc, &progressRect, GetSysColorBrush(COLOR_BTNFACE));
                            long width = progressRect.right - progressRect.left; progressRect.right = progressRect.left + (width * pct / 100);
                            COLORREF barColor = (transfer && transfer->stato == STATE_CANCELED) ? RGB(229, 57, 53) : ((transfer && transfer->stato == STATE_COMPLETED) ? RGB(67, 160, 71) : GetSysColor(COLOR_HIGHLIGHT));
                            HBRUSH hProgressBrush = CreateSolidBrush(barColor); FillRect(lplvcd->nmcd.hdc, &progressRect, hProgressBrush); DeleteObject(hProgressBrush);
                            return CDRF_SKIPDEFAULT;
                        } return CDRF_DODEFAULT;
                    }
                }
            } break;
        }

        case WM_TRAYICON_MSG: { if (lParam == WM_LBUTTONDBLCLK) { if (IsWindowVisible(hWnd)) ShowWindow(hWnd, SW_HIDE); else { ShowWindow(hWnd, SW_SHOW); SetForegroundWindow(hWnd); } } break; }

        case WM_FILE_START: {
            FileStartInfo* info = (FileStartInfo*)lParam; char sizeStr[32];
            if (info->fileSize < 1024) sprintf(sizeStr, "%lld B", info->fileSize); else if (info->fileSize < 1024 * 1024) sprintf(sizeStr, "%.1f KB", info->fileSize / 1024.0); else sprintf(sizeStr, "%.1f MB", info->fileSize / (1024.0 * 1024.0));
            if (g_transfersCount < 100) {
                strcpy(g_transfers[g_transfersCount].fileName, info->fileName); strcpy(g_transfers[g_transfersCount].fileId, info->fileId); g_transfers[g_transfersCount].fileSize = info->fileSize; g_transfers[g_transfersCount].percentuale = 0; g_transfers[g_transfersCount].stato = STATE_TRANSFERRING;
                SYSTEMTIME st; GetLocalTime(&st); _snprintf(g_transfers[g_transfersCount].timestamp, sizeof(g_transfers[g_transfersCount].timestamp), "%02d/%02d/%04d %02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
                LVITEMA lvi = {0}; lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_GROUPID; lvi.iItem = ListView_GetItemCount(hWndListView); lvi.iSubItem = LV_COL_NAME; lvi.pszText = info->fileName; lvi.iImage = GetSystemIconIndex(info->fileName); lvi.lParam = (LPARAM)&g_transfers[g_transfersCount]; lvi.iGroupId = 1;
                int insertedIdx = ListView_InsertItem(hWndListView, &lvi); ListView_SetItemText(hWndListView, insertedIdx, LV_COL_SIZE, sizeStr); ListView_SetItemText(hWndListView, insertedIdx, LV_COL_DATE, g_transfers[g_transfersCount].timestamp); g_transfersCount++;
            }
            char statusBuf[512]; _snprintf(statusBuf, sizeof(statusBuf), "Receiving: %s (from %s)", info->fileName, info->senderName); SetWindowTextA(hWndStatus, statusBuf); UpdateStatusIcon(ICON_TRANSFERRING); ShowWindow(hWndBtnCloseStatus, SW_HIDE); TabCtrl_SetCurSel(hWndTab, 0); UpdatePageVisibility(); ShowWindow(hWnd, SW_SHOW); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); free(info); break;
        }

        case WM_FILE_PROGRESS: {
            FileProgressInfo* info = (FileProgressInfo*)lParam; int count = ListView_GetItemCount(hWndListView);
            for(int i = 0; i < count; i++) {
                LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListView, &lvi); LoggedTransfer* transfer = (LoggedTransfer*)lvi.lParam;
                if (transfer && strcmp(transfer->fileId, info->fileId) == 0) { transfer->percentuale = info->pct; if (info->pct >= 100) { transfer->stato = STATE_COMPLETED; } RECT cellRect; ListView_GetSubItemRect(hWndListView, i, LV_COL_PROGRESS, LVIR_BOUNDS, &cellRect); InvalidateRect(hWndListView, &cellRect, FALSE); break; }
            } free(info); break;
        }

        case WM_FILE_CANCEL: {
            char* canceledId = (char*)lParam;
            if (canceledId) {
                int count = ListView_GetItemCount(hWndListView);
                for (int i = 0; i < count; i++) {
                    LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListView, &lvi); LoggedTransfer* transfer = (LoggedTransfer*)lvi.lParam;
                    if (transfer && strcmp(transfer->fileId, canceledId) == 0) { transfer->stato = STATE_CANCELED; ListView_Update(hWndListView, i); break; }
                }
                SetWindowTextA(hWndStatus, "Transfer canceled by sender."); UpdateStatusIcon(ICON_CANCELED); ShowWindow(hWndBtnCloseStatus, SW_SHOW); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); free(canceledId);
            } break;
        }

        case WM_DEVICE_DISCOVERED: {
            RemoteDevice* pDevice = (RemoteDevice*)lParam;
            if (pDevice) {
                int count = ListView_GetItemCount(hWndListDevices); BOOL bAlreadyExists = FALSE; int itemIndex = -1;
                for (int i = 0; i < count; i++) {
                    LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListDevices, &lvi); RemoteDevice* pExisting = (RemoteDevice*)lvi.lParam;
                    if (pExisting && strcmp(pExisting->ipAddress, pDevice->ipAddress) == 0) { bAlreadyExists = TRUE; itemIndex = i; free(pExisting); break; }
                }
                char infoBuf[256];
                const char* lastDot = strrchr(pDevice->ipAddress, '.');
                const char* devHashtag = lastDot ? (lastDot + 1) : pDevice->ipAddress;
                _snprintf(infoBuf, sizeof(infoBuf), "#%s (%s)", devHashtag, pDevice->deviceType);
                LVITEMA lvi = {0}; lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM; lvi.iSubItem = 0; lvi.pszText = pDevice->alias; lvi.iImage = GetDeviceIconIndex(pDevice->deviceType); lvi.lParam = (LPARAM)pDevice;
                if (bAlreadyExists) { lvi.iItem = itemIndex; ListView_SetItem(hWndListDevices, &lvi); ListView_SetItemText(hWndListDevices, itemIndex, 1, infoBuf); } else { lvi.iItem = count; int insertedIdx = ListView_InsertItem(hWndListDevices, &lvi); ListView_SetItemText(hWndListDevices, insertedIdx, 1, infoBuf); }
            } UpdateSendButtonsState(); break;
        }

        case WM_CONFIRM_TRANSFER: {
            ConfirmationRequest* req = (ConfirmationRequest*)lParam;
            if (req) {
                req->accepted = ShowTransferConfirmation(hWnd, req->senderName, req->fileCount, req->selectedPath);
                SetEvent(req->hEvent);
            }
            break;
        }

        case WM_REQUEST_PIN: {
            PinRequest* req = (PinRequest*)lParam;
            if (req) {
                req->success = ShowPinPrompt(hWnd, req->targetName, req->pinCode);
                SetEvent(req->hEvent);
            }
            break;
        }

        case WM_FILE_COMPLETE: { SetWindowTextA(hWndStatus, "All transfers complete."); UpdateStatusIcon(ICON_SUCCESS); ShowWindow(hWndBtnCloseStatus, SW_SHOW); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); break; }
        case WM_CLOSE: { SaveSettings(); if (IsDlgButtonChecked(hWnd, IDC_SET_MIN_CLOSE)) ShowWindow(hWnd, SW_HIDE); else DestroyWindow(hWnd); break; }
        case WM_DESTROY: {
            SaveSettings();
            if (hNormalFont) DeleteObject(hNormalFont);
            if (hLargeFont) DeleteObject(hLargeFont);
            if (hStatusFont) DeleteObject(hStatusFont);
            if (hDeviceImageList) ImageList_Destroy(hDeviceImageList);
            HICON hCurrentIcon = (HICON)SendMessage(hWndStatusIcon, STM_GETICON, 0, 0); if (hCurrentIcon) DestroyIcon(hCurrentIcon); Shell_NotifyIconA(NIM_DELETE, &nid);
            if (hWndListDevices) { int devCount = ListView_GetItemCount(hWndListDevices); for (int i = 0; i < devCount; i++) { LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListDevices, &lvi); if (lvi.lParam) free((void*)lvi.lParam); } }
            PostQuitMessage(0); break;
        }
        default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Fetches resource icons from ddores.dll for corresponding device type
int GetDeviceIconIndex(const char* deviceType){
    char sysPath[MAX_PATH]; GetSystemDirectoryA(sysPath, MAX_PATH); strcat(sysPath, "\\ddores.dll"); int iconIndex = 11; // default to mobile/phone icon
    if (deviceType){
        if (_stricmp(deviceType, "Laptop") == 0) iconIndex = 12;
        else if (_stricmp(deviceType, "Web") == 0) iconIndex = 48;
        else if (_stricmp(deviceType, "Terminal") == 0) iconIndex = 9;
        else if (_stricmp(deviceType, "Server") == 0) iconIndex = 13;
        else if (_stricmp(deviceType, "Phone") == 0) iconIndex = 11;
    }
    HICON hIcon = NULL; UINT iconId = 0;
    if (PrivateExtractIconsA(sysPath, iconIndex, 32, 32, &hIcon, &iconId, 1, 0) > 0 && hIcon != NULL){ int idx = ImageList_AddIcon(hDeviceImageList, hIcon); DestroyIcon(hIcon); return idx; } return -1;
}

// Adds a file to queue list and prepares metadata for sending
void AddFileToSendQueue(const char* filePath, const char* fileName, long long fileSize) {
    if (g_sendQueueCount >= MAX_SEND_FILES) { MessageBoxA(g_hWndMain, "You've added the maximum number of files possible.", "Warning", MB_ICONWARNING); return; }
    for (int i = 0; i < g_sendQueueCount; i++) { if (strcmp(g_sendQueue[i].filePath, filePath) == 0) return; }
    strncpy(g_sendQueue[g_sendQueueCount].filePath, filePath, 511); strncpy(g_sendQueue[g_sendQueueCount].fileName, fileName, 255);
    g_sendQueue[g_sendQueueCount].fileSize = fileSize; sprintf(g_sendQueue[g_sendQueueCount].fileId, "file-%ld-%d", GetTickCount(), g_sendQueueCount);
    char sizeStr[32]; if (fileSize < 1024) sprintf(sizeStr, "%lld B", fileSize); else if (fileSize < 1024 * 1024) sprintf(sizeStr, "%.1f KB", fileSize / 1024.0); else sprintf(sizeStr, "%.1f MB", fileSize / (1024.0 * 1024.0));
    LVITEMA lvi = {0}; lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM; lvi.iItem = ListView_GetItemCount(hWndListSendFiles);
    lvi.iSubItem = 0; lvi.pszText = (char*)fileName; lvi.iImage = GetSystemIconIndex(fileName); lvi.lParam = (LPARAM)g_sendQueueCount;
    int idx = ListView_InsertItem(hWndListSendFiles, &lvi); ListView_SetItemText(hWndListSendFiles, idx, 1, sizeStr);
    g_sendQueueCount++; UpdateSendButtonsState();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX icex; icex.dwSize = sizeof(INITCOMMONCONTROLSEX); icex.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES; InitCommonControlsEx(&icex);
    InitSettingsPath();
    InitLanguage();
#ifndef __arm__
    HMODULE hCrypto = LoadLibraryA("libcrypto-3.dll");
    HMODULE hSsl = LoadLibraryA("libssl-3.dll");
    if (!hCrypto || !hSsl) {
        const char* errMsg = "";
        if (!hCrypto && !hSsl) errMsg = g_Lang.errBothMissing;
        else if (!hCrypto) errMsg = g_Lang.errCryptoMissing;
        else errMsg = g_Lang.errSslMissing;
        MessageBoxA(NULL, errMsg, "Error", MB_ICONERROR);
        if (hCrypto) FreeLibrary(hCrypto);
        if (hSsl) FreeLibrary(hSsl);
        return 1;
    }
    FreeLibrary(hCrypto);
    FreeLibrary(hSsl);
#endif
    autoFirewall(); if (!initWinsock()) return 1;
    TlsInitGlobal();
    hInstance = GetModuleHandle(NULL);
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "LocalSendRT_GUI";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(101));
    if (!RegisterClassExA(&wc)) return 1;
    g_hWndMain = CreateWindowExA(0, "LocalSendRT_GUI", APP_NAME, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 800, 420, NULL, NULL, hInstance, NULL);
    if (!g_hWndMain) return 1;
    nid.cbSize = sizeof(NOTIFYICONDATAA); nid.hWnd = g_hWndMain; nid.uID = 1; nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP; nid.uCallbackMessage = WM_TRAYICON_MSG; nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(101)); strcpy(nid.szTip, APP_NAME); Shell_NotifyIconA(NIM_ADD, &nid);
 
    // Initialize COM libraries for the folder selection dialog
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
 
    ShowWindow(g_hWndMain, SW_SHOW); UpdateWindow(g_hWndMain);
    g_mySocket = createUdpSocket(); if (g_mySocket != INVALID_SOCKET && joinMulticastGroup(g_mySocket)) { HANDLE hUdpThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)startListeningLoop, (LPVOID)g_mySocket, 0, NULL); if (hUdpThread) CloseHandle(hUdpThread); }
    HANDLE hTcpThread = CreateThread(NULL, 0, tcpServerThread, NULL, 0, NULL); if (hTcpThread) CloseHandle(hTcpThread);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    if (g_mySocket != INVALID_SOCKET) {
        closesocket(g_mySocket);
    }
    WSACleanup();
    TlsCleanupGlobal();
    CoUninitialize();
    return 0;
}


