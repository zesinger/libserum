#pragma once

#ifdef _MSC_VER
#define SERUM_API extern "C" __declspec(dllexport)
#else
#define SERUM_API extern "C" __attribute__((visibility("default")))
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include "serum.h"

SERUM_API Serum_Frame_Struc* Serum_Load(const char* const altcolorpath, const char* const romname, UINT8 flags);
SERUM_API void Serum_SetIgnoreUnknownFramesTimeout(UINT16 milliseconds);
SERUM_API void Serum_SetMaximumUnknownFramesToSkip(UINT8 maximum);
SERUM_API void Serum_SetStandardPalette(const UINT8* palette, const int bitDepth);
SERUM_API void Serum_Dispose(void);
SERUM_API UINT Serum_Colorize(UINT8* frame);
SERUM_API UINT Serum_Rotate(void);
SERUM_API void Serum_DisableColorization(void);
SERUM_API void Serum_EnableColorization(void);
SERUM_API const char* Serum_GetVersion(void);
SERUM_API const char* Serum_GetMinorVersion(void);

