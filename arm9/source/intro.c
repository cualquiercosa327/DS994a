// =====================================================================================
// Copyright (c) 2023-2025 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated
// readme files, with or without modification, are permitted in any medium without
// royalty provided this copyright notice is used and wavemotion-dave is thanked profusely.
//
// The DS994a emulator is offered as-is, without any warranty.
//
// Please see the README.md file as it contains much useful info.
// =====================================================================================
#include <nds.h>
#include <stdio.h>
#include <maxmod9.h>

#include "DS99_utils.h"

#include "soundbank.h"
#include "splash.h"
#include "pdev_bg0.h"

extern u16 vusCptVBL;
extern void irqVBlank(void);

// --------------------------------------------------------------
// Intro Splash Screen
// --------------------------------------------------------------
void intro_logo(void)
{
    bool bOK;

    // Init graphics
    videoSetMode(MODE_0_2D | DISPLAY_BG0_ACTIVE);
    videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
    vramSetBankA(VRAM_A_MAIN_BG); vramSetBankC(VRAM_C_SUB_BG);
    irqSet(IRQ_VBLANK, irqVBlank);
    irqEnable(IRQ_VBLANK);

    // Init main screen background
    int bg1 = bgInit(0, BgType_Text8bpp, BgSize_T_256x256, 31,0);

    // Init sub (lower) screen background
    int bg1s = bgInitSub(0, BgType_Text8bpp, BgSize_T_256x256, 31,0);

    REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_DST_BG0; REG_BLDY = 16;
    REG_BLDCNT_SUB = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_DST_BG0; REG_BLDY_SUB = 16;

    mmEffect(SFX_MUS_INTRO);

    // Show splash screen
    decompress(splashTiles, bgGetGfxPtr(bg1), LZ77Vram);
    decompress(splashMap, (void*) bgGetMapPtr(bg1), LZ77Vram);
    dmaCopy((void *) splashPal,(u16*) BG_PALETTE,256*2);

    decompress(pdev_bg0Tiles, bgGetGfxPtr(bg1s), LZ77Vram);
    decompress(pdev_bg0Map, (void*) bgGetMapPtr(bg1s), LZ77Vram);
    dmaCopy((void *) pdev_bg0Pal,(u16*) BG_PALETTE_SUB,256*2);

    FadeToColor(0,BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_DST_BG0,3,0,3);

    bOK=false;
    while (!bOK) { if ( !(keysCurrent() & 0x1FFF) ) bOK=true; } // 0x1FFF = key or touch screen
    vusCptVBL=0;bOK=false;
    while (!bOK && (vusCptVBL<3*60)) { if (keysCurrent() & 0x1FFF ) bOK=true; }
    bOK=false;
    while (!bOK) { if ( !(keysCurrent() & 0x1FFF) ) bOK=true; }

    FadeToColor(1,BLEND_FADE_WHITE | BLEND_SRC_BG0 | BLEND_DST_BG0,3,16,3);
}

// End of file
