#include "layout.h"
#include "utils.h"
#include <commctrl.h>
#include <stdio.h>

// UI control handles
extern HWND g_hWndMain;
extern HWND hWndTab, hWndSettingsTab;

// Receive Tab Controls
extern HWND hWndDeviceNameTitle, hWndDeviceName, hWndBtnEditDevice;
extern HWND hWndEditDeviceBox, hWndBtnSaveDevice, hWndBtnCancelDevice;
extern HWND hWndGroupBox, hWndInfoText;
extern HWND hWndStatusIcon, hWndStatus, hWndBtnCloseStatus;
extern HWND hWndListView;

// Send Tab Controls
extern HWND hWndBtnSndFile, hWndBtnSndFolder, hWndBtnSndText, hWndBtnSndPaste;
extern HWND hWndLblSendFiles, hWndListSendFiles;
extern HWND hWndSendProgress, hWndSendStatusIcon, hWndSendStatusTxt, hWndBtnCancelSend;
extern HWND hWndBtnCleanFiles, hWndBtnSendFiles;
extern HWND hWndLblDevices, hWndListDevices;
extern HWND hWndBtnSearchAgain, hWndSearchLoading, hWndBtnSendManual;

// Settings Sub-Tab: General
extern HWND hWndCheckSavePos, hWndCheckMinClose, hWndCheckTopmost, hWndCheckRecursiveFolder;
extern HWND hWndLblLanguage, hWndComboLanguage;

// Settings Sub-Tab: Receive
extern HWND hWndRecvLbl, hWndRecvRadApp, hWndRecvRadDl, hWndRecvRadCustom, hWndRecvTxtPath, hWndRecvBtnBrowse;
extern HWND hWndCheckQuickSave;

// Settings Sub-Tab: Network
extern HWND hWndCheckReqPin, hWndLblPinCode, hWndEditPinCode;
extern HWND hWndLblDiscTimeout, hWndEditDiscTimeout;
extern HWND hWndLblMulticast, hWndEditMulticast;
extern HWND hWndCheckEncryption;
extern HWND hWndLblDevType, hWndComboDevType;
extern HWND hWndLblDevModel, hWndEditDevModel;
extern HWND hWndLblPort, hWndEditPort;
extern HWND hWndLblReqPin;

// Settings Sub-Tab: About
extern HWND hWndIconStatic, hWndLabelAbout, hWndBtnGithub, hWndBtnSupport, hWndBtnHelp;

// State and configuration globals defined in main.c
extern int g_SplitterPos;
extern BOOL g_bEditingDeviceName;
extern char g_MyDeviceName[];

extern void UpdateSendButtonsState(void);

// Positions controls inside the "Receive" tab (Tab 0)
static void LayoutReceiveTab(int sxWidth, int dxLeft, int dxWidth, int height) {
    // Top device name bar
    MoveWindow(hWndDeviceNameTitle, 20, 38, sxWidth - 20, 15, TRUE);
    if (!g_bEditingDeviceName) {
        // Show static display of device name
        MoveWindow(hWndDeviceName, 20, 54, sxWidth - 95, 30, TRUE);
        MoveWindow(hWndBtnEditDevice, sxWidth - 70, 53, 60, 24, TRUE);
    } else {
        // Show edit box to rename device
        MoveWindow(hWndEditDeviceBox, 20, 54, sxWidth - 155, 24, TRUE);
        MoveWindow(hWndBtnSaveDevice, sxWidth - 130, 53, 55, 24, TRUE);
        MoveWindow(hWndBtnCancelDevice, sxWidth - 70, 53, 60, 24, TRUE);
    }

    // Network connection metadata groupbox
    MoveWindow(hWndGroupBox, 15, 95, sxWidth - 10, 105, TRUE);
    MoveWindow(hWndInfoText, 30, 115, sxWidth - 40, 75, TRUE);

    // Bottom-left status icon (waiting, receiving, success)
    MoveWindow(hWndStatusIcon, 15, height - 35, 16, 16, TRUE);
    if (IsWindowVisible(hWndBtnCloseStatus)) {
        // Keep space for the "Close/Clear" button
        MoveWindow(hWndStatus, 38, height - 37, sxWidth - 115, 22, TRUE);
        MoveWindow(hWndBtnCloseStatus, sxWidth - 75, height - 39, 65, 24, TRUE);
    } else {
        MoveWindow(hWndStatus, 38, height - 37, sxWidth - 40, 22, TRUE);
    }

    // Main files list view (right side of splitter)
    MoveWindow(hWndListView, dxLeft, 35, dxWidth, height - 50, TRUE);
}

