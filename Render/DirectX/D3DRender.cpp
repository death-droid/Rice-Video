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


#include "..\..\stdafx.h"

#define RICEFVF_TEXRECTFVERTEX ( D3DFVF_XYZRHW | /*D3DFVF_DIFFUSE |*/ D3DFVF_TEX2 )
#define RICE_FVF_FILLRECTVERTEX ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE  )

extern FiddledVtx * g_pVtxBase;
const int d3d_bias_factor = 4;

//*****************************************************************************
// Creator function for singleton
//*****************************************************************************
D3DRender::D3DRender()
{
	m_Mux = 0;
	memset(&m_curCombineInfo, 0, sizeof( m_curCombineInfo) );
}

D3DRender::~D3DRender()
{	
	ClearDeviceObjects();
	gD3DDevWrapper.SetPixelShader(NULL);
}

bool D3DRender::ClearDeviceObjects()
{
	for( int i=0; i<MAX_TEXTURES; i++)
	{
		if (g_textures[i].m_lpsTexturePtr)		// We keep a reference to the most recently selected texture
		{
			g_textures[i].m_lpsTexturePtr->Release();
			g_textures[i].m_lpsTexturePtr = NULL;
			gD3DDevWrapper.SetTexture( i, NULL );
		}
	}
	return true;
}
	

bool D3DRender::InitDeviceObjects()
{
	// We never change these
	gD3DDevWrapper.SetRenderState( D3DRS_DITHERENABLE, FALSE ); //DIsabled, we shouldnt need to deal with 16-bit anymore

	// We do our own culling
	gD3DDevWrapper.SetRenderState( D3DRS_CULLMODE,   D3DCULL_NONE );

	// We do our own lighting
	gD3DDevWrapper.SetRenderState( D3DRS_AMBIENT, COLOR_RGBA(255,255,255,255) );
	gD3DDevWrapper.SetRenderState( D3DRS_LIGHTING,	  FALSE);

	gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE );
	gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	gD3DDevWrapper.SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	gD3DDevWrapper.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	
	if( ((CDXGraphicsContext*)CGraphicsContext::g_pGraphicsContext)->IsFSAAEnable() )
		gD3DDevWrapper.SetRenderState( D3DRS_MULTISAMPLEANTIALIAS , TRUE);
	else
		gD3DDevWrapper.SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, FALSE);

	// Initialize all the renderstate to our defaults.
	SetShadeMode( gRSP.shadeMode );
	gD3DDevWrapper.SetRenderState( D3DRS_TEXTUREFACTOR, 0xFFFFFFFF );

	gD3DDevWrapper.SetRenderState( D3DRS_FOGENABLE, FALSE);
	float density = 1.0f;
	gD3DDevWrapper.SetRenderState(D3DRS_FOGDENSITY,   *(uint32 *)(&density));
	gD3DDevWrapper.SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);

	gD3DDevWrapper.SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );

    gD3DDevWrapper.SetRenderState(D3DRS_ALPHATESTENABLE,TRUE );
    gD3DDevWrapper.SetRenderState(D3DRS_ALPHAREF, 0x04);
    gD3DDevWrapper.SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );

	m_Mux = 0;
	memset(&m_curCombineInfo, 0, sizeof( m_curCombineInfo) );

	gD3DDevWrapper.Initalize();

	for(int i = 0; i < 8; i++) 
	{ 
		// Texturing stuff 
		D3DSetMinFilter( i, D3DTEXF_LINEAR ); 
		D3DSetMagFilter( i, D3DTEXF_LINEAR ); 
		if(options.bMipMaps)
		{
			D3DSetMipFilter( i, D3DTEXF_LINEAR );
			g_pD3DDev->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, 10.0f);
		}
		g_pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP ); 
		g_pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP ); 
		

		gD3DDevWrapper.SetTextureStageState( i, D3DTSS_COLORARG1, D3DTA_TEXTURE ); 
		gD3DDevWrapper.SetTextureStageState( i, D3DTSS_COLORARG2, D3DTA_DIFFUSE ); 
		gD3DDevWrapper.SetTextureStageState( i, D3DTSS_COLORARG0, D3DTA_DIFFUSE ); 
		gD3DDevWrapper.SetTextureStageState( i, D3DTSS_COLOROP, D3DTOP_MODULATE ); 
		gD3DDevWrapper.SetTextureStageState( i, D3DTSS_ALPHAARG1, D3DTA_TEXTURE ); 
		gD3DDevWrapper.SetTextureStageState( i, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE ); 
		gD3DDevWrapper.SetTextureStageState( i, D3DTSS_ALPHAARG0, D3DTA_DIFFUSE ); 
		gD3DDevWrapper.SetTextureStageState( i, D3DTSS_ALPHAOP, D3DTOP_MODULATE ); 

		gD3DDevWrapper.SetTexture( i, NULL );
	} 
	
	((CDirectXPixelShaderCombiner*)m_pColorCombiner)->Initialize();
	
	status.curScissor = UNKNOWN_SCISSOR;

	D3DCLIPSTATUS9 clippingstatus;
	g_pD3DDev->GetClipStatus(&clippingstatus);
	clippingstatus.ClipUnion = D3DCS_BACK | D3DCS_BOTTOM | D3DCS_FRONT | D3DCS_LEFT | D3DCS_RIGHT | D3DCS_TOP;
	clippingstatus.ClipIntersection = 0xFFFFFFFF;
	g_pD3DDev->SetClipStatus(&clippingstatus);
	g_pD3DDev->GetClipStatus(&clippingstatus);
	g_pD3DDev->SetRenderState(D3DRS_CLIPPING, TRUE);

	return true;	
}

