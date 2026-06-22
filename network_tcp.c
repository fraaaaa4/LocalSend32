#include "network_tcp.h"
#include "cert.h"
#include "utils.h"
#include "tls_layer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <commctrl.h>
#include <shlobj.h>

#define TCP_PORT 53317
#define MAX_QUEUE_FILES 20

typedef struct {
    SOCKET clientSocket;
    char clientIP[16];
} ClientContext;

PreparedFile g_fileQueue[MAX_QUEUE_FILES];
int g_fileQueueCount = 0;
volatile LONG g_completedFilesCount = 0;
char g_lastSenderDeviceName[128] = {0};

// Handles the HTTP protocol dialogue with the sender (parsing prepare-upload requests, managing PIN verification, showing UI confirm dialogs, and writing uploaded files to disk).
void handleClientSession(SOCKET clientSocket, TlsSocket* tls, const char* clientIP, bool useTls) {
    char cleartextBuffer[32768];
    int clearLen = 0;

    if (useTls) {
        clearLen = TlsRead(tls, cleartextBuffer, sizeof(cleartextBuffer) - 1);
    } else {
        clearLen = recv(clientSocket, cleartextBuffer, sizeof(cleartextBuffer) - 1, 0);
    }

    if (clearLen <= 0) return;
    cleartextBuffer[clearLen] = '\0';

    if (strstr(cleartextBuffer, "POST /api/localsend/v2/prepare-upload") != NULL) {
        char *aliasPtr = strstr(cleartextBuffer, "\"alias\":\"");
        if (aliasPtr) {
            aliasPtr += 9;
            char *aliasEnd = strchr(aliasPtr, '"');
            if (aliasEnd) {
                strncpy(g_lastSenderDeviceName, aliasPtr, aliasEnd - aliasPtr);
                g_lastSenderDeviceName[aliasEnd - aliasPtr] = '\0';
            }
        } else {
            char *devicePtr = strstr(cleartextBuffer, "\"device\":\"");
            if (devicePtr) {
                devicePtr += 10;
                char *deviceEnd = strchr(devicePtr, '"');
                if (deviceEnd) {
                    strncpy(g_lastSenderDeviceName, devicePtr, deviceEnd - devicePtr);
                    g_lastSenderDeviceName[deviceEnd - devicePtr] = '\0';
                }
            }
        }

        printf("\nprepare-upload request...\n");

        g_fileQueueCount = 0;
        g_completedFilesCount = 0;
        memset(g_fileQueue, 0, sizeof(g_fileQueue));

        char *searchPtr = strstr(cleartextBuffer, "\"files\":{");
        if (searchPtr) {
            while (g_fileQueueCount < MAX_QUEUE_FILES) {
                char *idPtr = strstr(searchPtr, "\"id\":\"");
                if (!idPtr) break;
                idPtr += 6;
                char *idEnd = strchr(idPtr, '"');
                if (!idEnd) break;

                strncpy(g_fileQueue[g_fileQueueCount].fileId, idPtr, idEnd - idPtr);

                char *namePtr = strstr(idEnd, "\"fileName\":\"");
                if (namePtr) {
                    namePtr += 12;
                    char *nameEnd = strchr(namePtr, '"');
                    if (nameEnd) strncpy(g_fileQueue[g_fileQueueCount].fileName, namePtr, nameEnd - namePtr);
                }

                char *sizePtr = strstr(idEnd, "\"size\":");
                if (sizePtr) {
                    sizePtr += 7;
                    g_fileQueue[g_fileQueueCount].fileSize = atoll(sizePtr);
                }

                g_fileQueueCount++;
                searchPtr = idEnd;
            }
        }

        bool pinValid = true;
        if (g_RequirePin) {
            char pinParam[32] = {0};
            char* pinQuery = strstr(cleartextBuffer, "pin=");
            if (pinQuery) {
                pinQuery += 4;
                char* qEnd = strpbrk(pinQuery, " &\r\n\"");
                if (qEnd) strncpy(pinParam, pinQuery, qEnd - pinQuery);
                else strcpy(pinParam, pinQuery);
            } else {
                char* keyPtr = strstr(cleartextBuffer, "\"key\":\"");
                if (keyPtr) {
                    keyPtr += 7;
                    char* kEnd = strchr(keyPtr, '"');
                    if (kEnd) strncpy(pinParam, keyPtr, kEnd - keyPtr);
                }
            }
            if (strcmp(pinParam, g_PinCode) != 0) {
                pinValid = false;
            }
        }

        char httpResponse[1536];
        if (!pinValid) {
            const char* jsonResponse = "{\"detail\":\"PIN invalid or required\"}";
            sprintf(httpResponse, "HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s", (int)strlen(jsonResponse), jsonResponse);
            if (useTls) TlsWrite(tls, httpResponse, strlen(httpResponse));
            else send(clientSocket, httpResponse, strlen(httpResponse), 0);
            return;
        }

        bool acceptTransfer = true;
        char tempSavePath[MAX_PATH] = {0};
        if (!g_QuickSave) {
            ConfirmationRequest req;
            req.senderName = g_lastSenderDeviceName;
            req.fileCount = g_fileQueueCount;
            GetSaveDirectory(req.selectedPath, MAX_PATH);
            req.accepted = false;
            req.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            
            if (req.hEvent) {
                PostMessage(g_hWndMain, WM_CONFIRM_TRANSFER, 0, (LPARAM)&req);
                WaitForSingleObject(req.hEvent, INFINITE);
                CloseHandle(req.hEvent);
                acceptTransfer = req.accepted;
                strcpy(tempSavePath, req.selectedPath);
            } else {
                acceptTransfer = false;
            }
        }

        if (acceptTransfer) {
            if (!g_QuickSave) {
                strcpy(g_SessionSavePath, tempSavePath);
            } else {
                g_SessionSavePath[0] = '\0';
            }
            
            char jsonResponse[1024] = {0};
            strcat(jsonResponse, "{\"sessionId\":\"surface-session\",\"files\":{");
            for (int i = 0; i < g_fileQueueCount; i++) {
                char fileToken[256];
                sprintf(fileToken, "\"%s\":\"accepted-token\"%s", g_fileQueue[i].fileId, (i == g_fileQueueCount - 1) ? "" : ",");
                strcat(jsonResponse, fileToken);
            }
            strcat(jsonResponse, "}}");

            sprintf(httpResponse, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s", (int)strlen(jsonResponse), jsonResponse);
        } else {
            const char* jsonResponse = "{\"error\":\"declined\"}";
            sprintf(httpResponse, "HTTP/1.1 403 Forbidden\r\nContent-Type: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s", (int)strlen(jsonResponse), jsonResponse);
        }

        if (useTls) TlsWrite(tls, httpResponse, strlen(httpResponse));
        else send(clientSocket, httpResponse, strlen(httpResponse), 0);
    }
    else if (strstr(cleartextBuffer, "POST /api/localsend/v2/upload") != NULL) {
        char currentFileName[256] = "download_error.dat";
        long long currentFileSize = 0;
        char extractedId[128] = {0};

        char *urlIdPtr = strstr(cleartextBuffer, "fileId=");
        if (urlIdPtr) {
            urlIdPtr += 7;
            char *urlIdEnd = strpbrk(urlIdPtr, " &\r\n");
            if (urlIdEnd) {
                strncpy(extractedId, urlIdPtr, urlIdEnd - urlIdPtr);

                for (int i = 0; i < g_fileQueueCount; i++) {
                    if (strcmp(g_fileQueue[i].fileId, extractedId) == 0) {
                        strcpy(currentFileName, g_fileQueue[i].fileName);
                        currentFileSize = g_fileQueue[i].fileSize;
                        break;
                    }
                }
            }
        }

        char finalPath[MAX_PATH] = {0};
        if (g_SessionSavePath[0] != '\0') {
            _snprintf(finalPath, sizeof(finalPath), "%s\\%s", g_SessionSavePath, currentFileName);
        } else {
            GetConfiguredSavePath(finalPath, sizeof(finalPath), currentFileName);
        }

        FILE* f = fopen(finalPath, "wb");
        if (g_hWndMain) {
            FileStartInfo* info = (FileStartInfo*)malloc(sizeof(FileStartInfo));
            if (info) {
                strcpy(info->fileName, currentFileName);
                strcpy(info->fileId, extractedId);
                strncpy(info->senderName, g_lastSenderDeviceName, 127);
                info->fileSize = currentFileSize;
                PostMessage(g_hWndMain, WM_FILE_START, 0, (LPARAM)info);
            }
        }

        long long uploadedBytesCount = 0;
        char* bodyStart = strstr(cleartextBuffer, "\r\n\r\n");
        if (bodyStart) {
            bodyStart += 4;
            int headerLen = bodyStart - cleartextBuffer;
            int fileBytesInFirstBlock = clearLen - headerLen;
            if (f && fileBytesInFirstBlock > 0) {
                fwrite(bodyStart, 1, fileBytesInFirstBlock, f);
                uploadedBytesCount += fileBytesInFirstBlock;
            }
        }

        if (g_hWndMain && currentFileSize > 0) {
            int pct = (int)((uploadedBytesCount * 100) / currentFileSize);
            FileProgressInfo* info = (FileProgressInfo*)malloc(sizeof(FileProgressInfo));
            if (info) {
                info->pct = pct;
                strcpy(info->fileId, extractedId);
                PostMessage(g_hWndMain, WM_FILE_PROGRESS, 0, (LPARAM)info);
            }
        }

        bool transferInterrupted = false;
        while (uploadedBytesCount < currentFileSize) {
            char readBuf[16384];
            int bytesRead = 0;
            if (useTls) {
                bytesRead = TlsRead(tls, readBuf, sizeof(readBuf));
            } else {
                bytesRead = recv(clientSocket, readBuf, sizeof(readBuf), 0);
            }

            if (bytesRead <= 0) {
                transferInterrupted = true;
                break;
            }

            if (f) fwrite(readBuf, 1, bytesRead, f);
            uploadedBytesCount += bytesRead;

            if (g_hWndMain && currentFileSize > 0) {
                int pct = (int)((uploadedBytesCount * 100) / currentFileSize);
                FileProgressInfo* info = (FileProgressInfo*)malloc(sizeof(FileProgressInfo));
                if (info) {
                    info->pct = pct;
                    strcpy(info->fileId, extractedId);
                    PostMessage(g_hWndMain, WM_FILE_PROGRESS, 0, (LPARAM)info);
                }
            }
        }

        if (f) fclose(f);

        if (transferInterrupted || uploadedBytesCount < currentFileSize) {
            DeleteFileA(finalPath);
            if (g_hWndMain) {
                char* canceledId = (char*)malloc(128);
                if (canceledId) {
                    strcpy(canceledId, extractedId);
                    PostMessage(g_hWndMain, WM_FILE_CANCEL, 0, (LPARAM)canceledId);
                }
                SetWindowTextA(hWndStatus, "Transfer canceled by sender.");
            }
            g_fileQueueCount = 0;
            g_completedFilesCount = 0;
            memset(g_fileQueue, 0, sizeof(g_fileQueue));
        } else {
            LONG completed = InterlockedIncrement((LONG volatile *)&g_completedFilesCount);
            char httpOk[256];
            if (completed >= g_fileQueueCount) {
                sprintf(httpOk, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            } else {
                sprintf(httpOk, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: keep-alive\r\n\r\n");
            }

            if (useTls) TlsWrite(tls, httpOk, strlen(httpOk));
            else send(clientSocket, httpOk, strlen(httpOk), 0);

            if (completed >= g_fileQueueCount) {
                g_fileQueueCount = 0;
                g_completedFilesCount = 0;
                memset(g_fileQueue, 0, sizeof(g_fileQueue));
                if (g_hWndMain) {
                    PostMessage(g_hWndMain, WM_FILE_COMPLETE, 0, 0);
                }
            }
        }
    }
}

// Spawned for each connected TCP client. If encryption is requested, it initializes a local SSL session (OpenSSL or Schannel depending on build configuration) before proceeding.
DWORD WINAPI ClientThread(LPVOID lpParam) {
    ClientContext* ctx = (ClientContext*)lpParam;
    bool useTls = (g_EnableEncryption != 0);

    TlsSocket* tls = NULL;
    if (useTls) {
        tls = TlsAccept(ctx->clientSocket);
    }

    if (!useTls || tls != NULL) {
        handleClientSession(ctx->clientSocket, tls, ctx->clientIP, useTls);
    }

    if (tls) TlsFreeSocket(tls);
    closesocket(ctx->clientSocket);
    free(ctx);
    return 0;
}

// Binds to the LocalSend TCP port, marks it as listening, and loops infinitely to accept connections and spawn connection threads.
DWORD WINAPI tcpServerThread(LPVOID lpParam) {
    SOCKET listeningSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == INVALID_SOCKET) return 1;

    int reuse = 1;
    setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(g_Port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listeningSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(listeningSocket);
        return 1;
    }

    if (listen(listeningSocket, 5) == SOCKET_ERROR) {
        closesocket(listeningSocket);
        return 1;
    }

    while (1) {
        clientSocket = accept(listeningSocket, (SOCKADDR*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) continue;

        char* clientIP = inet_ntoa(clientAddr.sin_addr);
        ClientContext* ctx = (ClientContext*)malloc(sizeof(ClientContext));
        if (ctx) {
            ctx->clientSocket = clientSocket;
            strncpy(ctx->clientIP, clientIP, sizeof(ctx->clientIP) - 1);
            ctx->clientIP[sizeof(ctx->clientIP) - 1] = '\0';

            HANDLE hThread = CreateThread(NULL, 0, ClientThread, ctx, 0, NULL);
            if (hThread) {
                CloseHandle(hThread);
            }
        } else {
            closesocket(clientSocket);
        }
    }

    closesocket(listeningSocket);
    return 0;
}
