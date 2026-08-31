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

// Copies string to heap to post to window procedure
char* AllocateString(const char* str) {
    char* newStr = (char*)malloc(strlen(str) + 1);
    if (newStr) strcpy(newStr, str);
    return newStr;
}

// Reads and decrypts HTTP response
static int ReadHttpPlaintext(SOCKET sock, TlsSocket* tls, char* outBuffer, int maxLen, bool useTls) {
    int total = 0;
    outBuffer[0] = '\0';
    // Set 10s receive timeout to prevent hangs if the remote device drops off
    DWORD timeout = 10000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    // Read HTTP headers until the end sequence \r\n\r\n is found
    while (total < maxLen - 1) {
        if (strstr(outBuffer, "\r\n\r\n") != NULL) {
            break;
        }
        int bytes = 0;
        if (useTls) {
            bytes = TlsRead(tls, outBuffer + total, maxLen - 1 - total);
        } else {
            bytes = recv(sock, outBuffer + total, maxLen - 1 - total, 0);
        }
        if (bytes <= 0) break;
        total += bytes;
        outBuffer[total] = '\0';
    }

    // Look for Content-Length in headers to determine how many body bytes to read
    char* bodyStart = strstr(outBuffer, "\r\n\r\n");
    if (bodyStart) {
        bodyStart += 4;
        int headersLen = (int)(bodyStart - outBuffer);
        char* contentLengthPtr = strstr(outBuffer, "Content-Length:");
        if (!contentLengthPtr) contentLengthPtr = strstr(outBuffer, "content-length:");
        if (contentLengthPtr) {
            int contentLength = atoi(contentLengthPtr + 15);
            // Continue reading until the expected body is complete
            while (total < headersLen + contentLength && total < maxLen - 1) {
                int bytes = 0;
                if (useTls) {
                    bytes = TlsRead(tls, outBuffer + total, maxLen - 1 - total);
                } else {
                    bytes = recv(sock, outBuffer + total, maxLen - 1 - total, 0);
                }
                if (bytes <= 0) break;
                total += bytes;
                outBuffer[total] = '\0';
            }
        } else {
            // If no Content-Length is present, read until connection is closed
            while (total < maxLen - 1) {
                int bytes = 0;
                if (useTls) {
                    bytes = TlsRead(tls, outBuffer + total, maxLen - 1 - total);
                } else {
                    bytes = recv(sock, outBuffer + total, maxLen - 1 - total, 0);
                }
                if (bytes <= 0) break;
                total += bytes;
                outBuffer[total] = '\0';
            }
        }
    }
    return total;
}

// Sends HTTP payload
static bool SendHttpPayload(SOCKET sock, TlsSocket* tls, const char* message, int len, bool useTls) {
    if (useTls) {
        return TlsWrite(tls, message, len) > 0;
    } else {
        return send(sock, message, len, 0) != SOCKET_ERROR;
    }
}