bool D3DRender::RenderTexRect()
{
	gD3DDevWrapper.SetRenderState(D3DRS_DEPTHBIAS,0);
	uint16 wIndices[2*3] = {1,0,2, 2,0,3};
	g_pD3DDev->SetFVF(RICE_FVF_TLITVERTEX);
	return S_OK == g_pD3DDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0, 4, 2, wIndices, D3DFMT_INDEX16, g_texRectTVtx, sizeof(TLITVERTEX));
}

bool D3DRender::RenderFillRect(uint32 dwColor, float depth)
{
	FILLRECTVERTEX frv[4] = {   {m_fillRectVtx[0].x, m_fillRectVtx[0].y, depth, 1, dwColor},
								{m_fillRectVtx[1].x, m_fillRectVtx[0].y, depth, 1, dwColor},
								{m_fillRectVtx[1].x, m_fillRectVtx[1].y, depth, 1, dwColor},
								{m_fillRectVtx[0].x, m_fillRectVtx[1].y, depth, 1, dwColor}  };
	static uint16 wIndices[2*3] = {1,0,2, 2,0,3};

	gD3DDevWrapper.SetRenderState(D3DRS_DEPTHBIAS,0);
	g_pD3DDev->SetFVF(RICE_FVF_FILLRECTVERTEX);
	return S_OK == g_pD3DDev->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, wIndices, D3DFMT_INDEX16, frv, sizeof(FILLRECTVERTEX));
}

void ApplyZBias(uint32 bias) //FIXME
{
	//We should be calculating the actual offsets
	float f1 = bias > 0 ? -0.00075f : 0.0f;
	float f2 = bias > 0 ? 0.0f : 0.0f;
	// Change by the bias
	gD3DDevWrapper.SetRenderState(D3DRS_DEPTHBIAS,*(DWORD*)(&f1));
	gD3DDevWrapper.SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)(&f2));
}

void D3DRender::SetZBias(int bias)
{
	if (m_dwZBias != bias)
	{
		DEBUGGER_IF_DUMP(pauseAtNext, TRACE1("Set zbias = %d", bias));

		m_dwZBias = bias;
		ApplyZBias(m_dwZBias);
	}
}

