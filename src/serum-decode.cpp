#define __STDC_WANT_LIB_EXT1_ 1

#include "serum-decode.h"

#include <miniz/miniz.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chrono>

#include "serum-version.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if not defined(__STDC_LIB_EXT1__)

// trivial implementation of the secure string functions if not directly
// upported by the compiler these do not perform all security checks and can be
// improved for sure
int strcpy_s(char* dest, int destsz, const char* src)
{
	if ((dest == NULL) || (src == NULL)) return 1;
	if (strlen(src) >= destsz) return 1;
	strcpy(dest, src);
	return 0;
}

int strcat_s(char* dest, int destsz, const char* src)
{
	if ((dest == NULL) || (src == NULL)) return 1;
	if (strlen(dest) + strlen(src) >= destsz) return 1;
	strcat(dest, src);
	return 0;
}

#endif

#pragma warning(disable : 4996)

const int pathbuflen = 4096;

const int IDENTIFY_SAME_FRAME = -2;

const int IDENTIFY_NO_FRAME = -1;

// header
char rname[64];
UINT8 SerumVersion = 0;
UINT fwidth, fheight;
UINT fwidthx, fheightx;
UINT nframes;
UINT nocolors, nccolors;
UINT ncompmasks, nmovmasks;
UINT nsprites;
UINT16 nbackgrounds;
// data
UINT* hashcodes = NULL;
UINT8* shapecompmode = NULL;
UINT8* compmaskID = NULL;
UINT8* movrctID = NULL;
UINT8* compmasks = NULL;
UINT8* movrcts = NULL;
UINT8* cpal = NULL;
UINT8* isextraframe = NULL;
UINT8* cframes = NULL;
UINT16* cframesn = NULL;
UINT16* cframesnx = NULL;
UINT8* dynamasks = NULL;
UINT8* dynamasksx = NULL;
UINT8* dyna4cols = NULL;
UINT16* dyna4colsn = NULL;
UINT16* dyna4colsnx = NULL;
UINT8* framesprites = NULL;
UINT8* spritedescriptionso = NULL;
UINT8* spritedescriptionsc = NULL;
UINT8* isextrasprite = NULL;
UINT8* spriteoriginal = NULL;
UINT8* spritemaskx = NULL;
UINT16* spritecolored = NULL;
UINT16* spritecoloredx = NULL;
UINT8* activeframes = NULL;
UINT8* colorrotations = NULL;
UINT16* colorrotationsn = NULL;
UINT16* colorrotationsnx = NULL;
UINT16* spritedetareas = NULL;
UINT* spritedetdwords = NULL;
UINT16* spritedetdwordpos = NULL;
UINT* triggerIDs = NULL;
UINT16* framespriteBB = NULL;
UINT8* isextrabackground = NULL;
UINT8* backgroundframes = NULL;
UINT16* backgroundframesn = NULL;
UINT16* backgroundframesnx = NULL;
UINT16* backgroundIDs = NULL;
UINT16* backgroundBB = NULL;
UINT8* backgroundmask = NULL;
UINT8* backgroundmaskx = NULL;

// variables
bool cromloaded = false;  // is there a crom loaded?
UINT16 lastfound = 0;     // last frame ID identified
UINT8* lastframe = NULL;  // last frame content identified
UINT16* lastframe32 = NULL, * lastframe64 = NULL; // last frame content in new version
UINT lastwidth, lasheight; // what were the dimension of the last frame
UINT lastframe_found = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
UINT lastframe_full_crc = 0;
UINT8* lastpalette = NULL;               // last palette identified
UINT8* lastrotations = NULL;             // last colour rotations identified
UINT16* lastrotations32 = NULL;             // last colour rotations identified new version
UINT16* lastrotations64 = NULL;             // last colour rotations identified new version
UINT16* lastrotationsinframe32 = NULL; // last precalculated pixels of the frame that are part of a rotation
UINT16* lastrotationsinframe64 = NULL; // last precalculated pixels of the frame that are part of a rotation
UINT8 lastsprite[MAX_SPRITES_PER_FRAME];  // last sprites identified
UINT lastnsprites = 0;                       // last amount of sprites detected
UINT16 lastfrx[MAX_SPRITES_PER_FRAME], lastfry[MAX_SPRITES_PER_FRAME];  // last position in the frame of the sprite
UINT16 lastspx[MAX_SPRITES_PER_FRAME], lastspy[MAX_SPRITES_PER_FRAME];  // last top left of the sprite to display
UINT16 lastwid[MAX_SPRITES_PER_FRAME], lasthei[MAX_SPRITES_PER_FRAME];  // last dimensions of the sprite to display
UINT lasttriggerID = 0xFFFFFFFF;  // last trigger ID found
bool isrotation = true;             // are there rotations to send
bool crc32_ready = false;           // is the crc32 table filled?
UINT crc32_table[256];            // initial table
bool* framechecked = NULL;          // are these frames checked?
UINT16 ignoreUnknownFramesTimeout = 0;
UINT8 maxFramesToSkip = 0;
UINT8 framesSkippedCounter = 0;
UINT8 standardPalette[PALETTE_SIZE];
UINT8 standardPaletteLength = 0;
UINT colorshifts[MAX_COLOR_ROTATIONS];         // how many color we shifted
UINT colorshiftinittime[MAX_COLOR_ROTATIONS];  // when was the tick for this
UINT colorshifts32[MAX_COLOR_ROTATIONN];         // how many color we shifted for extra res
UINT colorshiftinittime32[MAX_COLOR_ROTATIONN];  // when was the tick for this for extra res
UINT colorshifts64[MAX_COLOR_ROTATIONN];         // how many color we shifted for extra res
UINT colorshiftinittime64[MAX_COLOR_ROTATIONN];  // when was the tick for this for extra res
UINT colorrotseruminit; // initial time when all the rotations started
UINT colorrotnexttime[MAX_COLOR_ROTATIONS]; // next time of the next rotation
UINT colorrotnexttime32[MAX_COLOR_ROTATIONN]; // next time of the next rotation
UINT colorrotnexttime64[MAX_COLOR_ROTATIONN]; // next time of the next rotation
// rotation
bool enabled = true;                             // is colorization enabled?

bool isoriginalrequested = true; // are the original resolution frames requested by the caller
bool isextrarequested = false; // are the extra resolution frames requested by the caller

UINT rotationnextabsolutetime[MAX_COLOR_ROTATIONS]; // cumulative time for the next rotation for each color rotation 

Serum_Frame_Struc mySerum; // structure to keep communicate colorization data

void Free_element(void* pElement)
{
	// free a malloc block and set its pointer to NULL
	if (pElement)
	{
		free(pElement);
		pElement = NULL;
	}
}

void Serum_free(void)
{
	// Free the memory for a full Serum whatever the format version
	Free_element(hashcodes);
	Free_element(shapecompmode);
	Free_element(compmaskID);
	Free_element(movrctID);
	Free_element(compmasks);
	Free_element(movrcts);
	Free_element(cpal);
	Free_element(isextraframe);
	Free_element(cframesn);
	Free_element(cframesnx);
	Free_element(cframes);
	Free_element(dynamasks);
	Free_element(dynamasksx);
	Free_element(dyna4cols);
	Free_element(dyna4colsn);
	Free_element(dyna4colsnx);
	Free_element(framesprites);
	Free_element(spritedescriptionso);
	Free_element(spritedescriptionsc);
	Free_element(isextrasprite);
	Free_element(spriteoriginal);
	Free_element(spritemaskx);
	Free_element(spritecolored);
	Free_element(spritecoloredx);
	Free_element(activeframes);
	Free_element(colorrotations);
	Free_element(colorrotationsn);
	Free_element(colorrotationsnx);
	Free_element(spritedetareas);
	Free_element(spritedetdwords);
	Free_element(spritedetdwordpos);
	Free_element(triggerIDs);
	Free_element(framespriteBB);
	Free_element(isextrabackground);
	Free_element(backgroundframes);
	Free_element(backgroundframesn);
	Free_element(backgroundframesnx);
	Free_element(backgroundIDs);
	Free_element(backgroundBB);
	Free_element(backgroundmask);
	Free_element(backgroundmaskx);
	Free_element(lastframe);
	Free_element(lastframe32);
	Free_element(lastframe64);
	Free_element(lastpalette);
	Free_element(lastrotations);
	Free_element(lastrotations32);
	Free_element(lastrotations64);
	Free_element(lastrotationsinframe32);
	Free_element(lastrotationsinframe64);
	Free_element(framechecked);
	Free_element(mySerum.frame);
	Free_element(mySerum.frame32);
	Free_element(mySerum.frame64);
	Free_element(mySerum.palette);
	Free_element(mySerum.rotations);
	Free_element(mySerum.rotations32);
	Free_element(mySerum.rotations64);
	Free_element(mySerum.rotationsinframe32);
	Free_element(mySerum.rotationsinframe64);
	Free_element(mySerum.modifiedelements32);
	Free_element(mySerum.modifiedelements64);
	cromloaded = false;
}

SERUM_API const char* Serum_GetVersion() { return SERUM_VERSION; }

SERUM_API const char* Serum_GetMinorVersion() { return SERUM_MINOR_VERSION; }

void CRC32encode(void)  // initiating the CRC table, must be called at startup
{
	for (int i = 0; i < 256; i++)
	{
		UINT ch = i;
		UINT crc = 0;
		for (int j = 0; j < 8; j++)
		{
			UINT b = (ch ^ crc) & 1;
			crc >>= 1;
			if (b != 0) crc = crc ^ 0xEDB88320;
			ch >>= 1;
		}
		crc32_table[i] = crc;
	}
	crc32_ready = true;
}

