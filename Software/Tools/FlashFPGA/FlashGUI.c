/*
======================================================================
P-Vision Flash Utility
Copyright (C) 2025 by Oliver Achten
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


#define CMD_FLASH	1
#define CMD_INFO	2
#define	CMD_INIT	3
#define	CMD_LOAD	4
#define	CMD_SAVE	5
#define	CMD_QUIT	6

/* Gadget defines of our choosing, to be used as GadgetID's,
** also used as the index into the gadget array my_gads[].
*/
#define BUTTON_FLASH (0)
#define BUTTON_INFO  (1)
#define BUTTON_INIT  (2)
#define BUTTON_SAVE  (3)
#define BUTTON_LOAD  (4)
#define BUTTON_QUIT  (5)

/* Range for the slider: */
#define SLIDER_MIN  (1)
#define SLIDER_MAX (20)


#define MYLEFTEDGE 0
#define MYTOPEDGE  0
#define MYWIDTH    320
#define MYHEIGHT   400

#define	PVISION_ID_HIGH	0x80
#define PVISION_ID_LOW	0x84

#define	PVISION_OSC		25
#define PVISION_BASE	0x00a00000

#define	FPGA_DATE				0x88

#define	FLASH_DATA_ADDRH		0xe0
#define	FLASH_DATA_ADDRL		0xe2
#define	FLASH_DATA_WRITE_PORT	0xe4
#define	FLASH_DATA_WRITE_STROBE	0xe8
#define FLASH_DATA_READ_STROBE	0xea
#define	FLASH_DATA_READ_PORT	0xec
#define	FLASH_DATA_STATUS		0xf0

#define	FLASH_CTRL_ADDR			0xf2
#define	FLASH_CTRL_WRITE_DATA	0xf4
#define	FLASH_CTRL_WRITE_STROBE	0xf8
#define	FLASH_CTRL_READ_STROBE	0xfa
#define	FLASH_CTRL_READ_DATA	0xfc

#define	FPGA_ID_HIGH			0xd8
#define	FPGA_ID_LOW				0xdc

#define	PLL_M_HCOUNT			0x110
#define	PLL_M_LCOUNT			0x112
#define	PLL_M_NCOUNT			0x11e

#define	BUS_CLK_M_HCOUNT		0x140
#define	BUS_CLK_M_LCOUNT		0x142

#define	MEMC_CLK_M_HCOUNT		0x150
#define	MEMC_CLK_M_LCOUNT		0x152

#define	PIX_CLK_M_HCOUNT		0x170
#define	PIX_CLK_M_LCOUNT		0x172

// 10M25SCE144 Device Flash Mapping (nowhere officially documented - THANKS INTEL!)
// Sector 1: UFM0 (16K)		0x0 - 0x3fff
// Sector 2: UFM1 (16K)		0x4000 - 0x7fff
// Sector 3: CFM0 (368K)	0x8000 - 0x63fff
// Sector 4: CFM1 (160K)	0x64000 - 0x8bfff

#define	FLASH_CONFIG_START		0x4000	// UFM1 Start
#define	FLASH_CONFIG_SIZE		0x4000

#define	FLASH_CORE_START		0x8000	// CFM1 Start
#define	FLASH_CORE_SIZE			0x84000	// Single compressed image: CFM1 & CFM0 (132 pages * 4K)


struct Library      *IntuitionBase;
struct Library      *ExpansionBase;
struct Library      *GfxBase;
struct Library      *AslBase;
struct Library      *GadToolsBase;

struct TextAttr Topaz80 = { "topaz.font", 8, 0, 0, };

struct TagItem frtags_flash[] =
{
    ASL_Hail,       (ULONG)"Select FPGA flash file",
    ASL_Height,     MYHEIGHT,
    ASL_Width,      MYWIDTH,
    ASL_LeftEdge,   MYLEFTEDGE,
    ASL_TopEdge,    MYTOPEDGE,
    ASL_OKText,     (ULONG)"OK",
    ASL_CancelText, (ULONG)"Cancel",
    ASL_File,       (ULONG)"P-Vision_FPGA.upd",
    ASL_Dir,        (ULONG)"PROGDIR:",
    TAG_DONE
};

struct TagItem frtags_load[] =
{
    ASL_Hail,       (ULONG)"Load FPGA config file",
    ASL_Height,     MYHEIGHT,
    ASL_Width,      MYWIDTH,
    ASL_LeftEdge,   MYLEFTEDGE,
    ASL_TopEdge,    MYTOPEDGE,
    ASL_OKText,     (ULONG)"OK",
    ASL_CancelText, (ULONG)"Cancel",
    ASL_File,       (ULONG)"P-Vision_config.acf",
    ASL_Dir,        (ULONG)"PROGDIR:",
    TAG_DONE
};

struct TagItem frtags_save[] =
{
    ASL_Hail,       (ULONG)"Save FPGA config file",
    ASL_Height,     MYHEIGHT,
    ASL_Width,      MYWIDTH,
    ASL_LeftEdge,   MYLEFTEDGE,
    ASL_TopEdge,    MYTOPEDGE,
    ASL_OKText,     (ULONG)"OK",
    ASL_CancelText, (ULONG)"Cancel",
    ASL_File,       (ULONG)"P-Vision_config.acf",
    ASL_Dir,        (ULONG)"PROGDIR:",
    TAG_DONE
};

short myBorderData[] =
{
  0,0, 100,0, 100,44, 0,44, 0,0,
};

short myBorderData2[] =
{
  0,0, 100,0, 100,60, 0,60, 0,0,
};

