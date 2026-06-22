#include "tls_layer.h"
#include "cert.h"
#include <stdio.h>
#include <stdlib.h>

#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>
#include <schnlsp.h>

struct TlsSocket {
    SOCKET sock;
    CtxtHandle hContext;
    bool hasContext;
    char leftOverBuffer[32768];
    DWORD cbLeftOver;
};

static CredHandle g_hCredClient;
static CredHandle g_hCredServer;
static bool g_bCredClientInit = false;
static bool g_bCredServerInit = false;

// Sets up default client and server credentials for Schannel, including a dynamically generated self-signed certificate for local authentication.
bool TlsInitGlobal() {
    // Client Credential
    SCHANNEL_CRED schannelCred = {0};
    schannelCred.dwVersion = SCHANNEL_CRED_VERSION;
    schannelCred.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
    schannelCred.dwFlags = SCH_CRED_MANUAL_CRED_VALIDATION;

    TimeStamp tsExpiry;
    if (AcquireCredentialsHandleA(NULL, UNISP_NAME, SECPKG_CRED_OUTBOUND, NULL, &schannelCred, NULL, NULL, &g_hCredClient, &tsExpiry) == SEC_E_OK) {
        g_bCredClientInit = true;
    }

    // Server Credential (cert.c dynamic cert binding)
    PCCERT_CONTEXT pCert = CreateSelfSignedCertificate();
    if (pCert) {
        SCHANNEL_CRED schServerCred = {0};
        schServerCred.dwVersion = SCHANNEL_CRED_VERSION;
        schServerCred.cCreds = 1;
        schServerCred.paCred = &pCert;
        schServerCred.grbitEnabledProtocols = SP_PROT_TLS1_2_SERVER;

        if (AcquireCredentialsHandleA(NULL, UNISP_NAME, SECPKG_CRED_INBOUND, NULL, &schServerCred, NULL, NULL, &g_hCredServer, &tsExpiry) == SEC_E_OK) {
            g_bCredServerInit = true;
        }
        CertFreeCertificateContext(pCert);
    }
    return g_bCredClientInit && g_bCredServerInit;
}

void TlsCleanupGlobal() {
    if (g_bCredClientInit) { FreeCredentialsHandle(&g_hCredClient); g_bCredClientInit = false; }
    if (g_bCredServerInit) { FreeCredentialsHandle(&g_hCredServer); g_bCredServerInit = false; }
}

