#pragma once

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT;

enum // returned by Serum_Load in *SerumVersion
{
	SERUM_V1,
	SERUM_V2
};

// Flags to send to Serum_Load
enum
{
	FLAG_REQUEST_32P_FRAMES = 1,				// there is a output DMD which is 32 leds high
	FLAG_REQUEST_64P_FRAMES = 2,				// there is a output h is 64 leds high
	FLAG_REQUEST_FILL_MODIFIED_ELEMENTS = 4,	// does the modifiedelementsXX structures must be allocated and returned
};

enum // returned values by Serum_Colorize for new format
{
	FLAG_RETURNED_32P_FRAME_OK = 1, // the 32p frame has been filled
	FLAG_RETURNED_64P_FRAME_OK = 2, // the 64p frame has been filled
};

enum // returned flags that are added to the timings if there were rotations
{
	FLAG_RETURNED_V1_ROTATED = 0x10000,
	FLAG_RETURNED_V2_ROTATED32 = 0x10000,
	FLAG_RETURNED_V2_ROTATED64 = 0x20000,
};

typedef struct _Serum_Frame_Struc
{
	// data for v1 Serum format
	UINT8* frame; // return the colorized frame
	UINT8* palette; // and its palette
	UINT8* rotations; // and its color rotations
	// data for v2 Serum format
	// the frame (frame32 or frame64) corresponding to the resolution of the ROM must ALWAYS be defined
	// if a frame pointer is defined, its width, rotations and rotationsinframe pointers must be defined
	UINT16* frame32;
	UINT width32; // 0 is returned if the 32p colorized frame is not available for this frame
	UINT16* rotations32;
	UINT16* rotationsinframe32; // [width32*32*2] precalculated array to tell if a color is in a color rotations of the frame ([X*Y*0]=0xffff if not part of a rotation)
	UINT8* modifiedelements32; // (optional) 32P pixels modified during the last rotation
	UINT16* frame64;
	UINT width64; // 0 is returned if the 64p colorized frame is not available for this frame
	UINT16* rotations64;
	UINT16* rotationsinframe64;  // [width64*64*2] precalculated array to tell if a color is in a color rotations of the frame ([X*Y*0]=0xffff if not part of a rotation)
	UINT8* modifiedelements64; // (optional) 64P pixels modified during the last rotation
	// common data
	UINT SerumVersion; // SERUM_V1 or SERUM_V2
	/// <summary>
	/// flags for return:
	/// if flags & 1 : frame32 has been filled
	/// if flags & 2 : frame64 has been filled
	/// if flags & 4 : frame + palette have been filled
	/// if none of them, display the original frame
	/// </summary>
	UINT8 flags;
	unsigned int nocolors; // number of shades of orange in the ROM
	unsigned int ntriggers; // number of triggers in the Serum file
	UINT triggerID; // return 0xffff if no trigger for that frame, the ID of the trigger if one is set for that frame
	int frameID; // for CDMD ingame tester
	UINT rotationtimer; 
}Serum_Frame_Struc;

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
const int MAX_BACKGROUND_IMAGES = 255;  // max number of background images

typedef Serum_Frame_Struc* (*Serum_LoadFunc)(const char* const altcolorpath, const char* const romname, UINT8 flags);
typedef void (*Serum_DisposeFunc)(void);
typedef UINT (*Serum_ColorizeFunc)(UINT8* frame);
typedef UINT (*Serum_RotateFunc)(void);
typedef const char* (*Serum_GetVersionFunc)(void);
