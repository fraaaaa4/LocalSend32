# LocalSend32
<img width="128" height="128" alt="Frame 10" src="https://github.com/user-attachments/assets/d96d232a-955c-4ef4-b3de-fa42f966880c" />

LocalSend32 is a lightweight C/Win32 client for the LocalSend protocol, offered in the following versions:
- **LocalSend9x**: version for Windows NT 3.50 or superior, x86-32
- **LocalSend32**: version for Windows 2000 or superior, x86-32
- **LocalSendRT**: version for Windows RT 8.x or superior, ARM32
- **LocalSendARM**: version for Windows 10/11 WoA, ARM64

It's built entirely using C11 and native Win32 controls; it uses system DLL calls for icons and native APIs. For more informations about compatibility of the different versions, check out the **Compatibility** section of this readme.

It integrates the LocalSend v2 protocol using OpenSSl for any version for TLS 1.3 handshakes, providing encrypted transfers via HTTPS. Settings are saved on an .ini file in the same location as the executable, and are loaded/modified dynamically. It also adds a rule in Windows Firewall to let the program

# Screenshots
<img width="775" height="384" alt="1" src="https://github.com/user-attachments/assets/a8ee1c3c-9f84-4623-bfbb-bd30867ac261" />
<img width="800" height="420" alt="2" src="https://github.com/user-attachments/assets/2bd3a78a-e0b0-42df-86af-cc309535f0fd" />
<img width="904" height="450" alt="3" src="https://github.com/user-attachments/assets/05c9032f-c3be-457a-b31f-0b0ad26be1e4" />

# Files structure
- cert.c/cert.h : generates self-signed certificates using NCrypt APIs for TLS.
- dialogs.c/dialogs.h: dialog window procedures, such as text input and manual IP input
- main.c : main GUI loop, message dispatching, window resizing
- manifest.rc/manifest.o : theme manifest
- network_tcp.c/network_tcp.h : handles incoming TCP server requests, for receiving files, using TlsSocket abstraction
- network_tx.c/network_tx.h : handless outbound TCP client connections, using TlsSocket astraction
- network_udp.c/network_udp.h: handles UDP multicast discovery beacons
- settings.c/settings.h : loading and saving settings from .ini file
- tls_layer.h : abstract TLS socket API definition
- tls_schannel.h : Windows Schannel/SSPI implementation (LocalSend RT)
- tls_openssl.c : OpenSSL TLS implementation (LocalSend32)
- ui_creator.c/ui_creator.h : creation and layout of Win32 GUI controls
- utils.c/utils.h : helper utilities, such as JSON parser, file type mapping, path helpers

# Networking stuff
The app, as said, implements the LocalSend v2 protocol: https://github.com/localsend/protocol. It integrates OpenSSL; versions prior to 2.0 used Schannel for the RT version, which may not work anymore with LocalSend 1.18 or superior. The networking part is divided essentially into two: UDP and TCP.

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
The project is compiled using the `LLVM-MinGW` toolchain (UCRT) which support compiling to ARMv7 and x86 architectures. Compilation has been tested/made from a Linux host. The compilation commands here are for Version 2.0 onwards.

