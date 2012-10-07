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

#include "CSortedList.h"

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

class D3DRender;

typedef struct {
	uint64  mux64;
#ifdef _XBOX
	bool   bPrimLOD;
	bool   bFog;
#else
	ID3DXBuffer* pVS;
#endif
	uint32 dwShaderID;
	char *pShaderText;
#if DIRECTX_VERSION > 8
	IDirect3DPixelShader9* pShader;
#endif
} PixelShaderEntry;

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
	int GeneratePixelShaderFromMux(void);
	int FindCompiledShader(void);

	D3DRender *m_pD3DRender;

	std::vector<PixelShaderEntry> m_pixelShaderList;

#ifdef _DEBUG
	virtual void DisplaySimpleMuxString(void);
#endif

#define PIXELSHADERTEXTBUFSIZE	16000
	char m_textBuf[PIXELSHADERTEXTBUFSIZE];

};
#endif

