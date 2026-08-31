#include <windows.h>
#include <stdio.h>
#include "utils.h"
#include "settings.h"
#include "tls_layer.h"

// External HWNDs defined in main.c
extern HWND g_hWndMain;
extern HWND hWndCheckTopmost;
extern HWND hWndCheckMinClose;
extern HWND hWndCheckSavePos;
extern HWND hWndCheckQuickSave;
extern HWND hWndDeviceName;
extern HWND hWndCheckReqPin;
extern HWND hWndEditPinCode;
extern HWND hWndEditDiscTimeout;
extern HWND hWndEditMulticast;
extern HWND hWndCheckEncryption;
extern HWND hWndComboDevType;
extern HWND hWndEditDevModel;
extern HWND hWndEditPort;
extern HWND hWndRecvRadApp;
extern HWND hWndRecvRadDl;
extern HWND hWndRecvRadCustom;
extern HWND hWndRecvTxtPath;
extern HWND hWndRecvBtnBrowse;
extern HWND hWndComboLanguage;


// Determines the path to the config file based on module filename
void InitSettingsPath() {
    GetModuleFileNameA(NULL, g_IniPath, MAX_PATH);
    // Strip the executable filename to get the folder path
    char* p = strrchr(g_IniPath, '\\'); if (p) *p = '\0';
#ifdef __arm__
    strcat(g_IniPath, "\\localsend_rt.ini");
#else
    strcat(g_IniPath, "\\localsend32.ini");
#endif
}

// Loads saved preferences from the ini file and populates variables/controls
void LoadSettings() {
    InitSettingsPath();

    // General receive preferences: SaveMode (0=app path, 1=downloads, 2=custom), QuickSave flag, and custom target path
    g_SaveMode = GetPrivateProfileIntA("Settings", "SaveMode", 1, g_IniPath);
    g_QuickSave = GetPrivateProfileIntA("Settings", "QuickSave", 0, g_IniPath);
    GetPrivateProfileStringA("Settings", "CustomPath", "", g_CustomPath, MAX_PATH, g_IniPath);

    // Window behavior flags: Always on Top, Minimize to Tray on Close, Remember Window Position, and Recursive Folder
    int topmost = GetPrivateProfileIntA("Settings", "TopMost", 0, g_IniPath);
    int minClose = GetPrivateProfileIntA("Settings", "MinClose", 0, g_IniPath);
    int savePos = GetPrivateProfileIntA("Settings", "SavePos", 0, g_IniPath);
    g_RecursiveFolder = GetPrivateProfileIntA("Settings", "RecursiveFolder", 0, g_IniPath);

    SendMessage(hWndCheckTopmost, BM_SETCHECK, topmost ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hWndCheckMinClose, BM_SETCHECK, minClose ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hWndCheckSavePos, BM_SETCHECK, savePos ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hWndCheckRecursiveFolder, BM_SETCHECK, g_RecursiveFolder ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hWndCheckQuickSave, BM_SETCHECK, g_QuickSave ? BST_CHECKED : BST_UNCHECKED, 0);

    // Security & Network: PIN requirement, discovery timeout, and UDP multicast address
    g_RequirePin = GetPrivateProfileIntA("Settings", "RequirePin", 0, g_IniPath);
    GetPrivateProfileStringA("Settings", "PinCode", "1234", g_PinCode, sizeof(g_PinCode), g_IniPath);
    g_DiscoveryTimeout = GetPrivateProfileIntA("Settings", "DiscoveryTimeout", 5, g_IniPath);
    GetPrivateProfileStringA("Settings", "MulticastAddr", "224.0.0.167", g_MulticastAddr, sizeof(g_MulticastAddr), g_IniPath);
    g_EnableEncryption = GetPrivateProfileIntA("Settings", "EnableEncryption", 1, g_IniPath);
    if (!TlsIsAvailable()) {
        g_EnableEncryption = 0;
    }
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
    GetPrivateProfileStringA("Settings", "DeviceName", defaultName, g_MyDeviceName, MAX_COMPUTERNAME_LENGTH + 1, g_IniPath);

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
    SendMessageA(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)g_Lang.devTypePhone);
    SendMessageA(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)g_Lang.devTypeLaptop);
    SendMessageA(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)g_Lang.devTypeWeb);
    SendMessageA(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)g_Lang.devTypeTerminal);
    SendMessageA(hWndComboDevType, CB_ADDSTRING, 0, (LPARAM)g_Lang.devTypeServer);
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
        if (x != (int)CW_USEDEFAULT) {
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

// Serializes current preference states and coordinates to the ini file
void SaveSettings() {
    if (!g_IniPath[0]) InitSettingsPath();
    char buf[32];

    // Read current checkbox and text states from the dialog controls
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
        char tmp[16];
        GetWindowTextA(hWndEditPort, tmp, sizeof(tmp)); g_Port = atoi(tmp);
    }
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
    if (hWndCheckRecursiveFolder != NULL) g_RecursiveFolder = (SendMessage(hWndCheckRecursiveFolder, BM_GETCHECK, 0, 0) == BST_CHECKED);

    if (hWndComboLanguage != NULL) g_Language = (int)SendMessage(hWndComboLanguage, CB_GETCURSEL, 0, 0);
    sprintf(buf, "%d", g_Language); WritePrivateProfileStringA("Settings", "Language", buf, g_IniPath);
    sprintf(buf, "%d", topmost); WritePrivateProfileStringA("Settings", "TopMost", buf, g_IniPath);
    sprintf(buf, "%d", minClose); WritePrivateProfileStringA("Settings", "MinClose", buf, g_IniPath);
    sprintf(buf, "%d", savePos); WritePrivateProfileStringA("Settings", "SavePos", buf, g_IniPath);
    sprintf(buf, "%d", g_RecursiveFolder); WritePrivateProfileStringA("Settings", "RecursiveFolder", buf, g_IniPath);

    if (savePos) {
        RECT rc; GetWindowRect(g_hWndMain, &rc);
        sprintf(buf, "%d", (int)rc.left); WritePrivateProfileStringA("Settings", "WinX", buf, g_IniPath);
        sprintf(buf, "%d", (int)rc.top); WritePrivateProfileStringA("Settings", "WinY", buf, g_IniPath);
        sprintf(buf, "%d", (int)(rc.right - rc.left)); WritePrivateProfileStringA("Settings", "WinW", buf, g_IniPath);
        sprintf(buf, "%d", (int)(rc.bottom - rc.top)); WritePrivateProfileStringA("Settings", "WinH", buf, g_IniPath);
    }
}
