#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool initWinsock(){
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
        if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
            printf("Can't start Winsock.\n");
            return false;
        }
    }
    return true;
}

bool checkRulesExistence(){
    int result = system("netsh advfirewall firewall show rule name=\"LocalSend Custom UDP\" >nul 2>nul");
    return (result == 0);
}

// Automatically configure local firewall rules to allow traffic
void autoFirewall() {
    // Set up incoming firewall rules to allow UDP discovery and TCP file transfers on port 53317
    if (checkRulesExistence()){
        printf("Firewall rules already configured. Skipping configuration.\n");
        return; 
    }

    printf("Configuring firewall rules...\n");
    int r1 = system("netsh advfirewall firewall add rule name=\"LocalSend Custom UDP\" dir=in action=allow protocol=UDP localport=53317");
    int r2 = system("netsh advfirewall firewall add rule name=\"LocalSend Custom TCP\" dir=in action=allow protocol=TCP localport=53317");

    if (r1 == 0 && r2 == 0) printf("Rules applied successfully\n");
    else printf("Can't apply the rules; did you start the app as Administrator?\n");
}

// JSON parser for LocalSend discovery announcement payloads
bool parseLocalSendJSON(const char *json, RemoteDevice *outDevice) {
    memset(outDevice, 0, sizeof(RemoteDevice));
    outDevice->port = 53317;
    outDevice->isHttps = true;

    char key[64];
    char value[128];
    const char *p = json;

    while (*p) {
        if (*p == '"') {
            p++;
            const char *keyStart = p;
            while (*p && *p != '"') p++;
            if (!*p) return false;

            size_t keyLen = p - keyStart;
            if (keyLen >= sizeof(key)) keyLen = sizeof(key) - 1;
            strncpy(key, keyStart, keyLen);
            key[keyLen] = '\0';
            p++;

            while (*p && *p != ':') p++;
            if (!*p) return false;
            p++;

            while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;

            const char *valStart = p;
            if (*p == '"') {
                p++;
                while (*p && *p != '"') p++;
                if (*p == '"') p++;
            } else {
                while (*p && *p != ',' && *p != '}' && *p != ' ' && *p != '\r' && *p != '\n') p++;
            }

            size_t valLen = p - valStart;
            if (valLen >= sizeof(value)) valLen = sizeof(value) - 1;
            strncpy(value, valStart, valLen);
            value[valLen] = '\0';

            // Map standard LocalSend JSON fields to our RemoteDevice struct
            if (strcmp(key, "alias") == 0) {
                cleanQuotes(outDevice->alias, value, sizeof(outDevice->alias));
            } else if (strcmp(key, "version") == 0) {
                cleanQuotes(outDevice->version, value, sizeof(outDevice->version));
            } else if (strcmp(key, "deviceModel") == 0) {
                cleanQuotes(outDevice->deviceModel, value, sizeof(outDevice->deviceModel));
            } else if (strcmp(key, "deviceType") == 0) {
                cleanQuotes(outDevice->deviceType, value, sizeof(outDevice->deviceType));
            } else if (strcmp(key, "fingerprint") == 0) {
                cleanQuotes(outDevice->fingerprint, value, sizeof(outDevice->fingerprint));
            } else if (strcmp(key, "port") == 0) {
                outDevice->port = atoi(value);
            } else if (strcmp(key, "protocol") == 0) {
                char prot[32];
                cleanQuotes(prot, value, sizeof(prot));
                outDevice->isHttps = (strcmp(prot, "http") != 0);
            } else if (strcmp(key, "announce") == 0 || strcmp(key, "announcement") == 0) {
                outDevice->announce = (strstr(value, "true") != NULL);
            }
        }
        p++;
    }
    return (strlen(outDevice->alias) > 0);
}

