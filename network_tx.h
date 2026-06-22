#ifndef NETWORK_TX_H
#define NETWORK_TX_H

#include <winsock2.h>
#include <stdbool.h>

// Holds metadata for a single file scheduled for outbound transmission.
typedef struct {
    char filePath[512];
    char fileName[256];
    long long fileSize;
    char fileId[128];
} FileToSend;

// Contains all context needed to execute an outbound file transfer batch to a remote peer.
typedef struct {
    char targetIP[16];
    int targetPort;
    FileToSend files[50];
    int fileCount;
} SendSessionContext;

// Main entry point for the background sending thread, orchestrating the prepare-upload handshake and subsequent data streams.
DWORD WINAPI StartSendSessionThread(LPVOID lpParam);
#endif