// Executes the client-side Schannel handshake logic by exchanging security tokens with the remote host until a secure context is established.
static bool RunSchannelClientHandshake(SOCKET serverSocket, CtxtHandle* phContext, const char* targetHost) {
    SecBufferDesc inBufferDesc, outBufferDesc;
    SecBuffer inBuffers[2], outBuffers[1];
    DWORD dwSSPIFlags = ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT | ASC_REQ_CONFIDENTIALITY | ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM;
    DWORD dwContextFlags;
    SECURITY_STATUS scRet;
    char ioBuffer[8192];
    DWORD cbIoBuffer = 0;

    outBuffers[0].pvBuffer = NULL; outBuffers[0].cbBuffer = 0; outBuffers[0].BufferType = SECBUFFER_TOKEN;
    outBufferDesc.cBuffers = 1; outBufferDesc.pBuffers = outBuffers; outBufferDesc.ulVersion = SECBUFFER_VERSION;

    scRet = InitializeSecurityContextA(&g_hCredClient, NULL, (char*)targetHost, dwSSPIFlags, 0, SECURITY_NATIVE_DREP, NULL, 0, phContext, &outBufferDesc, &dwContextFlags, NULL);
    if (scRet != SEC_I_CONTINUE_NEEDED) return false;

    if (outBuffers[0].cbBuffer != 0 && outBuffers[0].pvBuffer != NULL) {
        send(serverSocket, (char*)outBuffers[0].pvBuffer, outBuffers[0].cbBuffer, 0);
        FreeContextBuffer(outBuffers[0].pvBuffer);
    }

    while (scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE) {
        if (cbIoBuffer == 0 || scRet == SEC_E_INCOMPLETE_MESSAGE) {
            int bytesRead = recv(serverSocket, ioBuffer + cbIoBuffer, sizeof(ioBuffer) - cbIoBuffer, 0);
            if (bytesRead <= 0) return false;
            cbIoBuffer += bytesRead;
        }

        inBuffers[0].pvBuffer = ioBuffer; inBuffers[0].cbBuffer = cbIoBuffer; inBuffers[0].BufferType = SECBUFFER_TOKEN;
        inBuffers[1].pvBuffer = NULL; inBuffers[1].cbBuffer = 0; inBuffers[1].BufferType = SECBUFFER_EMPTY;
        inBufferDesc.cBuffers = 2; inBufferDesc.pBuffers = inBuffers; inBufferDesc.ulVersion = SECBUFFER_VERSION;

        outBuffers[0].pvBuffer = NULL; outBuffers[0].cbBuffer = 0; outBuffers[0].BufferType = SECBUFFER_TOKEN;
        outBufferDesc.cBuffers = 1; outBufferDesc.pBuffers = outBuffers; outBufferDesc.ulVersion = SECBUFFER_VERSION;

        scRet = InitializeSecurityContextA(&g_hCredClient, phContext, (char*)targetHost, dwSSPIFlags, 0, SECURITY_NATIVE_DREP, &inBufferDesc, 0, phContext, &outBufferDesc, &dwContextFlags, NULL);

        if (scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_OK) {
            if (outBuffers[0].cbBuffer != 0 && outBuffers[0].pvBuffer != NULL) {
                send(serverSocket, (char*)outBuffers[0].pvBuffer, outBuffers[0].cbBuffer, 0);
                FreeContextBuffer(outBuffers[0].pvBuffer);
            }
            if (inBuffers[1].BufferType == SECBUFFER_EXTRA) {
                memmove(ioBuffer, ioBuffer + (cbIoBuffer - inBuffers[1].cbBuffer), inBuffers[1].cbBuffer);
                cbIoBuffer = inBuffers[1].cbBuffer;
            } else { cbIoBuffer = 0; }
        } else if (scRet != SEC_E_INCOMPLETE_MESSAGE) { return false; }
    }
    return (scRet == SEC_E_OK);
}

// Executes the server-side Schannel handshake logic by validating incoming client tokens and returning the server's signed credentials.
static bool RunSchannelServerHandshake(SOCKET clientSocket, CtxtHandle* phContext) {
    SecBufferDesc inBufferDesc, outBufferDesc;
    SecBuffer inBuffers[2], outBuffers[1];
    DWORD dwSSPIFlags = ASC_REQ_SEQUENCE_DETECT | ASC_REQ_REPLAY_DETECT | ASC_REQ_CONFIDENTIALITY | ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_STREAM;
    DWORD dwContextFlags;
    SECURITY_STATUS scRet = SEC_I_CONTINUE_NEEDED;
    char ioBuffer[8192];
    DWORD cbIoBuffer = 0;
    bool bFirstLoop = true;

    while (scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE) {
        if (cbIoBuffer == 0 || scRet == SEC_E_INCOMPLETE_MESSAGE) {
            int bytesRead = recv(clientSocket, ioBuffer + cbIoBuffer, sizeof(ioBuffer) - cbIoBuffer, 0);
            if (bytesRead <= 0) return false;
            cbIoBuffer += bytesRead;
        }

        inBuffers[0].pvBuffer = ioBuffer; inBuffers[0].cbBuffer = cbIoBuffer; inBuffers[0].BufferType = SECBUFFER_TOKEN;
        inBuffers[1].pvBuffer = NULL; inBuffers[1].cbBuffer = 0; inBuffers[1].BufferType = SECBUFFER_EMPTY;
        inBufferDesc.cBuffers = 2; inBufferDesc.pBuffers = inBuffers; inBufferDesc.ulVersion = SECBUFFER_VERSION;

        outBuffers[0].pvBuffer = NULL; outBuffers[0].cbBuffer = 0; outBuffers[0].BufferType = SECBUFFER_TOKEN;
        outBufferDesc.cBuffers = 1; outBufferDesc.pBuffers = outBuffers; outBufferDesc.ulVersion = SECBUFFER_VERSION;

        scRet = AcceptSecurityContext(
            &g_hCredServer,
            bFirstLoop ? NULL : phContext,
            &inBufferDesc,
            dwSSPIFlags,
            SECURITY_NATIVE_DREP,
            bFirstLoop ? phContext : NULL,
            &outBufferDesc,
            &dwContextFlags,
            NULL
        );

        bFirstLoop = false;

        if (scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_OK) {
            if (outBuffers[0].cbBuffer != 0 && outBuffers[0].pvBuffer != NULL) {
                send(clientSocket, (char*)outBuffers[0].pvBuffer, outBuffers[0].cbBuffer, 0);
                FreeContextBuffer(outBuffers[0].pvBuffer);
            }
            if (inBuffers[1].BufferType == SECBUFFER_EXTRA) {
                memmove(ioBuffer, ioBuffer + (cbIoBuffer - inBuffers[1].cbBuffer), inBuffers[1].cbBuffer);
                cbIoBuffer = inBuffers[1].cbBuffer;
            } else { cbIoBuffer = 0; }
        } else if (scRet != SEC_E_INCOMPLETE_MESSAGE) { return false; }
    }
    return (scRet == SEC_E_OK);
}

