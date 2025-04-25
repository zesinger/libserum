# libserum (concentrate)
This is a (friendly) fork of [libserum](https://github.com/zesinger/libserum), the great cross-platform library for decoding Serum files, a colorization format for pinball ROMs, created and maintained by [Zed](https://github.com/zesinger).

While the main development still happens in libserum itself, libserum (concentrate) is a drop-in replacement, especially for low-memory devices like mobile devices, VR headsets or real pinball machines.

## Disclaimer
The code is a bit more complicated, might not always be up-to-date with the original and might introduce new bugs.
libserum (concentrate) is not meant to replace libserum entirely.

## Usage
VPX Standlone and all PPUC projects will now use libserum (concentrate) by default. So you should get it as part of an update.
If you want to use it on Windows in combination with DMDExt, just download the appropriate version of the [latest release](https://github.com/PPUC/libserum_concentrate/releases/latest) and extract serum.dll and/or serum64.dll into your VPinMAME folder.
Don't forget to unblock the DLL(s). DMDext will automatically pick-up and use these DLLs, dynamically replacing the serum.dll enbedded in dmddevice.dll.
