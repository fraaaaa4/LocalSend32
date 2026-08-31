# Notes about this version
The version you've downloaded is **Version 2.0.0**, compiled on 1st September 2026.

As of this version, LocalSend32 works on x86-32, x86-64, ARM32 and ARM64, and all versions use OpenSSL. The versions are divided as follows:
- **LocalSend9x**: LocalSend for Windows NT 3.51/95/98/ME/NT 4/2000. 
	- *Note*: this version doesn't have certain UI graphical elements that other versions do. You need also to have OpenSSL compiled in the same folder
	- On Windows NT 3.51 it can have graphical issues for certain buttons, and doesn't properly support multi-device sending
	- On Windows NT 3.51 and Windows 95, you might need to disable TLS encryption in order to make it work
- **LocalSend32**: LocalSend for Vista or superior.
- **LocalSendRT**: LocalSend for Windows RT (8.0, 8.1, 10)
- **LocalSendARM**: LocalSend for Windows 10/11 ARM (ARM64)

The following new things have been introduced in this version:
- All versions now use OpenSSL due to a change in LocalSend 1.18, now needing TLS3 for https connections
- Now the tray icon has a right click menu where you can drop files in, and quickly select a device to send these files to
- Completed the full translation in italian and english (I did forget quite a few strings)
- Added Simplified Chinese translation (thanks BluestacksPenguin)
- Fixed a bug where it wouldn't see other devices on the network if you were through a VPN or mobile hotspot
- Now the app checks on RT on first run if it has correctly added the firewall rules or not
- Fixed a bug where the send files button was enabled only when clicking on a device, rather than selecting one
- Now you can cancel sending jobs
- Errors as to why something isn't sending or stuff is more descriptive
- You can choose to recursive add files and folders when adding a folder to send
- Network status in the Receive tab now change dynamically according to your network (so your IP isn't always the same, yk, kinda useful)
- Improved the code with more comments and a bit more separation
- Created versons for NT 3.51 and superior, and ARM64.
- This guide for new users

# Previous versions
This paragraph includes the changelog notes of all previous versions prior to this, taken directly from the GitHub repository.

## Version 1.1
The following new things have been introduced in this version:
- Multi-device sending of files, by selecting multiple devices under the Send tab with checkboxes
- Languages support in utils.c with the language selector under Settings
- LocalSend32 checks at startup if the OpenSSL DLLs are present in the folder
- Fixed a bug where some UI elements could become empty when not interacting with it
- Refactored main.c into more sub-files

## Version 1.0
The following things have been introduced in this version (over the pre-release):
- New icon
- Few bug fixes