TlsSocket* TlsConnect(SOCKET sock, const char* targetIP) {
    if (!g_bCredClientInit) return NULL;
    TlsSocket* tls = (TlsSocket*)malloc(sizeof(TlsSocket));
    if (!tls) return NULL;
    memset(tls, 0, sizeof(TlsSocket));
    tls->sock = sock;

    if (!RunSchannelClientHandshake(sock, &tls->hContext, targetIP)) {
        free(tls);
        return NULL;
    }
    tls->hasContext = true;
    return tls;
}

TlsSocket* TlsAccept(SOCKET clientSock) {
    if (!g_bCredServerInit) return NULL;
    TlsSocket* tls = (TlsSocket*)malloc(sizeof(TlsSocket));
    if (!tls) return NULL;
    memset(tls, 0, sizeof(TlsSocket));
    tls->sock = clientSock;

    if (!RunSchannelServerHandshake(clientSock, &tls->hContext)) {
        free(tls);
        return NULL;
    }
    tls->hasContext = true;
    return tls;
}

// Reads encrypted packets from the socket, passing them through DecryptMessage to fill the caller's output buffer with plaintext data.
int TlsRead(TlsSocket* tls, char* outBuffer, int maxLen) {
    if (!tls || !tls->hasContext) return -1;

    char encBuffer[32768];
    DWORD cbIoBuffer = 0;
    int totalPlaintext = 0;

    if (tls->cbLeftOver > 0) {
        memcpy(encBuffer, tls->leftOverBuffer, tls->cbLeftOver);
        cbIoBuffer = tls->cbLeftOver;
        tls->cbLeftOver = 0;
    }

    bool keepDecrypting = true;
    while (keepDecrypting) {
        if (cbIoBuffer == 0) {
            int bytesRead = recv(tls->sock, encBuffer, sizeof(encBuffer), 0);
            if (bytesRead <= 0) return totalPlaintext > 0 ? totalPlaintext : bytesRead;
            cbIoBuffer = bytesRead;
        }

        SecBuffer msgBuffers[4] = {
            { cbIoBuffer, SECBUFFER_DATA, encBuffer },
            { 0, SECBUFFER_EMPTY, NULL },
            { 0, SECBUFFER_EMPTY, NULL },
            { 0, SECBUFFER_EMPTY, NULL }
        };
        SecBufferDesc msgDesc = { SECBUFFER_VERSION, 4, msgBuffers };

        SECURITY_STATUS decRet = DecryptMessage(&tls->hContext, &msgDesc, 0, NULL);
        if (decRet == SEC_E_OK) {
            char* clearData = NULL;
            DWORD clearLen = 0;
            for (int i = 0; i < 4; i++) {
                if (msgBuffers[i].BufferType == SECBUFFER_DATA) {
                    clearData = (char*)msgBuffers[i].pvBuffer;
                    clearLen = msgBuffers[i].cbBuffer;
                    break;
                }
            }

            if (clearData && clearLen > 0) {
                if (totalPlaintext + (int)clearLen < maxLen) {
                    memcpy(outBuffer + totalPlaintext, clearData, clearLen);
                    totalPlaintext += clearLen;
                }
            }

            SecBuffer* pExtra = NULL;
            for (int i = 0; i < 4; i++) {
                if (msgBuffers[i].BufferType == SECBUFFER_EXTRA) pExtra = &msgBuffers[i];
            }

            if (pExtra && pExtra->cbBuffer > 0) {
                // Se c'è del buffer rimanente (per la prossima richiesta HTTP keep-alive)
                if (totalPlaintext > 0) {
                    // Salviamo nei leftover del socket per la prossima chiamata TlsRead
                    memcpy(tls->leftOverBuffer, pExtra->pvBuffer, pExtra->cbBuffer);
                    tls->cbLeftOver = pExtra->cbBuffer;
                    break;
                } else {
                    memmove(encBuffer, pExtra->pvBuffer, pExtra->cbBuffer);
                    cbIoBuffer = pExtra->cbBuffer;
                }
            } else {
                cbIoBuffer = 0;
                keepDecrypting = false;
            }
        }
        else if (decRet == SEC_E_INCOMPLETE_MESSAGE) {
            int bytesRead = recv(tls->sock, encBuffer + cbIoBuffer, sizeof(encBuffer) - cbIoBuffer, 0);
            if (bytesRead <= 0) return totalPlaintext > 0 ? totalPlaintext : bytesRead;
            cbIoBuffer += bytesRead;
        }
        else {
            return -1;
        }
    }
    return totalPlaintext;
}

