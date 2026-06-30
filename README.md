# LocalSend32
LocalSend32 is a lightweight C/Win32 client for the LocalSend protocol, made specifically for Windows versions to be completely native with the system. It's completely derived from LocalSend RT, the project in this repository on the main branch.

It's built entirely using C11 and native Win32 controls; it uses system DLL calls for icons and native APIs.

It integrates the LocalSend v2 protocol using OpenSSL. Settings are saved on an .ini file in the same location as the executable, and are loaded/modified dynamically.

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
The app, as said, implements the LocalSend v2 protocol: https://github.com/localsend/protocol. Everything about networking is identical between the two versions, except for:
- When sending files, LocalSend32 initiates an OpenSSl client handshake
- When receiving files, OpenSSL generates a RSA-2048 keypair/X509 cert in-memory

# Compiling
The project is compiled using the `LLVM-MinGW` toolchain (UCRT) which support compiling to ARMv7 and x86 architectures. Compilation has been tested/made from a Linux and macOS host.

The x86 version is meant only as a "fun extra"; it's not tested actively, but it should work without any problem on Windows XP SP3+. Requires dynamic linking to OpenSSL, using *libcrypto-3.dll* and *libssl-3.dll* from the DiscordMessenger's fork of OpenSSL: https://github.com/DiscordMessenger/openssl. These two DLLs must be placed in the same folder as the exe itself.

The files for the 32-bit version are the same as the RT version, so get the files from the main branch. Tested on Wine with Fedora, working fine.

## x86 build
As for the RT build, first compile the manifest for the theme (which is different between the two versions):
```bash
  i686-w64-mingw32-windres -o manifest_x86.o manifest.rc
```

Then compile the project itself:
```bash
i686-w64-mingw32-gcc -O2 -Wall       main.c network_tcp.c network_tx.c network_udp.c tls_openssl.c utils.c manifest_x86.o       -o LocalSend32.exe       -I/home/fratta/Scaricati/openssl-stuff/include       -L/home/fratta/Scaricati/openssl-stuff       -lssl -lcrypto -lws2_32 -lcomctl32 -lshlwapi -lole32 -luuid -lcomdlg32       -mwindows -D_WIN32_WINNT=0x0500
```

## Compatibility
Given you use dm's OpenSSL, it should work on any PC running Windows 2000 or superior. I haven't tried the networking part on an older system, but theoretically it shouldn't have any problem.
<img width="1124" height="1038" alt="Schermata del 2026-06-30 23-19-45" src="https://github.com/user-attachments/assets/a823abe6-17a6-4902-ad27-f34bdefc3a2b" />