short myBorderData3[] =
{
  0,0, 256,0, 256,112, 0,112, 0,0,
};

short myBorderData4[] =
{
  0,0, 256,0, 256,14, 0,14, 0,0,
};

void WriteReg16(APTR base, ULONG offset, UWORD val)
{
#ifndef DEBUG
	*((UWORD *)((ULONG)base+offset)) = val;
#endif	
}

void WriteReg32(APTR base, ULONG offset, ULONG val)
{
#ifndef DEBUG
	*((ULONG *)((ULONG)base+offset)) = val;
#endif
}

UWORD ReadReg16(APTR base, ULONG offset)
{
#ifndef DEBUG	
	return *((UWORD *)((ULONG)base+offset));
#else
	return 0x0;
#endif
}

WORD ReadReg16S(APTR base, ULONG offset)
{
#ifndef DEBUG	
	return *((WORD *)((ULONG)base+offset));
#else
	return 0x0;
#endif
}

ULONG ReadReg32(APTR base, ULONG offset)
{
#ifndef DEBUG
	return *((ULONG *)((ULONG)base+offset));
#else
	return 0x0;
#endif
}

void EraseConfigFlash(APTR base)
{	
	ULONG status,sector;
	
	status = 0xffffffff;
	sector = 0x00200000;	// 0x1800 - 0x2fff
	
	WriteReg16(base, FLASH_CTRL_ADDR, 0x1);
	WriteReg32(base, FLASH_CTRL_WRITE_DATA, 0x0);				// Clear protection bits
	WriteReg16(base, FLASH_CTRL_WRITE_STROBE, 0x0);
		
	WriteReg32(base, FLASH_CTRL_WRITE_DATA, sector);	// Sector erase
	WriteReg16(base, FLASH_CTRL_WRITE_STROBE, 0x0);
	WriteReg16(base, FLASH_CTRL_ADDR, 0x0);						
		
	do {
		WriteReg16(base, FLASH_CTRL_READ_STROBE, 0x0);	
		
		status = (ULONG)(ReadReg32(base, FLASH_CTRL_READ_DATA) & (ULONG)0x3);	// Read Status register
		
	} while (status != 0x0);
}

void ProgramConfigFlash(APTR base, APTR memory)
{
	ULONG	memdata,flashoffset,status,cnt,length;
	
	flashoffset = (ULONG)(FLASH_CONFIG_START>>2);
	length = FLASH_CONFIG_SIZE;
	
	status = 0xffffffff;
	cnt = 0x0;
	
	WriteReg16(base, FLASH_CTRL_ADDR, 0x1);
	WriteReg32(base, FLASH_CTRL_WRITE_DATA, 0x0);				// Clear protection bits
	WriteReg16(base, FLASH_CTRL_WRITE_STROBE, 0x0);
	
	WriteReg16(base, FLASH_CTRL_ADDR, 0x0);
	
	do {
		memdata = *((ULONG *)((ULONG)memory+cnt));
		
		WriteReg32(base, FLASH_DATA_ADDRH, flashoffset);
		WriteReg32(base, FLASH_DATA_WRITE_PORT, memdata);
		WriteReg16(base, FLASH_DATA_WRITE_STROBE, 0x0);
		
		do {
			WriteReg16(base, FLASH_CTRL_READ_STROBE, 0x0);	
		
			status = (ULONG)(ReadReg32(base, FLASH_CTRL_READ_DATA) & (ULONG)0x3);	// Read Status register
		
		} while (status != 0x0);		
		
		
		flashoffset++;
		cnt+=4;
	}
	while (cnt < length);
}

void ReadConfigFlash(APTR base, APTR memory)
{
	ULONG	memdata,flashoffset,status,cnt,length;
	
	flashoffset = (ULONG)(FLASH_CONFIG_START>>2);
	length = FLASH_CONFIG_SIZE;
	
	status = 0xffffffff;
	cnt = 0x0;
	
	do {	
		WriteReg32(base, FLASH_DATA_ADDRH, flashoffset);
		WriteReg16(base, FLASH_DATA_READ_STROBE, 0x0);
		
		memdata = ReadReg32(base, FLASH_DATA_READ_PORT);
		*((ULONG *)((ULONG)memory+cnt)) = memdata;
		
		flashoffset++;
		cnt+=4;		
	}
	while (cnt < length);		
}

void EraseCoreFlash(APTR base)
{	
	ULONG status,sector;
	
	status = 0xffffffff;
	sector = 0x00100000;
	
	WriteReg16(base, FLASH_CTRL_ADDR, 0x1);
	WriteReg32(base, FLASH_CTRL_WRITE_DATA, 0x0);				// Clear protection bits
	WriteReg16(base, FLASH_CTRL_WRITE_STROBE, 0x0);
		
	do {
		WriteReg32(base, FLASH_CTRL_WRITE_DATA, sector);	// Sector erase
		WriteReg16(base, FLASH_CTRL_WRITE_STROBE, 0x0);
		WriteReg16(base, FLASH_CTRL_ADDR, 0x0);						
		
		do {
			WriteReg16(base, FLASH_CTRL_READ_STROBE, 0x0);	
		
			status = (ULONG)(ReadReg32(base, FLASH_CTRL_READ_DATA) & (ULONG)0x3);	// Read Status register
		
		} while (status != 0x0);

		sector += 0x00100000;
		
//		if (sector == 0x00200000)	// We skip 2nd sector, which contains the configuration data!
//			sector += 0x00100000;
		
		WriteReg16(base, FLASH_CTRL_ADDR, 0x1);
		
	} while (sector != 0x00500000);
}

