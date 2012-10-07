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

//========================================================================
extern MYD3DCAPS g_D3DDeviceCaps;

//Cleanup needs to be a seperate function to ensure that the pixel shaders are cleaned up properly
void CDirectXPixelShaderCombiner::CleanUp(void)
{
	gD3DDevWrapper.SetPixelShader(NULL);
	int n = m_pixelShaderList.size();
	for( int i=0; i<n; i++)
	{
		if( m_pixelShaderList[i].pShaderText )
		{
			delete m_pixelShaderList[i].pShaderText;
			m_pixelShaderList[i].pShaderText = NULL;
		}

#ifdef _XBOX
		g_pD3DDev->DeletePixelShader(m_pixelShaderList[i].dwShaderID);
#else
#if DIRECTX_VERSION == 8
		g_pD3DDev->DeletePixelShader(m_pixelShaderList[i].dwShaderID);
#else
		SAFE_RELEASE(m_pixelShaderList[i].pShader);
#endif
		m_pixelShaderList[i].pVS->Release();
#endif
	}
	m_pixelShaderList.clear();
}

CDirectXPixelShaderCombiner::CDirectXPixelShaderCombiner(CRender *pRender)
	:CColorCombiner(pRender), m_pD3DRender((D3DRender*)pRender)
{

	m_pDecodedMux = new DecodedMuxForPixelShader;
	m_supportedStages = g_D3DDeviceCaps.MaxTextureBlendStages;
}


CDirectXPixelShaderCombiner::~CDirectXPixelShaderCombiner()
{
	CleanUp();
}

void CDirectXPixelShaderCombiner::DisableCombiner(void)
{
	gD3DDevWrapper.SetPixelShader( NULL );

	if(!m_bTex1Enabled || !m_bTex0Enabled || options.bWinFrameMode )
	{
		gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
		gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

		m_pD3DRender->m_curCombineInfo.m_dwShadeColorChannelFlag = 0;
		m_pD3DRender->m_curCombineInfo.m_dwShadeAlphaChannelFlag = 0;
		m_pD3DRender->m_curCombineInfo.specularPostOp = 0;
		m_pD3DRender->m_curCombineInfo.nStages = 1;

		if( m_bTexelsEnable && !options.bWinFrameMode )
		{
			if( m_pDecodedMux->isUsed(MUX_SHADE) )
			{
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );

				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
			}
			else
			{
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );

				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			}

			if( m_bTex0Enabled )
			{
				gD3DDevWrapper.SetTexture( 0, MYLPDIRECT3DTEXTURE(g_textures[gRSP.curTile].m_lpsTexturePtr) );
				m_pD3DRender->SetTexelRepeatFlags(gRSP.curTile);
			}
			else
			{
				gD3DDevWrapper.SetTexture( 0, MYLPDIRECT3DTEXTURE(g_textures[(gRSP.curTile+1)&7].m_lpsTexturePtr) );
				m_pD3DRender->SetTexelRepeatFlags((gRSP.curTile+1)&7);
			}

			m_pD3DRender->m_curCombineInfo.stages[0].bTextureUsed = true;
		}
		else
		{
			gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
			gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );

			gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
			gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
			gD3DDevWrapper.SetTexture( 0, NULL );

			m_pD3DRender->m_curCombineInfo.stages[0].bTextureUsed = false;
		}
	}
	else
	{
		// Both textures are enabled, and we have at least 2 combiner stages

		gD3DDevWrapper.SetTextureStageState( 2, D3DTSS_COLOROP, D3DTOP_DISABLE );
		gD3DDevWrapper.SetTextureStageState( 2, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		m_pD3DRender->m_curCombineInfo.nStages = 2;

		m_pD3DRender->m_curCombineInfo.specularPostOp = 0;

		uint8 mask = ~MUX_COMPLEMENT;

		if( m_pDecodedMux->isUsedInColorChannel(MUX_SHADE,mask) )
		{
			m_pD3DRender->m_curCombineInfo.m_dwShadeColorChannelFlag = 0;
		}
		else if( m_pDecodedMux->isUsedInColorChannel(MUX_PRIM,mask) )
		{
			m_pD3DRender->m_curCombineInfo.m_dwShadeColorChannelFlag = MUX_PRIM;
		}
		else if( m_pDecodedMux->isUsedInColorChannel(MUX_ENV,mask) )
		{
			m_pD3DRender->m_curCombineInfo.m_dwShadeColorChannelFlag = MUX_ENV;
		}

		if( m_pDecodedMux->isUsedInColorChannel(MUX_SHADE|MUX_ALPHAREPLICATE,mask) ||
			m_pDecodedMux->isUsedInAlphaChannel(MUX_SHADE,mask) )
		{
			m_pD3DRender->m_curCombineInfo.m_dwShadeAlphaChannelFlag = 0;
		}
		else if( m_pDecodedMux->isUsedInColorChannel(MUX_PRIM|MUX_ALPHAREPLICATE,mask) ||
				 m_pDecodedMux->isUsedInAlphaChannel(MUX_PRIM,mask) )
		{
			m_pD3DRender->m_curCombineInfo.m_dwShadeAlphaChannelFlag = MUX_PRIM;
		}
		else if( m_pDecodedMux->isUsedInColorChannel(MUX_ENV|MUX_ALPHAREPLICATE,mask) ||
				 m_pDecodedMux->isUsedInAlphaChannel(MUX_ENV,mask) )
		{
			m_pD3DRender->m_curCombineInfo.m_dwShadeAlphaChannelFlag = MUX_ENV;
		}

		if( !options.bWinFrameMode )
		{
			if( m_pDecodedMux->isUsedInColorChannel(MUX_SHADE, mask) || m_pD3DRender->m_curCombineInfo.m_dwShadeColorChannelFlag != 0 )
			{
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
			}
			else if( m_pDecodedMux->isUsedInColorChannel(MUX_SHADE|MUX_ALPHAREPLICATE, mask) ||
				m_pDecodedMux->isUsedInColorChannel(MUX_PRIM|MUX_ALPHAREPLICATE, mask) ||
				m_pDecodedMux->isUsedInColorChannel(MUX_ENV|MUX_ALPHAREPLICATE, mask))
			{
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE|D3DTA_ALPHAREPLICATE );
			}
			else
			{
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			}

			if( m_pDecodedMux->isUsedInAlphaChannel(MUX_SHADE) || m_pD3DRender->m_curCombineInfo.m_dwShadeAlphaChannelFlag != 0 )
			{
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
			}
			else
			{
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
				gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			}

			gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
			gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );

			gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
			gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );

			gD3DDevWrapper.SetTexture( 0, MYLPDIRECT3DTEXTURE(g_textures[gRSP.curTile].m_lpsTexturePtr) );
			gD3DDevWrapper.SetTexture( 1, MYLPDIRECT3DTEXTURE(g_textures[(gRSP.curTile+1)&7].m_lpsTexturePtr) );

			m_pD3DRender->SetTexelRepeatFlags(gRSP.curTile);
			m_pD3DRender->SetTexelRepeatFlags((gRSP.curTile+1)&7);

			m_pD3DRender->m_curCombineInfo.stages[0].bTextureUsed = true;
			m_pD3DRender->m_curCombineInfo.stages[1].bTextureUsed = true;
		}
	}
}

