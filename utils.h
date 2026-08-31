#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
#include <shlwapi.h>

#ifdef LOCALSEND_NT
    #define APP_NAME "LocalSend9x"
    #define APP_ABOUT "LocalSend client built with C/Win32 APIs. Made for Windows 95/98/NT/2000."
    #define APP_SUPPORT_BTN "About LocalSend9x"
    #define APP_SUPPORT_URL "https://github.com/fraaaaa4/LocalSend32"
#elif defined(__arm__)
    #define APP_NAME "LocalSend RT"
    #define APP_ABOUT "LocalSend client built with C/Win32 APIs. Made for Windows RT."
    #define APP_SUPPORT_BTN "About LocalSend RT"
    #define APP_SUPPORT_URL "https://github.com/fraaaaa4/LocalSend32"
#else
    #define APP_NAME "LocalSend32"
    #define APP_ABOUT "LocalSend client built with C/Win32 APIs. Made for Windows."
    #define APP_SUPPORT_BTN "About LocalSend32"
    #define APP_SUPPORT_URL "https://github.com/fraaaaa4/LocalSend32"
#endif

#define APP_VERSION "2.0.0"

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
#define IDC_BTN_CANCEL_SEND   4014

#define IDC_SET_SAVE_POS      5001
#define IDC_SET_MIN_CLOSE     5002
#define IDC_SET_TOPMOST       5003
#define IDC_SET_BTN_GH        5004
#define IDC_SET_BTN_SUPP      5005
#define IDC_SET_LANG_LABEL    5016
#define IDC_SET_LANG_COMBO    5017
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
#define IDC_SET_RECURSIVE_FOLDER 5021
#define IDC_SET_BTN_HELP      5022

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

typedef enum { ICON_WAITING = 0, ICON_TRANSFERRING = 1, ICON_SUCCESS = 2, ICON_CANCELED = 3 } StatusIconType;

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
#define WM_UPDATE_NET_INFO (WM_USER + 112)

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
    char fingerprint[128]; int port; bool announce; char ipAddress[16];
    bool isHttps;
} RemoteDevice;

extern HWND g_hWndMain;
extern HWND hWndStatus;

extern int g_SaveMode;
extern int g_QuickSave;
extern char g_CustomPath[MAX_PATH];
extern char g_SessionSavePath[MAX_PATH];
extern char g_MyFingerprint[128];

extern int g_RequirePin;
extern char g_PinCode[16];
extern int g_DiscoveryTimeout;
extern char g_MulticastAddr[64];
extern int g_EnableEncryption;
extern char g_DeviceType[32];
extern char g_DeviceModel[64];
extern int g_Port;
extern char g_MyDeviceName[];
extern char g_IniPath[MAX_PATH];
extern volatile bool g_bCancelSendSession;

void GenerateFallbackFingerprint(void);
bool initWinsock();
bool checkRulesExistence();
void autoFirewall();
bool parseLocalSendJSON(const char *json, RemoteDevice *outDevice);
void cleanQuotes(char *dest, const char *src, size_t maxLen);

typedef enum {
    LANG_EN = 0,
    LANG_IT,
    LANG_zh_CN,
    LANG_COUNT
} LanguageId;

