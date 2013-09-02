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

#include "..\stdafx.h"
// header for loading hires textures
void LoadHiresTexture( TxtrCacheEntry &entry );

extern uObjMtxReal gObjMtxReal;
extern Matrix gD3DMtxReal;
extern Matrix gD3DObjOffset;

//========================================================================

void CRender::LoadFrameBuffer(bool useVIreg, uint32 left, uint32 top, uint32 width, uint32 height)
{
	uint32 VIwidth = *g_GraphicsInfo.VI_WIDTH_REG;

	TxtrInfo gti;

	gti.clampS = gti.clampT = 0;
	gti.maskS = gti.maskT = gti.mirrorS = gti.mirrorT = 0;
	gti.TLutFmt	= TLUT_FMT_RGBA16;	//RGBA16
	gti.bSwapped	= FALSE;
	gti.Palette	= 0;

	if( useVIreg && *g_GraphicsInfo.VI_ORIGIN_REG > VIwidth*2 )
	{
		gti.Format	= 0;
		gti.Size	= 2;

		gti.Address	= (*g_GraphicsInfo.VI_ORIGIN_REG & (g_dwRamSize-1) ) - VIwidth*2;
		gti.LeftToLoad	= 0;
		gti.TopToLoad	= 0;

		gti.PalAddress = (uint32)(&g_wRDPTlut[0]);

		gti.WidthToCreate	= windowSetting.uViWidth;
		gti.HeightToCreate	= windowSetting.uViHeight;

		if( gti.WidthToCreate == 0 || gti.HeightToCreate == 0 )
		{
			TRACE0("Loading frame buffer: size = 0 x 0");
			return;
		}

		gti.Pitch	= VIwidth << gti.Size >> 1;
	}
	else
	{
		gti.Format	= g_CI.dwFormat;
		gti.Size	= g_CI.dwSize;
		gti.PalAddress = (uint32)(&g_wRDPTlut[0]);

		gti.Address	= RSPSegmentAddr(g_CI.dwAddr);

		if( width == 0 || height == 0 )
		{
			gti.LeftToLoad		= 0;
			gti.TopToLoad		= 0;

			gti.WidthToCreate		= g_CI.dwWidth;
			gti.HeightToCreate		= g_CI.dwWidth*3/4;
		}
		else
		{
			gti.LeftToLoad		= left;
			gti.TopToLoad		= top;

			gti.WidthToCreate		= width;
			gti.HeightToCreate		= height;
		}

		if( gti.Size == TXT_SIZE_4b )
		{
			gti.Pitch	= g_CI.dwWidth >> 1;
		}
		else
		{
			gti.Pitch	= g_CI.dwWidth << (gti.Size-1);
		}
	}


	if( gti.Address + gti.Pitch*gti.HeightToCreate > g_dwRamSize )
	{
		TRACE0("Skip frame buffer loading, memory out of bound");
		return;
	}

#ifdef _DEBUG
	if( pauseAtNext )
	{
		DebuggerAppendMsg("Load Frame Buffer Imag at: %08X, (%d, %d) - (%d, %d)", gti.Address,
			gti.LeftToLoad, gti.TopToLoad, gti.WidthToCreate, gti.HeightToCreate );
	}
#endif


	gti.HeightToLoad = gti.HeightToCreate;
	gti.WidthToLoad = gti.WidthToCreate;

	gti.pPhysicalAddress = ((uint8*)g_pu32RamBase)+gti.Address;
	gti.tileNo = -1;
	TxtrCacheEntry *pEntry = gTextureManager.GetTexture(&gti, false, false);
	if( pEntry ) SetCurrentTexture( 0, pEntry->pTexture, pEntry->ti.WidthToCreate, pEntry->ti.HeightToCreate, pEntry);
}

void CRender::LoadTextureFromMemory(void *buf, uint32 left, uint32 top, uint32 width, uint32 height, uint32 pitch, uint32 format)
{
	TxtrInfo gti;
	gti.Format	= g_CI.dwFormat;
	gti.Size	= g_CI.dwSize;
	gti.Palette	= 0;
	gti.TLutFmt	= TLUT_FMT_RGBA16;	//RGBA16
	gti.PalAddress = 0;
	gti.bSwapped	= FALSE;
	gti.Address	= 0;
	gti.LeftToLoad		= 0;
	gti.TopToLoad		= 0;
	gti.WidthToCreate		= width;
	gti.HeightToCreate		= height;

	gti.Pitch	= pitch;

	gti.HeightToLoad = height;
	gti.WidthToLoad = width;
	gti.pPhysicalAddress = (uint8*)buf;

	gti.tileNo = -1;
	TxtrCacheEntry *pEntry = gTextureManager.GetTexture(&gti, false);
	//Upto here, the texture is loaded wrong because the format is wrong

	DrawInfo info;	
	if( pEntry->pTexture->StartUpdate(&info) )
	{
		for( uint32 i=0; i<height; i++)
		{
			uint32 *psrc = (uint32*)((uint8*)buf+pitch*(i+top))+left;
			uint32 *pdst = (uint32*)((uint8*)info.lpSurface+i*info.lPitch);
			for( uint32 j=0; j<width; j++)
			{
				pdst[j]=psrc[j];
			}
		}
		pEntry->pTexture->EndUpdate(&info);
	}
	SetCurrentTexture( 0, pEntry->pTexture,	width, height, pEntry);
}


