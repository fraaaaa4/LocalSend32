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
#include <process.h>

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
#include "layout.h"
#include "tray_menu.h"

void ApplyWindowFont(HWND hWndChild);
void ApplyLargeFont(HWND hWndChild);
void ApplyStatusFont(HWND hWndChild);
void UpdateStatusIcon(StatusIconType type);
void UpdateSendStatusIcon(StatusIconType type);
void AddTooltip(HWND hCtrl, char* text);
void UpdateTooltipText(HWND hCtrl, char* text);
void GetDeviceNetworkInfo(char* outBuffer, size_t maxLen, char* outIpStr);
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
int g_TcpServerStatus = 0;

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
HWND hWndSendProgress = NULL, hWndSendStatusTxt = NULL, hWndSendStatusIcon = NULL, hWndBtnCancelSend = NULL;

HWND hWndCheckSavePos = NULL, hWndCheckMinClose = NULL, hWndCheckTopmost = NULL, hWndCheckRecursiveFolder = NULL;
HWND hWndCheckQuickSave = NULL;
HWND hWndLblLanguage = NULL, hWndComboLanguage = NULL;
HWND hWndRecvLbl = NULL, hWndRecvRadApp = NULL, hWndRecvRadDl = NULL, hWndRecvRadCustom = NULL, hWndRecvTxtPath = NULL, hWndRecvBtnBrowse = NULL;
HWND hWndBtnGithub = NULL, hWndBtnSupport = NULL, hWndBtnHelp = NULL, hWndLabelAbout = NULL, hWndIconStatic = NULL;

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
LoggedTransfer g_transfers[100];
int g_transfersCount = 0, g_SplitterPos = 360;
bool g_bDraggingSplitter = false;

// Assigns standard UI font to the given child control
void ApplyWindowFont(HWND hWndChild) {
    if (hNormalFont == NULL) {
        NONCLIENTMETRICSA ncm;
        memset(&ncm, 0, sizeof(NONCLIENTMETRICSA));
        ncm.cbSize = sizeof(NONCLIENTMETRICSA);
        // Query system default font dimensions from OS metrics
        if (!SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), &ncm, 0)) {
            ncm.cbSize = sizeof(NONCLIENTMETRICSA) - sizeof(int);
            if (!SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0)) {
                hNormalFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            }
        }
        if (hNormalFont == NULL) {
            // Select appropriate font family according to active language
            if (g_Language == LANG_zh_CN) {
                strcpy(ncm.lfMessageFont.lfFaceName, "Microsoft YaHei");
                ncm.lfMessageFont.lfCharSet = GB2312_CHARSET;
            } else {
                ncm.lfMessageFont.lfCharSet = DEFAULT_CHARSET;
            }
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
        BYTE charSet = DEFAULT_CHARSET;
        if (g_Language == LANG_zh_CN) {
            faceName = "Microsoft YaHei";
            charSet = GB2312_CHARSET;
        } else if (ok) {
            height = ncm.lfMessageFont.lfHeight * 1.5;
            faceName = ncm.lfMessageFont.lfFaceName;
        }

        hLargeFont = CreateFontA(height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, charSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, faceName);
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
        const char* faceName = "Segoe UI";
        BYTE charSet = DEFAULT_CHARSET;
        if (g_Language == LANG_zh_CN) {
            faceName = "Microsoft YaHei";
            charSet = GB2312_CHARSET;
        } else if (ok) {
            height = ncm.lfMessageFont.lfHeight - 2;
            faceName = ncm.lfMessageFont.lfFaceName;
        }

        hStatusFont = CreateFontA(height, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, charSet, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, faceName);
        if (hStatusFont == NULL) {
            hStatusFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
    }
    SendMessage(hWndChild, WM_SETFONT, (WPARAM)hStatusFont, TRUE);
}

// Frees and invalidates cached GDI font handles when language changes
static void RecreateFonts() {
    if (hNormalFont) { DeleteObject(hNormalFont); hNormalFont = NULL; }
    if (hLargeFont) { DeleteObject(hLargeFont); hLargeFont = NULL; }
    if (hStatusFont) { DeleteObject(hStatusFont); hStatusFont = NULL; }
}

// EnumChildWindows callback to apply updated font style across all child controls
static BOOL CALLBACK ApplyFontToChild(HWND hwnd, LPARAM lParam) {
    if (hwnd == hWndDeviceName) {
        ApplyLargeFont(hwnd);
    } else if (hwnd == hWndStatus || hwnd == hWndLblSendFiles || hwnd == hWndSendStatusTxt || hwnd == hWndLblDevices) {
        ApplyStatusFont(hwnd);
    } else {
        ApplyWindowFont(hwnd);
    }
    return TRUE;
}

// Swaps the visual status icon on the receive progress page
void UpdateStatusIcon(StatusIconType type) {
    char sysPath[MAX_PATH]; GetSystemDirectoryA(sysPath, MAX_PATH); strcat(sysPath, "\\shell32.dll");
    int iconIndex = (type == ICON_WAITING) ? 54 : (type == ICON_TRANSFERRING) ? 89 : (type == ICON_SUCCESS) ? 301 : 131;
    HICON hIcon = NULL;
    if (SafeExtractIcon(sysPath, iconIndex, 16, 16, &hIcon) && hIcon != NULL) {
        HICON hOldIcon = (HICON)SendMessage(hWndStatusIcon, STM_SETICON, (WPARAM)hIcon, 0); if (hOldIcon) DestroyIcon(hOldIcon);
    }
}

// Swaps the status icon on the sending page
void UpdateSendStatusIcon(StatusIconType type) {
    char sysPath[MAX_PATH]; GetSystemDirectoryA(sysPath, MAX_PATH); strcat(sysPath, "\\shell32.dll");
    int iconIndex = (type == ICON_WAITING) ? 54 : (type == ICON_TRANSFERRING) ? 89 : (type == ICON_SUCCESS) ? 301 : 131;
    HICON hIcon = NULL;
    if (SafeExtractIcon(sysPath, iconIndex, 16, 16, &hIcon) && hIcon != NULL) {
        HICON hOldIcon = (HICON)SendMessage(hWndSendStatusIcon, STM_SETICON, (WPARAM)hIcon, 0); if (hOldIcon) DestroyIcon(hOldIcon);
    }
}

// Associates a tooltip helper with a control
void AddTooltip(HWND hCtrl, char* text) {
#ifndef LOCALSEND_NT
    if (!hWndToolTip && g_hWndMain) {
        hWndToolTip = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, g_hWndMain, NULL, GetModuleHandle(NULL), NULL);
        if (hWndToolTip) SetWindowPos(hWndToolTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    if (hWndToolTip && hCtrl) {
        TOOLINFOA ti = {0}; ti.cbSize = 40; ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND; ti.hwnd = g_hWndMain; ti.uId = (UINT_PTR)hCtrl; ti.lpszText = text;
        SendMessage(hWndToolTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
    }
#endif
}

// Converts UTF-8 string to Wide string and sets control window text
static void SetWindowTextUTF8(HWND hWnd, const char* utf8Str) {
    if (!hWnd || !utf8Str) return;
    int wLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wLen > 0) {
        wchar_t* wStr = (wchar_t*)malloc(wLen * sizeof(wchar_t));
        if (wStr) {
            MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wStr, wLen);
            SetWindowTextW(hWnd, wStr);
            free(wStr);
        }
    }
}

