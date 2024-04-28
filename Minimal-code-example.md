To use the library, here are some basic lines in plain C code (only the `HINSTANCE hSerumDLL;`, the whole part 3/ and all the `FreeLibrary(hSerumDLL);` should be Windows specific):

1/ Includes:

`#include "serum.h"`

2/ Global variables:

```
// Functions from the dll
HINSTANCE hSerumDLL;
Serum_LoadFunc serum_Load;
Serum_DisposeFunc serum_Dispose;
Serum_ColorizeFunc serum_Colorize;
Serum_RotateFunc serum_Rotate;

UINT8 SerumFormat = SERUM_V1; // Serum version (see enum in serum.h)
Serum_Frame MyOldFrame; // structure to communicate with former format Serum 
Serum_Frame_New MyNewFrame; // structure to communicate with new format Serum 
UINT8* ModifiedElements32 = NULL; // for the new color rotations in 32P, optional
UINT8* ModifiedElements64 = NULL; // for the new color rotations in 64P, optional
UINT triggerID; // return PuP pack trigger ID (0xffff if no trigger)
UINT8 returnflag; // what frame resolutions are returned with new format colorization
UINT32 noColors; // number of colors of the original ROM (4 or 16)
UINT32 fWidth, fHeight; // dimensions of the original ROM (MUST BE KNOWN BEFORE calling Serum functions)
UINT width32 = 0, width64 = 0; // widths of the colorized frames returned respectively for the height=32 and height=64 frames
UINT ntriggers = 0; // number of PuP triggers found in the file
```

3/ Code to load the library and its functions:

```
bool Load_Serum_DLL(void)
{
    // Function to load the serum library and all its needed functions, call it in your initial code
    // replace File_SerumDLL by a const char* with the full path and name of the DLL
    // like "c:\\visual pinball\\vpinmame\\serum64.dll"
    hSerumDLL = LoadLibraryA(File_SerumDLL);
    if (hSerumDLL == NULL)
    {
        // add an error message if you want
        return false;
    }
    serum_Load = (Serum_LoadFunc)GetProcAddress(hSerumDLL, "Serum_Load");
    if (serum_Load == NULL)
    {
        // add an error message if you want
        FreeLibrary(hSerumDLL);
        return false;
    }
    serum_Dispose = (Serum_DisposeFunc)GetProcAddress(hSerumDLL, "Serum_Dispose");
    if (serum_Dispose == NULL)
    {
        // add an error message if you want
        FreeLibrary(hSerumDLL);
        return false;
    }
    serum_Colorize = (Serum_ColorizeFunc)GetProcAddress(hSerumDLL, "Serum_Colorize");
    if (serum_Colorize == NULL)
    {
        // add an error message if you want
        FreeLibrary(hSerumDLL);
        return false;
    }
    serum_Rotate = (Serum_RotateFunc)GetProcAddress(hSerumDLL, "Serum_Rotate");
    if (serum_Rotate == NULL)
    {
        // add an error message if you want
        FreeLibrary(hSerumDLL);
        return false;
    }
    return true;
}
```

4/ Example functions to allocate and free the buffers for Serum:

```
void Free_element(void* pElement)
{
    if (pElement)
    {
        free(pElement);
        pElement = NULL;
    }
}

void Free_Serum(void)
{
    Free_element(MyOldFrame.frame);
    Free_element(MyOldFrame.palette);
    Free_element(MyOldFrame.rotations);
    Free_element(MyNewFrame.frame32);
    Free_element(MyNewFrame.frame64);
    Free_element(MyNewFrame.rotations32);
    Free_element(MyNewFrame.rotations64);
    Free_element(MyNewFrame.rotationsinframe32);
    Free_element(MyNewFrame.rotationsinframe64);
    Free_element(ModifiedElements32);
    Free_element(ModifiedElements64);
}

bool Allocate_Serum(void)
{
    MyOldFrame.palette = NULL;
    MyOldFrame.rotations = NULL;
    MyNewFrame.frame32 = NULL;
    MyNewFrame.frame64 = NULL;
    MyNewFrame.rotations32 = NULL;
    MyNewFrame.rotations64 = NULL;
    MyNewFrame.rotationsinframe32 = NULL;
    MyNewFrame.rotationsinframe64 = NULL;
    if (SerumVersion == SERUM_V1)
    {
        MyOldFrame.frame = (UINT8*)malloc(fWidth * fHeight);
        MyOldFrame.palette = (UINT8*)malloc(3 * 64);
        MyOldFrame.rotations = (UINT8*)malloc(3 * MAX_COLOR_ROTATIONS);
        MyOldFrame.triggerID = &triggerID;
        if (!MyOldFrame.frame || !MyOldFrame.palette || !MyOldFrame.rotations)
        {
            // add an error message if you want
            FreeLibrary(hSerumDLL);
            Free_Serum();
            serum_Dispose();
            return -1;
        }
    }
    else
    {
        MyNewFrame.flags = &returnflag;
        MyNewFrame.triggerID = &triggerID;
        // ---------- Both ModifiedElementsXX are optional so this code may be skipped ----------
        // They are only needed if you only want to change the modified pixels of a frame after a color rotation
        ModifiedElements32 = (UINT8*)malloc(width32 * 32);
        ModifiedElements64 = (UINT8*)malloc(width64 * 64);
        if (!ModifiedElements32 || !ModifiedElements64)
        {
            // add an error message if you want
            FreeLibrary(hSerumDLL);
            Free_Serum();
            serum_Dispose();
            return -1;
        }
        // --------------------------------------------------------------------------------------
        if (width32 > 0)
        {
            MyNewFrame.frame32 = (UINT16*)malloc(2 * 32 * width32);
            MyNewFrame.rotations32 = (UINT16*)malloc(2 * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
            MyNewFrame.rotationsinframe32 = (UINT16*)malloc(2 * 2 * 32 * width32);
            MyNewFrame.width32 = &width32;
            if (!MyNewFrame.frame32 || !MyNewFrame.rotations32 || !MyNewFrame.rotationsinframe32)
            {
                // add an error message if you want
                FreeLibrary(hSerumDLL);
                Free_Serum();
                serum_Dispose();
                return -1;
            }
        }
        if (width64 > 0)
        {
            MyNewFrame.frame64 = (UINT16*)malloc(2 * 64 * width64);
            MyNewFrame.rotations64 = (UINT16*)malloc(2 * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
            MyNewFrame.rotationsinframe64 = (UINT16*)malloc(2 * 2 * 64 * width64);
            MyNewFrame.width64 = &width64;
            if (!MyNewFrame.frame64 || !MyNewFrame.rotations64 || !MyNewFrame.rotationsinframe64)
            {
                // add an error message if you want
                FreeLibrary(hSerumDLL);
                Free_Serum();
                serum_Dispose();
                return -1;
            }
        }
    }
}
```

5/ Initialization code (to place in your main function before the loop):

```
if (!Load_Serum_DLL())
{
    return -1;
}
if (!serum_Load(Dir_Altcolor, romname, &noColors, &ntriggers, FLAG_REQUEST_32P_FRAMES | FLAG_REQUEST_64P_FRAMES, &width32, &width64, &SerumFormat))
{
    // add an error message if you want
    FreeLibrary(hSerumDLL);
    return -1;
}
if (!Allocate_Serum())
{
    // add an error message if you want
    return -1;
}
```

6/ Code to call when you have received a new uncolorized frame from the ROM:

```
            if (SerumVersion == SERUM_V1) serum_Colorize(frame, &MyOldFrame, NULL);
            else serum_Colorize(frame, NULL, &MyNewFrame);
            // then add your code to update the display:
            // New format: using RGB565 colors MyNewFrame.frame32[tj * width32 + ti] and/or MyNewFrame.frame64[tj * width64 + ti]
            // Former format: RGB888 color pointed by &MyOldFrame.palette[MyOldFrame.frame[tj * fWidth + ti] * 3]
            // where (ti,tj) are the coordinates of the pixel
```

7/ Code to call in your loop to update the color rotations:

```
        bool isrot = false;
        isrot = serum_Rotate(&MyOldFrame, &MyNewFrame, ModifiedElements32, ModifiedElements64); // if you don't need them replace ModifiedElementsXX by NULL
        // then if isrot == true, update your display with the content of &MyOldFrame.palette[MyOldFrame.frame[tj * fWidth + ti] * 3]
        // or MyNewFrame.frame32[tj * width32 + ti] and/or MyNewFrame.frame64[tj * width64 + ti] as above
        // for new format, if ModifiedElementsXX are defined, you may check that ModifiedElementsXX[tj * widthXX + ti]
        // is >0 to only update the modified pixels
```

8/ Code when the Serum content is not needed anymore to free the resources:
```
    Free_Serum();
    serum_Dispose();
    FreeLibrary(hSerumDLL);
```