// implementation of ucode with same name (e.g. opcode 0x0A of ucode map 5)
// this is an high-level implementation for loading static backgrounds
// It is rarely used, but Zelda, a quite popular game is using it.
void CRender::LoadObjBGCopy(uObjBg &info)
{
	TxtrInfo gti;
	gti.Format		= info.imageFmt;
	gti.Size		= info.imageSiz;
	gti.Address		= RSPSegmentAddr(info.imagePtr);
	gti.LeftToLoad	= 0;
	gti.TopToLoad	= 0;
	gti.Palette		= info.imagePal;

	gti.PalAddress	= (uint32)(&g_wRDPTlut[0]);
	gti.bSwapped	= FALSE;
	gti.TLutFmt		= TLUT_FMT_RGBA16;	//RGBA16

	gti.WidthToCreate	= info.imageW/4;
	gti.HeightToCreate	= info.imageH/4;

	if( options.bEnableHacks )
	{
		if( g_CI.dwWidth == 0x200 && gti.Format == g_CI.dwFormat && gti.Size == g_CI.dwSize &&
			gti.WidthToCreate == 0x200 )
		{
			// Hack for RE2
			uint32 w = *g_GraphicsInfo.VI_WIDTH_REG & 0xFFF;
			gti.HeightToCreate = (gti.WidthToCreate*gti.HeightToCreate)/w;
			gti.WidthToCreate = w;
		}
	}

	gti.Pitch	= gti.WidthToCreate << gti.Size >> 1;
	gti.Pitch	= (gti.Pitch>>3)<<3;	// Align to 8 bytes

	if( gti.Address + gti.Pitch*gti.HeightToCreate > g_dwRamSize )
	{
		TRACE0("Skip BG copy loading, memory out of bound");
		return;
	}

	gti.HeightToLoad = gti.HeightToCreate;
	gti.WidthToLoad = gti.WidthToCreate;
	gti.pPhysicalAddress = ((uint8*)g_pu32RamBase)+gti.Address;
	gti.tileNo = -1;
	// get the original texture
	TxtrCacheEntry *pEntry = gTextureManager.GetTexture(&gti, false);
	// check if a hires has been enabled and not yet loaded
	if( options.bLoadHiResTextures && (pEntry->pEnhancedTexture == NULL || pEntry->dwEnhancementFlag < TEXTURE_EXTERNAL ) )
	{
		// try to load hires replacement
		LoadHiresTexture(*pEntry);
	}
	
	// and push it to texture memory
	SetCurrentTexture(0,pEntry);

	DEBUGGER_IF_DUMP((pauseAtNext && (eventToPause == NEXT_OBJ_TXT_CMD||eventToPause == NEXT_FLUSH_TRI||eventToPause == NEXT_OBJ_BG)),
	{
		TRACE0("Load Obj BG Copy:\n");
		DebuggerAppendMsg("Addr=0x%08X, W=%d, H=%d, Left=%d, Top=%d\n", 
			gti.Address, gti.WidthToCreate, gti.HeightToCreate, gti.LeftToLoad, gti.TopToLoad);
		DebuggerAppendMsg("Fmt=%s-%db, Pal=%d\n",
			pszImgFormat[gti.Format], pnImgSize[gti.Size], gti.Palette);
	}
	);
}

void CRender::LoadTxtrBufIntoTexture(void)
{
	TxtrInfo gti;

	gti.Format	= g_pRenderTextureInfo->CI_Info.dwFormat;
	gti.Size	= g_pRenderTextureInfo->CI_Info.dwSize;

	gti.Address	= RSPSegmentAddr(g_pRenderTextureInfo->CI_Info.dwAddr);
	gti.LeftToLoad		= 0;
	gti.TopToLoad		= 0;
	gti.Palette	= 0;

	gti.PalAddress = (uint32)(&g_wRDPTlut[0]);
	gti.bSwapped	= FALSE;

	gti.WidthToCreate		= g_pRenderTextureInfo->N64Width;
	gti.HeightToCreate		= g_pRenderTextureInfo->N64Height;
	gti.TLutFmt	= TLUT_FMT_RGBA16;	//RGBA16

	gti.Pitch	= gti.WidthToCreate << (gti.Size-1);

	gti.HeightToLoad = gti.HeightToCreate;
	gti.WidthToLoad = gti.WidthToCreate;
	gti.pPhysicalAddress = ((uint8*)g_pu32RamBase)+gti.Address;
	gti.tileNo = -1;
	TxtrCacheEntry *pEntry = gTextureManager.GetTexture(&gti, false);
	SetCurrentTexture(0,pEntry);
}

