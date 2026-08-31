#ifndef NETWORK_TCP_H
#define NETWORK_TCP_H

#include <winsock2.h>
#include <windows.h>

extern int g_TcpServerStatus;
DWORD WINAPI tcpServerThread(LPVOID lpParam);

#endif

