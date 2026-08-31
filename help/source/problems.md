# Problems
Here are all the common problems and solutions to them.

|Problem|Version|Solution|
|-|-|-|
|"The program can't start because api-ms-win-crt-private-l1-1-0.dll is missing from your computer" error message|LocalSend32 or RT|Install the VC Redistributables 2017 for your corresponding architecture and version of Windows|
|I can send files from RT to another device, but not viceversa|LocalSend RT|If you're having any networking problems with the RT version, try to run it as Administrator so it can add the needed firewall rules|
|I can't find my device on the Receive/Send tab|Any version|On the device running LocalSend32/NT/ARM/RT, head to the Send tab and hit the "Search again" button. Otherwise, close and reopen the app|
|I can't transfer or receive files|LocalSend9x|Disable TLS encryption in Settings|

## The app can't find the OpenSSL DLLs
If this error shows up on your device, copy the respective OpenSSL DLLs to the folder of the executable, based on your current computer architecture and version of Windows.

The files must be named as follows:
- **OpenSSL 3.x**:
    - they can be named as *libcrypto-3.dll* and *libssl-3.dll*, or according to your CPU architecture
        - *libcrypto-3-arm.dll* and *libssl-3.dll* for Windows RT/ARM32
        - *libcrypto-3-arm64.dll* and *libssl-3-arm64.dll* for WoA/ARM64
- **OpenSSL 4.x**:
    - they can be named as *libcrypto-4.dll* and *libssl-4.dll*, or according to your CPU architecutre with the respective suffixes (-arm, -arm64)
