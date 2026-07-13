#include "utils.h"
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include "dialogs.h"

#define IDC_TEXT_DLG_EDIT     7001
#define IDC_MANUAL_DLG_EDIT   7002
#define IDC_MANUAL_RAD_IP     7003
#define IDC_MANUAL_RAD_HASH   7004

extern HIMAGELIST hSystemImageList;
extern int GetSystemIconIndex(const char* fileName);
extern void ApplyWindowFont(HWND hWndChild);

// Dialog proc for typing/pasting a custom text message to send
INT_PTR CALLBACK TextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            SetWindowTextA(hDlg, g_Lang.insertTextMessage); RECT rc; GetClientRect(hDlg, &rc);
            HWND hWndEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL, 10, 10, rc.right - 20, rc.bottom - 45, hDlg, (HMENU)IDC_TEXT_DLG_EDIT, GetModuleHandle(NULL), NULL);
            HWND hWndOK = CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, (rc.right / 2) - 80, rc.bottom - 30, 75, 24, hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            HWND hWndCancel = CreateWindowExA(0, "BUTTON", g_Lang.cancelBtn, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, (rc.right / 2) + 5, rc.bottom - 30, 75, 24, hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
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

// Dialog proc for the manual send dialog where users input IP or hashtag
INT_PTR CALLBACK ManualSendDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            SetWindowTextA(hDlg, g_Lang.sendManually); RECT rc; GetClientRect(hDlg, &rc);
            HWND hRadIP = CreateWindowExA(0, "BUTTON", g_Lang.ipAddress, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 10, 10, 90, 20, hDlg, (HMENU)IDC_MANUAL_RAD_IP, GetModuleHandle(NULL), NULL);
            HWND hRadHash = CreateWindowExA(0, "BUTTON", g_Lang.hashtag, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 110, 10, 90, 20, hDlg, (HMENU)IDC_MANUAL_RAD_HASH, GetModuleHandle(NULL), NULL);
            HWND hWndEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 10, 35, rc.right - 20, 24, hDlg, (HMENU)IDC_MANUAL_DLG_EDIT, GetModuleHandle(NULL), NULL);
            HWND hWndOK = CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, (rc.right / 2) - 80, rc.bottom - 30, 75, 24, hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            HWND hWndCancel = CreateWindowExA(0, "BUTTON", g_Lang.cancelBtn, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, (rc.right / 2) + 5, rc.bottom - 30, 75, 24, hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
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

// Dialog proc that displays the PIN code entry dialog when a target requires one
INT_PTR CALLBACK PinPromptDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
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

// Wrapper function to spawn the PIN dialog in modal mode
bool ShowPinPrompt(HWND parent, const char* targetName, char* outPin) {
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

// WndProc callback for the custom transfer confirmation window
static LRESULT CALLBACK ConfirmationWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HWND hEditPath = NULL;
    static HWND hBtnBrowse = NULL;
    static HWND hListView = NULL;
    static HWND hBtnAccept = NULL;
    static HWND hBtnCancel = NULL;
    static HFONT hFont = NULL;
    static ConfirmationRequest* ctx = NULL;
    switch (message) {
        case WM_CREATE: {
            CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
            ctx = (ConfirmationRequest*)pcs->lpCreateParams;

            NONCLIENTMETRICSA ncm; ncm.cbSize = sizeof(NONCLIENTMETRICSA);
            SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), &ncm, 0);
            hFont = CreateFontA(ncm.lfMessageFont.lfHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            
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
            if (hFont) { DeleteObject(hFont); hFont = NULL; }
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Spawns the incoming file transfer confirmation window
bool ShowTransferConfirmation(HWND parent, const char* senderName, int fileCount, char* outSavePath) {
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
