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

#ifdef __arm__
    #define APP_NAME "LocalSend RT"
    #define APP_ABOUT "LocalSend client built with C/Win32 APIs. Made for Windows RT."
    #define APP_SUPPORT_BTN "About LocalSend RT"
    #define APP_SUPPORT_URL "https://github.com/fraaaaa4/LocalSend32"
#else
    #define APP_NAME "LocalSend32"
    #define APP_ABOUT "LocalSend client built with C/Win32 APIs. Made for 32-bit Windows."
    #define APP_SUPPORT_BTN "About LocalSend32"
    #define APP_SUPPORT_URL "https://github.com/fraaaaa4/LocalSend32"
#endif

#define IDC_TAB_CONTROL       3001
#define IDC_SETTINGS_TAB      3002
#define IDC_STATUS_TEXT       2002
#define IDC_GROUP_BOX         2003
#define IDC_INFO_TEXT         2004
#define IDC_FILE_LISTVIEW     2005
#define IDC_BTN_EDIT_DEVICE   2006
#define IDC_EDIT_DEVICE_NAME  2007
#define IDC_BTN_SAVE_DEVICE   2008
#define IDC_BTN_CANCEL_DEVICE 2009
#define IDC_BTN_CLOSE_STATUS  2010
#define IDC_BTN_SEND_FILE     4001
#define IDC_BTN_SEND_FOLDER   4002
#define IDC_BTN_SEND_TEXT     4003
#define IDC_BTN_SEND_PASTE    4004
#define IDC_LIST_SEND_FILES   4005
#define IDC_BTN_CLEAN_FILES   4006
#define IDC_BTN_SEND_FILES    4007
#define IDC_LIST_DEVICES      4008
#define IDC_BTN_SEARCH_AGAIN  4009
#define IDC_BTN_SEND_MANUAL   4010
#define IDC_SEND_PROGRESS     4011
#define IDC_SEND_STATUS_TXT   4012
#define IDC_SEND_STATUS_ICON  4013

#define IDC_SET_SAVE_POS      5001
#define IDC_SET_MIN_CLOSE     5002
#define IDC_SET_TOPMOST       5003
#define IDC_SET_BTN_GH        5004
#define IDC_SET_BTN_SUPP      5005
#define IDC_RECV_LBL          5006
#define IDC_RECV_RAD_APP      5007
#define IDC_RECV_RAD_DL       5008
#define IDC_RECV_RAD_CUSTOM   5009
#define IDC_RECV_TXT_PATH     5010
#define IDC_RECV_BTN_BROWSE   5011
#define IDC_SET_QUICK_SAVE    5012
#define IDC_SET_REQ_PIN       5013
#define IDC_SET_PIN_CODE      5014
#define IDC_SET_DISC_TIMEOUT  5015
#define IDC_SET_MULTICAST     5016
#define IDC_SET_ENCRYPTION    5017
#define IDC_SET_DEV_TYPE      5018
#define IDC_SET_DEV_MODEL     5019
#define IDC_SET_PORT          5020

#define IDM_OPEN_FILE         6001
#define IDM_COPY_FILE         6002
#define IDM_DELETE_FILE       6003
#define IDM_PROP_FILE         6004

#define MAX_SEND_FILES        50
#define IDC_TEXT_DLG_EDIT     7001
#define IDC_MANUAL_DLG_EDIT   7002
#define IDC_MANUAL_RAD_IP     7003
#define IDC_MANUAL_RAD_HASH   7004

#ifndef CDDS_SUBITEMPREPAINT
#define CDDS_SUBITEMPREPAINT (CDDS_ITEM | CDDS_SUBITEM | CDDS_PREPAINT)
#endif

__declspec(dllimport) UINT WINAPI PrivateExtractIconsA(LPCSTR szFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT *piconid, UINT nIcons, UINT flags);

typedef enum { ICON_WAITING = 0, ICON_TRANSFERRING = 1, ICON_SUCCESS = 2, ICON_CANCELED = 3 } StatusIconType;

void ApplyWindowFont(HWND hWndChild);
void ApplyLargeFont(HWND hWndChild);
void ApplyStatusFont(HWND hWndChild);
void UpdateStatusIcon(StatusIconType type);
void UpdateSendStatusIcon(StatusIconType type);
void AddTooltip(HWND hCtrl, char* text);
void GetDeviceNetworkInfo(char* outBuffer, size_t maxLen);
int GetSystemIconIndex(const char* fileName);
void ShowSettingsSubPage(int subTab);
void UpdatePageVisibility(void);
void ResizeControls(HWND hWnd, int width, int height);
int GetDeviceIconIndex(const char* deviceType);
void AddFileToSendQueue(const char* filePath, const char* fileName, long long fileSize);
void UpdateSendButtonsState(void);

void InitSettingsPath();
void LoadSettings();
void SaveSettings();

INT_PTR CALLBACK TextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ManualSendDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void sendDiscoveryShout(SOCKET mySocket);
static bool ShowTransferConfirmation(HWND parent, const char* senderName, int fileCount, char* outSavePath);
static bool ShowPinPrompt(HWND parent, const char* targetName, char* outPin);

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

void InitSettingsPath() {
    GetModuleFileNameA(NULL, g_IniPath, MAX_PATH);
    char* p = strrchr(g_IniPath, '\\'); if (p) *p = '\0';
#ifdef __arm__
    strcat(g_IniPath, "\\localsend_rt.ini");
#else
    strcat(g_IniPath, "\\localsend32.ini");
#endif
}

