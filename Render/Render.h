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

#ifndef _RICE_RENDER_H
#define _RICE_RENDER_H

#include "../stdafx.h"
#include "ExtendedRender.h"

enum TextureChannel 
{
	TXT_RGB,
	TXT_ALPHA,
	TXT_RGBA,
};

class CRender : public CExtendedRender
{
protected:
	CRender();

	int TileUFlags[8];
	int TileVFlags[8];

public:
	BOOL	m_bZUpdate;
	BOOL	m_bZCompare;
	uint32	m_dwZBias;

	int	m_dwMinFilter;
	int	m_dwMagFilter;

	uint32	m_dwAlpha;

	uint64		m_Mux;

	CColorCombiner *m_pColorCombiner;
	
	virtual ~CRender();
	
	inline bool IsTexel0Enable() {return m_pColorCombiner->m_bTex0Enabled;}
	inline bool IsTexel1Enable() {return m_pColorCombiner->m_bTex1Enabled;}
	inline bool CRender::IsTextureEnabled() { return (m_pColorCombiner->m_bTex0Enabled||m_pColorCombiner->m_bTex1Enabled); }

	inline RenderTexture& GetCurrentTexture() { return g_textures[gRSP.curTile]; }
	inline RenderTexture& GetTexture(uint32 dwTile) { return g_textures[dwTile]; }
	void SetViewport(int nLeft, int nTop, int nRight, int nBottom, int maxZ);
	virtual void UpdateScissor() {}
	virtual void ApplyRDPScissor(bool force=false) {}
	virtual void UpdateClipRectangle();
	virtual void UpdateScissorWithClipRatio();
	virtual void ApplyScissorWithClipRatio(bool force=false) {}
	virtual void CaptureScreen(char *folder) {}

	void SetTextureEnable(bool bEnable);
	void SetTextureScale(int dwTile, float fScaleX, float fScaleY);
	
	virtual void SetFogEnable(bool bEnable) {}
	virtual void SetFogMinMax(float fMin, float fMax) = 0;
	virtual void TurnFogOnOff(bool flag)=0;

	virtual void SetFogColor(uint32 r, uint32 g, uint32 b, uint32 a) 
	{ 
		gRDP.fogColor = COLOR_RGBA(r, g, b, a); 
	}
	uint32 GetFogColor() { return gRDP.fogColor; }

	void SetProjection(const u32 addr, bool bReplace);
	void SetWorldView(const u32 addr, bool bPush, bool bReplace);

	inline void PopProjection()
	{
	/*	if (gRSP.mProjectionTop > 0)
			gRSP.mProjectionTop--;
		else
			TRACE0("Popping past projection stack limits");*/
	}

	void PopWorldView(u32 num = 1);
	
	void ResetMatrices(uint32 size);

	inline RenderShadeMode GetShadeMode() { return gRSP.shadeMode; }

	inline void CopyVtx(uint32 dwSrc, uint32 dwDest)
	{
		g_vecProjected[dwDest].Texture.x = g_vecProjected[dwSrc].Texture.x;
		g_vecProjected[dwDest].Texture.y = g_vecProjected[dwSrc].Texture.y;
	}
	inline void SetVtxTextureCoord(uint32 dwV, float tu, float tv)
	{
		
		g_vecProjected[dwV].Texture.x = tu;
		g_vecProjected[dwV].Texture.y = tv;
	}

	virtual void RenderReset();
	virtual void SetCombinerAndBlender();
	virtual void SetMux(uint32 dwMux0, uint32 dwMux1);
	void SetCullMode(bool bCullFront, bool bCullBack)
	{ gRDP.tnl.TriCull = bCullFront; gRDP.tnl.CullBack = bCullBack; }

	virtual void BeginRendering(void) {CRender::gRenderReferenceCount++;}		// For DirectX only
	virtual void EndRendering(void) 
	{
		if( CRender::gRenderReferenceCount > 0 )
			CRender::gRenderReferenceCount--;
	}

	virtual void ClearBuffer(bool cbuffer, bool zbuffer)=0;
	virtual void ClearZBuffer(float depth)=0;
	virtual void ClearBuffer(bool cbuffer, bool zbuffer, D3DRECT &rect) 
	{
		ClearBuffer(cbuffer, zbuffer);
	}
	virtual void ZBufferEnable(BOOL bZBuffer)=0;
	virtual void SetZCompare(BOOL bZCompare)=0;
	virtual void SetZUpdate(BOOL bZUpdate)=0;
	virtual void SetZBias(int bias)=0;
	virtual void SetAlphaTestEnable(BOOL bAlphaTestEnable)=0;

	void SetTextureFilter(uint32 dwFilter);
	virtual void ApplyTextureFilter() {}
	
	virtual void SetShadeMode(RenderShadeMode mode)=0;

