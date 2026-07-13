#ifndef DIALOGS_H
#define DIALOGS_H

#include <windows.h>
#include <commctrl.h>

INT_PTR CALLBACK TextDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ManualSendDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PinPromptDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
bool ShowPinPrompt(HWND parent, const char* targetName, char* outPin);
bool ShowTransferConfirmation(HWND parent, const char* senderName, int fileCount, char* outSavePath);

#endif