bool D3DRender::RenderFlushTris()
{
	ApplyZBias(m_dwZBias);

	g_pD3DDev->SetFVF(RICE_FVF_TLITVERTEX);
	ClipVertexes();
	if( g_clippedVtxCount > 0 )
		g_pD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, g_clippedVtxCount/3, g_clippedVtxBuffer, sizeof(TLITVERTEX));

	return true;
}

bool D3DRender::RenderLine3D()
{
	FILLRECTVERTEX frv[4] = {   {m_line3DVector[0].x, m_line3DVector[0].y, m_line3DVtx[0].z, m_line3DVtx[0].rhw, m_line3DVtx[0].dcDiffuse},
								{m_line3DVector[1].x, m_line3DVector[1].y, m_line3DVtx[0].z, m_line3DVtx[0].rhw, m_line3DVtx[0].dcDiffuse},
								{m_line3DVector[2].x, m_line3DVector[2].y, m_line3DVtx[0].z, m_line3DVtx[0].rhw, m_line3DVtx[1].dcDiffuse},
								{m_line3DVector[3].x, m_line3DVector[3].y, m_line3DVtx[0].z, m_line3DVtx[0].rhw, m_line3DVtx[1].dcDiffuse}  };

	static uint16 wIndices[2*3] = {1,0,2, 2,1,3};

	gD3DDevWrapper.SetRenderState(D3DRS_DEPTHBIAS,0);
	g_pD3DDev->SetFVF(RICE_FVF_FILLRECTVERTEX);
	HRESULT hr = g_pD3DDev->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, wIndices, D3DFMT_INDEX16, frv, sizeof(FILLRECTVERTEX));

	return hr == S_OK;
}


LPDIRECT3DSURFACE9 g_pLockableBackBuffer=NULL;

void D3DRender::DrawSimple2DTexture(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, D3DCOLOR dif, D3DCOLOR spe, float z, float rhw)
{
	if( status.bVIOriginIsUpdated == true && currentRomOptions.screenUpdateSetting==SCREEN_UPDATE_AT_1ST_PRIMITIVE )
	{
		status.bVIOriginIsUpdated=false;
		CGraphicsContext::Get()->UpdateFrame();
		DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_SET_CIMG,{TRACE0("Screen Update at 1st Simple2DTexture");});
	}

	StartDrawSimple2DTexture(x0, y0, x1, y1, u0, v0, u1, v1, dif, spe, z, rhw);
	
	uint16 wIndices[2*3] = {1,0,2, 2,0,3};
	g_pD3DDev->SetFVF(RICE_FVF_TLITVERTEX);
	g_pD3DDev->DrawIndexedPrimitiveUP( D3DPT_TRIANGLELIST, 0, 4, 2, wIndices, D3DFMT_INDEX16, g_texRectTVtx, sizeof(TLITVERTEX));
}

void D3DRender::DrawSimpleRect(LONG nX0, LONG nY0, LONG nX1, LONG nY1, uint32 dwColor, float depth, float rhw)
{
	StartDrawSimpleRect(nX0, nY0, nX1, nY1, dwColor, depth, rhw);

	// Divide by 2 instead of 4, because screen is magnified by two
	FILLRECTVERTEX frv[4] = {   {m_simpleRectVtx[0].x, m_simpleRectVtx[0].y, depth, rhw, dwColor},
								{m_simpleRectVtx[1].x, m_simpleRectVtx[0].y, depth, rhw, dwColor},
								{m_simpleRectVtx[1].x, m_simpleRectVtx[1].y, depth, rhw, dwColor},
								{m_simpleRectVtx[0].x, m_simpleRectVtx[1].y, depth, rhw, dwColor}  };

	static uint16 wIndices[2*3] = {1,0,2, 2,0,3};

	g_pD3DDev->SetFVF(RICE_FVF_FILLRECTVERTEX);
	g_pD3DDev->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, wIndices, D3DFMT_INDEX16, frv, sizeof(FILLRECTVERTEX));
}

