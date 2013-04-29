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

CDirectXBlender::CDirectXBlender(CRender *pRender) 
	: CBlender(pRender),m_pD3DRender((D3DRender*)pRender) 
{
}

void CDirectXBlender::NormalAlphaBlender(void)
{
	gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE );
}

void CDirectXBlender::DisableAlphaBlender(void)
{
	gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE );
}

void CDirectXBlender::BlendFunc(uint32 srcFunc, uint32 desFunc)
{
	gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND, srcFunc);
	gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND, desFunc);
}

void CDirectXBlender::Enable()
{
	gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE );
}

void CDirectXBlender::Disable()
{
	gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE );
}

