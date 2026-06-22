#include "network_tx.h"
#include "utils.h"
#include "tls_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <commctrl.h>

#define IDC_SEND_PROGRESS     4011

typedef struct {
    char fileId[128];
    char token[128];
} FileTokenMap;

extern char g_MyDeviceName[];
extern char g_DeviceModel[];
extern char g_DeviceType[];
extern char g_MyFingerprint[];
extern int g_Port;

// Helper to safely allocate a copy of a status string on the heap, allowing it to be sent to WndProc via PostMessage.
char* AllocateString(const char* str) {
    char* newStr = (char*)malloc(strlen(str) + 1);
    if (newStr) strcpy(newStr, str);
    return newStr;
}

// Reads raw HTTP data from the socket stream, handling SSL decryption transparently via the TLS layer if enabled.
static int ReadHttpPlaintext(SOCKET sock, TlsSocket* tls, char* outBuffer, int maxLen, bool useTls) {
    int total = 0;
    outBuffer[0] = '\0';
    DWORD timeout = 60000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    while (1) {
        char chunk[4096];
        int bytes = 0;
        if (useTls) {
            bytes = TlsRead(tls, chunk, sizeof(chunk) - 1);
        } else {
            bytes = recv(sock, chunk, sizeof(chunk) - 1, 0);
        }

        if (bytes <= 0) break;
        chunk[bytes] = '\0';

        if (total + bytes < maxLen - 1) {
            memcpy(outBuffer + total, chunk, bytes);
            total += bytes;
            outBuffer[total] = '\0';
        }

        if (strstr(outBuffer, "\r\n\r\n")) {
            if (strstr(outBuffer, "}}") != NULL || strstr(outBuffer, "HTTP/1.1 4") != NULL || strstr(outBuffer, "HTTP/1.1 5") != NULL) {
                break;
            }
        }
    }
    return total;
}

// Sends an HTTP request payload across the wire, encrypting it on the fly if encryption is enabled.
static bool SendHttpPayload(SOCKET sock, TlsSocket* tls, const char* message, int len, bool useTls) {
    if (useTls) {
        return TlsWrite(tls, message, len) > 0;
    } else {
        return send(sock, message, len, 0) != SOCKET_ERROR;
    }
}

