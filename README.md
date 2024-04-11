# libserum
This is a cross-platform library for decoding Serum files, a colorization format for pinball ROMs.

Thanks to Markus Kalkbrenner for all the Github stuff I am not very good at and the tests on other platforms than Windows.

## Usage ingame

1. At table loading, call `bool Serum_Load(const char* const altcolorpath, const char* const romname, unsigned int* pnocolors, unsigned int* pntriggers, UINT8 flags, UINT* width32, UINT* width64, UINT8* newformat)`
The file "altcolorpath/romname/romname.cRZ" is loaded. For example, with "altcolorpath=c:/visual pinball/VPinMame/altcolor" (the final "/" is not mandatory, it is added if missing) and "romname=ss_15", "c:/visual pinball/VPinMame/altcolor/ss_15/ss_15.cRZ" is loaded.
In flags you should set the first bit to 1 ("| 1") if you want the 32P version of the frame in case of multiple resolution new format file if available and the second bit to 1 ("| 2") if you want the 64P version of the frame if available.
In return, the ints *pwidth, *pheight, unsigned int *pnocolors and *pntriggers contain width and height and number of colours in the PinMame incoming frames and number of triggers.
width32 contains the width of the 32P frames and width64 the width of 64P frames and newformat=0 if former format file (created with CDMD<3.0.0), > 0 if this is a new format file.

2. When PinMame sends a frame made of width * height bytes, pass it to `bool Serum_Colorize(UINT8* frame, Serum_Frame* poldframe, Serum_Frame_New* pnewframe);`
With Serum_Frame is a structure (see "serum-decode.h") that you must have filled before calling, if you use a former version file, or Serum_Frame_New for new format file.

Former format:
	- frame will be modified with the colorized frame (indices to the palette below)
	- you must have allocated 64*3 bytes in palette, it will receive the palette of the frame
	- you must have allocated 8*3 bytes in rotations, it will receive the color rotations for the frame
	- triggerID is a pointer to a unsigned int that represent a PuP pack trigger ID to send when this frame is identified (if = 0xffff, no trigger)
 
New format:
	- if you want the 32P frame (if available in the file) frame32 must be a pointer to an allocated block of 32*width32 (from Serum_Load), if not must be NULL
	- if frame32 is not NULL, width32 must be a pointer to a UINT and will receive the width of the 32P frame. If no frame32 is available, width32 will be 0
	- if frame32 is not NULL and a 32P frame is available, rotations32 must be a pointer to 4*64 UINT16 and will receive the rotations of the frame
	- if frame32 is not NULL and a 32P frame is available, rotationsinframe32 must be a pointer to 2 * 32*width32 UINT16 and will receive the pixels of the frame with colors that rotate
	- same for 64P for all the xxxx64 elements
	- triggerID is a pointer to a unsigned int that represent a PuP pack trigger ID to send when this frame is identified (if = 0xffff, no trigger)
	- if (flags & 1) the 32P frame is returned, if (flags & 2) the 64P frame is returned

4. By default `Serum_Colorize()` will ignore unknown frames and return the last colorized frame instead to remain visible unless a new known/expected frame gets colorized.
In some cases like for incomplete colorizations or WIP colorization projects this is not the required behavior.
By calling `void Serum_SetIgnoreUnknownFramesTimeout(UINT16 milliseconds)` you can specify an amount of milliseconds any unknown frames will be ignored.
After that timeout `Serum_Colorize()` will no longer return the previous colorized frame but return false to allow the user to render the original frame as it is.
If a known frame gets colored the timeout starts from 0.

5. Call `bool Serum_ApplyRotations(UINT8* palette, UINT8* rotations)` for former format files and `bool Serum_ApplyRotationsN(UINT16* frame, UINT8* modelements, UINT16* rotationsinframe, UINT sizeframe, UINT16* rotations, bool is32)` for new format file.
6. 
Former format:
	- the Serum_Frame::palette returned by `Serum_Colorize()` and will be modified to apply rotations
	- is the Serum_Frame::rotations returned by `Serum_Colorize()`
 - 
New format:
	- frame is the Serum_Frame_New::frame32 (if you set is32 to true) or the Serum_Frame_New::frame64 (you set is32 to false) returned by `Serum_Colorize()` and will be modified according the rotations to be applied
	- modelements is a pointer to 32 * width32 or 64 * width64 (according to is32) UINT8 that will be 1 if the corresponding pixel has changed, 0 if not (so that you don't need to re-paint all the pixels)
	- rotationsinframe is either the Serum_Frame_New::rotationsinframe32 or Serum_Frame_New::rotationsinframe64 (according to is32) returned by `Serum_Colorize()`
	- sizeframe is either 32 * width32 or 64 * width64 (according to is32)
	- rotations is either the Serum_Frame_New::rotations32 or Serum_Frame_New::rotations64 (according to is32) returned by `Serum_Colorize()`
	- is32 indicates if you want the rotations for the 32P frame (true) or for the 64P frame (false)

7. When releasing the table, call `void Serum_Dispose(void)`

That's all!

## License 
The code in this directory and all sub-directories is licenced under **GPLv2+** with only a restriction on names (see below), except if a different license is
mentioned in a file's header or in a sub-directory. Be aware of the fact that your own enhancements of libserum need to
be licenced under a compatible licence.

Any code, device or whatever linked to libserum or using the file format it defines, must provide a link to https://github.com/zesinger/libserum in its description.
If you find issues within libserum or have ideas for enhancements, we encourage you to contribute to libserum by creating PRs (GitHub Pull Requests) instead of maintaining your own copy.

Due to complicated dependency management on different platforms, these libraries are included as source code copy:
* [miniz](https://github.com/richgel999/miniz)

## Compiling

#### Windows (x64)
Use Visual Studio.

or

```shell
cmake -G "Visual Studio 17 2022" -DPLATFORM=win -DARCH=x64 -B build
cmake --build build --config Release
```

#### Windows (x86)
Use Visual Studio.

or

```shell
cmake -G "Visual Studio 17 2022" -A Win32 -DPLATFORM=win -DARCH=x86 -B build
cmake --build build --config Release
```

#### Linux (x64)
```shell
cmake -DPLATFORM=linux -DARCH=x64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### Linux (aarch64)
```shell
cmake -DPLATFORM=linux -DARCH=aarch64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### MacOS (arm64)
```shell
cmake -DPLATFORM=macos -DARCH=arm64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### MacOS (x64)
```shell
cmake -DPLATFORM=macos -DARCH=arm64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### iOS (arm64)
```shell
cmake -DPLATFORM=ios -DARCH=arm64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### tvOS (arm64)
```shell
cmake -DPLATFORM=tvos -DARCH=arm64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### Android (arm64-v8a)
```shell
cmake -DPLATFORM=android -DARCH=arm64-v8a -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### For C# code
If you want to include Serum colorization in your C# project, declare things this way:

```
[DllImport("serum.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
public static extern bool Serum_Load(string altcolorpath, string romname,ref int width, ref int height, ref uint nocolors);

[DllImport("serum.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
public static extern void Serum_Colorize(Byte[] frame, int width, int height, byte[] palette, byte[] rotations);

[DllImport("serum.dll", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
public static extern void Serum_Dispose();
```

Then, to call Serum_Colorize(...):

```
// "frame" is a byte[frame_width * frame_height] containing the PinMame frame
byte[] pal = new byte[64 * 3];
byte[] rotations = new byte[MAX_COLOR_ROTATIONS * 3];
Serum_Colorize(frame, frame_width, frame_height, pal, rotations);
```
