# libserum
This is a cross-platform library for decoding Serum files, a colorization format for pinball ROMs.

Thanks to Markus Kalkbrenner for all the Github stuff I am not very good at and the tests on other platforms than Windows.

## Usage ingame

1. At table loading, call `bool Serum_Load(const char* const altcolorpath, const char* const romname, unsigned int* pnocolors, unsigned int* pntriggers, UINT8 flags, UINT* width32, UINT* width64, UINT8* newformat)`. The file "altcolorpath/romname/romname.cRZ" is loaded.

For example, with "altcolorpath=c:/visual pinball/VPinMame/altcolor" (the final "/" is not mandatory, it is added if missing) and "romname=ss_15", "c:/visual pinball/VPinMame/altcolor/ss_15/ss_15.cRZ" is loaded.
In flags you should set the first bit to 1 ("| 1") if you want the 32P version of the frame in case of multiple resolution new format file if available and the second bit to 1 ("| 2") if you want the 64P version of the frame if available.
In return, the ints *pwidth, *pheight, unsigned int *pnocolors and *pntriggers contain width and height and number of colours in the PinMame incoming frames and number of triggers.
width32 contains the width of the 32P frames and width64 the width of 64P frames and newformat=0 if former format file (created with CDMD<3.0.0), > 0 if this is a new format file.

2. When PinMame sends a frame made of width * height bytes, pass it to `bool Serum_Colorize(UINT8* frame, Serum_Frame* poldframe, Serum_Frame_New* pnewframe);`

Where `Serum_Frame` is a structure (see "serum-decode.h") that you must have filled before calling if `Serum_Load` returns 0 in NewFormat, if you use a former version file, or `Serum_Frame_New` is a structure that you must have filled for new format file (if `Serum_Load` NewFormat > 0).

Former format:

	- frame received from VPinMame
 	- you must have allocated fWidth * fHeight UINT8 in Serum_Frame::frame, (fWidth,fHeight) being the dimensions of the frame sent by VPinMame, it will receive the colorized indices of the frame in the palette below
	- you must have allocated 64*3 UINT8 in Serum_Frame::palette, it will receive the palette of the frame
	- you must have allocated 8*3 UINT8 in Serum_Frame::rotations, it will receive the color rotations for the frame
	- triggerID is a pointer to a unsigned int that will receive a PuP pack trigger ID to send when this frame is identified (if = 0xffff, no trigger)
 
New format:

	- frame received from VPinMame
	- if you want the 32P frame (if available in the file) Serum_Frame_New::frame32 must be a pointer to an allocated block of 32 * width32 (from Serum_Load) UINT16 as RGB565 colors, if not must be NULL
	- if frame32 is not NULL, Serum_Frame_New::width32 must be a pointer to a UINT and will receive the width of the 32P frame. If no frame32 is available, width32 will return 0
	- if frame32 is not NULL and a 32P frame is available, Serum_Frame_New::rotations32 must be a pointer to 4*64 UINT16 and will receive the rotations of the frame
	- if frame32 is not NULL and a 32P frame is available, Serum_Frame_New::rotationsinframe32 must be a pointer to 2 * (32 * width32) UINT16 and will receive the pixels of the frame with colors that rotate. "2 *" as the first UINT16 receives the frame color rotation # and the second, the position in this rotation
	- same for 64P for all the xxxx64 elements
	- triggerID is a pointer to a unsigned int that represent a PuP pack trigger ID to send when this frame is identified (if = 0xffff, no trigger)
	- if (flags & 1) the 32P frame was available and is returned, if (flags & 2) the 64P frame was available and is returned

3. By default `Serum_Colorize()` will ignore unknown frames and return the last colorized frame instead to remain visible unless a new known/expected frame gets colorized.
In some cases like for incomplete colorizations or WIP colorization projects this is not the required behavior.
By calling `void Serum_SetIgnoreUnknownFramesTimeout(UINT16 milliseconds)` you can specify an amount of milliseconds any unknown frames will be ignored.
After that timeout `Serum_Colorize()` will no longer return the previous colorized frame but return false to allow the user to render the original frame as it is.
If a known frame gets colored the timeout starts from 0.

4. Call `bool Serum_ApplyRotations(Serum_Frame* poldframe)` for former format files and `bool Serum_ApplyRotationsN(Serum_Frame_New* pnewframe, UINT8* modelements32, UINT8* modelements64)` for new format file to update the color rotations if available.

Former format: Just send the Serum_Frame received from `Serum_Colorize()`. It will return true if a rotation was done and you must redraw the frame, false if not.

New format (will return true if rotations were made, false if not:

	- send the Serum_Frame_New received from `Serum_Colorize()`
 	- modelements32 and modelements64 are 2 optional buffers that you must have allocated with respectively 32 * width32 and 64 * width64 UINT8. For each pixel, the corresponding modelementsXX will be 0 if it didn't change or >0 if it did. If you don't need this information (as tou will redraw the full frame when there are rotations), you can set them as NULL.

5. When releasing the table, call `void Serum_Dispose(void)`

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
