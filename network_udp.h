#ifndef NETWORK_UDP_H
#define NETWORK_UDP_H

#include <winsock2.h>
#include <stdbool.h>

#define PORT 53317
#define MULTICAST_IP "224.0.0.167"
#define MY_FINGERPRINT "surface_rt"

// Allocates and binds a local UDP socket for multicast discovery announcements.
SOCKET createUdpSocket();

// Configures socket options to join the LocalSend multicast group.
bool joinMulticastGroup(SOCKET mySocket);

// Enters the listening loop to handle incoming UDP broadcast discovery shouts.
void startListeningLoop(SOCKET mySocket);

#endif