void LoadSettings() {
    InitSettingsPath();
    g_SaveMode = GetPrivateProfileIntA("Settings", "SaveMode", 1, g_IniPath);
    g_QuickSave = GetPrivateProfileIntA("Settings", "QuickSave", 0, g_IniPath);
    GetPrivateProfileStringA("Settings", "CustomPath", "", g_CustomPath, MAX_PATH, g_IniPath);

    int topmost = GetPrivateProfileIntA("Settings", "TopMost", 0, g_IniPath);
    int minClose = GetPrivateProfileIntA("Settings", "MinClose", 0, g_IniPath);
    int savePos = GetPrivateProfileIntA("Settings", "SavePos", 0, g_IniPath);

    SendMessage(hWndCheckTopmost, BM_SETCHECK, topmost ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hWndCheckMinClose, BM_SETCHECK, minClose ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hWndCheckSavePos, BM_SETCHECK, savePos ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hWndCheckQuickSave, BM_SETCHECK, g_QuickSave ? BST_CHECKED : BST_UNCHECKED, 0);

    g_RequirePin = GetPrivateProfileIntA("Settings", "RequirePin", 0, g_IniPath);
    GetPrivateProfileStringA("Settings", "PinCode", "1234", g_PinCode, sizeof(g_PinCode), g_IniPath);
    g_DiscoveryTimeout = GetPrivateProfileIntA("Settings", "DiscoveryTimeout", 5, g_IniPath);
    GetPrivateProfileStringA("Settings", "MulticastAddr", "224.0.0.167", g_MulticastAddr, sizeof(g_MulticastAddr), g_IniPath);
    g_EnableEncryption = GetPrivateProfileIntA("Settings", "EnableEncryption", 1, g_IniPath);
    GetPrivateProfileStringA("Settings", "DeviceType", "Laptop", g_DeviceType, sizeof(g_DeviceType), g_IniPath);
#ifdef __arm__
    GetPrivateProfileStringA("Settings", "DeviceModel", "Surface RT", g_DeviceModel, sizeof(g_DeviceModel), g_IniPath);
#else
    GetPrivateProfileStringA("Settings", "DeviceModel", "PC Desktop", g_DeviceModel, sizeof(g_DeviceModel), g_IniPath);
#endif
    g_Port = GetPrivateProfileIntA("Settings", "Port", 53317, g_IniPath);
    
    char defaultName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD dwSize = sizeof(defaultName);
    if (!GetComputerNameA(defaultName, &dwSize)) strcpy(defaultName, "Unknown-Device");
    GetPrivateProfileStringA("Settings", "DeviceName", defaultName, g_MyDeviceName, sizeof(g_MyDeviceName), g_IniPath);

    int fj = 0;
    for (int i = 0; g_MyDeviceName[i] && fj < 63; i++) {
        if (g_MyDeviceName[i] != ' ') g_MyFingerprint[fj++] = tolower(g_MyDeviceName[i]);
    }
    g_MyFingerprint[fj] = '\0';
    if (hWndDeviceName) {
        SetWindowTextA(hWndDeviceName, g_MyDeviceName);
    }

    SendMessage(hWndCheckReqPin, BM_SETCHECK, g_RequirePin ? BST_CHECKED : BST_UNCHECKED, 0);
    SetWindowTextA(hWndEditPinCode, g_PinCode);
    EnableWindow(hWndEditPinCode, g_RequirePin);
    char tmp[64];
    sprintf(tmp, "%d", g_DiscoveryTimeout); SetWindowTextA(hWndEditDiscTimeout, tmp);
    SetWindowTextA(hWndEditMulticast, g_MulticastAddr);
    SendMessage(hWndCheckEncryption, BM_SETCHECK, g_EnableEncryption ? BST_CHECKED : BST_UNCHECKED, 0);

    SendMessage(hWndComboDevType, CB_RESETCONTENT, 0, 0);
    SendMessage(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)"Phone");
    SendMessage(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)"Laptop");
    SendMessage(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)"Web");
    SendMessage(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)"Terminal");
    SendMessage(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)"Server");
    int selIdx = 0;
    if (_stricmp(g_DeviceType, "Laptop") == 0) selIdx = 1;
    else if (_stricmp(g_DeviceType, "Web") == 0) selIdx = 2;
    else if (_stricmp(g_DeviceType, "Terminal") == 0) selIdx = 3;
    else if (_stricmp(g_DeviceType, "Server") == 0) selIdx = 4;
    SendMessage(hWndComboDevType, CB_SETCURSEL, selIdx, 0);

    SetWindowTextA(hWndEditDevModel, g_DeviceModel);
    sprintf(tmp, "%d", g_Port); SetWindowTextA(hWndEditPort, tmp);

    SendMessage(hWndRecvRadApp, BM_SETCHECK, (g_SaveMode == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hWndRecvRadDl, BM_SETCHECK, (g_SaveMode == 1) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hWndRecvRadCustom, BM_SETCHECK, (g_SaveMode == 2) ? BST_CHECKED : BST_UNCHECKED, 0);
    SetWindowTextA(hWndRecvTxtPath, g_CustomPath);

    BOOL isCustom = (g_SaveMode == 2);
    EnableWindow(hWndRecvTxtPath, isCustom); EnableWindow(hWndRecvBtnBrowse, isCustom);

    if (savePos) {
        int x = GetPrivateProfileIntA("Settings", "WinX", CW_USEDEFAULT, g_IniPath);
        int y = GetPrivateProfileIntA("Settings", "WinY", CW_USEDEFAULT, g_IniPath);
        int w = GetPrivateProfileIntA("Settings", "WinW", 800, g_IniPath);
        int h = GetPrivateProfileIntA("Settings", "WinH", 420, g_IniPath);
        if (x != CW_USEDEFAULT) {
            SetWindowPos(g_hWndMain, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, x, y, w, h, 0);
        } else {
            SetWindowPos(g_hWndMain, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
    } else {
        SetWindowPos(g_hWndMain, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    // If INI file doesn't exist, create it with default settings
    DWORD attrib = GetFileAttributesA(g_IniPath);
    if (attrib == INVALID_FILE_ATTRIBUTES) {
        SaveSettings();
    }
}

void SaveSettings() {
    if (!g_IniPath[0]) InitSettingsPath();
    char buf[32];

    if (hWndCheckQuickSave != NULL) {
        g_QuickSave = (SendMessage(hWndCheckQuickSave, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
    sprintf(buf, "%d", g_SaveMode); WritePrivateProfileStringA("Settings", "SaveMode", buf, g_IniPath);
    sprintf(buf, "%d", g_QuickSave); WritePrivateProfileStringA("Settings", "QuickSave", buf, g_IniPath);
    WritePrivateProfileStringA("Settings", "CustomPath", g_CustomPath, g_IniPath);

    if (hWndCheckReqPin != NULL) {
        g_RequirePin = (SendMessage(hWndCheckReqPin, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
    if (hWndEditPinCode != NULL) {
        EnableWindow(hWndEditPinCode, g_RequirePin);
        GetWindowTextA(hWndEditPinCode, g_PinCode, sizeof(g_PinCode));
    }
    char tmp[64];
    if (hWndEditDiscTimeout != NULL) {
        GetWindowTextA(hWndEditDiscTimeout, tmp, sizeof(tmp)); g_DiscoveryTimeout = atoi(tmp);
    }
    if (hWndEditMulticast != NULL) {
        GetWindowTextA(hWndEditMulticast, g_MulticastAddr, sizeof(g_MulticastAddr));
    }
    if (hWndCheckEncryption != NULL) {
        g_EnableEncryption = (SendMessage(hWndCheckEncryption, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }

    if (hWndComboDevType != NULL) {
        int typeIdx = SendMessage(hWndComboDevType, CB_GETCURSEL, 0, 0);
        if (typeIdx == 1) strcpy(g_DeviceType, "Laptop");
        else if (typeIdx == 2) strcpy(g_DeviceType, "Web");
        else if (typeIdx == 3) strcpy(g_DeviceType, "Terminal");
        else if (typeIdx == 4) strcpy(g_DeviceType, "Server");
        else strcpy(g_DeviceType, "Phone");
    }

    if (hWndEditDevModel != NULL) {
        GetWindowTextA(hWndEditDevModel, g_DeviceModel, sizeof(g_DeviceModel));
    }
    if (hWndEditPort != NULL) {
        GetWindowTextA(hWndEditPort, tmp, sizeof(tmp)); g_Port = atoi(tmp);
    }

    int fj = 0;
    for (int i = 0; g_MyDeviceName[i] && fj < 63; i++) {
        if (g_MyDeviceName[i] != ' ') g_MyFingerprint[fj++] = tolower(g_MyDeviceName[i]);
    }
    g_MyFingerprint[fj] = '\0';

    WritePrivateProfileStringA("Settings", "DeviceName", g_MyDeviceName, g_IniPath);
    sprintf(buf, "%d", g_RequirePin); WritePrivateProfileStringA("Settings", "RequirePin", buf, g_IniPath);
    WritePrivateProfileStringA("Settings", "PinCode", g_PinCode, g_IniPath);
    sprintf(buf, "%d", g_DiscoveryTimeout); WritePrivateProfileStringA("Settings", "DiscoveryTimeout", buf, g_IniPath);
    WritePrivateProfileStringA("Settings", "MulticastAddr", g_MulticastAddr, g_IniPath);
    sprintf(buf, "%d", g_EnableEncryption); WritePrivateProfileStringA("Settings", "EnableEncryption", buf, g_IniPath);
    WritePrivateProfileStringA("Settings", "DeviceType", g_DeviceType, g_IniPath);
    WritePrivateProfileStringA("Settings", "DeviceModel", g_DeviceModel, g_IniPath);
    sprintf(buf, "%d", g_Port); WritePrivateProfileStringA("Settings", "Port", buf, g_IniPath);

    int topmost = 0;
    int minClose = 0;
    int savePos = 0;
    if (hWndCheckTopmost != NULL) topmost = (SendMessage(hWndCheckTopmost, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (hWndCheckMinClose != NULL) minClose = (SendMessage(hWndCheckMinClose, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (hWndCheckSavePos != NULL) savePos = (SendMessage(hWndCheckSavePos, BM_GETCHECK, 0, 0) == BST_CHECKED);

    sprintf(buf, "%d", topmost); WritePrivateProfileStringA("Settings", "TopMost", buf, g_IniPath);
    sprintf(buf, "%d", minClose); WritePrivateProfileStringA("Settings", "MinClose", buf, g_IniPath);
    sprintf(buf, "%d", savePos); WritePrivateProfileStringA("Settings", "SavePos", buf, g_IniPath);

    if (savePos) {
        RECT rc; GetWindowRect(g_hWndMain, &rc);
        sprintf(buf, "%d", (int)rc.left); WritePrivateProfileStringA("Settings", "WinX", buf, g_IniPath);
        sprintf(buf, "%d", (int)rc.top); WritePrivateProfileStringA("Settings", "WinY", buf, g_IniPath);
        sprintf(buf, "%d", (int)(rc.right - rc.left)); WritePrivateProfileStringA("Settings", "WinW", buf, g_IniPath);
        sprintf(buf, "%d", (int)(rc.bottom - rc.top)); WritePrivateProfileStringA("Settings", "WinH", buf, g_IniPath);
    }
}

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

void UpdateStatusIcon(StatusIconType type) {
    char sysPath[MAX_PATH]; GetSystemDirectoryA(sysPath, MAX_PATH); strcat(sysPath, "\\shell32.dll");
    int iconIndex = (type == ICON_WAITING) ? 54 : (type == ICON_TRANSFERRING) ? 89 : (type == ICON_SUCCESS) ? 301 : 131;
    HICON hIcon = NULL; UINT iconId = 0;
    if (PrivateExtractIconsA(sysPath, iconIndex, 16, 16, &hIcon, &iconId, 1, 0) > 0 && hIcon != NULL) {
        HICON hOldIcon = (HICON)SendMessage(hWndStatusIcon, STM_SETICON, (WPARAM)hIcon, 0); if (hOldIcon) DestroyIcon(hOldIcon);
    }
}

void UpdateSendStatusIcon(StatusIconType type) {
    char sysPath[MAX_PATH]; GetSystemDirectoryA(sysPath, MAX_PATH); strcat(sysPath, "\\shell32.dll");
    int iconIndex = (type == ICON_WAITING) ? 54 : (type == ICON_TRANSFERRING) ? 89 : (type == ICON_SUCCESS) ? 301 : 131;
    HICON hIcon = NULL; UINT iconId = 0;
    if (PrivateExtractIconsA(sysPath, iconIndex, 16, 16, &hIcon, &iconId, 1, 0) > 0 && hIcon != NULL) {
        HICON hOldIcon = (HICON)SendMessage(hWndSendStatusIcon, STM_SETICON, (WPARAM)hIcon, 0); if (hOldIcon) DestroyIcon(hOldIcon);
    }
}

void AddTooltip(HWND hCtrl, char* text) {
    if (!hWndToolTip) {
        hWndToolTip = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, g_hWndMain, NULL, GetModuleHandle(NULL), NULL);
        SetWindowPos(hWndToolTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    TOOLINFOA ti = {0}; ti.cbSize = sizeof(TOOLINFOA); ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND; ti.hwnd = g_hWndMain; ti.uId = (UINT_PTR)hCtrl; ti.lpszText = text;
    SendMessage(hWndToolTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
}

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

int GetSystemIconIndex(const char* fileName) {
    SHFILEINFOA sfi = {0};
    HIMAGELIST hInstIL = (HIMAGELIST)SHGetFileInfoA(fileName, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    if (hSystemImageList == NULL && hInstIL != NULL) { hSystemImageList = hInstIL; ListView_SetImageList(hWndListView, hSystemImageList, LVSIL_SMALL); ListView_SetImageList(hWndListSendFiles, hSystemImageList, LVSIL_SMALL); }
    return sfi.iIcon;
}

void ShowSettingsSubPage(int subTab) {
    int hide = SW_HIDE;
    ShowWindow(hWndCheckSavePos, hide); ShowWindow(hWndCheckMinClose, hide); ShowWindow(hWndCheckTopmost, hide);
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

void UpdateSendButtonsState() {
    BOOL hasFiles = (g_sendQueueCount > 0);
    int selectedDevice = ListView_GetNextItem(hWndListDevices, -1, LVNI_SELECTED);
    EnableWindow(hWndBtnCleanFiles, hasFiles); EnableWindow(hWndBtnSendFiles, hasFiles && (selectedDevice != -1));
}

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
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            hWndTab = CreateWindowExA(0, WC_TABCONTROLA, "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, 0, 0, hWnd, (HMENU)IDC_TAB_CONTROL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndTab);
            TCITEMA tie; tie.mask = TCIF_TEXT;
            tie.pszText = "Receive"; TabCtrl_InsertItem(hWndTab, 0, &tie); tie.pszText = "Send"; TabCtrl_InsertItem(hWndTab, 1, &tie); tie.pszText = "Settings"; TabCtrl_InsertItem(hWndTab, 2, &tie);

            hWndDeviceNameTitle = CreateWindowExA(0, "STATIC", "Device name", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndDeviceNameTitle);
            hWndDeviceName = CreateWindowExA(0, "STATIC", g_MyDeviceName, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyLargeFont(hWndDeviceName);
            hWndBtnEditDevice = CreateWindowExA(0, "BUTTON", "Edit", WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_EDIT_DEVICE, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndBtnEditDevice);
            hWndEditDeviceBox = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_DEVICE_NAME, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndEditDeviceBox); SendMessage(hWndEditDeviceBox, EM_SETLIMITTEXT, MAX_COMPUTERNAME_LENGTH, 0);
            hWndBtnSaveDevice = CreateWindowExA(0, "BUTTON", "Save", WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_SAVE_DEVICE, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndBtnSaveDevice);
            hWndBtnCancelDevice = CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_CANCEL_DEVICE, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndBtnCancelDevice);

            hWndGroupBox = CreateWindowExA(0, "BUTTON", "Network information", WS_CHILD | BS_GROUPBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_GROUP_BOX, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndGroupBox);
            char infoNet[512] = {0}; GetDeviceNetworkInfo(infoNet, sizeof(infoNet));
            hWndInfoText = CreateWindowExA(0, "STATIC", infoNet, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, (HMENU)IDC_INFO_TEXT, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndInfoText);

            hWndStatusIcon = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_ICON, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL);
            hWndStatus = CreateWindowExA(0, "STATIC", "Waiting for files...", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, (HMENU)IDC_STATUS_TEXT, GetModuleHandle(NULL), NULL); ApplyStatusFont(hWndStatus); UpdateStatusIcon(ICON_WAITING);
            hWndBtnCloseStatus = CreateWindowExA(0, "BUTTON", "Close", WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_CLOSE_STATUS, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndBtnCloseStatus); ShowWindow(hWndBtnCloseStatus, SW_HIDE);

            hWndListView = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 0, 0, 0, 0, hWnd, (HMENU)IDC_FILE_LISTVIEW, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndListView); ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            LVCOLUMNA lvc; lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; lvc.iSubItem = LV_COL_NAME; lvc.cx = 125; lvc.pszText = "File"; ListView_InsertColumn(hWndListView, LV_COL_NAME, &lvc); lvc.iSubItem = LV_COL_SIZE; lvc.cx = 75; lvc.pszText = "Size"; ListView_InsertColumn(hWndListView, LV_COL_SIZE, &lvc); lvc.iSubItem = LV_COL_DATE; lvc.cx = 125; lvc.pszText = "Date"; ListView_InsertColumn(hWndListView, LV_COL_DATE, &lvc); lvc.iSubItem = LV_COL_PROGRESS; lvc.cx = 125; lvc.pszText = "Progress"; ListView_InsertColumn(hWndListView, LV_COL_PROGRESS, &lvc);

            ListView_EnableGroupView(hWndListView, TRUE); LVGROUP lvg = {0}; lvg.cbSize = sizeof(LVGROUP); lvg.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE; lvg.state = LVGS_COLLAPSIBLE; wchar_t wToday[] = L"Today"; wchar_t wOlder[] = L"Older Transfers"; lvg.pszHeader = wToday; lvg.iGroupId = 1; ListView_InsertGroup(hWndListView, -1, &lvg); lvg.pszHeader = wOlder; lvg.iGroupId = 2; ListView_InsertGroup(hWndListView, -1, &lvg);

            hWndBtnSndFile = CreateWindowExA(0, "BUTTON", "File", WS_CHILD | BS_PUSHBUTTON | BS_ICON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_FILE, GetModuleHandle(NULL), NULL);
            hWndBtnSndFolder = CreateWindowExA(0, "BUTTON", "Folder", WS_CHILD | BS_PUSHBUTTON | BS_ICON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_FOLDER, GetModuleHandle(NULL), NULL);
            hWndBtnSndText = CreateWindowExA(0, "BUTTON", "Text", WS_CHILD | BS_PUSHBUTTON | BS_ICON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_TEXT, GetModuleHandle(NULL), NULL);
            hWndBtnSndPaste = CreateWindowExA(0, "BUTTON", "Paste", WS_CHILD | BS_PUSHBUTTON | BS_ICON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_PASTE, GetModuleHandle(NULL), NULL);

            char sysPath[MAX_PATH]; GetSystemDirectoryA(sysPath, MAX_PATH); strcat(sysPath, "\\shell32.dll"); HICON hIFile, hIFolder, hIText, hIPaste; UINT dum; PrivateExtractIconsA(sysPath, 54, 24, 24, &hIFile, &dum, 1, 0); PrivateExtractIconsA(sysPath, 3, 24, 24, &hIFolder, &dum, 1, 0); PrivateExtractIconsA(sysPath, 70, 24, 24, &hIText, &dum, 1, 0); PrivateExtractIconsA(sysPath, 260, 24, 24, &hIPaste, &dum, 1, 0);
            SendMessage(hWndBtnSndFile, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIFile); SendMessage(hWndBtnSndFolder, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIFolder); SendMessage(hWndBtnSndText, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIText); SendMessage(hWndBtnSndPaste, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIPaste);
            AddTooltip(hWndBtnSndFile, "Add files"); AddTooltip(hWndBtnSndFolder, "Add folder"); AddTooltip(hWndBtnSndText, "Send text message"); AddTooltip(hWndBtnSndPaste, "Paste from clipboard");

            hWndLblSendFiles = CreateWindowExA(0, "STATIC", "Files to send:", WS_CHILD | SS_LEFT, 0,0,0,0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyStatusFont(hWndLblSendFiles);
            hWndListSendFiles = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 0,0,0,0, hWnd, (HMENU)IDC_LIST_SEND_FILES, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndListSendFiles); ListView_SetExtendedListViewStyle(hWndListSendFiles, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; lvc.iSubItem = 0; lvc.cx = 200; lvc.pszText = "File"; ListView_InsertColumn(hWndListSendFiles, 0, &lvc); lvc.iSubItem = 1; lvc.cx = 80; lvc.pszText = "Size"; ListView_InsertColumn(hWndListSendFiles, 1, &lvc);

            hWndSendProgress = CreateWindowExA(0, PROGRESS_CLASSA, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, (HMENU)IDC_SEND_PROGRESS, GetModuleHandle(NULL), NULL);
            hWndSendStatusIcon = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_ICON, 0, 0, 0, 0, hWnd, (HMENU)IDC_SEND_STATUS_ICON, GetModuleHandle(NULL), NULL);
            hWndSendStatusTxt = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, (HMENU)IDC_SEND_STATUS_TXT, GetModuleHandle(NULL), NULL); ApplyStatusFont(hWndSendStatusTxt);
            ShowWindow(hWndSendProgress, SW_HIDE); ShowWindow(hWndSendStatusIcon, SW_HIDE); ShowWindow(hWndSendStatusTxt, SW_HIDE);

            hWndBtnCleanFiles = CreateWindowExA(0, "BUTTON", "Clean file list", WS_CHILD | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_CLEAN_FILES, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndBtnCleanFiles);
            hWndBtnSendFiles  = CreateWindowExA(0, "BUTTON", "Send files", WS_CHILD | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_FILES, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndBtnSendFiles);

            hWndLblDevices = CreateWindowExA(0, "STATIC", "Nearby devices:", WS_CHILD | SS_LEFT, 0,0,0,0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyStatusFont(hWndLblDevices);
            hWndListDevices = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 0,0,0,0, hWnd, (HMENU)IDC_LIST_DEVICES, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndListDevices); ListView_SetExtendedListViewStyle(hWndListDevices, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            hDeviceImageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 0, 1); ListView_SetImageList(hWndListDevices, hDeviceImageList, LVSIL_SMALL);
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; lvc.iSubItem = 0; lvc.cx = 180; lvc.pszText = "Device"; ListView_InsertColumn(hWndListDevices, 0, &lvc); lvc.iSubItem = 1; lvc.cx = 150; lvc.pszText = "Info"; ListView_InsertColumn(hWndListDevices, 1, &lvc);

            hWndBtnSearchAgain = CreateWindowExA(0, "BUTTON", "Search again", WS_CHILD | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEARCH_AGAIN, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndBtnSearchAgain);
            hWndSearchLoading = CreateWindowExA(0, PROGRESS_CLASSA, NULL, WS_CHILD | PBS_MARQUEE, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); SendMessage(hWndSearchLoading, PBM_SETMARQUEE, TRUE, 30); ShowWindow(hWndSearchLoading, SW_HIDE);
            hWndBtnSendManual = CreateWindowExA(0, "BUTTON", "Send manually", WS_CHILD | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_MANUAL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndBtnSendManual);

            hWndSettingsTab = CreateWindowExA(0, WC_TABCONTROLA, "", WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 0, 0, hWndTab, (HMENU)IDC_SETTINGS_TAB, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndSettingsTab);
            tie.pszText = "General"; TabCtrl_InsertItem(hWndSettingsTab, 0, &tie); tie.pszText = "Receive"; TabCtrl_InsertItem(hWndSettingsTab, 1, &tie); tie.pszText = "Network"; TabCtrl_InsertItem(hWndSettingsTab, 2, &tie); tie.pszText = "Other"; TabCtrl_InsertItem(hWndSettingsTab, 3, &tie);

            hWndCheckSavePos = CreateWindowExA(0, "BUTTON", "Save window position when exiting", WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_SAVE_POS, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndCheckSavePos);
            hWndCheckMinClose = CreateWindowExA(0, "BUTTON", "Minimise to notification icon instead of closing the app", WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_MIN_CLOSE, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndCheckMinClose);
            hWndCheckTopmost = CreateWindowExA(0, "BUTTON", "Make app always on top", WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_TOPMOST, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndCheckTopmost);

            hWndRecvLbl = CreateWindowExA(0, "STATIC", "When I receive a file...", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndRecvLbl);
            hWndRecvRadApp = CreateWindowExA(0, "BUTTON", "Save files in the same path as this app", WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP, 0, 0, 0, 0, hWnd, (HMENU)IDC_RECV_RAD_APP, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndRecvRadApp);
            hWndRecvRadDl = CreateWindowExA(0, "BUTTON", "Save files in the Downloads folder", WS_CHILD | BS_AUTORADIOBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_RECV_RAD_DL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndRecvRadDl);
            hWndRecvRadCustom = CreateWindowExA(0, "BUTTON", "Save files in this path:", WS_CHILD | BS_AUTORADIOBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_RECV_RAD_CUSTOM, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndRecvRadCustom);
            hWndRecvTxtPath = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_RECV_TXT_PATH, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndRecvTxtPath);
            hWndRecvBtnBrowse = CreateWindowExA(0, "BUTTON", "Browse", WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_RECV_BTN_BROWSE, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndRecvBtnBrowse);
            SHAutoComplete(hWndRecvTxtPath, SHACF_FILESYS_DIRS);
            hWndCheckQuickSave = CreateWindowExA(0, "BUTTON", "Quick Save (save automatically without asking)", WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_QUICK_SAVE, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndCheckQuickSave);

            // Network controls creation
            hWndCheckReqPin = CreateWindowExA(0, "BUTTON", "Require PIN", WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_REQ_PIN, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndCheckReqPin);
            hWndLblPinCode = CreateWindowExA(0, "STATIC", "PIN Code:", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndLblPinCode);
            hWndEditPinCode = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL | ES_NUMBER, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_PIN_CODE, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndEditPinCode); SendMessage(hWndEditPinCode, EM_SETLIMITTEXT, 8, 0);

            hWndLblDiscTimeout = CreateWindowExA(0, "STATIC", "Discovery Timeout (sec):", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndLblDiscTimeout);
            hWndEditDiscTimeout = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL | ES_NUMBER, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_DISC_TIMEOUT, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndEditDiscTimeout); SendMessage(hWndEditDiscTimeout, EM_SETLIMITTEXT, 4, 0);

            hWndLblMulticast = CreateWindowExA(0, "STATIC", "Multicast Address:", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndLblMulticast);
            hWndEditMulticast = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_MULTICAST, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndEditMulticast); SendMessage(hWndEditMulticast, EM_SETLIMITTEXT, 63, 0);

            hWndCheckEncryption = CreateWindowExA(0, "BUTTON", "Enable Encryption (TLS)", WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_ENCRYPTION, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndCheckEncryption);

            hWndLblDevType = CreateWindowExA(0, "STATIC", "Device Type:", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndLblDevType);
            hWndComboDevType = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_DEV_TYPE, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndComboDevType);

            hWndLblDevModel = CreateWindowExA(0, "STATIC", "Device Model:", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndLblDevModel);
            hWndEditDevModel = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_DEV_MODEL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndEditDevModel); SendMessage(hWndEditDevModel, EM_SETLIMITTEXT, 63, 0);

            hWndLblPort = CreateWindowExA(0, "STATIC", "Port:", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndLblPort);
            hWndEditPort = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL | ES_NUMBER, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_PORT, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndEditPort); SendMessage(hWndEditPort, EM_SETLIMITTEXT, 5, 0);

            hWndIconStatic = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_ICON, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL);
            HICON hExeIcon = (HICON)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCE(101), IMAGE_ICON, 48, 48, LR_SHARED);
            if (hExeIcon) SendMessage(hWndIconStatic, STM_SETICON, (WPARAM)hExeIcon, 0); else SendMessage(hWndIconStatic, STM_SETICON, (WPARAM)LoadIcon(NULL, IDI_APPLICATION), 0);
            char aboutTxt[512]; _snprintf(aboutTxt, sizeof(aboutTxt), "%s\r\nVersion: 1.0.0\r\nPublisher: Fratta\r\n\r\n%s", APP_NAME, APP_ABOUT);
            hWndLabelAbout = CreateWindowExA(0, "STATIC", aboutTxt, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndLabelAbout);
            hWndBtnGithub = CreateWindowExA(0, "BUTTON", "About LocalSend", WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_BTN_GH, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndBtnGithub);
            hWndBtnSupport = CreateWindowExA(0, "BUTTON", APP_SUPPORT_BTN, WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_BTN_SUPP, GetModuleHandle(NULL), NULL); ApplyWindowFont(hWndBtnSupport);

            LoadSettings();
            UpdatePageVisibility();
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
            return 1;
        }

        case WM_GETMINMAXINFO: { LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam; lpMMI->ptMinTrackSize.x = 600; lpMMI->ptMinTrackSize.y = 340; return 0; }
        case WM_SIZE: { ResizeControls(hWnd, LOWORD(lParam), HIWORD(lParam)); break; }
        case WM_LBUTTONDOWN: { int xPos = LOWORD(lParam); int mainTab = TabCtrl_GetCurSel(hWndTab); if ((mainTab == 0 || mainTab == 1) && xPos >= g_SplitterPos && xPos <= g_SplitterPos + 10) { g_bDraggingSplitter = true; SetCapture(hWnd); SetCursor(LoadCursor(NULL, IDC_SIZEWE)); } break; }
        case WM_MOUSEMOVE: { int mainTab = TabCtrl_GetCurSel(hWndTab); if (mainTab != 0 && mainTab != 1) break; int xPos = LOWORD(lParam); if (g_bDraggingSplitter) { if (xPos > 200 && xPos < 600) { g_SplitterPos = xPos; RECT rect; GetClientRect(hWnd, &rect); InvalidateRect(hWnd, NULL, TRUE); ResizeControls(hWnd, rect.right, rect.bottom); } } else if (xPos >= g_SplitterPos && xPos <= g_SplitterPos + 10) { SetCursor(LoadCursor(NULL, IDC_SIZEWE)); } break; }
        case WM_LBUTTONUP: { if (g_bDraggingSplitter) { g_bDraggingSplitter = false; ReleaseCapture(); } break; }
        case WM_CTLCOLORSTATIC: { HDC hdcStatic = (HDC)wParam; SetBkMode(hdcStatic, TRANSPARENT); return (LRESULT)GetSysColorBrush(COLOR_BTNFACE); }

        case WM_CONTEXTMENU: {
            HWND hTrigger = (HWND)wParam;
            if (hTrigger == hWndListView) { int selectedIdx = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED); if (selectedIdx != -1) { HMENU hMenu = CreatePopupMenu(); AppendMenuA(hMenu, MF_STRING, IDM_OPEN_FILE, "Open file"); AppendMenuA(hMenu, MF_STRING, IDM_COPY_FILE, "Copy path"); AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL); AppendMenuA(hMenu, MF_STRING, IDM_DELETE_FILE, "Delete"); AppendMenuA(hMenu, MF_STRING, IDM_PROP_FILE, "Properties"); TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, NULL); DestroyMenu(hMenu); } }
            break;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam); int wmEvent = HIWORD(wParam);
            if (wmId == 7001) {
                int selectedIdx = ListView_GetNextItem(hWndListSendFiles, -1, LVNI_SELECTED);
                if (selectedIdx != -1) {
                    LVITEMA lvi = {0}; lvi.iItem = selectedIdx; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListSendFiles, &lvi);
                    int queueIdx = (int)lvi.lParam;
                    if (queueIdx >= 0 && queueIdx < g_sendQueueCount) {
                        for (int i = queueIdx; i < g_sendQueueCount - 1; i++) {
                            g_sendQueue[i] = g_sendQueue[i + 1];
                        }
                        g_sendQueueCount--;
                        ListView_DeleteItem(hWndListSendFiles, selectedIdx);
                        int count = ListView_GetItemCount(hWndListSendFiles);
                        for (int i = 0; i < count; i++) {
                            LVITEMA updateLvi = {0}; updateLvi.iItem = i; updateLvi.mask = LVIF_PARAM; ListView_GetItem(hWndListSendFiles, &updateLvi);
                            int currentQIdx = (int)updateLvi.lParam;
                            if (currentQIdx > queueIdx) {
                                updateLvi.lParam = (LPARAM)(currentQIdx - 1);
                                ListView_SetItem(hWndListSendFiles, &updateLvi);
                            }
                        }
                        UpdateSendButtonsState();
                    }
                }
                break;
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
                break;
            }
            if (wmId == IDC_SET_BTN_GH) { ShellExecuteA(NULL, "open", "https://localsend.org", NULL, NULL, SW_SHOWNORMAL); break; }
            if (wmId == IDC_SET_BTN_SUPP) { ShellExecuteA(NULL, "open", APP_SUPPORT_URL, NULL, NULL, SW_SHOWNORMAL); break; }

            if (wmId == IDC_RECV_RAD_APP || wmId == IDC_RECV_RAD_DL || wmId == IDC_RECV_RAD_CUSTOM) {
                g_SaveMode = (wmId == IDC_RECV_RAD_APP) ? 0 : (wmId == IDC_RECV_RAD_DL) ? 1 : 2;
                BOOL isCustom = (g_SaveMode == 2); EnableWindow(hWndRecvTxtPath, isCustom); EnableWindow(hWndRecvBtnBrowse, isCustom); SaveSettings(); break;
            }
            if (wmId == IDC_RECV_BTN_BROWSE) {
                BROWSEINFOA bi = {0}; bi.hwndOwner = hWnd; bi.lpszTitle = "Select save folder:"; bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI; LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
                if (pidl != NULL) { char folderPath[MAX_PATH]; if (SHGetPathFromIDListA(pidl, folderPath)) { SetWindowTextA(hWndRecvTxtPath, folderPath); strcpy(g_CustomPath, folderPath); g_SaveMode = 2; SendMessage(hWndRecvRadApp, BM_SETCHECK, BST_UNCHECKED, 0); SendMessage(hWndRecvRadDl, BM_SETCHECK, BST_UNCHECKED, 0); SendMessage(hWndRecvRadCustom, BM_SETCHECK, BST_CHECKED, 0); EnableWindow(hWndRecvTxtPath, TRUE); EnableWindow(hWndRecvBtnBrowse, TRUE); SaveSettings(); } CoTaskMemFree(pidl); } break;
            }
            if (wmId == IDC_RECV_TXT_PATH && wmEvent == EN_KILLFOCUS) { GetWindowTextA(hWndRecvTxtPath, g_CustomPath, MAX_PATH); SaveSettings(); break; }

            if (wmId == IDC_BTN_SEND_FILE) {
                char fileBuffer[4096] = {0}; OPENFILENAMEA ofn = {0}; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hWnd; ofn.lpstrFile = fileBuffer; ofn.nMaxFile = sizeof(fileBuffer); ofn.lpstrFilter = "Tutti i file\0*.*\0"; ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    char* p = fileBuffer; char dir[512]; strncpy(dir, p, 511); p += strlen(p) + 1;
                    if (*p == '\0') { char* fileName = strrchr(dir, '\\'); if (fileName) { fileName++; HANDLE hFile = CreateFileA(dir, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); if (hFile != INVALID_HANDLE_VALUE) { LARGE_INTEGER fs; GetFileSizeEx(hFile, &fs); CloseHandle(hFile); AddFileToSendQueue(dir, fileName, fs.QuadPart); } } }
                    else { while (*p) { char fullPath[512]; sprintf(fullPath, "%s\\%s", dir, p); HANDLE hFile = CreateFileA(fullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); if (hFile != INVALID_HANDLE_VALUE) { LARGE_INTEGER fs; GetFileSizeEx(hFile, &fs); CloseHandle(hFile); AddFileToSendQueue(fullPath, p, fs.QuadPart); } p += strlen(p) + 1; } }
                } break;
            }

            if (wmId == IDC_BTN_SEND_FOLDER) {
                BROWSEINFOA bi = {0}; bi.hwndOwner = hWnd; bi.lpszTitle = "Select a folder to send:"; bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI; LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
                if (pidl != NULL) { char folderPath[MAX_PATH]; if (SHGetPathFromIDListA(pidl, folderPath)) { char searchPath[512]; sprintf(searchPath, "%s\\*.*", folderPath); WIN32_FIND_DATAA ffd; HANDLE hFind = FindFirstFileA(searchPath, &ffd); if (hFind != INVALID_HANDLE_VALUE) { do { if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) { char fullFilePath[512]; sprintf(fullFilePath, "%s\\%s", folderPath, ffd.cFileName); LARGE_INTEGER fs; fs.LowPart = ffd.nFileSizeLow; fs.HighPart = ffd.nFileSizeHigh; AddFileToSendQueue(fullFilePath, ffd.cFileName, fs.QuadPart); } } while (ffd.cFileName[0] && FindNextFileA(hFind, &ffd)); FindClose(hFind); } } CoTaskMemFree(pidl); } break;
            }

            if (wmId == IDC_BTN_SEND_TEXT) {
                WORD* pDlgMem = (WORD*)malloc(1024); memset(pDlgMem, 0, 1024); LPDLGTEMPLATEA lpd = (LPDLGTEMPLATEA)pDlgMem; lpd->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER; lpd->cx = 160; lpd->cy = 90;
                char* textResult = (char*)DialogBoxIndirectParamA(GetModuleHandle(NULL), lpd, hWnd, TextDialogProc, 0); free(pDlgMem);
                if (textResult) { char tempPath[MAX_PATH], tempFile[MAX_PATH]; GetTempPathA(MAX_PATH, tempPath); GetTempFileNameA(tempPath, "ls", 0, tempFile); char finalTxtPath[MAX_PATH]; sprintf(finalTxtPath, "%s.txt", tempFile); FILE* tf = fopen(finalTxtPath, "w"); if (tf) { fputs(textResult, tf); fclose(tf); AddFileToSendQueue(finalTxtPath, "text.txt", strlen(textResult)); } free(textResult); } break;
            }

            if (wmId == IDC_BTN_SEND_PASTE) {
                if (OpenClipboard(hWnd)) {
                    if (IsClipboardFormatAvailable(CF_TEXT)) { HGLOBAL hMem = GetClipboardData(CF_TEXT); if (hMem) { char* pText = (char*)GlobalLock(hMem); if (pText) { char tempPath[MAX_PATH], finalTxtPath[MAX_PATH]; GetTempPathA(MAX_PATH, tempPath); sprintf(finalTxtPath, "%s\\clip_%ld.txt", tempPath, GetTickCount()); FILE* tf = fopen(finalTxtPath, "w"); if (tf) { fputs(pText, tf); fclose(tf); AddFileToSendQueue(finalTxtPath, "text.txt", strlen(pText)); } GlobalUnlock(hMem); } } }
                    else if (IsClipboardFormatAvailable(CF_HDROP)) { HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP); if (hDrop) { int fileCount = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0); for (int i = 0; i < fileCount; i++) { char filePath[MAX_PATH]; DragQueryFileA(hDrop, i, filePath, MAX_PATH); char* fileName = strrchr(filePath, '\\'); fileName = fileName ? fileName + 1 : filePath; HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); if (hFile != INVALID_HANDLE_VALUE) { LARGE_INTEGER fs; GetFileSizeEx(hFile, &fs); CloseHandle(hFile); AddFileToSendQueue(filePath, fileName, fs.QuadPart); } } } }
                    else if (IsClipboardFormatAvailable(CF_BITMAP)) { HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP); if (hBitmap) { char tempPath[MAX_PATH], bmpPath[MAX_PATH]; GetTempPathA(MAX_PATH, tempPath); sprintf(bmpPath, "%s\\clip_%ld.bmp", tempPath, GetTickCount()); BITMAP bmp; HDC hdc = GetDC(hWnd); GetObject(hBitmap, sizeof(BITMAP), &bmp); BITMAPFILEHEADER bfh = {0}; bfh.bfType = 0x4D42; bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); bfh.bfSize = bfh.bfOffBits + bmp.bmWidthBytes * bmp.bmHeight; BITMAPINFOHEADER bih = {0}; bih.biSize = sizeof(BITMAPINFOHEADER); bih.biWidth = bmp.bmWidth; bih.biHeight = bmp.bmHeight; bih.biPlanes = 1; bih.biBitCount = bmp.bmBitsPixel; bih.biCompression = BI_RGB; FILE* f = fopen(bmpPath, "wb"); if (f) { fwrite(&bfh, 1, sizeof(bfh), f); fwrite(&bih, 1, sizeof(bih), f); char* pBits = (char*)malloc(bmp.bmWidthBytes * bmp.bmHeight); GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, pBits, (BITMAPINFO*)&bih, DIB_RGB_COLORS); fwrite(pBits, 1, bmp.bmWidthBytes * bmp.bmHeight, f); free(pBits); fclose(f); AddFileToSendQueue(bmpPath, "immagine_appunti.bmp", bfh.bfSize); } ReleaseDC(hWnd, hdc); } }
                    CloseClipboard();
                } break;
            }

            if (wmId == IDC_BTN_CLEAN_FILES) { ListView_DeleteAllItems(hWndListSendFiles); g_sendQueueCount = 0; memset(g_sendQueue, 0, sizeof(g_sendQueue)); ShowWindow(hWndSendProgress, SW_HIDE); ShowWindow(hWndSendStatusIcon, SW_HIDE); ShowWindow(hWndSendStatusTxt, SW_HIDE); UpdateSendButtonsState(); break; }

            if (wmId == IDC_BTN_SEND_FILES) {
                int selectedDeviceIdx = ListView_GetNextItem(hWndListDevices, -1, LVNI_SELECTED);
                if (g_sendQueueCount == 0 || selectedDeviceIdx == -1) { UpdateSendButtonsState(); break; }
                LVITEMA lvi = {0}; lvi.iItem = selectedDeviceIdx; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListDevices, &lvi); RemoteDevice* targetDevice = (RemoteDevice*)lvi.lParam;
                if (targetDevice) {
                    SendSessionContext* txCtx = (SendSessionContext*)malloc(sizeof(SendSessionContext));
                    if (txCtx) {
                        strncpy(txCtx->targetIP, targetDevice->ipAddress, 15); txCtx->targetIP[15] = '\0'; txCtx->targetPort = targetDevice->port; txCtx->fileCount = g_sendQueueCount; memcpy(txCtx->files, g_sendQueue, sizeof(FileToSend) * g_sendQueueCount);
                        ShowWindow(hWndSendProgress, SW_SHOW); SendMessage(hWndSendProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); SendMessage(hWndSendProgress, PBM_SETPOS, 0, 0);
                        char* copyMsg = (char*)malloc(128);
                        if (copyMsg) {
                            strcpy(copyMsg, "Connecting to remote peer...");
                            PostMessage(hWnd, WM_SEND_STATUS_UPDATE, ICON_TRANSFERRING, (LPARAM)copyMsg);
                        }
                        HANDLE hTxThread = CreateThread(NULL, 0, StartSendSessionThread, (LPVOID)txCtx, 0, NULL); if (hTxThread) CloseHandle(hTxThread);
                    }
                } break;
            }

            if (wmId == IDC_BTN_SEND_MANUAL) {
                if (g_sendQueueCount == 0) { MessageBoxA(hWnd, "Please add some files first.", "List Empty", MB_ICONEXCLAMATION); break; }
                WORD* pDlgMem = (WORD*)malloc(1024); memset(pDlgMem, 0, 1024); LPDLGTEMPLATEA lpd = (LPDLGTEMPLATEA)pDlgMem; lpd->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER; lpd->cx = 160; lpd->cy = 75;
                char* textResult = (char*)DialogBoxIndirectParamA(GetModuleHandle(NULL), lpd, hWnd, ManualSendDialogProc, 0); free(pDlgMem);

                if (textResult) {
                    char targetIp[32] = {0}; int targetPort = 53317; BOOL found = FALSE;

                    if (textResult[0] == '#') {
                        // Search ignoring case and spaces
                        char targetHash[128] = {0}; int j = 0;
                        for (int k = 1; textResult[k]; k++) { if (textResult[k] != ' ') targetHash[j++] = tolower(textResult[k]); }

                        int count = ListView_GetItemCount(hWndListDevices);
                        for (int i = 0; i < count; i++) {
                            LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListDevices, &lvi); RemoteDevice* dev = (RemoteDevice*)lvi.lParam;
                            if (dev) {
                                char cleanAlias[128] = {0}; int idx = 0;
                                for (int k = 0; dev->alias[k]; k++) { if (dev->alias[k] != ' ') cleanAlias[idx++] = tolower(dev->alias[k]); }
                                char cleanFingerprint[128] = {0}; idx = 0;
                                for (int k = 0; dev->fingerprint[k]; k++) { if (dev->fingerprint[k] != ' ') cleanFingerprint[idx++] = tolower(dev->fingerprint[k]); }
                                const char* lastDot = strrchr(dev->ipAddress, '.');
                                const char* devHashtag = lastDot ? (lastDot + 1) : dev->ipAddress;

                                if (strcmp(cleanAlias, targetHash) == 0 || strcmp(cleanFingerprint, targetHash) == 0 || strcmp(devHashtag, targetHash) == 0) {
                                    strcpy(targetIp, dev->ipAddress); targetPort = dev->port; found = TRUE; break;
                                }
                            }
                        }
                        if (!found) { MessageBoxA(hWnd, "Hashtag not found in nearby list. Please scan again.", "Not found", MB_ICONERROR); free(textResult); break; }
                    } else { strcpy(targetIp, textResult); found = TRUE; }
                    free(textResult);

                    if (found) {
                        SendSessionContext* txCtx = (SendSessionContext*)malloc(sizeof(SendSessionContext));
                        if (txCtx) {
                            strncpy(txCtx->targetIP, targetIp, 15); txCtx->targetIP[15] = '\0'; txCtx->targetPort = targetPort; txCtx->fileCount = g_sendQueueCount; memcpy(txCtx->files, g_sendQueue, sizeof(FileToSend) * g_sendQueueCount);
                            ShowWindow(hWndSendProgress, SW_SHOW); SendMessage(hWndSendProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); SendMessage(hWndSendProgress, PBM_SETPOS, 0, 0);
                            char* copyMsg = (char*)malloc(128);
                            if (copyMsg) {
                                strcpy(copyMsg, "Connecting to remote peer...");
                                PostMessage(hWnd, WM_SEND_STATUS_UPDATE, ICON_TRANSFERRING, (LPARAM)copyMsg);
                            }
                            HANDLE hTxThread = CreateThread(NULL, 0, StartSendSessionThread, (LPVOID)txCtx, 0, NULL); if (hTxThread) CloseHandle(hTxThread);
                        }
                    }
                } break;
            }

            if (wmId == IDC_BTN_SEARCH_AGAIN) {
                int devCount = ListView_GetItemCount(hWndListDevices);
                for (int i = 0; i < devCount; i++) {
                    LVITEMA lvi = {0}; lvi.iItem = i; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListDevices, &lvi);
                    if (lvi.lParam) free((void*)lvi.lParam);
                }
                ListView_DeleteAllItems(hWndListDevices);
                ShowWindow(hWndSearchLoading, SW_SHOW); EnableWindow(hWndBtnSearchAgain, FALSE); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); if (g_mySocket != INVALID_SOCKET) { sendDiscoveryShout(g_mySocket); } SetTimer(hWnd, 999, 3000, NULL); UpdateSendButtonsState(); break;
            }
            if (wmId == IDC_BTN_CLOSE_STATUS) { int totalItems = ListView_GetItemCount(hWndListView); for (int i = 0; i < totalItems; i++) { LVITEMA itemMod = {0}; itemMod.iItem = i; itemMod.mask = LVIF_GROUPID; ListView_GetItem(hWndListView, &itemMod); if (itemMod.iGroupId == 1) { itemMod.iGroupId = 2; ListView_SetItem(hWndListView, &itemMod); } } SetWindowTextA(hWndStatus, "Waiting for files..."); UpdateStatusIcon(ICON_WAITING); ShowWindow(hWndBtnCloseStatus, SW_HIDE); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); break; }
            if (wmId == IDC_BTN_EDIT_DEVICE) { g_bEditingDeviceName = TRUE; SetWindowTextA(hWndEditDeviceBox, g_MyDeviceName); RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); UpdatePageVisibility(); SetFocus(hWndEditDeviceBox); break; }
            if (wmId == IDC_BTN_SAVE_DEVICE) { GetWindowTextA(hWndEditDeviceBox, g_MyDeviceName, sizeof(g_MyDeviceName)); SetWindowTextA(hWndDeviceName, g_MyDeviceName); g_bEditingDeviceName = FALSE; RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); UpdatePageVisibility(); SaveSettings(); break; }
            if (wmId == IDC_BTN_CANCEL_DEVICE) { g_bEditingDeviceName = FALSE; RECT rc; GetClientRect(hWnd, &rc); ResizeControls(hWnd, rc.right, rc.bottom); UpdatePageVisibility(); break; }

            if (wmId == IDC_SET_TOPMOST && wmEvent == BN_CLICKED) { LRESULT lChecked = SendMessage(hWndCheckTopmost, BM_GETCHECK, 0, 0); if (lChecked == BST_CHECKED) { SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); } else { SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); } SaveSettings(); break; }
            if (wmId == IDC_SET_MIN_CLOSE && wmEvent == BN_CLICKED) { SaveSettings(); break; }
            if (wmId == IDC_SET_SAVE_POS && wmEvent == BN_CLICKED) { SaveSettings(); break; }
            if (wmId == IDC_SET_QUICK_SAVE && wmEvent == BN_CLICKED) { SaveSettings(); break; }
            if (wmId == IDC_SET_REQ_PIN && wmEvent == BN_CLICKED) { SaveSettings(); break; }
            if (wmId == IDC_SET_PIN_CODE && wmEvent == EN_KILLFOCUS) { SaveSettings(); break; }
            if (wmId == IDC_SET_DISC_TIMEOUT && wmEvent == EN_KILLFOCUS) { SaveSettings(); break; }
            if (wmId == IDC_SET_MULTICAST && wmEvent == EN_KILLFOCUS) { SaveSettings(); break; }
            if (wmId == IDC_SET_ENCRYPTION && wmEvent == BN_CLICKED) { SaveSettings(); break; }
            if (wmId == IDC_SET_DEV_TYPE && wmEvent == CBN_SELCHANGE) { SaveSettings(); break; }
            if (wmId == IDC_SET_DEV_MODEL && wmEvent == EN_KILLFOCUS) { SaveSettings(); break; }
            if (wmId == IDC_SET_PORT && wmEvent == EN_KILLFOCUS) { SaveSettings(); break; }

            int selectedIdx = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
            if (selectedIdx != -1) {
                LVITEMA lvi = {0}; lvi.iItem = selectedIdx; lvi.mask = LVIF_PARAM; ListView_GetItem(hWndListView, &lvi); LoggedTransfer* transfer = (LoggedTransfer*)lvi.lParam;
                char filePath[MAX_PATH] = {0}; GetConfiguredSavePath(filePath, MAX_PATH, transfer->fileName);

                if (wmId == IDM_OPEN_FILE) ShellExecuteA(NULL, "open", filePath, NULL, NULL, SW_SHOWNORMAL);
                else if (wmId == IDM_DELETE_FILE) {
                    if (DeleteFileA(filePath) || GetLastError() == ERROR_FILE_NOT_FOUND) { char fileToPurge[256]; strcpy(fileToPurge, transfer->fileName); int totalItems = ListView_GetItemCount(hWndListView); for (int i = totalItems - 1; i >= 0; i--) { LVITEMA itemLoop = {0}; itemLoop.iItem = i; itemLoop.mask = LVIF_PARAM; ListView_GetItem(hWndListView, &itemLoop); LoggedTransfer* tLoop = (LoggedTransfer*)itemLoop.lParam; if (tLoop && strcmp(tLoop->fileName, fileToPurge) == 0) { ListView_DeleteItem(hWndListView, i); } } } else { MessageBoxA(hWnd, "Unable to delete file.", "Error", MB_ICONERROR); }
                }
                else if (wmId == IDM_PROP_FILE) { SHELLEXECUTEINFOA sei = {0}; sei.cbSize = sizeof(SHELLEXECUTEINFOA); sei.fMask = SEE_MASK_INVOKEIDLIST; sei.lpVerb = "properties"; sei.lpFile = filePath; ShellExecuteExA(&sei); }
                else if (wmId == IDM_COPY_FILE) { if (OpenClipboard(NULL)) { EmptyClipboard(); HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strlen(filePath) + 1); memcpy(GlobalLock(hMem), filePath, strlen(filePath) + 1); GlobalUnlock(hMem); SetClipboardData(CF_TEXT, hMem); CloseClipboard(); } }
            } break;
        }

        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            if (nmhdr->code == TCN_SELCHANGE) { UpdatePageVisibility(); InvalidateRect(hWnd, NULL, TRUE); RECT rect; GetClientRect(hWnd, &rect); ResizeControls(hWnd, rect.right, rect.bottom); }
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

