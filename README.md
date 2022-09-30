# ioauthorize32
Allows a program Direct I/O access to hardware devices on a system.
## Requirements
1. Windows XP/Vista/7 32-bit
2. Visual C++ Redistributable for Visual Studio 2019 (version 16.7)
## Use
1. Open up command prompt as administrator and issue the following command:
> ```sc create ioauthorize32d type= kernel displayName= ioauthorize32d binPath= "Path\To\Driver.sys"```
2. Replace "**Path\To\Driver.sys**" with the full path to **ioauthorize32d.sys**
3. If step 1 is successful, issue this command: 
> ```sc start ioauthorize32d```
4. Finally, execute **ioauthorize32.exe** with the path to the program that you wish to use Direct I/O as the first argument