void wPrintF(int line, char * text, BOOL erase,UWORD topborder,struct TextFont *font,struct Window *mywin)
{

	struct IntuiText  myIText;
	struct TextAttr   myTextAttr;

	long myTEXTPEN = 2;
	long myBACKGROUNDPEN = 3;	
	
	if (erase == TRUE)
	{
		SetAPen(mywin->RPort, 3);
		RectFill(mywin->RPort, 120, topborder+26-14, 120+253, topborder+135-14);
	}
	
	myTextAttr.ta_Name  = font->tf_Message.mn_Node.ln_Name;
    myTextAttr.ta_YSize = font->tf_YSize;
    myTextAttr.ta_Style = font->tf_Style;
    myTextAttr.ta_Flags = font->tf_Flags;
	  
	myIText.FrontPen    = myTEXTPEN;
	myIText.BackPen     = myBACKGROUNDPEN;
    myIText.DrawMode    = JAM2;
    myIText.LeftEdge    = 0;
    myIText.TopEdge     = 0;
    myIText.ITextFont   = &myTextAttr;
    myIText.NextText    = NULL;
	  
	myIText.IText       = text;
	PrintIText(mywin->RPort,&myIText,124,topborder+30-14+(line<<3));
}

void ProgramCoreFlash(APTR base, APTR memory, UWORD topborder, struct Window *mywin)
{
	ULONG	memdata,memoffset,flashoffset,status,cnt,pcnt,length,startx;
	
	flashoffset = (ULONG)(FLASH_CORE_START>>2);
	length = (ULONG)FLASH_CORE_SIZE;
	status = 0xffffffff;
	cnt = 0x0;
	pcnt = 0x0;
	startx = 0;
	
	SetAPen(mywin->RPort, 3);
	
	WriteReg16(base, FLASH_CTRL_ADDR, 0x1);
	WriteReg32(base, FLASH_CTRL_WRITE_DATA, 0x0);				// Clear protection bits
	WriteReg16(base, FLASH_CTRL_WRITE_STROBE, 0x0);
	WriteReg16(base, FLASH_CTRL_ADDR, 0x0);
	
	do {
		memdata = *((ULONG *)((ULONG)memory+cnt));
		
		WriteReg32(base, FLASH_DATA_ADDRH, flashoffset);
		WriteReg32(base, FLASH_DATA_WRITE_PORT, memdata);
		WriteReg16(base, FLASH_DATA_WRITE_STROBE, 0x0);
		
		do {
			WriteReg16(base, FLASH_CTRL_READ_STROBE, 0x0);	
		
			status = (ULONG)(ReadReg32(base, FLASH_CTRL_READ_DATA) & (ULONG)0x3);	// Read Status register
		
		} while (status != 0x0);		
		
		pcnt++;
		if (pcnt==533)
		{
			pcnt = 0;
			RectFill(mywin->RPort, 120+startx, topborder+146-14, 121+startx, topborder+157-14);				
			startx++;
		}
		
		flashoffset++;
		cnt+=4;
	}
	while (cnt < length);
}

BOOL VerifyCoreFlash(APTR base, APTR memory, UWORD topborder, struct Window *mywin)
{
	ULONG	memdata,retry,readdata,memoffset,flashoffset,status,cnt,pcnt,length,startx;
	
	flashoffset = (ULONG)(FLASH_CORE_START>>2);
	length = (ULONG)FLASH_CORE_SIZE;
	status = 0xffffffff;
	cnt = 0x0;
	pcnt = 0x0;
	startx = 0;
	
	SetAPen(mywin->RPort, 3);
	
	do {
		memdata = *((ULONG *)((ULONG)memory+cnt));
		
		WriteReg32(base, FLASH_DATA_ADDRH, flashoffset);
		WriteReg16(base, FLASH_DATA_READ_STROBE, 0x0);
		
		retry = 0;
		do{ 

			readdata = ReadReg32(base, FLASH_DATA_READ_PORT);
			if (memdata == readdata)
				break;
			retry++;
		} while (retry < 1000);
	
			printf("RD: %d MEM: %d retry:\n",readdata,memdata,retry);	
		if (retry == 1000)
			return FALSE;
		
		pcnt++;
		if (pcnt==533)
		{
			pcnt = 0;
			RectFill(mywin->RPort, 120+startx, topborder+146-14, 121+startx, topborder+157-14);				
			startx++;
		}
		
		flashoffset++;
		cnt+=4;
	}
	while (cnt < length);
	return TRUE;
}

void initCfgMem (APTR cfg_mem)
{
	ULONG cnt = 0;
		
	do {
		*((ULONG *)((ULONG)cfg_mem+cnt)) = 0xffffffff;
		cnt+=4;
	} while (cnt != 0x4000);
		
	*((ULONG *)((ULONG)cfg_mem+0x0000)) = 0x414d4947;	// Magic Token - Unlock

}

void errorMessage(STRPTR error)
{
if (error)
    printf("Error: %s\n", error);
}