INT_PTR CALLBACK TextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            SetWindowTextA(hDlg, "Insert text message"); RECT rc; GetClientRect(hDlg, &rc);
            HWND hWndEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL, 10, 10, rc.right - 20, rc.bottom - 45, hDlg, (HMENU)IDC_TEXT_DLG_EDIT, GetModuleHandle(NULL), NULL);
            HWND hWndOK = CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, (rc.right / 2) - 80, rc.bottom - 30, 75, 24, hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            HWND hWndCancel = CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, (rc.right / 2) + 5, rc.bottom - 30, 75, 24, hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
            ApplyWindowFont(hWndEdit); ApplyWindowFont(hWndOK); ApplyWindowFont(hWndCancel); SetFocus(hWndEdit); return FALSE;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDOK) {
                HWND hWndEdit = GetDlgItem(hDlg, IDC_TEXT_DLG_EDIT); int len = GetWindowTextLengthA(hWndEdit);
                if (len > 0) { char* buf = (char*)malloc(len + 1); GetWindowTextA(hWndEdit, buf, len + 1); EndDialog(hDlg, (INT_PTR)buf); } else { EndDialog(hDlg, 0); } return TRUE;
            } else if (wmId == IDCANCEL) { EndDialog(hDlg, 0); return TRUE; } break;
        }
    } return FALSE;
}