UINT crc32_fast(UINT8* s, UINT n, UINT8 ShapeMode)
// computing a buffer CRC32, "CRC32encode()" must have been called before the first use
{
	UINT crc = 0xFFFFFFFF;
	for (int i = 0; i < (int)n; i++)
	{
		UINT8 val = s[i];
		if ((ShapeMode == 1) && (val > 1)) val = 1;
		crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
	}
	return ~crc;
}

UINT crc32_fast_mask(UINT8* source, UINT8* mask, UINT n, UINT8 ShapeMode)
// computing a buffer CRC32 on the non-masked area,
// "CRC32encode()" must have been called before the first use
// take into account if we are in shape mode
{
	UINT crc = 0xFFFFFFFF;
	for (UINT i = 0; i < n; i++)
	{
		if (mask[i] == 0)
		{
			UINT8 val = source[i];
			if ((ShapeMode == 1) && (val > 1)) val = 1;
			crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
		}
	}
	return ~crc;
}

bool unzip_crz(const char* const filename, const char* const extractpath, char* cromname, int cromsize)
{
	bool ok = true;
	mz_zip_archive zip_archive;
	memset(&zip_archive, 0, sizeof(zip_archive));

	if (!mz_zip_reader_init_file(&zip_archive, filename, 0))
	{
		return false;
	}

	int num_files = mz_zip_reader_get_num_files(&zip_archive);

	if (num_files == 0 || !mz_zip_reader_get_filename(&zip_archive, 0, cromname, cromsize))
	{
		mz_zip_reader_end(&zip_archive);
		return false;
	}

	for (int i = 0; i < num_files; i++)
	{
		mz_zip_archive_file_stat file_stat;
		if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;

		char dstPath[pathbuflen];
		if (strcpy_s(dstPath, pathbuflen, extractpath)) goto fail;
		if (strcat_s(dstPath, pathbuflen, file_stat.m_filename)) goto fail;

		mz_zip_reader_extract_file_to_file(&zip_archive, file_stat.m_filename,
			dstPath, 0);
	}

	goto nofail;
fail:
	ok = false;
nofail:

	mz_zip_reader_end(&zip_archive);

	return ok;
}

void Full_Reset_ColorRotations(void)
{
	memset(colorshifts, 0, MAX_COLOR_ROTATIONS * sizeof(UINT));
	colorrotseruminit = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	for (int ti = 0; ti < MAX_COLOR_ROTATIONS; ti++) colorshiftinittime[ti] = colorrotseruminit;
	memset(colorshifts32, 0, MAX_COLOR_ROTATIONN * sizeof(UINT));
	memset(colorshifts64, 0, MAX_COLOR_ROTATIONN * sizeof(UINT));
	for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
	{
		colorshiftinittime32[ti] = colorrotseruminit;
		colorshiftinittime64[ti] = colorrotseruminit;
	}
}

UINT max(UINT v1, UINT v2)
{
	if (v1 > v2) return v1;
	return v2;
}

UINT min(UINT v1, UINT v2)
{
	if (v1 < v2) return v1;
	return v2;
}

long serum_file_length;
long long ac_pos_in_file;
FILE* fconsole;

const bool IS_DEBUG_READ = false;

size_t my_fread(void* pBuffer, size_t sizeElement, size_t nElements, FILE* stream)
{
	size_t readelem = fread(pBuffer, sizeElement, nElements, stream);
	ac_pos_in_file += readelem * sizeElement;
	if (IS_DEBUG_READ) printf("sent elements: %llu / written elements: %llu / written bytes: %llu / current position: %llu\r", nElements, readelem, readelem * sizeElement, ac_pos_in_file);
	return readelem;
}

