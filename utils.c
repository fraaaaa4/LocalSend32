#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool initWinsock(){
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
        printf("Can't start Winsock.\n"); return false;
    }
    return true;
}

bool checkRulesExistence(){
    int result = system("netsh advfirewall firewall show rule name=\"LocalSend Custom UDP\" >nul 2>nul");
    return (result == 0);
}

void autoFirewall() {
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

bool parseLocalSendJSON(const char *json, RemoteDevice *outDevice) {
    memset(outDevice, 0, sizeof(RemoteDevice));
    outDevice->port = 53317;

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
            } else if (strcmp(key, "announce") == 0 || strcmp(key, "announcement") == 0) {
                outDevice->announce = (strstr(value, "true") != NULL);
            }
        }
        p++;
    }
    return (strlen(outDevice->alias) > 0);
}

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
char g_MyFingerprint[64] = {0};

int g_RequirePin = 0;
char g_PinCode[16] = "1234";
int g_DiscoveryTimeout = 5;
char g_MulticastAddr[64] = "224.0.0.167";
int g_EnableEncryption = 1;
char g_DeviceType[32] = "tablet";
char g_DeviceModel[64] = "Surface RT";
int g_Port = 53317;

void GetSaveDirectory(char* outDir, size_t maxLen) {
    if (g_SaveMode == 0) {
        GetModuleFileNameA(NULL, outDir, (DWORD)maxLen);
        char* lastSlash = strrchr(outDir, '\\');
        if (lastSlash) *lastSlash = '\0';
    } else if (g_SaveMode == 1) {
        char userProfile[MAX_PATH] = {0};
        GetEnvironmentVariableA("USERPROFILE", userProfile, sizeof(userProfile));
        _snprintf(outDir, maxLen, "%s\\Downloads", userProfile);
    } else {
        strncpy(outDir, g_CustomPath, maxLen);
    }
}

void GetConfiguredSavePath(char* outPath, size_t maxLen, const char* fileName) {
    char dir[MAX_PATH] = {0};
    GetSaveDirectory(dir, sizeof(dir));
    int dLen = strlen(dir);
    if (dLen > 0 && dir[dLen - 1] == '\\') dir[dLen - 1] = '\0';
    _snprintf(outPath, maxLen, "%s\\%s", dir, fileName);
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
        "PIN Code:",
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
        "IP Address",
        "Hashtag",
        "File",
        "Size",
        "Date",
        "Progress",
        "Device",
        "Info",
        "Today",
        "Older Transfers",
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
        "libcrypto-3.dll and libssl-3.dll are missing."
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
        "libcrypto-3.dll e libssl-3.dll sono mancanti."
    }
};

extern char g_IniPath[MAX_PATH];

void InitLanguage() {
    g_Language = GetPrivateProfileIntA("Settings", "Language", 0, g_IniPath);
    if (g_Language < 0 || g_Language >= LANG_COUNT) {
        g_Language = 0;
    }
    memcpy(&g_Lang, &g_Languages[g_Language], sizeof(LanguageStrings));
}