INT_PTR CALLBACK ManualSendDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            SetWindowTextA(hDlg, "Send manually"); RECT rc; GetClientRect(hDlg, &rc);
            HWND hRadIP = CreateWindowExA(0, "BUTTON", "IP Address", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 10, 10, 90, 20, hDlg, (HMENU)IDC_MANUAL_RAD_IP, GetModuleHandle(NULL), NULL);
            HWND hRadHash = CreateWindowExA(0, "BUTTON", "Hashtag", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 110, 10, 90, 20, hDlg, (HMENU)IDC_MANUAL_RAD_HASH, GetModuleHandle(NULL), NULL);
            HWND hWndEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 10, 35, rc.right - 20, 24, hDlg, (HMENU)IDC_MANUAL_DLG_EDIT, GetModuleHandle(NULL), NULL);
            HWND hWndOK = CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, (rc.right / 2) - 80, rc.bottom - 30, 75, 24, hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            HWND hWndCancel = CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, (rc.right / 2) + 5, rc.bottom - 30, 75, 24, hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
            SendMessage(hRadIP, BM_SETCHECK, BST_CHECKED, 0);
            ApplyWindowFont(hRadIP); ApplyWindowFont(hRadHash); ApplyWindowFont(hWndEdit); ApplyWindowFont(hWndOK); ApplyWindowFont(hWndCancel); SetFocus(hWndEdit); return FALSE;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDOK) {
                HWND hWndEdit = GetDlgItem(hDlg, IDC_MANUAL_DLG_EDIT); int len = GetWindowTextLengthA(hWndEdit);
                if (len > 0) {
                    char* buf = (char*)malloc(len + 2); GetWindowTextA(hWndEdit, buf, len + 1);
                    BOOL isHash = (SendMessage(GetDlgItem(hDlg, IDC_MANUAL_RAD_HASH), BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (isHash && buf[0] != '#') { memmove(buf + 1, buf, len + 1); buf[0] = '#'; }
                    EndDialog(hDlg, (INT_PTR)buf);
                } else { EndDialog(hDlg, 0); } return TRUE;
            } else if (wmId == IDCANCEL) { EndDialog(hDlg, 0); return TRUE; } break;
        }
    } return FALSE;
}