// Background worker thread executing file send sessions to one or multiple targets
DWORD WINAPI StartSendSessionThread(LPVOID lpParam) {
    SendSessionContext* ctx = (SendSessionContext*)lpParam;
    if (!ctx) return 1;

    bool overallSuccess = true;
    g_bCancelSendSession = false;

    printf("[Sender] StartSendSessionThread started. Target count = %d, File count = %d\n", ctx->targetCount, ctx->fileCount);

    // Iterate through all selected target devices
    for (int t = 0; t < ctx->targetCount; t++) {
        if (g_bCancelSendSession) {
            overallSuccess = false;
            break;
        }
        bool useTls = ctx->targets[t].isHttps && (g_EnableEncryption != 0) && TlsIsAvailable();
        char* alias = ctx->targets[t].alias;
        char* targetIP = ctx->targets[t].ipAddress;
        int targetPort = ctx->targets[t].port;
        
        char statusMsg[512];
        if (ctx->targetCount > 1) {
            char fmtTarget[256];
            _snprintf(fmtTarget, sizeof(fmtTarget), "%s (%d/%d)", alias, t + 1, ctx->targetCount);
            _snprintf(statusMsg, sizeof(statusMsg), g_Lang.msgConnectingTo, fmtTarget);
        } else {
            _snprintf(statusMsg, sizeof(statusMsg), g_Lang.msgConnectingTo, alias);
        }
        if (g_hWndMain) {
            PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 1, (LPARAM)AllocateString(statusMsg));
        }

        printf("[Sender] Connecting to %s at %s:%d...\n", alias, targetIP, targetPort);

        // Open TCP connection to target device
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in targetAddr = {0};
        targetAddr.sin_family = AF_INET; 
        targetAddr.sin_port = htons(targetPort); 
        targetAddr.sin_addr.s_addr = inet_addr(targetIP);

        if (connect(sock, (SOCKADDR*)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR) {
            int wsaErr = WSAGetLastError();
            printf("[Sender] Connection to %s failed! WSA error = %d\n", alias, wsaErr);
            if (wsaErr == WSAECONNREFUSED) {
                _snprintf(statusMsg, sizeof(statusMsg), "%s: Connection refused (Device offline / port closed).", alias);
            } else if (wsaErr == WSAETIMEDOUT) {
                _snprintf(statusMsg, sizeof(statusMsg), "%s: Connection timed out (Unreachable).", alias);
            } else if (wsaErr == WSAEHOSTUNREACH) {
                _snprintf(statusMsg, sizeof(statusMsg), "%s: Host unreachable (Check network).", alias);
            } else {
                _snprintf(statusMsg, sizeof(statusMsg), "%s: Connection failed (Error %d).", alias, wsaErr);
            }
            overallSuccess = false;
            if (g_hWndMain) {
                PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString(statusMsg));
                Sleep(2500);
            }
            closesocket(sock); 
            continue;
        }

        printf("[Sender] Connected to %s successfully. useTls = %d\n", alias, useTls);

        // Perform TLS handshake if encryption is enabled
        TlsSocket* tls = NULL;
        if (useTls) {
            printf("[Sender] Attempting TLS handshake (TlsConnect) with %s...\n", alias);
            tls = TlsConnect(sock, targetIP);
            if (!tls) {
                _snprintf(statusMsg, sizeof(statusMsg), "%s: TLS handshake failed (Encryption error).", alias);
                overallSuccess = false;
                if (g_hWndMain) {
                    PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString(statusMsg));
                    Sleep(2500);
                }
                closesocket(sock); 
                continue;
            }
        }

        // Build prepare-upload JSON payload containing sender info and file metadata list
        char jsonPayload[4096] = {0};
        sprintf(jsonPayload, "{\"info\":{\"alias\":\"%s\",\"version\":\"2.0\",\"deviceModel\":\"%s\",\"deviceType\":\"%s\",\"fingerprint\":\"%s\",\"port\":%d,\"protocol\":\"%s\",\"download\":false},\"files\":{",
                g_MyDeviceName, g_DeviceModel, GetProtocolDeviceType(g_DeviceType), g_MyFingerprint, g_Port, useTls ? "https" : "http");

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
                targetIP, (int)strlen(jsonPayload), jsonPayload);

        printf("[Sender] Sending prepare-upload request to %s:\n%s\n", alias, httpRequest);
        SendHttpPayload(sock, tls, httpRequest, strlen(httpRequest), useTls);

        // Read target response to determine if PIN is required or transfer is accepted
        char rxPlaintext[8192] = {0};
        int rxBytes = ReadHttpPlaintext(sock, tls, rxPlaintext, sizeof(rxPlaintext), useTls);

        if (rxBytes > 0 && strstr(rxPlaintext, "HTTP/1.1 401") != NULL) {
            if (useTls) { TlsFreeSocket(tls); tls = NULL; }
            closesocket(sock);

            PinRequest req;
            req.targetName = targetIP;
            req.success = false;
            req.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (req.hEvent) {
                PostMessage(g_hWndMain, WM_REQUEST_PIN, 0, (LPARAM)&req);
                WaitForSingleObject(req.hEvent, INFINITE);
                CloseHandle(req.hEvent);
            }

            if (!req.success) {
                _snprintf(statusMsg, sizeof(statusMsg), "%s: PIN required but not entered.", alias);
                overallSuccess = false;
                if (g_hWndMain) {
                    PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString(statusMsg));
                    Sleep(2500);
                }
                continue;
            }

            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (connect(sock, (SOCKADDR*)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR) {
                _snprintf(statusMsg, sizeof(statusMsg), "%s: Retry connection failed.", alias);
                overallSuccess = false;
                if (g_hWndMain) {
                    PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString(statusMsg));
                    Sleep(2500);
                }
                continue;
            }

            if (useTls) {
                tls = TlsConnect(sock, targetIP);
                if (!tls) {
                    _snprintf(statusMsg, sizeof(statusMsg), "%s: Retry TLS Handshake failed.", alias);
                    overallSuccess = false;
                    if (g_hWndMain) {
                        PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString(statusMsg));
                        Sleep(2500);
                    }
                    closesocket(sock); 
                    continue;
                }
            }

            sprintf(httpRequest, "POST /api/localsend/v2/prepare-upload?pin=%s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
                    req.pinCode, targetIP, (int)strlen(jsonPayload), jsonPayload);

            SendHttpPayload(sock, tls, httpRequest, strlen(httpRequest), useTls);
            rxBytes = ReadHttpPlaintext(sock, tls, rxPlaintext, sizeof(rxPlaintext), useTls);
        }

        if (useTls) { TlsFreeSocket(tls); tls = NULL; }
        closesocket(sock);

        if (rxBytes > 0) {
            printf("[Sender] prepare-upload response from %s:\n%s\n", alias, rxPlaintext);
        }

        if (rxBytes <= 0) {
            _snprintf(statusMsg, sizeof(statusMsg), "%s: No response received (Connection closed).", alias);
            overallSuccess = false;
            if (g_hWndMain) {
                PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString(statusMsg));
                Sleep(2500);
            }
            continue;
        } else if (strstr(rxPlaintext, "HTTP/1.1 403") != NULL) {
            _snprintf(statusMsg, sizeof(statusMsg), "%s declined the transfer request.", alias);
            overallSuccess = false;
            if (g_hWndMain) {
                PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString(statusMsg));
                Sleep(2500);
            }
            continue;
        } else if (strstr(rxPlaintext, "HTTP/1.1 401") != NULL) {
            _snprintf(statusMsg, sizeof(statusMsg), "%s: Invalid or missing PIN code.", alias);
            overallSuccess = false;
            if (g_hWndMain) {
                PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString(statusMsg));
                Sleep(2500);
            }
            continue;
        } else if (strstr(rxPlaintext, "HTTP/1.1 200") == NULL) {
            _snprintf(statusMsg, sizeof(statusMsg), "%s rejected the transfer.", alias);
            overallSuccess = false;
            if (g_hWndMain) {
                PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString(statusMsg));
                Sleep(2500);
            }
            continue;
        }

        // Extract session ID and accepted per-file upload tokens from response
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
            _snprintf(statusMsg, sizeof(statusMsg), "%s sent invalid SessionId.", alias);
            overallSuccess = false;
            if (g_hWndMain) {
                PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 3, (LPARAM)AllocateString(statusMsg));
                Sleep(2000);
            }
            continue;
        }

        // Map individual file tokens returned by the server
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

        // Upload each queued file sequentially
        for (int i = 0; i < ctx->fileCount; i++) {
            if (g_bCancelSendSession) {
                overallSuccess = false;
                break;
            }
            char currentToken[128] = "";
            for (int j = 0; j < ctx->fileCount; j++) {
                if (strcmp(fileTokens[j].fileId, ctx->files[i].fileId) == 0) {
                    strcpy(currentToken, fileTokens[j].token); break;
                }
            }

            if (strlen(currentToken) == 0) continue;

            if (g_hWndMain) {
                _snprintf(statusMsg, sizeof(statusMsg), "[%d/%d] %s: Sending %s...", t + 1, ctx->targetCount, alias, ctx->files[i].fileName);
                PostMessage(g_hWndMain, WM_SEND_STATUS_UPDATE, 1, (LPARAM)AllocateString(statusMsg));
            }

            FILE* f = fopen(ctx->files[i].filePath, "rb");
            if (!f) continue;

            // Connect dedicated upload socket for file stream
            SOCKET uploadSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (connect(uploadSock, (SOCKADDR*)&targetAddr, sizeof(targetAddr)) == SOCKET_ERROR) { fclose(f); continue; }

            TlsSocket* uploadTls = NULL;
            if (useTls) {
                uploadTls = TlsConnect(uploadSock, targetIP);
                if (!uploadTls) { closesocket(uploadSock); fclose(f); continue; }
            }

            // HTTP POST upload header with sessionId and fileToken
            char uploadHeader[1024];
            sprintf(uploadHeader, "POST /api/localsend/v2/upload?sessionId=%s&fileId=%s&token=%s HTTP/1.1\r\n"
                                  "Host: %s\r\n"
                                  "Content-Type: application/octet-stream\r\n"
                                  "Content-Length: %lld\r\n"
                                  "Connection: close\r\n\r\n",
                    serverSessionId, ctx->files[i].fileId, currentToken, targetIP, ctx->files[i].fileSize);

            SendHttpPayload(uploadSock, uploadTls, uploadHeader, strlen(uploadHeader), useTls);

            char readBuf[8192];
            int bytesRead = 0;
            long long totalSent = 0;
            HWND hProg = g_hWndMain ? GetDlgItem(g_hWndMain, IDC_SEND_PROGRESS) : NULL;

            while ((bytesRead = (int)fread(readBuf, 1, sizeof(readBuf), f)) > 0) {
                if (g_bCancelSendSession) break;
                if (useTls) {
                    if (TlsWrite(uploadTls, readBuf, bytesRead) <= 0) break;
                } else {
                    if (send(uploadSock, readBuf, bytesRead, 0) == SOCKET_ERROR) break;
                }

                totalSent += bytesRead;

                if (hProg && ctx->files[i].fileSize > 0) {
                    int pct = (int)((totalSent * 100) / ctx->files[i].fileSize);
                    SendMessage(hProg, PBM_SETPOS, pct, 0);
                }
            }
            fclose(f);

            char finalPlain[1024] = {0};
            ReadHttpPlaintext(uploadSock, uploadTls, finalPlain, sizeof(finalPlain), useTls);

            if (useTls) { TlsFreeSocket(uploadTls); uploadTls = NULL; }
            closesocket(uploadSock);
        }
    }

    free(ctx);
    if (g_hWndMain) {
        PostMessage(g_hWndMain, WM_SEND_DONE, overallSuccess, 0);
    }
    return 0;
}