void D3DRender::SetTextureUFlag(int dwFlag, uint32 tile)
{
	TileUFlags[tile] = dwFlag;
	if( gRDP.otherMode.cycle_type  >= CYCLE_TYPE_COPY )
	{
		g_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, dwFlag );
	}
	else
	{
		for( int i=0; i<m_curCombineInfo.nStages; i++ )
		{
			if( m_curCombineInfo.stages[i].dwTexture == tile-gRSP.curTile )
			{
				g_pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSU, dwFlag );
			}
		}
	}
}

void D3DRender::SetTextureVFlag(int dwFlag, uint32 tile)
{
	TileVFlags[tile] = dwFlag;
	if( gRDP.otherMode.cycle_type  >= CYCLE_TYPE_COPY )
	{
		g_pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, dwFlag );
	}
	else
	{
		for( int i=0; i<m_curCombineInfo.nStages; i++ )
		{
			if( m_curCombineInfo.stages[i].dwTexture == tile-gRSP.curTile )
			{
				g_pD3DDev->SetSamplerState(i, D3DSAMP_ADDRESSV, dwFlag );
			}
		}
	}
}

void D3DRender::SetAddressUAllStages(uint32 dwTile, int dwFlag)
{
	SetTextureUFlag(dwFlag, dwTile);
}

void D3DRender::SetAddressVAllStages(uint32 dwTile, int dwFlag)
{
	SetTextureVFlag(dwFlag, dwTile);
}

void D3DRender::ZBufferEnable(BOOL bZBuffer)
{
	if( g_curRomInfo.bForceDepthBuffer )
		bZBuffer = TRUE;

	SetZCompare(bZBuffer);
	SetZUpdate(bZBuffer);
}
 
void D3DRender::SetZCompare(BOOL bZCompare)
{
	if( g_curRomInfo.bForceDepthBuffer )
		bZCompare = TRUE;

	gRDP.tnl.Zbuffer = bZCompare;
	m_bZCompare = bZCompare;
	gD3DDevWrapper.SetRenderState(D3DRS_ZENABLE, bZCompare ? D3DZB_TRUE : D3DZB_FALSE );
}

void D3DRender::SetZUpdate(BOOL bZUpdate)
{
	if( g_curRomInfo.bForceDepthBuffer )
		bZUpdate = TRUE;

	m_bZUpdate = bZUpdate;
	if( bZUpdate )	
	{
		gD3DDevWrapper.SetRenderState(D3DRS_ZENABLE,  D3DZB_TRUE );
	}
	gD3DDevWrapper.SetRenderState(D3DRS_ZWRITEENABLE, bZUpdate );
}

void D3DRender::ApplyTextureFilter()
{
	if( gRDP.otherMode.cycle_type  >= CYCLE_TYPE_COPY )
	{
		D3DSetMinFilter( 0, m_dwMinFilter );
		D3DSetMagFilter( 0, m_dwMagFilter );
	}
	else
	{
		for( int i=0; i<m_curCombineInfo.nStages; i++ )
		{
			D3DSetMinFilter( i, m_dwMinFilter );
			D3DSetMagFilter( i, m_dwMagFilter );
		}
	}
}

void D3DRender::SetShadeMode(RenderShadeMode mode)
{
	//If this wasnt our last mode then continue
	if (gRSP.shadeMode != mode)
	{
		//Store our new value
		gRSP.shadeMode = mode;
		if( mode == SHADE_DISABLED || mode == SHADE_FLAT )	//Shade is disabled, use Primitive color for flat shade
		{
			gD3DDevWrapper.SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );
		}
		else
			gD3DDevWrapper.SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	}
}

void D3DRender::SetAlphaRef(uint32 dwAlpha)
{
	//If the alpha value is not the same value as the last one we set, then continue
	if (m_dwAlpha != dwAlpha)
	{
		//Store our new value
		m_dwAlpha = dwAlpha;
		//Set our alpharef state to our new alpha value
		gD3DDevWrapper.SetRenderState(D3DRS_ALPHAREF,dwAlpha);
	}
}

void D3DRender::ForceAlphaRef(uint32 dwAlpha)
{
	gD3DDevWrapper.SetRenderState( D3DRS_ALPHAREF, dwAlpha );	
}

