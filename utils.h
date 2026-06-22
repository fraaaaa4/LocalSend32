#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>

#define WM_TRAYICON_MSG (WM_USER + 100)
#define WM_FILE_START (WM_USER + 101)
#define WM_FILE_PROGRESS (WM_USER + 102)
#define WM_FILE_COMPLETE (WM_USER + 103)
#define WM_FILE_CANCEL (WM_USER + 104)
#define WM_DEVICE_DISCOVERED (WM_USER + 105)
#define WM_SEND_STATUS_UPDATE (WM_USER + 106)
#define WM_SEND_DONE (WM_USER + 107)
#define WM_CONFIRM_TRANSFER (WM_USER + 110)
#define WM_REQUEST_PIN (WM_USER + 111)

#define LV_COL_NAME    0
#define LV_COL_SIZE    1
#define LV_COL_PROGRESS 2
#define LV_COL_DATE 3

typedef enum { STATE_TRANSFERRING = 0, STATE_COMPLETED, STATE_CANCELED } TransferState;

typedef struct {
    char fileName[256]; char fileId[128]; long long fileSize;
    int percentuale; TransferState stato; char timestamp[32];
} LoggedTransfer;

#define MAX_QUEUE_FILES 20
typedef struct {
    char fileId[128];
    char fileName[256];
    long long fileSize;
} PreparedFile;

extern PreparedFile g_fileQueue[MAX_QUEUE_FILES];
extern int g_fileQueueCount;

typedef struct {
    char fileName[256]; char fileId[128]; char senderName[128]; long long fileSize;
} FileStartInfo;

typedef struct { int pct; char fileId[128]; } FileProgressInfo;

typedef struct {
    const char* senderName;
    int fileCount;
    char selectedPath[MAX_PATH];
    bool accepted;
    HANDLE hEvent;
} ConfirmationRequest;
typedef struct {
    const char* targetName;
    char pinCode[16];
    bool success;
    HANDLE hEvent;
} PinRequest;

typedef struct {
    char alias[64]; char version[16]; char deviceModel[64]; char deviceType[32];
    char fingerprint[64]; int port; bool announce; char ipAddress[16];
} RemoteDevice;

extern HWND g_hWndMain;
extern HWND hWndStatus;

// Global settings and dynamic hashtag variables
extern int g_SaveMode;
extern int g_QuickSave;
extern char g_CustomPath[MAX_PATH];
extern char g_SessionSavePath[MAX_PATH];
extern char g_MyFingerprint[64];

extern int g_RequirePin;
extern char g_PinCode[16];
extern int g_DiscoveryTimeout;
extern char g_MulticastAddr[64];
extern int g_EnableEncryption;
extern char g_DeviceType[32];
extern char g_DeviceModel[64];
extern int g_Port;

bool initWinsock();
bool checkRulesExistence();
void autoFirewall();
bool parseLocalSendJSON(const char *json, RemoteDevice *outDevice);
void cleanQuotes(char *dest, const char *src, size_t maxLen);

// Calculates where to save received files depending on configuration options
void GetSaveDirectory(char* outDir, size_t maxLen);
void GetConfiguredSavePath(char* outPath, size_t maxLen, const char* fileName);

#endif

