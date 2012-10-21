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

#ifndef __RICE_D3DRENDER_H__
#define __RICE_D3DRENDER_H__

#include "stdafx.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)  { if(p) { (p)->Release();     (p)=NULL; } }
#endif

class ofstream;
extern MYLPDIRECT3DDEVICE g_pD3DDev;
extern CD3DDevWrapper    gD3DDevWrapper;
extern LPDIRECT3DVERTEXSHADER9 gVertexShader;

class D3DRender : public CRender
{
	friend class DirectXDeviceBuilder;
	friend class CDeviceBuilder;
	friend class CDirectXPixelShaderCombiner;
	friend class CDirectXBlender;
	
protected:
	D3DRender();
	~D3DRender();

public:
	void BeginRendering(void); 
	void EndRendering(void) 
	{
		if( CRender::gRenderReferenceCount > 0 )
		{
			CRender::gRenderReferenceCount--;
			if( CRender::gRenderReferenceCount == 0 )
			{
				g_pD3DDev->EndScene(); 
			}
		}
	}

	bool InitDeviceObjects();
	bool ClearDeviceObjects();

	void CaptureScreen(char *filename);

	void ApplyTextureFilter();

	void SetAddressUAllStages(uint32 dwTile, TextureUVFlag dwFlag);
	void SetAddressVAllStages(uint32 dwTile, TextureUVFlag dwFlag);

	void SetTextureUFlag(TextureUVFlag dwFlag, uint32 tile);
	void SetTextureVFlag(TextureUVFlag dwFlag, uint32 tile);

	void SetCullMode(bool bCullFront, bool bCullBack);

	inline void SetTextureFactor(uint32 dwCol)
	{
		gD3DDevWrapper.SetRenderState(D3DRS_TEXTUREFACTOR, dwCol);
	}


	void SetShadeMode(RenderShadeMode mode);
	void ZBufferEnable(BOOL bZBuffer);
	void ClearZBuffer(float depth);
	void ClearBuffer(bool cbuffer, bool zbuffer);
	void ClearBuffer(bool cbuffer, bool zbuffer, D3DRECT &rect);

	void SetZCompare(BOOL bZCompare);
	void SetZUpdate(BOOL bZUpdate);
	void SetZBias(int bias);
	void SetAlphaRef(uint32 dwAlpha);
	void ForceAlphaRef(uint32 dwAlpha);
	void SetFillMode(FillMode mode);
	void SetAlphaTestEnable(BOOL bAlphaTestEnable);

	bool SetCurrentTexture(int tile, CTexture *handler,uint32 dwTileWidth, uint32 dwTileHeight, TxtrCacheEntry *pTextureEntry);
	bool SetCurrentTexture(int tile, TxtrCacheEntry *pTextureEntry);

	void DrawSimple2DTexture(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, D3DCOLOR dif, D3DCOLOR spe, float z, float rhw);
	void DrawSimpleRect(LONG nX0, LONG nY0, LONG nX1, LONG nY1, uint32 dwColor, float depth, float rhw);
	void DrawText(const char* str, RECT *rect);

	void SetFogMinMax(float fMin, float fMax);
	void SetFogEnable(bool bEnable);
	void TurnFogOnOff(bool flag);
	void SetFogColor(uint32 r, uint32 g, uint32 b, uint32 a);
	void UpdateScissor();
	void ApplyRDPScissor(bool force=false);
	void ApplyScissorWithClipRatio(bool force=false);


protected:
	SetCombineInfo m_curCombineInfo;

protected:

	D3DCOLOR PostProcessDiffuseColor(D3DCOLOR curDiffuseColor);
	D3DCOLOR PostProcessSpecularColor();

	// Basic render drawing functions
	bool RenderFlushTris();
	bool RenderTexRect();
	bool RenderFillRect(uint32 dwColor, float depth);
	bool RenderLine3D();

	// Combiner and blender functions

	void D3DSetMinFilter(uint32 dwStage, uint32 filter);
	void D3DSetMagFilter(uint32 dwStage, uint32 filter);
	void D3DSetMipFilter(uint32 dwStage, uint32 filter);
	
	// Extended render functions
public:
	// Frame buffer functions
	void LoadTxtrBufFromRDRAM(void);

	// Sprite2D functions
	LPD3DXSPRITE InitSpriteDraw(void);
	void FinishSpriteDraw(LPD3DXSPRITE d3dSprite);
	void DrawSpriteR_Render();

	// S2DEX GBI2 function
	void DrawObjBGCopy(uObjBg &info);
};


extern void UpdateOptionsForVertexShader(float halfS, float halfT);
extern bool InitVertexShader();
extern void InitVertexShaderConstants();
extern void UpdateVertexShaderConstant();

#endif // __RICE_D3DRENDER_H__