bool D3DRender::SetCurrentTexture(int tile, CTexture *handler, uint32 dwTileWidth, uint32 dwTileHeight, TxtrCacheEntry *pTextureEntry)
{
	RenderTexture &texture = g_textures[tile];
	texture.pTextureEntry = pTextureEntry;

	if( handler != NULL  && texture.m_lpsTexturePtr != handler->GetTexture() )
	{
		if( texture.m_lpsTexturePtr )
		{
			texture.m_lpsTexturePtr->Release();
			texture.m_lpsTexturePtr = NULL;
		}

		texture.m_lpsTexturePtr = handler->GetTexture();
		texture.m_pCTexture = handler;

		if (texture.m_lpsTexturePtr != NULL)
		{
			texture.m_lpsTexturePtr->AddRef();
		}
			
		texture.m_dwTileWidth = dwTileWidth;
		texture.m_dwTileHeight = dwTileHeight;

		if( handler->m_bIsEnhancedTexture )
		{
			texture.m_fTexWidth = (float)pTextureEntry->pTexture->m_dwCreatedTextureWidth;
			texture.m_fTexHeight = (float)pTextureEntry->pTexture->m_dwCreatedTextureHeight;
		}
		else
		{
			texture.m_fTexWidth = (float)handler->m_dwCreatedTextureWidth;
			texture.m_fTexHeight = (float)handler->m_dwCreatedTextureHeight;
		}
	}

	return true;
}

bool D3DRender::SetCurrentTexture(int tile, TxtrCacheEntry *pEntry)
{
	//If the entry isnt null, and the texture is not null then continue
	if (pEntry != NULL && pEntry->pTexture != NULL)
	{	
		// if a hires-texture is available
		// microdev: CHECK for side-effects (e.g. at purgeTextures)
		if(pEntry->pEnhancedTexture != NULL)
			// use the hires tex instead of the original one
			SetCurrentTexture( tile, pEntry->pEnhancedTexture,pEntry->ti.WidthToCreate, pEntry->ti.HeightToCreate, pEntry);
		// no hires replacement
		else
			// so use the original one
			SetCurrentTexture( tile, pEntry->pTexture,pEntry->ti.WidthToCreate, pEntry->ti.HeightToCreate, pEntry);
		return true;
	}
	else
	{
		//No texture exists, set us to an empty non existatn one
		SetCurrentTexture( tile, NULL, 64, 64, NULL );
		return false;
	}
}


void D3DRender::SetFillMode(FillMode mode)
{
	switch( mode )
	{
	case RICE_FILLMODE_WINFRAME:
		//Draw all tri's without any fill, hence wireframe
		gD3DDevWrapper.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME );
		break;
	case RICE_FILLMODE_SOLID:
		//Draw all tri's with fill
		gD3DDevWrapper.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID );
		break;
	}
}

void D3DRender::SetAlphaTestEnable(BOOL bAlphaTestEnable)
{
#ifdef _DEBUG
	gD3DDevWrapper.SetRenderState(D3DRS_ALPHATESTENABLE,(bAlphaTestEnable && debuggerEnableAlphaTest));
#else
	gD3DDevWrapper.SetRenderState(D3DRS_ALPHATESTENABLE,(bAlphaTestEnable));
#endif
}

extern float HackZ(float z);
void D3DRender::SetFogMinMax(float fMin, float fMax)
{
	if( g_curRomInfo.bZHack )
	{
		float minf = HackZ(gRSPfFogMin);
		float maxf = HackZ(gRSPfFogMax);
		gD3DDevWrapper.SetRenderState(D3DRS_FOGSTART, *(uint32 *)(&minf));
		gD3DDevWrapper.SetRenderState(D3DRS_FOGEND,   *(uint32 *)(&maxf));
	}
	else
	{
		gD3DDevWrapper.SetRenderState(D3DRS_FOGSTART, *(uint32 *)(&gRSPfFogMin));
		gD3DDevWrapper.SetRenderState(D3DRS_FOGEND,   *(uint32 *)(&gRSPfFogMax));
	}
	FOG_DUMP(TRACE2("D3D Set Fog: min = %f, max = %f", gRSPfFogMin, gRSPfFogMax));
}