// Positions controls inside the "Send" tab (Tab 1)
static void LayoutSendTab(int sxWidth, int dxLeft, int dxWidth, int height) {
    // Add buttons row (File, Folder, Text, Paste)
    int btnWidth = (sxWidth - 25) / 4;
    MoveWindow(hWndBtnSndFile, 15, 35, btnWidth, 35, TRUE);
    MoveWindow(hWndBtnSndFolder, 15 + btnWidth + 5, 35, btnWidth, 35, TRUE);
    MoveWindow(hWndBtnSndText, 15 + btnWidth*2 + 10, 35, btnWidth, 35, TRUE);
    MoveWindow(hWndBtnSndPaste, 15 + btnWidth*3 + 15, 35, btnWidth, 35, TRUE);

    // Queue section header and items list view
    MoveWindow(hWndLblSendFiles, 15, 80, sxWidth - 10, 15, TRUE);
    MoveWindow(hWndListSendFiles, 15, 100, sxWidth - 10, height - 195, TRUE);

    // Progress bar and transfer status indicator (bottom-left)
    MoveWindow(hWndSendProgress, 15, height - 88, sxWidth - 10, 14, TRUE);
    MoveWindow(hWndSendStatusIcon, 15, height - 66, 16, 16, TRUE);
    if (IsWindowVisible(hWndBtnCancelSend)) {
        MoveWindow(hWndSendStatusTxt, 38, height - 68, sxWidth - 110, 32, TRUE);
        MoveWindow(hWndBtnCancelSend, sxWidth - 70, height - 68, 60, 24, TRUE);
    } else {
        MoveWindow(hWndSendStatusTxt, 38, height - 68, sxWidth - 33, 32, TRUE);
    }

    // Clean / Send action buttons row
    int botBtnWidthL = (sxWidth - 15) / 2;
    MoveWindow(hWndBtnCleanFiles, 15, height - 35, botBtnWidthL, 24, TRUE);
    MoveWindow(hWndBtnSendFiles, 15 + botBtnWidthL + 5, height - 35, botBtnWidthL, 24, TRUE);

    // Nearby discovered devices panel (right side of splitter)
    MoveWindow(hWndLblDevices, dxLeft, 35, dxWidth, 15, TRUE);
    MoveWindow(hWndListDevices, dxLeft, 55, dxWidth, height - 100, TRUE);

    // Search and Manual Send row at bottom-right
    int botBtnWidthR = (dxWidth - 5) / 2;
    if (IsWindowVisible(hWndSearchLoading)) {
        MoveWindow(hWndSearchLoading, dxLeft, height - 30, 16, 16, TRUE);
        MoveWindow(hWndBtnSearchAgain, dxLeft + 22, height - 35, botBtnWidthR - 22, 24, TRUE);
    } else {
        MoveWindow(hWndBtnSearchAgain, dxLeft, height - 35, botBtnWidthR, 24, TRUE);
    }
    MoveWindow(hWndBtnSendManual, dxLeft + botBtnWidthR + 5, height - 35, botBtnWidthR, 24, TRUE);
}