void CRender::LoadSprite2D(Sprite2DInfo &info, uint32 ucode)
{
	TxtrInfo gti;

	gti.Format	= info.spritePtr->format;
	gti.Size	= info.spritePtr->size;

	gti.Address	= RSPSegmentAddr(info.spritePtr->address);
	gti.Palette	= 0;
	gti.PalAddress = (uint32)(g_pu8RamBase+RSPSegmentAddr(info.spritePtr->tlut));

	if( options.enableHackForGames == HACK_FOR_NITRO )
	{
		gti.WidthToCreate	= uint32(info.spritePtr->width/info.scaleX);
		gti.HeightToCreate	= uint32(info.spritePtr->height/info.scaleY);
		gti.LeftToLoad		= uint32(info.spritePtr->imageX/info.scaleX);
		gti.TopToLoad		= uint32(info.spritePtr->imageY/info.scaleY);
		gti.Pitch	= info.spritePtr->Stride << gti.Size >> 1;
		gti.Pitch	= uint32(gti.Pitch*info.scaleY);
	}
	else
	{
		gti.WidthToCreate	= info.spritePtr->width;
		gti.HeightToCreate	= info.spritePtr->height;
		gti.LeftToLoad		= info.spritePtr->imageX;
		gti.TopToLoad		= info.spritePtr->imageY;
		gti.Pitch	= info.spritePtr->Stride << gti.Size >> 1;
	}

	if( gti.Address + gti.Pitch*gti.HeightToCreate > g_dwRamSize )
	{
		TRACE0("Skip Sprite image decompress, memory out of bound");
		return;
	}

	gti.HeightToLoad = gti.HeightToCreate;
	gti.WidthToLoad = gti.WidthToCreate;

	gti.TLutFmt		= TLUT_FMT_RGBA16;	//RGBA16
	gti.bSwapped	= FALSE;

	gti.pPhysicalAddress = ((uint8*)g_pu32RamBase)+gti.Address;
	gti.tileNo = -1;
	TxtrCacheEntry *pEntry = gTextureManager.GetTexture(&gti, false);
	SetCurrentTexture(0,pEntry);

	DEBUGGER_IF_DUMP((pauseAtNext && (eventToPause == NEXT_OBJ_TXT_CMD||eventToPause == NEXT_FLUSH_TRI||eventToPause == NEXT_SPRITE_2D)),
	{
		TRACE0("Load Sprite 2D\n");
		DebuggerAppendMsg("Addr=0x%08X, W=%d, H=%d, Left=%d, Top=%d\n", 
			gti.Address, gti.WidthToCreate, gti.HeightToCreate, gti.LeftToLoad, gti.TopToLoad);
		DebuggerAppendMsg("Fmt=%s-%db, Pal=%d, Pitch=%d\n",
			pszImgFormat[gti.Format], pnImgSize[gti.Size], gti.Palette, gti.Pitch);
	}
	);
}


void CRender::DrawSprite2D(Sprite2DInfo &info, uint32 ucode)
{
	if( !status.bCIBufferIsRendered ) g_pFrameBufferManager->ActiveTextureBuffer();

	if( status.bHandleN64RenderTexture )
	{
		g_pRenderTextureInfo->maxUsedHeight = g_pRenderTextureInfo->N64Height;
		status.bFrameBufferIsDrawn = true;
		status.bFrameBufferDrawnByTriangles = true;
	}
	LoadSprite2D(info, ucode);

	info.scaleX = 1/info.scaleX;
	info.scaleY = 1/info.scaleY;

	int x0, y0, x1, y1;
	float t0, s0, t1, s1;
	x0 = info.px;
    y0 = info.py;
	x1 = info.px + int(info.spritePtr->width*info.scaleX);
	y1 = info.py + int(info.spritePtr->height*info.scaleY);
	
	if( info.flipX )
		std::swap(x0, x1);

	if( info.flipY )
		std::swap(y0, y1);

	t0 = s0 = 0;
	if( options.enableHackForGames == HACK_FOR_NITRO )
	{
		t1 = info.spritePtr->width*info.scaleX/g_textures[0].m_fTexWidth;
		s1 = info.spritePtr->height*info.scaleY/g_textures[0].m_fTexHeight;
	}
	else
	{
		t1 = info.spritePtr->width/g_textures[0].m_fTexWidth;
		s1 = info.spritePtr->height/g_textures[0].m_fTexHeight;
	}

	SetCombinerAndBlender();
	SetAddressUAllStages( 0, D3DTADDRESS_CLAMP );
	SetAddressVAllStages( 0, D3DTADDRESS_CLAMP );

	D3DCOLOR speColor = PostProcessSpecularColor();
	D3DCOLOR difColor = PostProcessDiffuseColor(0xffffffff);

	float depth = ( gRDP.otherMode.depth_source == 1 ) ? gRDP.fPrimitiveDepth : 0;
	DrawSimple2DTexture((float)x0, (float)y0, (float)x1, (float)y1, t0, s0, t1, s1, speColor, difColor, depth, 1.0f);
}


