#ifndef TRAY_MENU_H
#define TRAY_MENU_H

#include <windows.h>
#include <shellapi.h>
#include <stdbool.h>

#define IDM_TRAY_OPEN          8001
#define IDM_TRAY_SETTINGS      8002
#define IDM_TRAY_EXIT          8003
#define IDM_TRAY_CLEAR_FILES   8004
#define IDM_TRAY_DROP_HERE     8005
#define IDM_TRAY_SCAN_AGAIN    8006
#define IDM_TRAY_ADD_FILES     8007
#define IDM_TRAY_ADD_FOLDER    8008
#define IDM_TRAY_PASTE         8009
#define IDM_TRAY_DROP_ZONE     8010
#define IDM_TRAY_DEVICE_START  8100
#define IDM_TRAY_DEVICE_MAX    8199

// Initializes the taskbar notification tray icon
void InitTrayIcon(HWND hWnd, HINSTANCE hInstance);

// Removes the taskbar notification tray icon on application exit
void RemoveTrayIcon(HWND hWnd);

// Displays a balloon notification popup from the system tray icon
void ShowTrayNotification(HWND hWnd, const char* title, const char* message, DWORD infoFlags);

// Constructs and opens the right-click context menu for the tray icon
void ShowTrayContextMenu(HWND hWnd);

// Handles command actions triggered from the tray menu
void HandleTrayCommand(HWND hWnd, int cmdId);

// Drop Zone Window functions
void InitDropZoneWindow(HINSTANCE hInstance, HWND hParent);
void ToggleDropZoneWindow(void);
void ShowDropZoneWindow(BOOL bShow);

#endif // TRAY_MENU_H