#ifdef _XBOX
bool CDirectXPixelShaderCombiner::Initialize()
{
	gD3DDevWrapper.SetPixelShader( NULL );
	g_pD3DDev->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
	g_pD3DDev->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 1 );
	g_pD3DDev->SetTextureStageState( 2, D3DTSS_TEXCOORDINDEX, 0 );	// Texture 3 uses index as texture 1

	return true;
}

//DWORD CreateTestShader(MYLPDIRECT3DDEVICE g_pD3DDev)
//{
//	D3DPIXELSHADERDEF psd;
//	ZeroMemory( &psd, sizeof(psd) );
//
//	//---------------------------------------------------------------------------
//	//  Texture configuration - The following members of the D3DPixelShaderDef   
//	//  structure define the addressing modes of each of the four texture stages 
//	//---------------------------------------------------------------------------
//	psd.PSTextureModes = PS_TEXTUREMODES( PS_TEXTUREMODES_PROJECT2D,
//		PS_TEXTUREMODES_NONE,
//		PS_TEXTUREMODES_NONE,
//		PS_TEXTUREMODES_NONE );
//
//	psd.PSDotMapping   = PS_DOTMAPPING( 0, 0, 0, 0 );
//	psd.PSInputTexture = PS_INPUTTEXTURE( 0, 0, 1, 0 );
//	psd.PSCompareMode  = PS_COMPAREMODE( 0, 0, 0, 0 );
//
//	//---------------------------------------------------------------------------------
//	//  Color combiners - The following members of the D3DPixelShaderDef structure     
//	//  define the state for the eight stages of color combiners                       
//	//---------------------------------------------------------------------------------
//	psd.PSCombinerCount = PS_COMBINERCOUNT(
//		4,
//		PS_COMBINERCOUNT_MUX_LSB | PS_COMBINERCOUNT_SAME_C0 | PS_COMBINERCOUNT_SAME_C1);
//
//	psd.PSRGBInputs[0] = PS_COMBINERINPUTS(
//		PS_REGISTER_T0    | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB,
//		PS_REGISTER_V0    | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB,
//		PS_REGISTER_ZERO,
//		PS_REGISTER_ZERO );
//
//	psd.PSAlphaInputs[0] = PS_COMBINERINPUTS(
//		PS_REGISTER_T0  | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
//		PS_REGISTER_V0  | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
//		PS_REGISTER_ZERO,
//		PS_REGISTER_ZERO);
//
//	psd.PSRGBOutputs[0] = PS_COMBINEROUTPUTS(
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_R0,
//		PS_COMBINEROUTPUT_IDENTITY | PS_COMBINEROUTPUT_AB_MULTIPLY);
//	psd.PSAlphaOutputs[0] = PS_COMBINEROUTPUTS(
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_R0,
//		PS_COMBINEROUTPUT_IDENTITY | PS_COMBINEROUTPUT_AB_MULTIPLY);
//
//	psd.PSRGBInputs[1] = PS_COMBINERINPUTS(
//		PS_REGISTER_ZERO,
//		PS_REGISTER_R0    | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB,
//		PS_REGISTER_ONE,
//		PS_REGISTER_T1 );
//
//	psd.PSAlphaInputs[1] = PS_COMBINERINPUTS(
//		PS_REGISTER_ZERO,
//		PS_REGISTER_R0  | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
//		PS_REGISTER_ONE,
//		PS_REGISTER_T1);
//
//	psd.PSRGBOutputs[1] = PS_COMBINEROUTPUTS(
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_R0,
//		PS_COMBINEROUTPUT_IDENTITY | PS_COMBINEROUTPUT_AB_MULTIPLY | PS_COMBINEROUTPUT_CD_MULTIPLY | PS_COMBINEROUTPUT_AB_CD_SUM );
//	psd.PSAlphaOutputs[1] = PS_COMBINEROUTPUTS(
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_R0,
//		PS_COMBINEROUTPUT_IDENTITY | PS_COMBINEROUTPUT_AB_MULTIPLY | PS_COMBINEROUTPUT_CD_MULTIPLY | PS_COMBINEROUTPUT_AB_CD_SUM );
//
//	psd.PSRGBInputs[2] = PS_COMBINERINPUTS(
//		PS_REGISTER_ONE,
//		PS_REGISTER_V0    | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB,
//		PS_REGISTER_ZERO,
//		PS_REGISTER_ZERO );
//
//	psd.PSAlphaInputs[2] = PS_COMBINERINPUTS(
//		PS_REGISTER_ONE,
//		PS_REGISTER_V0  | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
//		PS_REGISTER_ZERO,
//		PS_REGISTER_ZERO);
//
//	psd.PSRGBOutputs[2] = PS_COMBINEROUTPUTS(
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_R0,
//		PS_COMBINEROUTPUT_IDENTITY | PS_COMBINEROUTPUT_AB_MULTIPLY);
//	psd.PSAlphaOutputs[2] = PS_COMBINEROUTPUTS(
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_R0,
//		PS_COMBINEROUTPUT_IDENTITY | PS_COMBINEROUTPUT_AB_MULTIPLY);
//
//	psd.PSRGBInputs[3] = PS_COMBINERINPUTS(
//		PS_REGISTER_T0    | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB,
//		PS_REGISTER_V0    | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB,
//		PS_REGISTER_ZERO,
//		PS_REGISTER_ZERO );
//
//	psd.PSAlphaInputs[3] = PS_COMBINERINPUTS(
//		PS_REGISTER_T0  | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
//		PS_REGISTER_V0  | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
//		PS_REGISTER_ZERO,
//		PS_REGISTER_ZERO);
//
//	psd.PSRGBOutputs[3] = PS_COMBINEROUTPUTS(
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_R0,
//		PS_COMBINEROUTPUT_IDENTITY | PS_COMBINEROUTPUT_AB_MULTIPLY);
//	psd.PSAlphaOutputs[3] = PS_COMBINEROUTPUTS(
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_DISCARD,
//		PS_REGISTER_R0,
//		PS_COMBINEROUTPUT_IDENTITY | PS_COMBINEROUTPUT_AB_MULTIPLY);
//
//
//	// AB + (1-A)C + D
//	psd.PSFinalCombinerInputsABCD = PS_COMBINERINPUTS(
//		PS_REGISTER_ZERO,
//		PS_REGISTER_R0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB,
//		PS_REGISTER_R0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_RGB,
//		PS_REGISTER_ZERO);
//
//	psd.PSFinalCombinerInputsEFG = PS_COMBINERINPUTS(
//		PS_REGISTER_ZERO,
//		PS_REGISTER_ZERO,
//		PS_REGISTER_R0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
//		PS_FINALCOMBINERSETTING_CLAMP_SUM);
//
//	psd.PSFinalCombinerConstants = PS_FINALCOMBINERCONSTANTS(0,0,PS_GLOBALFLAGS_NO_TEXMODE_ADJUST);
//
//	DWORD shader;
//	g_pD3DDev->CreatePixelShader( &psd, &shader );
//	return shader;
//}