// Updates tab title with UTF-8 string
static void TabCtrl_SetItemUTF8(HWND hWndTab, int index, const char* utf8Str) {
    if (!hWndTab || !utf8Str) return;
    int wLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wLen > 0) {
        wchar_t* wStr = (wchar_t*)malloc(wLen * sizeof(wchar_t));
        if (wStr) {
            MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wStr, wLen);
            TCITEMW tie = {0};
            tie.mask = TCIF_TEXT;
            tie.pszText = wStr;
            SendMessageW(hWndTab, TCM_SETITEMW, index, (LPARAM)&tie);
            free(wStr);
        }
    }
}

static void ListView_SetColumnUTF8(HWND hWndLV, int colIndex, const char* utf8Str) {
    if (!hWndLV || !utf8Str) return;
    int wLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wLen > 0) {
        wchar_t* wStr = (wchar_t*)malloc(wLen * sizeof(wchar_t));
        if (wStr) {
            MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wStr, wLen);
            LVCOLUMNW lvc = {0};
            lvc.mask = LVCF_TEXT;
            lvc.pszText = wStr;
            SendMessageW(hWndLV, LVM_SETCOLUMNW, colIndex, (LPARAM)&lvc);
            free(wStr);
        }
    }
}

// Dynamically changes tooltip text for localized strings
void UpdateTooltipText(HWND hCtrl, char* text) {
    if (!hWndToolTip) return;
    int wLen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (wLen > 0) {
        wchar_t* wStr = (wchar_t*)malloc(wLen * sizeof(wchar_t));
        if (wStr) {
            MultiByteToWideChar(CP_UTF8, 0, text, -1, wStr, wLen);
            TOOLINFOW ti = {0};
            ti.cbSize = sizeof(TOOLINFOW);
            ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
            ti.hwnd = g_hWndMain;
            ti.uId = (UINT_PTR)hCtrl;
            ti.lpszText = wStr;
            SendMessageW(hWndToolTip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
            free(wStr);
        }
    }
}

// Get current IP and hashtag. 127.0.0.1 means offline
void GetDeviceNetworkInfo(char* outBuffer, size_t maxLen, char* outIpStr) {
    char ipStr[32] = "127.0.0.1"; char hostName[256] = {0};
    if (gethostname(hostName, sizeof(hostName)) == 0) {
        struct hostent* phe = gethostbyname(hostName);
        if (phe && phe->h_addr_list && phe->h_addr_list[0]) {
            struct in_addr addr; memcpy(&addr, phe->h_addr_list[0], sizeof(struct in_addr)); strcpy(ipStr, inet_ntoa(addr));
        }
    }
    if (outIpStr) {
        strcpy(outIpStr, ipStr);
    }
    const char* lastDot = strrchr(ipStr, '.');
    const char* myHashtag = lastDot ? (lastDot + 1) : ipStr;
    
    BOOL isOffline = (strcmp(ipStr, "127.0.0.1") == 0);
    const char* statusStr = isOffline ? g_Lang.lblDisconnected :
                            (g_TcpServerStatus == 1) ? g_Lang.lblStatusReady :
                            (g_TcpServerStatus == 0) ? g_Lang.lblStatusStarting :
                                                       g_Lang.lblStatusError;
                                                       
    _snprintf(outBuffer, maxLen, "%s\r\n%s#%s\r\n%s%s\r\n%s%d", statusStr, g_Lang.lblHashtag, myHashtag, g_Lang.lblIpAddress, ipStr, g_Lang.lblActivePort, g_Port);
}

// Update the network status block and main waiting label if the user disconnected the network adapter
void UpdateNetworkInfoText(void) {
    char ipStr[32] = {0};
    char infoNet[512] = {0};
    GetDeviceNetworkInfo(infoNet, sizeof(infoNet), ipStr);

    if (hWndInfoText) {
        SetWindowTextA(hWndInfoText, infoNet);
        InvalidateRect(hWndInfoText, NULL, TRUE);
        UpdateWindow(hWndInfoText);
    }
    
    BOOL isOffline = (strcmp(ipStr, "127.0.0.1") == 0);
    if (hWndStatus && !IsWindowVisible(hWndBtnCloseStatus)) {
        if (isOffline) {
            SetWindowTextUTF8(hWndStatus, g_Lang.lblDisconnected);
            UpdateStatusIcon(ICON_CANCELED);
        } else {
            SetWindowTextUTF8(hWndStatus, g_Lang.msgWaitingForFiles);
            UpdateStatusIcon(ICON_WAITING);
        }
    }
}

// Finds and registers file icon from shell registry
int GetSystemIconIndex(const char* fileName) {
    SHFILEINFOA sfi = {0};
    HIMAGELIST hInstIL = (HIMAGELIST)SHGetFileInfoA(fileName, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    if (hSystemImageList == NULL && hInstIL != NULL) { hSystemImageList = hInstIL; ListView_SetImageList(hWndListView, hSystemImageList, LVSIL_SMALL); ListView_SetImageList(hWndListSendFiles, hSystemImageList, LVSIL_SMALL); }
    return sfi.iIcon;
}

// Translates all visual labels and controls to the selected language
void UpdateUITexts() {
    if (!hWndTab) return;

    TabCtrl_SetItemUTF8(hWndTab, 0, g_Lang.tabReceive);
    TabCtrl_SetItemUTF8(hWndTab, 1, g_Lang.tabSend);
    TabCtrl_SetItemUTF8(hWndTab, 2, g_Lang.tabSettings);

    if (hWndSettingsTab) {
        TabCtrl_SetItemUTF8(hWndSettingsTab, 0, g_Lang.settingsGeneral);
        TabCtrl_SetItemUTF8(hWndSettingsTab, 1, g_Lang.settingsReceive);
        TabCtrl_SetItemUTF8(hWndSettingsTab, 2, g_Lang.settingsNetwork);
        TabCtrl_SetItemUTF8(hWndSettingsTab, 3, g_Lang.settingsOther);
    }

    if (hWndCheckSavePos) SetWindowTextUTF8(hWndCheckSavePos, g_Lang.saveWindowPos);
    if (hWndCheckMinClose) SetWindowTextUTF8(hWndCheckMinClose, g_Lang.minimizeToTray);
    if (hWndCheckTopmost) SetWindowTextUTF8(hWndCheckTopmost, g_Lang.alwaysOnTop);
    if (hWndCheckRecursiveFolder) SetWindowTextUTF8(hWndCheckRecursiveFolder, g_Lang.setRecursiveFolder);
    if (hWndLblLanguage) SetWindowTextUTF8(hWndLblLanguage, g_Lang.language);

    if (hWndDeviceNameTitle) SetWindowTextUTF8(hWndDeviceNameTitle, g_Lang.deviceName);
    if (hWndBtnEditDevice) SetWindowTextUTF8(hWndBtnEditDevice, g_Lang.editBtn);
    if (hWndBtnSaveDevice) SetWindowTextUTF8(hWndBtnSaveDevice, g_Lang.saveBtn);
    if (hWndBtnCancelDevice) SetWindowTextUTF8(hWndBtnCancelDevice, g_Lang.cancelBtn);
    if (hWndGroupBox) SetWindowTextUTF8(hWndGroupBox, g_Lang.networkInfo);
    if (hWndStatus && !IsWindowVisible(hWndBtnCloseStatus)) SetWindowTextUTF8(hWndStatus, g_Lang.waitingForFiles);
    if (hWndBtnCloseStatus) SetWindowTextUTF8(hWndBtnCloseStatus, g_Lang.closeBtn);

    if (hWndRecvLbl) SetWindowTextUTF8(hWndRecvLbl, g_Lang.whenIReceive);
    if (hWndRecvRadApp) SetWindowTextUTF8(hWndRecvRadApp, g_Lang.saveAppPath);
    if (hWndRecvRadDl) SetWindowTextUTF8(hWndRecvRadDl, g_Lang.saveDownloads);
    if (hWndRecvRadCustom) SetWindowTextUTF8(hWndRecvRadCustom, g_Lang.saveCustomPath);
    if (hWndRecvBtnBrowse) SetWindowTextUTF8(hWndRecvBtnBrowse, g_Lang.browse);
    if (hWndCheckQuickSave) SetWindowTextUTF8(hWndCheckQuickSave, g_Lang.quickSave);

    if (hWndCheckReqPin) SetWindowTextUTF8(hWndCheckReqPin, g_Lang.requirePin);
    if (hWndLblPinCode) SetWindowTextUTF8(hWndLblPinCode, g_Lang.pinCode);
    if (hWndLblPort) SetWindowTextUTF8(hWndLblPort, g_Lang.port);
    if (hWndLblDiscTimeout) SetWindowTextUTF8(hWndLblDiscTimeout, g_Lang.lblDiscTimeout);
    if (hWndLblMulticast) SetWindowTextUTF8(hWndLblMulticast, g_Lang.lblMulticast);
    if (hWndCheckEncryption) SetWindowTextUTF8(hWndCheckEncryption, g_Lang.lblEncryption);
    if (hWndLblDevType) SetWindowTextUTF8(hWndLblDevType, g_Lang.lblDeviceType);
    if (hWndLblDevModel) SetWindowTextUTF8(hWndLblDevModel, g_Lang.lblDeviceModel);

    // Refresh localized device type choices in Settings dropdown
    if (hWndComboDevType) {
        int sel = (int)SendMessage(hWndComboDevType, CB_GETCURSEL, 0, 0);
        SendMessage(hWndComboDevType, CB_RESETCONTENT, 0, 0);
        SendMessageA(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)g_Lang.devTypePhone);
        SendMessageA(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)g_Lang.devTypeLaptop);
        SendMessageA(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)g_Lang.devTypeWeb);
        SendMessageA(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)g_Lang.devTypeTerminal);
        SendMessageA(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)g_Lang.devTypeServer);
        if (sel >= 0) SendMessage(hWndComboDevType, CB_SETCURSEL, sel, 0);
    }

    if (hWndLabelAbout) {
        char aboutTxt[512]; _snprintf(aboutTxt, sizeof(aboutTxt), "%s\r\nVersion: " APP_VERSION "\r\nPublisher: Fratta\r\n\r\n%s", APP_NAME, APP_ABOUT);
        SetWindowTextUTF8(hWndLabelAbout, aboutTxt);
    }
    if (hWndBtnGithub) SetWindowTextUTF8(hWndBtnGithub, g_Lang.aboutBtn);
    if (hWndBtnSupport) SetWindowTextUTF8(hWndBtnSupport, g_Lang.supportBtn);
    if (hWndBtnHelp) SetWindowTextUTF8(hWndBtnHelp, g_Lang.btnHelp);

    if (hWndBtnCleanFiles) SetWindowTextUTF8(hWndBtnCleanFiles, g_Lang.cleanFiles);
    if (hWndBtnSendFiles) SetWindowTextUTF8(hWndBtnSendFiles, g_Lang.sendFiles);
    if (hWndLblDevices) SetWindowTextUTF8(hWndLblDevices, g_Lang.nearbyDevices);
    if (hWndBtnSearchAgain) SetWindowTextUTF8(hWndBtnSearchAgain, g_Lang.searchAgain);
    if (hWndBtnSendManual) SetWindowTextUTF8(hWndBtnSendManual, g_Lang.sendManually);
    UpdateNetworkInfoText();

    // Update list view columns
    if (hWndListView) {
        ListView_SetColumnUTF8(hWndListView, LV_COL_NAME, g_Lang.colFile);
        ListView_SetColumnUTF8(hWndListView, LV_COL_SIZE, g_Lang.colSize);
        ListView_SetColumnUTF8(hWndListView, LV_COL_DATE, g_Lang.colDate);
        ListView_SetColumnUTF8(hWndListView, LV_COL_PROGRESS, g_Lang.colProgress);

        LVGROUP lvg = {0}; lvg.cbSize = sizeof(LVGROUP); lvg.mask = LVGF_HEADER;
        wchar_t wToday[64]; MultiByteToWideChar(CP_UTF8, 0, g_Lang.grpToday, -1, wToday, 64);
        wchar_t wOlder[64]; MultiByteToWideChar(CP_UTF8, 0, g_Lang.grpOlder, -1, wOlder, 64);
        lvg.pszHeader = wToday; ListView_SetGroupInfo(hWndListView, 1, &lvg);
        lvg.pszHeader = wOlder; ListView_SetGroupInfo(hWndListView, 2, &lvg);
    }
    if (hWndListSendFiles) {
        ListView_SetColumnUTF8(hWndListSendFiles, 0, g_Lang.colFile);
        ListView_SetColumnUTF8(hWndListSendFiles, 1, g_Lang.colSize);
    }
    if (hWndListDevices) {
        ListView_SetColumnUTF8(hWndListDevices, 0, g_Lang.colDevice);
        ListView_SetColumnUTF8(hWndListDevices, 1, g_Lang.colInfo);
    }

    // Update Action buttons
    if (hWndBtnSndFile) SetWindowTextUTF8(hWndBtnSndFile, g_Lang.btnFile);
    if (hWndBtnSndFolder) SetWindowTextUTF8(hWndBtnSndFolder, g_Lang.btnFolder);
    if (hWndBtnSndText) SetWindowTextUTF8(hWndBtnSndText, g_Lang.btnText);
    if (hWndBtnSndPaste) SetWindowTextUTF8(hWndBtnSndPaste, g_Lang.btnPaste);
    if (hWndLblSendFiles) SetWindowTextUTF8(hWndLblSendFiles, g_Lang.filesToSend);
    if (hWndBtnCancelSend) SetWindowTextUTF8(hWndBtnCancelSend, g_Lang.cancelBtn);

    // Update Tooltips
    if (hWndBtnSndFile) UpdateTooltipText(hWndBtnSndFile, g_Lang.addFilesTooltip);
    if (hWndBtnSndFolder) UpdateTooltipText(hWndBtnSndFolder, g_Lang.addFolderTooltip);
    if (hWndBtnSndText) UpdateTooltipText(hWndBtnSndText, g_Lang.sendTextTooltip);
    if (hWndBtnSndPaste) UpdateTooltipText(hWndBtnSndPaste, g_Lang.pasteTooltip);
}