void drawBorders(struct RastPort *rp,UWORD topborder,struct TextFont *font)
{

	struct Border    shineBorder;
	struct Border    shadowBorder;
  
	struct IntuiText  myIText;
	struct TextAttr   myTextAttr;

	long mySHADOWPEN = 1;  /* set default values for pens */
	long mySHINEPEN  = 2;  /* in case can't get info...   */ 

	long myTEXTPEN = 1;
	long myBACKGROUNDPEN = 0;
	
	int startx,starty;
	
	startx = 12;
	starty = 10;

    shadowBorder.LeftEdge   = 0;
    shadowBorder.TopEdge    = 0;
    shadowBorder.FrontPen   = mySHADOWPEN;
    shadowBorder.NextBorder = &shineBorder;
	  
    shineBorder.LeftEdge    = 0 + 1;
    shineBorder.TopEdge     = 0 + 1;
    shineBorder.FrontPen    = mySHINEPEN;
    shineBorder.NextBorder  = NULL;	  

    shadowBorder.BackPen    = shineBorder.BackPen   = 0;
    shadowBorder.DrawMode   = shineBorder.DrawMode  = JAM1;
    shadowBorder.Count      = shineBorder.Count     = 5;
    shadowBorder.XY         = shineBorder.XY        = myBorderData;
	  
	DrawBorder(rp,&shadowBorder,startx,topborder+starty);

	starty +=52;
    shadowBorder.XY         = shineBorder.XY        = myBorderData2;	
//	DrawBorder(rp,&shadowBorder,startx,topborder+starty);	

	starty =10;
	startx +=106;
    shadowBorder.XY         = shineBorder.XY        = myBorderData3;	
	DrawBorder(rp,&shadowBorder,startx,topborder+starty);

	starty +=120;
	startx +=0;
    shadowBorder.XY         = shineBorder.XY        = myBorderData4;	
	DrawBorder(rp,&shadowBorder,startx,topborder+starty);		


	myTextAttr.ta_Name  = font->tf_Message.mn_Node.ln_Name;
    myTextAttr.ta_YSize = font->tf_YSize;
    myTextAttr.ta_Style = font->tf_Style;
    myTextAttr.ta_Flags = font->tf_Flags;
	  
	myIText.FrontPen    = myTEXTPEN;
	myIText.BackPen     = myBACKGROUNDPEN;
    myIText.DrawMode    = JAM2;
    myIText.LeftEdge    = 0;
    myIText.TopEdge     = 0;
    myIText.ITextFont   = &myTextAttr;
    myIText.NextText    = NULL;
	  
	startx = 28;
	starty = 8;
	myIText.IText       = "FPGA Core";
	PrintIText(rp,&myIText,startx,topborder+ starty);
	  
	startx-=8;
	starty += 52;
	myIText.IText       = "FPGA Config";
//	PrintIText(rp,&myIText,startx,topborder+ starty);			  
}



/*
** Function to handle a GADGETUP or GADGETDOWN event.  For GadTools gadgets,
** it is possible to use this function to handle MOUSEMOVEs as well, with
** little or no work.
*/
UWORD handleGadgetEvent(struct Window *win, struct Gadget *gad, UWORD code,
    WORD *slider_level, struct Gadget *my_gads[])
{
switch (gad->GadgetID)
    {
    case BUTTON_FLASH:
		return CMD_FLASH;
        break;
    case BUTTON_INFO:
		return CMD_INFO;
        break;
    case BUTTON_INIT:
		return CMD_INIT;
        break;
    case BUTTON_SAVE:
		return CMD_SAVE;
        break;
    case BUTTON_LOAD:
		return CMD_LOAD;
        break;		
	case BUTTON_QUIT:
		return CMD_QUIT;
        break;
	default:
		return 0;
		break;
    }
}


/*
** Function to handle vanilla keys.
*/
VOID handleVanillaKey(struct Window *win, UWORD code,
    WORD *slider_level, struct Gadget *my_gads[])
{
switch (code)
    {
    case 'v':
        break;
    case 'V':
        break;
    case 'c':
    case 'C':
        break;
    case 'f':
    case 'F':
        break;
    case 's':
    case 'S':
        break;
    case 't':
    case 'T':
        break;
    }
}


