/*
======================================================================
P-Vision Reset Utility
Copyright (C) 2026 by Oliver Achten
======================================================================

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//#define DEBUG
#define INTUI_V36_NAMES_ONLY

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <libraries/configvars.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/dos.h>

#include <dos/dosasl.h>

#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/alib_stdio_protos.h>
#include <clib/asl_protos.h>
#include <clib/expansion_protos.h>

#include <proto/expansion.h>
#include <proto/dos.h>

#ifdef LATTICE
int CXBRK(void)    { return(0); }  /* Disable Lattice CTRL/C handling */
int chkabort(void) { return(0); }  /* really */
#endif

void WriteReg16(ULONG base, ULONG offset, UWORD val)
{
	*((UWORD *)((ULONG)base+offset)) = val;	
}

UWORD ReadReg16(APTR base, ULONG offset)
{
	return *((UWORD *)((ULONG)base+offset));
}

void WriteReg32(ULONG base, ULONG offset, ULONG val)
{
	*((ULONG *)((ULONG)base+offset)) = val;	
}

void resetPCMCIA(void)
{
	UWORD regval, dummy, i;
	
	// This code works on both Amiga 600 / 1200
	
	// AmigaOS "card.resource" uses the wrong Gayle reset register, which only works on Amiga 600
	// Therefore, the root-cause of the so-called Amiga 1200 "Gayle hardware bug" is actually a software issue
	
	regval = ReadReg16(0xda9000, 0);		// Get actual state
	
	WriteReg16(0xda9000,0,regval | 0x0300);	// Confirmed by Gayle schematics
	
	for (i=0; i<100; i++)
	{
		dummy = ReadReg16(0xda9000, 0);		// I/O timed delay
	}
	
	WriteReg16(0xda9000,0,regval);			// Restore previous state
	
}

void main(void)
{
	resetPCMCIA();	// Easy peasy....	
}