// Enables or disables the Send files buttons based on queue and device selections
void UpdateSendButtonsState() {
    BOOL hasFiles = (g_sendQueueCount > 0);
    int checkedCount = 0;
    int listCount = ListView_GetItemCount(hWndListDevices);
    for (int i = 0; i < listCount; i++) {
        if (ListView_GetCheckState(hWndListDevices, i)) {
            checkedCount++;
        }
    }
    EnableWindow(hWndBtnCleanFiles, hasFiles); 
    EnableWindow(hWndBtnSendFiles, hasFiles && (checkedCount > 0));
}

// Helper to recursively or non-recursively add all files from a directory into send queue
void AddDirectoryFilesRecursive(const char* baseDir, bool recursive) {
    char searchPath[MAX_PATH];
    _snprintf(searchPath, sizeof(searchPath), "%s\\*.*", baseDir);
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(searchPath, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) continue;
        char fullFilePath[MAX_PATH];
        _snprintf(fullFilePath, sizeof(fullFilePath), "%s\\%s", baseDir, ffd.cFileName);
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive) {
                AddDirectoryFilesRecursive(fullFilePath, true);
            }
        } else {
            long long fileSize = ((long long)ffd.nFileSizeHigh << 32) | (long long)ffd.nFileSizeLow;
            AddFileToSendQueue(fullFilePath, ffd.cFileName, fileSize);
        }
    } while (FindNextFileA(hFind, &ffd));
    FindClose(hFind);
}

