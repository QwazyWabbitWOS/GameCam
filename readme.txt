GameCam v1.04 - camera proxy game module for Quake II
Copyright (c) 1998-99, by Avi "Zung!" Rozen
e-mail: zungbang@telefragged.com

**********
This is a special build meant to be installed like
the linux/solaris versions.
**********

Installation
============
Perform the following for each mod you want to add GameCam to:

On Linux:
To run gamecam on Linux for your mod with q2admin:
1. Rebuild GameCam with 'make all' 
2. Rename your existing gamex86_64.so as gamex86.real.so
3. Copy the Q2admin module to your mod folder and name it q2admin.so
4. Copy the GameCam gamex86_64.so module to your mod folder.

To run gamecam on Linux for your mod without q2admin:
1. Comment out the line in Makfile containing -DQ2ADMIN
2. Rebuild GameCam with make all 
3. Rename the existing gamex86_64.so as gamex86_64.real.so
4. Copy the GameCam gamex86_64.so module to your mod folder.

On Windows:
To run gamecam on Windows for your mod without q2admin:
1. Rebuild GameCam with Visual Studio  
2. Rename your existing gamex86.dll as gamex86.real.dll
3. Copy the GameCam gamex86.dll to the mod directory.

That's it. Have fun.

Usage
=====
Refer to the v1.02 manual contained in this archive.
