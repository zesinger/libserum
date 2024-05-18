#pragma once

#ifdef _MSC_VER
#define SERUM_API extern "C" __declspec(dllexport)
#else
#define SERUM_API extern "C" __attribute__((visibility("default")))
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include "serum.h"

SERUM_API Serum_Frame_Struc* Serum_Load(const char* const altcolorpath, const char* const romname, uint8_t flags);
SERUM_API void Serum_SetIgnoreUnknownFramesTimeout(uint16_t milliseconds);
SERUM_API void Serum_SetMaximumUnknownFramesToSkip(uint8_t maximum);
SERUM_API void Serum_SetStandardPalette(const uint8_t* palette, const int bitDepth);
SERUM_API void Serum_Dispose(void);
SERUM_API uint32_t Serum_Colorize(uint8_t* frame);
SERUM_API uint32_t Serum_Rotate(void);
SERUM_API void Serum_DisableColorization(void);
SERUM_API void Serum_EnableColorization(void);
SERUM_API const char* Serum_GetVersion(void);
SERUM_API const char* Serum_GetMinorVersion(void);

