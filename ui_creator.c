#include "utils.h"
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#include "ui_creator.h"
#include "settings.h"

// External HWND variables defined in main.c
extern HWND hWndTab, hWndSettingsTab;
extern HWND hWndDeviceNameTitle, hWndDeviceName, hWndBtnEditDevice;
extern HWND hWndEditDeviceBox, hWndBtnSaveDevice, hWndBtnCancelDevice;
extern HWND hWndGroupBox, hWndInfoText;
extern HWND hWndStatusIcon, hWndStatus, hWndBtnCloseStatus;
extern HWND hWndListView;
extern HWND hWndBtnSndFile, hWndBtnSndFolder, hWndBtnSndText, hWndBtnSndPaste;
extern HWND hWndLblSendFiles, hWndListSendFiles;
extern HWND hWndSendProgress, hWndSendStatusIcon, hWndSendStatusTxt;
extern HWND hWndBtnCleanFiles, hWndBtnSendFiles;
extern HWND hWndLblDevices, hWndListDevices;
extern HWND hWndBtnSearchAgain, hWndSearchLoading, hWndBtnSendManual;
extern HWND hWndCheckSavePos, hWndCheckMinClose, hWndCheckTopmost;
extern HWND hWndLblLanguage, hWndComboLanguage;
extern HWND hWndRecvLbl, hWndRecvRadApp, hWndRecvRadDl, hWndRecvRadCustom, hWndRecvTxtPath, hWndRecvBtnBrowse;
extern HWND hWndCheckQuickSave;
extern HWND hWndCheckReqPin, hWndLblPinCode, hWndEditPinCode;
extern HWND hWndLblDiscTimeout, hWndEditDiscTimeout;
extern HWND hWndLblMulticast, hWndEditMulticast;
extern HWND hWndCheckEncryption;
extern HWND hWndLblDevType, hWndComboDevType;
extern HWND hWndLblDevModel, hWndEditDevModel;
extern HWND hWndLblPort, hWndEditPort;
extern HWND hWndIconStatic, hWndLabelAbout, hWndBtnGithub, hWndBtnSupport;
extern HIMAGELIST hDeviceImageList;

// Global settings/language variables defined in main.c / utils.c
extern char g_MyDeviceName[];
extern int g_Language;

// Font helper prototypes
extern void ApplyWindowFont(HWND hWndChild);
extern void ApplyLargeFont(HWND hWndChild);
extern void ApplyStatusFont(HWND hWndChild);
extern void UpdateStatusIcon(int type);
extern void AddTooltip(HWND hCtrl, char* text);
extern void GetDeviceNetworkInfo(char* outBuffer, size_t maxLen);
extern void UpdateUITexts(void);
extern void UpdatePageVisibility(void);