void CRender::DrawSpriteR(uObjTxSprite &sprite, bool initCombiner, uint32 tile, uint32 left, uint32 top, uint32 width, uint32 height)	// With Rotation
{
	if( !status.bCIBufferIsRendered ) g_pFrameBufferManager->ActiveTextureBuffer();

	if( status.bHandleN64RenderTexture )
	{
		g_pRenderTextureInfo->maxUsedHeight = g_pRenderTextureInfo->N64Height;
		status.bFrameBufferIsDrawn = true;
		status.bFrameBufferDrawnByTriangles = true;
	}
	SetCombinerAndBlender();

	float scaleX = sprite.sprite.scaleW/1024.0f;
	float scaleY = sprite.sprite.scaleH/1024.0f;

	if( width == 0 || height == 0 )
	{
		width = g_textures[tile].m_dwTileWidth;
		height = g_textures[tile].m_dwTileHeight;
	}

	RECT src = {left,top,width, height};
	float depth = ( gRDP.otherMode.depth_source == 1 ) ? depth = gRDP.fPrimitiveDepth : 0;

	float x0 = sprite.sprite.objX/4.0f;
	float y0 = sprite.sprite.objY/4.0f;
	float x1 = sprite.sprite.imageW / 32.0f / scaleX + x0;
	float y1 = sprite.sprite.imageH / 32.0f / scaleY + y0;

	if( sprite.sprite.imageFlags&1 ) { float temp = x0; x0 = x1; x1 = temp;	} // flip X 
	if( sprite.sprite.imageFlags&0x10 ) { float temp = y0; y0 = y1; y1 = temp; } // flip Y

	g_texRectTVtx[0].x = (gObjMtxReal.A*x0 + gObjMtxReal.B*y0 + gObjMtxReal.X)*windowSetting.fMultX;
	g_texRectTVtx[0].y = (gObjMtxReal.C*x0 + gObjMtxReal.D*y0 + gObjMtxReal.Y)*windowSetting.fMultY;
	g_texRectTVtx[0].z = depth;
	g_texRectTVtx[0].rhw = 1;
	g_texRectTVtx[1].x = (gObjMtxReal.A*x1 + gObjMtxReal.B*y0 + gObjMtxReal.X)*windowSetting.fMultX;
	g_texRectTVtx[1].y = (gObjMtxReal.C*x1 + gObjMtxReal.D*y0 + gObjMtxReal.Y)*windowSetting.fMultY;
	g_texRectTVtx[1].z = depth;
	g_texRectTVtx[1].rhw = 1;
	g_texRectTVtx[2].x = (gObjMtxReal.A*x1 + gObjMtxReal.B*y1 + gObjMtxReal.X)*windowSetting.fMultX;
	g_texRectTVtx[2].y = (gObjMtxReal.C*x1 + gObjMtxReal.D*y1 + gObjMtxReal.Y)*windowSetting.fMultY;
	g_texRectTVtx[2].z = depth;
	g_texRectTVtx[2].rhw = 1;
	g_texRectTVtx[3].x = (gObjMtxReal.A*x0 + gObjMtxReal.B*y1 + gObjMtxReal.X)*windowSetting.fMultX;
	g_texRectTVtx[3].y = (gObjMtxReal.C*x0 + gObjMtxReal.D*y1 + gObjMtxReal.Y)*windowSetting.fMultY;
	g_texRectTVtx[3].z = depth;
	g_texRectTVtx[3].rhw = 1;

	g_texRectTVtx[0].tcord[0].u = left/g_textures[tile].m_fTexWidth;
	g_texRectTVtx[0].tcord[0].v = top/g_textures[tile].m_fTexHeight;
	g_texRectTVtx[1].tcord[0].u = (left+width)/g_textures[tile].m_fTexWidth;
	g_texRectTVtx[1].tcord[0].v = top/g_textures[tile].m_fTexHeight;
	g_texRectTVtx[2].tcord[0].u = (left+width)/g_textures[tile].m_fTexWidth;
	g_texRectTVtx[2].tcord[0].v = (top+height)/g_textures[tile].m_fTexHeight;
	g_texRectTVtx[3].tcord[0].u = left/g_textures[tile].m_fTexWidth;
	g_texRectTVtx[3].tcord[0].v = (top+height)/g_textures[tile].m_fTexHeight;

	D3DCOLOR speColor = PostProcessSpecularColor();
	D3DCOLOR difColor = PostProcessDiffuseColor(0xffffffff);

	g_texRectTVtx[0].dcDiffuse = g_texRectTVtx[1].dcDiffuse 
		= g_texRectTVtx[2].dcDiffuse = g_texRectTVtx[3].dcDiffuse = difColor;
	g_texRectTVtx[0].dcSpecular = g_texRectTVtx[1].dcSpecular 
		= g_texRectTVtx[2].dcSpecular = g_texRectTVtx[3].dcSpecular = difColor;

	DrawSpriteR_Render();
}