void D3DRender::TurnFogOnOff(bool flag)
{
	gD3DDevWrapper.SetRenderState( D3DRS_FOGENABLE, flag?TRUE:FALSE);
}

#define RSP_ZELDA_CULL_FRONT 0x00000400
void D3DRender::SetFogEnable(bool bEnable)
{
	DEBUGGER_IF_DUMP( (gRDP.tnl.Fog != (bEnable==TRUE) && logFog ), TRACE1("Set Fog %s", bEnable? "enable":"disable"));

	if( options.enableHackForGames == HACK_FOR_TWINE && gRDP.tnl.Fog == FALSE && bEnable == FALSE && (gRDP.tnl.TriCull) )
	{
		g_pD3DDev->Clear(1, NULL, D3DCLEAR_ZBUFFER, 0xFF000000, 1.0, 0);
	}

	DEBUGGER_IF_DUMP(pauseAtNext,{DebuggerAppendMsg("Set Fog %s", bEnable?"enable":"disable");});
	
	//gD3DDevWrapper.SetRenderState( D3DRS_FOGENABLE, FALSE);
	//return;		//Fog does work, need to fix

	if( gRDP.tnl.Fog )
	{
		gD3DDevWrapper.SetRenderState( D3DRS_FOGENABLE, TRUE);
		gD3DDevWrapper.SetRenderState(D3DRS_FOGCOLOR, gRDP.fogColor);
		if( g_curRomInfo.bZHack )
		{
			float minf = HackZ(gRSPfFogMin);
			float maxf = HackZ(gRSPfFogMax);
			gD3DDevWrapper.SetRenderState(D3DRS_FOGSTART, *(uint32 *)(&minf));
			gD3DDevWrapper.SetRenderState(D3DRS_FOGEND,   *(uint32 *)(&maxf));
		}
		else
		{
			gD3DDevWrapper.SetRenderState(D3DRS_FOGSTART, *(uint32 *)(&gRSPfFogMin));
			gD3DDevWrapper.SetRenderState(D3DRS_FOGEND,   *(uint32 *)(&gRSPfFogMax));
		}
	}
	else
	{
		gD3DDevWrapper.SetRenderState( D3DRS_FOGENABLE, FALSE);
	}
}

void D3DRender::SetFogColor(uint32 r, uint32 g, uint32 b, uint32 a)
{
	gRDP.fogColor = COLOR_RGBA(r, g, b, a); 
    gD3DDevWrapper.SetRenderState(D3DRS_FOGCOLOR, gRDP.fogColor);
}

void D3DRender::ClearBuffer(bool cbuffer, bool zbuffer)
{
	float depth = ((gRDP.originalFillColor&0xFFFF)>>2)/(float)0x3FFF;
	if( cbuffer ) g_pD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF000000, depth, 0);
	if( zbuffer ) g_pD3DDev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0xFF000000, depth, 0);
}

void D3DRender::ClearZBuffer(float depth)
{
	g_pD3DDev->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0xFF000000, depth, 0);
}

void D3DRender::ClearBuffer(bool cbuffer, bool zbuffer, D3DRECT &rect)
{
	float depth = ((gRDP.originalFillColor&0xFFFF)>>2)/(float)0x3FFF;
	if( cbuffer ) g_pD3DDev->Clear(1, &rect, D3DCLEAR_TARGET, 0xFF000000, depth, 0);
	if( zbuffer ) g_pD3DDev->Clear(1, &rect, D3DCLEAR_ZBUFFER, 0xFF000000, depth, 0);
}


