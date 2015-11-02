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

	CDirectXPixelShaderCombiner(CRender *pRender);
	~CDirectXPixelShaderCombiner();
	bool Initialize(void);
	void InitCombinerBlenderForSimpleTextureDraw(uint32 tile=0);

protected:
	friend class DirectXDeviceBuilder;


	void CleanUp(void);
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