void CRender::DrawFrameBuffer(bool useVIreg, uint32 left, uint32 top, uint32 width, uint32 height)
{
	BeginRendering();

	LoadFrameBuffer(useVIreg, left, top, width, height);

	m_pColorCombiner->InitCombinerBlenderForSimpleTextureDraw(0);

	ZBufferEnable(FALSE);
	SetZUpdate(FALSE);
	if( left == 0 )
		SetAlphaTestEnable(FALSE);
	else
		SetAlphaTestEnable(TRUE);	// use Alpha Test for partial frame buffer draw, for Dr. Mario 64

	CBlender::Disable();

	CTexture *pTexture = g_textures[0].m_pCTexture;
	if( pTexture )
	{
		if( useVIreg )
		{
			// Draw the whole frame buffer
			DrawSimple2DTexture(0, 0, windowSetting.uViWidth, windowSetting.uViHeight, 
				0, 0, 1/pTexture->m_fXScale, 1/pTexture->m_fYScale, 0xFFFFFFFF, 0xFFFFFFFF, 0, 1);
		}
		else
		{
			// Draw a small texture in frame buffer
			DrawSimple2DTexture((float)left, (float)top, (float)(left+width), (float)(top+height), 
				0, 0, 1/pTexture->m_fXScale, 1/pTexture->m_fYScale, 0xFFFFFFFF, 0xFFFFFFFF, 0, 1);
		}
	}

	TXTRBUF_OR_CI_DUMP(TRACE0("Draw Frame Buffer Img"));
#ifdef _DEBUG
	if( pauseAtNext && ( eventToPause == NEXT_FRAME || eventToPause == NEXT_FLUSH_TRI ) )
	{
		TRACE0("Draw Frame Buffer Img");
		debuggerPause = true;
		DebuggerPause();
	}
#endif

	EndRendering();
}

void CRender::DrawObjBGCopy(uObjBg &info)
{
	if( !status.bCIBufferIsRendered ) g_pFrameBufferManager->ActiveTextureBuffer();

	if( status.bHandleN64RenderTexture )
	{
		g_pRenderTextureInfo->maxUsedHeight = g_pRenderTextureInfo->N64Height;
		status.bFrameBufferIsDrawn = true;
		status.bFrameBufferDrawnByTriangles = true;
	}
	SetCombinerAndBlender();

	uint32 frameH = info.frameH;
	uint32 frameW = info.frameW;
	uint32 imageH = info.imageH;
	uint32 imageW = info.imageW;

	if( options.bEnableHacks )
	{
		if( g_CI.dwWidth == 0x200 && info.imageFmt == g_CI.dwFormat && info.imageSiz == g_CI.dwSize &&
			frameW == 0x800 )
		{
			// Hack for RE2
			uint32 width = *g_GraphicsInfo.VI_WIDTH_REG & 0xFFF;
			imageH = frameH = (frameW/4*frameH/4)/width*4;
			imageW = frameW = width*4;
		}
	}

	float x0 = info.frameX / 4.0f;
	float y0 = info.frameY / 4.0f;
	float x1 = frameW / 4.0f + x0;
	float y1 = frameH / 4.0f + y0;

	float s0 = info.imageX / 32.0f;
	float t0 = info.imageY / 32.0f;

	float texwidth = imageW/4.0f;
	float texheight = imageH/4.0f;

	float u0 = s0/g_textures[0].m_fTexWidth;
	float v0 = t0/g_textures[0].m_fTexHeight;
	float maxu = texwidth/g_textures[0].m_fTexWidth;
	float maxv = texheight/g_textures[0].m_fTexHeight;

	float x2 = x0 + (texwidth-s0);
	float y2 = y0 + (texheight-t0);
	float u1 = (x1-x2)/g_textures[0].m_fTexWidth;
	float v1 = (y1-y2)/g_textures[0].m_fTexHeight;

	float depth = (gRDP.otherMode.depth_source == 1 ? gRDP.fPrimitiveDepth : 0.0f);

	D3DCOLOR speColor = PostProcessSpecularColor();
	D3DCOLOR difColor = PostProcessDiffuseColor(0xffffffff);


	if( options.enableHackForGames == HACK_FOR_COMMANDCONQUER )
	{
		float s1 = (x1-x0) + s0;
		float t1 = (y1-y0) + t0;
		DrawSimple2DTexture(x0, y0, x1, y1, u0, v0, 
			s1/g_textures[0].m_fTexWidth, t1/g_textures[0].m_fTexHeight, difColor, speColor, depth, 1);
	}
	else if( x2 >= x1 )
	{
		float s1 = (x1-x0) + s0;
		if( y2 >= y1 )
		{
			float t1 = (y1-y0) + t0;
			DrawSimple2DTexture(x0, y0, x1, y1, u0, v0, 
				s1/g_textures[0].m_fTexWidth, t1/g_textures[0].m_fTexHeight, difColor, speColor, depth, 1);
		}
		else
		{
			DrawSimple2DTexture(x0, y0, x1, y2, u0, v0, 
				s1/g_textures[0].m_fTexWidth, maxv, difColor, speColor, depth, 1);
			DrawSimple2DTexture(x0, y2, x1, y1, u0, 0, 
				s1/g_textures[0].m_fTexWidth, v1, difColor, speColor, depth, 1);
		}
	}
	else
	{
		if( y2 >= y1 )
		{
			float t1 = (y1-y0) + t0;
			DrawSimple2DTexture(x0, y0, x2, y1, u0, v0, 
				maxu, t1/g_textures[0].m_fTexHeight, difColor, speColor, depth, 1);
			DrawSimple2DTexture(x2, y0, x1, y1, 0, v0, 
				u1, t1/g_textures[0].m_fTexHeight, difColor, speColor, depth, 1);
		}
		else
		{
			DrawSimple2DTexture(x0, y0, x2, y2, u0, v0, maxu, maxv, difColor, speColor, depth, 1);
			DrawSimple2DTexture(x2, y0, x1, y2, 0, v0, u1, maxv, difColor, speColor, depth, 1);
			DrawSimple2DTexture(x0, y2, x2, y1, u0, 0, maxu, v1, difColor, speColor, depth, 1);
			DrawSimple2DTexture(x2, y2, x1, y1, 0, 0, u1, v1, difColor, speColor, depth, 1);
		}
	}

	DEBUGGER_PAUSE_AT_COND_AND_DUMP_COUNT_N(
		(pauseAtNext&&(eventToPause==NEXT_OBJ_BG||eventToPause==NEXT_FLUSH_TRI||eventToPause==NEXT_OBJ_TXT_CMD)),
		{
			TRACE0("Pause ObjBG Copy");
		}
	);
}

