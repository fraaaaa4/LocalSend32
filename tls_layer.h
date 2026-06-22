#ifndef TLS_LAYER_H
#define TLS_LAYER_H

#include <winsock2.h>
#include <stdbool.h>

// Opaque structure wrapping the platform-specific SSL/TLS engine socket state (Schannel or OpenSSL).
typedef struct TlsSocket TlsSocket;

// Performs global setup of the active TLS backend (e.g. initializing OpenSSL contexts or Schannel credentials).
bool TlsInitGlobal();

// Releases globally allocated TLS backend resources when shutting down the application.
void TlsCleanupGlobal();

// Initiates a client-side SSL/TLS handshake over an established TCP socket.
TlsSocket* TlsConnect(SOCKET sock, const char* targetIP);

// Accept a client-initiated SSL/TLS handshake on an incoming connection.
TlsSocket* TlsAccept(SOCKET clientSock);

// Reads decrypted data from a secure connection into the provided buffer.
int TlsRead(TlsSocket* tls, char* outBuffer, int maxLen);

// Encrypts and transmits data over a secure connection.
int TlsWrite(TlsSocket* tls, const char* message, int len);

// Shuts down the secure session and frees the associated socket context structures.
void TlsFreeSocket(TlsSocket* tls);

#endif // TLS_LAYER_H

