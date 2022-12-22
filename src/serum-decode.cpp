#include "serum-decode.h"
#include "miniz/zip_file.hpp"
#include <stdio.h>
#include <stdlib.h>

#pragma warning(disable: 4996)

// header
char rname[64];
UINT32 fwidth, fheight;
UINT32 nframes;
UINT32 nocolors, nccolors;
UINT32 ncompmasks, nmovmasks;
UINT32 nsprites;
// data
UINT32* hashcodes = NULL;
UINT8* shapecompmode = NULL;
UINT8* compmaskID = NULL;
UINT8* movrctID = NULL;
UINT8* compmasks = NULL;
UINT8* movrcts = NULL;
UINT8* cpal = NULL;
UINT8* cframes = NULL;
UINT8* dynamasks = NULL;
UINT8* dyna4cols = NULL;
UINT8* framesprites = NULL;
UINT8* spritedescriptionso = NULL;
UINT8* spritedescriptionsc = NULL;
UINT8* activeframes = NULL;
UINT8* colorrotations = NULL;
UINT16* spritedetareas = NULL;
UINT32* spritedetdwords = NULL;
UINT16* spritedetdwordpos = NULL;

// variables
bool cromloaded = false; // is there a crom loaded?
UINT32 lastfound = 0; // last frame ID identified
UINT8* lastframe = NULL; // last frame content identified
UINT8* lastpalette = NULL; // last palette identified
UINT8* lastrotations = NULL; // last colour rotations identified
UINT8 lastsprite; // last sprite identified
UINT16 lastfrx, lastfry; // last position in the frame of the sprite
UINT16 lastspx, lastspy; // last top left of the sprite to display
UINT16 lastwid, lasthei; // last dimensions of the sprite to display
bool isrotation = true; // are there rotations to send
bool crc32_ready = false; // is the crc32 table filled?
UINT32 crc32_table[256]; // initial table
bool* framechecked = NULL; // are these frames checked?


void Serum_free(void)
{
    if (hashcodes)
    {
        free(hashcodes);
        hashcodes = NULL;
    }
    if (shapecompmode)
    {
        free(shapecompmode);
        shapecompmode = NULL;
    }
    if (compmaskID)
    {
        free(compmaskID);
        compmaskID = NULL;
    }
    if (movrctID)
    {
        free(movrctID);
        movrctID = NULL;
    }
    if (compmasks)
    {
        free(compmasks);
        compmasks = NULL;
    }
    if (movrcts)
    {
        free(movrcts);
        movrcts = NULL;
    }
    if (cpal)
    {
        free(cpal);
        cpal = NULL;
    }
    if (cframes)
    {
        free(cframes);
        cframes = NULL;
    }
    if (dynamasks)
    {
        free(dynamasks);
        dynamasks = NULL;
    }
    if (dyna4cols)
    {
        free(dyna4cols);
        dyna4cols = NULL;
    }
    if (framesprites)
    {
        free(framesprites);
        framesprites = NULL;
    }
    if (spritedescriptionso)
    {
        free(spritedescriptionso);
        spritedescriptionso = NULL;
    }
    if (spritedescriptionsc)
    {
        free(spritedescriptionsc);
        spritedescriptionsc = NULL;
    }
    if (spritedetdwords)
    {
        free(spritedetdwords);
        spritedetdwords = NULL;
    }
    if (spritedetdwordpos)
    {
        free(spritedetdwordpos);
        spritedetdwordpos = NULL;
    }
    if (spritedetareas)
    {
        free(spritedetareas);
        spritedetareas = NULL;
    }
    if (activeframes)
    {
        free(activeframes);
        activeframes = NULL;
    }
    if (colorrotations)
    {
        free(colorrotations);
        colorrotations = NULL;
    }
    if (lastframe)
    {
        free(lastframe);
        lastframe = NULL;
    }
    if (lastpalette)
    {
        free(lastpalette);
        lastpalette = NULL;
    }
    if (lastrotations)
    {
        free(lastrotations);
        lastrotations = NULL;
    }
    if (framechecked)
    {
        free(framechecked);
        framechecked = NULL;
    }
    cromloaded = false;
}