void CRender::DrawObjBG1CYC(uObjScaleBg &bg, bool scaled)	//Without Ratation
{
	if( !status.bCIBufferIsRendered ) g_pFrameBufferManager->ActiveTextureBuffer();

	if( g_curRomInfo.bDisableObjBG )
		return;

	if( status.bHandleN64RenderTexture )
	{
		g_pRenderTextureInfo->maxUsedHeight = g_pRenderTextureInfo->N64Height;
		status.bFrameBufferIsDrawn = true;
		status.bFrameBufferDrawnByTriangles = true;
	}

	SetCombinerAndBlender();

	float depth = (gRDP.otherMode.depth_source == 1 ? gRDP.fPrimitiveDepth : 0.0f);

	float x0 = bg.frameX / 4.0f;
	float y0 = bg.frameY / 4.0f;
	float x1 = bg.frameW / 4.0f + x0;
	float y1 = bg.frameH / 4.0f + y0;

	float s0 = bg.imageX / 32.0f;
	float t0 = bg.imageY / 32.0f;

	float scaleX = bg.scaleW/1024.0f;
	float scaleY = bg.scaleH/1024.0f;

	float texwidth = bg.imageW/4.0f;
	float texheight = bg.imageH/4.0f;

	float u0 = s0/g_textures[0].m_fTexWidth;
	float v0 = t0/g_textures[0].m_fTexHeight;
	float maxu = texwidth/g_textures[0].m_fTexWidth;
	float maxv = texheight/g_textures[0].m_fTexHeight;

	float x2 = x0 + (texwidth-s0)/scaleX;
	float y2 = y0 + (texheight-t0)/scaleY;
	float u1 = (x1-x2)*scaleX/g_textures[0].m_fTexWidth;
	float v1 = (y1-y2)*scaleY/g_textures[0].m_fTexHeight;

	D3DCOLOR speColor = PostProcessSpecularColor();
	D3DCOLOR difColor = PostProcessDiffuseColor(0xffffffff);

	SetAlphaTestEnable(FALSE);

	if( options.enableHackForGames != HACK_FOR_YOSHI )
	{
		float s1 = (x1-x0)*scaleX + s0;
		float t1 = (y1-y0)*scaleY + t0;
		DrawSimple2DTexture(x0, y0, x1, y1, u0, v0, 
			s1/g_textures[0].m_fTexWidth, t1/g_textures[0].m_fTexHeight, difColor, speColor, depth, 1);
	}
	else if( x2 >= x1 )
	{
		float s1 = (x1-x0)*scaleX + s0;
		if( y2 >= y1 )
		{
			float t1 = (y1-y0)*scaleY + t0;
			DrawSimple2DTexture(x0, y0, x1, y1, u0, v0, 
				s1/g_textures[0].m_fTexWidth, t1/g_textures[0].m_fTexHeight, difColor, speColor, depth, 1);
		}
		else
		{
			DrawSimple2DTexture(x0, y0, x1, y2, u0, v0, 
				s1/g_textures[0].m_fTexWidth, maxv, difColor, speColor, depth, 1);
			DrawSimple2DTexture(x0, y2, x1, y1, u0, 0, 
				s1/g_textures[0].m_fTexWidth, v1, difColor, speColor, depth, 1);
		}
	}
	else
	{
		if( y2 >= y1 )
		{
			float t1 = (y1-y0)*scaleY + t0;
			DrawSimple2DTexture(x0, y0, x2, y1, u0, v0, 
				maxu, t1/g_textures[0].m_fTexHeight, difColor, speColor, depth, 1);
			DrawSimple2DTexture(x2, y0, x1, y1, 0, v0, 
				u1, t1/g_textures[0].m_fTexHeight, difColor, speColor, depth, 1);
		}
		else
		{
			DrawSimple2DTexture(x0, y0, x2, y2, u0, v0, maxu, maxv, difColor, speColor, depth, 1);
			DrawSimple2DTexture(x2, y0, x1, y2, 0, v0, u1, maxv, difColor, speColor, depth, 1);
			DrawSimple2DTexture(x0, y2, x2, y1, u0, 0, maxu, v1, difColor, speColor, depth, 1);
			DrawSimple2DTexture(x2, y2, x1, y1, 0, 0, u1, v1, difColor, speColor, depth, 1);
		}
	}

	DEBUGGER_PAUSE_AT_COND_AND_DUMP_COUNT_N(
		(pauseAtNext&&(eventToPause==NEXT_OBJ_BG||eventToPause==NEXT_FLUSH_TRI||eventToPause==NEXT_OBJ_TXT_CMD)),
		{
			DebuggerAppendMsg("Pause BG 1CYC: (%.0f,%.0f - %.0f,%.0f), \ntex (%.2f,%.2f), scale (%.2f,%.2f)",x0,y0,x1,y1,s0,t0,scaleX,scaleY);
		}
	);
}