// Processes dropped files/folders from Drag and Drop
void ProcessDroppedFiles(HDROP hDrop) {
    UINT fileCount = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);
    for (UINT i = 0; i < fileCount; i++) {
        char filePath[MAX_PATH];
        DragQueryFileA(hDrop, i, filePath, sizeof(filePath));
        
        DWORD attr = GetFileAttributesA(filePath);
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            AddDirectoryFilesRecursive(filePath, (g_RecursiveFolder != 0));
        } else {
            HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                long long fileSize = GetFileSizeBytes(hFile);
                const char* fileName = strrchr(filePath, '\\');
                if (!fileName) fileName = strrchr(filePath, '/');
                if (fileName) fileName++;
                else fileName = filePath;
                
                AddFileToSendQueue(filePath, fileName, fileSize);
                CloseHandle(hFile);
            }
        }
    }
    DragFinish(hDrop);
}

// Handles command messages sent to the main window
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
    if (wmId == IDC_SET_BTN_HELP) {
        char exePath[MAX_PATH] = {0};
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash) *lastSlash = '\0';
        char chmPath[MAX_PATH] = {0};
        _snprintf(chmPath, sizeof(chmPath), "%s\\LocalSend32.chm", exePath);
        ShellExecuteA(NULL, "open", chmPath, NULL, NULL, SW_SHOWNORMAL);
        return TRUE;
    }

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
        char fileBuffer[4096] = {0};
        char filter[256] = {0};
        _snprintf(filter, sizeof(filter), "%s", g_Lang.filterAllFiles);
        int len = strlen(filter);
        filter[len + 1] = '*';
        filter[len + 2] = '.';
        filter[len + 3] = '*';
        filter[len + 4] = '\0';
        filter[len + 5] = '\0';
        
        OPENFILENAMEA ofn = {0};
#ifdef LOCALSEND_NT
        ofn.lStructSize = 76; // OPENFILENAME_SIZE_VERSION_400
#else
        ofn.lStructSize = sizeof(ofn);
