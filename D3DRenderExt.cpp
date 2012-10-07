/*
Copyright (C) 2003-2009 Rice1964

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

#include "stdafx.h"

extern uObjMtxReal gObjMtxReal;
extern Matrix gD3DMtxReal;
extern Matrix gD3DObjOffset;

static BOOL bSavedZEnabled = TRUE;
static uint32 dwSavedMinFilter = 0;
static uint32 dwSavedMagFilter = 0;


//========================================================================

void D3DRender::DrawSpriteR_Render()	// With Rotation
{
	uint16 wIndices[2*3] = {1,0,2, 2,0,3};
	gD3DDevWrapper.SetFVF(RICE_FVF_TLITVERTEX);
	g_pD3DDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0, 4, 2, wIndices, D3DFMT_INDEX16, g_texRectTVtx, sizeof(TLITVERTEX));
}

void D3DRender::LoadTxtrBufFromRDRAM(void)
{
	LoadTxtrBufIntoTexture();
	LPD3DXSPRITE d3dSprite = InitSpriteDraw();
	if( d3dSprite != NULL )
	{
#if DIRECTX_VERSION == 8
		D3DXVECTOR2 scales(1, 1);
		D3DXVECTOR2	trans(0,0);
		d3dSprite->Draw(MYLPDIRECT3DTEXTURE(g_textures[0].m_lpsTexturePtr), NULL, &scales, NULL, 0, &trans, 0xFFFFFFFF);
#else
		d3dSprite->Draw(MYLPDIRECT3DTEXTURE(g_textures[0].m_lpsTexturePtr), NULL, NULL, NULL, 0xFFFFFFFF);
#endif
#ifdef _DEBUG
		if( pauseAtNext && eventToPause == NEXT_RENDER_TEXTURE )
		{
			MYLPDIRECT3DTEXTURE pD3DTexture = MYLPDIRECT3DTEXTURE(g_textures[0].m_lpsTexturePtr);
			MYLPDIRECT3DSURFACE pD3DSurface = NULL;
			pD3DTexture->GetSurfaceLevel(0,&pD3DSurface);
			((CDXGraphicsContext*)CGraphicsContext::g_pGraphicsContext)->SaveSurfaceToFile("\\DxTxtBuf25", pD3DSurface);
		}
#endif
	}
	FinishSpriteDraw(d3dSprite);
}


// #define SUPPORT_ZBUFFER_IMG		//Doesn't work good enough, still need lockable zbuffer
//#define SUPPORT_LOCKABLE_ZBUFFER
void D3DRender::DrawObjBGCopy(uObjBg &info)
{
	if( IsUsedAsDI(g_CI.dwAddr) )
	{
#ifndef SUPPORT_LOCKABLE_ZBUFFER
#ifndef SUPPORT_ZBUFFER_IMG
		g_pD3DDev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0, 0);	//Check me
		LOG_UCODE("    Clearing ZBuffer by using ZeldaBG");
#else
		if( g_CI.dwWidth == 0x200 && info.imageFmt == g_CI.dwFormat && info.imageSiz == g_CI.dwSize &&
				info.frameW == 0x800 )
		{
			InitCombinerBlenderForSimpleTextureDraw();
			ZBufferEnable( TRUE );
			gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR );
			gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );
			SetTextureFactor(0);
			//SetTextureFactor(0x80808080);
			//gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
			//gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			
			// Resident Evil2
			uint32 width = *g_GraphicsInfo.VI_WIDTH_REG & 0xFFF;
			uint32 height = (info.frameW/4*info.frameH/4)/width;
			uint32 pitch = width;

			//LoadObjBGCopy(info);

			// We don't support lockable Zbuffer, but we can simular it by splitting the image
			// to small pieces and render the piece with depth from the depth image

			uint16 *Base = (uint16*)(g_pRDRAMu8+RSPSegmentAddr(info.imagePtr));
			float depth;

			const uint32 inc=10;

			for( uint32 i=0; i<height; i+=inc )
			{
				uint16 *pSrc = Base + i * pitch;
				depth = (*pSrc + 1.0f ) / 0x10000;
				for( uint32 j=0; j<width; j+=inc)
				{
					DrawSimpleRect(j, i, j+inc, i+inc, gRDP.primitiveColor, depth, 1/depth);

#ifdef _DEBUG
					if( pauseAtNext && eventToPause == NEXT_TRIANGLE )
					{
						debuggerPause = true;
						TRACE0("Pause after drawing a rect for Z buffer");
						DebuggerPause();
					}
#endif
				}
			}
		}
#endif
#else
		if( g_pLockableBackBuffer == NULL && status.bHandleN64RenderTexture == false )
		{
			if( IsResultGood(g_pD3DDev->CreateDepthStencilSurface(windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, D3DFMT_D16_LOCKABLE, D3DMULTISAMPLE_NONE, &g_pLockableBackBuffer)) && g_pLockableBackBuffer )
			{
				g_pD3DDev->SetRenderTarget(NULL, g_pLockableBackBuffer);
				TRACE0("Created and use lockable depth buffer");
			}
			else
			{
				if( g_pLockableBackBuffer )
				{
					g_pLockableBackBuffer->Release();
					g_pLockableBackBuffer = NULL;
				}
				TRACE0("Can not create lockable depth buffer");
			}
		}

		DEBUGGER_IF_DUMP((pauseAtNext&& (eventToPause==NEXT_OBJ_TXT_CMD||eventToPause==NEXT_OBJ_BG)), 
		{TRACE0("Using BG to update Z buffer has not been implemented yet");});

		// I can not finish this function because Z buffer is not lockable
		// and lockable zbuffer does not work
		MYLPDIRECT3DSURFACE pDepthBufferSurf = NULL;
		g_pD3DDev->GetDepthStencilSurface(&pDepthBufferSurf);

		D3DLOCKED_RECT dlre;
		ZeroMemory( &dlre, sizeof(D3DLOCKED_RECT) );
		//int pixSize = GetPixelSize();
		uint32 width = *g_GraphicsInfo.VI_WIDTH_REG & 0xFFF;
		uint32 height = (info.frameW/4*info.frameH/4)/width;
		uint32 pitch = width;

		if (IsResultGood(pDepthBufferSurf->LockRect(&dlre, NULL, D3DLOCK_NOSYSLOCK)))
		{
			uint16 *Base = (uint16*)(g_pRDRAMu8+RSPSegmentAddr(g_ZI.dwAddr));
			int x, y;
			for( int i=0; i<windowSetting.uDisplayHeight; i++ )
			{
				y = i*height/windowSetting.uDisplayHeight;

				uint16 *pSrc = Base + y * pitch;
				uint16 *pDst = (uint16*)((uint8 *)dlre.pBits + i * dlre.Pitch);

				for( int j=0; j<windowSetting.uDisplayWidth; j++ )
				{
					x = j*width/windowSetting.uDisplayWidth;
					pDst[j] = pSrc[x];
				}
			}
			pDepthBufferSurf->UnlockRect();
		}
		
		pDepthBufferSurf->Release();
#endif
		return;
	}
	else
	{	
		CRender::LoadObjBGCopy(info);
		CRender::DrawObjBGCopy(info);
	}
}

Matrix spriteMtx(1.0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 1.0);

extern uObjTxtrTLUT *gObjTlut;
extern void TH_RemoveTextureEntry(TxtrCacheEntry * pEntry);


void D3DRender::FinishSpriteDraw(LPD3DXSPRITE d3dSprite)
{
	if( d3dSprite != NULL )
	{
		d3dSprite->End();
		d3dSprite->Release();
	}
}



LPD3DXSPRITE D3DRender::InitSpriteDraw(void)
{
	gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

	gD3DDevWrapper.SetTexture( 0, MYLPDIRECT3DTEXTURE(g_textures[gRSP.curTile].m_lpsTexturePtr) );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );

	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );
	SetTextureFactor(0xFFFFFFFF);

	LPD3DXSPRITE d3dSprite;
	HRESULT hr = D3DXCreateSprite(g_pD3DDev, &d3dSprite);
	if( hr == S_OK )
	{
#if DIRECTX_VERSION == 8
		d3dSprite->Begin();
#else
		d3dSprite->Begin(D3DXSPRITE_DONOTMODIFY_RENDERSTATE|D3DXSPRITE_DONOTSAVESTATE);
#endif
		return d3dSprite;
	}
	else
	{
		if( d3dSprite != NULL )
			d3dSprite->Release();
		return NULL;
	}
	return NULL;
}


void D3DRender::DrawText(const char* str, RECT *rect)
{
	return;

	//// code below does not work

	//LOGFONT     lf;
	//HFONT       hFont;

	//// Let's create a TrueType font to display.
	//memset(&lf,0,sizeof(LOGFONT));
	//lf.lfHeight               =   20 ;
	//lf.lfWeight               =   FW_NORMAL ;
	//lf.lfCharSet              =   ANSI_CHARSET ;
	//lf.lfOutPrecision         =   OUT_DEFAULT_PRECIS ;
	//lf.lfClipPrecision        =   CLIP_DEFAULT_PRECIS ;
	//lf.lfQuality              =   DEFAULT_QUALITY ;
	//lf.lfPitchAndFamily       =   FF_DONTCARE|DEFAULT_PITCH;
	//lstrcpy (lf.lfFaceName, "Arial") ;
	//hFont = CreateFontIndirect(&lf);

	//HDC hdc;
	//if ((hdc = GetDC(g_GraphicsInfo.hWnd)) == NULL)
	//{
	//	ErrorMsg("GetDC on main window failed");
	//	return;
	//}

	//HGDIOBJ oldFont = SelectObject(hdc, hFont);

	//LPD3DXFONT pFont;
	//HRESULT res = D3DXCreateFont(g_pD3DDev, hFont, &pFont);
	//if( res == S_OK )
	//{
	//	pFont->DrawText(str, -1, rect, DT_CENTER, 0xFFFFFFFF);
	//	pFont->Release();
	//}

	//DeleteObject(SelectObject(hdc, oldFont));

	//ReleaseDC(g_GraphicsInfo.hWnd, hdc);
}