// Strips leading and trailing quotation marks from JSON string values
void cleanQuotes(char *dest, const char *src, size_t maxLen){
    size_t len = strlen(src);
    if (len == 0) { dest[0] = '\0'; return ; }

    size_t start = (src[0] == '"') ? 1 : 0;
    size_t end = (len > start && src[len - 1] == '"') ? len - 1 : len;

    size_t toCopy = end - start;
    if (toCopy >= maxLen) toCopy = maxLen - 1;

    strncpy(dest, src + start, toCopy);
    dest[toCopy] = '\0';
}

int g_SaveMode = 1;
int g_QuickSave = 0;
char g_CustomPath[MAX_PATH] = {0};
char g_SessionSavePath[MAX_PATH] = {0};
char g_MyFingerprint[128] = {0};

void GenerateFallbackFingerprint(void) {
    if (strlen(g_MyFingerprint) == 0) {
        srand((unsigned int)GetTickCount());
        for (int i = 0; i < 64; i++) {
            sprintf(g_MyFingerprint + i, "%x", rand() % 16);
        }
        g_MyFingerprint[64] = '\0';
    }
}
int g_RequirePin = 0;
char g_PinCode[16] = "1234";
int g_DiscoveryTimeout = 5;
char g_MulticastAddr[64] = "224.0.0.167";
int g_EnableEncryption = 1;
char g_DeviceType[32] = "tablet";
char g_DeviceModel[64] = "Surface RT";
int g_Port = 53317;
volatile bool g_bCancelSendSession = false;
int g_RecursiveFolder = 0;

// Resolves destination folder based on user settings: 0=App Directory, 1=User Downloads, 2=Custom Directory
void GetSaveDirectory(char* outDir, size_t maxLen) {
    if (!outDir || maxLen == 0) return;
    if (g_SaveMode == 0) {
        GetModuleFileNameA(NULL, outDir, (DWORD)maxLen);
        char* lastSlash = strrchr(outDir, '\\');
        if (lastSlash) *lastSlash = '\0';
    } else if (g_SaveMode == 1) {
        char userProfile[MAX_PATH] = {0};
        GetEnvironmentVariableA("USERPROFILE", userProfile, sizeof(userProfile));
        
        if (strlen(userProfile) > 0) {
            // NT 4.0 (C:\WINNT\Profiles\...), 2000/XP (C:\Documents and Settings\...), Vista/7/8/10/11 (C:\Users\...)
            _snprintf(outDir, maxLen, "%s\\Downloads", userProfile);
        } else {
            // Fallback for Windows 95/98/ME where %USERPROFILE% might not exist
            char homeDrive[32] = {0}, homePath[MAX_PATH] = {0};
            GetEnvironmentVariableA("HOMEDRIVE", homeDrive, sizeof(homeDrive));
            GetEnvironmentVariableA("HOMEPATH", homePath, sizeof(homePath));
            if (strlen(homeDrive) > 0 && strlen(homePath) > 0) {
                _snprintf(outDir, maxLen, "%s%s\\Downloads", homeDrive, homePath);
            } else {
                char winDir[MAX_PATH] = {0};
                GetWindowsDirectoryA(winDir, sizeof(winDir));
                char driveLetter = (winDir[0] != '\0') ? winDir[0] : 'C';
                _snprintf(outDir, maxLen, "%c:\\Downloads", driveLetter);
            }
        }
        CreateDirectoryA(outDir, NULL);
    } else {
        strncpy(outDir, g_CustomPath, maxLen);
    }
}

// Combines the configured destination folder with the filename to form a full target path
void GetConfiguredSavePath(char* outPath, size_t maxLen, const char* fileName) {
    char dir[MAX_PATH] = {0};
    GetSaveDirectory(dir, sizeof(dir));
    int dLen = strlen(dir);
    if (dLen > 0 && dir[dLen - 1] == '\\') dir[dLen - 1] = '\0';
    CreateDirectoryA(dir, NULL);
    _snprintf(outPath, maxLen, "%s\\%s", dir, fileName ? fileName : "file");
}

LanguageStrings g_Lang;
int g_Language = 0;