#define PS_ALPHA	PS_CHANNEL_ALPHA
#define PS_RGB		PS_CHANNEL_RGB
#define PS_ONE		PS_REGISTER_ONE
#define PS_ZERO		PS_REGISTER_ZERO
#define PS_R0		PS_REGISTER_R0
#define PS_R1		PS_REGISTER_R1
#define PS_T0		PS_REGISTER_T0
#define PS_T1		PS_REGISTER_T1
#define PS_SHADE	PS_REGISTER_V0
#define PS_PRIM		PS_REGISTER_C0
#define PS_ENV		PS_REGISTER_C1
#define PS_PRIMLOD	PS_REGISTER_T2
#define PS_LODFAC	PS_REGISTER_T2
#define PS_COMB		PS_REGISTER_R0
#define PS_NEG		PS_INPUTMAPPING_SIGNED_NEGATE
#define PS_UNSIGN	PS_INPUTMAPPING_UNSIGNED_IDENTITY
#define PS_NULL		PS_REGISTER_DISCARD

#define NORMALOUTPUT	(PS_COMBINEROUTPUT_IDENTITY | PS_COMBINEROUTPUT_AB_CD_SUM | PS_COMBINEROUTPUT_AB_MULTIPLY | PS_COMBINEROUTPUT_CD_MULTIPLY)

DWORD muxToPSMaps[][2] = {
	PS_ZERO|PS_RGB,				PS_ZERO|PS_ALPHA,			//MUX_0 = 0,
	PS_ONE|PS_RGB,				PS_ONE|PS_ALPHA,			//MUX_1,
	PS_COMB|PS_RGB,				PS_COMB|PS_ALPHA,			//MUX_COMBINED,
	PS_T0|PS_RGB,				PS_T0|PS_ALPHA,				//MUX_TEXEL0,
	PS_T1|PS_RGB,				PS_T1|PS_ALPHA,				//MUX_TEXEL1,
	PS_PRIM|PS_RGB,				PS_PRIM|PS_ALPHA,			//MUX_PRIM,
	PS_SHADE|PS_RGB,			PS_SHADE|PS_ALPHA,			//MUX_SHADE,
	PS_ENV|PS_RGB,				PS_ENV|PS_ALPHA,			//MUX_ENV,

	PS_COMB|PS_ALPHA,			PS_COMB|PS_ALPHA,			//MUX_COMBALPHA,		// Should not be used
	PS_T0|PS_ALPHA,				PS_T0|PS_ALPHA,				//MUX_T0_ALPHA,			// Should not be used
	PS_T1|PS_ALPHA,				PS_T1|PS_ALPHA,				//MUX_T1_ALPHA,			// Should not be used
	PS_PRIM|PS_ALPHA,			PS_PRIM|PS_ALPHA,			//MUX_PRIM_ALPHA,		// Should not be used
	PS_SHADE|PS_ALPHA,			PS_SHADE|PS_ALPHA,			//MUX_SHADE_ALPHA,		// Should not be used
	PS_ENV|PS_ALPHA,			PS_ENV|PS_ALPHA,			//MUX_ENV_ALPHA,		// Should not be used

	PS_LODFAC|PS_ALPHA,			PS_LODFAC|PS_ALPHA,			//MUX_LODFRAC,
	PS_PRIMLOD|PS_ALPHA,		PS_PRIMLOD|PS_ALPHA,		//MUX_PRIMLODFRAC,
	PS_ONE|PS_RGB,				PS_ONE|PS_ALPHA,			//MUX_K5,
	PS_ZERO|PS_RGB,				PS_ZERO|PS_ALPHA,			//MUX_UNK,				// Should not be used
};

DWORD MuxToC(uint8 val)
{
	// For color channel
	if( val&MUX_ALPHAREPLICATE )
	{
		return muxToPSMaps[val&0x1F][1];
	}
	else
	{
		return muxToPSMaps[val&0x1F][0];
	}
}

DWORD MuxToA(uint8 val)
{
	// For alpha channel
	return muxToPSMaps[val&0x1F][1];
}

