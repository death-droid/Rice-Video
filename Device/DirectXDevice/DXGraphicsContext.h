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

#ifndef _DX_GRAPHICS_CONTEXT_H_
#define _DX_GRAPHICS_CONTEXT_H_
//-----------------------------------------------------------------------------
// Error codes
//-----------------------------------------------------------------------------
enum APPMSGTYPE { MSG_NONE, MSGERR_APPMUSTEXIT, MSGWARN_SWITCHEDTOREF };

#define D3DAPPERR_NODIRECT3D          0x82000001
#define D3DAPPERR_NOCOMPATIBLEDEVICES 0x82000003
#define D3DAPPERR_NOWINDOWABLEDEVICES 0x82000004
#define D3DAPPERR_NOHARDWAREDEVICE    0x82000005
#define D3DAPPERR_HALNOTCOMPATIBLE    0x82000006
#define D3DAPPERR_NOWINDOWEDHAL       0x82000007
#define D3DAPPERR_NODESKTOPHAL        0x82000008
#define D3DAPPERR_NOHALTHISMODE       0x82000009
#define D3DAPPERR_NONZEROREFCOUNT     0x8200000a
#define D3DAPPERR_RESIZEFAILED        0x8200000c

struct TxtrCacheEntry;

// Implementation
class CDXGraphicsContext : public CGraphicsContext
{
public:
	CDXGraphicsContext();
	virtual ~CDXGraphicsContext();
	
	
	bool Initialize(HWND hWnd, HWND hWndStatus, uint32 dwWidth, uint32 dwHeight, BOOL bWindowed );
	void CleanUp();
	
	void Clear(ClearFlag dwFlags, uint32 color=0xFF000000, float depth=1.0f);
	void UpdateFrame(bool swaponly=false);
	bool IsFSAAEnable() {return m_FSAAIsEnabled;}

	virtual void SaveSurfaceToFile(char *filenametosave, LPDIRECT3DSURFACE9 surf, bool bShow = true);

	int ToggleFullscreen();		// return 0 as the result is windowed
	static bool IsResultGood(HRESULT hr, bool displayError = false);
	static void InitDeviceParameters();

protected:
	friend class DirectXDeviceBuilder;

	HRESULT Create3D( BOOL bWindowed );
	HRESULT InitializeD3D();

	HRESULT ResizeD3DEnvironment();
	HRESULT DoToggleFullscreen();
	HRESULT AdjustWindowForChange();
	
	void Pause( bool bPause );
	
	HRESULT InitDeviceObjects();
	HRESULT CleanDeviceObjects();

	bool	CreateFontObjects();

	// Device information
	static D3DCAPS9			m_d3dCaps;           // Caps for the device
	static bool				m_bSupportAnisotropy;
	static HRESULT			DisplayD3DErrorMsg( HRESULT hr, uint32 dwType );
	static D3DDISPLAYMODE	m_displayMode;

	// Graphic context
	LPDIRECT3D9			m_pD3D;
	LPDIRECT3DDEVICE9	m_pd3dDevice;

	D3DPRESENT_PARAMETERS m_d3dpp;

    D3DSURFACE_DESC		m_d3dsdBackBuffer;   // Surface desc of the backbuffer

	bool				m_FSAAIsEnabled;
};

#define MAX_RENDER_STATE					152
#define MAX_NUM_OF_PIXEL_SHADER_CONSTANT	30
#define MAX_TEXTURE_STAGE_STATE				25
class CD3DDevWrapper
{
public:
	CD3DDevWrapper() 
	{
		m_pD3DDev = NULL;
	}

	~CD3DDevWrapper()
	{
		m_pD3DDev = NULL;
	}

	void SetD3DDev(LPDIRECT3DDEVICE9 pD3DDev);

	HRESULT SetRenderState(D3DRENDERSTATETYPE State,DWORD Value);
	HRESULT SetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value);
	HRESULT SetPixelShader(IDirect3DPixelShader9* pShader);
	HRESULT SetPixelShaderConstant (DWORD Register,float* pfdata);
	HRESULT SetViewport (D3DVIEWPORT9* pViewport);
	HRESULT SetTexture (DWORD Stage,IDirect3DBaseTexture9* pTexture);
	HRESULT SetFVF (DWORD Handle);
	void    Initalize(void);

private:
	LPDIRECT3DDEVICE9 m_pD3DDev;
	DWORD m_savedRenderStates[MAX_RENDER_STATE];
	DWORD m_savedTextureStageStates[8][MAX_TEXTURE_STAGE_STATE];
	IDirect3DPixelShader9* m_savedPixelShader;
	DWORD m_savedFVF;
	float m_savedPixelShaderConstants[MAX_NUM_OF_PIXEL_SHADER_CONSTANT][4];
	D3DVIEWPORT9 m_savedViewport;
	IDirect3DBaseTexture9* m_savedTexturePointers[8];
};

#endif