// Helper function to create all main window widgets
void CreateMainControls(HWND hWnd, HINSTANCE hInstance) {
    hWndTab = CreateWindowExA(0, WC_TABCONTROLA, "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 0, 0, hWnd, (HMENU)IDC_TAB_CONTROL, hInstance, NULL); ApplyWindowFont(hWndTab);
    TCITEMA tie; tie.mask = TCIF_TEXT;
    tie.pszText = g_Lang.tabReceive; TabCtrl_InsertItem(hWndTab, 0, &tie); tie.pszText = g_Lang.tabSend; TabCtrl_InsertItem(hWndTab, 1, &tie); tie.pszText = g_Lang.tabSettings; TabCtrl_InsertItem(hWndTab, 2, &tie);

    hWndDeviceNameTitle = CreateWindowExA(0, "STATIC", g_Lang.deviceName, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); ApplyWindowFont(hWndDeviceNameTitle);
    hWndDeviceName = CreateWindowExA(0, "STATIC", g_MyDeviceName, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); ApplyLargeFont(hWndDeviceName);
    hWndBtnEditDevice = CreateWindowExA(0, "BUTTON", g_Lang.editBtn, WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_EDIT_DEVICE, hInstance, NULL); ApplyWindowFont(hWndBtnEditDevice);
    hWndEditDeviceBox = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_DEVICE_NAME, hInstance, NULL); ApplyWindowFont(hWndEditDeviceBox); SendMessage(hWndEditDeviceBox, EM_SETLIMITTEXT, MAX_COMPUTERNAME_LENGTH, 0);
    hWndBtnSaveDevice = CreateWindowExA(0, "BUTTON", g_Lang.saveBtn, WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_SAVE_DEVICE, hInstance, NULL); ApplyWindowFont(hWndBtnSaveDevice);
    hWndBtnCancelDevice = CreateWindowExA(0, "BUTTON", g_Lang.cancelBtn, WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_CANCEL_DEVICE, hInstance, NULL); ApplyWindowFont(hWndBtnCancelDevice);

    hWndGroupBox = CreateWindowExA(0, "BUTTON", g_Lang.networkInfo, WS_CHILD | BS_GROUPBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_GROUP_BOX, hInstance, NULL); ApplyWindowFont(hWndGroupBox);
    char infoNet[512] = {0}; GetDeviceNetworkInfo(infoNet, sizeof(infoNet));
    hWndInfoText = CreateWindowExA(0, "STATIC", infoNet, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, (HMENU)IDC_INFO_TEXT, hInstance, NULL); ApplyWindowFont(hWndInfoText);

    hWndStatusIcon = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_ICON, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL);
    hWndStatus = CreateWindowExA(0, "STATIC", g_Lang.waitingForFiles, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, (HMENU)IDC_STATUS_TEXT, hInstance, NULL); ApplyStatusFont(hWndStatus); UpdateStatusIcon(0);
    hWndBtnCloseStatus = CreateWindowExA(0, "BUTTON", g_Lang.closeBtn, WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_CLOSE_STATUS, hInstance, NULL); ApplyWindowFont(hWndBtnCloseStatus); ShowWindow(hWndBtnCloseStatus, SW_HIDE);

    hWndListView = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 0, 0, 0, 0, hWnd, (HMENU)IDC_FILE_LISTVIEW, hInstance, NULL); ApplyWindowFont(hWndListView); ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    LVCOLUMNA lvc; lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; lvc.iSubItem = LV_COL_NAME; lvc.cx = 125; lvc.pszText = g_Lang.colFile; ListView_InsertColumn(hWndListView, LV_COL_NAME, &lvc); lvc.iSubItem = LV_COL_SIZE; lvc.cx = 75; lvc.pszText = g_Lang.colSize; ListView_InsertColumn(hWndListView, LV_COL_SIZE, &lvc); lvc.iSubItem = LV_COL_DATE; lvc.cx = 125; lvc.pszText = g_Lang.colDate; ListView_InsertColumn(hWndListView, LV_COL_DATE, &lvc); lvc.iSubItem = LV_COL_PROGRESS; lvc.cx = 125; lvc.pszText = g_Lang.colProgress; ListView_InsertColumn(hWndListView, LV_COL_PROGRESS, &lvc);

    ListView_EnableGroupView(hWndListView, TRUE); LVGROUP lvg = {0}; lvg.cbSize = sizeof(LVGROUP); lvg.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE; lvg.state = LVGS_COLLAPSIBLE;
    wchar_t wToday[64]; MultiByteToWideChar(CP_UTF8, 0, g_Lang.grpToday, -1, wToday, 64);
    wchar_t wOlder[64]; MultiByteToWideChar(CP_UTF8, 0, g_Lang.grpOlder, -1, wOlder, 64);
    lvg.pszHeader = wToday; lvg.iGroupId = 1; ListView_InsertGroup(hWndListView, -1, &lvg); lvg.pszHeader = wOlder; lvg.iGroupId = 2; ListView_InsertGroup(hWndListView, -1, &lvg);

    hWndBtnSndFile = CreateWindowExA(0, "BUTTON", g_Lang.btnFile, WS_CHILD | BS_PUSHBUTTON | BS_ICON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_FILE, hInstance, NULL);
    hWndBtnSndFolder = CreateWindowExA(0, "BUTTON", g_Lang.btnFolder, WS_CHILD | BS_PUSHBUTTON | BS_ICON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_FOLDER, hInstance, NULL);
    hWndBtnSndText = CreateWindowExA(0, "BUTTON", g_Lang.btnText, WS_CHILD | BS_PUSHBUTTON | BS_ICON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_TEXT, hInstance, NULL);
    hWndBtnSndPaste = CreateWindowExA(0, "BUTTON", g_Lang.btnPaste, WS_CHILD | BS_PUSHBUTTON | BS_ICON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_PASTE, hInstance, NULL);

    char sysPath[MAX_PATH]; GetSystemDirectoryA(sysPath, MAX_PATH); strcat(sysPath, "\\shell32.dll"); HICON hIFile, hIFolder, hIText, hIPaste; UINT dum; PrivateExtractIconsA(sysPath, 54, 24, 24, &hIFile, &dum, 1, 0); PrivateExtractIconsA(sysPath, 3, 24, 24, &hIFolder, &dum, 1, 0); PrivateExtractIconsA(sysPath, 70, 24, 24, &hIText, &dum, 1, 0); PrivateExtractIconsA(sysPath, 260, 24, 24, &hIPaste, &dum, 1, 0);
    SendMessage(hWndBtnSndFile, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIFile); SendMessage(hWndBtnSndFolder, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIFolder); SendMessage(hWndBtnSndText, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIText); SendMessage(hWndBtnSndPaste, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIPaste);
    AddTooltip(hWndBtnSndFile, g_Lang.addFilesTooltip); AddTooltip(hWndBtnSndFolder, g_Lang.addFolderTooltip); AddTooltip(hWndBtnSndText, g_Lang.sendTextTooltip); AddTooltip(hWndBtnSndPaste, g_Lang.pasteTooltip);

    hWndLblSendFiles = CreateWindowExA(0, "STATIC", g_Lang.filesToSend, WS_CHILD | SS_LEFT, 0,0,0,0, hWnd, NULL, hInstance, NULL); ApplyStatusFont(hWndLblSendFiles);
    hWndListSendFiles = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 0,0,0,0, hWnd, (HMENU)IDC_LIST_SEND_FILES, hInstance, NULL); ApplyWindowFont(hWndListSendFiles); ListView_SetExtendedListViewStyle(hWndListSendFiles, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; lvc.iSubItem = 0; lvc.cx = 200; lvc.pszText = g_Lang.colFile; ListView_InsertColumn(hWndListSendFiles, 0, &lvc); lvc.iSubItem = 1; lvc.cx = 80; lvc.pszText = g_Lang.colSize; ListView_InsertColumn(hWndListSendFiles, 1, &lvc);

    hWndSendProgress = CreateWindowExA(0, PROGRESS_CLASSA, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, (HMENU)IDC_SEND_PROGRESS, hInstance, NULL);
    hWndSendStatusIcon = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_ICON, 0, 0, 0, 0, hWnd, (HMENU)IDC_SEND_STATUS_ICON, hInstance, NULL);
    hWndSendStatusTxt = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, (HMENU)IDC_SEND_STATUS_TXT, hInstance, NULL); ApplyStatusFont(hWndSendStatusTxt);
    ShowWindow(hWndSendProgress, SW_HIDE); ShowWindow(hWndSendStatusIcon, SW_HIDE); ShowWindow(hWndSendStatusTxt, SW_HIDE);

    hWndBtnCleanFiles = CreateWindowExA(0, "BUTTON", g_Lang.cleanFiles, WS_CHILD | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_CLEAN_FILES, hInstance, NULL); ApplyWindowFont(hWndBtnCleanFiles);
    hWndBtnSendFiles  = CreateWindowExA(0, "BUTTON", g_Lang.sendFiles, WS_CHILD | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_FILES, hInstance, NULL); ApplyWindowFont(hWndBtnSendFiles);

    hWndLblDevices = CreateWindowExA(0, "STATIC", g_Lang.nearbyDevices, WS_CHILD | SS_LEFT, 0,0,0,0, hWnd, NULL, hInstance, NULL); ApplyStatusFont(hWndLblDevices);
    hWndListDevices = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 0,0,0,0, hWnd, (HMENU)IDC_LIST_DEVICES, hInstance, NULL); ApplyWindowFont(hWndListDevices); ListView_SetExtendedListViewStyle(hWndListDevices, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);
    hDeviceImageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 0, 1); ListView_SetImageList(hWndListDevices, hDeviceImageList, LVSIL_SMALL);
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM; lvc.iSubItem = 0; lvc.cx = 180; lvc.pszText = g_Lang.colDevice; ListView_InsertColumn(hWndListDevices, 0, &lvc); lvc.iSubItem = 1; lvc.cx = 150; lvc.pszText = g_Lang.colInfo; ListView_InsertColumn(hWndListDevices, 1, &lvc);

    hWndBtnSearchAgain = CreateWindowExA(0, "BUTTON", g_Lang.searchAgain, WS_CHILD | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEARCH_AGAIN, hInstance, NULL); ApplyWindowFont(hWndBtnSearchAgain);
    hWndSearchLoading = CreateWindowExA(0, PROGRESS_CLASSA, NULL, WS_CHILD | PBS_MARQUEE, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); SendMessage(hWndSearchLoading, PBM_SETMARQUEE, TRUE, 30); ShowWindow(hWndSearchLoading, SW_HIDE);
    hWndBtnSendManual = CreateWindowExA(0, "BUTTON", g_Lang.sendManually, WS_CHILD | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)IDC_BTN_SEND_MANUAL, hInstance, NULL); ApplyWindowFont(hWndBtnSendManual);

    hWndSettingsTab = CreateWindowExA(0, WC_TABCONTROLA, "", WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 0, 0, hWndTab, (HMENU)IDC_SETTINGS_TAB, hInstance, NULL); ApplyWindowFont(hWndSettingsTab);
    tie.pszText = g_Lang.settingsGeneral; TabCtrl_InsertItem(hWndSettingsTab, 0, &tie); tie.pszText = g_Lang.settingsReceive; TabCtrl_InsertItem(hWndSettingsTab, 1, &tie); tie.pszText = g_Lang.settingsNetwork; TabCtrl_InsertItem(hWndSettingsTab, 2, &tie); tie.pszText = g_Lang.settingsOther; TabCtrl_InsertItem(hWndSettingsTab, 3, &tie);

    hWndCheckSavePos = CreateWindowExA(0, "BUTTON", g_Lang.saveWindowPos, WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_SAVE_POS, hInstance, NULL); ApplyWindowFont(hWndCheckSavePos);
    hWndCheckMinClose = CreateWindowExA(0, "BUTTON", g_Lang.minimizeToTray, WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_MIN_CLOSE, hInstance, NULL); ApplyWindowFont(hWndCheckMinClose);
    hWndCheckTopmost = CreateWindowExA(0, "BUTTON", g_Lang.alwaysOnTop, WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_TOPMOST, hInstance, NULL); ApplyWindowFont(hWndCheckTopmost);

    hWndLblLanguage = CreateWindowExA(0, "STATIC", g_Lang.language, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_LANG_LABEL, hInstance, NULL); ApplyWindowFont(hWndLblLanguage);
    hWndComboLanguage = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_LANG_COMBO, hInstance, NULL); ApplyWindowFont(hWndComboLanguage);
    SendMessage(hWndComboLanguage, CB_ADDSTRING, 0, (LPARAM)"English");
    SendMessage(hWndComboLanguage, CB_ADDSTRING, 0, (LPARAM)"Italiano");
    SendMessage(hWndComboLanguage, CB_SETCURSEL, g_Language, 0);

    hWndRecvLbl = CreateWindowExA(0, "STATIC", g_Lang.whenIReceive, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); ApplyWindowFont(hWndRecvLbl);
    hWndRecvRadApp = CreateWindowExA(0, "BUTTON", g_Lang.saveAppPath, WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP, 0, 0, 0, 0, hWnd, (HMENU)IDC_RECV_RAD_APP, hInstance, NULL); ApplyWindowFont(hWndRecvRadApp);
    hWndRecvRadDl = CreateWindowExA(0, "BUTTON", g_Lang.saveDownloads, WS_CHILD | BS_AUTORADIOBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_RECV_RAD_DL, hInstance, NULL); ApplyWindowFont(hWndRecvRadDl);
    hWndRecvRadCustom = CreateWindowExA(0, "BUTTON", g_Lang.saveCustomPath, WS_CHILD | BS_AUTORADIOBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_RECV_RAD_CUSTOM, hInstance, NULL); ApplyWindowFont(hWndRecvRadCustom);
    hWndRecvTxtPath = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_RECV_TXT_PATH, hInstance, NULL); ApplyWindowFont(hWndRecvTxtPath);
    hWndRecvBtnBrowse = CreateWindowExA(0, "BUTTON", g_Lang.browse, WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_RECV_BTN_BROWSE, hInstance, NULL); ApplyWindowFont(hWndRecvBtnBrowse);
    SHAutoComplete(hWndRecvTxtPath, SHACF_FILESYS_DIRS);
    hWndCheckQuickSave = CreateWindowExA(0, "BUTTON", g_Lang.quickSave, WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_QUICK_SAVE, hInstance, NULL); ApplyWindowFont(hWndCheckQuickSave);

    // Network controls creation
    hWndCheckReqPin = CreateWindowExA(0, "BUTTON", g_Lang.requirePin, WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_REQ_PIN, hInstance, NULL); ApplyWindowFont(hWndCheckReqPin);
    hWndLblPinCode = CreateWindowExA(0, "STATIC", g_Lang.pinCode, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); ApplyWindowFont(hWndLblPinCode);
    hWndEditPinCode = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL | ES_NUMBER, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_PIN_CODE, hInstance, NULL); ApplyWindowFont(hWndEditPinCode); SendMessage(hWndEditPinCode, EM_SETLIMITTEXT, 8, 0);

    hWndLblDiscTimeout = CreateWindowExA(0, "STATIC", "Discovery Timeout (sec):", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); ApplyWindowFont(hWndLblDiscTimeout);
    hWndEditDiscTimeout = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL | ES_NUMBER, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_DISC_TIMEOUT, hInstance, NULL); ApplyWindowFont(hWndEditDiscTimeout); SendMessage(hWndEditDiscTimeout, EM_SETLIMITTEXT, 4, 0);

    hWndLblMulticast = CreateWindowExA(0, "STATIC", "Multicast Address:", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); ApplyWindowFont(hWndLblMulticast);
    hWndEditMulticast = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_MULTICAST, hInstance, NULL); ApplyWindowFont(hWndEditMulticast); SendMessage(hWndEditMulticast, EM_SETLIMITTEXT, 63, 0);

    hWndCheckEncryption = CreateWindowExA(0, "BUTTON", "Enable Encryption (TLS)", WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_ENCRYPTION, hInstance, NULL); ApplyWindowFont(hWndCheckEncryption);

    hWndLblDevType = CreateWindowExA(0, "STATIC", "Device Type:", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); ApplyWindowFont(hWndLblDevType);
    hWndComboDevType = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_DEV_TYPE, hInstance, NULL); ApplyWindowFont(hWndComboDevType);

    hWndLblDevModel = CreateWindowExA(0, "STATIC", "Device Model:", WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); ApplyWindowFont(hWndLblDevModel);
    hWndEditDevModel = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_DEV_MODEL, hInstance, NULL); ApplyWindowFont(hWndEditDevModel); SendMessage(hWndEditDevModel, EM_SETLIMITTEXT, 63, 0);

    hWndLblPort = CreateWindowExA(0, "STATIC", g_Lang.port, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); ApplyWindowFont(hWndLblPort);
    hWndEditPort = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | ES_AUTOHSCROLL | ES_NUMBER, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_PORT, hInstance, NULL); ApplyWindowFont(hWndEditPort); SendMessage(hWndEditPort, EM_SETLIMITTEXT, 5, 0);

    hWndIconStatic = CreateWindowExA(0, "STATIC", "", WS_CHILD | SS_ICON, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL);
    HICON hExeIcon = (HICON)LoadImageA(hInstance, MAKEINTRESOURCE(101), IMAGE_ICON, 48, 48, LR_SHARED);
    if (hExeIcon) SendMessage(hWndIconStatic, STM_SETICON, (WPARAM)hExeIcon, 0); else SendMessage(hWndIconStatic, STM_SETICON, (WPARAM)LoadIcon(NULL, IDI_APPLICATION), 0);
    char aboutTxt[512]; _snprintf(aboutTxt, sizeof(aboutTxt), "%s\r\nVersion: 1.1.0\r\nPublisher: Fratta\r\n\r\n%s", APP_NAME, APP_ABOUT);
    hWndLabelAbout = CreateWindowExA(0, "STATIC", aboutTxt, WS_CHILD | SS_LEFT, 0, 0, 0, 0, hWnd, NULL, hInstance, NULL); ApplyWindowFont(hWndLabelAbout);
    hWndBtnGithub = CreateWindowExA(0, "BUTTON", g_Lang.aboutBtn, WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_BTN_GH, hInstance, NULL); ApplyWindowFont(hWndBtnGithub);
    hWndBtnSupport = CreateWindowExA(0, "BUTTON", g_Lang.supportBtn, WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_SET_BTN_SUPP, hInstance, NULL); ApplyWindowFont(hWndBtnSupport);

    LoadSettings();
    UpdateUITexts();
    if (hWndComboLanguage) SendMessage(hWndComboLanguage, CB_SETCURSEL, g_Language, 0);
    UpdatePageVisibility();
}