void Process1Stage(DecodedMux &mux, int cycle, int channel, DWORD &out1, DWORD &out2)
{
	int pos = cycle*2+channel;
	N64CombinerType &m = mux.m_n64Combiners[cycle*2+channel];
	CombinerFormatType type = mux.splitType[cycle*2+channel];

	DWORD (*func)(BYTE) = (channel == COLOR_CHANNEL ? MuxToC : MuxToA);
	DWORD flag = (channel == COLOR_CHANNEL ? PS_RGB : PS_ALPHA);

	switch( type )
	{
	case CM_FMT_TYPE_NOT_USED:
		out1 = PS_COMBINERINPUTS( PS_ONE, PS_ONE, PS_ZERO, PS_ZERO );
		out2 = PS_COMBINERINPUTS( PS_R0|flag, PS_ONE, PS_ZERO, PS_ZERO );
		break;
	case CM_FMT_TYPE_D:
		out1 = PS_COMBINERINPUTS( func(m.d), PS_ONE, PS_ZERO, PS_ZERO );
		out2 = PS_COMBINERINPUTS( func(m.d), PS_ONE, PS_ZERO, PS_ZERO );
		break;
	case CM_FMT_TYPE_A_MOD_C:
		out1 = PS_COMBINERINPUTS( PS_ONE, PS_ONE, PS_ZERO, PS_ZERO );
		out2 = PS_COMBINERINPUTS( func(m.a), func(m.c), PS_ZERO, PS_ZERO );
		break;
	case CM_FMT_TYPE_A_ADD_D:
		out1 = PS_COMBINERINPUTS( PS_ONE, PS_ONE, PS_ZERO, PS_ZERO );
		out2 = PS_COMBINERINPUTS( func(m.a), PS_ONE, func(m.d), PS_ONE );
		break;
	case CM_FMT_TYPE_A_SUB_B:
		out1 = PS_COMBINERINPUTS( PS_ONE, PS_ONE, PS_ZERO, PS_ZERO );
		out2 = PS_COMBINERINPUTS( func(m.a), PS_ONE, func(m.b)|PS_NEG, PS_ONE );
		break;
	case CM_FMT_TYPE_A_MOD_C_ADD_D:
		out1 = PS_COMBINERINPUTS( PS_ONE, PS_ONE, PS_ZERO, PS_ZERO );
		out2 = PS_COMBINERINPUTS( func(m.a), func(m.c), func(m.d), PS_ONE );
		break;
	//case CM_FMT_TYPE_A_LERP_B_C:
	//	out1 = PS_COMBINERINPUTS( PS_ONE, PS_ONE, PS_ZERO, PS_ZERO );
	//	out2 = PS_COMBINERINPUTS( func(m.a), func(m.c), func(m.b), func(m.c)|PS_INPUTMAPPING_UNSIGNED_INVERT );
	//	break;
	case CM_FMT_TYPE_A_SUB_B_MOD_C:
		out1 = PS_COMBINERINPUTS( PS_ONE, PS_ONE, PS_ZERO, PS_ZERO );
		out2 = PS_COMBINERINPUTS( func(m.a), func(m.c), func(m.b)|PS_NEG, func(m.c) );
		break;
	default:
		out1 = PS_COMBINERINPUTS( func(m.a), PS_ONE, func(m.b), PS_REGISTER_NEGATIVE_ONE );	// R0=A-B
		out2 = PS_COMBINERINPUTS( PS_R1|flag|PS_INPUTMAPPING_SIGNED_IDENTITY, func(m.c), func(m.d), PS_ONE );				// R0*C+D = (A-B)*C+D
		break;
	}
}

