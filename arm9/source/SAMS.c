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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fat.h>

#include "DS99.h"
#include "DS99_utils.h"
#include "SAMS.h"
#include "cpu/tms9918a/tms9918a.h"
#include "cpu/tms9900/tms9901.h"
#include "cpu/tms9900/tms9900.h"
#include "cpu/sn76496/SN76496.h"

u8 *MemSAMS             __attribute__((section(".dtcm"))) = 0;  // Allocated to support 512K for DS-Lite and 1MB for DSi and above
SAMS theSAMS            __attribute__((section(".dtcm")));      // The entire state of the SAMS memory map handler
u8 sams_highwater_bank  __attribute__((section(".dtcm"))) = 0;  // To track how far into SAMS memory we used

// ---------------------------------------------------------------------------------------
// SAMS is handled via the CRU and has registers mapped into the DSR space but it does
// not have a proper DSR. The two CRU bits used are:
//
// CRU >1E00  - Enable or Disable the visibility of the SAMS registers
// CRU >1E02  - Enable SAMS memory mapping or 'Pass-Thru' mode (acting like a traditional 32K expansion)
//
// If the SAMS registers are mapped in, there are 16 word-registers (covering 32 bytes) but
// only a subset are actually used to map in 4K memory banks into the TI-99 system:
//
// > 4000 - Banking for TI-99/4a Memory Range 0000-0FFF (not mappable)
// > 4002 - Banking for TI-99/4a Memory Range 1000-1FFF (not mappable)
// > 4004 - Banking for TI-99/4a Memory Range 2000-2FFF (mappable)
// > 4006 - Banking for TI-99/4a Memory Range 3000-3FFF (mappable)
// > 4008 - Banking for TI-99/4a Memory Range 4000-4FFF (not mappable)
// > 400A - Banking for TI-99/4a Memory Range 5000-5FFF (not mappable)
// > 400C - Banking for TI-99/4a Memory Range 6000-6FFF (not mappable)
// > 400E - Banking for TI-99/4a Memory Range 7000-7FFF (not mappable)
// > 4010 - Banking for TI-99/4a Memory Range 8000-8FFF (not mappable)
// > 4012 - Banking for TI-99/4a Memory Range 9000-9FFF (not mappable)
// > 4014 - Banking for TI-99/4a Memory Range A000-AFFF (mappable)
// > 4016 - Banking for TI-99/4a Memory Range B000-BFFF (mappable)
// > 4018 - Banking for TI-99/4a Memory Range C000-CFFF (mappable)
// > 401A - Banking for TI-99/4a Memory Range D000-DFFF (mappable)
// > 401C - Banking for TI-99/4a Memory Range E000-EFFF (mappable)
// > 401E - Banking for TI-99/4a Memory Range F000-FFFF (mappable)
// ---------------------------------------------------------------------------------------