const LanguageStrings g_Languages[LANG_COUNT] = {
    // English (LANG_EN)
    {
        "Send",
        "Receive",
        "Settings",
        "Nearby devices:",
        "Send files",
        "Clean files",
        "Browse",
        "Search again",
        "Send manually",
        "General",
        "Receive",
        "Network",
        "Other",
        "Save window position when exiting",
        "Minimise to notification icon instead of closing the app",
        "Make app always on top",
        "When I receive a file...",
        "Save files in the same path as this app",
        "Save files in the Downloads folder",
        "Save files in this path:",
        "Quick Save (save automatically without asking)",
        "Require PIN",
        "PIN code:",
        "Port:",
        "About LocalSend",
        "About LocalSend RT/32",
        "Language:",
        "Device name",
        "Edit",
        "Save",
        "Cancel",
        "Network information",
        "Waiting for files...",
        "Close",
        "Select save folder:",
        "Select a folder to send:",
        "Insert text message",
        "IP address",
        "Hashtag",
        "File",
        "Size",
        "Date",
        "Progress",
        "Device",
        "Info",
        "Today",
        "Older transfers",
        "Files to send:",
        "Add files",
        "Add folder",
        "Send text message",
        "Paste from clipboard",
        "File",
        "Folder",
        "Text",
        "Paste",
        "libcrypto-3.dll is missing.",
        "libssl-3.dll is missing.",
        "libcrypto-3.dll and libssl-3.dll are missing.",
        "Enter PIN",
        "The receiver requires a PIN code to complete the transfer:",
        "Save to:",
        "Browse...",
        "Accept",
        "Incoming file transfer",
        "Please check or select a device to send files to.",
        "No selection",
        "Hashtag not found in nearby list. Please scan again.",
        "Not found",
        "Unable to delete file.",
        "Error",
        "Transfer canceled by sender.",
        "All transfers complete.",
        "You've added the maximum number of files possible.",
        "Warning",
        "If this is the first time you're running this app, run it as Administrator so Windows can add the needed firewall rules. If not, try it anyway as it should work.",
        "Firewall rules missing",
        "All files transferred successfully.",
        "Waiting for files...",
        "Select save folder:",
        "Discovery timeout (sec):",
        "Multicast address:",
        "Enable encryption (TLS)",
        "Device type:",
        "Device model:",
        "All files",
        "wants to send you",
        "%d files",
        "File",
        "Size",
        "Status: Ready to receive",
        "Hashtag: ",
        "IP address: ",
        "Active port: ",
        "Status: Starting...",
        "Status: Offline (Error)",
        "Status: Disconnected (No network)",
        "Phone",
        "Laptop",
        "Web",
        "Terminal",
        "Server",
        "Connecting to %s...",
        "Please enter a valid IPv4 address (e.g. 192.168.1.5).",
        "Please enter a valid hashtag or device alias.",
        "Drop files here",
        "Files to send (%d)",
        "Send to device",
        "(No nearby devices)",
        "Open",
        "Settings",
        "Exit",
        "Clear list",
        "Sending %d file(s) to %s...",
        "Recursively add files when selecting to add a folder",
        "Help",
        "Add files...",
        "Add folder...",
        "Paste from clipboard",
        "Quick Drop Target",
        "%d file(s) added to send queue."
    },
    // Italian (LANG_IT)
    {
        "Invia",
        "Ricevi",
        "Impostazioni",
        "Dispositivi vicini:",
        "Invia file",
        "Pulisci lista",
        "Sfoglia",
        "Cerca ancora",
        "Invia manualmente",
        "Generale",
        "Ricevi",
        "Rete",
        "Altro",
        "Salva posizione finestra all'uscita",
        "Riduci a icona nell'area di notifica invece di chiudere l'app",
        "Mantieni l'app sempre in primo piano",
        "Quando ricevo un file...",
        "Salva i file nello stesso percorso dell'app",
        "Salva i file nella cartella Download",
        "Salva i file in questo percorso:",
        "Salvataggio rapido (salva automaticamente senza chiedere)",
        "Richiedi PIN",
        "Codice PIN:",
        "Porta:",
        "Info su LocalSend",
        "Info su LocalSend 32/RT",
        "Lingua:",
        "Nome dispositivo",
        "Modifica",
        "Salva",
        "Annulla",
        "Informazioni di rete",
        "In attesa di file...",
        "Chiudi",
        "Seleziona cartella di salvataggio:",
        "Seleziona una cartella da inviare:",
        "Inserisci messaggio di testo",
        "Indirizzo IP",
        "Hashtag",
        "File",
        "Dimensione",
        "Data",
        "Avanzamento",
        "Dispositivo",
        "Info",
        "Oggi",
        "Trasferimenti precedenti",
        "File da inviare:",
        "Aggiungi file",
        "Aggiungi cartella",
        "Invia messaggio di testo",
        "Incolla dagli appunti",
        "File",
        "Cartella",
        "Testo",
        "Incolla",
        "libcrypto-3.dll è mancante.",
        "libssl-3.dll è mancante.",
        "libcrypto-3.dll e libssl-3.dll sono mancanti.",
        "Inserisci PIN",
        "Il ricevente richiede un codice PIN per completare il trasferimento:",
        "Salva in:",
        "Sfoglia...",
        "Accetta",
        "Trasferimento file in arrivo",
        "Si prega di verificare o selezionare un dispositivo a cui inviare i file.",
        "Nessuna selezione",
        "Hashtag non trovato nella lista dei dispositivi vicini. Si prega di scansionare di nuovo.",
        "Non trovato",
        "Impossibile eliminare il file.",
        "Errore",
        "Trasferimento annullato dal mittente.",
        "Tutti i trasferimenti sono completati.",
        "Hai aggiunto il numero massimo di file consentito.",
        "Avviso",
        "Se è la prima volta che esegui questa app, eseguila come Amministratore in modo che Windows possa aggiungere le regole del firewall necessarie. Altrimenti, prova comunque poiché dovrebbe funzionare.",
        "Regole del firewall mancanti",
        "Tutti i file sono stati trasferiti con successo.",
        "In attesa di file...",
        "Seleziona cartella di salvataggio:",
        "Timeout ricerca (sec):",
        "Indirizzo multicast:",
        "Abilita crittografia (TLS)",
        "Tipo dispositivo:",
        "Modello dispositivo:",
        "Tutti i file",
        "vorrebbe inviarti",
        "%d file",
        "File",
        "Dimensione",
        "Stato: Pronto a ricevere",
        "Hashtag: ",
        "Indirizzo IP: ",
        "Porta Attiva: ",
        "Stato: In avvio...",
        "Stato: Offline (Errore)",
        "Stato: Disconnesso (Nessuna rete)",
        "Telefono",
        "Portatile",
        "Web",
        "Terminale",
        "Server",
        "Connessione a %s in corso...",
        "Inserisci un indirizzo IPv4 valido (es. 192.168.1.5).",
        "Inserisci un hashtag o nome dispositivo valido.",
        "Trascina qui i file",
        "File da inviare (%d)",
        "Invia al dispositivo",
        "(Nessun dispositivo vicino)",
        "Apri",
        "Impostazioni",
        "Esci",
        "Pulisci lista",
        "Invio di %d file a %s in corso...",
        "Aggiungi ricorsivamente i file quando selezioni una cartella",
        "Guida",
        "Aggiungi file...",
        "Aggiungi cartella...",
        "Incolla dagli appunti",
        "Area di rilascio rapida",
        "%d file aggiunti alla coda di invio."
    },
    // Chinese(Simplified) (LANG_zh_CN)
    {
        "发送",
        "接收",
        "设置",
        "附近的设备:",
        "发送文件",
        "当前发送文件全部清除",
        "选择文件夹",
        "再次搜寻",
        "输入地址",
        "通用",
        "接收",
        "网络",
        "其他",
        "退出时自动保存记忆当前窗口位置",
        "最小化到Windows右下角通知图标，但不关闭应用程序",
        "让应用程序始终置顶于屏幕窗口",
        "当我收到文件时...",
        "将文件保存在与此应用程序相同的路径中",
        "将文件保存在“下载”文件夹中",
        "将文件保存在该自定义路径中:",
        "自动保存 (无需询问自动接受所有文件传输请求。请注意，这会让此网络中的所有人都可以向你发送文件。)",
        "启用PIN密码",
        "PIN密码:",
        "端口:",
        "关于LocalSend",
        "关于LocalSend RT/32",
        "语言:",
        "设备名称",
        "编辑",
        "保存",
        "取消",
        "网络信息",
        "等待响应中...",
        "关闭",
        "选择保存的文件夹:",
        "选择文件夹发送:",
        "输入消息",
        "IP地址",
        "设备标签",
        "文件",
        "文件大小",
        "发送日期",
        "文件发送情况",
        "附近的设备",
        "设备标签",
        "今天",
        "之前的文件传输记录",
        "待发送的文件:",
        "添加文件",
        "添加文件夹",
		"发送文本",
        "从系统剪贴板粘贴",
        "文件",
        "文件夹",
        "信息",
        "粘贴",
        "没有找到libcrypto-3.dll文件.",
        "没有找到libssl-3.dll文件.",
        "没有找到libcrypto-3.dll和libssl-3.dll文件.",
        "输入 PIN",
        "接收方需要 PIN 码以完成传输:",
        "保存至:",
        "浏览...",
        "接受",
        "收到文件传输请求",
        "请检查或选择要发送文件的设备。",
        "未选择",
        "在附近列表中未找到该标签的设备。请重新扫描。",
        "未找到",
        "无法删除文件。",
        "错误",
        "传输已被发送方取消。",
        "所有传输已完成。",
        "您已添加了最大数量的文件。",
        "警告",
        "如果这是您第一次运行此应用程序，请以管理员身份运行，以便 Windows 可以添加所需的防火墙规则。否则，请直接尝试，通常它应该可以正常工作。",
        "缺少防火墙规则",
        "所有文件已成功传输。",
        "正在等待接收文件...",
        "选择保存文件夹:",
        "设备发现超时 (秒):",
        "组播地址:",
        "启用加密 (TLS)",
        "设备类型:",
        "设备型号:",
        "所有文件",
        "想发送给你",
        "%d 个文件",
        "文件名",
        "大小",
        "状态: 准备接收",
        "哈希标签: ",
        "IP 地址: ",
        "活动端口: ",
        "状态: 正在启动...",
        "状态: 离线 (错误)",
        "状态: 未连接 (无网络)",
        "手机",
        "笔记本",
        "网页",
        "终端",
        "服务器",
        "正在连接到 %s...",
        "请输入有效的 IPv4 地址 (例如 192.168.1.5)。",
        "请输入有效的设备标签或别名。",
        "拖放文件到此处",
        "待发送文件 (%d)",
        "发送到设备",
        "(未发现附近设备)",
        "打开",
        "设置",
        "退出",
        "清除列表",
        "正在发送 %d 个文件至 %s...",
        "选择添加文件夹时递归添加内部所有文件",
        "帮助",
        "添加文件...",
        "添加文件夹...",
        "从剪贴板粘贴",
        "快速拖放区域",
        "已添加 %d 个文件至发送队列。"
    },
};

