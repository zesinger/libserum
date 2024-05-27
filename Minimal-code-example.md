To use the library, here are some minimal lines in plain C code:

## 1/ Includes:

```
#include "serum.h"
```
or __under Windows and using the dynamic library__:
```
#include "serumwin.h"
```

## 2/ Global variables:

```
Serum_Frame_Struc* pSerum; // structure returned at load time
UINT32 fWidth, fHeight; // dimensions of the original ROM (MUST BE KNOWN BEFORE calling Serum functions)
```

## 3/ Initialization code to get access to the DLL functions (__for Windows using the dynamic library only__):

```
if (!Serum_LoadDLL(pathtoDLL))
{
    // manage error
}
```
- where `pathtoDLL` is the path and name of the serum.dll/serum64.dll

## 4/ Serum file loading code:

```
pSerum = Serum_Load(Dir_Altcolor, romname, FLAG_REQUEST_32P_FRAMES | FLAG_REQUEST_64P_FRAMES); // add FLAG_REQUEST_FILL_MODIFIED_ELEMENTS if needed
if (!pSerum)
{
    Serum_ReleaseDLL(); // for Windows using the dynamic library only
    // manage error
}
```
where:
- `Dir_Altcolor` is the altcolor directory
- `romname` is the name of the ROM like "afm_113b"
- the `flags` let you decide which data you need to be returned in pSerum in case of a Serum v2 file:
    - FLAG_REQUEST_32P_FRAMES: provide 32P frames in (frame32, width32) if available (if not, width32 is 0)
    - FLAG_REQUEST_64P_FRAMES: provide 64P frames in (frame64, width64) if available (if not, width64 is 0)
    - FLAG_REQUEST_FILL_MODIFIED_ELEMENTS: returns buffers in modifiedelements32 and/or modifiedelements64 
    to tell which pixels have changed during the last rotation

## 5/ Code to call when you have received a new uncolorized frame from the ROM:

```
            UINT firstrot = Serum_Colorize(frame);
		// returned:
	        // firstrot == 0xffffffff if no new frame detected
	        // firstrot == 0 if new frame with no rotation detected
	        // firstrot > 0 if new frame with rotations detected, the value is the delay before the first rotation in ms (= first call needed to Serum_Rotate())
            // then add your code to update the display:
            // if (pSerum->SerumVersion == SERUM_V1):
            //      the (i,j) point RGB888 color is "pSerum->palette[pSerum->frame[tj * fWidth + ti] * 3], pSerum->palette[pSerum->frame[tj * fWidth + ti] * 3 + 1], pSerum->palette[pSerum->frame[tj * fWidth + ti] * 3 + 2]"
            // if (pSerum->SerumVersion == SERUM_V2):
            //      if (pSerum->width32 > 0) the (i,j) 32P point RGB565 color is "pSerum->frame32[j * pSerum->width32 + i]"
            //      if (pSerum->width64 > 0) the (i,j) 64P point RGB565 color is "pSerum->frame64[j * pSerum->width64 + i]"
```
where:
- `frame` is the frame provided by the ROM with values in [0,3] for 4 color ROMs or [0,15] for 16 color ROMs

## 6/ Code to call in your loop or after `firstrot`/`nextrot` to update the color rotations:

```
        UINT nextrot = Serum_Rotate();
	// returned:
        // low word of nextrot: delay before the next rotation in ms (= next call needed to Serum_Rotate())
        // high word of nextrot:
        // - bit 1 is set, nextrot & FLAG_RETURNED_V1_ROTATED (0x10000) to test (if v1 file and a rotation really happened)
        // - bit 1 is set, nextrot & FLAG_RETURNED_V2_ROTATED32 (0x10000) to test (if v2 file and rotation really happened in the 32P frame)
        // - bit 2 is set, nextrot & FLAG_RETURNED_V2_ROTATED64 (0x20000) to test (if v2 file and rotation really happened in the 64P frame)
        // then if (nextrot & FLAG_RETURNED_VX_ROTATEDXX > 0): change the corresponding display like described for Serum_Colorize() above
        // if you don't use a loop, set a timer of the low word of nextrot ms before calling Serum_Rotate() again
```

## 7/ Code when the Serum content is not needed anymore to free the resources:
```
    Free_Serum();
    Serum_Dispose();
    Serum_ReleaseDLL(); // for Windows using the dynamic library only
```