#endif
        ofn.hwndOwner = hWnd;
        ofn.lpstrFile = fileBuffer;
        ofn.nMaxFile = sizeof(fileBuffer);
        ofn.lpstrFilter = filter;
        ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;
        if (GetOpenFileNameA(&ofn)) {
            char* p = fileBuffer; char dir[512]; strncpy(dir, p, 511); p += strlen(p) + 1;
            if (*p == '\0') { char* fileName = strrchr(dir, '\\'); if (fileName) { fileName++; HANDLE hFile = CreateFileA(dir, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); if (hFile != INVALID_HANDLE_VALUE) { long long fs = GetFileSizeBytes(hFile); CloseHandle(hFile); AddFileToSendQueue(dir, fileName, fs); } } }
            else { while (*p) { char fullPath[512]; sprintf(fullPath, "%s\\%s", dir, p); HANDLE hFile = CreateFileA(fullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); if (hFile != INVALID_HANDLE_VALUE) { long long fs = GetFileSizeBytes(hFile); CloseHandle(hFile); AddFileToSendQueue(fullPath, p, fs); } p += strlen(p) + 1; } }
        }
        return TRUE;
    }

    if (wmId == IDC_BTN_SEND_FOLDER) {
        BROWSEINFOA bi = {0}; bi.hwndOwner = hWnd; bi.lpszTitle = g_Lang.selectFolderToSend; bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI; LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        if (pidl != NULL) {
            char folderPath[MAX_PATH];
            if (SHGetPathFromIDListA(pidl, folderPath)) {
                AddDirectoryFilesRecursive(folderPath, (g_RecursiveFolder != 0));
            }
            CoTaskMemFree(pidl);
        }
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
            else if (IsClipboardFormatAvailable(CF_HDROP)) { HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP); if (hDrop) { int fileCount = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0); for (int i = 0; i < fileCount; i++) { char filePath[MAX_PATH]; DragQueryFileA(hDrop, i, filePath, MAX_PATH); char* fileName = strrchr(filePath, '\\'); fileName = fileName ? fileName + 1 : filePath; HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); if (hFile != INVALID_HANDLE_VALUE) { long long fs = GetFileSizeBytes(hFile); CloseHandle(hFile); AddFileToSendQueue(filePath, fileName, fs); } } } }
            else if (IsClipboardFormatAvailable(CF_BITMAP)) { HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP); if (hBitmap) { char tempPath[MAX_PATH], bmpPath[MAX_PATH]; GetTempPathA(MAX_PATH, tempPath); sprintf(bmpPath, "%s\\clip_%ld.bmp", tempPath, GetTickCount()); BITMAP bmp; HDC hdc = GetDC(hWnd); GetObject(hBitmap, sizeof(BITMAP), &bmp); BITMAPFILEHEADER bfh = {0}; bfh.bfType = 0x4D42; bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); bfh.bfSize = bfh.bfOffBits + bmp.bmWidthBytes * bmp.bmHeight; BITMAPINFOHEADER bih = {0}; bih.biSize = sizeof(BITMAPINFOHEADER); bih.biWidth = bmp.bmWidth; bih.biHeight = bmp.bmHeight; bih.biPlanes = 1; bih.biBitCount = bmp.bmBitsPixel; bih.biCompression = BI_RGB; FILE* f = fopen(bmpPath, "wb"); if (f) { fwrite(&bfh, 1, sizeof(bfh), f); fwrite(&bih, 1, sizeof(bih), f); char* pBits = (char*)malloc(bmp.bmWidthBytes * bmp.bmHeight); GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, pBits, (BITMAPINFO*)&bih, DIB_RGB_COLORS); fwrite(pBits, 1, bmp.bmWidthBytes * bmp.bmHeight, f); free(pBits); fclose(f); AddFileToSendQueue(bmpPath, "immagine_appunti.bmp", bfh.bfSize); } ReleaseDC(hWnd, hdc); } }
            CloseClipboard();
        }
        return TRUE;
    }

    if (wmId == IDC_BTN_CLEAN_FILES) {
        ClearSendQueue();
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
            MessageBoxA(hWnd, g_Lang.msgSelectDevice, g_Lang.titleNoSelection, MB_ICONWARNING);
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
                        txCtx->targets[tgtIdx].isHttps = dev->isHttps;
                        tgtIdx++;
                    }
                }
            }
            txCtx->targetCount = tgtIdx;
            txCtx->fileCount = g_sendQueueCount;
            memcpy(txCtx->files, g_sendQueue, sizeof(FileToSend) * g_sendQueueCount);
            ShowWindow(hWndSendProgress, SW_SHOW); SendMessage(hWndSendProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); SendMessage(hWndSendProgress, PBM_SETPOS, 0, 0);
            char* copyMsg = (char*)malloc(256);
            if (copyMsg) {
                _snprintf(copyMsg, 256, g_Lang.msgConnectingTo, txCtx->targets[0].alias);
                PostMessage(hWnd, WM_SEND_STATUS_UPDATE, ICON_TRANSFERRING, (LPARAM)copyMsg);
            }
            DWORD thId = 0;
            HANDLE hTxThread = CreateThread(NULL, 0, StartSendSessionThread, (LPVOID)txCtx, 0, &thId);
            if (hTxThread) CloseHandle(hTxThread);
        }
        return TRUE;
    }

    // Handles user cancelling active sending session from Send tab
    if (wmId == IDC_BTN_CANCEL_SEND) {
        g_bCancelSendSession = true;
        ShowWindow(hWndBtnCancelSend, SW_HIDE);
        ShowWindow(hWndSendProgress, SW_HIDE);
        SetWindowTextA(hWndSendStatusTxt, g_Lang.msgTransferCanceled);
        UpdateSendStatusIcon(ICON_CANCELED);
        SetTimer(hWnd, 998, 4000, NULL);
        RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom);
        InvalidateRect(hWnd, NULL, TRUE);
        UpdateWindow(hWnd);
        return TRUE;
    }

    // Handle manual target send (either by IP or by device hashtag)
    if (wmId == IDC_BTN_SEND_MANUAL) {
        WORD* pDlgMem = (WORD*)malloc(1024); memset(pDlgMem, 0, 1024); LPDLGTEMPLATEA lpd = (LPDLGTEMPLATEA)pDlgMem; lpd->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER; lpd->cx = 175; lpd->cy = 72;
        char* textResult = (char*)DialogBoxIndirectParamA(GetModuleHandle(NULL), lpd, hWnd, ManualSendDialogProc, 0); free(pDlgMem);
        if (textResult) {
            char targetIp[64] = {0}; char manualLabel[64] = {0}; int targetPort = 53317; BOOL found = FALSE;
            strncpy(manualLabel, textResult, sizeof(manualLabel) - 1);
            // If the user typed a hashtag, search for a match in the resolved devices list view
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
                            strcpy(targetIp, dev->ipAddress); targetPort = dev->port; found = TRUE;
                            if (strlen(dev->alias) > 0) strncpy(manualLabel, dev->alias, sizeof(manualLabel) - 1);
                            break;
                        }
                    }
                }
                if (!found) { MessageBoxA(hWnd, g_Lang.msgHashtagNotFound, g_Lang.titleNotFound, MB_ICONERROR); free(textResult); return TRUE; }
            } else { strcpy(targetIp, textResult); found = TRUE; }
            free(textResult);

            // Start sending files in a background worker thread to keep the UI responsive
            if (found) {
                SendSessionContext* txCtx = (SendSessionContext*)malloc(sizeof(SendSessionContext));
                if (txCtx) {
                    memset(txCtx, 0, sizeof(SendSessionContext));
                    strncpy(txCtx->targets[0].ipAddress, targetIp, 15);
                    txCtx->targets[0].ipAddress[15] = '\0';
                    txCtx->targets[0].port = targetPort;
                    strncpy(txCtx->targets[0].alias, manualLabel, sizeof(txCtx->targets[0].alias) - 1);
                    txCtx->targets[0].isHttps = (g_EnableEncryption != 0);
                    txCtx->targetCount = 1;
                    txCtx->fileCount = g_sendQueueCount;
                    memcpy(txCtx->files, g_sendQueue, sizeof(FileToSend) * g_sendQueueCount);
                    ShowWindow(hWndSendProgress, SW_SHOW); SendMessage(hWndSendProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); SendMessage(hWndSendProgress, PBM_SETPOS, 0, 0);
                    char* copyMsg = (char*)malloc(256);
                    if (copyMsg) {
                        _snprintf(copyMsg, 256, g_Lang.msgConnectingTo, manualLabel);
                        PostMessage(hWnd, WM_SEND_STATUS_UPDATE, ICON_TRANSFERRING, (LPARAM)copyMsg);
                    }
                    DWORD thId = 0;
                    HANDLE hTxThread = CreateThread(NULL, 0, StartSendSessionThread, (LPVOID)txCtx, 0, &thId);
                    if (hTxThread) CloseHandle(hTxThread);
                }
            }
        }
        return TRUE;
    }

    // Refresh devices list
    if (wmId == IDC_BTN_SEARCH_AGAIN) {
        int devCount = ListView_GetItemCount(hWndListDevices);
        for (int i = 0; i < devCount; i++) {
            LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListDevices, &lvi);
            if (lvi.lParam) free((void*)lvi.lParam);
        }
        ListView_DeleteAllItems(hWndListDevices);
      
        ShowWindow(hWndSearchLoading, SW_SHOW); EnableWindow(hWndBtnSearchAgain, FALSE); 
        RECT rc; 
        GetClientRect(hWnd, &rc); 
        ResizeControls(hWnd, rc.right, rc.bottom); 
        if (g_mySocket != INVALID_SOCKET)
            sendDiscoveryShout(g_mySocket); 
        SetTimer(hWnd, 999, 3000, NULL); 
        UpdateSendButtonsState();
        return TRUE;
    }
    if (wmId == IDC_BTN_CLOSE_STATUS) { int totalItems = ListView_GetItemCount(hWndListView); for (int i = 0; i < totalItems; i++) { LVITEMA itemMod = {0}; itemMod.iItem = i; itemMod.mask = LVIF_GROUPID; ListView_GetItem(hWndListView, &itemMod); if (itemMod.iGroupId == 1) { itemMod.iGroupId = 2; ListView_SetItem(hWndListView, &itemMod); } } SetWindowTextA(hWndStatus, g_Lang.msgWaitingForFiles); UpdateStatusIcon(ICON_WAITING); ShowWindow(hWndBtnCloseStatus, SW_HIDE); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); return TRUE; }
    if (wmId == IDC_BTN_EDIT_DEVICE) { g_bEditingDeviceName = TRUE; SetWindowTextA(hWndEditDeviceBox, g_MyDeviceName); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); UpdatePageVisibility(); SetFocus(hWndEditDeviceBox); return TRUE; }
    if (wmId == IDC_BTN_SAVE_DEVICE) { GetWindowTextA(hWndEditDeviceBox, g_MyDeviceName, sizeof(g_MyDeviceName)); SetWindowTextA(hWndDeviceName, g_MyDeviceName); g_bEditingDeviceName = FALSE; RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); UpdatePageVisibility(); SaveSettings(); return TRUE; }
    if (wmId == IDC_BTN_CANCEL_DEVICE) { g_bEditingDeviceName = FALSE; RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); UpdatePageVisibility(); return TRUE; }

    if (wmId == IDC_SET_TOPMOST && wmEvent == BN_CLICKED) { LRESULT lChecked = SendMessage(hWndCheckTopmost, BM_GETCHECK, 0, 0); if (lChecked == BST_CHECKED) { SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); } else { SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); } SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_MIN_CLOSE && wmEvent == BN_CLICKED) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_SAVE_POS && wmEvent == BN_CLICKED) { SaveSettings(); return TRUE; }
    if (wmId == IDC_SET_RECURSIVE_FOLDER && wmEvent == BN_CLICKED) { SaveSettings(); return TRUE; }
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
            RecreateFonts();
            EnumChildWindows(hWnd, ApplyFontToChild, 0);
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
            if (DeleteFileA(filePath) || GetLastError() == ERROR_FILE_NOT_FOUND) { char fileToPurge[256]; strcpy(fileToPurge, transfer->fileName); int totalItems = ListView_GetItemCount(hWndListView); for (int i = totalItems - 1; i >= 0; i--) { LVITEMA itemLoop = {0}; itemLoop.iItem = i; itemLoop.mask = LVIF_PARAM; ListView_GetItem(hWndListView, &itemLoop); LoggedTransfer* tLoop = (LoggedTransfer*)itemLoop.lParam; if (tLoop && strcmp(tLoop->fileName, fileToPurge) == 0) { ListView_DeleteItem(hWndListView, i); } } } else { MessageBoxA(hWnd, g_Lang.msgUnableDelete, g_Lang.titleError, MB_ICONERROR); }
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
            InitTrayIcon(hWnd, GetModuleHandle(NULL));
            InitDropZoneWindow(GetModuleHandle(NULL), hWnd);
            SetTimer(hWnd, 997, 3000, NULL);
            break;
        }

        case WM_DROPFILES: {
            ProcessDroppedFiles((HDROP)wParam);
            if (!IsWindowVisible(hWnd)) {
                char dropMsg[128];
                _snprintf(dropMsg, sizeof(dropMsg), g_Lang.trayFilesQueued, g_sendQueueCount);
                ShowTrayNotification(hWnd, APP_NAME, dropMsg, NIIF_INFO);
            }
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
            if (msg) {
                UpdateSendStatusIcon(iconType);
                SetWindowTextA(hWndSendStatusTxt, msg);
                ShowWindow(hWndSendStatusIcon, SW_SHOW);
                ShowWindow(hWndSendStatusTxt, SW_SHOW);
                if (iconType == ICON_TRANSFERRING) {
                    ShowWindow(hWndBtnCancelSend, SW_SHOW);
                } else {
                    ShowWindow(hWndBtnCancelSend, SW_HIDE);
                }
                RECT rc; GetClientRect(hWnd, &rc);
                ResizeControls(hWnd, rc.right, rc.bottom);
                InvalidateRect(hWnd, NULL, TRUE);
                UpdateWindow(hWnd);
                free(msg);
            }
            break;
        }
        case WM_UPDATE_NET_INFO: {
            UpdateNetworkInfoText();
            break;
        }
        case WM_SEND_DONE: {
            BOOL isSuccess = (BOOL)wParam;
            ShowWindow(hWndSendProgress, SW_HIDE);
            ShowWindow(hWndBtnCancelSend, SW_HIDE);
            if (isSuccess) {
                UpdateSendStatusIcon(ICON_SUCCESS);
                SetWindowTextA(hWndSendStatusTxt, g_Lang.msgAllFilesTransferred);
                ShowWindow(hWndSendStatusIcon, SW_SHOW);
                ShowWindow(hWndSendStatusTxt, SW_SHOW);
                ShowTrayNotification(hWnd, APP_NAME, g_Lang.msgAllFilesTransferred, NIIF_INFO);
            } else {
                char errBuf[256] = {0};
                GetWindowTextA(hWndSendStatusTxt, errBuf, sizeof(errBuf));
                if (errBuf[0] == '\0') {
                    strncpy(errBuf, g_Lang.msgTransferCanceled, sizeof(errBuf) - 1);
                }
                ShowTrayNotification(hWnd, APP_NAME, errBuf, NIIF_ERROR);
            }
            SetTimer(hWnd, 998, 4000, NULL);
            RECT rc; GetClientRect(hWnd, &rc);
            ResizeControls(hWnd, rc.right, rc.bottom);
            InvalidateRect(hWnd, NULL, TRUE);
            UpdateWindow(hWnd);
            UpdateSendButtonsState();
            break;
        }
        case WM_TIMER: {
            if (wParam == 999) { KillTimer(hWnd, 999); ShowWindow(hWndSearchLoading, SW_HIDE); EnableWindow(hWndBtnSearchAgain, TRUE); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); }
            if (wParam == 998) {
                KillTimer(hWnd, 998);
                ShowWindow(hWndSendStatusIcon, SW_HIDE);
                ShowWindow(hWndSendStatusTxt, SW_HIDE);
                ShowWindow(hWndSendProgress, SW_HIDE);
                ShowWindow(hWndBtnCancelSend, SW_HIDE);
                InvalidateRect(hWnd, NULL, TRUE);
                UpdateWindow(hWnd);
            }
            if (wParam == 997) { UpdateNetworkInfoText(); }
            break;
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

        case WM_TRAYICON_MSG: {
            if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
                ShowTrayContextMenu(hWnd);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                if (IsWindowVisible(hWnd)) ShowWindow(hWnd, SW_HIDE);
                else { ShowWindow(hWnd, SW_SHOW); SetForegroundWindow(hWnd); }
            }
            break;
        }

        // File transfer starting
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

        // Live progress update from streaming TCP socket
        case WM_FILE_PROGRESS: {
            FileProgressInfo* info = (FileProgressInfo*)lParam; int count = ListView_GetItemCount(hWndListView);
            for(int i = 0; i < count; i++) {
                LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListView, &lvi); LoggedTransfer* transfer = (LoggedTransfer*)lvi.lParam;
                if (transfer && strcmp(transfer->fileId, info->fileId) == 0) { transfer->percentuale = info->pct; if (info->pct >= 100) { transfer->stato = STATE_COMPLETED; } RECT cellRect; ListView_GetSubItemRect(hWndListView, i, LV_COL_PROGRESS, LVIR_BOUNDS, &cellRect); InvalidateRect(hWndListView, &cellRect, FALSE); break; }
            } free(info); break;
        }

        // Cancellation event, network drop or aborted transfer
        case WM_FILE_CANCEL: {
            char* canceledId = (char*)lParam;
            if (canceledId) {
                int count = ListView_GetItemCount(hWndListView);
                for (int i = 0; i < count; i++) {
                    LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListView, &lvi); LoggedTransfer* transfer = (LoggedTransfer*)lvi.lParam;
                    if (transfer && strcmp(transfer->fileId, canceledId) == 0) { transfer->stato = STATE_CANCELED; ListView_Update(hWndListView, i); break; }
                }
                SetWindowTextA(hWndStatus, g_Lang.msgTransferCanceled); UpdateStatusIcon(ICON_CANCELED); ShowWindow(hWndBtnCloseStatus, SW_SHOW); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom);
                ShowTrayNotification(hWnd, APP_NAME, g_Lang.msgTransferCanceled, NIIF_ERROR);
                free(canceledId);
            } break;
        }

        // Peer Discovered Event via UDP multicast or HTTP registration
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
                const char* locType = GetLocalizedDeviceType(pDevice->deviceType);
                _snprintf(infoBuf, sizeof(infoBuf), "#%s (%s)", devHashtag, locType);
                LVITEMA lvi = {0}; lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM; lvi.iSubItem = 0; lvi.pszText = pDevice->alias; lvi.iImage = GetDeviceIconIndex(pDevice->deviceType); lvi.lParam = (LPARAM)pDevice;
                if (bAlreadyExists) { lvi.iItem = itemIndex; ListView_SetItem(hWndListDevices, &lvi); ListView_SetItemText(hWndListDevices, itemIndex, 1, infoBuf); } else { lvi.iItem = count; int insertedIdx = ListView_InsertItem(hWndListDevices, &lvi); ListView_SetItemText(hWndListDevices, insertedIdx, 1, infoBuf); }
            } UpdateSendButtonsState(); break;
        }

        // Prompts user with modal dialog to accept or reject incoming file transfer
        case WM_CONFIRM_TRANSFER: {
            ConfirmationRequest* req = (ConfirmationRequest*)lParam;
            if (req) {
                req->accepted = ShowTransferConfirmation(hWnd, req->senderName, req->fileCount, req->selectedPath);
                SetEvent(req->hEvent);
            }
            break;
        }

        // Prompts user for target PIN code if required by recipient
        case WM_REQUEST_PIN: {
            PinRequest* req = (PinRequest*)lParam;
            if (req) {
                req->success = ShowPinPrompt(hWnd, req->targetName, req->pinCode);
                SetEvent(req->hEvent);
            }
            break;
        }

        case WM_FILE_COMPLETE: { SetWindowTextA(hWndStatus, g_Lang.msgTransfersComplete); UpdateStatusIcon(ICON_SUCCESS); ShowWindow(hWndBtnCloseStatus, SW_SHOW); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); break; }
        case WM_CLOSE: { SaveSettings(); if (IsDlgButtonChecked(hWnd, IDC_SET_MIN_CLOSE)) ShowWindow(hWnd, SW_HIDE); else DestroyWindow(hWnd); break; }
        case WM_DESTROY: {
            SaveSettings();
            if (hNormalFont) DeleteObject(hNormalFont);
            if (hLargeFont) DeleteObject(hLargeFont);
            if (hStatusFont) DeleteObject(hStatusFont);
            if (hDeviceImageList) ImageList_Destroy(hDeviceImageList);
            HICON hCurrentIcon = (HICON)SendMessage(hWndStatusIcon, STM_GETICON, 0, 0); if (hCurrentIcon) DestroyIcon(hCurrentIcon);
            RemoveTrayIcon(hWnd);
            if (g_hWndDropZone) DestroyWindow(g_hWndDropZone);
            if (hWndListDevices) { int devCount = ListView_GetItemCount(hWndListDevices); for (int i = 0; i < devCount; i++) { LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListDevices, &lvi); if (lvi.lParam) free((void*)lvi.lParam); } }
            PostQuitMessage(0); break;
        }
        default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Fetches resource icons from ddores.dll or shell32.dll for corresponding device type