/*
** Here is where all the initialization and creation of GadTools gadgets
** take place.  This function requires a pointer to a NULL-initialized
** gadget list pointer.  It returns a pointer to the last created gadget,
** which can be checked for success/failure.
*/
struct Gadget *createAllGadgets(struct Gadget **glistptr, void *vi,
    UWORD topborder, WORD slider_level, struct Gadget *my_gads[])
{
	struct NewGadget ng;
	struct Gadget *gad;

	gad = CreateContext(glistptr);

	ng.ng_LeftEdge   = 32;
	ng.ng_TopEdge    = 20+topborder;
	ng.ng_Width      = 64;
	ng.ng_Height     = 12;
	ng.ng_GadgetText = "_Flash";
	ng.ng_TextAttr   = &Topaz80;
	ng.ng_GadgetID   = BUTTON_FLASH;
	ng.ng_Flags      = 0;
	my_gads[BUTTON_FLASH] = gad = CreateGadget(BUTTON_KIND, gad, &ng,
						GT_Underscore, '_',
						TAG_END);

	ng.ng_LeftEdge   += 0;
	ng.ng_TopEdge    += 16;
	ng.ng_Width      = 64;
	ng.ng_Height     = 12;
	ng.ng_GadgetText = "_Info";
	ng.ng_TextAttr   = &Topaz80;
	ng.ng_GadgetID   = BUTTON_INFO;
	ng.ng_Flags      = 0;
	my_gads[BUTTON_INFO] = gad = CreateGadget(BUTTON_KIND, gad, &ng,
						GT_Underscore, '_',
						TAG_END);					

	ng.ng_LeftEdge   += 0;
	ng.ng_TopEdge    += 36;
	ng.ng_Width      = 64;
	ng.ng_Height     = 12;
	ng.ng_GadgetText = "_Init";
	ng.ng_TextAttr   = &Topaz80;
	ng.ng_GadgetID   = BUTTON_INIT;
	ng.ng_Flags      = 0;
//	my_gads[BUTTON_INIT] = gad = CreateGadget(BUTTON_KIND, gad, &ng,
//						GT_Underscore, '_',
//						TAG_END);	

	ng.ng_LeftEdge   += 0;
	ng.ng_TopEdge    += 16;
	ng.ng_Width      = 64;
	ng.ng_Height     = 12;
	ng.ng_GadgetText = "_Load";
	ng.ng_TextAttr   = &Topaz80;
	ng.ng_GadgetID   = BUTTON_LOAD;
	ng.ng_Flags      = 0;
//	my_gads[BUTTON_LOAD] = gad = CreateGadget(BUTTON_KIND, gad, &ng,
//						GT_Underscore, '_',
//						TAG_END);
						
	ng.ng_LeftEdge   += 0;
	ng.ng_TopEdge    += 16;
	ng.ng_Width      = 64;
	ng.ng_Height     = 12;
	ng.ng_GadgetText = "_Save";
	ng.ng_TextAttr   = &Topaz80;
	ng.ng_GadgetID   = BUTTON_SAVE;
	ng.ng_Flags      = 0;
//	my_gads[BUTTON_SAVE] = gad = CreateGadget(BUTTON_KIND, gad, &ng,
//						GT_Underscore, '_',
//						TAG_END);

	ng.ng_LeftEdge   -= 8;
	ng.ng_TopEdge    += 28;
	ng.ng_Width      = 80;
	ng.ng_Height     = 12;
	ng.ng_GadgetText = "_Quit";
	ng.ng_TextAttr   = &Topaz80;
	ng.ng_GadgetID   = BUTTON_QUIT;
	ng.ng_Flags      = 0;
	my_gads[BUTTON_QUIT] = gad = CreateGadget(BUTTON_KIND, gad, &ng,
						GT_Underscore, '_',
						TAG_END);
						
	return(gad);
}

