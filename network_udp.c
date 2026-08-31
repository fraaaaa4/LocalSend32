#include "network_udp.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws2tcpip.h>

extern char g_MyDeviceName[];
extern char g_MyFingerprint[];

// Broadcasts presence on the network (multicast and interface-specific broadcasts)
void sendDiscoveryShout(SOCKET mySocket) {
    struct sockaddr_in multicastAddress = {0};
    multicastAddress.sin_family = AF_INET;
    multicastAddress.sin_port = htons(g_Port);
    multicastAddress.sin_addr.s_addr = inet_addr(g_MulticastAddr);

    char jsonShout[512];

    _snprintf(jsonShout, sizeof(jsonShout),
        "{\"alias\":\"%s\",\"version\":\"2.1\",\"deviceModel\":\"%s\",\"deviceType\":\"%s\",\"fingerprint\":\"%s\",\"port\":%d,\"protocol\":\"%s\",\"announce\":true}",
        g_MyDeviceName, g_DeviceModel, GetProtocolDeviceType(g_DeviceType), g_MyFingerprint, g_Port,
        (g_EnableEncryption != 0) ? "https" : "http"
    );

    printf("Announcing device presence: %s (Hashtag: #%s)...\n", g_MyDeviceName, g_MyFingerprint);

    sendto(mySocket, jsonShout, (int)strlen(jsonShout), 0, (SOCKADDR *)&multicastAddress, sizeof(multicastAddress));

    // Universal subnet broadcast shout
    struct sockaddr_in bcastGeneral = {0};
    bcastGeneral.sin_family = AF_INET;
    bcastGeneral.sin_port = htons(g_Port);
    bcastGeneral.sin_addr.s_addr = inet_addr("255.255.255.255");
    sendto(mySocket, jsonShout, (int)strlen(jsonShout), 0, (SOCKADDR *)&bcastGeneral, sizeof(bcastGeneral));

    // In broadcast for every network interface if supported
    INTERFACE_INFO InterfaceList[20];
    unsigned long nBytesReturned;
    if (WSAIoctl(mySocket, SIO_GET_INTERFACE_LIST, NULL, 0, &InterfaceList,
                 sizeof(InterfaceList), &nBytesReturned, NULL, NULL) != SOCKET_ERROR) {
        int nNumInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);
        for (int i = 0; i < nNumInterfaces; ++i) {
            u_long flags = InterfaceList[i].iiFlags;
            if ((flags & IFF_UP) && !(flags & IFF_LOOPBACK)) {
                struct sockaddr_in bcastAddr = {0};
                bcastAddr.sin_family = AF_INET;
                bcastAddr.sin_port = htons(g_Port);
                
                u_long ip = InterfaceList[i].iiAddress.AddressIn.sin_addr.s_addr;
                u_long mask = InterfaceList[i].iiNetmask.AddressIn.sin_addr.s_addr;
                // Calculate subnet broadcast address using subnet mask inversion (e.g. IP | ~Mask)
                bcastAddr.sin_addr.s_addr = ip | ~mask;

                sendto(mySocket, jsonShout, (int)strlen(jsonShout), 0, (SOCKADDR *)&bcastAddr, sizeof(bcastAddr));
            }
        }
    }
}

// Creates UDP socket
SOCKET createUdpSocket(){
    SOCKET mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (mySocket == INVALID_SOCKET) {
        printf("UDP socket creation error: %d\n", WSAGetLastError());
        return INVALID_SOCKET;
    }

    int reuse = 1;
    setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
    int broadcastEnable = 1;
    setsockopt(mySocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable));

    struct sockaddr_in listeningAddress = {0};
    listeningAddress.sin_family = AF_INET;
    listeningAddress.sin_port = htons(g_Port);
    listeningAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(mySocket, (SOCKADDR *)&listeningAddress, sizeof(listeningAddress)) == SOCKET_ERROR){
        printf("Can't bind UDP: %d\n", WSAGetLastError());
        closesocket(mySocket); return INVALID_SOCKET;
    }
    return mySocket;
}