/************************************************************************/
int CDirectXPixelShaderCombiner::GeneratePixelShaderFromMux(void)
{
	/************************************************************************/
	/*
	/*    STEP 1: generate pixel shader text according to current MUX
	/*
	/* Ideas of implementation:
	/* - 8 arithmetic instructions are just enough to simulate N64 combiners
	/*   in such a way:
	/*      For color channel
	/*			r0.rgb = a1-b1
	/*      	r0.rgb = r0*c1+d1
	/*      	r0.rgb = a2-b2
	/*      	r0.rgb = r0*c2+dd
	/*      For alpha channel
	/*			r0.a = a1-b1
	/*      	r0.a = r0*c1+d1
	/*      	r0.a = a2-b2
	/*      	r0.a = r0*c2+dd
	/*
	/* - Constant definition:
	/*   c0: = 0	will never change
	/*   c1: = 1	will never change
	/*   c2: = MUX_PRIM
	/*   c3: = MUX_ENV
	/*   c4: = MUX_LOADFRAC
	/*   c5: = MUX_PRIMLOADPRAC
	/*
	/* - Will use register r0 as MUX_COMB, never changes
	/*
	/************************************************************************/

	DecodedMux &m = *m_pDecodedMux;
	PixelShaderEntry newEntry;
	newEntry.mux64 = m_pD3DRender->m_Mux;
	newEntry.bPrimLOD = m.isUsed(MUX_PRIMLODFRAC) || m.isUsed(MUX_LODFRAC);

	m.splitType[0] = m.splitType[1] = m.splitType[2] = m.splitType[3] = CM_FMT_TYPE_NOT_CHECKED;
	m_pDecodedMux->Reformat(false);

	newEntry.bFog = gRDP.bFogEnableInBlender && gRSP.bFogEnabled;


	//--------------------------
	// Define a XBOX Geforce 3 pixel shader
	//--------------------------
	D3DPIXELSHADERDEF psd;
	ZeroMemory( &psd, sizeof(psd) );

	DWORD t0flag = m_bTex0Enabled ? PS_TEXTUREMODES_PROJECT2D : PS_TEXTUREMODES_NONE;
	DWORD t1flag = m_bTex1Enabled ? PS_TEXTUREMODES_PROJECT2D : PS_TEXTUREMODES_NONE;
	//DWORD t2flag = newEntry.bPrimLOD ? PS_TEXTUREMODES_PROJECT2D : PS_TEXTUREMODES_NONE;
	DWORD t2flag = newEntry.bPrimLOD ? PS_TEXTUREMODES_NONE : PS_TEXTUREMODES_NONE;


	// The pixel shader uses 4 combiner stages
	psd.PSTextureModes  = PS_TEXTUREMODES( t0flag, t1flag, t2flag, PS_TEXTUREMODES_NONE );
	psd.PSInputTexture  = PS_INPUTTEXTURE( 0, 0, 0, 0 );
	psd.PSDotMapping    = PS_DOTMAPPING( 0, 0, 0, 0);
	psd.PSCompareMode   = PS_COMPAREMODE( 0, 0, 0, 0 );

	psd.PSCombinerCount = PS_COMBINERCOUNT( 4,
		PS_COMBINERCOUNT_MUX_LSB | PS_COMBINERCOUNT_SAME_C0 | PS_COMBINERCOUNT_SAME_C1 );
	psd.PSC0Mapping     = PS_CONSTANTMAPPING(0,0,0,0,0,0,0,0);
	psd.PSC1Mapping     = PS_CONSTANTMAPPING(1,1,1,1,1,1,1,1);


	////--------------------------
	//// Stage 0:	R0 = A*1+(-B)*1 = A-B
	////--------------------------

	//psd.PSRGBInputs   [0] = PS_COMBINERINPUTS( MuxToC(m.aRGB0), PS_ONE, MuxToC(m.bRGB0)|PS_NEG, PS_ONE );
	//psd.PSAlphaInputs [0] = PS_COMBINERINPUTS( MuxToA(m.aA0),   PS_ONE, MuxToA(m.bA0)|PS_NEG,   PS_ONE );
	psd.PSRGBOutputs  [0] = PS_COMBINEROUTPUTS( PS_NULL, PS_NULL, PS_R1, NORMALOUTPUT );
	psd.PSAlphaOutputs[0] = PS_COMBINEROUTPUTS( PS_NULL, PS_NULL, PS_R1, NORMALOUTPUT );

	////--------------------------
	//// Stage 1: R0 = R0*C+D*1 = (A-B)*C+D
	////--------------------------

	//psd.PSRGBInputs   [1] = PS_COMBINERINPUTS( PS_R1|PS_RGB,   MuxToC(m.cRGB0), MuxToC(m.dRGB0), PS_ONE );
	//psd.PSAlphaInputs [1] = PS_COMBINERINPUTS( PS_R1|PS_ALPHA, MuxToA(m.cA0),   MuxToA(m.dA0),   PS_ONE );
	psd.PSRGBOutputs  [1] = PS_COMBINEROUTPUTS( PS_NULL, PS_NULL, PS_R0, NORMALOUTPUT );
	psd.PSAlphaOutputs[1] = PS_COMBINEROUTPUTS( PS_NULL, PS_NULL, PS_R0, NORMALOUTPUT );


	////--------------------------
	//// Stage 2: R1 = A*1+(-B)*1 = A-B
	////--------------------------

	//psd.PSRGBInputs   [2] = PS_COMBINERINPUTS( MuxToC(m.aRGB1), PS_ONE, MuxToC(m.bRGB1)|PS_NEG, PS_ONE );
	//psd.PSAlphaInputs [2] = PS_COMBINERINPUTS( MuxToA(m.aA1),   PS_ONE, MuxToA(m.bA1)|PS_NEG,   PS_ONE );
	psd.PSRGBOutputs  [2] = PS_COMBINEROUTPUTS( PS_NULL, PS_NULL, PS_R1, NORMALOUTPUT );
	psd.PSAlphaOutputs[2] = PS_COMBINEROUTPUTS( PS_NULL, PS_NULL, PS_R1, NORMALOUTPUT );

	////--------------------------
	//// Stage 3: R0 = R1*C+D*1 = (A-B)*C+D
	////--------------------------

	//psd.PSRGBInputs   [3] = PS_COMBINERINPUTS( PS_R1|PS_RGB,   MuxToC(m.cRGB1), MuxToC(m.dRGB1), PS_ONE );
	//psd.PSAlphaInputs [3] = PS_COMBINERINPUTS( PS_R1|PS_ALPHA, MuxToA(m.cA1),   MuxToA(m.dA1),   PS_ONE );
	psd.PSRGBOutputs  [3] = PS_COMBINEROUTPUTS( PS_NULL, PS_NULL, PS_R0, NORMALOUTPUT );
	psd.PSAlphaOutputs[3] = PS_COMBINEROUTPUTS( PS_NULL, PS_NULL, PS_R0, NORMALOUTPUT );


	Process1Stage(m, 0, COLOR_CHANNEL, psd.PSRGBInputs[0],   psd.PSRGBInputs[1]);
	Process1Stage(m, 0, ALPHA_CHANNEL, psd.PSAlphaInputs[0], psd.PSAlphaInputs[1]);
	Process1Stage(m, 1, COLOR_CHANNEL, psd.PSRGBInputs[2],   psd.PSRGBInputs[3]);
	Process1Stage(m, 1, ALPHA_CHANNEL, psd.PSAlphaInputs[2], psd.PSAlphaInputs[3]);

	if( newEntry.bFog )
	{
		// Fog is used
		//--------------------------
		// Final combiner: rgbout   = FogFactor*R0 + (1-FogFactor)*FogColor + 0 = A*B + (1-A)*C + D
		// Final combiner: alphaout = alpha of R0
		//--------------------------
		psd.PSFinalCombinerInputsABCD=PS_COMBINERINPUTS(
			PS_REGISTER_FOG  | PS_CHANNEL_ALPHA | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_R0   | PS_CHANNEL_RGB   | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_FOG  | PS_CHANNEL_RGB   | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_ZERO | PS_CHANNEL_RGB   | PS_INPUTMAPPING_UNSIGNED_IDENTITY );

		// E = 0, F = 0, G = 1. (From above, EF is not used. G is alpha and is set to 1.)
		psd.PSFinalCombinerInputsEFG = PS_COMBINERINPUTS(
			PS_REGISTER_ZERO,
			PS_REGISTER_ZERO,
			PS_REGISTER_R0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
			PS_FINALCOMBINERSETTING_CLAMP_SUM);
	}
	else
	{
		// No fog
		//--------------------------
		// Final combiner: rgbout   = 1*R0 + (1-1)*0 + 0 = R0
		// Final combiner: alphaout = alpha of R0
		//--------------------------
		psd.PSFinalCombinerInputsABCD=PS_COMBINERINPUTS(
			PS_REGISTER_ONE  | PS_CHANNEL_RGB | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_R0   | PS_CHANNEL_RGB | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_ZERO | PS_CHANNEL_RGB | PS_INPUTMAPPING_UNSIGNED_IDENTITY,
			PS_REGISTER_ZERO | PS_CHANNEL_RGB | PS_INPUTMAPPING_UNSIGNED_IDENTITY );

		// E = 0, F = 0, G = 1. (From above, EF is not used. G is alpha and is set to 1.)
		psd.PSFinalCombinerInputsEFG = PS_COMBINERINPUTS(
			PS_REGISTER_ZERO,
			PS_REGISTER_ZERO,
			PS_REGISTER_R0 | PS_INPUTMAPPING_UNSIGNED_IDENTITY | PS_CHANNEL_ALPHA,
			PS_FINALCOMBINERSETTING_CLAMP_SUM);
	}

	psd.PSFinalCombinerConstants = PS_FINALCOMBINERCONSTANTS(0,0,PS_GLOBALFLAGS_NO_TEXMODE_ADJUST);

	//newEntry.dwShaderID = CreateTestShader(g_pD3DDev);
	g_pD3DDev->CreatePixelShader( &psd, (DWORD*)&(newEntry.dwShaderID));

	newEntry.pShaderText = NULL;
	newEntry.bPrimLOD = m.isUsed(MUX_PRIMLODFRAC) || m.isUsed(MUX_LODFRAC);
	m_pixelShaderList.push_back(newEntry);

	return m_pixelShaderList.size()-1;
}
#else