/*
** Standard message handling loop with GadTools message handling functions
** used (GT_GetIMsg() and GT_ReplyIMsg()).
*/
VOID process_window_events(struct Window *mywin,
    WORD *slider_level, struct Gadget *my_gads[],struct TextFont *font,UWORD topborder)
{
	struct IntuiMessage *imsg;
	ULONG imsgClass;
	UWORD imsgCode;
	struct Gadget *gad;
	struct FileRequester *fr;
	BOOL terminated = FALSE;
	UWORD command;

	struct ConfigDev *myCD;
	struct FileInfoBlock* fib;

	APTR	board_base;
	
	char	filename[1024];
	char	txtbuf[256];

	BPTR 	filehandle;
    long 	filesize;
	
#ifndef DEBUG
	BOOL	board_found = FALSE;
#else
	BOOL	board_found = TRUE;
#endif
	
	UWORD	regval;
	
	UWORD	fpga_date_minute;
	UWORD	fpga_date_hour;
	UWORD	fpga_date_day;
	UWORD	fpga_date_month;
	UWORD	fpga_date_year;
	
	ULONG	pvision_id_high;
	ULONG	pvision_id_low;
	ULONG	fpga_date;
	ULONG	fpga_id_high;
	ULONG	fpga_id_low;
	
	UWORD	clk_pll;
	UWORD	clk_bus;
	UWORD	clk_mem;
	UWORD	clk_pix;

	APTR	cfg_mem;
	APTR	core_mem;
	

	myCD = NULL;
	
	wPrintF(0,"P-Vision Flash Tool V0.2", TRUE,topborder,font,mywin);
	wPrintF(1,"(C)2026 by Oliver Achten", FALSE,topborder,font,mywin);
	
	/* Find AmiGus Card */
	
	board_base = (APTR)PVISION_BASE;
	
	pvision_id_high =  ReadReg32(board_base,PVISION_ID_HIGH);
	pvision_id_low =  ReadReg32(board_base,PVISION_ID_LOW);	
	
	if ((pvision_id_high == 0x502d5649) && (pvision_id_low == 0x53494f4e))
	{
		board_found = TRUE;
		
		fpga_date 		= ReadReg32(board_base,FPGA_DATE);
		
		fpga_date_minute = 	(UWORD)(fpga_date & (ULONG)0x3f);
		fpga_date_hour = 	(UWORD)((fpga_date & (ULONG)0x7c0)>>6);
		fpga_date_day = 	(UWORD)((fpga_date & (ULONG)0xf800)>>11);
		fpga_date_month = 	(UWORD)((fpga_date & (ULONG)0xf0000)>>16);
		fpga_date_year = 	(UWORD)((fpga_date & (ULONG)0xfff00000)>>20);
		
		fpga_id_high 	= ReadReg32(board_base,FPGA_ID_HIGH);
		fpga_id_low 	= ReadReg32(board_base,FPGA_ID_LOW);
		
		clk_pll	= ReadReg16(board_base,PLL_M_NCOUNT) * PVISION_OSC;
		
		regval = (ReadReg16(board_base,BUS_CLK_M_HCOUNT) + ReadReg16(board_base,BUS_CLK_M_LCOUNT));
		if 	(regval != 0)
			clk_bus = clk_pll / regval;
		else
			clk_bus = 0;
		
		regval = (ReadReg16(board_base,MEMC_CLK_M_HCOUNT) + ReadReg16(board_base,MEMC_CLK_M_LCOUNT));
		if 	(regval != 0)
			clk_mem = clk_pll / regval;
		else
			clk_mem = 0;		

		regval = (ReadReg16(board_base,PIX_CLK_M_HCOUNT) + ReadReg16(board_base,PIX_CLK_M_LCOUNT));
		if 	(regval != 0)
			clk_pix = clk_pll / regval;
		else
			clk_pix = 0;		
	}

	board_found = TRUE;

	if (board_found == TRUE)
		wPrintF(3,"P-Vision card found!", FALSE,topborder,font,mywin);
	else
	{
		wPrintF(3,"P-Vision card not found!", FALSE,topborder,font,mywin);
		
		GT_SetGadgetAttrs(my_gads[BUTTON_FLASH], mywin, NULL,
						GA_Disabled, TRUE,
                        TAG_END);
		GT_SetGadgetAttrs(my_gads[BUTTON_INFO], mywin, NULL,
						GA_Disabled, TRUE,
                        TAG_END);				
	}
/*	GT_SetGadgetAttrs(my_gads[BUTTON_INIT], mywin, NULL,
						GA_Disabled, TRUE,
                        TAG_END);
	GT_SetGadgetAttrs(my_gads[BUTTON_LOAD], mywin, NULL,
						GA_Disabled, TRUE,
                        TAG_END);
	GT_SetGadgetAttrs(my_gads[BUTTON_SAVE], mywin, NULL,
						GA_Disabled, TRUE,
                        TAG_END);	
*/
	if (cfg_mem = AllocMem(FLASH_CONFIG_SIZE,MEMF_ANY))
    {
		if (core_mem = AllocMem(FLASH_CORE_SIZE,MEMF_ANY))
		{
		}
		else
		{
			FreeMem(cfg_mem,FLASH_CONFIG_SIZE);
			return;
		}
	}
	else 
	{
		return;
	}	
		
	while (!terminated)
	{
		Wait (1 << mywin->UserPort->mp_SigBit);

		/* GT_GetIMsg() returns an IntuiMessage with more friendly information for
		** complex gadget classes.  Use it wherever you get IntuiMessages where
		** using GadTools gadgets.
		*/
		
		command = 0;
		
		while ((!terminated) &&
			   (imsg = GT_GetIMsg(mywin->UserPort)))
		{
			/* Presuming a gadget, of course, but no harm...
			** Only dereference this value (gad) where the Class specifies
			** that it is a gadget event.
			*/
			gad = (struct Gadget *)imsg->IAddress;

			imsgClass = imsg->Class;
			imsgCode = imsg->Code;
			
			/* Use the toolkit message-replying function here... */
			GT_ReplyIMsg(imsg);

			switch (imsgClass)
				{
				/*  --- WARNING --- WARNING --- WARNING --- WARNING --- WARNING ---
				** GadTools puts the gadget address into IAddress of IDCMP_MOUSEMOVE
				** messages.  This is NOT true for standard Intuition messages,
				** but is an added feature of GadTools.
				*/
				case IDCMP_GADGETDOWN:
				case IDCMP_MOUSEMOVE:
				case IDCMP_GADGETUP:
					command = handleGadgetEvent(mywin, gad, imsgCode, slider_level, my_gads);
					break;
				case IDCMP_VANILLAKEY:
					handleVanillaKey(mywin, imsgCode, slider_level, my_gads);
					break;
				case IDCMP_CLOSEWINDOW:
					terminated = TRUE;
					break;
				case IDCMP_REFRESHWINDOW:
					/* With GadTools, the application must use GT_BeginRefresh()
					** where it would normally have used BeginRefresh()
					*/
					GT_BeginRefresh(mywin);
					GT_EndRefresh(mywin, TRUE);
					drawBorders(mywin->RPort,topborder,font);
					break;
				}
		}
		
		switch (command)
		{
			case CMD_FLASH:
				if (fr = (struct FileRequester *)
					AllocAslRequest(ASL_FileRequest, frtags_flash))
				{		
			
					if (AslRequest(fr, 0L))
					{
						sprintf(filename,"%s", fr->rf_Dir);
						AddPart(filename,fr->rf_File,sizeof(filename));
					}
					else
					{
						printf("No file selected!\n");
						break;
					}
				}
				FreeAslRequest(fr);			

				filesize = 0;				
				fib = AllocDosObject(DOS_FIB, NULL);
				if (!fib)
				{
					break;
				}	

				if (filehandle = Open(filename,MODE_OLDFILE))
				{
					if (!ExamineFH(filehandle,fib))
					{
						FreeDosObject(DOS_FIB,fib);			
						Close(filehandle);
						wPrintF(0,"ERROR: could not open file", TRUE,topborder,font,mywin);
						break;
					}
					filesize = fib->fib_Size;
					FreeDosObject(DOS_FIB,fib);
					
					if (filesize == FLASH_CORE_SIZE)
					{				
						if (Read(filehandle, core_mem, filesize)==-1)	// Preload all memory buffers
						{
							FreeDosObject(DOS_FIB,fib);			
							Close(filehandle);
							wPrintF(0,"ERROR: could not read file", TRUE,topborder,font,mywin);
							break;
						}
						else
						{
							SetAPen(mywin->RPort, 0);
							RectFill(mywin->RPort, 120, topborder+146-14, 120+253, topborder+157-14);	
							
							wPrintF(0,"File loaded successfully!", TRUE,topborder,font,mywin);
							wPrintF(1,"Flashing FPGA core...", FALSE,topborder,font,mywin);	
							
							EraseCoreFlash(board_base);
							ProgramCoreFlash(board_base,core_mem,topborder,mywin);
							
							wPrintF(1,"Flashing FPGA core... done!", FALSE,topborder,font,mywin);
							
//							Delay(60);

//							SetAPen(mywin->RPort, 0);
//							RectFill(mywin->RPort, 120, topborder+146-14, 120+253, topborder+157-14);	

//							wPrintF(2,"Verifying FPGA core...", FALSE,topborder,font,mywin);							
//							if (VerifyCoreFlash(board_base,core_mem,topborder,mywin)==TRUE)
//							{
//								wPrintF(2,"Verifying FPGA core...done!", FALSE,topborder,font,mywin);								
								wPrintF(4,"Power-cycle your machine", FALSE,topborder,font,mywin);	
//							}
//							else
//							{
//								wPrintF(2,"Verifying FPGA core...FAIL!", FALSE,topborder,font,mywin);								
//								wPrintF(4,"Please flash again!", FALSE,topborder,font,mywin);									
//							}
						}
					}
					else
					{
						wPrintF(0,"ERROR: wrong file type", TRUE,topborder,font,mywin);
						break;
					}
				}
				else
				{
					wPrintF(0,"ERROR: could not open file", TRUE,topborder,font,mywin);
				}
				break;
			case CMD_INFO:
				wPrintF(0,"FPGA core date:", TRUE,topborder,font,mywin);
				sprintf(txtbuf,"  %4d-%02d-%02d, %02d:%02d",fpga_date_year,fpga_date_month,fpga_date_day,fpga_date_hour,fpga_date_minute);
				wPrintF(1,txtbuf, FALSE,topborder,font,mywin);
				
				wPrintF(3,"FPGA ID:", FALSE,topborder,font,mywin);
				sprintf(txtbuf,"  0x%lx%lx",fpga_id_high,fpga_id_low);
				wPrintF(4,txtbuf, FALSE,topborder,font,mywin);
				
				wPrintF(6,"FPGA Status:", FALSE,topborder,font,mywin);
				sprintf(txtbuf,"  BUS Clock: %dMHz",clk_bus);
				wPrintF(7,txtbuf, FALSE,topborder,font,mywin);
				sprintf(txtbuf,"  MEM Clock: %dMHz",clk_mem);
				wPrintF(8,txtbuf, FALSE,topborder,font,mywin);
				sprintf(txtbuf,"  PIX Clock: %dMHz",clk_pix);
				wPrintF(9,txtbuf, FALSE,topborder,font,mywin);
				break;
			case CMD_INIT:
				wPrintF(0,"Reset FPGA config data... done!", TRUE,topborder,font,mywin);
				SetAPen(mywin->RPort, 0);
				RectFill(mywin->RPort, 120, topborder+146-14, 120+253, topborder+157-14);	
				initCfgMem(cfg_mem);
				EraseConfigFlash(board_base);
				ProgramConfigFlash(board_base,cfg_mem);
				SetAPen(mywin->RPort, 3);
				RectFill(mywin->RPort, 120, topborder+146-14, 120+253, topborder+157-14);
				break;
			case CMD_LOAD:
				if (fr = (struct FileRequester *)
					AllocAslRequest(ASL_FileRequest, frtags_load))
				{		
			
					if (AslRequest(fr, 0L))
					{
						sprintf(filename,"%s", fr->rf_Dir);
						AddPart(filename,fr->rf_File,sizeof(filename));
					}
					else
					{
						break;
					}
				}
				FreeAslRequest(fr);			

				filesize = 0;				
				fib = AllocDosObject(DOS_FIB, NULL);
				if (!fib)
				{
					break;
				}	

				if (filehandle = Open(filename,MODE_OLDFILE))
				{
					if (!ExamineFH(filehandle,fib))
					{
						FreeDosObject(DOS_FIB,fib);			
						Close(filehandle);
						wPrintF(0,"ERROR: could not open file", TRUE,topborder,font,mywin);
						break;
					}
					filesize = fib->fib_Size;
					FreeDosObject(DOS_FIB,fib);
					
					if (filesize == FLASH_CONFIG_SIZE)
					{				
						if (Read(filehandle, cfg_mem, filesize)==-1)	// Preload all memory buffers
						{
							FreeDosObject(DOS_FIB,fib);			
							Close(filehandle);
							wPrintF(0,"ERROR: could not read file", TRUE,topborder,font,mywin);
							break;
						}
						else
						{
							SetAPen(mywin->RPort, 0);
							RectFill(mywin->RPort, 120, topborder+146-14, 120+253, topborder+157-14);	
							
							wPrintF(0,"File loaded successfully!", TRUE,topborder,font,mywin);
							wPrintF(1,"Flashing FPGA config...", FALSE,topborder,font,mywin);							
							
							EraseConfigFlash(board_base);
							ProgramConfigFlash(board_base,cfg_mem);
							
							SetAPen(mywin->RPort, 3);
							RectFill(mywin->RPort, 120, topborder+146-14, 120+253, topborder+157-14);	
							
							wPrintF(1,"Flashing FPGA config... done!", FALSE,topborder,font,mywin);
							wPrintF(3,"Please reset your machine", FALSE,topborder,font,mywin);							
						}
					}
					else
					{
						wPrintF(0,"ERROR: wrong file type", TRUE,topborder,font,mywin);
						break;
					}
				}
				else
				{
					wPrintF(0,"ERROR: could not open file", TRUE,topborder,font,mywin);
				}
				break;
			case CMD_SAVE:		
				if (fr = (struct FileRequester *)
					AllocAslRequest(ASL_FileRequest, frtags_save))
				{		
			
					if (AslRequest(fr, 0L))
					{
						sprintf(filename,"%s", fr->rf_Dir);
						AddPart(filename,fr->rf_File,sizeof(filename));
					}
					else
					{
						wPrintF(0,"Config save aborted", TRUE,topborder,font,mywin);
						break;
					}
				}
				FreeAslRequest(fr);					
				ReadConfigFlash(board_base,cfg_mem);				
		
				filesize = FLASH_CONFIG_SIZE;				

				if (filehandle = Open(filename,MODE_NEWFILE))
				{
					if (Write(filehandle, cfg_mem, filesize)==-1)	// Preload all memory buffers
					{	
						Close(filehandle);
						wPrintF(0,"ERROR: could not write file", TRUE,topborder,font,mywin);
						break;
					}
					else
					{
						SetAPen(mywin->RPort, 0);
						RectFill(mywin->RPort, 120, topborder+146-14, 120+253, topborder+157-14);	
						
						wPrintF(0,"Config file written", TRUE,topborder,font,mywin);					
					}
					Close(filehandle);
				}
				else
				{
					wPrintF(0,"ERROR: file not written", TRUE,topborder,font,mywin);
					break;
				}	
				break;
			case CMD_QUIT:
				terminated = TRUE;
				break;
		}
	}
	
	FreeMem(cfg_mem,FLASH_CONFIG_SIZE);
	FreeMem(core_mem,FLASH_CORE_SIZE);
}

