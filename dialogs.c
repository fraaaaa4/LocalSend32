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

// Dialog for typing a custom text message to send
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

// Helper to validate IPv4 format
static bool IsValidIPv4String(const char* str) {
    if (!str || !*str) return false;
    int a, b, c, d;
    char extra = '\0';
    if (sscanf(str, "%d.%d.%d.%d%c", &a, &b, &c, &d, &extra) != 4) return false;
    if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255) return false;
    for (const char* p = str; *p; p++) {
        if (!isdigit((unsigned char)*p) && *p != '.') return false;
    }
    return true;
}

// Dialog for the manual send dialog where users input IP or hashtag
INT_PTR CALLBACK ManualSendDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        // Initialize the controls for entering IP/Hashtag target manually
        case WM_INITDIALOG: {
            SetWindowTextA(hDlg, g_Lang.sendManually); RECT rc; GetClientRect(hDlg, &rc);
            HWND hRadIP = CreateWindowExA(0, "BUTTON", g_Lang.ipAddress, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, 10, 8, 85, 18, hDlg, (HMENU)IDC_MANUAL_RAD_IP, GetModuleHandle(NULL), NULL);
            HWND hRadHash = CreateWindowExA(0, "BUTTON", g_Lang.hashtag, WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 100, 8, 85, 18, hDlg, (HMENU)IDC_MANUAL_RAD_HASH, GetModuleHandle(NULL), NULL);
            HWND hWndEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 10, 30, rc.right - 20, 22, hDlg, (HMENU)IDC_MANUAL_DLG_EDIT, GetModuleHandle(NULL), NULL);
            HWND hWndOK = CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, (rc.right / 2) - 68, rc.bottom - 26, 62, 22, hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            HWND hWndCancel = CreateWindowExA(0, "BUTTON", g_Lang.cancelBtn, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, (rc.right / 2) + 6, rc.bottom - 26, 62, 22, hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
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
                    if (isHash) {
                        const char* p = (buf[0] == '#') ? (buf + 1) : buf;
                        if (strlen(p) == 0) {
                            MessageBoxA(hDlg, g_Lang.errInvalidHashtag, g_Lang.titleError, MB_ICONWARNING);
                            SetFocus(hWndEdit); free(buf); return TRUE;
                        }
                        // Add hashtag prefix if user chose the hashtag option and forgot to type it
                        if (buf[0] != '#') { memmove(buf + 1, buf, len + 1); buf[0] = '#'; }
                    } else {
                        if (!IsValidIPv4String(buf)) {
                            MessageBoxA(hDlg, g_Lang.errInvalidIp, g_Lang.titleError, MB_ICONWARNING);
                            SetFocus(hWndEdit);
                            SendMessage(hWndEdit, EM_SETSEL, 0, -1);
                            free(buf);
                            return TRUE;
                        }
                    }
                    EndDialog(hDlg, (INT_PTR)buf);
                } else {
                    BOOL isHash = (SendMessage(GetDlgItem(hDlg, IDC_MANUAL_RAD_HASH), BM_GETCHECK, 0, 0) == BST_CHECKED);
                    MessageBoxA(hDlg, isHash ? g_Lang.errInvalidHashtag : g_Lang.errInvalidIp, g_Lang.titleError, MB_ICONWARNING);
                    SetFocus(GetDlgItem(hDlg, IDC_MANUAL_DLG_EDIT));
                }
                return TRUE;
            } else if (wmId == IDCANCEL) { EndDialog(hDlg, 0); return TRUE; } break;
        }
    } return FALSE;
}

