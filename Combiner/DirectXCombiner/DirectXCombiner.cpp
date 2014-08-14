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

//========================================================================
extern D3DCAPS9 g_D3DDeviceCaps;

CDirectXPixelShaderCombiner::CDirectXPixelShaderCombiner(CRender *pRender)
	:CColorCombiner(pRender), m_pD3DRender((D3DRender*)pRender)
{
	m_pDecodedMux = new DecodedMux;
	TRACE0("Create the pixel shader combiner");
}

//Todo, fix me so i dont have to be a seperate function
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

		SAFE_RELEASE(m_pixelShaderList[i].pShader);
		m_pixelShaderList[i].pVS->Release();
	}
	m_pixelShaderList.clear();
}

CDirectXPixelShaderCombiner::~CDirectXPixelShaderCombiner()
{
	CleanUp();
}

int CDirectXPixelShaderCombiner::FindCompiledShader(void)
{
	int n = m_pixelShaderList.size();
	bool found=false;

	for( int i=0; i<n; i++ )
	{
		if( m_pixelShaderList[i].mux64 == m_pD3DRender->m_Mux )
		{
			found = true;
			return i;
		}
	}

	return -1;
}

bool CDirectXPixelShaderCombiner::Initialize(void)
{
	gD3DDevWrapper.SetPixelShader( NULL );
	return true;
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

	gD3DDevWrapper.SetTexture( 0, g_textures[gRSP.curTile].m_lpsTexturePtr );
	m_pD3DRender->SetTexelRepeatFlags(gRSP.curTile);
	gD3DDevWrapper.SetTexture( 1, NULL );
	gD3DDevWrapper.SetPixelShader( NULL );

	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );

	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
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

	HRESULT e = D3DXAssembleShader( m_textBuf, strlen(m_textBuf),  0, NULL, NULL, &(newEntry.pVS), NULL );
	if( e != S_OK )
	{
#ifdef _DEBUG
		TRACE0("Error to assemble shader");
		TRACE0(m_textBuf);
		DisplaySimpleMuxString();
#endif
	}

	e = g_pD3DDev->CreatePixelShader( (DWORD*)newEntry.pVS->GetBufferPointer(), &(newEntry.pShader) );

	if( e != S_OK )
	{
		TRACE0("Error to create shader");
		TRACE0(m_textBuf);
		newEntry.dwShaderID = 0;
		newEntry.pShader = NULL;
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


void CDirectXPixelShaderCombiner::InitCombinerCycle12(void)
{
	// Step 1: set pixel shader
	int idx = FindCompiledShader();
	if( idx < 0 )	idx = GeneratePixelShaderFromMux();

	gD3DDevWrapper.SetPixelShader(m_pixelShaderList[idx].pShader);

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
			gD3DDevWrapper.SetTexture( 0, g_textures[gRSP.curTile].m_lpsTexturePtr );
	}

	if( m_bTex1Enabled )
	{
		m_pD3DRender->SetTexelRepeatFlags((gRSP.curTile+1)&7);
		if( g_textures[(gRSP.curTile+1)&7].m_pCTexture != NULL )
			gD3DDevWrapper.SetTexture( 1, g_textures[(gRSP.curTile+1)&7].m_lpsTexturePtr );
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

	gD3DDevWrapper.SetTexture( 0, g_textures[tile].m_lpsTexturePtr );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );

	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	gD3DDevWrapper.SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

	m_pD3DRender->m_curCombineInfo.m_dwShadeColorChannelFlag = 0;
	m_pD3DRender->m_curCombineInfo.m_dwShadeAlphaChannelFlag = 0;
	m_pD3DRender->m_curCombineInfo.specularPostOp = 0;

	m_pD3DRender->SetAddressUAllStages( 0, D3DTADDRESS_CLAMP );
	m_pD3DRender->SetAddressVAllStages( 0, D3DTADDRESS_CLAMP );
}

#ifdef _DEBUG
void CDirectXPixelShaderCombiner::DisplaySimpleMuxString(void)
{
        CColorCombiner::DisplaySimpleMuxString();
        TRACE0("\n");

        int idx = FindCompiledShader();
        if( idx < 0 )   idx = GeneratePixelShaderFromMux();

        if( m_pixelShaderList[idx].pShaderText != NULL )
                TRACE0(m_pixelShaderList[idx].pShaderText);

        TRACE0("\n\n");
}
#endif