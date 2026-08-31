#ifndef NETWORK_UDP_H
#define NETWORK_UDP_H

#include <winsock2.h>
#include <stdbool.h>

#define PORT 53317
#define MULTICAST_IP "224.0.0.167"
#define MY_FINGERPRINT "surface_rt"

SOCKET createUdpSocket();
bool joinMulticastGroup(SOCKET mySocket);
DWORD WINAPI startListeningLoop(LPVOID lpParam);

#endif