static INT_PTR CALLBACK PinPromptDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static char* outPin = NULL;
    static HFONT hFont = NULL;
    switch (message) {
        case WM_INITDIALOG: {
            outPin = (char*)lParam;
            SetWindowTextA(hDlg, "Enter PIN");
            RECT rc; GetClientRect(hDlg, &rc);
            HWND hLbl = CreateWindowExA(0, "STATIC", "The receiver requires a PIN code to complete the transfer:", WS_CHILD | WS_VISIBLE | SS_LEFT, 15, 12, rc.right - 30, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
            HWND hEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | ES_CENTER, (rc.right - 100) / 2, 38, 100, 24, hDlg, (HMENU)7010, GetModuleHandle(NULL), NULL);
            HWND hBtnOK = CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, (rc.right / 2) - 80, rc.bottom - 32, 75, 24, hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            HWND hBtnCancel = CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, (rc.right / 2) + 5, rc.bottom - 32, 75, 24, hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
            SendMessage(hEdit, EM_SETLIMITTEXT, 8, 0);
            NONCLIENTMETRICSA ncm; ncm.cbSize = sizeof(NONCLIENTMETRICSA);
            SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), &ncm, 0);
            hFont = CreateFontA(ncm.lfMessageFont.lfHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            SendMessage(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hBtnOK, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);
            SetFocus(hEdit);
            return FALSE;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDOK) {
                HWND hEdit = GetDlgItem(hDlg, 7010);
                GetWindowTextA(hEdit, outPin, 15);
                if (hFont) { DeleteObject(hFont); hFont = NULL; }
                EndDialog(hDlg, 1);
                return TRUE;
            } else if (wmId == IDCANCEL) {
                if (hFont) { DeleteObject(hFont); hFont = NULL; }
                EndDialog(hDlg, 0);
                return TRUE;
            }
            break;
        }
        case WM_NCDESTROY: {
            if (hFont) { DeleteObject(hFont); hFont = NULL; }
            break;
        }
    }
    return FALSE;
}

