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

struct Sprite2DStruct
{
	uint32 address;
	uint32 tlut;

	short width;
	short Stride;

	char  size;
	char  format;
	short height;

	short imageY;
	short imageX;

	char	dummy[4];
};			//Converted Sprint struct in Intel format

struct Sprite2DInfo{
	float scaleX;
	float scaleY;

	uint8  flipX;
	uint8  flipY;
};

void RSP_Sprite2DDraw(MicroCodeCommand command, Sprite2DInfo &info, Sprite2DStruct *sprite)
{
	// This ucode is shared by PopMtx and gSPSprite2DDraw
	if (!status.bCIBufferIsRendered) g_pFrameBufferManager->ActiveTextureBuffer();

	if (status.bHandleN64RenderTexture)
	{
		g_pRenderTextureInfo->maxUsedHeight = g_pRenderTextureInfo->N64Height;
		status.bFrameBufferIsDrawn = true;
		status.bFrameBufferDrawnByTriangles = true;
	}

	TxtrInfo gti;

	gti.Format = sprite->format;
	gti.Size = sprite->size;

	gti.Address = RSPSegmentAddr(sprite->address);

	gti.PalAddress = (uintptr_t)(g_pu8RamBase+RSPSegmentAddr(sprite->tlut));

	gti.WidthToCreate  = sprite->width;
	gti.HeightToCreate = sprite->height;
	gti.LeftToLoad	   = sprite->imageX;
	gti.TopToLoad	   = sprite->imageY;
	gti.Pitch		   = sprite->Stride << sprite->size >> 1;

	gti.HeightToLoad   = gti.HeightToCreate;
	gti.WidthToLoad    = gti.WidthToCreate;

	gti.TLutFmt = TLUT_FMT_RGBA16;	//RGBA16
	gti.Palette = 0;
	gti.bSwapped = FALSE;
	
	gti.pPhysicalAddress = (g_pu8RamBase) + gti.Address;
	gti.tileNo = -1;

	CRender::GetRender()->SetCurrentTexture(0, gTextureManager.GetTexture(&gti, false));

	short px = (short)(((command.inst.cmd1) >> 16) & 0xFFFF) / 4;
	short py = (short)((command.inst.cmd1) & 0xFFFF) / 4;
	unsigned short pw = (unsigned short)(sprite->width / info.scaleX);
	unsigned short ph = (unsigned short)(sprite->height / info.scaleY);

	s32 frameX = px;
	s32 frameY = py;
	s32 frameW = px + pw;
	s32 frameH = py + ph;

	if (info.flipX)
		std::swap(frameX, frameW);

	if (info.flipY)
		std::swap(frameY, frameH);


	float t1, s1 = 0;

	t1 = sprite->width / g_textures[0].m_fTexWidth;
	s1 = sprite->height / g_textures[0].m_fTexHeight;

	CRender::GetRender()->SetCombinerAndBlender();
	CRender::GetRender()->SetAddressUAllStages(0, D3DTADDRESS_CLAMP);
	CRender::GetRender()->SetAddressVAllStages(0, D3DTADDRESS_CLAMP);

	float depth = (gRDP.otherMode.depth_source == 1) ? gRDP.fPrimitiveDepth : 0;
	CRender::GetRender()->DrawSimple2DTexture((float)frameX, (float)frameY, (float)frameW, (float)frameH, 0, 0, t1, s1, 0xffffffff, depth, 1.0f);

}

void RSP_Sprite2DScaleFlip(MicroCodeCommand command, Sprite2DInfo *info)
{

	info->scaleX = (((command.inst.cmd1) >> 16) & 0xFFFF) / 1024.0f;
	info->scaleY = ((command.inst.cmd1)			& 0xFFFF) / 1024.0f;

	info->flipX = (uint8)(((command.inst.cmd0) >> 8) & 0xFF);
	info->flipY = (uint8)((command.inst.cmd0)		 & 0xFF);

	DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_SPRITE_2D,
	{ DebuggerAppendMsg("Pause after Sprite2DScaleFlip, Flip (%d,%d), Scale (%f, %f)\n", info->flipX, info->flipY,
	info->scaleX, info->scaleY); });
}

// Sprite2D Ucodes
void RSP_GBI_Sprite2DBase(MicroCodeCommand command)
{
	u32 address;
	Sprite2DInfo info;
	Sprite2DStruct *sprite;

	u32 pc = gDlistStack.address[gDlistStackPointer];
	u32 * pCmdBase = (u32 *)(g_pu8RamBase + pc);

	// Try to execute as many sprite2d ucodes as possible, I seen chains over 200! in FB
	// NB Glover calls RDP Sync before draw for the sky.. so checks were added
	do
	{
		address = RSPSegmentAddr(command.inst.cmd1) & (g_dwRamSize-1);
		sprite = (Sprite2DStruct *)(g_ps8RamBase + address);

		// Fetch Sprite2D Flip
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		if(command.inst.cmd != G_GBI1_SPRITE2D_SCALEFLIP)
		{
			pc += 8;
			break;
		}
		RSP_Sprite2DScaleFlip( command, &info);

		// Fetch Sprite2D Draw
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		if(command.inst.cmd != G_GBI1_SPRITE2D_DRAW)
		{
			pc += 16;	//We have executed atleast 2 instructions at this point
			break;
		}
		RSP_Sprite2DDraw( command, info, sprite);

		// Fetch Sprite2D Base
		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		pc += 24;
	}while(command.inst.cmd == G_GBI1_SPRITE2D_BASE);

	gDlistStack.address[gDlistStackPointer] = pc-8;
	DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_SPRITE_2D, {DebuggerAppendMsg("Pause after Sprite2DBase: Addr=%08X\n", address);});
}