// ---------------------------------------------------------
// Setup for SAMS 512K (DS) or 1MB (DSi)
// ---------------------------------------------------------
void SAMS_Initialize(void)
{
    // Start with everything cleared out...
    memset(&theSAMS, 0x00, sizeof(theSAMS));

    // --------------------------------------------------------------
    // For SAMS memory, ensure everything points to the right banks
    // and default the system to pass-thru mode...
    // --------------------------------------------------------------
    theSAMS.cruSAMS[0] = 0;
    theSAMS.cruSAMS[1] = 0;

    // ------------------------------------------------------------------------
    // SAMS memory is bigger for the DSi where we have more room... See a bit
    // further below since with the DS-Lite/Phat we need to reduce the size
    // of the max cart buffer in order to support the 512K SAMS memory.
    // ------------------------------------------------------------------------
    theSAMS.numBanks = (isDSiMode() ? 256 : 128);  // 256 * 4K = 1024K,  128 * 4K = 512K

    // For each bank... set the default memory banking pointers
    for (u8 i=0; i<16; i++)
    {
        theSAMS.bankMapSAMS[i] = i;
        theSAMS.memoryPtr[i] = MemSAMS + (i * 0x1000);
    }

    // --------------------------------------------------------------------------
    // We don't map the MemType[] here.. only when CRU bit is written but we do
    // clear out the SAMS memory to all zeros (helps with savestate compression)
    // --------------------------------------------------------------------------
    memset(MemSAMS, 0x00, ((theSAMS.numBanks) * 0x1000));

    // -----------------------------------------------------------------
    // If we are configured for SAMS operation... set the accuracy flag
    // to map in slower (but more accurate) mapping.
    // -----------------------------------------------------------------
    if (myConfig.machineType == MACH_TYPE_SAMS)
    {
        TMS9900_SetAccurateEmulationFlag(ACCURATE_EMU_SAMS);
        SAMS_cru_write(0,0);    // Swap out the visibility of the SAMS memory mapped registers (so they are not visible)
        SAMS_cru_write(1,0);    // Mapper Disabled... (pass-thru mode which is basically like having a 32K expansion)

        if (!isDSiMode()) MAX_CART_SIZE = (256 * 1024);  // If we are DS-Lite/Phat, we reduce the size of the cart to support larger SAMS
    }
    else
    {
        if (!isDSiMode()) MAX_CART_SIZE = (512 * 1024);  // If we are DS-Lite/Phat, we can support a larger cart size when SAMS is disabled
    }

    sams_highwater_bank = 0x00;
}


// --------------------------------------------------------------------------------------
// SAMS memory bank swapping will point into the 4K region of the large SAMS memory pool.
// We only allow mapping of SAMS 4K memory banks into the TI-99/4a memory map that would
// hold expansion RAM ... that is: >2000 and >A000 areas. We cannot map RAM into areas
// that hold console the console ROMS, cart ROM or other peripherals.
// --------------------------------------------------------------------------------------
const u8 IsSwappableSAMS[16] = {0,0,1,1,0,0,0,0,0,0,1,1,1,1,1,1};

inline void SAMS_SwapBank(u8 memory_region, u8 bank)
{
    // For smaller than 1MB SAMS, it's acceptable to mirror the memory (so 512K ends up visible in both halves of the banking)
    bank &= (theSAMS.numBanks - 1);

    if (bank < theSAMS.numBanks)
    {
        if (IsSwappableSAMS[(memory_region&0xF)])    // Make sure this is an area we allow swapping...
        {
            theSAMS.memoryPtr[memory_region] = MemSAMS + ((u32)bank * 0x1000);
            if (bank > sams_highwater_bank) sams_highwater_bank = bank;
        }
    }
}

// -------------------------------------------------------------------------------------------
// The SAMS banks are 4K and we only allow mapping of the banks at >2000-3FFF and >A000-FFFF
// -------------------------------------------------------------------------------------------
void SAMS_WriteBank(u16 address, u8 data)
{
    if (theSAMS.cruSAMS[0] == 1)    // Do we have access to the registers?
    {
        u8 memory_region = (address >> 1) & 0xF;

        if (theSAMS.cruSAMS[1] == 1)    // If the mapper is enabled, swap banks
        {
            SAMS_SwapBank(memory_region, data);
        }

        // Set this as the new bank for that memory region
        theSAMS.bankMapSAMS[memory_region] = data;
    }
}

// ----------------------------------------------------------
// Return the current bank mapped at a particular address.
// ----------------------------------------------------------
u8 SAMS_ReadBank(u16 address)
{
    return theSAMS.bankMapSAMS[(address & 0x1E) >> 1];
}

