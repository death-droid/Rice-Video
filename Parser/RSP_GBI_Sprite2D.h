/*
Copyright (C) 2002-2009 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Sprite2D Ucodes


Sprite2DInfo g_Sprite2DInfo;
uint32 g_SavedUcode=1;
 
void RSP_GBI_Sprite2DBase(MicroCodeCommand command)
{
	u32 address;

	u32 pc = gDlistStack[gDlistStackPointer].pc;
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	// Try to execute as many sprite2d ucodes as possible, I seen chains over 200! in FB
	// NB Glover calls RDP Sync before draw for the sky.. so checks were added
	do
	{
		address = RSPSegmentAddr(command.inst.cmd1) & (g_dwRamSize-1);
		g_Sprite2DInfo.spritePtr = (SpriteStruct *)(g_ps8RamBase + address);

		// Fetch Sprite2D Flip
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		if(command.inst.cmd != G_GBI1_SPRITE2D_SCALEFLIP)
		{
			pc += 8;
			break;
		}
		RSP_GBI1_Sprite2DScaleFlip( command );

		// Fetch Sprite2D Draw
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		if(command.inst.cmd != G_GBI1_SPRITE2D_DRAW)
		{
			pc += 16;	//We have executed atleast 2 instructions at this point
			break;
		}
		RSP_GBI1_Sprite2DDraw( command );

		// Fetch Sprite2D Base
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		pc += 24;
	}while(command.inst.cmd == G_GBI1_SPRITE2D_BASE);

	gDlistStack[gDlistStackPointer].pc = pc-8;
	DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_SPRITE_2D, {DebuggerAppendMsg("Pause after Sprite2DBase: Addr=%08X\n", address);});
}

typedef struct{
	uint32 address; 
	uint32 tlut;

	short width;
	short Stride;

	char  size;
	char  format;
	short height;

	short scaleY;
	short scaleX;

	short imageX;
	char  dummy1[2]; 

	short px;
	short imageY;

	char  dummy2[2]; 
	short py;

} PuzzleMasterSprite;

void RSP_GBI_Sprite2D_PuzzleMaster64(MicroCodeCommand command)
{
	uint32 dwAddr = RSPSegmentAddr((command.inst.cmd1));
	dwAddr &= (g_dwRamSize-1);

	g_Sprite2DInfo.spritePtr = (SpriteStruct *)(g_ps8RamBase+dwAddr);

	g_Sprite2DInfo.flipX = 0;
	g_Sprite2DInfo.flipY = 0;
	g_Sprite2DInfo.px = 0;
	g_Sprite2DInfo.py = 0;

	SpriteStruct tempInfo;
	memcpy(&tempInfo, g_Sprite2DInfo.spritePtr, sizeof(SpriteStruct));
	PuzzleMasterSprite info;
	memcpy(&info, g_Sprite2DInfo.spritePtr, sizeof(PuzzleMasterSprite));

	g_Sprite2DInfo.px = info.px>>2;
	g_Sprite2DInfo.py = info.py>>2;
	g_Sprite2DInfo.scaleX = info.scaleX / 1024.0f;
	g_Sprite2DInfo.scaleY = info.scaleY / 1024.0f;
	tempInfo.imageX = info.imageX;
	tempInfo.imageY = info.imageY;
	g_Sprite2DInfo.spritePtr = &tempInfo;

	CRender::g_pRender->DrawSprite2D(g_Sprite2DInfo, 1);
	DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_SPRITE_2D, {DebuggerAppendMsg("Pause after Sprite2DBase: Addr=%08X\n", dwAddr);});
}


void RSP_GBI1_Sprite2DDraw(MicroCodeCommand command)
{
	// This ucode is shared by PopMtx and gSPSprite2DDraw
	g_Sprite2DInfo.px = (short)(((command.inst.cmd1)>>16)&0xFFFF)/4;
	g_Sprite2DInfo.py = (short)((command.inst.cmd1)&0xFFFF)/4;

	//RSP_RDP_NOIMPL("gSPSprite2DDraw is not implemented", (command.inst.cmd0), (command.inst.cmd1));
	CRender::g_pRender->DrawSprite2D(g_Sprite2DInfo, 1);
	DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_SPRITE_2D, 
		{DebuggerAppendMsg("Pause after Sprite2DDraw at (%d, %d)\n", g_Sprite2DInfo.px, g_Sprite2DInfo.py);});

//	LoadedUcodeMap[RSP_SPRITE2D_SCALEFLIP] = &RSP_GBI1_CullDL;
//	LoadedUcodeMap[RSP_SPRITE2D_DRAW] = &RSP_GBI1_PopMtx;
//	LoadedUcodeMap[RSP_SPRITE2D_BASE] = &RSP_GBI1_Sprite2DBase;

}

void RSP_GBI0_Sprite2DDraw(MicroCodeCommand command)
{
	// This ucode is shared by PopMtx and gSPSprite2DDraw
	g_Sprite2DInfo.px = (short)(((command.inst.cmd1)>>16)&0xFFFF)/4;
	g_Sprite2DInfo.py = (short)((command.inst.cmd1)&0xFFFF)/4;

	//RSP_RDP_NOIMPL("gSPSprite2DDraw is not implemented", (command.inst.cmd0), (command.inst.cmd1));
	CRender::g_pRender->DrawSprite2D(g_Sprite2DInfo, 0);
	DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_SPRITE_2D, {TRACE0("Pause after Sprite2DDraw\n");});
}


void RSP_GBI1_Sprite2DScaleFlip(MicroCodeCommand command)
{

	g_Sprite2DInfo.scaleX = (((command.inst.cmd1)>>16)&0xFFFF)/1024.0f;
	g_Sprite2DInfo.scaleY = ( (command.inst.cmd1)     &0xFFFF)/1024.0f;

	if( ((command.inst.cmd1)&0xFFFF) < 0x100 )
	{
		g_Sprite2DInfo.scaleY = g_Sprite2DInfo.scaleX;
	}

	g_Sprite2DInfo.flipX = (uint8)(((command.inst.cmd0)>>8)&0xFF);
	g_Sprite2DInfo.flipY = (uint8)( (command.inst.cmd0)    &0xFF);
	//RSP_RDP_NOIMPL("RSP_SPRITE2D_SCALEFLIP is not implemented", (command.inst.cmd0), (command.inst.cmd1));
	DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_SPRITE_2D, 
		{DebuggerAppendMsg("Pause after Sprite2DScaleFlip, Flip (%d,%d), Scale (%f, %f)\n", g_Sprite2DInfo.flipX, g_Sprite2DInfo.flipY,
			g_Sprite2DInfo.scaleX, g_Sprite2DInfo.scaleY);});
}


//remove us
void RSP_GBI1_Sprite2DBase(MicroCodeCommand command)
{
	RSP_GBI_Sprite2DBase(command);
}


//remove us
void RSP_GBI0_Sprite2DBase(MicroCodeCommand command)
{
	//Weired, this ucode 0 game is using ucode 1, but sprite2D cmd is working differently from
	//normal ucode1 sprite2D game

	TRACE0("Ucode 0 game is using Sprite2D, and using ucode 1 codes, create a new ucode for me");

	RSP_GBI_Sprite2DBase(command);
}