static bool ShowPinPrompt(HWND parent, const char* targetName, char* outPin) {
    WORD* pDlgMem = (WORD*)malloc(1024);
    memset(pDlgMem, 0, 1024);
    LPDLGTEMPLATEA lpd = (LPDLGTEMPLATEA)pDlgMem;
    lpd->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER;
    lpd->cx = 180;
    lpd->cy = 75;
    INT_PTR ret = DialogBoxIndirectParamA(GetModuleHandle(NULL), lpd, parent, PinPromptDlgProc, (LPARAM)outPin);
    free(pDlgMem);
    return (ret == 1);
}

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
    g_hWndMain = CreateWindowExA(0, "LocalSendRT_GUI", APP_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 420, NULL, NULL, hInstance, NULL);
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

static LRESULT CALLBACK ConfirmationWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HWND hEditPath = NULL;
    static HWND hBtnBrowse = NULL;
    static HWND hListView = NULL;
    static HWND hBtnAccept = NULL;
    static HWND hBtnCancel = NULL;
    static ConfirmationRequest* ctx = NULL;
    switch (message) {
        case WM_CREATE: {
            CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
            ctx = (ConfirmationRequest*)pcs->lpCreateParams;

            NONCLIENTMETRICSA ncm; ncm.cbSize = sizeof(NONCLIENTMETRICSA);
            SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), &ncm, 0);
            HFONT hFont = CreateFontA(ncm.lfMessageFont.lfHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

            char promptText[512];
            _snprintf(promptText, sizeof(promptText), "%s wants to send you", ctx->senderName);
            HWND hLbl1 = CreateWindowExA(0, "STATIC", promptText, WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 20, 440, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hLbl1, WM_SETFONT, (WPARAM)hFont, TRUE);

            _snprintf(promptText, sizeof(promptText), "%d files", ctx->fileCount);
            HWND hLbl2 = CreateWindowExA(0, "STATIC", promptText, WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 40, 440, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hLbl2, WM_SETFONT, (WPARAM)hFont, TRUE);

            HWND hLbl3 = CreateWindowExA(0, "STATIC", "Save to:", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 70, 60, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hLbl3, WM_SETFONT, (WPARAM)hFont, TRUE);

            hEditPath = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", ctx->selectedPath, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 80, 68, 280, 22, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hEditPath, WM_SETFONT, (WPARAM)hFont, TRUE);

            hBtnBrowse = CreateWindowExA(0, "BUTTON", "Browse...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 370, 67, 90, 24, hWnd, (HMENU)1001, GetModuleHandle(NULL), NULL);
            SendMessage(hBtnBrowse, WM_SETFONT, (WPARAM)hFont, TRUE);

            hListView = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL, 20, 105, 440, 210, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
            ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            if (hSystemImageList == NULL && ctx->fileCount > 0) {
                GetSystemIconIndex(g_fileQueue[0].fileName);
            }
            if (hSystemImageList) {
                ListView_SetImageList(hListView, hSystemImageList, LVSIL_SMALL);
            }

            LVCOLUMNA lvc;
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = "File";
            lvc.cx = 300;
            ListView_InsertColumn(hListView, 0, &lvc);
            lvc.pszText = "Size";
            lvc.cx = 100;
            ListView_InsertColumn(hListView, 1, &lvc);

            for (int i = 0; i < ctx->fileCount; i++) {
                LVITEMA lvi = {0};
                lvi.mask = LVIF_TEXT | LVIF_IMAGE;
                lvi.iItem = i;
                lvi.iSubItem = 0;
                lvi.pszText = g_fileQueue[i].fileName;
                lvi.iImage = GetSystemIconIndex(g_fileQueue[i].fileName);
                int idx = ListView_InsertItem(hListView, &lvi);
                
                char sizeStr[64];
                if (g_fileQueue[i].fileSize > 1024 * 1024) {
                    sprintf(sizeStr, "%.2f MB", (double)g_fileQueue[i].fileSize / (1024 * 1024));
                } else if (g_fileQueue[i].fileSize > 1024) {
                    sprintf(sizeStr, "%.2f KB", (double)g_fileQueue[i].fileSize / 1024);
                } else {
                    sprintf(sizeStr, "%lld B", g_fileQueue[i].fileSize);
                }
                ListView_SetItemText(hListView, idx, 1, sizeStr);
            }

            hBtnAccept = CreateWindowExA(0, "BUTTON", "Accept", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 260, 330, 90, 30, hWnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            SendMessage(hBtnAccept, WM_SETFONT, (WPARAM)hFont, TRUE);

            hBtnCancel = CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 370, 330, 90, 30, hWnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
            SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

            break;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == 1001) {
                BROWSEINFOA bi = {0};
                bi.hwndOwner = hWnd;
                bi.lpszTitle = "Select save folder:";
                bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
                LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
                if (pidl != NULL) {
                    char folderPath[MAX_PATH];
                    if (SHGetPathFromIDListA(pidl, folderPath)) {
                        SetWindowTextA(hEditPath, folderPath);
                    }
                    CoTaskMemFree(pidl);
                }
            } else if (wmId == IDOK) {
                GetWindowTextA(hEditPath, ctx->selectedPath, MAX_PATH);
                ctx->accepted = true;
                DestroyWindow(hWnd);
            } else if (wmId == IDCANCEL) {
                ctx->accepted = false;
                DestroyWindow(hWnd);
            }
            break;
        }
        case WM_CLOSE:
            ctx->accepted = false;
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

static bool ShowTransferConfirmation(HWND parent, const char* senderName, int fileCount, char* outSavePath) {
    WNDCLASSEXA wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEXA);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = ConfirmationWndProc;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = "LocalSendConfirmClass";
    RegisterClassExA(&wcex);

    ConfirmationRequest ctx;
    ctx.senderName = senderName;
    ctx.fileCount = fileCount;
    ctx.accepted = false;
    strncpy(ctx.selectedPath, outSavePath, MAX_PATH);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int w = 490;
    int h = 410;
    int x = (screenW - w) / 2;
    int y = (screenH - h) / 2;

    if (parent) EnableWindow(parent, FALSE);

    HWND hWnd = CreateWindowExA(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, "LocalSendConfirmClass", "Incoming File Transfer", 
                                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_DLGFRAME,
                                x, y, w, h, parent, NULL, GetModuleHandle(NULL), &ctx);
    
    if (!hWnd) {
        if (parent) EnableWindow(parent, TRUE);
        return false;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (parent) {
        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
    }

    UnregisterClassA("LocalSendConfirmClass", GetModuleHandle(NULL));

    if (ctx.accepted) {
        strcpy(outSavePath, ctx.selectedPath);
        return true;
    }
    return false;
}