void D3DRender::UpdateScissor()
{
	if( options.bEnableHacks && g_CI.dwWidth == 0x200 && gRDP.scissor.right == 0x200 && g_CI.dwWidth>(*g_GraphicsInfo.VI_WIDTH_REG & 0xFFF) )
	{
		// Hack for RE2
		uint32 width = *g_GraphicsInfo.VI_WIDTH_REG & 0xFFF;
		uint32 height = (gRDP.scissor.right*gRDP.scissor.bottom)/width;
		D3DVIEWPORT9 vp = {0, 0, (uint32)(width*windowSetting.fMultX), (uint32)(height*windowSetting.fMultY), 0, 1};
		//if( !gRSP.bNearClip )
		//	vp.MinZ = -10000;

		if( vp.Width+vp.X > (DWORD)windowSetting.uDisplayWidth-1) vp.Width = windowSetting.uDisplayWidth-1-vp.X;
		if( vp.Height+vp.Y > (DWORD)windowSetting.uDisplayHeight-1) vp.Height = windowSetting.uDisplayHeight-1-vp.Y;

		gD3DDevWrapper.SetViewport(&vp);
	}
	else
	{
		UpdateScissorWithClipRatio();
	}
}

void D3DRender::ApplyRDPScissor(bool force)
{
	if( !force && status.curScissor == RDP_SCISSOR )
		return;

	if( options.bEnableHacks && g_CI.dwWidth == 0x200 && gRDP.scissor.right == 0x200 && g_CI.dwWidth>(*g_GraphicsInfo.VI_WIDTH_REG & 0xFFF) )
	{
		// Hack for RE2
		uint32 width = *g_GraphicsInfo.VI_WIDTH_REG & 0xFFF;
		uint32 height = (gRDP.scissor.right*gRDP.scissor.bottom)/width;
		D3DVIEWPORT9 vp = {0, 0, (uint32)(width*windowSetting.fMultX), (uint32)(height*windowSetting.fMultY), 0, 1};
		//if( !gRSP.bNearClip )
		//	vp.MinZ = -10000;

		if( vp.Width+vp.X > (DWORD)windowSetting.uDisplayWidth-1) vp.Width = windowSetting.uDisplayWidth-1-vp.X;
		if( vp.Height+vp.Y > (DWORD)windowSetting.uDisplayHeight-1) vp.Height = windowSetting.uDisplayHeight-1-vp.Y;

		gD3DDevWrapper.SetViewport(&vp);
	}
	else
	{
		D3DVIEWPORT9 vp = {
			(uint32)(gRDP.scissor.left*windowSetting.fMultX), 
				(uint32)(gRDP.scissor.top*windowSetting.fMultY), 
				(uint32)((gRDP.scissor.right-gRDP.scissor.left+1)*windowSetting.fMultX), 
				(uint32)((gRDP.scissor.bottom-gRDP.scissor.top+1)*windowSetting.fMultY), 0, 1
		};

		if( vp.Width+vp.X > (DWORD)windowSetting.uDisplayWidth-1) vp.Width = windowSetting.uDisplayWidth-1-vp.X;
		if( vp.Height+vp.Y > (DWORD)windowSetting.uDisplayHeight-1) vp.Height = windowSetting.uDisplayHeight-1-vp.Y;

		//if( !gRSP.bNearClip )
		//	vp.MinZ = -10000;
		gD3DDevWrapper.SetViewport(&vp);
	}

	status.curScissor = RDP_SCISSOR;
}

void D3DRender::ApplyScissorWithClipRatio(bool force)
{
	if( !force && status.curScissor == RSP_SCISSOR )	return;

	WindowSettingStruct &w = windowSetting;
	D3DVIEWPORT9 vp = { w.clipping.left, w.clipping.top, w.clipping.width, w.clipping.height, 0, 1};
	//if( !gRSP.bNearClip )
		//	vp.MinZ = -10000;

	if( vp.Width+vp.X > (DWORD)windowSetting.uDisplayWidth-1) vp.Width = windowSetting.uDisplayWidth-1-vp.X;
	if( vp.Height+vp.Y > (DWORD)windowSetting.uDisplayHeight-1) vp.Height = windowSetting.uDisplayHeight-1-vp.Y;

	gD3DDevWrapper.SetViewport(&vp);
	status.curScissor = RSP_SCISSOR;
}