void CRender::DrawSprite(uObjTxSprite &sprite, bool rectR)	//Without Ratation
{
	if( !status.bCIBufferIsRendered ) g_pFrameBufferManager->ActiveTextureBuffer();

	if( status.bHandleN64RenderTexture )
	{
		g_pRenderTextureInfo->maxUsedHeight = g_pRenderTextureInfo->N64Height;
		status.bFrameBufferIsDrawn = true;
		status.bFrameBufferDrawnByTriangles = true;
	}
	SetCombinerAndBlender();
	D3DCOLOR speColor = PostProcessSpecularColor();
	D3DCOLOR difColor = PostProcessDiffuseColor(0xffffffff);


	float objX = sprite.sprite.objX/4.0f;
	float objY = sprite.sprite.objY/4.0f;
	float width = sprite.sprite.imageW / 32.0f;
	float high = sprite.sprite.imageH / 32.0f;
	float scaleW = sprite.sprite.scaleW/1024.0f;
	float scaleH = sprite.sprite.scaleH/1024.0f;

	if( g_curRomInfo.bIncTexRectEdge )
	{
		width++;
		high++;
	}

	float x0, y0, x1, y1;
	if( rectR )
	{
		// Upper-left coordinate
		// ( X + objX / BaseScaleX, Y+objY/BaseScaleY )
		// Lower-right coordinate
		// ( X + (objX + imageW / scaleW) / BaseScaleX - 1,	Y + (objY + imageH / scaleH) / BaseScaleY - 1 )

		x0 = gObjMtxReal.X + objX/gObjMtxReal.BaseScaleX;
		y0 = gObjMtxReal.Y + objY/gObjMtxReal.BaseScaleY;
		x1 = gObjMtxReal.X + (objX + width / scaleW) / gObjMtxReal.BaseScaleX - 1;
		y1 = gObjMtxReal.Y + (objY + high / scaleH) / gObjMtxReal.BaseScaleY - 1;
	}
	else
	{
		// (objX, objY) - ( objX+imageW/scaleW-1, objY+imageH/scaleH-1)
		x0 = objX;
		y0 = objY;
		x1 = objX + width / scaleW - 1;
		y1 = objY + high / scaleH - 1;

		if( (sprite.sprite.imageFlags&1) ) // flipX
		{
			float temp = x0;
			x0 = x1;
			x1 = temp;
		}

		if( (sprite.sprite.imageFlags&0x10) ) // flipY
		{
			float temp = y0;
			y0 = y1;
			y1 = temp;
		}
	}

	float depth = (gRDP.otherMode.depth_source == 1 ? gRDP.fPrimitiveDepth : 0.0f);
	CTexture *pTexture = g_textures[0].m_pCTexture;
	DrawSimple2DTexture(x0, y0, x1, y1, 0, 0, 1/pTexture->m_fXScale, 1/pTexture->m_fYScale, 
		difColor, speColor, depth, 1);
}