int GetDeviceIconIndex(const char* deviceType) {
    char sysPath[MAX_PATH];
    GetSystemDirectoryA(sysPath, MAX_PATH);
    strcat(sysPath, "\\ddores.dll");
    int iconIndex = 11; // default to mobile/phone icon
    if (deviceType) {
        if (_stricmp(deviceType, "Laptop") == 0 || _stricmp(deviceType, "desktop") == 0) iconIndex = 12;
        else if (_stricmp(deviceType, "Web") == 0) iconIndex = 48;
        else if (_stricmp(deviceType, "Terminal") == 0 || _stricmp(deviceType, "headless") == 0) iconIndex = 9;
        else if (_stricmp(deviceType, "Server") == 0) iconIndex = 13;
        else if (_stricmp(deviceType, "Phone") == 0 || _stricmp(deviceType, "mobile") == 0) iconIndex = 11;
    }
    HICON hIcon = NULL;
    if (SafeExtractIcon(sysPath, iconIndex, 32, 32, &hIcon) && hIcon != NULL) {
        int idx = ImageList_AddIcon(hDeviceImageList, hIcon);
        DestroyIcon(hIcon);
        return idx;
    }
    // Fallback to shell32.dll for Windows NT 4.0 / 2000 / XP
    GetSystemDirectoryA(sysPath, MAX_PATH);
    strcat(sysPath, "\\shell32.dll");
    int fallbackIndex = 15;
    if (deviceType) {
        if (_stricmp(deviceType, "Laptop") == 0 || _stricmp(deviceType, "desktop") == 0) fallbackIndex = 15;
        else if (_stricmp(deviceType, "Server") == 0) fallbackIndex = 17;
        else if (_stricmp(deviceType, "Phone") == 0 || _stricmp(deviceType, "mobile") == 0) fallbackIndex = 18;
        else fallbackIndex = 15;
    }
    if (SafeExtractIcon(sysPath, fallbackIndex, 32, 32, &hIcon) && hIcon != NULL) {
        int idx = ImageList_AddIcon(hDeviceImageList, hIcon);
        DestroyIcon(hIcon);
        return idx;
    }
    return -1;
}