Serum_Frame_Struc* Serum_LoadFilev2(FILE* pfile, const UINT8 flags, bool uncompressedCROM, char* pathbuf)
{
	mySerum.SerumVersion = SerumVersion = SERUM_V2;
	my_fread(&fwidth, 4, 1, pfile);
	my_fread(&fheight, 4, 1, pfile);
	my_fread(&fwidthx, 4, 1, pfile);
	my_fread(&fheightx, 4, 1, pfile);
	isoriginalrequested = false;
	isextrarequested = false;
	mySerum.width32 = 0;
	mySerum.width64 = 0;
	if (fheight == 32)
	{
		if (flags & FLAG_REQUEST_32P_FRAMES)
		{
			isoriginalrequested = true;
			mySerum.width32 = fwidth;
		}
		if (flags & FLAG_REQUEST_64P_FRAMES)
		{
			isextrarequested = true;
			mySerum.width64 = fwidthx;
		}

	}
	else
	{
		if (flags & FLAG_REQUEST_64P_FRAMES)
		{
			isoriginalrequested = true;
			mySerum.width64 = fwidth;
		}
		if (flags & FLAG_REQUEST_32P_FRAMES)
		{
			isextrarequested = true;
			mySerum.width32 = fwidthx;
		}
	}
	my_fread(&nframes, 4, 1, pfile);
	my_fread(&nocolors, 4, 1, pfile);
	mySerum.nocolors = nocolors;
	if ((fwidth == 0) || (fheight == 0) || (nframes == 0) || (nocolors == 0))
	{
		// incorrect file format
		fclose(pfile);
		enabled = false;
		return NULL;
	}
	my_fread(&ncompmasks, 4, 1, pfile);
	my_fread(&nsprites, 4, 1, pfile);
	my_fread(&nbackgrounds, 2, 1, pfile); // nbackgrounds is a UINT16

	hashcodes = (UINT*)malloc(sizeof(UINT) * nframes);
	shapecompmode = (UINT8*)malloc(nframes);
	compmaskID = (UINT8*)malloc(nframes);
	compmasks = (UINT8*)malloc(ncompmasks * fwidth * fheight);
	isextraframe = (UINT8*)malloc(nframes);
	cframesn = (UINT16*)malloc(nframes * fwidth * fheight * sizeof(UINT16));
	cframesnx = (UINT16*)malloc(nframes * fwidthx * fheightx * sizeof(UINT16));
	dynamasks = (UINT8*)malloc(nframes * fwidth * fheight);
	dynamasksx = (UINT8*)malloc(nframes * fwidthx * fheightx);
	dyna4colsn = (UINT16*)malloc(nframes * MAX_DYNA_SETS_PER_FRAMEN * nocolors * sizeof(UINT16));
	dyna4colsnx = (UINT16*)malloc(nframes * MAX_DYNA_SETS_PER_FRAMEN * nocolors * sizeof(UINT16));
	isextrasprite = (UINT8*)malloc(nsprites);
	framesprites = (UINT8*)malloc(nframes * MAX_SPRITES_PER_FRAME);
	framespriteBB = (UINT16*)malloc(nframes * MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16));
	spriteoriginal = (UINT8*)malloc(nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
	spritecolored = (UINT16*)malloc(nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
	spritemaskx = (UINT8*)malloc(nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT);
	spritecoloredx = (UINT16*)malloc(nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT * sizeof(UINT16));
	activeframes = (UINT8*)malloc(nframes);
	colorrotationsn = (UINT16*)malloc(nframes * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
	colorrotationsnx = (UINT16*)malloc(nframes * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN * sizeof(UINT16));
	spritedetdwords = (UINT*)malloc(nsprites * sizeof(UINT) * MAX_SPRITE_DETECT_AREAS);
	spritedetdwordpos = (UINT16*)malloc(nsprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS);
	spritedetareas = (UINT16*)malloc(nsprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS * 4);
	triggerIDs = (UINT*)malloc(nframes * sizeof(UINT));
	isextrabackground = (UINT8*)malloc(nbackgrounds);
	backgroundframesn = (UINT16*)malloc(nbackgrounds * fwidth * fheight * sizeof(UINT16));
	backgroundframesnx = (UINT16*)malloc(nbackgrounds * fwidthx * fheightx * sizeof(UINT16));
	backgroundIDs = (UINT16*)malloc(nframes * sizeof(UINT16));
	backgroundmask = (UINT8*)malloc(nframes * fwidth * fheight);
	backgroundmaskx = (UINT8*)malloc(nframes * fwidthx * fheightx);
	if (!hashcodes || !shapecompmode || !compmaskID || (ncompmasks > 0 && !compmasks) || !isextraframe ||
		!cframesn || !cframesnx || !dynamasks || !dynamasksx || !dyna4colsn || !dyna4colsnx ||
		(nsprites > 0 && (!isextrasprite || !spriteoriginal || !spritecolored || !spritemaskx ||
			!spritecoloredx || !spritedetdwords || !spritedetdwordpos || !spritedetareas)) ||
		!framesprites || !framespriteBB || !activeframes || !colorrotationsn || !colorrotationsnx ||
		!triggerIDs || (nbackgrounds > 0 && (!isextrabackground || !backgroundframesn ||
			!backgroundframesnx)) || !backgroundIDs || !backgroundmask || !backgroundmaskx)
	{
		Serum_free();
		fclose(pfile);
		enabled = false;
		return NULL;
	}
	if (flags & FLAG_REQUEST_32P_FRAMES)
	{
		mySerum.frame32 = (UINT16*)malloc(32 * mySerum.width32 * sizeof(UINT16));
		mySerum.rotations32 = (UINT16*)malloc(MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * sizeof(UINT16));
		mySerum.rotationsinframe32 = (UINT16*)malloc(2 * 32 * mySerum.width32 * sizeof(UINT16));
		if (flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS)
			mySerum.modifiedelements32 = (UINT8*)malloc(32 * mySerum.width32);
		if (!mySerum.frame32 || !mySerum.rotations32 || !mySerum.rotationsinframe32 ||
			(flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS && !mySerum.modifiedelements32))
		{
			Serum_free();
			fclose(pfile);
			enabled = false;
			return NULL;
		}
	}
	if (flags & FLAG_REQUEST_64P_FRAMES)
	{
		mySerum.frame64 = (UINT16*)malloc(64 * mySerum.width64 * sizeof(UINT16));
		mySerum.rotations64 = (UINT16*)malloc(MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * sizeof(UINT16));
		mySerum.rotationsinframe64 = (UINT16*)malloc(2 * 64 * mySerum.width64 * sizeof(UINT16));
		if (flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS)
			mySerum.modifiedelements64 = (UINT8*)malloc(64 * mySerum.width64);
		if (!mySerum.frame64 || !mySerum.rotations64 || !mySerum.rotationsinframe64 ||
			(flags & FLAG_REQUEST_FILL_MODIFIED_ELEMENTS && !mySerum.modifiedelements64))
		{
			Serum_free();
			fclose(pfile);
			enabled = false;
			return NULL;
		}
	}

	my_fread(hashcodes, 4, nframes, pfile);
	my_fread(shapecompmode, 1, nframes, pfile);
	my_fread(compmaskID, 1, nframes, pfile);
	my_fread(compmasks, 1, ncompmasks * fwidth * fheight, pfile);
	my_fread(isextraframe, 1, nframes, pfile);
	my_fread(cframesn, 2, nframes * fwidth * fheight, pfile);
	my_fread(cframesnx, 2, nframes * fwidthx * fheightx, pfile);
	my_fread(dynamasks, 1, nframes * fwidth * fheight, pfile);
	my_fread(dynamasksx, 1, nframes * fwidthx * fheightx, pfile);
	my_fread(dyna4colsn, 2, nframes * MAX_DYNA_SETS_PER_FRAMEN * nocolors, pfile);
	my_fread(dyna4colsnx, 2, nframes * MAX_DYNA_SETS_PER_FRAMEN * nocolors, pfile);
	my_fread(isextrasprite, 1, nsprites, pfile);
	my_fread(framesprites, 1, nframes * MAX_SPRITES_PER_FRAME, pfile);
	my_fread(spriteoriginal, 1, nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, pfile);
	my_fread(spritecolored, 2, nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, pfile);
	my_fread(spritemaskx, 1, nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, pfile);
	my_fread(spritecoloredx, 2, nsprites * MAX_SPRITE_WIDTH * MAX_SPRITE_HEIGHT, pfile);
	my_fread(activeframes, 1, nframes, pfile);
	my_fread(colorrotationsn, 2, nframes * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN, pfile);
	my_fread(colorrotationsnx, 2, nframes * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN, pfile);
	my_fread(spritedetdwords, 4, nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
	my_fread(spritedetdwordpos, 2, nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
	my_fread(spritedetareas, 2, nsprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile);
	my_fread(triggerIDs, 4, nframes, pfile);
	my_fread(framespriteBB, 2, nframes * MAX_SPRITES_PER_FRAME * 4, pfile);
	my_fread(isextrabackground, 1, nbackgrounds, pfile);
	my_fread(backgroundframesn, 2, nbackgrounds * fwidth * fheight, pfile);
	my_fread(backgroundframesnx, 2, nbackgrounds * fwidthx * fheightx, pfile);
	my_fread(backgroundIDs, 2, nframes, pfile);
	my_fread(backgroundmask, 1, nframes * fwidth * fheight, pfile);
	my_fread(backgroundmaskx, 1, nframes * fwidthx * fheightx, pfile);

	fclose(pfile);

	mySerum.ntriggers = 0;
	for (UINT ti = 0; ti < nframes; ti++)
	{
		if (triggerIDs[ti] != 0xFFFFFFFF) mySerum.ntriggers++;
	}
	// allocate memory for previous detected frame
	lastframe32 = (UINT16*)malloc(min(fwidth, fwidthx) * 32 * 2);
	lastframe64 = (UINT16*)malloc(max(fwidth, fwidthx) * 64 * 2);
	lastrotations32 = (UINT16*)malloc(MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
	lastrotations64 = (UINT16*)malloc(MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
	lastrotationsinframe32 = (UINT16*)malloc(min(fwidth, fwidthx) * 32 * 2 * 2);
	lastrotationsinframe64 = (UINT16*)malloc(max(fwidth, fwidthx) * 64 * 2 * 2);
	framechecked = (bool*)malloc(sizeof(bool) * nframes);
	if (!lastframe32 || !lastframe64 || !lastrotations32 || !lastrotations64 || !lastrotationsinframe32 || !lastrotationsinframe64 || !framechecked)
	{
		Serum_free();
		enabled = false;
		return NULL;
	}
	memset(lastframe32, 0, min(fwidth, fwidthx) * 32 * 2);
	memset(lastframe64, 0, max(fwidth, fwidthx) * 64 * 2);
	memset(lastrotations32, 0, MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
	memset(lastrotations64, 0, MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
	memset(lastrotationsinframe32, 0xff, min(fwidth, fwidthx) * 32 * 2 * 2);
	memset(lastrotationsinframe64, 0xff, max(fwidth, fwidthx) * 64 * 2 * 2);
	memset(lastsprite, 255, MAX_SPRITES_PER_FRAME);

	if (flags & FLAG_REQUEST_32P_FRAMES)
	{
		if (fheight == 32) mySerum.width32 = fwidth;
		else mySerum.width32 = fwidthx;
	}
	else mySerum.width32 = 0;
	if (flags & FLAG_REQUEST_64P_FRAMES)
	{
		if (fheight == 32) mySerum.width64 = fwidthx;
		else mySerum.width64 = fwidth;
	}
	else mySerum.width64 = 0;

	Full_Reset_ColorRotations();
	cromloaded = true;

	if (!uncompressedCROM)
	{
		// remove temporary file that had been extracted from compressed CRZ file
		remove(pathbuf);
	}

	if (IS_DEBUG_READ) fclose(fconsole);

	enabled = true;
	return &mySerum;
}

Serum_Frame_Struc* Serum_LoadFilev1(const char* const filename, const UINT8 flags)
{
	char pathbuf[pathbuflen];
	ac_pos_in_file = 0;
	if (!crc32_ready) CRC32encode();

	// check if we're using an uncompressed cROM file
	const char* ext;
	bool uncompressedCROM = false;
	if ((ext = strrchr(filename, '.')) != NULL)
	{
		if (strcmp(ext, ".cROM") == 0)
		{
			uncompressedCROM = true;
			if (strcpy_s(pathbuf, pathbuflen, filename)) return NULL;
		}
	}

	// extract file if it is compressed
	if (!uncompressedCROM)
	{
		char cromname[pathbuflen];
#if !(defined(__APPLE__) && ((defined(TARGET_OS_IOS) && TARGET_OS_IOS) || (defined(TARGET_OS_TV) && TARGET_OS_TV)))
		if (strcpy_s(pathbuf, pathbuflen, filename)) return NULL;
#else
		if (strcpy_s(pathbuf, pathbuflen, getenv("TMPDIR"))) return NULL;
		if (strcat_s(pathbuf, pathbuflen, "/")) return NULL;
#endif
		if (!unzip_crz(filename, pathbuf, cromname, pathbuflen)) return NULL;
		if (strcat_s(pathbuf, pathbuflen, cromname)) return NULL;
	}

	// Open cRom
	FILE* pfile;
	pfile = fopen(pathbuf, "rb");
	if (!pfile)
	{
		enabled = false;
		return NULL;
	}

	if (IS_DEBUG_READ)
	{
		fconsole = freopen("e:\\output.txt", "w", stdout);
		fseek(pfile, 0, SEEK_END);
		serum_file_length = ftell(pfile);
		fseek(pfile, 0, SEEK_SET);
	}

	// read the header to know how much memory is needed
	my_fread(rname, 1, 64, pfile);
	UINT sizeheader;
	my_fread(&sizeheader, 4, 1, pfile);
	// if this is a new format file, we load with Serum_LoadNewFile()
	if (sizeheader >= 14 * sizeof(UINT)) return Serum_LoadFilev2(pfile, flags, uncompressedCROM, pathbuf);
	mySerum.SerumVersion = SerumVersion = SERUM_V1;
	my_fread(&fwidth, 4, 1, pfile);
	my_fread(&fheight, 4, 1, pfile);
	// The serum file stored the number of frames as UINT, but in fact, the
	// number of frames will never exceed the size of UINT16 (65535)
	UINT nframes32;
	my_fread(&nframes32, 4, 1, pfile);
	nframes = (UINT16)nframes32;
	my_fread(&nocolors, 4, 1, pfile);
	mySerum.nocolors = nocolors;
	my_fread(&nccolors, 4, 1, pfile);
	if ((fwidth == 0) || (fheight == 0) || (nframes == 0) || (nocolors == 0) || (nccolors == 0))
	{
		// incorrect file format
		fclose(pfile);
		enabled = false;
		return NULL;
	}
	my_fread(&ncompmasks, 4, 1, pfile);
	my_fread(&nmovmasks, 4, 1, pfile);
	my_fread(&nsprites, 4, 1, pfile);
	if (sizeheader >= 13 * sizeof(UINT))
		my_fread(&nbackgrounds, 2, 1, pfile);
	else
		nbackgrounds = 0;
	// allocate memory for the serum format
	hashcodes = (UINT*)malloc(sizeof(UINT) * nframes);
	shapecompmode = (UINT8*)malloc(nframes);
	compmaskID = (UINT8*)malloc(nframes);
	movrctID = (UINT8*)malloc(nframes);
	compmasks = (UINT8*)malloc(ncompmasks * fwidth * fheight);
	movrcts = (UINT8*)malloc(nmovmasks * 4);
	cpal = (UINT8*)malloc(nframes * 3 * nccolors);
	cframes = (UINT8*)malloc(nframes * fwidth * fheight);
	dynamasks = (UINT8*)malloc(nframes * fwidth * fheight);
	dyna4cols = (UINT8*)malloc(nframes * MAX_DYNA_4COLS_PER_FRAME * nocolors);
	framesprites = (UINT8*)malloc(nframes * MAX_SPRITES_PER_FRAME);
	framespriteBB = (UINT16*)malloc(nframes * MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16));
	spritedescriptionso = (UINT8*)malloc(nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
	spritedescriptionsc = (UINT8*)malloc(nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
	activeframes = (UINT8*)malloc(nframes);
	colorrotations = (UINT8*)malloc(nframes * 3 * MAX_COLOR_ROTATIONS);
	spritedetdwords = (UINT*)malloc(nsprites * sizeof(UINT) * MAX_SPRITE_DETECT_AREAS);
	spritedetdwordpos = (UINT16*)malloc(nsprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS);
	spritedetareas = (UINT16*)malloc(nsprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS * 4);
	triggerIDs = (UINT*)malloc(nframes * sizeof(UINT));
	backgroundframes = (UINT8*)malloc(nbackgrounds * fwidth * fheight);
	backgroundIDs = (UINT16*)malloc(nframes * sizeof(UINT16));
	backgroundBB = (UINT16*)malloc(nframes * 4 * sizeof(UINT16));
	mySerum.frame = (UINT8*)malloc(fwidth * fheight);
	mySerum.palette = (UINT8*)malloc(3 * 64);
	mySerum.rotations = (UINT8*)malloc(MAX_COLOR_ROTATIONS * 3);
	if (!hashcodes || !shapecompmode || !compmaskID || !movrctID || !cpal ||
		!cframes || !dynamasks || !dyna4cols || !framesprites || !framespriteBB ||
		!activeframes || !colorrotations || !triggerIDs ||
		(!compmasks && ncompmasks > 0) || (!movrcts && nmovmasks > 0) ||
		((nsprites > 0) && (!spritedescriptionso || !spritedescriptionsc || !spritedetdwords ||
		!spritedetdwordpos || !spritedetareas)) ||
		((nbackgrounds > 0) && (!backgroundframes || !backgroundIDs || !backgroundBB)) ||
		!mySerum.frame || !mySerum.palette || !mySerum.rotations)
	{
		Serum_free();
		fclose(pfile);
		enabled = false;
		return NULL;
	}
	// read the cRom file
	my_fread(hashcodes, sizeof(UINT), nframes, pfile);
	my_fread(shapecompmode, 1, nframes, pfile);
	my_fread(compmaskID, 1, nframes, pfile);
	my_fread(movrctID, 1, nframes, pfile);
	my_fread(compmasks, 1, ncompmasks * fwidth * fheight, pfile);
	my_fread(movrcts, 1, nmovmasks * fwidth * fheight, pfile);
	my_fread(cpal, 1, nframes * 3 * nccolors, pfile);
	my_fread(cframes, 1, nframes * fwidth * fheight, pfile);
	my_fread(dynamasks, 1, nframes * fwidth * fheight, pfile);
	my_fread(dyna4cols, 1, nframes * MAX_DYNA_4COLS_PER_FRAME * nocolors, pfile);
	my_fread(framesprites, 1, nframes * MAX_SPRITES_PER_FRAME, pfile);
	for (int ti = 0; ti < (int)nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE; ti++)
	{
		my_fread(&spritedescriptionsc[ti], 1, 1, pfile);
		my_fread(&spritedescriptionso[ti], 1, 1, pfile);
	}
	my_fread(activeframes, 1, nframes, pfile);
	my_fread(colorrotations, 1, nframes * 3 * MAX_COLOR_ROTATIONS, pfile);
	my_fread(spritedetdwords, sizeof(UINT), nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
	my_fread(spritedetdwordpos, sizeof(UINT16), nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
	my_fread(spritedetareas, sizeof(UINT16), nsprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile);
	mySerum.ntriggers = 0;
	if (sizeheader >= 11 * sizeof(UINT))
	{
		my_fread(triggerIDs, sizeof(UINT), nframes, pfile);
		for (UINT ti = 0; ti < nframes; ti++)
		{
			if (triggerIDs[ti] != 0xFFFFFFFF) mySerum.ntriggers++;
		}
	}
	else memset(triggerIDs, 0xFF, sizeof(UINT) * nframes);
	if (sizeheader >= 12 * sizeof(UINT)) my_fread(framespriteBB, sizeof(UINT16), nframes * MAX_SPRITES_PER_FRAME * 4, pfile);
	else
	{
		for (UINT tj = 0; tj < nframes; tj++)
		{
			for (UINT ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
			{
				framespriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = 0;
				framespriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = 0;
				framespriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = fwidth - 1;
				framespriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] =
					fheight - 1;
			}
		}
	}
	if (sizeheader >= 13 * sizeof(UINT))
	{
		my_fread(backgroundframes, fwidth * fheight, nbackgrounds, pfile);
		my_fread(backgroundIDs, sizeof(UINT16), nframes, pfile);
		my_fread(backgroundBB, 4 * sizeof(UINT16), nframes, pfile);
	}
	else memset(backgroundIDs, 0xFF, nframes * sizeof(UINT16));
	fclose(pfile);

	if (IS_DEBUG_READ) fclose(fconsole);

	// allocate memory for previous detected frame
	lastframe = (UINT8*)malloc(fwidth * fheight);
	lastpalette = (UINT8*)malloc(nccolors * 3);
	lastrotations = (UINT8*)malloc(3 * MAX_COLOR_ROTATIONS);
	framechecked = (bool*)malloc(sizeof(bool) * nframes);
	if (!lastframe || !lastpalette || !lastrotations || !framechecked) {
		Serum_free();
		enabled = false;
		return NULL;
	}
	memset(lastframe, 0, fwidth * fheight);
	memset(lastpalette, 0, nccolors * 3);
	memset(lastrotations, 255, 3 * MAX_COLOR_ROTATIONS);
	memset(lastsprite, 255, MAX_SPRITES_PER_FRAME);
	if (fheight == 64)
	{
		mySerum.width64 = fwidth;
		mySerum.width32 = 0;
	}
	else
	{
		mySerum.width32 = fwidth;
		mySerum.width64 = 0;
	}
	Full_Reset_ColorRotations();
	cromloaded = true;

	if (!uncompressedCROM)
	{
		// remove temporary file that had been extracted from compressed CRZ file
		remove(pathbuf);
	}

	enabled = true;
	return &mySerum;
}

SERUM_API Serum_Frame_Struc* Serum_Load(const char* const altcolorpath, const char* const romname, UINT8 flags)
{
	mySerum.frame = NULL;
	mySerum.frame32 = NULL;
	mySerum.frame64 = NULL;
	mySerum.palette = NULL;
	mySerum.rotations = NULL;
	mySerum.rotations32 = NULL;
	mySerum.rotations64 = NULL;
	mySerum.rotationsinframe32 = NULL;
	mySerum.rotationsinframe64 = NULL;
	mySerum.modifiedelements32 = NULL;
	mySerum.modifiedelements64 = NULL;
	char pathbuf[pathbuflen];
	if (strcpy_s(pathbuf, pathbuflen, altcolorpath) || ((pathbuf[strlen(pathbuf) - 1] != '\\') && (pathbuf[strlen(pathbuf) - 1] != '/') &&
		strcat_s(pathbuf, pathbuflen, "/")) || strcat_s(pathbuf, pathbuflen, romname) || strcat_s(pathbuf, pathbuflen, "/") ||
		strcat_s(pathbuf, pathbuflen, romname) || strcat_s(pathbuf, pathbuflen, ".cRZ"))
	{
		enabled = false;
		return NULL;
	}
	return Serum_LoadFilev1(pathbuf, flags);
}

SERUM_API void Serum_Dispose(void)
{
	Serum_free();
}

int Identify_Frame(UINT8* frame)
{
	// Usually the first frame has the ID 0, but lastfound is also initialized
	// with 0. So we need a helper to be able to detect frame 0 as new.
	static bool first_match = true;

	if (!cromloaded) return IDENTIFY_NO_FRAME;
	UINT8* pmask;
	memset(framechecked, false, nframes);
	UINT16 tj = lastfound;  // we start from the frame we last found
	UINT pixels = fwidth * fheight;
	do
	{
		if (!framechecked[tj])
		{
			// calculate the hashcode for the generated frame with the mask and
			// shapemode of the current crom frame
			UINT8 mask = compmaskID[tj];
			UINT8 Shape = shapecompmode[tj];
			UINT Hashc;
			if (mask < 255)
			{
				pmask = &compmasks[mask * pixels];
				Hashc = crc32_fast_mask(frame, pmask, pixels, Shape);
			}
			else Hashc = crc32_fast(frame, pixels, Shape);
			// now we can compare with all the crom frames that share these same mask
			// and shapemode
			UINT16 ti = tj;
			do
			{
				if (!framechecked[ti])
				{
					if ((compmaskID[ti] == mask) && (shapecompmode[ti] == Shape))
					{
						if (Hashc == hashcodes[ti])
						{
							if (first_match || ti != lastfound || mask < 255)
							{
								//Reset_ColorRotations();
								lastfound = ti;
								lastframe_full_crc = crc32_fast(frame, pixels, 0);
								first_match = false;
								return ti;  // we found the frame, we return it
							}

							UINT full_crc = crc32_fast(frame, pixels, 0);
							if (full_crc != lastframe_full_crc)
							{
								lastframe_full_crc = full_crc;
								return ti;  // we found the same frame with shape as before, but
								// the full frame is different
							}
							return IDENTIFY_SAME_FRAME;  // we found the frame, but it is the
							// same full frame as before (no
							// mask)
						}
						framechecked[ti] = true;
					}
				}
				if (++ti >= nframes) ti = 0;
			} while (ti != tj);
		}
		if (++tj >= nframes) tj = 0;
	} while (tj != lastfound);

	return IDENTIFY_NO_FRAME;  // we found no corresponding frame
}

void GetSpriteSize(int nospr, int* pswid, int* pshei, UINT8* pspro, int sswid, int sshei)
{
	*pswid = *pshei = 0;
	if (nospr >= nsprites) return;
	for (int tj = 0; tj < sshei; tj++)
	{
		for (int ti = 0; ti < sswid; ti++)
		{
			if (pspro[nospr * sswid * sshei + tj * sswid + ti] < 255)
			{
				if (tj > *pshei) *pshei = tj;
				if (ti > *pswid) *pswid = ti;
			}
		}
	}
	(*pshei)++;
	(*pswid)++;
}

bool Check_Sprites(UINT8* Frame, int quelleframe, UINT8* pquelsprites, UINT8* nspr, UINT16* pfrx, UINT16* pfry, UINT16* pspx, UINT16* pspy, UINT16* pwid, UINT16* phei)
{
	UINT8 ti = 0;
	UINT mdword;
	*nspr = 0;
	int spr_width, spr_height;
	UINT8* pspro;
	if (SerumVersion == SERUM_V2)
	{
		spr_width = MAX_SPRITE_WIDTH;
		spr_height = MAX_SPRITE_HEIGHT;
		pspro = spriteoriginal;
	}
	else
	{
		spr_width = MAX_SPRITE_SIZE;
		spr_height = MAX_SPRITE_SIZE;
		pspro = spritedescriptionso;
	}
	while ((ti < MAX_SPRITES_PER_FRAME) && (framesprites[quelleframe * MAX_SPRITES_PER_FRAME + ti] < 255))
	{
		UINT8 qspr = framesprites[quelleframe * MAX_SPRITES_PER_FRAME + ti];
		int spw, sph;
		GetSpriteSize(qspr, &spw, &sph, pspro, spr_width, spr_height);
		short minxBB = (short)framespriteBB[(quelleframe * MAX_SPRITES_PER_FRAME + ti) * 4];
		short minyBB = (short)framespriteBB[(quelleframe * MAX_SPRITES_PER_FRAME + ti) * 4 + 1];
		short maxxBB = (short)framespriteBB[(quelleframe * MAX_SPRITES_PER_FRAME + ti) * 4 + 2];
		short maxyBB = (short)framespriteBB[(quelleframe * MAX_SPRITES_PER_FRAME + ti) * 4 + 3];
		for (UINT tm = 0; tm < MAX_SPRITE_DETECT_AREAS; tm++)
		{
			if (spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4] == 0xffff) continue;
			// we look for the sprite in the frame sent
			for (short ty = minyBB; ty <= maxyBB; ty++)
			{
				mdword = (UINT)(Frame[ty * fwidth + minxBB] << 8) | (UINT)(Frame[ty * fwidth + minxBB + 1] << 16) |
					(UINT)(Frame[ty * fwidth + minxBB + 2] << 24);
				for (short tx = minxBB; tx <= maxxBB - 3; tx++)
				{
					UINT tj = ty * fwidth + tx;
					mdword = (mdword >> 8) | (UINT)(Frame[tj + 3] << 24);
					// we look for the magic dword first:
					UINT16 sddp = spritedetdwordpos[qspr * MAX_SPRITE_DETECT_AREAS + tm];
					if (mdword == spritedetdwords[qspr * MAX_SPRITE_DETECT_AREAS + tm])
					{
						short frax = (short)tx; // position in the frame of the detection dword
						short fray = (short)ty;
						short sprx = (short)(sddp % spr_width); // position in the sprite of the detection dword
						short spry = (short)(sddp / spr_width);
						// details of the det area:
						short detx = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4]; // position of the detection area in the sprite
						short dety = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 1];
						short detw = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 2]; // size of the detection area
						short deth = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 3];
						// if the detection area starts before the frame (left or top), continue:
						if ((frax - minxBB < sprx - detx) || (fray - minyBB < spry - dety)) continue;
						// position of the detection area in the frame
						int offsx = frax - sprx + detx;
						int offsy = fray - spry + dety;
						// if the detection area extends beyond the bounding box (right or bottom), continue:
						if ((offsx + detw > (int)maxxBB + 1) || (offsy + deth > (int)maxyBB + 1)) continue;
						// we can now check if the full detection area is around the found detection dword
						bool notthere = false;
						for (UINT16 tk = 0; tk < deth; tk++)
						{
							for (UINT16 tl = 0; tl < detw; tl++)
							{
								UINT8 val = pspro[qspr * spr_height * spr_width + (tk + dety) * spr_width + tl + detx];
								if (val == 255) continue;
								if (val != Frame[(tk + offsy) * fwidth + tl + offsx])
								{
									notthere = true;
									break;
								}
							}
							if (notthere == true) break;
						}
						if (!notthere)
						{
							pquelsprites[*nspr] = qspr;
							if (frax - minxBB < sprx)
							{
								pspx[*nspr] = (UINT16)(sprx - (frax - minxBB)); // display sprite from point
								pfrx[*nspr] = (UINT16)minxBB;
								pwid[*nspr] = (UINT16)(spw - pspx[*nspr]);
							}
							else
							{
								pspx[*nspr] = 0;
								pfrx[*nspr] = (UINT16)(frax - sprx);
								pwid[*nspr] = MIN((UINT16)(maxxBB - pfrx[*nspr] + 1), (UINT16)spw);
							}
							if (fray - minyBB < spry)
							{
								pspy[*nspr] = (UINT16)(spry - (fray - minyBB));
								pfry[*nspr] = (UINT16)minyBB;
								phei[*nspr] = (UINT16)(sph - pspy[*nspr]);
							}
							else
							{
								pspy[*nspr] = 0;
								pfry[*nspr] = (UINT16)(fray - spry);
								phei[*nspr] = MIN((UINT16)(maxyBB - pfry[*nspr] + 1), (UINT16)sph);
							}
							// we check the identical sprites as there may be duplicate due to
							// the multi detection zones
							bool identicalfound = false;
							for (UINT8 tk = 0; tk < *nspr; tk++)
							{
								if ((pquelsprites[*nspr] == pquelsprites[tk]) &&
									(pfrx[*nspr] == pfrx[tk]) && (pfry[*nspr] == pfry[tk]) &&
									(pwid[*nspr] == pwid[tk]) && (phei[*nspr] == phei[tk]))
									identicalfound = true;
							}
							if (!identicalfound)
							{
								(*nspr)++;
								if (*nspr == MAX_SPRITES_PER_FRAME) return true;
							}
						}
					}
				}
			}
		}
		ti++;
	}
	if (*nspr > 0) return true;
	return false;
}

void Colorize_Framev1(UINT8* frame, int IDfound)
{
	UINT16 tj, ti;
	// Generate the colorized version of a frame once identified in the crom
	// frames
	for (tj = 0; tj < fheight; tj++)
	{
		for (ti = 0; ti < fwidth; ti++)
		{
			UINT16 tk = tj * fwidth + ti;

			if ((backgroundIDs[IDfound] < nbackgrounds) && (frame[tk] == 0) && (ti >= backgroundBB[IDfound * 4]) &&
				(tj >= backgroundBB[IDfound * 4 + 1]) && (ti <= backgroundBB[IDfound * 4 + 2]) &&
				(tj <= backgroundBB[IDfound * 4 + 3]))
				mySerum.frame[tk] = backgroundframes[backgroundIDs[IDfound] * fwidth * fheight + tk];
			else
			{
				UINT8 dynacouche = dynamasks[IDfound * fwidth * fheight + tk];
				if (dynacouche == 255) mySerum.frame[tk] = cframes[IDfound * fwidth * fheight + tk];
				else mySerum.frame[tk] = dyna4cols[IDfound * MAX_DYNA_4COLS_PER_FRAME * nocolors + dynacouche * nocolors + frame[tk]];
			}
		}
	}
}

bool CheckExtraFrameAvailable(UINT frID)
{
	// Check if there is an extra frame for this frame
	// (and if all the sprites and background involved are available)
	if (isextraframe[frID] == 0) return false;
	if (backgroundIDs[frID] < 0xffff && isextrabackground[backgroundIDs[frID]] == 0) return false;
	for (UINT ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
	{
		if (framesprites[frID * MAX_SPRITES_PER_FRAME + ti] < 255 && isextrasprite[framesprites[frID * MAX_SPRITES_PER_FRAME + ti]] == 0) return false;
	}
	return true;
}

bool ColorInRotation(int IDfound, UINT16 col, UINT16* norot, UINT16* posinrot, bool isextra)
{
	UINT16* pcol = NULL;
	if (isextra) pcol = &colorrotationsnx[IDfound * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
	else pcol = &colorrotationsn[IDfound * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
	*norot = 0xffff;
	for (UINT ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
	{
		for (UINT tj = 2; tj < 2 + pcol[ti * MAX_LENGTH_COLOR_ROTATION]; tj++) // val [0] is for length and val [1] is for duration in ms
		{
			if (col == pcol[ti * MAX_LENGTH_COLOR_ROTATION + tj])
			{
				*norot = ti;
				*posinrot = tj - 2; // val [0] is for length and val [1] is for duration in ms
				return true;
			}
		}
	}
	return false;
}

void Colorize_Framev2(UINT8* frame, int IDfound)
{
	UINT16 tj, ti;
	// Generate the colorized version of a frame once identified in the crom
	// frames
	bool isextra = CheckExtraFrameAvailable(IDfound);
	mySerum.flags = 0;
	UINT16* pfr;
	UINT16* prot;
	UINT16* prt;
	UINT* cshft;
	if (mySerum.frame32) mySerum.width32 = 0;
	if (mySerum.frame64) mySerum.width64 = 0;
	if (((mySerum.frame32 && fheight == 32) || (mySerum.frame64 && fheight == 64)) && isoriginalrequested)
	{
		// create the original res frame
		if (fheight == 32)
		{
			pfr = mySerum.frame32;
			mySerum.flags |= FLAG_RETURNED_32P_FRAME_OK;
			prot = mySerum.rotationsinframe32;
			mySerum.width32 = fwidth;
			prt = &colorrotationsn[IDfound * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
			cshft = colorshifts32;
		}
		else
		{
			pfr = mySerum.frame64;
			mySerum.flags |= FLAG_RETURNED_64P_FRAME_OK;
			prot = mySerum.rotationsinframe64;
			mySerum.width64 = fwidthx;
			prt = &colorrotationsn[IDfound * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
			cshft = colorshifts64;
		}
		UINT16* protorg = NULL, * protxtra = NULL;
		for (tj = 0; tj < fheight; tj++)
		{
			for (ti = 0; ti < fwidth; ti++)
			{
				UINT16 tk = tj * fwidth + ti;

				if ((backgroundIDs[IDfound] < nbackgrounds) && (frame[tk] == 0) && (backgroundmask[IDfound * fwidth * fheight + tk] > 0))
				{
					pfr[tk] = backgroundframesn[backgroundIDs[IDfound] * fwidth * fheight + tk];
					if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], false))
						pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (cshft[prot[tk * 2]] + prot[tk * 2 + 1]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
				}
				else
				{
					UINT8 dynacouche = dynamasks[IDfound * fwidth * fheight + tk];
					if (dynacouche == 255)
					{
						pfr[tk] = cframesn[IDfound * fwidth * fheight + tk];
						if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], false))
							pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
					}
					else
					{
						pfr[tk] = dyna4colsn[IDfound * MAX_DYNA_SETS_PER_FRAMEN * nocolors + dynacouche * nocolors + frame[tk]];
						prot[tk * 2] = prot[tk * 2 + 1] = 0xffff;
					}
				}
			}
		}
	}
	if (isextra && ((mySerum.frame32 && fheightx == 32) || (mySerum.frame64 && fheightx == 64)) && isextrarequested)
	{
		// create the extra res frame
		if (fheightx == 32)
		{
			pfr = mySerum.frame32;
			mySerum.flags |= FLAG_RETURNED_32P_FRAME_OK;
			prot = mySerum.rotationsinframe32;
			mySerum.width32 = fwidthx;
			prt = &colorrotationsnx[IDfound * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
			cshft = colorshifts32;
		}
		else
		{
			pfr = mySerum.frame64;
			mySerum.flags |= FLAG_RETURNED_64P_FRAME_OK;
			prot = mySerum.rotationsinframe64;
			mySerum.width64 = fwidthx;
			prt = &colorrotationsnx[IDfound * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
			cshft = colorshifts64;
		}
		for (tj = 0; tj < fheightx;tj++)
		{
			for (ti = 0; ti < fwidthx; ti++)
			{
				UINT16 tk = tj * fwidthx + ti;
				UINT16 tl;
				if (fheightx == 64) tl = tj / 2 * fwidth + ti / 2;
				else tl = tj * 2 * fwidth + ti * 2;

				if ((backgroundIDs[IDfound] < nbackgrounds) && (frame[tl] == 0) && (backgroundmaskx[IDfound * fwidthx * fheightx + tk] > 0))
				{
					pfr[tk] = backgroundframesnx[backgroundIDs[IDfound] * fwidthx * fheightx + tk];
					if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], true))
					{
						pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
					}
				}
				else
				{
					UINT8 dynacouche = dynamasksx[IDfound * fwidthx * fheightx + tk];
					if (dynacouche == 255)
					{
						pfr[tk] = cframesnx[IDfound * fwidthx * fheightx + tk];
						if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], true))
						{
							pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
						}
					}
					else
					{
						pfr[tk] = dyna4colsnx[IDfound * MAX_DYNA_SETS_PER_FRAMEN * nocolors + dynacouche * nocolors + frame[tl]];
						prot[tk * 2] = prot[tk * 2 + 1] = 0xffff;
					}
				}
			}
		}
	}
}

void Colorize_Spritev1(UINT8 nosprite, UINT16 frx, UINT16 fry, UINT16 spx, UINT16 spy, UINT16 wid, UINT16 hei)
{
	for (UINT16 tj = 0; tj < hei; tj++)
	{
		for (UINT16 ti = 0; ti < wid; ti++)
		{
			if (spritedescriptionso[(nosprite * MAX_SPRITE_SIZE + tj + spy) * MAX_SPRITE_SIZE + ti + spx] < 255)
			{
				mySerum.frame[(fry + tj) * fwidth + frx + ti] = spritedescriptionsc[(nosprite * MAX_SPRITE_SIZE + tj + spy) * MAX_SPRITE_SIZE + ti + spx];
			}
		}
	}
}

void Colorize_Spritev2(UINT8 nosprite, UINT16 frx, UINT16 fry, UINT16 spx, UINT16 spy, UINT16 wid, UINT16 hei, int IDfound)
{
	UINT16* pfr, * prot;
	UINT16* prt;
	UINT* cshft;
	if (((mySerum.flags & FLAG_RETURNED_32P_FRAME_OK) && fheight == 32) || ((mySerum.flags & FLAG_RETURNED_64P_FRAME_OK) && fheight == 64))
	{
		if (fheight == 32)
		{
			pfr = mySerum.frame32;
			prot = mySerum.rotationsinframe32;
			prt = &colorrotationsn[IDfound * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
			cshft = colorshifts32;
		}
		else
		{
			pfr = mySerum.frame64;
			prot = mySerum.rotationsinframe64;
			prt = &colorrotationsn[IDfound * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
			cshft = colorshifts64;
		}
		for (UINT16 tj = 0; tj < hei; tj++)
		{
			for (UINT16 ti = 0; ti < wid; ti++)
			{
				UINT16 tk = (fry + tj) * fwidth + frx + ti;
				if (spriteoriginal[(nosprite * MAX_SPRITE_HEIGHT + tj + spy) * MAX_SPRITE_WIDTH + ti + spx] < 255)
				{
					pfr[(fry + tj) * fwidth + frx + ti] = spritecolored[(nosprite * MAX_SPRITE_HEIGHT + tj + spy) * MAX_SPRITE_WIDTH + ti + spx];
					if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], false))
						pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
				}
			}
		}
	}
	if (((mySerum.flags & FLAG_RETURNED_32P_FRAME_OK) && fheightx == 32) || ((mySerum.flags & FLAG_RETURNED_64P_FRAME_OK) && fheightx == 64))
	{
		UINT16 thei, twid, tfrx, tfry, tspy, tspx;
		if (fheightx == 32)
		{
			pfr = mySerum.frame32;
			prot = mySerum.rotationsinframe32;
			thei = hei / 2;
			twid = wid / 2;
			tfrx = frx / 2;
			tfry = fry / 2;
			tspx = spx / 2;
			tspy = spy / 2;
			prt = &colorrotationsnx[IDfound * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
			cshft = colorshifts32;
		}
		else
		{
			pfr = mySerum.frame64;
			prot = mySerum.rotationsinframe64;
			thei = hei * 2;
			twid = wid * 2;
			tfrx = frx * 2;
			tfry = fry * 2;
			tspx = spx * 2;
			tspy = spy * 2;
			prt = &colorrotationsnx[IDfound * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION];
			cshft = colorshifts64;
		}
		for (UINT16 tj = 0; tj < thei; tj++)
		{
			for (UINT16 ti = 0; ti < twid; ti++)
			{
				UINT16 tk = (tfry + tj) * fwidthx + tfrx + ti;
				if (spritemaskx[(nosprite * MAX_SPRITE_HEIGHT + tj + tspy) * MAX_SPRITE_WIDTH + ti + tspx] < 255)
				{
					pfr[tk] = spritecoloredx[(nosprite * MAX_SPRITE_HEIGHT + tj + tspy) * MAX_SPRITE_WIDTH + ti + tspx];
					if (ColorInRotation(IDfound, pfr[tk], &prot[tk * 2], &prot[tk * 2 + 1], true))
						pfr[tk] = prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION + 2 + (prot[tk * 2 + 1] + cshft[prot[tk * 2]]) % prt[prot[tk * 2] * MAX_LENGTH_COLOR_ROTATION]];
				}
			}
		}
	}
}

void Copy_Frame_Palette(int nofr)
{
	memcpy(mySerum.palette, &cpal[nofr * 64 * 3], 64 * 3);
}

SERUM_API void Serum_SetIgnoreUnknownFramesTimeout(UINT16 milliseconds)
{
	ignoreUnknownFramesTimeout = milliseconds;
}

SERUM_API void Serum_SetMaximumUnknownFramesToSkip(UINT8 maximum)
{
	maxFramesToSkip = maximum;
}

SERUM_API void Serum_SetStandardPalette(const UINT8* palette, const int bitDepth)
{
	int palette_length = (1 << bitDepth) * 3;
	assert(palette_length < PALETTE_SIZE);

	if (palette_length <= PALETTE_SIZE)
	{
		memcpy(standardPalette, palette, palette_length);
		standardPaletteLength = palette_length;
	}
}

UINT Serum_ColorizeWithMetadatav1(UINT8* frame)
{
	// return 0xffffffff if no new frame detected
	// return 0 if new frame with no rotation detected
	// return > 0 if new frame with rotations detected, the value is the delay before the first rotation in ms 
	mySerum.triggerID = 0xffffffff;

	if (!enabled)
	{
		// apply standard palette
		memcpy(mySerum.palette, standardPalette, standardPaletteLength);
		return 0;
	}

	// Let's first identify the incoming frame among the ones we have in the crom
	int frameID = Identify_Frame(frame);
	UINT8 nosprite[MAX_SPRITES_PER_FRAME], nspr;
	UINT16 frx[MAX_SPRITES_PER_FRAME], fry[MAX_SPRITES_PER_FRAME], spx[MAX_SPRITES_PER_FRAME], spy[MAX_SPRITES_PER_FRAME], wid[MAX_SPRITES_PER_FRAME], hei[MAX_SPRITES_PER_FRAME];
	memset(nosprite, 255, MAX_SPRITES_PER_FRAME);

	if (frameID != IDENTIFY_NO_FRAME)
	{
		if (frameID!= IDENTIFY_SAME_FRAME) mySerum.frameID = frameID;
		lastframe_found = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (maxFramesToSkip)
		{
			framesSkippedCounter = 0;
		}
		bool isspr = Check_Sprites(frame, lastfound, nosprite, &nspr, frx, fry, spx, spy, wid, hei);
		if (((frameID >= 0) || isspr) && activeframes[lastfound] != 0)
		{
			Colorize_Framev1(frame, lastfound);
			Copy_Frame_Palette(lastfound);
			UINT ti = 0;
			while (ti < nspr)
			{
				Colorize_Spritev1(nosprite[ti], frx[ti], fry[ti], spx[ti], spy[ti], wid[ti], hei[ti]);
				lastsprite[ti] = nosprite[ti];
				lastnsprites = nspr;
				lastfrx[ti] = frx[ti];
				lastfry[ti] = fry[ti];
				lastspx[ti] = spx[ti];
				lastspy[ti] = spy[ti];
				lastwid[ti] = wid[ti];
				lasthei[ti] = hei[ti];
				ti++;
			}
			memcpy(lastframe, frame, fwidth * fheight);
			memcpy(lastpalette, mySerum.palette, 64 * 3);
			memcpy(mySerum.rotations, &colorrotations[lastfound * 3 * MAX_COLOR_ROTATIONS], MAX_COLOR_ROTATIONS * 3);
			memcpy(lastrotations, &colorrotations[lastfound * 3 * MAX_COLOR_ROTATIONS], MAX_COLOR_ROTATIONS * 3);
			UINT now = lastframe_found;
			for (UINT ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
			{
				if (mySerum.rotations[ti * 3] == 255)
				{
					colorrotnexttime[ti] = 0;
					continue;
				}
				// reset the timer if the previous frame had this rotation inactive
				// or if the last init time is more than a new rotation away
				if (colorrotnexttime[ti] == 0 || (colorshiftinittime[ti] + mySerum.rotations[ti * 3 + 2] * 10) <= now) colorshiftinittime[ti] = now;
				if (mySerum.rotationtimer == 0)
				{
					mySerum.rotationtimer = colorshiftinittime[ti] + mySerum.rotations[ti * 3 + 2] * 10 - now;
					colorrotnexttime[ti] = colorshiftinittime[ti] + mySerum.rotations[ti * 3 + 2] * 10;
					continue;
				}
				else if ((colorshiftinittime[ti] + mySerum.rotations[ti * 3 + 2] * 10 - now) < mySerum.rotationtimer) mySerum.rotationtimer = colorshiftinittime[ti] + mySerum.rotations[ti * 3 + 2] * 10 - now;
				if (mySerum.rotationtimer <= 0) mySerum.rotationtimer = 10;
			}
			if (triggerIDs[lastfound] != lasttriggerID) lasttriggerID = mySerum.triggerID = triggerIDs[lastfound];
			return (int)mySerum.rotationtimer;  // new frame
		}
	}

	UINT now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if ((ignoreUnknownFramesTimeout && (now - lastframe_found) >= ignoreUnknownFramesTimeout) || (maxFramesToSkip && (frameID == IDENTIFY_NO_FRAME) && (++framesSkippedCounter >= maxFramesToSkip)))
	{
		// apply standard palette
		memcpy(mySerum.palette, standardPalette, standardPaletteLength);
		// disable render features like rotations
		for (UINT ti = 0; ti < MAX_COLOR_ROTATIONS * 3; ti++)
		{
			lastrotations[ti] = mySerum.rotations[ti] = 255;
		}
		for (UINT ti = 0; ti < lastnsprites; ti++)
		{
			lastsprite[ti] = nosprite[ti];
		}

		return 0;  // new but not colorized frame, return true
	}

	// values for the renderer
	mySerum.frameID = 0xffff;
	memcpy(frame, lastframe, fwidth * fheight);
	memcpy(mySerum.rotations, lastrotations, 3 * MAX_COLOR_ROTATIONS);
	nspr = lastnsprites;
	for (UINT ti = 0; ti < lastnsprites; ti++)
	{
		nosprite[ti] = lastsprite[ti];
		frx[ti] = lastfrx[ti];
		fry[ti] = lastfry[ti];
		spx[ti] = lastspx[ti];
		spy[ti] = lastspy[ti];
		wid[ti] = lastwid[ti];
		hei[ti] = lasthei[ti];
	}
	mySerum.rotationtimer = 0xffffffff;
	return 0xffffffff;  // no new frame, return false, client has to update rotations!
}

SERUM_API UINT Serum_ColorizeWithMetadatav2(UINT8* frame)
{
	// return 0xffffffff if no new frame detected
	// return 0 if new frame with no rotation detected
	// return > 0 if new frame with rotations detected, the value is the delay before the first rotation in ms 
	mySerum.triggerID = 0xffffffff;

	// Let's first identify the incoming frame among the ones we have in the crom
	int frameID = Identify_Frame(frame);
	UINT8 nosprite[MAX_SPRITES_PER_FRAME], nspr;
	UINT16 frx[MAX_SPRITES_PER_FRAME], fry[MAX_SPRITES_PER_FRAME], spx[MAX_SPRITES_PER_FRAME], spy[MAX_SPRITES_PER_FRAME], wid[MAX_SPRITES_PER_FRAME], hei[MAX_SPRITES_PER_FRAME];
	memset(nosprite, 255, MAX_SPRITES_PER_FRAME);

	if (frameID != IDENTIFY_NO_FRAME)
	{
		if (frameID != IDENTIFY_SAME_FRAME) mySerum.frameID = frameID;
		// frame identified
		lastframe_found = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (maxFramesToSkip)
		{
			framesSkippedCounter = 0;
		}

		bool isspr = Check_Sprites(frame, lastfound, nosprite, &nspr, frx, fry, spx, spy, wid, hei);
		if (((frameID >= 0) || isspr) && activeframes[lastfound] != 0)
		{
			// the frame identified is not the same as the preceding
			Colorize_Framev2(frame, lastfound);
			UINT ti = 0;
			while (ti < nspr)
			{
				Colorize_Spritev2(nosprite[ti], frx[ti], fry[ti], spx[ti], spy[ti], wid[ti], hei[ti], lastfound);
				lastsprite[ti] = nosprite[ti];
				lastnsprites = nspr;
				lastfrx[ti] = frx[ti];
				lastfry[ti] = fry[ti];
				lastspx[ti] = spx[ti];
				lastspy[ti] = spy[ti];
				lastwid[ti] = wid[ti];
				lasthei[ti] = hei[ti];
				ti++;
			}
			if (mySerum.frame32)
			{
				memcpy(lastframe32, mySerum.frame32, min(fwidth, fwidthx) * 32 * 2);
				memcpy(lastrotationsinframe32, mySerum.rotationsinframe32, min(fwidth, fwidthx) * 32 * 2 * 2);
			}
			if (mySerum.frame64)
			{
				memcpy(lastframe64, mySerum.frame64, max(fwidth, fwidthx) * 64 * 2);
				memcpy(lastrotationsinframe64, mySerum.rotationsinframe64, max(fwidth, fwidthx) * 64 * 2 * 2);
			}
			UINT16* pcr32, * pcr64;
			if (fheight == 32)
			{
				pcr32 = colorrotationsn;
				pcr64 = colorrotationsnx;
			}
			else
			{
				pcr32 = colorrotationsnx;
				pcr64 = colorrotationsn;
			}
			UINT now = lastframe_found;
			if (mySerum.frame32)
			{
				memcpy(mySerum.rotations32, &pcr32[lastfound * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN], MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
				memcpy(lastrotations32, mySerum.rotations32, MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
				for (UINT ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
				{
					if (mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION] == 0)
					{
						colorrotnexttime32[ti] = 0;
						continue;
					}
					// reset the timer if the previous frame had this rotation inactive
					// or if the last init time is more than a new rotation away
					if (colorrotnexttime32[ti] == 0 || (colorshiftinittime32[ti] + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1]) <= now) colorshiftinittime32[ti] = now;
					if (mySerum.rotationtimer == 0)
					{
						mySerum.rotationtimer = colorshiftinittime32[ti] + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now;
						colorrotnexttime32[ti] = colorshiftinittime32[ti] + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1];
						continue;
					}
					else if ((colorshiftinittime32[ti] + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now) < mySerum.rotationtimer) mySerum.rotationtimer = colorshiftinittime32[ti] + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now;
					if (mySerum.rotationtimer <= 0) mySerum.rotationtimer = 10;
				}
			}
			if (mySerum.frame64)
			{
				memcpy(mySerum.rotations64, &pcr64[lastfound * MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN], MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
				memcpy(lastrotations64, mySerum.rotations64, MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION * 2);
				for (UINT ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
				{
					if (mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION] == 0)
					{
						colorrotnexttime64[ti] = 0;
						continue;
					}
					// reset the timer if the previous frame had this rotation inactive
					// or if the last init time is more than a new rotation away
					if (colorrotnexttime64[ti] == 0 || (colorshiftinittime64[ti] + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1]) <= now) colorshiftinittime64[ti] = now;
					if (mySerum.rotationtimer == 0)
					{
						mySerum.rotationtimer = colorshiftinittime64[ti] + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now;
						colorrotnexttime64[ti] = colorshiftinittime64[ti] + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1];
						continue;
					}
					else if ((colorshiftinittime64[ti] + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now) < mySerum.rotationtimer) mySerum.rotationtimer = colorshiftinittime64[ti] + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1] - now;
					if (mySerum.rotationtimer <= 0) mySerum.rotationtimer = 10;
				}
			}
			if (triggerIDs[lastfound] != lasttriggerID) lasttriggerID = mySerum.triggerID = triggerIDs[lastfound];
			return (UINT)mySerum.rotationtimer;  // new frame, return true
		}
	}
	

	// no frame identified from the inbound frame or the frame identified is the same with the same sprites
	// we resent the previous one
	mySerum.frameID = 0xffff;
	if (mySerum.frame32)
	{
		memcpy(mySerum.frame32, lastframe32, min(fwidth, fwidthx) * 32 * 2);
		memcpy(mySerum.rotations32, lastrotations32, MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN);
		memcpy(mySerum.rotationsinframe32, lastrotationsinframe32, min(fwidth, fwidthx) * 32 * 2 * 2);
	}
	if (mySerum.frame64)
	{
		memcpy(mySerum.frame64, lastframe64, max(fwidth, fwidthx) * 64 * 2);
		memcpy(mySerum.rotations64, lastrotations64, MAX_LENGTH_COLOR_ROTATION * MAX_COLOR_ROTATIONN);
		memcpy(mySerum.rotationsinframe64, lastrotationsinframe64, max(fwidth, fwidthx) * 64 * 2 * 2);
	}
	nspr = lastnsprites;
	for (UINT ti = 0; ti < lastnsprites; ti++)
	{
		nosprite[ti] = lastsprite[ti];
		frx[ti] = lastfrx[ti];
		fry[ti] = lastfry[ti];
		spx[ti] = lastspx[ti];
		spy[ti] = lastspy[ti];
		wid[ti] = lastwid[ti];
		hei[ti] = lasthei[ti];
	}

    for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
    {
        if (mySerum.rotationtimer == 0 && mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION] > 0) mySerum.rotationtimer = mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION];
        if (mySerum.rotationtimer == 0 && mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION] > 0) mySerum.rotationtimer = mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION];
        if (mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION] > 0 && mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION] < mySerum.rotationtimer) mySerum.rotationtimer = mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION];
        if (mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION] > 0 && mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION] < mySerum.rotationtimer) mySerum.rotationtimer = mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION];
    }
	mySerum.rotationtimer = 0xffffffff;
	return 0xffffffff;  // no new frame, return false, client has to update rotations!
}