void CRender::LoadObjBG1CYC(uObjScaleBg &bg)
{
	uint32 imageWidth = bg.imageW/4;
	uint32 imageHeight = bg.imageH/4;

	TxtrInfo gti;
	gti.Format	= bg.imageFmt;
	gti.Size		= bg.imageSiz;

	uint8* img = (uint8*)(g_pu8RamBase+RSPSegmentAddr(bg.imagePtr));
	
	uint32 palAddr = (uint32)(&g_wRDPTlut[0]);
	gti.Address	= RSPSegmentAddr(bg.imagePtr);

	gti.LeftToLoad		= 0;
	gti.TopToLoad		= 0;

	gti.WidthToCreate		= imageWidth;
	gti.HeightToCreate		= imageHeight;

	gti.clampS = gti.clampT = 1;
	gti.maskS = gti.maskT = 0;

	gti.Palette		= bg.imagePal;
	gti.PalAddress	= palAddr;

	gti.Pitch	= imageWidth << gti.Size >> 1;
	gti.Pitch	= (gti.Pitch>>3)<<3;	// Align to 8 bytes

	if( gti.Address + gti.Pitch*gti.HeightToCreate > g_dwRamSize )
	{
		TRACE0("Skip BG 1CYC loading, memory out of bound");
		return;
	}

	gti.TLutFmt	= TLUT_FMT_RGBA16;	//RGBA16
	gti.bSwapped	= FALSE;

	gti.HeightToLoad = gti.HeightToCreate;
	gti.WidthToLoad = gti.WidthToCreate;
	gti.pPhysicalAddress = ((uint8*)g_pu32RamBase)+gti.Address;
	gti.tileNo = -1;
	TxtrCacheEntry *pEntry = gTextureManager.GetTexture(&gti, false,false);
	// check if a hires has been enabled and not yet loaded
	if( options.bLoadHiResTextures && (pEntry->pEnhancedTexture == NULL || pEntry->dwEnhancementFlag < TEXTURE_EXTERNAL ) )
	{
		// try to load hires replacement
		LoadHiresTexture(*pEntry);
	}
	
	// and push it to texture memory
	SetCurrentTexture(0,pEntry);

	DEBUGGER_IF_DUMP((pauseAtNext && (eventToPause == NEXT_OBJ_TXT_CMD||eventToPause == NEXT_FLUSH_TRI||eventToPause == NEXT_OBJ_BG)),
		{
			TRACE0("Load Obj BG 1CYC:\n");
			DebuggerAppendMsg("Addr=0x%08X, W=%d, H=%d, Left=%d, Top=%d\n", 
				gti.Address, gti.WidthToCreate, gti.HeightToCreate, gti.LeftToLoad, gti.TopToLoad);
			DebuggerAppendMsg("Fmt=%s-%db, Pal=%d\n",
				pszImgFormat[gti.Format], pnImgSize[gti.Size], gti.Palette);
		}
	);
}

void CRender::LoadObjSprite(uObjTxSprite &sprite, bool useTIAddr)//backtomenow
{
	TxtrInfo gti;
	gti.Format	= sprite.sprite.imageFmt;
	gti.Size	= sprite.sprite.imageSiz;

	uint8* img;
	if( useTIAddr )
	{
		img = (uint8*)(g_pu8RamBase+RSPSegmentAddr(g_TI.dwAddr));
	}
	else
		img = (uint8*)(g_pu8RamBase+RSPSegmentAddr(sprite.txtr.block.image));
	uint32 palAddr = (uint32)(&g_wRDPTlut[0]);

	gti.Address	= RSPSegmentAddr(sprite.txtr.block.image);
	gti.Address += sprite.sprite.imageAdrs<<3;
	gti.LeftToLoad		= 0;
	gti.TopToLoad		= 0;
	gti.Palette	= sprite.sprite.imagePal;
	gti.PalAddress = palAddr;

	if( sprite.txtr.block.type == S2DEX_OBJLT_TXTRBLOCK )
	{
		gti.WidthToCreate		= sprite.sprite.imageW/32;
		if( sprite.sprite.imageW >= 0x8000 )
		{
			gti.WidthToCreate = (0x10000-sprite.sprite.imageW)/32;
		}
		gti.HeightToCreate	= sprite.sprite.imageH/32;
		if( sprite.sprite.imageH >= 0x8000 )
		{
			gti.HeightToCreate	= (0x10000-sprite.sprite.imageH)/32;
		}
		gti.Pitch		= (2047/(sprite.txtr.block.tline-1)) << 3;
	}
	else if( sprite.txtr.block.type == S2DEX_OBJLT_TXTRTILE )
	{
//#define	GS_PIX2TMEM(pix, siz)	((pix)>>(4-(siz)))
//#define	GS_TT_TWIDTH(pix,siz)	((GS_PIX2TMEM((pix), (siz))<<2)-1)
//#define	GS_TT_THEIGHT(pix,siz)	(((pix)<<2)-1)

		gti.WidthToCreate		= ((sprite.txtr.tile.twidth+1)>>2)<<(4-gti.Size);
		gti.HeightToCreate	= (sprite.txtr.tile.theight+1)>>2;

		if( gti.Size == TXT_SIZE_4b )
		{
			gti.Pitch = gti.WidthToCreate >> 1;
		}
		else
			//gti.Pitch		= (sprite.txtr.tile.twidth+1) << 3;
			gti.Pitch		= gti.WidthToCreate << (gti.Size-1);
	}

	if( gti.Address + gti.Pitch*gti.HeightToCreate > g_dwRamSize )
	{
		TRACE0("Skip Obj sprite loading, memory out of bound");
		return;
	}

	gti.TLutFmt	= TLUT_FMT_RGBA16;	//RGBA16
	gti.bSwapped	= FALSE;

	gti.HeightToLoad = gti.HeightToCreate;
	gti.WidthToLoad = gti.WidthToCreate;
	gti.pPhysicalAddress = ((uint8*)g_pu32RamBase)+gti.Address;
	gti.tileNo = -1;
	TxtrCacheEntry *pEntry = gTextureManager.GetTexture(&gti, false);
	SetCurrentTexture(0,pEntry);
}