char *muxToPSMaps[][2] = {
	"c0",	"c0",	//MUX_0 = 0,
	"c1",	"c1",	//MUX_1,
	"r1",	"r1.a",	//MUX_COMBINED,
	"t0",	"t0.a",	//MUX_TEXEL0,
	"t1",	"t1.a",	//MUX_TEXEL1,
	"c2",	"c2.a",	//MUX_PRIM,
	"v0",	"v0.a",	//MUX_SHADE,
	"c3",	"c3.a",	//MUX_ENV,

	"r1.a",	"r1.a",	//MUX_COMBALPHA,		// Should not be used
	"t0.a",	"t0.a",	//MUX_T0_ALPHA,			// Should not be used
	"t1.a",	"t1.a",	//MUX_T1_ALPHA,			// Should not be used
	"c2.a",	"c2.a",	//MUX_PRIM_ALPHA,		// Should not be used
	"v0.a",	"v0.a",	//MUX_SHADE_ALPHA,		// Should not be used
	"c3.a",	"c3.a",	//MUX_ENV_ALPHA,		// Should not be used

	"c4",	"c4",	//MUX_LODFRAC,
	"c5",	"c5",	//MUX_PRIMLODFRAC,
	"c1",	"c1",	//MUX_K5,

	"c0",	"c0",	//MUX_UNK,				// Should not be used
};

char* MuxToC(uint8 val)
{
	// For color channel
	if( val&MUX_ALPHAREPLICATE )
	{
		return muxToPSMaps[val&0x1F][1];
	}
	else
	{
		return muxToPSMaps[val&0x1F][0];
	}
}

char* MuxToA(uint8 val)
{
	// For alpha channel
	return muxToPSMaps[val&0x1F][0];
}

bool CDirectXPixelShaderCombiner::Initialize(void)
{
//
//	gD3DDevWrapper.SetPixelShader( NULL );
//	m_pDecodedMux = new DecodedMuxForPixelShader;
	return true;
}