/*
** Prepare for using GadTools, set up gadgets and open window.
** Clean up and when done or on error.
*/
VOID gadtoolsWindow(VOID)
{
struct TextFont *font;
struct Screen   *mysc;
struct Window   *mywin;
struct Gadget   *glist, *my_gads[6];
void            *vi;
WORD            slider_level = 5;
UWORD           topborder;

/* Open topaz 8 font, so we can be sure it's openable
** when we later set ng_TextAttr to &Topaz80:
*/
if (NULL == (font = OpenFont(&Topaz80)))
    errorMessage( "Failed to open Topaz 80");
else
    {
    if (NULL == (mysc = LockPubScreen(NULL)))
        errorMessage( "Couldn't lock default public screen");
    else
        {
        if (NULL == (vi = GetVisualInfo(mysc, TAG_END)))
            errorMessage( "GetVisualInfo() failed");
        else
            {
            /* Here is how we can figure out ahead of time how tall the  */
            /* window's title bar will be:                               */
            topborder = mysc->WBorTop + (mysc->Font->ta_YSize + 1);

            if (NULL == createAllGadgets(&glist, vi, topborder,
                                         slider_level, my_gads))
                errorMessage( "createAllGadgets() failed");
            else
                {
                if (NULL == (mywin = OpenWindowTags(NULL,
                        WA_Title,     "P-Vision Flash Tool",
                        WA_Gadgets,   glist,      WA_AutoAdjust,    TRUE,
                        WA_Width,       392,      WA_MinWidth,        50,
                        WA_InnerHeight, 156,      WA_MinHeight,       50,
                        WA_DragBar,    TRUE,      WA_DepthGadget,   TRUE,
                        WA_Activate,   TRUE,      WA_CloseGadget,   TRUE,
                        WA_SizeGadget, FALSE,      WA_SmartRefresh, TRUE,
                        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW |
                            IDCMP_VANILLAKEY | SLIDERIDCMP | STRINGIDCMP |
                            BUTTONIDCMP,
                        WA_PubScreen, mysc,
                        TAG_END)))
                    errorMessage( "OpenWindow() failed");
                else
                    {
                    /* After window is open, gadgets must be refreshed with a
                    ** call to the GadTools refresh window function.
                    */
					drawBorders(mywin->RPort,topborder,font);
					
                    GT_RefreshWindow(mywin, NULL);

                    process_window_events(mywin, &slider_level, my_gads,font,topborder);

                    CloseWindow(mywin);
                    }
                }
            /* FreeGadgets() even if createAllGadgets() fails, as some
            ** of the gadgets may have been created...If glist is NULL
            ** then FreeGadgets() will do nothing.
            */
            FreeGadgets(glist);
            FreeVisualInfo(vi);
            }
        UnlockPubScreen(NULL, mysc);
        }
    CloseFont(font);
    }
}


/*
** Open all libraries and run.  Clean up when finished or on error..
*/
void main(void)
{
if (NULL == (IntuitionBase = OpenLibrary("intuition.library", 37)))
    errorMessage( "Requires V37 intuition.library");
else
	{
	if (NULL == (ExpansionBase = OpenLibrary("expansion.library", 37)))
		errorMessage( "Requires V37 expansion.library");
	else	
		{
		if (NULL == (GfxBase = OpenLibrary("graphics.library", 37)))
			errorMessage( "Requires V37 graphics.library");
		else
			{
			if (NULL == (AslBase = OpenLibrary("asl.library", 37)))
				errorMessage( "Requires V37 asl.library");
			else			
				{	
				if (NULL == (GadToolsBase = OpenLibrary("gadtools.library", 37)))
					errorMessage( "Requires V37 gadtools.library");
				else			
					{
					gadtoolsWindow();

					CloseLibrary(GadToolsBase);
					}
				CloseLibrary(GfxBase);
				}
			CloseLibrary(AslBase);
			}
		CloseLibrary(ExpansionBase);
		}
	CloseLibrary(IntuitionBase);	
	}
}