// Adds a file to queue list and prepares metadata for sending
void AddFileToSendQueue(const char* filePath, const char* fileName, long long fileSize) {
    if (g_sendQueueCount >= MAX_SEND_FILES) { MessageBoxA(g_hWndMain, g_Lang.msgMaxFiles, g_Lang.titleWarning, MB_ICONWARNING); return; }
    for (int i = 0; i < g_sendQueueCount; i++) { if (strcmp(g_sendQueue[i].filePath, filePath) == 0) return; }
    strncpy(g_sendQueue[g_sendQueueCount].filePath, filePath, 511); strncpy(g_sendQueue[g_sendQueueCount].fileName, fileName, 255);
    g_sendQueue[g_sendQueueCount].fileSize = fileSize; sprintf(g_sendQueue[g_sendQueueCount].fileId, "file-%ld-%d", GetTickCount(), g_sendQueueCount);
    char sizeStr[32]; if (fileSize < 1024) sprintf(sizeStr, "%lld B", fileSize); else if (fileSize < 1024 * 1024) sprintf(sizeStr, "%.1f KB", fileSize / 1024.0); else sprintf(sizeStr, "%.1f MB", fileSize / (1024.0 * 1024.0));
    LVITEMA lvi = {0}; lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM; lvi.iItem = ListView_GetItemCount(hWndListSendFiles);
    lvi.iSubItem = 0; lvi.pszText = (char*)fileName; lvi.iImage = GetSystemIconIndex(fileName); lvi.lParam = (LPARAM)g_sendQueueCount;
    int idx = ListView_InsertItem(hWndListSendFiles, &lvi); ListView_SetItemText(hWndListSendFiles, idx, 1, sizeStr);
    g_sendQueueCount++; UpdateSendButtonsState();
}