|Project|Compiler|Manifest|Program|
|-|-|-|-|
|LocalSend32|i686-w64-mingw32-gcc|`i686-w64-mingw32-windres manifest.rc -o manifest_x86.o`|`i686-w64-mingw32-gcc -mwindows -O2 -Wall main.c dialogs.c settings.c ui_creator.c layout.c openssl_dyn.c network_tcp.c network_tx.c network_udp.c tls_openssl.c utils.c tray_menu.c manifest_x86.o -o LocalSend32.exe -Iinclude -lws2_32 -lcomctl32 -lole32 -luuid -lshell32 -lcomdlg32 -lgdi32`|
|LocalSend9x|i686-w64-mingw32-gcc|`i686-w64-mingw32-windres manifest_nt.rc -o manifest_nt.o`|`i686-w64-mingw32-gcc -mwindows -O2 -Wall -DLOCALSEND_NT main.c dialogs.c settings.c ui_creator.c layout.c openssl_dyn.c network_tcp.c network_tx.c network_udp.c tls_openssl.c utils.c tray_menu.c manifest_nt.o -o LocalSend9x.exe -Iinclude -lws2_32 -lcomctl32 -lole32 -luuid -lshell32 -lcomdlg32 -lgdi32 -Wl,--subsystem,windows:3.50,--major-os-version,3,--minor-os-version,50,--major-subsystem-version,3,--minor-subsystem-version,50`|
|LocalSendRT|armv7-w64-mingw32-clang|`armv7-w64-mingw32-windres manifest_arm.rc -o manifest_arm.o`|`armv7-w64-mingw32-clang -mwindows -O2 -Wall -D__arm__ main.c dialogs.c settings.c ui_creator.c layout.c openssl_dyn.c network_tcp.c network_tx.c network_udp.c tls_openssl.c utils.c tray_menu.c manifest_arm.o -o LocalSendRT.exe -Iinclude -lws2_32 -lcomctl32 -lole32 -luuid -lshell32 -lcomdlg32 -lgdi32`|
|LocalSendARM|aarch64-w64-mingw32-clang|`aarch64-w64-mingw32-windres manifest_arm64.rc -o manifest_arm64.o`|`aarch64-w64-mingw32-clang -mwindows -O2 -Wall main.c dialogs.c settings.c ui_creator.c layout.c openssl_dyn.c network_tcp.c network_tx.c network_udp.c tls_openssl.c utils.c tray_menu.c manifest_arm64.o -o LocalSend64_ARM.exe -Iinclude -lws2_32 -lcomctl32 -lole32 -luuid -lshell32 -lcomdlg32 -lgdi32`|

Together with the program, from Version 2.0, there's also a chm file. The following commands are needed, listed for compiling on a Linux host:
- Install python3-sphinx, python3-myst-parser, and fp-utils with your package manager
- Open a Terminal and move to the `help` file of the project
- `make htmlhelp`
- `chmcmd build/htmlhelp/LocalSend32.hhp`
- Copy the help file in the main folder of the project


# Translations
Help translating the app! All strings are defined in the source code in utils.c inside the `g_Languages` array. To add a new language:
- open `utils.h` and add a new language identifier in the `LanguageId` enum right before `LANG_COUNT`:
   ```c
   typedef enum {
       LANG_EN = 0,
       LANG_IT,
       LANG_FR, // <- add a new language here
       LANG_COUNT
   } LanguageId;
   ```
- in the `g_Languages` array, append your translated block inside brackets. Of course, your structure should have all the strings of the other languages:
  ```c
   const LanguageStrings g_Languages[LANG_COUNT] = {
       // English (LANG_EN)
       { ... },
       // Italian (LANG_IT)
       { ... },
       // French (LANG_FR)
       {
           // Add here all the translated strings which should be the same as the others and in the same order
       }
   };
   ```
  - in `ui_creator.c` find the dropdown menu creation inside `CreatMainControls()`. Append the name of your language using `SendMessageA` with `CB_ADDSTRING`:
  ```c
   SendMessage(hWndComboLanguage, CB_ADDSTRING, 0, (LPARAM)"English");
   SendMessage(hWndComboLanguage, CB_ADDSTRING, 0, (LPARAM)"Italiano");
   SendMessage(hWndComboLanguage, CB_ADDSTRING, 0, (LPARAM)"Français"); // <--- Add this line
   ```
  The order in g_Languages should be the exact same that is in the dropdown menu of the languages!


# Compatibility
**It's important you check this before running any version of LocalSend32 to check what's the best version to use and what to use!**.

During testing, versions of OpenSSL from DiscordMessenger have been used. Head there to check out the OpenSSL version needed for your architecture and version of Windows.

WinSock2 NT 3.51 is used from this project: https://github.com/DaniElectra/winsock351.

When it says that TLS encryption may need to be disabled, it means that to ensure proper transfer to and from any device, disable TLS on any device. Otherwise, if TLS is enabled, usually it works between LocalSend32 clients and not with the official ones.

For versions of Windows that support both LocalSend9x and LocalSend32, we suggest the 32 version as it has a few additional graphical effects (such as groups in listboxes).

