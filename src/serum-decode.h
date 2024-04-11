#pragma once

#ifdef _MSC_VER
#define SERUM_API extern "C" __declspec(dllexport)
#else
#define SERUM_API extern "C" __attribute__((visibility("default")))
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned int UINT;

typedef struct
{
	// in former format (prior to 2.0.0) the returned frame replaces the original frame, so this is not
	// part of this 
	UINT8* palette;
	UINT8* rotations;
	UINT32* triggerID;
}Serum_Frame;

typedef struct
{
	// the frame (frame32 or frame64) corresponding to the original resolution must ALWAYS be defined
	// but the frame corresponding to the extra resolution must be defined only if we request it
	// if a frame is defined, its width, rotations and rotationsinframe must be defined
	UINT16* frame32;
	UINT* width32; // 0 is returned if the 32p colorized frame is not available for this frame
	UINT16* rotations32;
	UINT16* rotationsinframe32; // [(96 or 128)*32*2] precalculated array to tell if a color is in a color rotations of the frame ([X*Y*0]=0xffff if not part of a rotation)
	UINT16* frame64;
	UINT* width64; // 0 is returned if the 64p colorized frame is not available for this frame
	UINT16* rotations64;
	UINT16* rotationsinframe64;  // [(192 or 256)*64*2] precalculated array to tell if a color is in a color rotations of the frame ([X*Y*0]=0xffff if not part of a rotation)
	UINT32* triggerID;
	UINT8* flags; // return flags:
	// if flags & 1 : frame32 has been filled
	// if flags & 2 : frame64 has been filled
	// if none of them, display the original frame
}Serum_Frame_New;

const int MAX_DYNA_4COLS_PER_FRAME = 16;  // max number of color sets for dynamic content for each frame (old version)
const int MAX_DYNA_SETS_PER_FRAMEN = 32;  // max number of color sets for dynamic content for each frame (new version)
const int MAX_SPRITE_SIZE = 128;  // maximum size of the sprites
const int MAX_SPRITE_WIDTH = 256; // maximum width of the new sprites
const int MAX_SPRITE_HEIGHT = 64; // maximum height of the new sprites
const int MAX_SPRITES_PER_FRAME = 32;  // maximum amount of sprites to look for per frame
const int MAX_COLOR_ROTATIONS = 8;  // maximum amount of color rotations per frame
const int MAX_COLOR_ROTATIONN = 4; // maximum number of new color rotations per frame
const int MAX_LENGTH_COLOR_ROTATION = 64; // maximum number of new colors in a rotation
const int MAX_SPRITE_DETECT_AREAS = 4;  // maximum number of areas to detect the sprite

const int PALETTE_SIZE = 64 * 3;  // size of a palette
const int ROTATION_SIZE = 3 * MAX_COLOR_ROTATIONS;  // size of a color rotation block
const int MAX_SPRITE_TO_DETECT = 16;  // max number of sprites detected in a frame
const int MAX_BACKGROUND_IMAGES = 255;  // max number of background images

// Flags sent with Serum_Load
const int FLAG_REQUEST_32P_FRAMES = 1; // there is a output DMD which is 32 leds high
const int FLAG_REQUEST_64P_FRAMES = 2; // there is a output h is 64 leds high
const int FLAG_32P_FRAME_OK = 1; // the 32p frame has been filled
const int FLAG_64P_FRAME_OK = 2; // the 64p frame has been filled

SERUM_API bool Serum_LoadFile(const char* const filename, unsigned int* pnocolors, unsigned int* pntriggers, UINT8 flags, UINT* width32, UINT* width64);
SERUM_API bool Serum_Load(const char* const altcolorpath, const char* const romname, unsigned int* pnocolors, unsigned int* pntriggers, UINT8 flags, UINT* width32, UINT* width64, UINT8* newformat);
SERUM_API void Serum_SetIgnoreUnknownFramesTimeout(UINT16 milliseconds);
SERUM_API void Serum_SetMaximumUnknownFramesToSkip(UINT8 maximum);
SERUM_API void Serum_SetStandardPalette(const UINT8* palette, const int bitDepth);
SERUM_API void Serum_Dispose(void);
SERUM_API bool Serum_ColorizeWithMetadata(UINT8* frame, Serum_Frame* poldframe);
SERUM_API bool Serum_ColorizeWithMetadataN(UINT8* frame, Serum_Frame_New* pnewframe);
SERUM_API bool Serum_Colorize(UINT8* frame, Serum_Frame* poldframe, Serum_Frame_New* pnewframe);
SERUM_API bool Serum_ApplyRotations(UINT8* palette, UINT8* rotations);
SERUM_API bool Serum_ApplyRotationsN(UINT16* frame, UINT8* modelements, UINT16* rotationsinframe, UINT sizeframe, UINT16* rotations, bool is32);
/*SERUM_API bool Serum_ColorizeWithMetadataOrApplyRotations(
	UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations,
	UINT32* triggerID, UINT32* hashcode, int* frameID);
SERUM_API bool Serum_ColorizeOrApplyRotations(UINT8* frame, int width,
											  int height, UINT8* palette,
											  UINT32* triggerID);*/
SERUM_API void Serum_DisableColorization(void);
SERUM_API void Serum_EnableColorization(void);
SERUM_API const char* Serum_GetVersion(void);
SERUM_API const char* Serum_GetMinorVersion(void);