DWORD WINAPI StartSendSessionThread(LPVOID lpParam) {
    SendSessionContext* ctx = (SendSessionContext*)lpParam;
    if (!ctx) return 1;

    bool useTls = (g_EnableEncryption != 0);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in targetAddr;
    targetAddr.sin_family = AF_INET; targetAddr.sin_port = htons(ctx->targetPort); targetAddr.sin_addr.s_addr = inet_addr(ctx->targetIP);

    if (connect(sock, (SOCKADDR*)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR) {
        if (g_hWndMain) {
            PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString("Connection failed. Target not reachable."));
            PostMessage(g_hWndMain, WM_SEND_DONE, FALSE, 0);
        }
        closesocket(sock); free(ctx); return 1;
    }

    TlsSocket* tls = NULL;
    if (useTls) {
        tls = TlsConnect(sock, ctx->targetIP);
        if (!tls) {
            if (g_hWndMain) {
                PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString("TLS Handshake rejected by remote peer."));
                PostMessage(g_hWndMain, WM_SEND_DONE, FALSE, 0);
            }
            closesocket(sock); free(ctx); return 1;
        }
    }

    char jsonPayload[4096] = {0};
    sprintf(jsonPayload, "{\"info\":{\"alias\":\"%s\",\"version\":\"2.0\",\"deviceModel\":\"%s\",\"deviceType\":\"%s\",\"fingerprint\":\"%s\",\"port\":%d,\"protocol\":\"%s\",\"download\":true},\"files\":{",
            g_MyDeviceName, g_DeviceModel, g_DeviceType, g_MyFingerprint, g_Port, useTls ? "https" : "http");

    for (int i = 0; i < ctx->fileCount; i++) {
        char fileChunk[512];
        sprintf(fileChunk, "\"%s\":{\"id\":\"%s\",\"fileName\":\"%s\",\"size\":%lld,\"fileType\":\"application/octet-stream\"}%s",
                ctx->files[i].fileId, ctx->files[i].fileId, ctx->files[i].fileName, ctx->files[i].fileSize,
                (i == ctx->fileCount - 1) ? "" : ",");
        strcat(jsonPayload, fileChunk);
    }
    strcat(jsonPayload, "}}");

    char httpRequest[5120];
    sprintf(httpRequest, "POST /api/localsend/v2/prepare-upload HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
            ctx->targetIP, (int)strlen(jsonPayload), jsonPayload);

    SendHttpPayload(sock, tls, httpRequest, strlen(httpRequest), useTls);

    char rxPlaintext[8192] = {0};
    int rxBytes = ReadHttpPlaintext(sock, tls, rxPlaintext, sizeof(rxPlaintext), useTls);

    if (rxBytes > 0 && strstr(rxPlaintext, "HTTP/1.1 401") != NULL) {
        if (useTls) { TlsFreeSocket(tls); tls = NULL; }
        closesocket(sock);

        PinRequest req;
        req.targetName = ctx->targetIP;
        req.success = false;
        req.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (req.hEvent) {
            PostMessage(g_hWndMain, WM_REQUEST_PIN, 0, (LPARAM)&req);
            WaitForSingleObject(req.hEvent, INFINITE);
            CloseHandle(req.hEvent);
        }

        if (!req.success) {
            if (g_hWndMain) {
                PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString("PIN required but not provided."));
                PostMessage(g_hWndMain, WM_SEND_DONE, FALSE, 0);
            }
            free(ctx); return 1;
        }

        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connect(sock, (SOCKADDR*)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR) {
            if (g_hWndMain) {
                PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString("Retry connection failed."));
                PostMessage(g_hWndMain, WM_SEND_DONE, FALSE, 0);
            }
            free(ctx); return 1;
        }

        if (useTls) {
            tls = TlsConnect(sock, ctx->targetIP);
            if (!tls) {
                if (g_hWndMain) {
                    PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString("Retry TLS Handshake rejected."));
                    PostMessage(g_hWndMain, WM_SEND_DONE, FALSE, 0);
                }
                closesocket(sock); free(ctx); return 1;
            }
        }

        sprintf(httpRequest, "POST /api/localsend/v2/prepare-upload?pin=%s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
                req.pinCode, ctx->targetIP, (int)strlen(jsonPayload), jsonPayload);

        SendHttpPayload(sock, tls, httpRequest, strlen(httpRequest), useTls);
        rxBytes = ReadHttpPlaintext(sock, tls, rxPlaintext, sizeof(rxPlaintext), useTls);
    }

    if (useTls) { TlsFreeSocket(tls); tls = NULL; }
    closesocket(sock);

    if (rxBytes <= 0 || strstr(rxPlaintext, "HTTP/1.1 200") == NULL) {
        if (g_hWndMain) {
            PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString("Receiver rejected the request or invalid PIN."));
            PostMessage(g_hWndMain, WM_SEND_DONE, FALSE, 0);
        }
        free(ctx); return 1;
    }

    char serverSessionId[128] = {0};
    char* sessPtr = strstr(rxPlaintext, "\"sessionId\":\"");
    if (sessPtr) {
        sessPtr += 13;
        char* sessEnd = strchr(sessPtr, '"');
        if (sessEnd && (sessEnd - sessPtr) < 127) {
            strncpy(serverSessionId, sessPtr, sessEnd - sessPtr);
            serverSessionId[sessEnd - sessPtr] = '\0';
        }
    }

    if (strlen(serverSessionId) == 0) {
        if (g_hWndMain) {
            PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString("Transfer canceled: invalid SessionId."));
            PostMessage(g_hWndMain, WM_SEND_DONE, FALSE, 0);
        }
        free(ctx); return 1;
    }

    FileTokenMap fileTokens[50];
    memset(fileTokens, 0, sizeof(fileTokens));

    char* filesBlock = strstr(rxPlaintext, "\"files\":{");
    if (filesBlock) {
        for (int i = 0; i < ctx->fileCount; i++) {
            char searchStr[150];
            sprintf(searchStr, "\"%s\":\"", ctx->files[i].fileId);
            char* tokPtr = strstr(filesBlock, searchStr);
            if (tokPtr) {
                tokPtr += strlen(searchStr);
                char* tokEnd = strchr(tokPtr, '"');
                if (tokEnd) {
                    strncpy(fileTokens[i].fileId, ctx->files[i].fileId, 127);
                    strncpy(fileTokens[i].token, tokPtr, tokEnd - tokPtr);
                    fileTokens[i].token[tokEnd - tokPtr] = '\0';
                }
            }
        }
    }

    for (int i = 0; i < ctx->fileCount; i++) {
        char currentToken[128] = "";
        for (int j = 0; j < ctx->fileCount; j++) {
            if (strcmp(fileTokens[j].fileId, ctx->files[i].fileId) == 0) {
                strcpy(currentToken, fileTokens[j].token); break;
            }
        }

        if (strlen(currentToken) == 0) continue;

        if (g_hWndMain) {
            char statusMsg[512]; _snprintf(statusMsg, sizeof(statusMsg), "Uploading: %s...", ctx->files[i].fileName);
            PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 1, (LPARAM)AllocateString(statusMsg));
        }

        FILE* f = fopen(ctx->files[i].filePath, "rb");
        if (!f) continue;

        SOCKET uploadSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connect(uploadSock, (SOCKADDR*)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR) { fclose(f); continue; }

        TlsSocket* uploadTls = NULL;
        if (useTls) {
            uploadTls = TlsConnect(uploadSock, ctx->targetIP);
            if (!uploadTls) { closesocket(uploadSock); fclose(f); continue; }
        }

        char uploadHeader[1024];
        sprintf(uploadHeader, "POST /api/localsend/v2/upload?sessionId=%s&fileId=%s&token=%s HTTP/1.1\r\n"
                              "Host: %s\r\n"
                              "Content-Type: application/octet-stream\r\n"
                              "Content-Length: %lld\r\n"
                              "Connection: close\r\n\r\n",
                serverSessionId, ctx->files[i].fileId, currentToken, ctx->targetIP, ctx->files[i].fileSize);

        SendHttpPayload(uploadSock, uploadTls, uploadHeader, strlen(uploadHeader), useTls);

        char readBuf[8192];
        int bytesRead = 0;
        long long totalSent = 0;

        while ((bytesRead = (int)fread(readBuf, 1, sizeof(readBuf), f)) > 0) {
            if (useTls) {
                if (TlsWrite(uploadTls, readBuf, bytesRead) <= 0) break;
            } else {
                if (send(uploadSock, readBuf, bytesRead, 0) == SOCKET_ERROR) break;
            }

            totalSent += bytesRead;

            if (g_hWndMain && ctx->files[i].fileSize > 0) {
                int pct = (int)((totalSent * 100) / ctx->files[i].fileSize);
                HWND hProg = GetDlgItem(g_hWndMain, IDC_SEND_PROGRESS);
                if (hProg) SendMessage(hProg, PBM_SETPOS, pct, 0);
            }
        }
        fclose(f);

        char finalPlain[1024] = {0};
        ReadHttpPlaintext(uploadSock, uploadTls, finalPlain, sizeof(finalPlain), useTls);

        if (useTls) { TlsFreeSocket(uploadTls); uploadTls = NULL; }
        closesocket(uploadSock);
    }

    free(ctx);
    if (g_hWndMain) {
        PostMessage(g_hWndMain, WM_SEND_DONE, TRUE, 0);
    }
    return 0;
}
