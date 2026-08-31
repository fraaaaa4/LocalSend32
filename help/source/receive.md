# Receiving files
To receive files, two or multiple devices must be connected to the same network.

On the LocalSend client which will send files to your device, go to the Send tab, and search your devices in the "Nearby devices" list.

Your device will be shown with the device name written on the Receive tab. Click on it on the device that will send files, and your LocalSend32 client will automatically get the transmission.

## Settings for receiving
Under the Settings > Receive tab, you can customise a few settings about receiving files.

First option is what to do when you receive a file; you can choose to:
- save files in the same path as your executable (e.g. if the LocalSend32 executable is in "C:\Program Files\LocalSend", then any received file will be saved in that directory)
- save files to your Downloads folder
    - on Windows 95/98/ME files will be saved in **C:\Downloads**
    - on Windows NT 3.51 and 4 files will be saved in **C:\WINNT\Profiles\@yourprofilename\Downloads** (by using the %USERPROFILE% environment variable)
    - on Windows 2000 and XP, files will be saved in **C:\Documents and Settings\@yourusername\Downloads** (by using the %USERPROFILE% environment variable)
    - on Windows Vista or superior, files will be saved in **C:\Users\@yourusername\Downloads** (by using the %USERPROFILE% environment variable)
- save files to a path you've chosen: you can manually enter it, or choose it via the Browse button

If you toggle **Quick Save** on, files you receive will automatically be saved to the path you've chosen. If not, LocalSend32 will ask you for any transfer whether or not you want to accept it, and where to save it.

## PIN
If you've toggled "Require PIN" under Settings > Network, you'll be prompted to input a PIN when the other device is sending you files. On the other device, input the PIN that you've chosen in Settings to proceed with the transfer.

## During receiving
All the files that the other device wanted to transfer will be shown on the right side of the Receive tab. 

Under "Progress", you'll see the percentage at which it's transferring.

## After receiving
The files you've received will be shown in the same list as during receiving. You can double-click on an item to open it with the default application associated to the extension of the file, or you can right click to delete the file, copy its path, or open its properties.

On LocalSend32, LocalSendRT, and LocalSendARM, transfers will be divided into two categories: **Today** (which shows the most recent transmission session) and **Older transfers** (to show all the other sessions). To move all the items from the first group to the second, click the Close button on the bottom part of the app in the Receive tab when appears.

## Problems
If you're having problems with receiving files, check the **Problems** chapter of this help.