// Packs the plaintext data into SSPI stream buffers, encrypts them using EncryptMessage, and writes the resulting cyphertext to the wire.
int TlsWrite(TlsSocket* tls, const char* message, int len) {
    if (!tls || !tls->hasContext) return -1;

    SecPkgContext_StreamSizes sizes;
    QueryContextAttributes(&tls->hContext, SECPKG_ATTR_STREAM_SIZES, &sizes);

    DWORD totalLen = sizes.cbHeader + len + sizes.cbTrailer;
    char* encBuffer = (char*)malloc(totalLen);
    if (!encBuffer) return -1;

    memcpy(encBuffer + sizes.cbHeader, message, len);

    SecBuffer outBuffers[4] = {
        { sizes.cbHeader, SECBUFFER_STREAM_HEADER, encBuffer },
        { (DWORD)len, SECBUFFER_DATA, encBuffer + sizes.cbHeader },
        { sizes.cbTrailer, SECBUFFER_STREAM_TRAILER, encBuffer + sizes.cbHeader + len },
        { 0, SECBUFFER_EMPTY, NULL }
    };
    SecBufferDesc outDesc = { SECBUFFER_VERSION, 4, outBuffers };

    int ret = -1;
    if (EncryptMessage(&tls->hContext, 0, &outDesc, 0) == SEC_E_OK) {
        int totalEncBytes = outBuffers[0].cbBuffer + outBuffers[1].cbBuffer + outBuffers[2].cbBuffer;
        if (send(tls->sock, encBuffer, totalEncBytes, 0) != SOCKET_ERROR) {
            ret = len;
        }
    }
    free(encBuffer);
    return ret;
}

void TlsFreeSocket(TlsSocket* tls) {
    if (tls) {
        if (tls->hasContext) {
            DeleteSecurityContext(&tls->hContext);
        }
        free(tls);
    }
}
