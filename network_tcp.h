#ifndef NETWORK_TCP_H
#define NETWORK_TCP_H

#include <winsock2.h>
#include <windows.h>

// Entry point for the main TCP server thread that listens for incoming file transfers.
DWORD WINAPI tcpServerThread(LPVOID lpParam);

#endif