SERUM_API UINT Serum_Colorize(UINT8* frame)
{
	// return 0xffffffff if no new frame detected
	// return 0 if new frame with no rotation detected
	// return > 0 if new frame with rotations detected, the value is the delay before the first rotation in ms 
	mySerum.rotationtimer = 0;
	if (SerumVersion == SERUM_V2) return Serum_ColorizeWithMetadatav2(frame);
	else return Serum_ColorizeWithMetadatav1(frame);
}

UINT Calc_Next_Rotationv1(UINT now)
{
	UINT nextrot = 0xffffffff;
	for (int ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
	{
		if (mySerum.rotations[ti * 3] == 255) continue;
		if (colorrotnexttime[ti] < nextrot) nextrot = colorrotnexttime[ti];
	}
	if (nextrot == 0xffffffff) return 0;
	return nextrot - now;
}

UINT Serum_ApplyRotationsv1(void)
{
	UINT isrotation = 0;
	UINT now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	for (int ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
	{
		if (mySerum.rotations[ti * 3] == 255) continue;
		UINT elapsed = now - colorshiftinittime[ti];
		if (elapsed >= (long)(mySerum.rotations[ti * 3 + 2] * 10))
		{
			colorshifts[ti]++;
			colorshifts[ti] %= mySerum.rotations[ti * 3 + 1];
			colorshiftinittime[ti] = now;
			colorrotnexttime[ti] = now + mySerum.rotations[ti * 3 + 2] * 10;
			isrotation = 0x10000;
			UINT8 palsave[3 * 64];
			memcpy(palsave, &mySerum.palette[mySerum.rotations[ti * 3] * 3], (size_t)mySerum.rotations[ti * 3 + 1] * 3);
			for (int tj = 0; tj < mySerum.rotations[ti * 3 + 1]; tj++)
			{
				UINT shift = (tj + 1) % mySerum.rotations[ti * 3 + 1];
				mySerum.palette[(mySerum.rotations[ti * 3] + tj) * 3] = palsave[shift * 3];
				mySerum.palette[(mySerum.rotations[ti * 3] + tj) * 3 + 1] = palsave[shift * 3 + 1];
				mySerum.palette[(mySerum.rotations[ti * 3] + tj) * 3 + 2] = palsave[shift * 3 + 2];
			}
		}
	}
	mySerum.rotationtimer = (UINT16)Calc_Next_Rotationv1(now); // can't be more than 65s, so val is contained in the lower word of val
	return ((UINT)mySerum.rotationtimer | isrotation); // if there was a rotation, returns the next time in ms to the next one and set high dword to 1
	// if not, just the delay to the next rotation
}

UINT Calc_Next_Rotationv2(UINT now)
{
	UINT nextrot = 0xffffffff;
	for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
	{
		if (mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION] > 0)
		{
			if (colorrotnexttime32[ti] < nextrot) nextrot = colorrotnexttime32[ti];
		}
		if (mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION] > 0)
		{
			if (colorrotnexttime64[ti] < nextrot) nextrot = colorrotnexttime64[ti];
		}
	}
	if (nextrot == 0xffffffff) return 0;
	return nextrot - now;
}