// Joins multicast group on all active interfaces
bool joinMulticastGroup(SOCKET mySocket) {
    INTERFACE_INFO InterfaceList[20];
    unsigned long nBytesReturned;
    bool joinedAny = false;

    if (WSAIoctl(mySocket, SIO_GET_INTERFACE_LIST, NULL, 0, &InterfaceList,
                 sizeof(InterfaceList), &nBytesReturned, NULL, NULL) != SOCKET_ERROR) {
        int nNumInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);
        for (int i = 0; i < nNumInterfaces; ++i) {
            u_long flags = InterfaceList[i].iiFlags;
            if ((flags & IFF_UP) && !(flags & IFF_LOOPBACK)) {
                struct ip_mreq multicastGroup = {0};
                multicastGroup.imr_multiaddr.s_addr = inet_addr(g_MulticastAddr);
                multicastGroup.imr_interface = InterfaceList[i].iiAddress.AddressIn.sin_addr;
                
                int result = setsockopt(mySocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicastGroup, sizeof(multicastGroup));
                if (result != SOCKET_ERROR) {
                    char *ipStr = inet_ntoa(InterfaceList[i].iiAddress.AddressIn.sin_addr);
                    printf("Joined multicast group on interface: %s\n", ipStr);
                    joinedAny = true;
                }
            }
        }
    }

    if (!joinedAny) {
        struct ip_mreq multicastGroup = {0};
        multicastGroup.imr_multiaddr.s_addr = inet_addr(g_MulticastAddr);
        multicastGroup.imr_interface.s_addr = htonl(INADDR_ANY);
        int result = setsockopt(mySocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicastGroup, sizeof(multicastGroup));
        if (result != SOCKET_ERROR) {
            printf("Joined multicast group on default interface (INADDR_ANY)\n");
            joinedAny = true;
        }
    }

    return joinedAny;
}

DWORD WINAPI startListeningLoop(LPVOID lpParam) {
    SOCKET mySocket = (SOCKET)(uintptr_t)lpParam;
    char buffer[1024];
    struct sockaddr_in sendingAddress = {0};
    RemoteDevice discoveredDevice;

    printf("\n==================================================\n");
#ifdef __arm__
    printf("  LocalSend RT\n");
#else
    printf("  LocalSend32\n");
#endif
    printf("==================================================\n\n");
    fflush(stdout);

    sendDiscoveryShout(mySocket);
    fflush(stdout);

    while (1) {
        int sendingSize = sizeof(sendingAddress);
        int byteReceived = recvfrom(mySocket, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&sendingAddress, &sendingSize);
        if (byteReceived <= 0) continue;

        buffer[byteReceived] = '\0';

        if (parseLocalSendJSON(buffer, &discoveredDevice)) {
            // Ignore loopback discovery echoes
            if (strcmp(discoveredDevice.fingerprint, g_MyFingerprint) == 0) {
                continue;
            }
            printf("[UDP Debug] Loopback check failed. Mine: '%s', Discovered: '%s'\n", g_MyFingerprint, discoveredDevice.fingerprint);

            _snprintf(discoveredDevice.ipAddress, sizeof(discoveredDevice.ipAddress), "%s", inet_ntoa(sendingAddress.sin_addr));

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
                    "{\"alias\":\"%s\",\"version\":\"2.1\",\"deviceModel\":\"%s\",\"deviceType\":\"%s\",\"fingerprint\":\"%s\",\"port\":%d,\"protocol\":\"%s\",\"announce\":false}",
                    g_MyDeviceName, g_DeviceModel, GetProtocolDeviceType(g_DeviceType), g_MyFingerprint, g_Port,
                    (g_EnableEncryption != 0) ? "https" : "http"
                );

                sendingAddress.sin_port = htons(discoveredDevice.port);
                sendto(mySocket, jsonAnswer, (int)strlen(jsonAnswer), 0, (SOCKADDR *)&sendingAddress, sendingSize);
            }
            printf("--------------------------------------------------\n");
        }
    }
    return 0;
}
