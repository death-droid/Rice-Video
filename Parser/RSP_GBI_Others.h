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
uint16 ConvertYUVtoR5G5B5X1(int y, int u, int v)
{
	float r = y + (1.370705f * (v-128));
	float g = y - (0.698001f * (v-128)) - (0.337633f * (u-128));
	float b = y + (1.732446f * (u-128));
	r *= 0.125f;
	g *= 0.125f;
	b *= 0.125f;

	//clipping the result
	if (r > 32) r = 32;
	if (g > 32) g = 32;
	if (b > 32) b = 32;
	if (r < 0) r = 0;
	if (g < 0) g = 0;
	if (b < 0) b = 0;

	uint16 c = (uint16)(((uint16)(r) << 11) |
		((uint16)(g) << 6) |
		((uint16)(b) << 1) | 1);
	return c;
}

void TexRectToN64FrameBuffer_YUV_16b(uint32 x0, uint32 y0, uint32 width, uint32 height)
{
	// Convert YUV image at TImg and Copy the texture into the N64 RDRAM framebuffer memory

	uint32 n64CIaddr = g_CI.dwAddr;
	uint32 n64CIwidth = g_CI.dwWidth;

	for (uint32 y = 0; y < height; y++)
	{
		uint32* pN64Src = (uint32*)(g_pu8RamBase+(g_TI.dwAddr&(g_dwRamSize-1)))+y*(g_TI.dwWidth>>1);
		uint16* pN64Dst = (uint16*)(g_pu8RamBase+(n64CIaddr&(g_dwRamSize-1)))+(y+y0)*n64CIwidth;

		for (uint32 x = 0; x < width; x+=2)
		{
			uint32 val = *pN64Src++;
			int y0 = (uint8)val&0xFF;
			int v  = (uint8)(val>>8)&0xFF;
			int y1 = (uint8)(val>>16)&0xFF;
			int u  = (uint8)(val>>24)&0xFF;

			pN64Dst[x+x0] = ConvertYUVtoR5G5B5X1(y0,u,v);
			pN64Dst[x+x0+1] = ConvertYUVtoR5G5B5X1(y1,u,v);
		}
	}
}

extern uObjMtxReal gObjMtxReal;
void DLParser_OgreBatter64BG(MicroCodeCommand command)
{
	uint32 dwAddr = RSPSegmentAddr((command.inst.cmd1));
	uObjTxSprite *ptr = (uObjTxSprite*)(g_pu8RamBase+dwAddr);
	//CRender::g_pRender->LoadObjSprite(*ptr,true);
	PrepareTextures();

	CTexture *ptexture = g_textures[0].m_pCTexture;
	TexRectToN64FrameBuffer_16b( (uint32)gObjMtxReal.X, (uint32)gObjMtxReal.Y, ptexture->m_dwWidth, ptexture->m_dwHeight, gRSP.curTile);

#ifdef _DEBUG
	CRender::g_pRender->DrawSpriteR(*ptr, false);

	DEBUGGER_PAUSE_AT_COND_AND_DUMP_COUNT_N((pauseAtNext && (eventToPause==NEXT_OBJ_TXT_CMD|| eventToPause==NEXT_FLUSH_TRI)),
	{
		DebuggerAppendMsg("OgreBatter 64 BG: Addr=%08X\n", dwAddr);
	}
	);
#endif
}

void DLParser_Bomberman2TextRect(MicroCodeCommand command)
{
	// Bomberman 64 - The Second Attack! (U) [!]
	// The 0x02 cmd, list a TexRect cmd

	if( options.enableHackForGames == HACK_FOR_OGRE_BATTLE && gRDP.tiles[7].dwFormat == TXT_FMT_YUV )
	{
		TexRectToN64FrameBuffer_YUV_16b( (uint32)gObjMtxReal.X, (uint32)gObjMtxReal.Y, 16, 16);
		//DLParser_OgreBatter64BG((command.inst.cmd0), (command.inst.cmd1));
		return;
	}

	uint32 dwAddr = RSPSegmentAddr((command.inst.cmd1));
	uObjSprite *info = (uObjSprite*)(g_pu8RamBase+dwAddr);

	uint32 dwTile	= gRSP.curTile;

	PrepareTextures();
	
	//CRender::g_pRender->SetCombinerAndBlender();

	uObjTxSprite drawinfo;
	memcpy( &(drawinfo.sprite), info, sizeof(uObjSprite));
	CRender::g_pRender->DrawSpriteR(drawinfo, false, dwTile, 0, 0, drawinfo.sprite.imageW/32, drawinfo.sprite.imageH/32);

	DEBUGGER_PAUSE_AT_COND_AND_DUMP_COUNT_N((pauseAtNext && (eventToPause==NEXT_TRIANGLE|| eventToPause==NEXT_FLUSH_TRI)),
		{
			DebuggerAppendMsg("Bomberman 64 - TextRect: Addr=%08X\n", dwAddr);
			dwAddr &= (g_dwRamSize-1);
			DebuggerAppendMsg("%08X-%08X-%08X-%08X-%08X-%08X\n", RDRAM_UWORD(dwAddr), RDRAM_UWORD(dwAddr+4),
				RDRAM_UWORD(dwAddr+8), RDRAM_UWORD(dwAddr+12), RDRAM_UWORD(dwAddr+16), RDRAM_UWORD(dwAddr+20) );
		}
	);
}