// Clears all queued files from the send list
void ClearSendQueue() {
    if (hWndListSendFiles) {
        ListView_DeleteAllItems(hWndListSendFiles);
    }
    g_sendQueueCount = 0;
    memset(g_sendQueue, 0, sizeof(g_sendQueue));
    UpdateSendButtonsState();
}

// Application Entry Point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitCommonControls();
    HMODULE hComCtl = GetModuleHandleA("comctl32.dll");
    if (hComCtl) {
        typedef BOOL (WINAPI *PFN_InitCommonControlsEx)(const INITCOMMONCONTROLSEX*);
        PFN_InitCommonControlsEx pfnInitEx = (PFN_InitCommonControlsEx)GetProcAddress(hComCtl, "InitCommonControlsEx");
        if (pfnInitEx) {
            INITCOMMONCONTROLSEX icex;
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
            pfnInitEx(&icex);
        }
    }
    InitSettingsPath();
    InitLanguage();
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
#if !defined(__arm__) && !defined(LOCALSEND_NT)
    HMODULE hCrypto = LoadLibraryA("libcrypto-3.dll");
    HMODULE hSsl = LoadLibraryA("libssl-3.dll");
    if (!hCrypto || !hSsl) {
        const char* errMsg = "";
        if (!hCrypto && !hSsl) errMsg = g_Lang.errBothMissing;
        else if (!hCrypto) errMsg = g_Lang.errCryptoMissing;
        else errMsg = g_Lang.errSslMissing;
        MessageBoxA(NULL, errMsg, g_Lang.titleError, MB_ICONERROR);
        if (hCrypto) FreeLibrary(hCrypto);
        if (hSsl) FreeLibrary(hSsl);
        return 1;
    }
    FreeLibrary(hCrypto);
    FreeLibrary(hSsl);
#endif
#ifndef LOCALSEND_NT
    if (!checkRulesExistence()) {
        MessageBoxA(NULL, g_Lang.msgFirewallWarning, g_Lang.titleFirewallWarning, MB_ICONWARNING | MB_OK);
    }
    autoFirewall();
#endif
    if (!initWinsock()) return 1;
    if (!TlsInitGlobal()) {
        g_EnableEncryption = 0;
        GenerateFallbackFingerprint();
    }
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
    DragAcceptFiles(g_hWndMain, TRUE);
 
    // Initialize COM libraries for the folder selection dialog
    CoInitialize(NULL);

    ShowWindow(g_hWndMain, SW_SHOW);
    UpdateWindow(g_hWndMain);

    g_mySocket = createUdpSocket();
    printf("[Main] createUdpSocket returned: %d\n", (int)g_mySocket);
    fflush(stdout);
    if (g_mySocket != INVALID_SOCKET) {
        joinMulticastGroup(g_mySocket);
        DWORD thIdUdp = 0;
        HANDLE hUdpThread = CreateThread(NULL, 0, startListeningLoop, (LPVOID)g_mySocket, 0, &thIdUdp);
        printf("[Main] CreateThread UDP listener: handle=%p, thId=%lu, err=%lu\n", hUdpThread, thIdUdp, GetLastError());
        fflush(stdout);
        if (hUdpThread) CloseHandle(hUdpThread);
    }
    DWORD thIdTcp = 0;
    HANDLE hTcpThread = CreateThread(NULL, 0, tcpServerThread, NULL, 0, &thIdTcp);
    printf("[Main] CreateThread TCP server: handle=%p, thId=%lu, err=%lu\n", hTcpThread, thIdTcp, GetLastError());
    fflush(stdout);
    if (hTcpThread) CloseHandle(hTcpThread);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    if (g_mySocket != INVALID_SOCKET) {
        closesocket(g_mySocket);
    }
    WSACleanup();
    TlsCleanupGlobal();
    CoUninitialize();
    return 0;
}


