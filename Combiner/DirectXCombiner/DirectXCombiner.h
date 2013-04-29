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

#ifndef _DIRECTX_COMBINER_H_
#define _DIRECTX_COMBINER_H_

enum CycleSimplicity	//How simple is the mux
{
	CS_NOTUSED,				//2nd cycle is not used
	CS_POSSIBLE_0_STAGE,	//It is possible that 2nd cycle can be merged with 1st cycle
	CS_1_STAGE,				//Can be finished in D3D within 1 stage
	CS_POSSIBLE_1_STAGE,	//It is possible that this cycle can be finished in D3D within 1 stage
	CS_2_STAGE,				//Can be finished in D3D within 2 stages
	CS_POSSIBLE_2_STAGE,	//It is possible that this cycle can be finished in D3D within 2 stage
	CS_3_STAGE,				//Can be finished in D3D within 3 stages
	CS_4_STAGE,				//Can be finished in D3D within 4 stages
	CS_5_STAGE,				//Can be finished in D3D within 5 stages
	CS_6_STAGE,				//Can be finished in D3D within 6 stages
	CS_7_STAGE,				//Can be finished in D3D within 7 stages
	CS_8_STAGE,				//Can be finished in D3D within 8 stages
};

/*
#define SEL(arg)		D3DTOP_SELECTARG1,D3DTA_##arg,D3DTA_IGNORE
#define SKIP			SEL(CUR)

#define BLENDING2(op, arg1, arg2)	D3DTOP_##op,D3DTA_##arg1,D3DTA_##arg2
#define BLENDING3(op, arg1, arg2, arg0)	D3DTOP_##op,D3DTA_##arg1,D3DTA_##arg2,D3DTA_##arg0

#define LERP(arg1, arg2, arg0)		BLENDING3(LERP,arg1,arg2,arg0)
#define MULADD(arg1, arg2, arg0)	BLENDING3(MULTIPLYADD,arg1,arg2,arg0)
#define SUB(arg1,arg2)				BLENDING2(SUBTRACT,arg1,arg2)
#define BLENDFACTORALPHA(a1,arg2)	BLENDING2(BLENDFACTORALPHA,a1,arg2)
#define BLENDDIFFUSEALPHA(a1,arg2)	BLENDING2(BLENDDIFFUSEALPHA,a1,arg2)
#define BLENDSHADEALPHA(arg1,arg2)	BLENDING2(BLENDDIFFUSEALPHA,arg1,arg2)
#define BLENDCURRENTALPHA(a1,arg2)	BLENDING2(BLENDCURRENTALPHA,a1,arg2)
#define BLENDTEXTUREALPHA(a1,arg2)	BLENDING2(BLENDTEXTUREALPHA,a1,arg2)
#define ADDSMOOTH(arg1,arg2)		BLENDING2(ADDSMOOTH,arg1,arg2)
#define MOD(arg1,arg2)				BLENDING2(MODULATE,arg1,arg2)
#define MOD2X(arg1,arg2)			BLENDING2(MODULATE2X,arg1,arg2)
#define ADD(arg1,arg2)				BLENDING2(ADD,arg1,arg2)

#define D3DTA_SPE		D3DTA_SPECULAR
#define D3DTA_CUR		D3DTA_CURRENT
#define D3DTA_TEX		D3DTA_TEXTURE
#define D3DTA_FAC		D3DTA_TFACTOR
#define D3DTA_DIF		D3DTA_DIFFUSE
#define D3DTA_SPEA		D3DTA_SPECULAR|D3DTA_ALPHAREPLICATE
#define D3DTA_CURA		D3DTA_CURRENT|D3DTA_ALPHAREPLICATE
#define D3DTA_TEXA		D3DTA_TEXTURE|D3DTA_ALPHAREPLICATE
#define D3DTA_FACA		D3DTA_TFACTOR|D3DTA_ALPHAREPLICATE
#define D3DTA_DIFA		D3DTA_DIFFUSE|D3DTA_ALPHAREPLICATE
#define D3DTA_SPEC		D3DTA_SPECULAR|D3DTA_COMPLEMENT
#define D3DTA_CURC		D3DTA_CURRENT|D3DTA_COMPLEMENT
#define D3DTA_TEXC		D3DTA_TEXTURE|D3DTA_COMPLEMENT
#define D3DTA_FACC		D3DTA_TFACTOR|D3DTA_COMPLEMENT
#define D3DTA_DIFC		D3DTA_DIFFUSE|D3DTA_COMPLEMENT
#define D3DTA_SPEAC		D3DTA_SPECULAR|D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT
#define D3DTA_CURAC		D3DTA_CURRENT|D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT
#define D3DTA_TEXAC		D3DTA_TEXTURE|D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT
#define D3DTA_FACAC		D3DTA_TFACTOR|D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT
#define D3DTA_DIFAC		D3DTA_DIFFUSE|D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT
#define D3DTA_SPECA		D3DTA_SPEAC
#define D3DTA_CURCA		D3DTA_CURAC
#define D3DTA_TEXCA		D3DTA_TEXAC
#define D3DTA_FACCA		D3DTA_FACAC
#define D3DTA_DIFCA		D3DTA_DIFAC
*/

