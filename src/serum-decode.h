#ifndef SERUM_DECODE_H
#define SERUM_DECODE_H

#if defined _WIN32
#define SERUM_API(RetType) extern "C" __declspec(dllexport) RetType
#else
#define SERUM_API(RetType) extern "C" RetType __attribute__((visibility("default")))
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned int UINT;

const int MAX_DYNA_4COLS_PER_FRAME = 16; // max number of color sets for dynamic content for each frame
const int MAX_SPRITE_SIZE = 128; // maximum size of the sprites
const int MAX_SPRITES_PER_FRAME = 32; // maximum amount of sprites to look for per frame
const int MAX_COLOR_ROTATIONS = 8; // maximum amount of color rotations per frame
const int MAX_SPRITE_DETECT_AREAS = 4; // maximum number of areas to detect the sprite

const int PALETTE_SIZE = 64 * 3; // size of a palette
const int ROTATION_SIZE = 3 * MAX_COLOR_ROTATIONS; // size of a color rotation block

SERUM_API(bool) Serum_LoadFile(const char* const filename, int* pwidth, int* pheight, unsigned int* pnocolors, unsigned int* pntriggers);
SERUM_API(bool) Serum_Load(const char* const altcolorpath, const char* const romname, int* pwidth, int* pheight, unsigned int* pnocolors, unsigned int* pntriggers);
SERUM_API(void) Serum_SetIgnoreUnknownFramesTimeout(UINT16 milliseconds);
SERUM_API(void) Serum_SetMaximumUnknownFramesToSkip(UINT8 maximum);
SERUM_API(void) Serum_SetStandardPalette(UINT8* palette, int bitDepth);
SERUM_API(void) Serum_Dispose(void);
SERUM_API(bool) Serum_ColorizeWithMetadata(UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations, UINT32* triggerID, UINT32* hashcode, int* frameID);
SERUM_API(bool) Serum_Colorize(UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations, UINT32* triggerID);
SERUM_API(bool) Serum_ApplyRotations(UINT8* palette, UINT8* rotations);
SERUM_API(bool) Serum_ColorizeWithMetadataOrApplyRotations(UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations, UINT32* triggerID, UINT32* hashcode, int* frameID);
SERUM_API(bool) Serum_ColorizeOrApplyRotations(UINT8* frame, int width, int height, UINT8* palette, UINT32* triggerID);
SERUM_API(void) Serum_DisableColorization();
SERUM_API(void) Serum_EnableColorization();
SERUM_API(const char*) Serum_GetVersion();
SERUM_API(const char*) Serum_GetMinorVersion();

#endif