typedef struct {
    char tabSend[64];
    char tabReceive[64];
    char tabSettings[64];
    char nearbyDevices[128];
    char sendFiles[64];
    char cleanFiles[64];
    char browse[64];
    char searchAgain[64];
    char sendManually[64];
    char settingsGeneral[64];
    char settingsReceive[64];
    char settingsNetwork[64];
    char settingsOther[64];
    char saveWindowPos[256];
    char minimizeToTray[256];
    char alwaysOnTop[256];
    char whenIReceive[256];
    char saveAppPath[256];
    char saveDownloads[256];
    char saveCustomPath[256];
    char quickSave[256];
    char requirePin[256];
    char pinCode[64];
    char port[64];
    char aboutBtn[64];
    char supportBtn[64];
    char language[64];
    char deviceName[64];
    char editBtn[64];
    char saveBtn[64];
    char cancelBtn[64];
    char networkInfo[64];
    char waitingForFiles[64];
    char closeBtn[64];
    char selectSaveFolder[128];
    char selectFolderToSend[128];
    char insertTextMessage[64];
    char ipAddress[64];
    char hashtag[64];
    char colFile[64];
    char colSize[64];
    char colDate[64];
    char colProgress[64];
    char colDevice[64];
    char colInfo[64];
    char grpToday[64];
    char grpOlder[64];
    char filesToSend[64];
    char addFilesTooltip[64];
    char addFolderTooltip[64];
    char sendTextTooltip[64];
    char pasteTooltip[64];
    char btnFile[64];
    char btnFolder[64];
    char btnText[64];
    char btnPaste[64];
    char errCryptoMissing[128];
    char errSslMissing[128];
    char errBothMissing[128];
    char titleEnterPin[64];
    char lblPinRequired[128];
    char lblSaveTo[64];
    char btnBrowse[64];
    char btnAccept[64];
    char titleIncomingTransfer[128];
    char msgSelectDevice[128];
    char titleNoSelection[64];
    char msgHashtagNotFound[128];
    char titleNotFound[64];
    char msgUnableDelete[128];
    char titleError[64];
    char msgTransferCanceled[128];
    char msgTransfersComplete[128];
    char msgMaxFiles[128];
    char titleWarning[64];
    char msgFirewallWarning[512];
    char titleFirewallWarning[128];
    char msgAllFilesTransferred[128];
    char msgWaitingForFiles[128];
    char lblSelectSaveDir[128];
    char lblDiscTimeout[128];
    char lblMulticast[128];
    char lblEncryption[128];
    char lblDeviceType[128];
    char lblDeviceModel[128];
    char filterAllFiles[128];
    char lblWantsToSend[128];
    char lblFilesCount[128];
    char lblFileHeader[128];
    char lblSizeHeader[128];
    char lblStatusReady[128];
    char lblHashtag[128];
    char lblIpAddress[128];
    char lblActivePort[128];
    char lblStatusStarting[128];
    char lblStatusError[128];
    char lblDisconnected[128];
    char devTypePhone[64];
    char devTypeLaptop[64];
    char devTypeWeb[64];
    char devTypeTerminal[64];
    char devTypeServer[64];
    char msgConnectingTo[128];
    char errInvalidIp[128];
    char errInvalidHashtag[128];
    char trayDropFilesHere[64];
    char trayFilesCount[64];
    char traySendToDevice[64];
    char trayNoDevices[64];
    char trayOpen[64];
    char traySettings[64];
    char trayExit[64];
    char trayClearFiles[64];
    char traySendingNotification[128];
    char setRecursiveFolder[128];
    char btnHelp[64];
    char trayAddFiles[64];
    char trayAddFolder[64];
    char trayPaste[64];
    char trayDropZone[64];
    char trayFilesQueued[128];
} LanguageStrings;

#include "network_tx.h"

extern FileToSend g_sendQueue[MAX_SEND_FILES];
extern int g_sendQueueCount;
extern HWND hWndListSendFiles;
extern HWND hWndListDevices;
extern HWND hWndTab;
extern HWND hWndCheckRecursiveFolder;
extern HWND hWndBtnHelp;
extern HWND g_hWndDropZone;
extern int g_RecursiveFolder;

void AddFileToSendQueue(const char* filePath, const char* fileName, long long fileSize);
void ClearSendQueue();
void UpdateSendButtonsState();
void FormatByteSizeString(long long bytes, char* outStr, size_t maxLen);
void ProcessDroppedFiles(HDROP hDrop);
void AddDirectoryFilesRecursive(const char* baseDir, bool recursive);

extern LanguageStrings g_Lang;
extern int g_Language;
extern const LanguageStrings g_Languages[LANG_COUNT];
void InitLanguage();

void GetSaveDirectory(char* outDir, size_t maxLen);
void GetConfiguredSavePath(char* outPath, size_t maxLen, const char* fileName);
const char* GetProtocolDeviceType(const char* internalDeviceType);
const char* GetLocalizedDeviceType(const char* deviceType);
UINT SafeExtractIcon(LPCSTR szFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon);
long long GetFileSizeBytes(HANDLE hFile);

#endif
