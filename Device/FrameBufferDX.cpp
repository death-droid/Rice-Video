/*
Copyright (C) 2005 Rice1964

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


Overview of DX framebuffer code:
* Copies video backbuffer to rendertexture in main memory
* This is needed for framebuffer effects rendering
* This is slow due to the lack of hardware render to tex code


*/

#include "..\stdafx.h"
//copies DirectX backbuffer to the render_texture structure
//This can be slow....
//But, its needed to render framebuffer effects, due to the current implementation.
void DXFrameBufferManager::CopyBackBufferToRenderTexture(int idx, RecentCIInfo &ciInfo, RECT* pDstRect)
{
	LPDIRECT3DSURFACE9 pSavedBuffer;
	gRenderTextureInfos[idx].pRenderTexture->m_pTexture->GetTexture()->GetSurfaceLevel(0,&pSavedBuffer);

	HRESULT res;

	if( pSavedBuffer != NULL )
	{
		LPDIRECT3DSURFACE9 pBackBufferToSave = NULL;
		g_pD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBufferToSave);

		if( pBackBufferToSave )
		{
			if( pDstRect == NULL )
			{
				res = g_pD3DDev->UpdateSurface(pBackBufferToSave,NULL,pSavedBuffer,NULL);
			}
			else
			{
				float scaleX = windowSetting.uDisplayWidth/(float)ciInfo.dwWidth;
				float scaleY = windowSetting.uDisplayHeight/(float)ciInfo.dwHeight;
				RECT srcr = { uint32(pDstRect->left*scaleX), uint32(pDstRect->top*scaleY),
					uint32(pDstRect->right*scaleX), uint32(pDstRect->bottom*scaleY) };
				POINT srcp = {uint32(pDstRect->left*scaleX), uint32(pDstRect->top*scaleY)};
				res = g_pD3DDev->UpdateSurface(pBackBufferToSave,&srcr,pSavedBuffer,&srcp);
			}

			if( res != S_OK )
			{
				TRACE0("Cannot save back buffer");
			}
			pBackBufferToSave->Release();
		}
		pSavedBuffer->Release();
	}
	else
	{
		TRACE0("Cannot save back buffer");
	}
}

//Copies Direct3D graphics surface to the RDRAM memory structure
//Doesnt use render-targets (no HWFBE)
void DXFrameBufferManager::CopyD3DSurfaceToRDRAM(uint32 addr, uint32 fmt, uint32 siz, uint32 width, uint32 height, uint32 bufWidth, uint32 bufHeight, uint32 startaddr, uint32 memsize, uint32 pitch, IDirect3DSurface9 *surf)
{
	if( addr == 0 || addr>=g_dwRamSize )	return;
	if( pitch == 0 ) pitch = width;

	IDirect3DSurface9 *surf2 = NULL;

	TXTRBUF_DUMP(DebuggerAppendMsg("Copy Back to N64 RDRAM"););

	D3DLOCKED_RECT dlre;
	ZeroMemory( &dlre, sizeof(D3DLOCKED_RECT) );
	if( !SUCCEEDED(surf->LockRect(&dlre, NULL, D3DLOCK_READONLY)) )
	{
		D3DSURFACE_DESC desc;
		//TRACE0("Error, cannot lock the surface");
		surf->GetDesc(&desc);
		g_pD3DDev->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_DEFAULT, &surf2, NULL);
		g_pD3DDev->UpdateSurface(surf,NULL,surf2,NULL);
		ZeroMemory( &dlre, sizeof(D3DLOCKED_RECT) );
		if( !SUCCEEDED(surf2->LockRect(&dlre, NULL, D3DLOCK_READONLY)) )
		{
			TRACE0("Error, cannot lock the copied surface");
			return;
		}
	}

	CopyBufferToRDRAM(addr, fmt, siz, width, height, bufWidth, bufHeight, startaddr, memsize, pitch, dlre.pBits, dlre.Pitch);

	if( surf2 )	
	{
		surf2->UnlockRect();
		surf2->Release();
	}
	else
		surf->UnlockRect();
}
//Performs main backbuffer to main N64 emulated RDRAM for FB effects....
void DXFrameBufferManager::StoreBackBufferToRDRAM(uint32 addr, uint32 fmt, uint32 siz, uint32 width, uint32 height, uint32 bufWidth, uint32 bufHeight, uint32 startaddr, uint32 memsize, uint32 pitch)
{
	IDirect3DSurface9 *backBuffer = NULL;
	g_pD3DDev->GetBackBuffer(0, 0,D3DBACKBUFFER_TYPE_MONO, &backBuffer);

	TXTRBUF_DUMP(DebuggerAppendMsg("Copy Back Buffer to N64 RDRAM"););

	CopyD3DSurfaceToRDRAM(addr, fmt, siz, width, height, bufWidth, bufHeight, startaddr, memsize, pitch, backBuffer);
	backBuffer->Release();
}