// Positions controls inside the "Settings" subpages (Tab 2)
static void LayoutSettingsTab(int baseX, int baseY, int subW) {
    // General Settings Subpage
    MoveWindow(hWndCheckSavePos, baseX + 15, baseY + 15, subW - 30, 20, TRUE);
    MoveWindow(hWndCheckMinClose, baseX + 15, baseY + 45, subW - 30, 20, TRUE);
    MoveWindow(hWndCheckTopmost, baseX + 15, baseY + 75, subW - 30, 20, TRUE);
    MoveWindow(hWndCheckRecursiveFolder, baseX + 15, baseY + 105, subW - 30, 20, TRUE);
    MoveWindow(hWndLblLanguage, baseX + 15, baseY + 140, 100, 20, TRUE);
    MoveWindow(hWndComboLanguage, baseX + 120, baseY + 138, 150, 150, TRUE);
    
    // Receive Settings Subpage
    MoveWindow(hWndRecvLbl, baseX + 15, baseY + 15, subW - 30, 20, TRUE);
    MoveWindow(hWndRecvRadApp, baseX + 15, baseY + 40, subW - 30, 20, TRUE);
    MoveWindow(hWndRecvRadDl, baseX + 15, baseY + 65, subW - 30, 20, TRUE);
    MoveWindow(hWndRecvRadCustom, baseX + 15, baseY + 90, subW - 30, 20, TRUE);
    MoveWindow(hWndRecvTxtPath, baseX + 35, baseY + 115, subW - 120, 24, TRUE);
    MoveWindow(hWndRecvBtnBrowse, baseX + subW - 80, baseY + 114, 65, 26, TRUE);
    MoveWindow(hWndCheckQuickSave, baseX + 15, baseY + 150, subW - 30, 20, TRUE);
    
    // Network Settings Subpage
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

    // 4. About Subpage
    int btnW = (subW - 50) / 3;
    if (btnW < 70) btnW = 70;
    MoveWindow(hWndIconStatic, baseX + 15, baseY + 15, 48, 48, TRUE);
    MoveWindow(hWndLabelAbout, baseX + 80, baseY + 15, subW - 95, 100, TRUE);
    MoveWindow(hWndBtnHelp, baseX + 15, baseY + 125, btnW, 30, TRUE);
    MoveWindow(hWndBtnGithub, baseX + 15 + btnW + 10, baseY + 125, btnW, 30, TRUE);
    MoveWindow(hWndBtnSupport, baseX + 15 + (btnW + 10)*2, baseY + 125, btnW, 30, TRUE);
}

// Toggles visibility of settings subpages based on sub-tab index
void ShowSettingsSubPage(int subTab) {
    int hide = SW_HIDE;
    
    // Default: Hide all settings components across all sub-tabs
    ShowWindow(hWndCheckSavePos, hide); ShowWindow(hWndCheckMinClose, hide); ShowWindow(hWndCheckTopmost, hide); ShowWindow(hWndCheckRecursiveFolder, hide);
    ShowWindow(hWndLblLanguage, hide); ShowWindow(hWndComboLanguage, hide);
    ShowWindow(hWndRecvLbl, hide); ShowWindow(hWndRecvRadApp, hide); ShowWindow(hWndRecvRadDl, hide); ShowWindow(hWndRecvRadCustom, hide); ShowWindow(hWndRecvTxtPath, hide); ShowWindow(hWndRecvBtnBrowse, hide); ShowWindow(hWndCheckQuickSave, hide);
    
    ShowWindow(hWndCheckReqPin, hide); ShowWindow(hWndEditPinCode, hide);
    ShowWindow(hWndEditDiscTimeout, hide); ShowWindow(hWndEditMulticast, hide);
    ShowWindow(hWndCheckEncryption, hide); ShowWindow(hWndComboDevType, hide);
    ShowWindow(hWndEditDevModel, hide); ShowWindow(hWndEditPort, hide);
    ShowWindow(hWndLblReqPin, hide); ShowWindow(hWndLblPinCode, hide);
    ShowWindow(hWndLblDiscTimeout, hide); ShowWindow(hWndLblMulticast, hide);
    ShowWindow(hWndLblDevType, hide); ShowWindow(hWndLblDevModel, hide); ShowWindow(hWndLblPort, hide);

    ShowWindow(hWndIconStatic, hide); ShowWindow(hWndLabelAbout, hide); ShowWindow(hWndBtnGithub, hide); ShowWindow(hWndBtnSupport, hide); ShowWindow(hWndBtnHelp, hide);

    // Verify if we're in tab 2
    if (TabCtrl_GetCurSel(hWndTab) != 2) return;

    // Selectively display components corresponding to active sub-tab
    if (subTab == 0) { // General Sub-Tab
        ShowWindow(hWndCheckSavePos, SW_SHOW); ShowWindow(hWndCheckMinClose, SW_SHOW); ShowWindow(hWndCheckTopmost, SW_SHOW); ShowWindow(hWndCheckRecursiveFolder, SW_SHOW);
        ShowWindow(hWndLblLanguage, SW_SHOW); ShowWindow(hWndComboLanguage, SW_SHOW);
    } else if (subTab == 1) { // Receive Path Sub-Tab
        ShowWindow(hWndRecvLbl, SW_SHOW); ShowWindow(hWndRecvRadApp, SW_SHOW); ShowWindow(hWndRecvRadDl, SW_SHOW); ShowWindow(hWndRecvRadCustom, SW_SHOW);
        ShowWindow(hWndRecvTxtPath, SW_SHOW); ShowWindow(hWndRecvBtnBrowse, SW_SHOW); ShowWindow(hWndCheckQuickSave, SW_SHOW);
    } else if (subTab == 2) { // Advanced Network Sub-Tab
        ShowWindow(hWndCheckReqPin, SW_SHOW); ShowWindow(hWndEditPinCode, SW_SHOW);
        ShowWindow(hWndEditDiscTimeout, SW_SHOW); ShowWindow(hWndEditMulticast, SW_SHOW);
        ShowWindow(hWndCheckEncryption, SW_SHOW); ShowWindow(hWndComboDevType, SW_SHOW);
        ShowWindow(hWndEditDevModel, SW_SHOW); ShowWindow(hWndEditPort, SW_SHOW);
        ShowWindow(hWndLblPinCode, SW_SHOW); ShowWindow(hWndLblDiscTimeout, SW_SHOW); 
        ShowWindow(hWndLblMulticast, SW_SHOW); ShowWindow(hWndLblDevType, SW_SHOW); 
        ShowWindow(hWndLblDevModel, SW_SHOW); ShowWindow(hWndLblPort, SW_SHOW);
    } else if (subTab == 3) { // About Sub-Tab
        ShowWindow(hWndIconStatic, SW_SHOW); ShowWindow(hWndLabelAbout, SW_SHOW); ShowWindow(hWndBtnHelp, SW_SHOW); ShowWindow(hWndBtnGithub, SW_SHOW); ShowWindow(hWndBtnSupport, SW_SHOW);
    }
}