// Dialog that displays the PIN code entry dialog when a target requires one
INT_PTR CALLBACK PinPromptDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static char* outPin = NULL;
    switch (message) {
        case WM_INITDIALOG: {
            outPin = (char*)lParam;
            SetWindowTextA(hDlg, g_Lang.titleEnterPin);
            RECT rc; GetClientRect(hDlg, &rc);
            HWND hLbl = CreateWindowExA(0, "STATIC", g_Lang.lblPinRequired, WS_CHILD | WS_VISIBLE | SS_LEFT, 15, 12, rc.right - 30, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
            HWND hEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | ES_CENTER, (rc.right - 100) / 2, 38, 100, 24, hDlg, (HMENU)7010, GetModuleHandle(NULL), NULL);
            HWND hBtnOK = CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, (rc.right / 2) - 80, rc.bottom - 32, 75, 24, hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            HWND hBtnCancel = CreateWindowExA(0, "BUTTON", g_Lang.cancelBtn, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, (rc.right / 2) + 5, rc.bottom - 32, 75, 24, hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
            SendMessage(hEdit, EM_SETLIMITTEXT, 8, 0);
            ApplyWindowFont(hLbl);
            ApplyWindowFont(hEdit);
            ApplyWindowFont(hBtnOK);
            ApplyWindowFont(hBtnCancel);
            SetFocus(hEdit);
            return FALSE;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDOK) {
                HWND hEdit = GetDlgItem(hDlg, 7010);
                GetWindowTextA(hEdit, outPin, 15);
                EndDialog(hDlg, 1);
                return TRUE;
            } else if (wmId == IDCANCEL) {
                EndDialog(hDlg, 0);
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

// Spawn the PIN dialog
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

// Transfer confirmation window
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
            
            char promptText[512];
            _snprintf(promptText, sizeof(promptText), "%s %s", ctx->senderName, g_Lang.lblWantsToSend);
            HWND hLbl1 = CreateWindowExA(0, "STATIC", promptText, WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 15, 440, 18, hWnd, NULL, GetModuleHandle(NULL), NULL);
            ApplyWindowFont(hLbl1);

            _snprintf(promptText, sizeof(promptText), g_Lang.lblFilesCount, ctx->fileCount);
            HWND hLbl2 = CreateWindowExA(0, "STATIC", promptText, WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 35, 440, 18, hWnd, NULL, GetModuleHandle(NULL), NULL);
            ApplyWindowFont(hLbl2);

            HWND hLbl3 = CreateWindowExA(0, "STATIC", g_Lang.lblSaveTo, WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 60, 70, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);
            ApplyWindowFont(hLbl3);

            hEditPath = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", ctx->selectedPath, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 95, 58, 265, 22, hWnd, NULL, GetModuleHandle(NULL), NULL);
            ApplyWindowFont(hEditPath);

            hBtnBrowse = CreateWindowExA(0, "BUTTON", g_Lang.btnBrowse, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 370, 57, 90, 24, hWnd, (HMENU)1001, GetModuleHandle(NULL), NULL);
            ApplyWindowFont(hBtnBrowse);

            hListView = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL, 20, 90, 440, 215, hWnd, NULL, GetModuleHandle(NULL), NULL);
            ApplyWindowFont(hListView);
            ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);
            if (hSystemImageList == NULL && ctx->fileCount > 0) {
                GetSystemIconIndex(g_fileQueue[0].fileName);
            }
            if (hSystemImageList) {
                ListView_SetImageList(hListView, hSystemImageList, LVSIL_SMALL);
            }

            // Insert file headers and rows into preview ListView
            LVCOLUMNA lvc;
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = g_Lang.lblFileHeader;
            lvc.cx = 300;
            ListView_InsertColumn(hListView, 0, &lvc);
            lvc.pszText = g_Lang.lblSizeHeader;
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
                
                // Format file sizes dynamically in MB, KB, or bytes
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

            hBtnAccept = CreateWindowExA(0, "BUTTON", g_Lang.btnAccept, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 255, 320, 100, 28, hWnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            ApplyWindowFont(hBtnAccept);

            hBtnCancel = CreateWindowExA(0, "BUTTON", g_Lang.cancelBtn, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 365, 320, 95, 28, hWnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
            ApplyWindowFont(hBtnCancel);

            break;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            // Handle folder browse button click to change destination folder
            if (wmId == 1001) {
                BROWSEINFOA bi = {0};
                bi.hwndOwner = hWnd;
                bi.lpszTitle = g_Lang.lblSelectSaveDir;
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
                // User accepted the transfer: save selected path and dismiss dialog
                GetWindowTextA(hEditPath, ctx->selectedPath, MAX_PATH);
                ctx->accepted = true;
                DestroyWindow(hWnd);
            } else if (wmId == IDCANCEL) {
                // User declined the transfer
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

// Incoming file transfer confirmation window
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

    // Calculate center coordinates relative to active screen resolution
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int w = 490;
    int h = 410;
    int x = (screenW - w) / 2;
    int y = (screenH - h) / 2;

    // Temporarily disable parent window to create modal behavior
    if (parent) EnableWindow(parent, FALSE);

    HWND hWnd = CreateWindowExA(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, "LocalSendConfirmClass", g_Lang.titleIncomingTransfer, 
                                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_DLGFRAME,
                                x, y, w, h, parent, NULL, GetModuleHandle(NULL), &ctx);
    
    if (!hWnd) {
        if (parent) EnableWindow(parent, TRUE);
        return false;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Modal message loop until confirmation window is dismissed
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
