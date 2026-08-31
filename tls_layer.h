#ifndef TLS_LAYER_H
#define TLS_LAYER_H

#include <winsock2.h>
#include <stdbool.h>

typedef struct TlsSocket TlsSocket;

bool TlsInitGlobal();
void TlsCleanupGlobal();
bool TlsIsAvailable(void);

TlsSocket* TlsConnect(SOCKET sock, const char* targetIP);
TlsSocket* TlsAccept(SOCKET clientSock);
int TlsRead(TlsSocket* tls, char* outBuffer, int maxLen);
int TlsWrite(TlsSocket* tls, const char* message, int len);
void TlsFreeSocket(TlsSocket* tls);

#endif