// Shows or hides page layouts depending on current main tab select
void UpdatePageVisibility() {
    int mainTab = TabCtrl_GetCurSel(hWndTab);

    // Hide all receive controls first
    ShowWindow(hWndGroupBox, SW_HIDE);
    ShowWindow(hWndInfoText, SW_HIDE);
    ShowWindow(hWndStatus, SW_HIDE);
    ShowWindow(hWndListView, SW_HIDE);
    ShowWindow(hWndStatusIcon, SW_HIDE);
    ShowWindow(hWndDeviceNameTitle, SW_HIDE);
    ShowWindow(hWndBtnCloseStatus, SW_HIDE);
    ShowWindow(hWndDeviceName, SW_HIDE);
    ShowWindow(hWndBtnEditDevice, SW_HIDE);
    ShowWindow(hWndEditDeviceBox, SW_HIDE);
    ShowWindow(hWndBtnSaveDevice, SW_HIDE);
    ShowWindow(hWndBtnCancelDevice, SW_HIDE);

    // Hide all send controls
    ShowWindow(hWndBtnSndFile, SW_HIDE);
    ShowWindow(hWndBtnSndFolder, SW_HIDE);
    ShowWindow(hWndBtnSndText, SW_HIDE);
    ShowWindow(hWndBtnSndPaste, SW_HIDE);
    ShowWindow(hWndLblSendFiles, SW_HIDE);
    ShowWindow(hWndListSendFiles, SW_HIDE);
    ShowWindow(hWndBtnCleanFiles, SW_HIDE);
    ShowWindow(hWndBtnSendFiles, SW_HIDE);
    ShowWindow(hWndLblDevices, SW_HIDE);
    ShowWindow(hWndListDevices, SW_HIDE);
    ShowWindow(hWndBtnSearchAgain, SW_HIDE);
    ShowWindow(hWndBtnSendManual, SW_HIDE);
    ShowWindow(hWndSearchLoading, SW_HIDE);
    ShowWindow(hWndSendProgress, SW_HIDE);
    ShowWindow(hWndSendStatusTxt, SW_HIDE);
    ShowWindow(hWndSendStatusIcon, SW_HIDE);
    ShowWindow(hWndBtnCancelSend, SW_HIDE);

    // Hide all settings controls
    ShowWindow(hWndSettingsTab, SW_HIDE);
    ShowSettingsSubPage(-1);

    // Redraw main background to clear remnants of hidden controls
    InvalidateRect(g_hWndMain, NULL, TRUE);
    UpdateWindow(g_hWndMain);

    // Tab 0 = Receive, Tab 1 = Send, Tab 2 = Settings
    if (mainTab == 0) {
        // Show receive controls
        ShowWindow(hWndInfoText, SW_SHOW);
        ShowWindow(hWndStatus, SW_SHOW);
        ShowWindow(hWndListView, SW_SHOW);
        ShowWindow(hWndStatusIcon, SW_SHOW);
        ShowWindow(hWndDeviceNameTitle, SW_SHOW);
        ShowWindow(hWndGroupBox, SW_SHOW);

        if (IsWindowVisible(hWndBtnCloseStatus)) ShowWindow(hWndBtnCloseStatus, SW_SHOW);
        if (g_bEditingDeviceName) {
            ShowWindow(hWndEditDeviceBox, SW_SHOW);
            ShowWindow(hWndBtnSaveDevice, SW_SHOW);
            ShowWindow(hWndBtnCancelDevice, SW_SHOW);
        } else {
            ShowWindow(hWndDeviceName, SW_SHOW);
            ShowWindow(hWndBtnEditDevice, SW_SHOW);
        }
    } else if (mainTab == 1) {
        // Show send controls
        ShowWindow(hWndBtnSndFile, SW_SHOW);
        ShowWindow(hWndBtnSndFolder, SW_SHOW);
        ShowWindow(hWndBtnSndText, SW_SHOW);
        ShowWindow(hWndBtnSndPaste, SW_SHOW);
        ShowWindow(hWndLblSendFiles, SW_SHOW);
        ShowWindow(hWndListSendFiles, SW_SHOW);
        ShowWindow(hWndBtnCleanFiles, SW_SHOW);
        ShowWindow(hWndBtnSendFiles, SW_SHOW);
        ShowWindow(hWndLblDevices, SW_SHOW);
        ShowWindow(hWndListDevices, SW_SHOW);
        ShowWindow(hWndBtnSearchAgain, SW_SHOW);
        ShowWindow(hWndBtnSendManual, SW_SHOW);
        UpdateSendButtonsState();
    } else if (mainTab == 2) {
        // Show settings controls
        ShowWindow(hWndSettingsTab, SW_SHOW);
        ShowSettingsSubPage(TabCtrl_GetCurSel(hWndSettingsTab));
    }
}