	virtual void SetAlphaRef(uint32 dwAlpha)=0;
	virtual void ForceAlphaRef(uint32 dwAlpha)=0;

	virtual void InitOtherModes(void);

	void SetVertexTextureUVCoord(TLITVERTEX &v, float fTex0S, float fTex0T, float fTex1S, float fTex1T);
	void SetVertexTextureUVCoord(TLITVERTEX &v, float fTex0S, float fTex0T);

	bool DrawTriangles();
	virtual bool RenderFlushTris()=0;

	bool TexRect(LONG nX0, LONG nY0, LONG nX1, LONG nY1, float fS0, float fT0, float fScaleS, float fScaleT, bool colorFlag=false, uint32 difcolor=0xFFFFFFFF);
	bool TexRectFlip(LONG nX0, LONG nY0, LONG nX1, LONG nY1, float fS0, float fT0, float fS1, float fT1);
	bool FillRect(LONG nX0, LONG nY0, LONG nX1, LONG nY1, uint32 dwColor);
	bool Line3D(uint32 dwV0, uint32 dwV1, uint32 dwWidth);

	virtual void SetAddressUAllStages(uint32 dwTile, int dwFlag);
	virtual void SetAddressVAllStages(uint32 dwTile, int dwFlag);
	virtual void SetTextureUFlag(int dwFlag, uint32 tile)=0;
	virtual void SetTextureVFlag(int dwFlag, uint32 tile)=0;
	virtual void SetTexelRepeatFlags(uint32 dwTile);
	virtual void SetAllTexelRepeatFlag();
	
	virtual bool SetCurrentTexture(int tile, TxtrCacheEntry *pTextureEntry)=0;
	virtual bool SetCurrentTexture(int tile, CTexture *handler, uint32 dwTileWidth, uint32 dwTileHeight, TxtrCacheEntry *pTextureEntry) = 0;

	void SaveTextureToFile(CTexture &texture, char *filename, int width, int height);

	virtual bool InitDeviceObjects()=0;
	virtual bool ClearDeviceObjects()=0;
	virtual void Initialize(void);
	virtual void CleanUp(void);
	

	virtual void SetFillMode(FillMode mode)=0;

#ifdef _DEBUG
	virtual bool DrawTexture(int tex, TextureChannel channel = TXT_RGB );
#endif

	void LoadObjBGCopy(uObjBg &info);
	void LoadObjBG1CYC(uObjScaleBg &info);
	void LoadObjSprite(uObjTxSprite &info, bool useTIAddr=false);

	void LoadFrameBuffer(bool useVIreg=false, uint32 left=0, uint32 top=0, uint32 width=0, uint32 height=0);
	void LoadTextureFromMemory(void *buf, uint32 left, uint32 top, uint32 width, uint32 height, uint32 pitch, uint32 format);
	void LoadTxtrBufIntoTexture(void);
	void DrawSpriteR(uObjTxSprite &sprite, bool initCombiner=true, uint32 tile=0, uint32 left=0, uint32 top=0, uint32 width=0, uint32 height=0);
	void DrawSprite(uObjTxSprite &sprite, bool rectR = true);
	void DrawObjBGCopy(uObjBg &info);
	virtual void DrawSpriteR_Render(){};
	virtual void DrawSimple2DTexture(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, D3DCOLOR dif, float z, float rhw)=0;
	void DrawFrameBuffer(bool useVIreg=false, uint32 left=0, uint32 top=0, uint32 width=0, uint32 height=0);
	void DrawObjBG1CYC(uObjScaleBg &bg, bool scaled=true);

	static CRender * g_pRender;
	static int gRenderReferenceCount;
	static CRender * GetRender(void);
	static bool IsAvailable();


protected:

	// FillRect
	virtual bool	RenderFillRect(uint32 dwColor, float depth)=0;
	v2				m_fillRectVtx[2];
	
	// Line3D
	virtual bool	RenderLine3D()=0;

	LITVERTEX		m_line3DVtx[2];
	v2				m_line3DVector[4];
	
	// TexRect
	virtual bool	RenderTexRect()=0;

	TexCord			m_texRectTex1UV[2];
	TexCord			m_texRectTex2UV[2];

	// DrawSimple2DTexture
	virtual void	StartDrawSimple2DTexture(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, D3DCOLOR dif, float z, float rhw);

	// DrawSimpleRect
	virtual void	StartDrawSimpleRect(LONG nX0, LONG nY0, LONG nX1, LONG nY1, uint32 dwColor, float depth, float rhw);
	v2			m_simpleRectVtx[2];

	bool			RemapTextureCoordinate(float s0, float s1, uint32 tileWidth, uint32 mask, float textureWidth,
											float &u0, float &u1);

};

bool SaveRGBABufferToPNGFile(char *filename, unsigned char *buf, int width, int height);

#endif	//_RICE_RENDER_H