/************************************************************************/
/* To generate a new pixel shader from current mux                
/*                                                                
/* Support pixel shader version 1.1 only since it is supported by 
/* all pixel shader enabled hardwares                             
/*                                                                
/* Limits:
/*   -	For ps version 1.1
/*		max 8 arithmetic instructions                              
/*		max 4 texture address instructions                           
/*   - For ps version 1.4 - If we can use it
/*      max 16 arithmetic instructions in 2 phases, 8 in each phase
/*   - For ps version 2.0 - If we can use it
/*      max 64 arithmetic instructions
/*                                                                
/* Others info.                                                   
/*   - temp reg r0 is the output register                         
/*   - v0, v1                                                     
/*   - c0-c7                                                      
/*   - t0-t3                                                      
/*                                                                
/* Arithmetic Instructions:                                 
/*   - add dest, src0, src1			: dest=src0+src1					
/*   - dp3 dest, src0, src1                                       
/*   - lrp dest, src0, src1, src2	: dest=src0 * src1 + (1-src0) * src2, dest=src2 + src0 * (src1 - src2)
/*   - mad dest, src0, src1, src2	: dest = src0 * src1 + src2
/*   - mov dest, src
/*   - mul dest, src0, src1
/*   - sub dest, src0, src1			: dest = src0 - src1
/*   - nop
/*
/*  Instructions:                                                               
/*   - tex                                                              
/*
/*  Source Register Modifiers:
/*   - Invert:		1 - source register
/*	 - Negate:		- source register
/*  
/*  Source Register Selectors:
/*   - Blue replicate:		.b
/*   - Alpha replicate:		.a
/*
/*  Destination Reigster Selectors:
/*   - destination register.rgba
/*   - destination register.rgb 
/*   - destination register.a
/*   
/*  Other instructions:
/*   - ps.1.1
/*   - def c0-c7, fVal0, fVal1, fVal2, fVal3
/*
/************************************************************************/
int CDirectXPixelShaderCombiner::GeneratePixelShaderFromMux(void)
{
	char buf[100];

	/************************************************************************/
	/*
	/*    STEP 1: generate pixel shader text according to current MUX
	/*
	/* Ideas of implementation:
	/* - 8 arithmetic instructions are just enough to simulate N64 combiners
	/*   in such a way:
	/*      For color channel
	/*			r0.rgb = a1-b1
	/*      	r0.rgb = r0*c1+d1
	/*      	r0.rgb = a2-b2
	/*      	r0.rgb = r0*c2+dd
	/*      For alpha channel
	/*			r0.a = a1-b1
	/*      	r0.a = r0*c1+d1
	/*      	r0.a = a2-b2
	/*      	r0.a = r0*c2+dd
	/*
	/* - Constant definition:
	/*   c0: = 0	will never change
	/*   c1: = 1	will never change
	/*   c2: = MUX_PRIM
	/*   c3: = MUX_ENV
	/*   c4: = MUX_LOADFRAC
	/*   c5: = MUX_PRIMLOADPRAC
	/*
	/* - Will use register r0 as MUX_COMB, never changes
	/*
	/************************************************************************/

	DecodedMux &m = *m_pDecodedMux;

	m_textBuf[0] = 0;
	strcat(m_textBuf, "ps.1.1\n");
	strcat(m_textBuf, "def c0, 0.0, 0.0, 0.0, 0.0\n");
	strcat(m_textBuf, "def c1, 1.0, 1.0, 1.0, 1.0\n");
	if( m.m_bTexel0IsUsed ) strcat(m_textBuf, "tex t0\n");
	if( m.m_bTexel1IsUsed ) strcat(m_textBuf, "tex t1\n");

	// Color channel 1
	sprintf(buf, "sub r1.rgb,     %s, %s\n", MuxToC(m.aRGB0), MuxToC(m.bRGB0) );	strcat(m_textBuf, buf);
	sprintf(buf, "mad_sat r1.rgb, r1, %s, %s\n", MuxToC(m.cRGB0), MuxToC(m.dRGB0) );	strcat(m_textBuf, buf);

	// Alpha channel 1
	sprintf(buf, "sub r1.a,     %s, %s\n", MuxToA(m.aA0), MuxToA(m.bA0) );	strcat(m_textBuf, buf);
	sprintf(buf, "mad_sat r1.a, r1, %s, %s\n", MuxToA(m.cA0), MuxToA(m.dA0) );	strcat(m_textBuf, buf);

	// Color channel 2
	sprintf(buf, "sub r0.rgb,     %s, %s\n", MuxToC(m.aRGB1), MuxToC(m.bRGB1) );	strcat(m_textBuf, buf);
	sprintf(buf, "mad_sat r0.rgb, r0, %s, %s\n", MuxToC(m.cRGB1), MuxToC(m.dRGB1) );	strcat(m_textBuf, buf);


	// Alpha channel 2
	sprintf(buf, "sub r0.a,     %s, %s\n", MuxToA(m.aA1), MuxToA(m.bA1) );	strcat(m_textBuf, buf);
	sprintf(buf, "mad_sat r0.a, r0, %s, %s\n", MuxToA(m.cA1), MuxToA(m.dA1) );	strcat(m_textBuf, buf);

	// Step 2: Compile the shade text to generate a new pixel shader binary

	PixelShaderEntry newEntry;
	newEntry.mux64 = m_pD3DRender->m_Mux;

#if DIRECTX_VERSION == 8
	HRESULT e =D3DXAssembleShader( m_textBuf, strlen(m_textBuf),  0, NULL, &(newEntry.pVS), NULL );
#else
	HRESULT e =D3DXAssembleShader( m_textBuf, strlen(m_textBuf),  0, NULL, NULL, &(newEntry.pVS), NULL );
#endif
	if( e != S_OK )
	{
#ifdef _DEBUG
		TRACE0("Error to assemble shader");
		TRACE0(m_textBuf);
		DisplaySimpleMuxString();
#endif
	}

#if DIRECTX_VERSION == 8
	e = g_pD3DDev->CreatePixelShader( (DWORD*)newEntry.pVS->GetBufferPointer(), (DWORD*)&(newEntry.dwShaderID) );
#else
	e = g_pD3DDev->CreatePixelShader( (DWORD*)newEntry.pVS->GetBufferPointer(), &(newEntry.pShader) );
#endif
	if( e != S_OK )
	{
		TRACE0("Error to create shader");
		TRACE0(m_textBuf);
		newEntry.dwShaderID = 0;
#if DIRECTX_VERSION > 8
		newEntry.pShader = NULL;
#endif
	}
	
#ifdef _DEBUG
	newEntry.pShaderText = new char[strlen(m_textBuf)+1];
	strcpy(newEntry.pShaderText, m_textBuf);
#else
	newEntry.pShaderText = NULL;
#endif

	m_pixelShaderList.push_back(newEntry);

	return m_pixelShaderList.size()-1;
}
#endif


int CDirectXPixelShaderCombiner::FindCompiledShader(void)
{
	int n = m_pixelShaderList.size();
	bool found=false;

#ifdef _XBOX
	bool bFog = gRDP.bFogEnableInBlender && gRSP.bFogEnabled;
#endif

	for( int i=0; i<n; i++ )
	{
#ifdef _XBOX
		if( m_pixelShaderList[i].mux64 == m_pD3DRender->m_Mux && bFog == m_pixelShaderList[i].bFog )
#else
		if( m_pixelShaderList[i].mux64 == m_pD3DRender->m_Mux )
#endif
		{
			found = true;
			return i;
		}
	}

	return -1;
}


void CDirectXPixelShaderCombiner::InitCombinerCycleCopy(void)
{
	gD3DDevWrapper.SetPixelShader( NULL );
	gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

	gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
	gD3DDevWrapper.SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
	
	m_pD3DRender->m_curCombineInfo.stages[0].bTextureUsed = true;
	m_pD3DRender->m_curCombineInfo.nStages = 1;

	gD3DDevWrapper.SetTexture( 0, MYLPDIRECT3DTEXTURE(g_textures[gRSP.curTile].m_lpsTexturePtr) );
	m_pD3DRender->SetTexelRepeatFlags(gRSP.curTile);
	gD3DDevWrapper.SetTexture( 1, NULL );
	gD3DDevWrapper.SetPixelShader( NULL );

	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );

	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
}

