# First time running LocalSend32
Depending on the version of Windows you're running, you'll need to do different procedures. Don't worry, as LocalSend32 will check if everything is working correctly when it starts, and prompting you with the corresponding error.

## Windows RT
If you're running LocalSendRT on Windows RT, you'll need to do the following things:
- Install VC redistributables for ARM32: https://files.open-rt.party/Software/Redistributables/VCRedist/
- If this is the first time you're running it, run it as Administrator, so the app can add the custom UDP and TCP rules to the firewall. This is because RT has a much stricter firewall and security settings. These rules let LocalSend32 send and receive network requests
- the OpenSSL ARM32 files must be in the same folder as the executable, named as *libcrypto-3-arm.dll* and *libssl-3-arm.dll*. These **must** be the ARM32 version, which are available to download on the LocalSendRT GitHub page (copies compiled directly from DiscordMessenger's ARM32 port)