The app supports both OpenSSL 3.x and 4.x when available. Note that all tests have been made with 3.x. Files must be named as **libssl-3.dll** and **libcrypto-3.dll** (or with "4" if it's 4.x; I tested it only with 3.x though).

OpenSSL for ARM32 can be found in DiscordMessenger's RT port: https://github.com/ricol03/dm-Arm32

|Windows version|Architecture|LocalSend32 edition|Files needed|Works?|Tested?|Notes|
|-|-|-|-|-|-|-|
|Windows 3.x, Windows NT 3.1, Windows NT 3.50|-|x86-32|-|No|No|-|
|Windows NT 3.51|x86-32|LocalSend9x|OpenSSL 3.x for older versions of Windows, Winsock2 NT 3.51|Yes|Yes (VM)|Sends to all devices present in the "Nearby devices" list, TLS encryption may need to be disabled, icons are disabled, tray icon not available. <img width="106" height="80" alt="VirtualBox_Windows NT 3 5_29_08_2026_01_20_15" src="https://github.com/user-attachments/assets/9a5ad8f4-d644-4905-b875-d0dd89a9f606" />|
|Windows 95|x86-32|LocalSend9x|OpenSSL 3.x for older versions of Windows, DCOM95 OLE Update, Winsock 2 update, msvcrt.exe for 95|Yes|Yes (VM, on OSR2)|TLS encryption may need to be disabled, tray notifications not supported. <img width="106" height="80" alt="VirtualBox_Window 95_29_08_2026_01_23_57" src="https://github.com/user-attachments/assets/06026396-750c-493b-893d-173fabe9c2eb" />|
|Windows 98/ME|x86-32|LocalSend9x|OpenSSL 3.x for older versions of Windows|Yes|No|TLS encryption may need to be disabled, as behaviour should be similar to 95|
|Windows NT 4 SP4 or superior|x86-32|LocalSend9x|OpenSSL 3.x for older versions of Windows|Yes|Yes (VM)|Should work without any problem with TLS, tray notifications not supported. <img width="106" height="80" alt="VirtualBox_Windows NT 4_29_08_2026_01_31_12" src="https://github.com/user-attachments/assets/daedc5fd-d160-466e-a9c4-ee5f2135814a" />|
|Windows 2000|x86-32|LocalSend9x or 32|OpenSSL 3.x for older versions of Windows|Yes|No|Behaviour should be similar to NT 4|
|Windows XP|x86-32|LocalSend9x or 32|OpenSSL 3.x that supports XP|Yes|No|Behaviour should be similar to XP|
|Windows Vista/7/8.x/10/11|x86-32|LocalSend9x or 32|OpenSSL 3.x. that supports Vista, MSVCRT|Yes|No|Behaviour should be similar to XP|
|Windows RT 8.x/Windows RT 10|ARM32|LocalSendRT|OpenSSL for ARM32, MSVCRT for ARM32|Yes|Yes (RT 1)|<img width="455" height="256" alt="immagine_appunti" src="https://github.com/user-attachments/assets/29941edf-9f87-4738-95d6-4bff6fb6f5c0" />|
|Windows 10/11 WoA|ARM64|LocalSendARM|OpenSSL for ARM64|Yes|No|-|
|Wine|x86-32|LocalSend9x or 32|OpenSSL 3.x 32-bit Windows DLLs|Yes|Yes (Wine on Fedora 44, Whisky on macOS 27)|It may need Status: offline (error), but it's working fine; icons may not be right due to DLL differencies for icons. <img width="572" height="260" alt="image" src="https://github.com/user-attachments/assets/70eb0133-8783-4f61-8f3e-97891eba0c6d" />|


## Notes
- When opening the app for the first time, it creates an .ini file containing the preferences of the app. So, for cleanliness, I suggest you put the exe in Program Files, and then make the shortcut to it
- If you get the error "The program can't start because api-ms-win-crt-private-l1-1-0.dll is missing", install the VC Redistributables for ARM32 
- The first time you download this app, it is **imperative** that you run it as Administrator at least once. This is because the RT firewall is much more strict than normal Windows, so LocalSend creates custom rules for connecting to other devices; without running as Administrator, the rules aren't created, and the app doesn't work
- Starting from Version 1.18 of the LocalSend official client, in order to send files you need to use LocalSend 2.0 or superior
