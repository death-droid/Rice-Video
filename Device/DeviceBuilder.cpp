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

#include "..\stdafx.h"

//========================================================================
CDeviceBuilder* CDeviceBuilder::m_pInstance=NULL;

CDeviceBuilder* CDeviceBuilder::GetBuilder(void)
{
	if( m_pInstance == NULL )
	{
		m_pInstance = new DirectXDeviceBuilder();
		SAFE_CHECK(m_pInstance);
	}
	
	return m_pInstance;
}

void CDeviceBuilder::DeleteBuilder(void)
{
	delete m_pInstance;
	m_pInstance = NULL;
}

CDeviceBuilder::CDeviceBuilder() :
	m_pRender(NULL),
	m_pGraphicsContext(NULL),
	m_pColorCombiner(NULL),
	m_pAlphaBlender(NULL)
{
}

CDeviceBuilder::~CDeviceBuilder()
{
	DeleteGraphicsContext();
	DeleteRender();
	DeleteColorCombiner();
	DeleteAlphaBlender();
}

void CDeviceBuilder::DeleteGraphicsContext(void)
{
	if( m_pGraphicsContext != NULL )
	{
		delete m_pGraphicsContext;
		CGraphicsContext::g_pGraphicsContext = m_pGraphicsContext = NULL;
	}

	SAFE_DELETE(g_pFrameBufferManager);
}

void CDeviceBuilder::DeleteRender(void)
{
	if( m_pRender != NULL )
	{
		delete m_pRender;
		CRender::g_pRender = m_pRender = NULL;
		CRender::gRenderReferenceCount = 0;
	}
}

void CDeviceBuilder::DeleteColorCombiner(void)
{
	if( m_pColorCombiner != NULL )
	{
		delete m_pColorCombiner;
		m_pColorCombiner = NULL;
	}
}

void CDeviceBuilder::DeleteAlphaBlender(void)
{
	if( m_pAlphaBlender != NULL )
	{
		delete m_pAlphaBlender;
		m_pAlphaBlender = NULL;
	}
}


//========================================================================

CGraphicsContext * DirectXDeviceBuilder::CreateGraphicsContext(void)
{
	if( g_GraphicsInfo.hStatusBar )
	{
		SetWindowText(g_GraphicsInfo.hStatusBar,"Creating DirectX Device Context");
	}
	if( m_pGraphicsContext == NULL )
	{
		m_pGraphicsContext = new CDXGraphicsContext();
		SAFE_CHECK(m_pGraphicsContext);
		CGraphicsContext::g_pGraphicsContext = m_pGraphicsContext;
	}

	g_pFrameBufferManager = new DXFrameBufferManager;
	return m_pGraphicsContext;
}

CRender * DirectXDeviceBuilder::CreateRender(void)
{
	if( m_pRender == NULL )
	{
		m_pRender = new D3DRender();
		SAFE_CHECK(m_pRender);
		CRender::g_pRender = m_pRender;
	}

	return m_pRender;
}

CTexture * DirectXDeviceBuilder::CreateTexture(uint32 dwWidth, uint32 dwHeight, TextureUsage usage)
{
	CDirectXTexture *txtr = new CDirectXTexture(dwWidth, dwHeight, usage);
	if( txtr->m_pTexture == NULL )
	{
		delete txtr;
		TRACE0("Cannot create new texture, out of video memory");
		return NULL;
	}
	else
		return txtr;
}

CColorCombiner * DirectXDeviceBuilder::CreateColorCombiner(CRender *pRender)
{
	if( m_pColorCombiner == NULL )
	{
		extern D3DCAPS9 g_D3DDeviceCaps;
		bool canUsePixelShader = g_D3DDeviceCaps.PixelShaderVersion >= D3DPS_VERSION(1, 1);

		if(!canUsePixelShader)
		{
			//The user doesnt support pixel shaders, show a error message
			ErrorMsg("Video card does not support pixel shaders, Rice Video will not function properly");
		}
		else
		{
			//The user supports pixel shaders, create the combiner
			m_pColorCombiner = new CDirectXPixelShaderCombiner(pRender);
		}
	
		SAFE_CHECK(m_pColorCombiner);
	}

	return m_pColorCombiner;
}

CBlender * DirectXDeviceBuilder::CreateAlphaBlender(CRender *pRender)
{
	if( m_pAlphaBlender == NULL )
	{
		m_pAlphaBlender = new CDirectXBlender(pRender);
		SAFE_CHECK(m_pAlphaBlender);
	}

	return m_pAlphaBlender;
}


