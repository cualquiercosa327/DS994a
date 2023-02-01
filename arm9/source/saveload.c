// =====================================================================================
// Copyright (c) 2023 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated 
// readme files, with or without modification, are permitted in any medium without 
// royalty provided this copyright notice is used and wavemotion-dave is thanked profusely.
//
// The TI99DS emulator is offered as-is, without any warranty.
// =====================================================================================
#include <nds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fat.h>
#include <dirent.h>

#include "DS99.h"
#include "CRC32.h"
#include "cpu/tms9900/tms9901.h"
#include "cpu/tms9900/tms9900.h"
#include "DS99mngt.h"
#include "DS99_utils.h"
#include "SAMS.h"
#define NORAM 0xFF

#define TI_SAVE_VER   0x0006        // Change this if the basic format of the .SAV file changes. Invalidates older .sav files.

/*********************************************************************************
 * Save the current state - save everything we need to a single .sav file.
 ********************************************************************************/
u8  spare[512] = {0x00};    // We keep some spare bytes so we can use them in the future without changing the structure
static char szFile[160];
static char szCh1[33];
void TI99SaveState() 
{
  u32 uNbO;
  long pSvg;

  // Change into the last known ROMs directory
  chdir(currentDirROMs);
    
  // Init filename = romname and SAV in place of ROM
  DIR* dir = opendir("sav");
  if (dir) closedir(dir);  // Directory exists... close it out and move on.
  else mkdir("sav", 0777);   // Otherwise create the directory...
  siprintf(szFile,"sav/%s", gpFic[ucGameAct].szName);

  int len = strlen(szFile);
  if (szFile[len-3] == '.') // In case of .sg or .sc
  {
      szFile[len-2] = 's';
      szFile[len-1] = 'a';
      szFile[len-0] = 'v';
      szFile[len+1] = 0;
  }
  else
  {
      szFile[len-3] = 's';
      szFile[len-2] = 'a';
      szFile[len-1] = 'v';
  }
  
  strcpy(szCh1,"SAVING...");
  DS_Print(10,0,0,szCh1);
  
  FILE *handle = fopen(szFile, "wb+");  
  if (handle != NULL) 
  {
    // Write Version
    u16 save_ver = TI_SAVE_VER;
    uNbO = fwrite(&save_ver, sizeof(u16), 1, handle);
    
    // Write TMS9900 CPU and TMS9901 IO handling memory
    uNbO = fwrite(&tms9900, sizeof(tms9900), 1, handle);
    uNbO = fwrite(&tms9901, sizeof(tms9901), 1, handle);
      
    // Write SAMS memory
    if (uNbO) uNbO = fwrite(&theSAMS, sizeof(theSAMS),1, handle); 
      
    // Save TI Memory that might possibly be volatile (RAM areas mostly)
    if (uNbO) uNbO = fwrite(MemCPU+0x2000, 0x2000, 1, handle); 
    if (uNbO) uNbO = fwrite(MemCPU+0x6000, 0x2000, 1, handle); 
    if (uNbO) uNbO = fwrite(MemCPU+0x8000, 0x0400, 1, handle); 
    if (uNbO) uNbO = fwrite(MemCPU+0xA000, 0x6000, 1, handle); 
      
    // A few frame counters
    if (uNbO) uNbO = fwrite(&emuActFrames, sizeof(emuActFrames), 1, handle); 
    if (uNbO) uNbO = fwrite(&timingFrames, sizeof(timingFrames), 1, handle); 
      
    // Write VDP
    if (uNbO) uNbO = fwrite(VDP, sizeof(VDP),1, handle); 
    if (uNbO) uNbO = fwrite(&VDPCtrlLatch, sizeof(VDPCtrlLatch),1, handle); 
    if (uNbO) uNbO = fwrite(&VDPStatus, sizeof(VDPStatus),1, handle); 
    if (uNbO) uNbO = fwrite(&FGColor, sizeof(FGColor),1, handle); 
    if (uNbO) uNbO = fwrite(&BGColor, sizeof(BGColor),1, handle); 
    if (uNbO) uNbO = fwrite(&OH, sizeof(OH),1, handle); 
    if (uNbO) uNbO = fwrite(&IH, sizeof(IH),1, handle);       
    if (uNbO) uNbO = fwrite(&ScrMode, sizeof(ScrMode),1, handle); 
    if (uNbO) uNbO = fwrite(&VDPDlatch, sizeof(VDPDlatch),1, handle); 
    if (uNbO) uNbO = fwrite(&VAddr, sizeof(VAddr),1, handle); 
    if (uNbO) uNbO = fwrite(&CurLine, sizeof(CurLine),1, handle); 
    if (uNbO) uNbO = fwrite(&ColTabM, sizeof(ColTabM),1, handle); 
    if (uNbO) uNbO = fwrite(&ChrGenM, sizeof(ChrGenM),1, handle); 
    if (uNbO) uNbO = fwrite(pVDPVidMem, 0x4000,1, handle); 
    pSvg = ChrGen-pVDPVidMem;
    if (uNbO) uNbO = fwrite(&pSvg, sizeof(pSvg),1, handle); 
    pSvg = ChrTab-pVDPVidMem;
    if (uNbO) uNbO = fwrite(&pSvg, sizeof(pSvg),1, handle); 
    pSvg = ColTab-pVDPVidMem;
    if (uNbO) uNbO = fwrite(&pSvg, sizeof(pSvg),1, handle); 
    pSvg = SprGen-pVDPVidMem;
    if (uNbO) uNbO = fwrite(&pSvg, sizeof(pSvg),1, handle); 
    pSvg = SprTab-pVDPVidMem;
    if (uNbO) uNbO = fwrite(&pSvg, sizeof(pSvg),1, handle); 

    // Write PSG sound chips...
    if (uNbO) uNbO = fwrite(&sncol, sizeof(sncol),1, handle); 
      
    // Some spare memory we can eat into...
    if (uNbO) uNbO = fwrite(&spare, 512,1, handle); 
    
    // SAMS memory is huge (1MB) so we will do some simple Run-Length Encoding of 0x00000000 dwords 
    u32 i=0;
    u32 zero=0x00000000;
    while (i < (theSAMS.numBanks * (4*1024)))
    {
        if (SAMS_Read32(i) == zero) 
        {
            u32 count=0;
            u32 data32 = SAMS_Read32(i);
            while ((data32 == zero) && (i < (theSAMS.numBanks * (4*1024))))
            {
                count++;
                i+=4;
                data32 = SAMS_Read32(i);
            }
            if (uNbO) uNbO = fwrite(&zero,  sizeof(zero),  1, handle); 
            if (uNbO) uNbO = fwrite(&count, sizeof(count), 1, handle);            
        }
        else
        {
            u32 data32 = SAMS_Read32(i);
            if (uNbO) uNbO = fwrite(&data32, sizeof(data32), 1, handle); 
            i+=4;
        }
    }
    // Restore the memory banks as they were...
    SAMS_cru_write(0x0000, theSAMS.cruSAMS[0]);
    SAMS_cru_write(0x0001, theSAMS.cruSAMS[1]);
      
    // And finally the 'special' memory layout carts...
    if (myConfig.cartType == CART_TYPE_SUPERCART)
    {
        if (uNbO) uNbO = fwrite(MemCPU+0x6000, 0x2000, 1, handle); 
    }
    if (myConfig.cartType == CART_TYPE_MINIMEM)
    {
        if (uNbO) uNbO = fwrite(MemCPU+0x7000, 0x1000, 1, handle); 
    }
    if ((myConfig.cartType == CART_TYPE_MBX_NO_RAM) || (myConfig.cartType == CART_TYPE_MBX_WITH_RAM))
    {
        if (uNbO) uNbO = fwrite(MemCPU+0x6000, 0x2000, 1, handle); 
    }

    fclose(handle);
      
    if (uNbO) 
      strcpy(szCh1,"OK ");
    else
      strcpy(szCh1,"ERR");
     DS_Print(19,0,0,szCh1);
    WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
    DS_Print(10,0,0,"             "); 
    DisplayStatusLine(true);
  }
  else 
  {
    strcpy(szCh1,"Error opening SAV file ...");
  }
}


