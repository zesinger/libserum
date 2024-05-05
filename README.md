# libserum
This is a cross-platform library for decoding Serum files, a colorization format for pinball ROMs.

Thanks to Markus Kalkbrenner for all the Github stuff I am not very good at and the tests on other platforms than Windows.

## Usage ingame

### 1/ Format Description

All the content is memory allocated internally and stored in a structure `Serum_Frame_Struc`.
A pointer to that structure is returned by the `Serum_Load(...)` function. After colorizations and color rotations steps, this structure will provide the needed data to update your display.

Here is the description of this structure defined in `serum.h`, the not documented lines are used internally:
```
typedef struct
{
	// data for v1 Serum format
	UINT8* frame; // return the colorized frame (color indices in the palette) pixel at (x,y) is frame[y * width + x] where width is the width of the original ROM frame
	UINT8* palette; // and its palette
	UINT8* rotations;
	// data for v2 Serum format
	// the frame (frame32 or frame64) corresponding to the resolution of the ROM must ALWAYS be defined
	// if a frame pointer is defined, its width, rotations and rotationsinframe pointers must be defined
	UINT16* frame32; // return the 32 pixel high colorized frame in RGB565/RGB16 format. pixel at (x,y) is frame32[y * width32 + x] (width32 see below)
	UINT width32; // 0 is returned if the 32p colorized frame is not available for this frame
	UINT16* rotations32;
	UINT16* rotationsinframe32; 
	UINT8* modifiedelements32; // (optional) 32P pixels modified during the last rotation
	UINT16* frame64; // return the 64 pixel high colorized frame in RGB565/RGB16 format. pixel at (x,y) is frame64[y * width64 + x] (width64 see below)
	UINT width64; // 0 is returned if the 64p colorized frame is not available for this frame
	UINT16* rotations64;
	UINT16* rotationsinframe64;
	UINT8* modifiedelements64; // (optional) 64P pixels modified during the last rotation
	// common data
	UINT SerumVersion; // = SERUM_V1 or SERUM_V2
	/// <summary>
	/// flags for return:
	/// if flags & 1 : frame32 has been filled (you can simply rely on width32 == 0 or not)
	/// if flags & 2 : frame64 has been filled (you can simply rely on width64 == 0 or not)
	/// if flags & 4 : frame + palette have been filled
	/// if none of them, display the original frame
	/// </summary>
	UINT8 flags;
	unsigned int nocolors; // number of shades of orange in the ROM (set at Serum load time)
	unsigned int ntriggers; // number of triggers in the Serum file (set at Serum load time) 
	UINT triggerID; // return 0xffff if no trigger for the current frame, the ID of the trigger if one is set for that frame
	UINT frameID;
	UINT16 rotationtimer; // the value returned by Serum_Colorize() and Serum_Rotate() to tell how long to wait in ms before the next call to Serum_Rotate()
}Serum_Frame_Struc;
```

### 2/ Code example

There is a minimal code example with information on how to use the returned data given here:
https://github.com/zesinger/libserum/blob/main/Minimal-code-example.md

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
