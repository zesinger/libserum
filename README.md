# libserum
This is a cross-platform library for decoding Serum files, a colorization format for pinball ROMs.

Thanks to Markus Kalkbrenner for all the Github stuff I am not very good at and the tests on other platforms than Windows.

## Usage ingame

1. At table loading, call `bool Serum_Load(const char* altcolorpath, const char* romname, int* pwidth, int* pheight, unsigned int* pnocolors)`
The file "altcolorpath/romname/romname.cRZ" is loaded. For example, with "altcolorpath=c:/visual pinball/VPinMame/altcolor" (the final "/" is not mandatory, it is added if missing) and "romname=ss_15", "c:/visual pinball/VPinMame/altcolor/ss_15/ss_15.cRZ" is loaded.
In return, the ints *pwidth, *pheight and unsigned int *pnocolors contain width, height and the number of colors in the PinMame incoming frames.

2. When PinMame sends a frame made of width * height bytes, pass it to `bool Serum_Colorize(unsigned char* frame, int width, int height, unsigned char* palette, unsigned char* rotations)`
Where in return:
   * "frame" will contain the colorized frame ([0,64] values).
   * "palette" is a `64*3` byte buffer you create before calling the function that will receive the 64-RGB-colour palette.
   * "rotations" a `3*8` byte buffer you create before calling the function that will receive the colour rotation description [first colour, number of colours, delay between rotation in 10ms]. if first colour=255, the colour rotation is not active.

3. By default `Serum_Colorize()` will ignore unknown frames and return the last colorized frame instead to remain visible unless a new known/expected frame gets colorized.
In some cases like for incomplete colorizations or WIP colorization projects this is not the required behavior.
By calling `void Serum_SetIgnoreUnknownFramesTimeout(UINT16 milliseconds)` you can specify an amount of milliseconds any unknown frames will be ignored.
After that timeout `Serum_Colorize()` will no longer return the previous colorized frame but return false to allow the user to render the original frame as it is.
If a known frame gets colored the timeout starts from 0.

4. When releasing the table, call `void Serum_Dispose(void)`

That's all!

## License 
The code in this directory and all sub-directories is licenced under **GPLv2+** with only a restriction on names (see below), except if a different license is
mentioned in a file's header or in a sub-directory. Be aware of the fact that your own enhancements of libserum need to
be licenced under a compatible licence.

Any code, device or whatever linked to libserum or using the file format it defines, must provide a link to https://github.com/zesinger/libserum in its description.
If you find issues within libserum or have ideas for enhancements, we encourage you to contribute to libserum by creating PRs (GitHub Pull Requests) instead of maintaining your own copy.

Due to complicated dependency management on different platforms, these libraries are included as source code copy:
* [miniz-cpp](https://github.com/tfussell/miniz-cpp) by Thomas Fussel

## Compiling

#### Linux or macOS
```shell
cmake -DCMAKE_BUILD_TYPE=Release -B build/Release
cmake --build build/Release
```

#### Windows
Use Visual Studio.

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