extern char g_IniPath[MAX_PATH];

void InitLanguage() {
    g_Language = GetPrivateProfileIntA("Settings", "Language", 0, g_IniPath);
    if (g_Language < 0 || g_Language >= LANG_COUNT) {
        g_Language = 0;
    }
    memcpy(&g_Lang, &g_Languages[g_Language], sizeof(LanguageStrings));
}

// Helper to format byte counts into human-readable size strings (B, KB, MB, GB)
void FormatByteSizeString(long long bytes, char* outStr, size_t maxLen) {
    if (!outStr || maxLen == 0) return;
    if (bytes < 1024) {
        _snprintf(outStr, maxLen, "%lld B", bytes);
    } else if (bytes < 1024 * 1024) {
        _snprintf(outStr, maxLen, "%.1f KB", bytes / 1024.0);
    } else if (bytes < 1024LL * 1024LL * 1024LL) {
        _snprintf(outStr, maxLen, "%.1f MB", bytes / (1024.0 * 1024.0));
    } else {
        _snprintf(outStr, maxLen, "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

// Device type strings to LocalSend protocol format
const char* GetProtocolDeviceType(const char* internalDeviceType) {
    if (_stricmp(internalDeviceType, "Laptop") == 0 || _stricmp(internalDeviceType, "desktop") == 0) return "desktop";
    if (_stricmp(internalDeviceType, "Phone") == 0 || _stricmp(internalDeviceType, "mobile") == 0) return "mobile";
    if (_stricmp(internalDeviceType, "Terminal") == 0 || _stricmp(internalDeviceType, "headless") == 0) return "headless";
    if (_stricmp(internalDeviceType, "Web") == 0) return "web";
    if (_stricmp(internalDeviceType, "Server") == 0) return "server";
    return "desktop";
}

// Translates device type for display in UI
const char* GetLocalizedDeviceType(const char* deviceType) {
    if (!deviceType) return "";
    if (_stricmp(deviceType, "Laptop") == 0 || _stricmp(deviceType, "desktop") == 0) return g_Lang.devTypeLaptop;
    if (_stricmp(deviceType, "Phone") == 0 || _stricmp(deviceType, "mobile") == 0) return g_Lang.devTypePhone;
    if (_stricmp(deviceType, "Terminal") == 0 || _stricmp(deviceType, "headless") == 0) return g_Lang.devTypeTerminal;
    if (_stricmp(deviceType, "Web") == 0) return g_Lang.devTypeWeb;
    if (_stricmp(deviceType, "Server") == 0) return g_Lang.devTypeServer;
    return deviceType;
}

// Safely extracts icons on all Windows versions from NT 4.0 to Windows 11
typedef UINT (WINAPI *pfnPrivateExtractIconsA)(LPCSTR, int, int, int, HICON*, UINT*, UINT, UINT);
UINT SafeExtractIcon(LPCSTR szFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon) {
    if (!phicon) return 0;
    *phicon = NULL;
    static pfnPrivateExtractIconsA pfnPEI = NULL;
    static BOOL bChecked = FALSE;
    if (!bChecked) {
        HMODULE hUser = GetModuleHandleA("user32.dll");
        if (hUser) pfnPEI = (pfnPrivateExtractIconsA)GetProcAddress(hUser, "PrivateExtractIconsA");
        bChecked = TRUE;
    }
    if (pfnPEI) {
        UINT dum = 0;
        if (pfnPEI(szFileName, nIconIndex, cxIcon, cyIcon, phicon, &dum, 1, 0) > 0 && *phicon != NULL) {
            return 1;
        }
    }
    // Fallback for Windows NT 4.0 / 2000 using ExtractIconExA
    HICON hLarge = NULL, hSmall = NULL;
    if (ExtractIconExA(szFileName, nIconIndex, &hLarge, &hSmall, 1) > 0) {
        if (cxIcon >= 32 && hLarge) {
            *phicon = hLarge;
            if (hSmall) DestroyIcon(hSmall);
            return 1;
        } else if (hSmall) {
            *phicon = hSmall;
            if (hLarge) DestroyIcon(hLarge);
            return 1;
        } else if (hLarge) {
            *phicon = hLarge;
            return 1;
        }
    }
    return 0;
}

// Safely queries 64-bit file size across all Windows NT / Win32 versions
long long GetFileSizeBytes(HANDLE hFile) {
    if (hFile == INVALID_HANDLE_VALUE) return 0;
    DWORD highSize = 0;
    DWORD lowSize = GetFileSize(hFile, &highSize);
    if (lowSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        return 0;
    }
    return ((long long)highSize << 32) | (long long)lowSize;
}