class CombineStage
{
public:
	StageOperate colorOp;
	StageOperate alphaOp;
	uint32 dwTexture;	//Which texture to apply, 0 or 1
	bool  bTextureUsed;	
	
	BOOL operator!=(const CombineStage & cs) const { return !(operator==(cs)); }
	BOOL operator==(const CombineStage & cs) const 
	{
		return (
			cs.colorOp.Arg1 == colorOp.Arg1 &&
			cs.colorOp.Arg2 == colorOp.Arg2 &&
			cs.colorOp.Arg0 == colorOp.Arg0 &&
			cs.alphaOp.Arg0 == alphaOp.Arg0 &&
			cs.alphaOp.Arg1 == alphaOp.Arg1 &&
			cs.alphaOp.Arg2 == alphaOp.Arg2 &&
			cs.colorOp.op == colorOp.op &&
			cs.alphaOp.op == alphaOp.op &&
			cs.dwTexture == dwTexture);
	}
	
	/*
	CombineStage()
	{
		dwTexture = 0;
		colorOp.op = colorOp.Arg0 = colorOp.Arg1 = colorOp.Arg2 =0;
		alphaOp.op = alphaOp.Arg0 = alphaOp.Arg1 = alphaOp.Arg2 =0;
	}
	*/
};	

class SetCombineInfo
{
public:
	uint64 mux;
	LONG nStages;
	
	BlendingFunc  blendingFunc;
	
	CombineStage stages[8];
	uint32 TFactor;
	uint32 specularPostOp;
	uint32 m_dwShadeColorChannelFlag;
	uint32 m_dwShadeAlphaChannelFlag;
	
	BOOL operator!=(const SetCombineInfo & sci) const { return !(operator==(sci)); }
	BOOL operator==(const SetCombineInfo & sci) const 
	{
		LONG i;
		
		if (sci.nStages != nStages)
			return FALSE;
		if (sci.blendingFunc != blendingFunc)
			return FALSE;
		
		for (i = 0; i < nStages; i++)
		{
			if (sci.stages[i] != stages[i])
				return FALSE;
		}

		if( sci.TFactor != TFactor )
			return FALSE;
		if( sci.specularPostOp != specularPostOp )
			return FALSE;
		if( sci.m_dwShadeColorChannelFlag != m_dwShadeColorChannelFlag )
			return FALSE;
		if( sci.m_dwShadeAlphaChannelFlag != m_dwShadeAlphaChannelFlag )
			return FALSE;

		return TRUE;
	}
};

typedef struct {
	uint64  mux64;
	ID3DXBuffer* pVS;
	uint32 dwShaderID;
	char *pShaderText;
	IDirect3DPixelShader9* pShader;
} 

PixelShaderEntry;

class D3DRender;

class CDirectXPixelShaderCombiner : public CColorCombiner
{
public:
	bool Initialize(void);
	void InitCombinerBlenderForSimpleTextureDraw(uint32 tile=0);

protected:
	friend class DirectXDeviceBuilder;
	CDirectXPixelShaderCombiner(CRender *pRender);
	~CDirectXPixelShaderCombiner();

	void CleanUp(void);
	void DisableCombiner(void);
	void InitCombinerCycleCopy(void);
	void InitCombinerCycleFill(void);
	void InitCombinerCycle12(void);

	D3DRender *m_pD3DRender;
	
	int FindCompiledShader(void);
	int GeneratePixelShaderFromMux(void);

	std::vector<PixelShaderEntry> m_pixelShaderList;

#ifdef _DEBUG
	virtual void DisplaySimpleMuxString(void);
#endif

#define PIXELSHADERTEXTBUFSIZE	16000
	char m_textBuf[PIXELSHADERTEXTBUFSIZE];
};

#endif