void D3DRender::BeginRendering(void) 
{
	if( CRender::gRenderReferenceCount == 0 )
	{
		g_pD3DDev->BeginScene();
		D3DXMATRIX mat;
		D3DXMatrixIdentity(&mat);
		g_pD3DDev->SetTransform( D3DTS_WORLD, &mat );
		g_pD3DDev->SetTransform( D3DTS_PROJECTION, &mat );
		g_pD3DDev->SetTransform( D3DTS_VIEW, &mat );
	}

	CRender::gRenderReferenceCount++; 
}

void D3DRender::CaptureScreen(char *filename)
{
	LPDIRECT3DSURFACE9 surface;
	g_pD3DDev->GetRenderTarget(0,&surface);
	((CDXGraphicsContext*)CGraphicsContext::g_pGraphicsContext)->SaveSurfaceToFile(filename, surface, false);
	//D3DXSaveSurfaceToFile(filename,D3DXIFF_BMP,surface,NULL,NULL);
	surface->Release();
	TRACE1("Capture screen to %s", filename);
}

void D3DRender::SetCullMode(bool bCullFront, bool bCullBack)
{
	CRender::SetCullMode(bCullFront, bCullBack);
	/*
	if( bCullFront && bCullBack )
	{
		gD3DDevWrapper.SetRenderState( D3DRS_CULLMODE,   D3DCULL_CW );
		gD3DDevWrapper.SetRenderState( D3DRS_CULLMODE,   D3DCULL_CCW );
	}
	else if( bCullFront )
	{
		gD3DDevWrapper.SetRenderState( D3DRS_CULLMODE,   D3DCULL_CW );
	}
	else if( bCullBack )
	{
		gD3DDevWrapper.SetRenderState( D3DRS_CULLMODE,   D3DCULL_CCW );
	}
	else
	{
		gD3DDevWrapper.SetRenderState( D3DRS_CULLMODE,   D3DCULL_NONE );
	}
	*/
}

void D3DRender::D3DSetMinFilter(uint32 dwStage, uint32 filter)
{
	if( filter == D3DTEXF_LINEAR && options.DirectXAnisotropyValue > 0 )
	{
		// Use Anisotropy filter instead of LINEAR filter
		g_pD3DDev->SetSamplerState( dwStage, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC  );
		g_pD3DDev->SetSamplerState( dwStage, D3DSAMP_MAXANISOTROPY, min(options.DirectXAnisotropyValue, (uint32)CGraphicsContext::m_maxAnisotropy) );
	}
	else
		g_pD3DDev->SetSamplerState( dwStage, D3DSAMP_MINFILTER, filter );
}

void D3DRender::D3DSetMagFilter(uint32 dwStage, uint32 filter)
{
	if( filter == D3DTEXF_LINEAR && options.DirectXAnisotropyValue > 0 )
	{
		// Use Anisotropy filter instead of LINEAR filter
		g_pD3DDev->SetSamplerState( dwStage, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC  );
		g_pD3DDev->SetSamplerState( dwStage, D3DSAMP_MAXANISOTROPY, min(options.DirectXAnisotropyValue, (uint32)CGraphicsContext::m_maxAnisotropy) );
	}
	else
		g_pD3DDev->SetSamplerState( dwStage, D3DSAMP_MAGFILTER, filter );
}

void D3DRender::D3DSetMipFilter(uint32 dwStage, uint32 filter)
{
	if( filter == D3DTEXF_LINEAR && options.DirectXAnisotropyValue > 0 )
	{
		// Use Anisotropy filter instead of LINEAR filter
		g_pD3DDev->SetSamplerState( dwStage, D3DSAMP_MIPFILTER, D3DTEXF_ANISOTROPIC  );
		g_pD3DDev->SetSamplerState( dwStage, D3DSAMP_MAXANISOTROPY, min(options.DirectXAnisotropyValue, (uint32)CGraphicsContext::m_maxAnisotropy) );
	}
	else
		g_pD3DDev->SetSamplerState( dwStage, D3DSAMP_MIPFILTER, filter );
}