/*********************************************************************************
 * Load the current state - read everything back from the .sav file.
 ********************************************************************************/
void TI99LoadState() 
{
    u32 uNbO;
    long pSvg;

    // Change into the last known ROMs directory
    chdir(currentDirROMs);
    
    // Init filename = romname and .SAV in place of ROM
    siprintf(szFile,"sav/%s", gpFic[ucGameAct].szName);
    int len = strlen(szFile);
    if (szFile[len-3] == '.') // In case of .sg or .sc
    {
      szFile[len-2] = 's';
      szFile[len-1] = 'a';
      szFile[len-0] = 'v';
      szFile[len+1] = 0;
    }
    else
    {
      szFile[len-3] = 's';
      szFile[len-2] = 'a';
      szFile[len-1] = 'v';
    }
    FILE* handle = fopen(szFile, "rb"); 
    if (handle != NULL) 
    {    
         strcpy(szCh1,"LOADING...");
         DS_Print(10,0,0,szCh1);
       
        // Read Version
        u16 save_ver = 0xBEEF;
        uNbO = fread(&save_ver, sizeof(u16), 1, handle);
        
        if (save_ver == TI_SAVE_VER)
        {
            // Load TMS9900 CPU and TMS9901 IO handling memory
            if (uNbO) uNbO = fread(&tms9900, sizeof(tms9900), 1, handle);
            if (uNbO) uNbO = fread(&tms9901, sizeof(tms9901), 1, handle);
            
            // Load SAMS memory
            if (uNbO) uNbO = fread(&theSAMS, sizeof(theSAMS),1, handle); 
            
            // Ensure we are pointing to the right cart bank in memory
            if (tms9900.bankOffset == 0) tms9900.cartBankPtr = FastCartBuffer;
            else tms9900.cartBankPtr = MemCART+tms9900.bankOffset;
            
            // Restore TI Memory that might possibly be volatile (RAM areas mostly)
            if (uNbO) uNbO = fread(MemCPU+0x2000, 0x2000, 1, handle); 
            if (uNbO) uNbO = fread(MemCPU+0x6000, 0x2000, 1, handle); 
            if (uNbO) uNbO = fread(MemCPU+0x8000, 0x0400, 1, handle); 
            if (uNbO) uNbO = fread(MemCPU+0xA000, 0x6000, 1, handle); 
            
            // A few frame counters
            if (uNbO) uNbO = fread(&emuActFrames, sizeof(emuActFrames), 1, handle); 
            if (uNbO) uNbO = fread(&timingFrames, sizeof(timingFrames), 1, handle); 
            
            // Load VDP
            if (uNbO) uNbO = fread(VDP, sizeof(VDP),1, handle); 
            if (uNbO) uNbO = fread(&VDPCtrlLatch, sizeof(VDPCtrlLatch),1, handle); 
            if (uNbO) uNbO = fread(&VDPStatus, sizeof(VDPStatus),1, handle); 
            if (uNbO) uNbO = fread(&FGColor, sizeof(FGColor),1, handle); 
            if (uNbO) uNbO = fread(&BGColor, sizeof(BGColor),1, handle); 
            if (uNbO) uNbO = fread(&OH, sizeof(OH),1, handle); 
            if (uNbO) uNbO = fread(&IH, sizeof(IH),1, handle); 
            if (uNbO) uNbO = fread(&ScrMode, sizeof(ScrMode),1, handle); 
            extern void (*RefreshLine)(u8 uY);  RefreshLine = SCR[ScrMode].Refresh;
            if (uNbO) uNbO = fread(&VDPDlatch, sizeof(VDPDlatch),1, handle); 
            if (uNbO) uNbO = fread(&VAddr, sizeof(VAddr),1, handle); 
            if (uNbO) uNbO = fread(&CurLine, sizeof(CurLine),1, handle); 
            if (uNbO) uNbO = fread(&ColTabM, sizeof(ColTabM),1, handle); 
            if (uNbO) uNbO = fread(&ChrGenM, sizeof(ChrGenM),1, handle); 
            
            if (uNbO) uNbO = fread(pVDPVidMem, 0x4000,1, handle); 
            if (uNbO) uNbO = fread(&pSvg, sizeof(pSvg),1, handle); 
            ChrGen = pSvg + pVDPVidMem;
            if (uNbO) uNbO = fread(&pSvg, sizeof(pSvg),1, handle); 
            ChrTab = pSvg + pVDPVidMem;
            if (uNbO) uNbO = fread(&pSvg, sizeof(pSvg),1, handle); 
            ColTab = pSvg + pVDPVidMem;
            if (uNbO) uNbO = fread(&pSvg, sizeof(pSvg),1, handle); 
            SprGen = pSvg + pVDPVidMem;
            if (uNbO) uNbO = fread(&pSvg, sizeof(pSvg),1, handle); 
            SprTab = pSvg + pVDPVidMem;
            
            // Load PSG Sound Stuff
            if (uNbO) uNbO = fread(&sncol, sizeof(sncol),1, handle); 
            
            // Load spare memory for future use
            if (uNbO) uNbO = fread(&spare, 512,1, handle); 
            
            // SAMS memory is huge (1MB) so we will do some simple Run-Length Encoding of 0x00000000 dwords 
            u32 i=0;
            u32 zero=0x00000000;
            while (i < (theSAMS.numBanks * (4*1024)))
            {
                u32 data32;
                if (uNbO) uNbO = fread(&data32, sizeof(data32),1, handle); 
                if (data32 == zero) 
                {
                    u32 count=0;
                    if (uNbO) uNbO = fread(&count, sizeof(count),1, handle); 
                    while (count--) 
                    {
                        SAMS_Write32(i, 0x00000000);
                        i+=4;
                    }
                }
                else
                {
                    SAMS_Write32(i, data32);
                    i+=4;
                }
            }
            
            // And finally the 'special' memory layout carts...
            if (myConfig.cartType == CART_TYPE_SUPERCART)
            {
                if (uNbO) uNbO = fread(MemCPU+0x6000, 0x2000, 1, handle); 
            }
            if (myConfig.cartType == CART_TYPE_MINIMEM)
            {
                if (uNbO) uNbO = fread(MemCPU+0x7000, 0x1000, 1, handle); 
            }
            if ((myConfig.cartType == CART_TYPE_MBX_NO_RAM) || (myConfig.cartType == CART_TYPE_MBX_WITH_RAM))
            {
                if (uNbO) uNbO = fread(MemCPU+0x6000, 0x2000, 1, handle); 
            }
            
            // Make sure our DSR is installed propery and the right memory region is mapped in
            SAMS_MapDSR(theSAMS.cruSAMS[0]);
            
            // Fix up transparency
            if (BGColor)
            {
              u8 r = (u8) ((float) TMS9918A_palette[BGColor*3+0]*0.121568f);
              u8 g = (u8) ((float) TMS9918A_palette[BGColor*3+1]*0.121568f);
              u8 b = (u8) ((float) TMS9918A_palette[BGColor*3+2]*0.121568f);
              BG_PALETTE[0] = RGB15(r,g,b);
            }
            else
            {
               BG_PALETTE[0] = RGB15(0x00,0x00,0x00);
            }
        }
        else uNbO = 0;
        
        if (uNbO) 
          strcpy(szCh1,"OK ");
        else
          strcpy(szCh1,"ERR");
         DS_Print(19,0,0,szCh1);
        
        WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
        DS_Print(10,0,0,"              ");  
        DisplayStatusLine(true);
      }
      else
      {
        DS_Print(10,0,0,"NO SAVED GAME");
        WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;WAITVBL;
        DS_Print(10,0,0,"             ");  
      }

    fclose(handle);
}

// End of file
