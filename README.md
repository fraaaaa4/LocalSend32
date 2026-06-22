# LocalSend RT
LocalSend RT is a lightweight C/Win32 client for the LocalSend protocol, made specifically for Windows RT ARMv7 devices. 

It's built entirely using C11 and native Win32 controls; it uses system DLL calls for icons and native APIs.

It integrates the LocalSend v2 protocol using the Windows Secure Channel API for TLS 1.2 handshakes, providing encrypted transfers via HTTPS. Settings are saved on an .ini file in the same location as the executable, and are loaded/modified dynamically. It also adds a rule in Windows Firewall to let the program

# Screenshots
<img width="775" height="384" alt="1" src="https://github.com/user-attachments/assets/a8ee1c3c-9f84-4623-bfbb-bd30867ac261" />
<img width="800" height="420" alt="2" src="https://github.com/user-attachments/assets/2bd3a78a-e0b0-42df-86af-cc309535f0fd" />
<img width="904" height="450" alt="3" src="https://github.com/user-attachments/assets/05c9032f-c3be-457a-b31f-0b0ad26be1e4" />

# Files structure
- cert.c/cert.h : generates self-signed certificates using NCrypt APIs for TLS.
- main.c : main GUI loop, settings parser, UI event dispatching
- manifest.rc/manifest.o : theme manifest
- network_tcp.c/network_tcp.h : handles incoming TCP server requests, for receiving files, using TlsSocket abstraction
- network_tx.c/network_tx.h : handless outbound TCP client connections, using TlsSocket astraction
- network_udp.c/network_udp.h: handles UDP multicast discovery beacons
- tls_layer,h : abstract TLS socket API definition
- tls_schannel.h : Windows Schannel/SSPI implementation (LocalSend RT)
- tls_openssl.c : OpenSSL TLS implementation (LocalSend32)
- utils.c/utils.h : helper utilities, such as JSON parser, file type mapping, path helpers

# Networking stuff
The app, as said, implements the LocalSend v2 protocol: https://github.com/localsend/protocol. It integrates the native Windows Sockets API and Schannel for security. The networking part is divided essentially into two: UDP and TCP.

## UDP discovery
This part is contained in `network_udp.c`. Listens and transmits on UDP port 53317 and multicast group 224.0.0.167. These values can be configured in Settings.

On startup, or when needed, the app broadcasts a UDP discovery shout containing a JSON representation of the device. The JSON contains data that's gotten directly from the settings from the app. An example of the message:
```json
    {
      "alias": "Surface RT",
      "version": "2.1",
      "deviceModel": "Surface RT",
      "deviceType": "Laptop",
      "fingerprint": "a1b2c3d4...",
      "port": 53317,
      "announce": true
    }
```
A background listener thread, startListeningLoop, constantly monitors UDP multicast/unicast on port 53317. 

About the "announce" property:
- if "announce" is true, it means another device is scanning the network. The listener replies via unicast UDP back to the sender
- if "announce is false, the device parameters received are parsed and mapped to a RemoteDevice structure, which then will be used in the app

## Sending files
This part is contained in `network_tx.c`. When files are queued and a target device is selected for sending:
- a socket connection is created to the target IP and port
- if encryption is enabled, it sets up TLS 1.2 using Schannel
- Sends a `POST /api/localsend/v2/prepare-upload` request carrying details about the sender and metadata for all the files in the payload
- if the receiver requires a PIN, on the sender device a dialog requesting the PIN will be shown to the user
- once accepted, the sender parses the JSON response containing the receiver-generated sessionId and matching security file-upload tokens
- for each file a separate TCO socket is established. The file is read from the local disk in chunks of 8MB, encrypted and then streamed to the socket

## Receiving files
This part is contained in `network_tcp.c`. A persistent background thread binds to the active port and listens for incoming connections.
- when a client connects, a new thread ClientThread is connected
- if encryption is enabled, a self-signed certificate is generated via NCrypt APIs and bound to the Schannel context
- the app parses the incoming JSON query parameters and files list
- If quick save is inactive, the app shows to the user a window with the infos of the incoming files, asking the user to accept or cancel the request, and where to save them
- generates a unique sessionId and maps target tokens to the file structures, repliyng with a 200 OK JSON response
- receives the stream, decrypts it in blocks, and writes the output directly to the chosen storage directory
- in case of interruption, the partial file is deleted from the disk

# Compiling
The project is compiled using the `LLVM-MinGW` toolchain (UCRT) which support compiling to ARMv7 and x86 architectures. Compilation has been tested/made from a Linux and macOS host.

## RT build
First compile the manifest for the theme:
```bash
  armv7-w64-mingw32-windres -o manifest.o manifest.rc
```

Then compile the project iself:
```bash
armv7-w64-mingw32-gcc -o LocalSendRT.exe \
    main.c cert.c network_tcp.c network_tx.c network_udp.c utils.c manifest.o \
    -lws2_32 -lsecur32 -lcomctl32 -lole32 -luuid -lgdi32 -lshlwapi -lcomdlg32 -lncrypt -lcrypt32 \
    -mwindows -O2
```
