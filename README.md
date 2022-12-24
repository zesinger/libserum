# libserum
This is a cross-platform library for decoding Serum files, a colorization format for pinball ROMs.

## Usage ingame

1/ At table loading, call `bool Serum_Load(const char* altcolorpath, const char* romname, int* pwidth, int* pheight, unsigned int* pnocolors)`

The file "altcolorpath/romname/romname.cRZ" is loaded. For example, with "altcolorpath=c:/visual pinball/VPinMame/altcolor" (the final "/" is not mandatory, it is added if missing) and "romname=ss_15", "c:/visual pinball/VPinMame/altcolor/ss_15/ss_15.cRZ" is loaded.

In return, the ints *pwidth, *pheight and unsigned int *pnocolors contain width, height and the number of colors in the real pinball manufacturer's ROM.

2/ When PinMame sends a frame made of width * height bytes, pass it to `void Serum_Colorize(UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations)`

Where in return:
- "frame" will contain the colorized frame ([0,64] values).
- "palette" is a 64 * 3-byte buffer that will receive the 64-RGB-colour palette.
- "rotations" a 3 * 8-byte buffer that will receive the colour rotation description [first colour, number of colours, delay between rotation in 10ms]. if first colour=255, the colour rotation is not active.

3/ When releasing the table, call `void Serum_Dispose(void)`

That's all!

## License 
The code in this directory and all sub-directories is licenced under GPLv2 (or later), except if a different license is
mentioned in a file's header or in a sub-directory. Be aware of the fact that your own enhancements of libserum need to
be licenced under a compatible licence.

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