// ---------------------------------------------------------------------
// The SAMS CRU is at CRU base >1E00 and has only 2 bits.. the first
// turns on the visibility of the SAMS register map at >4000 and the
// second enables mapping vs "pass-thru" of the memory. In "pass-thru"
// we end up looking just like a normal 32K expanded memory system.
// ---------------------------------------------------------------------
void SAMS_cru_write(u16 cruAddress, u8 dataBit)
{
    // -----------------------------------------------------------
    // If the machine has been configured for SAMS operation...
    // -----------------------------------------------------------
    if (myConfig.machineType == MACH_TYPE_SAMS)
    {
        theSAMS.cruSAMS[cruAddress & 1] = dataBit;
        if (cruAddress & 1)    // If we are writing the mapper enabled bit...
        {
            if (theSAMS.cruSAMS[1] == 1)  // If the mapper is enabled...
            {
                SAMS_SwapBank(0x02, theSAMS.bankMapSAMS[0x2]);
                SAMS_SwapBank(0x03, theSAMS.bankMapSAMS[0x3]);
                SAMS_SwapBank(0x0A, theSAMS.bankMapSAMS[0xA]);
                SAMS_SwapBank(0x0B, theSAMS.bankMapSAMS[0xB]);
                SAMS_SwapBank(0x0C, theSAMS.bankMapSAMS[0xC]);
                SAMS_SwapBank(0x0D, theSAMS.bankMapSAMS[0xD]);
                SAMS_SwapBank(0x0E, theSAMS.bankMapSAMS[0xE]);
                SAMS_SwapBank(0x0F, theSAMS.bankMapSAMS[0xF]);
            }
            else    // Pass-thru mode - map the lower 32K in transparently...
            {
                SAMS_SwapBank(0x02, 0x2);
                SAMS_SwapBank(0x03, 0x3);
                SAMS_SwapBank(0x0A, 0xA);
                SAMS_SwapBank(0x0B, 0xB);
                SAMS_SwapBank(0x0C, 0xC);
                SAMS_SwapBank(0x0D, 0xD);
                SAMS_SwapBank(0x0E, 0xE);
                SAMS_SwapBank(0x0F, 0xF);
            }
        }
        else // We are dealing with the enabled bit (there is no DSR for the SAMS, but it's more a card enable so that registers are visible and can be written to)
        {
            SAMS_EnableDisable(dataBit);
        }
    }
}

// -----------------------------------------------------------
// It's unclear if SAMS hardware allows the readback of the
// CRU bits... but it doesn't hurt to provide the capability.
// -----------------------------------------------------------
u8 SAMS_cru_read(u16 cruAddress)
{
    // -----------------------------------------------------------
    // If the machine has been configured for SAMS operation...
    // -----------------------------------------------------------
    if (myConfig.machineType == MACH_TYPE_SAMS)
    {
        return theSAMS.cruSAMS[cruAddress & 1];
    }
    return 1;
}

// ------------------------------------------------------------------
// Map the SAMS registers in/out at address 0>4000 which is shared
// with the Disk Controller (and other periprhals in the future)
// Note: SAMS does not have a traditional DSR rom - so this CRU
// bit is really more like a SAMS enable/disable as we are just
// enabling the memory mapped registers here (no ROM is swapped).
// ------------------------------------------------------------------
void SAMS_EnableDisable(u8 dataBit)
{
    if (dataBit == 1) // Mapping SAMS card in
    {
        for (u16 address = 0x4000; address < 0x4020; address += 16)
        {
            MemType[address>>4] = MF_SAMS;    // SAMS expanded memory handling
        }
    }
    else // Mapping the SAMS card out
    {
        for (u16 address = 0x4000; address < 0x4020; address += 16)
        {
            MemType[address>>4] = MF_PERIF;    // Map back to original handling (peripheral ROM)
        }
    }
}

// --------------------------------------------------------------------------------------------------
// These 32-bit read/write functions are used only for the Load/Save state handlers in savegame.c
// and are mainly needed so we can do simple Run-Length-Encoding (RLE) on the big SAMS memory area.
// --------------------------------------------------------------------------------------------------
u32 SAMS_Read32(u32 address)
{
    u32* ptr = (u32*)MemSAMS;
    return ptr[address>>2];
}

void SAMS_Write32(u32 address, u32 data)
{
    u32* ptr = (u32*)MemSAMS;
    ptr[address>>2] = data;
}


// End of file
