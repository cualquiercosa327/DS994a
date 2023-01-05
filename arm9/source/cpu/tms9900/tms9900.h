
//----------------------------------------------------------------------------
//
// File:        tms9900.hpp
// Date:        23-Feb-1998
// Programmer:  Marc Rousseau
//
// Description:
//
// Copyright (c) 1998-2004 Marc Rousseau, All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
//
// Revision History:
//
//----------------------------------------------------------------------------

#ifndef TMS9900_HPP_
#define TMS9900_HPP_

#include <stdbool.h>

#define TMS_LOGICAL       0x8000
#define TMS_ARITHMETIC    0x4000
#define TMS_EQUAL         0x2000
#define TMS_CARRY         0x1000
#define TMS_OVERFLOW      0x0800
#define TMS_PARITY        0x0400
#define TMS_XOP           0x0200

#define MEMFLG_SPEECH        0x01
#define MEMFLG_BANKW         0x02
#define MEMFLG_8BIT          0x04
#define MEMFLG_VDPR          0x08
#define MEMFLG_VDPW          0x10
#define MEMFLG_GROMR         0x20
#define MEMFLG_GROMW         0x40
#define MEMFLG_SOUND         0x80

extern void TMS9900_Reset(char *szGame);
extern void SetPC( ADDRESS address ) ;
extern void SetWP( ADDRESS address ) ;
extern void SetST( UINT16 address ) ;
extern ADDRESS GetPC( ) ;
extern ADDRESS GetWP( ) ;
extern UINT16 GetST( ) ;
extern void TMS9900_Run( ) ;
extern void TMS9900_Stop( ) ;
extern bool IsRunning( ) ;
extern void Reset( ) ;
extern void SignalInterrupt( UINT8 ) ;
extern void ClearInterrupt( UINT8 ) ;
extern UINT32 GetClocks( ) ;
extern void AddClocks( int ) ;
extern void ResetClocks( ) ;
extern UINT32 GetCounter( ) ;
extern void ResetCounter( ) ;
extern UINT8 RegisterTrapHandler( TRAP_FUNCTION, void *, int ) ;
extern void DeRegisterTrapHandler( UINT8 ) ;
extern UINT8 GetTrapIndex( TRAP_FUNCTION, int ) ;
extern bool SetTrap( ADDRESS, UINT8, UINT8 ) ;
extern void ClearTrap( UINT8, ADDRESS, int ) ;
extern void RegisterDebugHandler( BREAKPOINT_FUNCTION, void * ) ;
extern void DeRegisterDebugHandler( ) ;
extern bool SetBreakpoint( ADDRESS, UINT8 ) ;
extern bool ClearBreakpoint( ADDRESS, UINT8 ) ;

typedef struct _sOpCode
{
    char        mnemonic[ 5 ];
    UINT16      opCode;
    UINT16      mask;
    UINT16      format;
    void      (*function)( );
    UINT32      clocks;
} sOpCode;


// Functions required by opcodes.cpp

extern void InvalidOpcode( );
extern void (*TimerHook)( );
extern UINT8 CallTrapB( bool read, ADDRESS address, UINT8 value );
extern UINT16 CallTrapW( bool read, bool isFetch, ADDRESS address, UINT16 value );

// Functions provided by opcodes.cpp

extern void Run( );
extern void Stop( );
extern bool Step( );
extern bool IsRunning( );
extern void ContextSwitch( UINT16 address );
extern void InitOpCodeLookup( );

extern UINT8           Memory[0x10000];            // 64K of CPU Memory Space
extern UINT8           MemGROM[0x10000];           // 64K of GROM Memory Space
extern UINT8           MemFlags[0x10000];          // And the memory flags that tell what is what...
extern UINT8           CartMem[0x10000];           // Cart C memory up to 64K

extern UINT16          InterruptFlag    ;
extern UINT16          WorkspacePtr     ;
extern UINT16          ProgramCounter   ;
extern UINT16          Status           ;
extern UINT32          ClockCycleCounter;
extern UINT16          fetchPtr         ;
extern UINT16          curOpCode        ;
extern UINT16          bankOffset       ;
extern UINT8           m_GromWriteShift;
extern UINT8           m_GromReadShift;
extern UINT16          gromAddress;


void opcode_A   ( );
void opcode_AB  ( );
void opcode_ABS ( );
void opcode_AI  ( );
void opcode_ANDI( );
void opcode_B   ( );
void opcode_BL  ( );
void opcode_BLWP( );
void opcode_C   ( );
void opcode_CB  ( );
void opcode_CI  ( );
void opcode_CKOF( );
void opcode_CKON( );
void opcode_CLR ( );
void opcode_COC ( );
void opcode_CZC ( );
void opcode_DEC ( );
void opcode_DECT( );
void opcode_DIV ( );
void opcode_IDLE( );
void opcode_INC ( );
void opcode_INCT( );
void opcode_INV ( );
void opcode_JEQ ( );
void opcode_JGT ( );
void opcode_JH  ( );
void opcode_JHE ( );
void opcode_JL  ( );
void opcode_JLE ( );
void opcode_JLT ( );
void opcode_JMP ( );
void opcode_JNC ( );
void opcode_JNE ( );
void opcode_JNO ( );
void opcode_JOC ( );
void opcode_JOP ( );
void opcode_LDCR( );
void opcode_LI  ( );
void opcode_LIMI( );
void opcode_LREX( );
void opcode_LWPI( );
void opcode_MOV ( );
void opcode_MOVB( );
void opcode_MPY ( );
void opcode_NEG ( );
void opcode_ORI ( );
void opcode_RSET( );
void opcode_RTWP( );
void opcode_S   ( );
void opcode_SB  ( );
void opcode_SBO ( );
void opcode_SBZ ( );
void opcode_SETO( );
void opcode_SLA ( );
void opcode_SOC ( );
void opcode_SOCB( );
void opcode_SRA ( );
void opcode_SRC ( );
void opcode_SRL ( );
void opcode_STCR( );
void opcode_STST( );
void opcode_STWP( );
void opcode_SWPB( );
void opcode_SZC ( );
void opcode_SZCB( );
void opcode_TB  ( );
void opcode_X   ( );
void opcode_XOP ( );
void opcode_XOR ( );

#endif
