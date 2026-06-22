#include "network_udp.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>

extern char g_MyDeviceName[];
extern char g_MyFingerprint[];

// Sends a multicast discovery packet to let other peers on the local network know our device details.
void sendDiscoveryShout(SOCKET mySocket) {
    struct sockaddr_in multicastAddress;
    multicastAddress.sin_family = AF_INET;
    multicastAddress.sin_port = htons(g_Port);
    multicastAddress.sin_addr.s_addr = inet_addr(g_MulticastAddr);

    char jsonShout[512];

    _snprintf(jsonShout, sizeof(jsonShout),
        "{\"alias\":\"%s\",\"version\":\"2.1\",\"deviceModel\":\"%s\",\"deviceType\":\"%s\",\"fingerprint\":\"%s\",\"port\":%d,\"announce\":true}",
        g_MyDeviceName, g_DeviceModel, g_DeviceType, g_MyFingerprint, g_Port
    );

    printf("Announcing device presence: %s (Hashtag: #%s)...\n", g_MyDeviceName, g_MyFingerprint);

    sendto(mySocket, jsonShout, (int)strlen(jsonShout), 0, (SOCKADDR *)&multicastAddress, sizeof(multicastAddress));
}

// Creates and binds a new UDP socket to listen on the LocalSend discovery port.
SOCKET createUdpSocket(){
    SOCKET mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (mySocket == INVALID_SOCKET) {
        printf("UDP socket creation error: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    int broadcastEnable = 1;
    setsockopt(mySocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable));

    struct sockaddr_in listeningAddress;
    listeningAddress.sin_family = AF_INET;
    listeningAddress.sin_port = htons(g_Port);
    listeningAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(mySocket, (SOCKADDR *)&listeningAddress, sizeof(listeningAddress)) == SOCKET_ERROR){
        printf("Can't bind UDP: %d\n", WSAGetLastError());
        closesocket(mySocket); return INVALID_SOCKET;
    }
    return mySocket;
}

// Joins the designated multicast group for device discovery, querying the local system host address to pick the right interface.
bool joinMulticastGroup(SOCKET mySocket) {
    struct ip_mreq multicastGroup;
    multicastGroup.imr_multiaddr.s_addr = inet_addr(g_MulticastAddr);
    multicastGroup.imr_interface.s_addr = htonl(INADDR_ANY);

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent *host = gethostbyname(hostname);
        if (host != NULL) {
            for (int i = 0; host->h_addr_list[i] != NULL; i++) {
                struct in_addr *addr = (struct in_addr *)host->h_addr_list[i];
                char *ipStr = inet_ntoa(*addr);
                if (strcmp(ipStr, "127.0.0.1") != 0) {
                    multicastGroup.imr_interface = *addr;
                    printf("Interface IP: %s\n", ipStr);
                    break;
                }
            }
        }
    }

    int result = setsockopt(mySocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicastGroup, sizeof(multicastGroup));
    if (result == SOCKET_ERROR) {
        printf("Error joining Multicast group: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

// Enters a blocking loop that listens for incoming UDP discovery packets, parsing peer info and reporting it back to the UI.
void startListeningLoop(SOCKET mySocket) {
    char buffer[1024];
    struct sockaddr_in sendingAddress;
    int sendingSize = sizeof(sendingAddress);
    RemoteDevice discoveredDevice;

    printf("\n==================================================\n");
#ifdef __arm__
    printf("  LocalSend RT Core\n");
#else
    printf("  LocalSend32 Core\n");
#endif
    printf("==================================================\n\n");

    sendDiscoveryShout(mySocket);

    while (1) {
        int byteReceived = recvfrom(mySocket, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&sendingAddress, &sendingSize);
        if (byteReceived <= 0) continue;

        buffer[byteReceived] = '\0';

        if (parseLocalSendJSON(buffer, &discoveredDevice)) {
            // Ignore loopback discovery echoes
            if (strcmp(discoveredDevice.fingerprint, g_MyFingerprint) == 0) {
                continue;
            }

            strncpy(discoveredDevice.ipAddress, inet_ntoa(sendingAddress.sin_addr), sizeof(discoveredDevice.ipAddress));

            // Notify UI
            if (g_hWndMain) {
                RemoteDevice* pDeviceCopy = (RemoteDevice*)malloc(sizeof(RemoteDevice));
                if (pDeviceCopy) {
                    memcpy(pDeviceCopy, &discoveredDevice, sizeof(RemoteDevice));
                    PostMessage(g_hWndMain, WM_DEVICE_DISCOVERED, 0, (LPARAM)pDeviceCopy);
                }
            }

            if (!discoveredDevice.announce) {
                printf("[Found Device] -> %s (%s)\n", discoveredDevice.alias, discoveredDevice.ipAddress);
            } else {
                printf("[Discovery Shout] from: %s (%s)\n", discoveredDevice.alias, discoveredDevice.ipAddress);

                char jsonAnswer[512];
                _snprintf(jsonAnswer, sizeof(jsonAnswer),
                    "{\"alias\":\"%s\",\"version\":\"2.1\",\"deviceModel\":\"%s\",\"deviceType\":\"%s\",\"fingerprint\":\"%s\",\"port\":%d,\"announce\":false}",
                    g_MyDeviceName, g_DeviceModel, g_DeviceType, g_MyFingerprint, g_Port
                );

                sendingAddress.sin_port = htons(discoveredDevice.port);
                sendto(mySocket, jsonAnswer, (int)strlen(jsonAnswer), 0, (SOCKADDR *)&sendingAddress, sendingSize);
            }
            printf("--------------------------------------------------\n");
        }
    }
}