// Handles positioning and layout reflow during window resizing
void ResizeControls(HWND hWnd, int width, int height) {
    if (!hWndTab) return;
    MoveWindow(hWndTab, 5, 5, width - 10, height - 10, TRUE);
    int mainTab = TabCtrl_GetCurSel(hWndTab);
    int sxWidth = g_SplitterPos; int dxLeft = g_SplitterPos + 10; int dxWidth = (width - 20) - dxLeft;

    if (mainTab == 0) {
        LayoutReceiveTab(sxWidth, dxLeft, dxWidth, height);
    }
    else if (mainTab == 1) {
        LayoutSendTab(sxWidth, dxLeft, dxWidth, height);
    }
    else if (mainTab == 2) {
        // Adjust nested Settings Sub-Tab boundaries inside main Tab boundaries
        RECT rcTab; GetClientRect(hWndTab, &rcTab); TabCtrl_AdjustRect(hWndTab, FALSE, &rcTab);
        MoveWindow(hWndSettingsTab, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, TRUE);

        RECT rcSubTab; rcSubTab.left = 0; rcSubTab.top = 0; rcSubTab.right = rcTab.right - rcTab.left; rcSubTab.bottom = rcTab.bottom - rcTab.top;
        TabCtrl_AdjustRect(hWndSettingsTab, FALSE, &rcSubTab);

        int baseX = rcTab.left + rcSubTab.left; int baseY = rcTab.top + rcSubTab.top; int subW = rcSubTab.right - rcSubTab.left;

        LayoutSettingsTab(baseX, baseY, subW);
    }
    
    // Keep tab controls always at the bottom of Z-order to prevent overlapping text labels
    if (hWndTab) SetWindowPos(hWndTab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    if (hWndSettingsTab) SetWindowPos(hWndSettingsTab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