UINT Serum_ApplyRotationsv2(void)
{
	UINT isrotation = 0;
	int sizeframe;
	UINT now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if (mySerum.frame32)
	{
		sizeframe = 32 * mySerum.width32;
		if (mySerum.modifiedelements32) memset(mySerum.modifiedelements32, 0, sizeframe);
		for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
		{
			if (mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION] == 0) continue;
			UINT elapsed = now - colorshiftinittime32[ti];
			if (elapsed >= (long)(mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1]))
			{
				colorshifts32[ti]++;
				colorshifts32[ti] %= mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION];
				colorshiftinittime32[ti] = now;
				colorrotnexttime32[ti] = now + mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 1];
				isrotation |= 0x10000;
				for (UINT tj = 0; tj < sizeframe; tj++)
				{
					if (mySerum.rotationsinframe32[tj * 2] == ti)
					{
						// if we have a pixel which is part of this rotation, we modify it
						mySerum.frame32[tj] = mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION + 2 + (mySerum.rotationsinframe32[tj * 2 + 1] + colorshifts32[ti]) % mySerum.rotations32[ti * MAX_LENGTH_COLOR_ROTATION]];
						if (mySerum.modifiedelements32) mySerum.modifiedelements32[tj] = 1;
					}
				}
			}
		}
	}
	if (mySerum.frame64)
	{
		sizeframe = 64 * mySerum.width64;
		if (mySerum.modifiedelements64) memset(mySerum.modifiedelements64, 0, sizeframe);
		for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
		{
			if (mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION] == 0) continue;
			UINT elapsed = now - colorshiftinittime64[ti];
			if (elapsed >= (long)(mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1]))
			{
				colorshifts64[ti]++;
				colorshifts64[ti] %= mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION];
				colorshiftinittime64[ti] = now;
				colorrotnexttime64[ti] = now + mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 1];
				isrotation |= 0x20000;
				for (UINT tj = 0; tj < sizeframe; tj++)
				{
					if (mySerum.rotationsinframe64[tj * 2] == ti)
					{
						// if we have a pixel which is part of this rotation, we modify it
						mySerum.frame64[tj] = mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION + 2 + (mySerum.rotationsinframe64[tj * 2 + 1] + colorshifts64[ti]) % mySerum.rotations64[ti * MAX_LENGTH_COLOR_ROTATION]];
						if (mySerum.modifiedelements64) mySerum.modifiedelements64[tj] = 1;
					}
				}
			}
		}
	}
	mySerum.rotationtimer = (UINT16)Calc_Next_Rotationv2(now); // can't be more than 65s, so val is contained in the lower word of val
	return ((UINT)mySerum.rotationtimer | isrotation); // returns the time in ms untill the next rotation in the lowest word
	// if there was a rotation in the 32P frame, the first bit of the high word is set (0x10000)
	// and if there was a rotation in the 64P frame, the second bit of the high word is set (0x20000)
}