void CDirectXPixelShaderCombiner::InitCombinerCycle12(void)
{
	// Step 1: set pixel shader
	int idx = FindCompiledShader();
	if( idx < 0 )	idx = GeneratePixelShaderFromMux();

#ifdef _XBOX
	g_pD3DDev->SetPixelShader(m_pixelShaderList[idx].dwShaderID);

	// Step 2: set constant colors
	//float zeros[] = {0.0f,0.0f,0.0f,0.0f};
	//float ones[] = {1.0f,1.0f,1.0f,1.0f};
	float *pf;
	pf = GetPrimitiveColorfv();
	gD3DDevWrapper.SetPixelShaderConstant(0,pf);

	pf = GetEnvColorfv();
	gD3DDevWrapper.SetPixelShaderConstant(1,pf);

	if( m_pixelShaderList[idx].bPrimLOD )
	{
		gRDP.primLODFrac = 255;
		TxtrCacheEntry *entry = gTextureManager.GetConstantColorTexture(MUX_PRIMLODFRAC);
		g_pD3DDev->SetTexture( 2, MYLPDIRECT3DTEXTURE(entry->pTexture->GetTexture()));
	}
#else
#if DIRECTX_VERSION == 8
	gD3DDevWrapper.SetPixelShader(m_pixelShaderList[idx].dwShaderID);
#else
	gD3DDevWrapper.SetPixelShader(m_pixelShaderList[idx].pShader);
#endif

	// Step 2: set constant colors
	float *pf;
	pf = GetPrimitiveColorfv();
	gD3DDevWrapper.SetPixelShaderConstant(2,pf);

	pf = GetEnvColorfv();
	gD3DDevWrapper.SetPixelShaderConstant(3,pf);

	float frac = gRDP.LODFrac / 255.0f;
	float tempf[4] = {frac,frac,frac,frac};
	gD3DDevWrapper.SetPixelShaderConstant(4,tempf);

	float frac2 = gRDP.primLODFrac / 255.0f;
	float tempf2[4] = {frac2,frac2,frac2,frac2};
	gD3DDevWrapper.SetPixelShaderConstant(5,tempf2);
#endif

	m_pD3DRender->m_curCombineInfo.nStages = 2;
	m_pD3DRender->m_curCombineInfo.stages[0].dwTexture = 0;
	m_pD3DRender->m_curCombineInfo.stages[1].dwTexture = 1;
	m_pD3DRender->m_curCombineInfo.stages[0].bTextureUsed = m_bTex0Enabled;
	m_pD3DRender->m_curCombineInfo.stages[1].bTextureUsed = m_bTex1Enabled;

	// Step 3: set textures
	if( m_bTex0Enabled ) 
	{
		m_pD3DRender->SetTexelRepeatFlags(gRSP.curTile);
		if( g_textures[gRSP.curTile].m_pCTexture != NULL )
			gD3DDevWrapper.SetTexture( 0, MYLPDIRECT3DTEXTURE(g_textures[gRSP.curTile].m_lpsTexturePtr) );
	}

	if( m_bTex1Enabled )
	{
		m_pD3DRender->SetTexelRepeatFlags((gRSP.curTile+1)&7);
		if( g_textures[(gRSP.curTile+1)&7].m_pCTexture != NULL )
			gD3DDevWrapper.SetTexture( 1, MYLPDIRECT3DTEXTURE(g_textures[(gRSP.curTile+1)&7].m_lpsTexturePtr) );
	}

	gRDP.texturesAreReloaded = false;
}

void CDirectXPixelShaderCombiner::InitCombinerCycleFill(void)
{
	gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

	m_pD3DRender->m_curCombineInfo.stages[0].bTextureUsed = true;

	gD3DDevWrapper.SetTexture( 0, NULL );
	gD3DDevWrapper.SetTexture( 1, NULL );
	gD3DDevWrapper.SetPixelShader( NULL );

	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );

	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
}

D3DCOLOR D3DRender::PostProcessDiffuseColor(D3DCOLOR curDiffuseColor)
{
	uint32 mask = (m_curCombineInfo.blendingFunc != DISABLE_COLOR?0xFFFFFFFF:0xFF000000);

	if( m_curCombineInfo.m_dwShadeColorChannelFlag + m_curCombineInfo.m_dwShadeAlphaChannelFlag == 0 )
	{
		return (curDiffuseColor&mask);
	}

	if( (m_curCombineInfo.m_dwShadeColorChannelFlag & 0xFFFFFF00) == 0 )
	{
		return (m_pColorCombiner->GetConstFactor(m_curCombineInfo.m_dwShadeColorChannelFlag, m_curCombineInfo.m_dwShadeAlphaChannelFlag, curDiffuseColor)&mask);
	}
	else
		return (CalculateConstFactor(m_curCombineInfo.m_dwShadeColorChannelFlag, m_curCombineInfo.m_dwShadeAlphaChannelFlag, curDiffuseColor)&mask);
}

D3DCOLOR D3DRender::PostProcessSpecularColor()
{
	if( m_curCombineInfo.specularPostOp == MUX_0 )
	{
		return 0xFFFFFFFF;
	}
	else
	{
		return m_pColorCombiner->GetConstFactor(m_curCombineInfo.specularPostOp, m_curCombineInfo.specularPostOp);
	}
}

void CDirectXPixelShaderCombiner::InitCombinerBlenderForSimpleTextureDraw(uint32 tile)
{
	gD3DDevWrapper.SetPixelShader( NULL );
	m_pD3DRender->ZBufferEnable( FALSE );

	gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	gD3DDevWrapper.SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

	gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ZERO);
	gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
	//gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
	gD3DDevWrapper.SetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
	//gD3DDevWrapper.SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
	
	m_pD3DRender->m_curCombineInfo.stages[0].bTextureUsed = true;

	gD3DDevWrapper.SetTexture( 0, MYLPDIRECT3DTEXTURE(g_textures[tile].m_lpsTexturePtr) );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );

	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

	m_pD3DRender->m_curCombineInfo.m_dwShadeColorChannelFlag = 0;
	m_pD3DRender->m_curCombineInfo.m_dwShadeAlphaChannelFlag = 0;
	m_pD3DRender->m_curCombineInfo.specularPostOp = 0;

	m_pD3DRender->SetAddressUAllStages( 0, TEXTURE_UV_FLAG_CLAMP );
	m_pD3DRender->SetAddressVAllStages( 0, TEXTURE_UV_FLAG_CLAMP );
}

#ifdef _DEBUG
void CDirectXPixelShaderCombiner::DisplaySimpleMuxString(void)
{
	CColorCombiner::DisplaySimpleMuxString();
	TRACE0("\n");

	int idx = FindCompiledShader();
	if( idx < 0 )	idx = GeneratePixelShaderFromMux();

	if( m_pixelShaderList[idx].pShaderText != NULL )
		TRACE0(m_pixelShaderList[idx].pShaderText);

	TRACE0("\n\n");
}
#endif