void CRC32encode(void) // initiating the CRC table, must be called at startup
{
    for (int i = 0; i < 256; i++)
    {
        UINT32 ch = i;
        UINT32 crc = 0;
        for (int j = 0; j < 8; j++)
        {
            UINT32 b = (ch ^ crc) & 1;
            crc >>= 1;
            if (b != 0) crc = crc ^ 0xEDB88320;
            ch >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_ready = true;
}

UINT32 crc32_fast(UINT8* s, UINT n, UINT8 ShapeMode) // computing a buffer CRC32, "init_crc32()" must have been called before the first use
{

    UINT32 crc = 0xFFFFFFFF;
    for (int i = 0; i < (int)n; i++)
    {
        UINT8 val = s[i];
        if ((ShapeMode == 1) && (val > 1))  val = 1;
        crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
    }
    return ~crc;
}

UINT32 crc32_fast_mask(UINT8* source, UINT8* mask, UINT n, UINT8 ShapeMode) // computing a buffer CRC32 on the non-masked area, "init_crc32()" must have been called before the first use
// take into account if we are in shape mode
{
    UINT32 crc = 0xFFFFFFFF;
    for (int i = 0; i < (int)n; i++)
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

SERUM_API(bool) Serum_Load(const char* altcolorpath, const char* romname)
{
    if (!crc32_ready) CRC32encode();

    char tbuf[260], tbuf2[260];
    strcpy(tbuf, altcolorpath);
    if ((tbuf[strlen(tbuf) - 1] != '\\') && (tbuf[strlen(tbuf) - 1] != '/')) strcat(tbuf, "/");
    strcat(tbuf, romname);
    strcat(tbuf, "/");
    strcpy(tbuf2, tbuf);
    strcat(tbuf, romname);
    strcat(tbuf, ".cRZ");
    // Use Thomas Fussell https://github.com/tfussell/miniz-cpp to uncompress cRZ to cRom
    miniz_cpp::zip_file file(tbuf);
    file.extractall(tbuf2);
    file.~zip_file();
    strcat(tbuf2, romname);
    strcat(tbuf2, ".cRom");
    // Open cRom
    FILE* pfile;
    pfile = fopen(tbuf2, "rb");
    if (!pfile)
    {
        return false;
    }
    // read the header to know how much memory is needed
    fread(rname, 1, 64, pfile);
    UINT32 sizeheader;
    fread(&sizeheader, 4, 1, pfile);
    fread(&fwidth, 4, 1, pfile);
    fread(&fheight, 4, 1, pfile);
    fread(&nframes, 4, 1, pfile);
    fread(&nocolors, 4, 1, pfile);
    fread(&nccolors, 4, 1, pfile);
    if ((fwidth == 0) || (fheight == 0) || (nframes == 0) || (nocolors == 0) || (nccolors == 0))
    {
        // incorrect file format
        fclose(pfile);
        return false;
    }
    fread(&ncompmasks, 4, 1, pfile);
    fread(&nmovmasks, 4, 1, pfile);
    fread(&nsprites, 4, 1, pfile);
    // allocate memory for the serum format
    hashcodes = (UINT32*)malloc(sizeof(UINT32) * nframes);
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
    spritedescriptionso = (UINT8*)malloc(nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
    spritedescriptionsc = (UINT8*)malloc(nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
    activeframes = (UINT8*)malloc(nframes);
    colorrotations = (UINT8*)malloc(nframes * 3 * MAX_COLOR_ROTATIONS);
    spritedetdwords = (UINT32*)malloc(nsprites * sizeof(UINT32) * MAX_SPRITE_DETECT_AREAS);
    spritedetdwordpos = (UINT16*)malloc(nsprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS);
    spritedetareas = (UINT16*)malloc(nsprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS * 4);
    if ((!hashcodes) || (!shapecompmode) || (!compmaskID) || (!movrctID) || (!cpal) || (!cframes) || (!dynamasks) ||
        (!dyna4cols) || (!framesprites) || (!activeframes) || (!colorrotations))
    {
        Serum_free();
        fclose(pfile);
        return false;
    }
    if ((ncompmasks > 0) && (!compmasks))
    {
        Serum_free();
        fclose(pfile);
        return false;
    }
    if ((nmovmasks > 0) && (!movrcts))
    {
        Serum_free();
        fclose(pfile);
        return false;
    }
    if ((nsprites > 0) && ((!spritedescriptionso) || (!spritedescriptionsc) || (!spritedetdwords) || (!spritedetdwordpos) || (!spritedetareas)))
    {
        Serum_free();
        fclose(pfile);
        return false;
    }
    // read the cRom file
    fread(hashcodes, sizeof(UINT32), nframes, pfile);
    fread(shapecompmode, 1, nframes, pfile);
    fread(compmaskID, 1, nframes, pfile);
    fread(movrctID, 1, nframes, pfile);
    fread(compmasks, 1, ncompmasks * fwidth * fheight, pfile);
    fread(movrcts, 1, nmovmasks * fwidth * fheight, pfile);
    fread(cpal, 1, nframes * 3 * nccolors, pfile);
    fread(cframes, 1, nframes * fwidth * fheight, pfile);
    fread(dynamasks, 1, nframes * fwidth * fheight, pfile);
    fread(dyna4cols, 1, nframes * MAX_DYNA_4COLS_PER_FRAME * nocolors, pfile);
    fread(framesprites, 1, nframes * MAX_SPRITES_PER_FRAME, pfile);
    for (int ti = 0; ti < (int)nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE; ti++)
    {
        fread(&spritedescriptionsc[ti], 1, 1, pfile);
        fread(&spritedescriptionso[ti], 1, 1, pfile);
    }
    fread(activeframes, 1, nframes, pfile);
    fread(colorrotations, 1, nframes * 3 * MAX_COLOR_ROTATIONS, pfile);
    fread(spritedetdwords, sizeof(UINT32), nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
    fread(spritedetdwordpos, sizeof(UINT16), nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
    fread(spritedetareas, sizeof(UINT16), nsprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile);
    fclose(pfile);
    // allocate memory for previous detected frame
    lastframe = (UINT8*)malloc(fwidth * fheight);
    lastpalette = (UINT8*)malloc(nccolors * 3);
    lastrotations = (UINT8*)malloc(3 * MAX_COLOR_ROTATIONS);
    framechecked = (bool*)malloc(sizeof(bool) * nframes);
    if ((!lastframe) || (!lastpalette) || (!lastrotations) || (!framechecked))
    {
        Serum_free();
        return false;
    }
    remove(tbuf2);
    cromloaded = true;
    return true;
}

SERUM_API(void) Serum_Dispose(void)
{
    Serum_free();
}

int Identify_Frame(UINT8* frame)
{
    // check if the generated frame is the same as one we have in the crom (
    if (!cromloaded) return -1;
    UINT8* pmask;
    memset(framechecked, false, nframes);
    UINT32 tj = lastfound; // we start from the frame we last found
    UINT8 mask = 255;
    UINT8 Shape = 0;
    do
    {
        // calculate the hashcode for the generated frame with the mask and shapemode of the current crom frame
        mask = compmaskID[tj];
        Shape = shapecompmode[tj];
        UINT32 Hashc;
        if (mask < 255)
        {
            pmask = &compmasks[mask * fwidth * fheight];
            Hashc = crc32_fast_mask(frame, pmask, fwidth * fheight, Shape);
        }
        else Hashc = crc32_fast(frame, fwidth * fheight, Shape);
        // now we can compare with all the crom frames that share these same mask and shapemode
        for (int ti = (int)tj; ti < (int)nframes; ti++)
        {
            if (framechecked[ti]) continue;
            if ((compmaskID[ti] == mask) && (shapecompmode[ti] == Shape))
            {
                if (Hashc == hashcodes[ti])
                {
                    lastfound = (UINT)ti;
                    return ti; // we found the frame, we return it
                }
                framechecked[ti] = true;
            }
        }
        for (int ti = 0; ti < (int)tj; ti++)
        {
            if (framechecked[ti]) continue;
            if ((compmaskID[ti] == mask) && (shapecompmode[ti] == Shape))
            {
                if (Hashc == hashcodes[ti])
                {
                    lastfound = (UINT)ti;
                    return ti; // we found the frame, we return it
                }
                framechecked[ti] = true;
            }
        }
        tj++;
        if (tj == nframes) tj = 0;
        while ((tj != lastfound) && (framechecked[tj] == true))
        {
            tj++;
            if (tj == nframes) tj = 0;
        }
    } while (tj != lastfound);
    return -1;
}

void Check_Sprites(UINT8* Frame, int quelleframe, UINT8* pquelsprite, UINT16* pfrx, UINT16* pfry, UINT16* pspx, UINT16* pspy, UINT16* pwid, UINT16* phei)
{
    UINT8 ti = 0;
    UINT32 mdword;
    while ((ti < MAX_SPRITES_PER_FRAME) && (framesprites[quelleframe * MAX_SPRITES_PER_FRAME + ti] < 255))
    {
        UINT8 qspr = framesprites[quelleframe * MAX_SPRITES_PER_FRAME + ti];
        for (UINT32 tm = 0; tm < MAX_SPRITE_DETECT_AREAS; tm++)
        {
            if (spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4] == 0xffff) continue;
            // we look for the sprite in the frame sent
            mdword = (UINT32)(Frame[0] << 8) | (UINT32)(Frame[1] << 16) | (UINT32)(Frame[2] << 24);
            for (UINT16 tj = 0; tj < fwidth * fheight - 3; tj++)
            {
                mdword = (mdword >> 8) | (UINT32)(Frame[tj + 3] << 24);
                // we look for the magic dword first
                UINT16 sddp = spritedetdwordpos[qspr * MAX_SPRITE_DETECT_AREAS + tm];
                if (mdword == spritedetdwords[qspr * MAX_SPRITE_DETECT_AREAS + tm])
                {
                    short frax = (short)(tj % fwidth);
                    short fray = (short)(tj / fwidth);
                    short sprx = (short)(sddp % MAX_SPRITE_SIZE);
                    short spry = (short)(sddp / MAX_SPRITE_SIZE);
                    short detx = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4];
                    short dety = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 1];
                    short detw = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 2];
                    short deth = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 3];
                    if ((frax < sprx - detx) || (fray < spry - dety)) continue; // if the detection area is outside the frame, continue
                    int offsx = frax - sprx + detx;
                    int offsy = fray - spry + dety;
                    if ((offsx + detw >= (int)fwidth) || (offsy + deth >= (int)fheight)) continue;
                    // we can now check if the sprite is there
                    bool notthere = false;
                    for (UINT16 tk = 0; tk < deth; tk++)
                    {
                        for (UINT16 tl = 0; tl < detw; tl++)
                        {
                            UINT8 val = spritedescriptionso[qspr * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (tk + dety) * MAX_SPRITE_SIZE + tl + detx];
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
                        *pquelsprite = qspr;
                        if (frax < sprx)
                        {
                            *pspx = (UINT16)(sprx - frax);
                            *pfrx = 0;
                            *pwid = MIN((UINT16)fwidth, (UINT16)(MAX_SPRITE_SIZE - *pspx));
                        }
                        else
                        {
                            *pspx = 0;
                            *pfrx = (UINT16)(frax - sprx);
                            *pwid = MIN((UINT16)(fwidth - *pfrx), (UINT16)(MAX_SPRITE_SIZE - *pfrx));
                        }
                        if (fray < spry)
                        {
                            *pspy = (UINT16)(spry - fray);
                            *pfry = 0;
                            *phei = MIN((UINT16)fheight, (UINT16)(MAX_SPRITE_SIZE - *pspy));
                        }
                        else
                        {
                            *pspy = 0;
                            *pfry = (UINT16)(fray - spry);
                            *phei = MIN((UINT16)(fheight - *pfry), (UINT16)(MAX_SPRITE_SIZE - *pfry));
                        }
                        return;
                    }
                }
            }
        }
        ti++;
    }
    *pquelsprite = 255;
    return;
}

void Colorize_Frame(UINT8* frame, int IDfound)
{
    UINT32 ti;
    // Generate the colorized version of a frame once identified in the crom frames
    for (ti = 0; ti < fwidth * fheight; ti++)
    {
        UINT8 dynacouche = dynamasks[IDfound * fwidth * fheight + ti];
        if (dynacouche == 255)
            frame[ti] = cframes[IDfound * fwidth * fheight + ti];
        else
            frame[ti] = dyna4cols[IDfound * MAX_DYNA_4COLS_PER_FRAME * nocolors + dynacouche * nocolors + frame[ti]];
    }
}

void Colorize_Sprite(UINT8* frame, UINT8 nosprite, UINT16 frx, UINT16 fry, UINT16 spx, UINT16 spy, UINT16 wid, UINT16 hei)
{
    for (UINT tj = 0; tj < hei; tj++)
    {
        for (UINT ti = 0; ti < wid; ti++)
        {
            if (spritedescriptionso[(nosprite * MAX_SPRITE_SIZE + tj + spy) * MAX_SPRITE_SIZE + ti + spx] < 255)
            {
                frame[(fry + tj) * fwidth + frx + ti] = spritedescriptionsc[(nosprite * MAX_SPRITE_SIZE + tj + spy) * MAX_SPRITE_SIZE + ti + spx];
            }
        }
    }
}

void Copy_Frame_Palette(int nofr, UINT8* dpal)
{
    memcpy(dpal, &cpal[nofr * 64 * 3], 64 * 3);
}

SERUM_API(void) Serum_Colorize(UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations)
{
    // Let's first identify the incoming frame among the ones we have in the crom
    int IDfound = Identify_Frame(frame);
    UINT8 nosprite = 255;
    UINT possprite = 0;
    UINT16 frx = 0, fry = 0, spx = 0, spy = 0, wid = 0, hei = 0;
    if ((IDfound == -1) || (activeframes[IDfound] == 0))
    {
        // code for the players
        for (UINT ti = 0; ti < fwidth * fheight; ti++) frame[ti] = lastframe[ti];
        for (UINT ti = 0; ti < 64; ti++) palette[ti] = lastpalette[ti];
        for (UINT ti = 0; ti < 3 * MAX_COLOR_ROTATIONS; ti++) rotations[ti] = lastrotations[ti];
        nosprite = lastsprite;
        frx = lastfrx;
        fry = lastfry;
        spx = lastspx;
        spy = lastspy;
        wid = lastwid;
        hei = lasthei;
    }
    else
    {
        Check_Sprites(frame, IDfound, &nosprite, &frx, &fry, &spx, &spy, &wid, &hei);
        Colorize_Frame(frame, IDfound);
        Copy_Frame_Palette(IDfound, palette);
        if (nosprite < 255)
        {
            Colorize_Sprite(frame, nosprite, frx, fry, spx, spy, wid, hei);
        }
        for (UINT ti = 0; ti < fwidth * fheight; ti++) lastframe[ti] = frame[ti];
        for (UINT ti = 0; ti < 64; ti++) lastpalette[ti] = palette[ti];
        for (UINT ti = 0; ti < MAX_COLOR_ROTATIONS * 3; ti++)
        {
            lastrotations[ti] = rotations[ti] = colorrotations[IDfound * 3 * MAX_COLOR_ROTATIONS + ti];
        }
        lastsprite = nosprite;
        lastfrx = frx;
        lastfry = fry;
        lastspx = spx;
        lastspy = spy;
        lastwid = wid;
        lasthei = hei;
    }
}