SERUM_API UINT Serum_Rotate(void)
{
	if (SerumVersion == SERUM_V2)
	{
		return Serum_ApplyRotationsv2();
	}
	else
	{
		return Serum_ApplyRotationsv1();
	}
	return 0;
}

/*SERUM_API bool Serum_ColorizeWithMetadataOrApplyRotations(UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations, UINT* triggerID, UINT* hashcode, int* frameID)
{
  bool new_frame = Serum_ColorizeWithMetadata(frame, width, height, palette, rotations, triggerID, hashcode, frameID);
  if (!new_frame) return Serum_ApplyRotations(palette, rotations);
  return new_frame;
}

SERUM_API bool Serum_ColorizeOrApplyRotations(UINT8* frame, int width, int height, UINT8* palette, UINT* triggerID)
{
  if (frame)
  {
	static UINT8 rotations[24] = {255};
	UINT hashcode;
	int frameID;
	bool new_frame = Serum_ColorizeWithMetadataOrApplyRotations( frame, width, height, palette, rotations, triggerID, &hashcode, &frameID);
	if (new_frame) memset(rotations, 255, 24);
	return new_frame;
  }
  else if (enabled)
  {
	Copy_Frame_Palette(lastfound, palette);
	return Serum_ApplyRotations(palette, lastrotations);
  }
  return false;
}*/

SERUM_API void Serum_DisableColorization()
{
	enabled = false;
}

SERUM_API void Serum_EnableColorization()
{
	enabled = true;
}
