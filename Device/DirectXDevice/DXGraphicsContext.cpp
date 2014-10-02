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
#include <WinGDI.h>

LPDIRECT3DDEVICE9 g_pD3DDev = NULL;
CD3DDevWrapper    gD3DDevWrapper;
D3DCAPS9 g_D3DDeviceCaps;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
D3DCAPS9		CDXGraphicsContext::m_d3dCaps;           // Caps for the device
D3DDISPLAYMODE CDXGraphicsContext::m_displayMode;
bool			CDXGraphicsContext::m_bSupportAnisotropy;

CDXGraphicsContext::CDXGraphicsContext() :
	m_pd3dDevice(NULL),
	m_pD3D(NULL),
	m_FSAAIsEnabled(false)
{
}

//*****************************************************************************
//
//*****************************************************************************
CDXGraphicsContext::~CDXGraphicsContext()
{
	CGraphicsContext::Get()->CleanUp();
}

//*****************************************************************************
//
//*****************************************************************************
void CDXGraphicsContext::Clear(ClearFlag dwFlags, uint32 color, float depth)
{
	uint32 flag=0;
	if( dwFlags&CLEAR_COLOR_BUFFER )	flag |= D3DCLEAR_TARGET;
	if( dwFlags&CLEAR_DEPTH_BUFFER )	flag |= D3DCLEAR_ZBUFFER;
	Lock();
	if (m_pd3dDevice != NULL)
		m_pd3dDevice->Clear(0, NULL, flag, color, depth, 0);
	Unlock();
}

//*****************************************************************************
//
//*****************************************************************************
extern RECT frameWriteByCPURect;

void CDXGraphicsContext::UpdateFrame(bool swaponly)
{
	HRESULT hr;

	status.gFrameCount++;

	if( CRender::g_pRender )	
	{
		CRender::g_pRender->BeginRendering();
	}

	g_pFrameBufferManager->UpdateFrameBufferBeforeUpdateFrame();

	if( CRender::g_pRender )
	{
		CRender::g_pRender->EndRendering();
	}
	
	Lock();
	if (m_pd3dDevice == NULL)
	{
		hr = E_FAIL;
	}
	else
	{
		// Test the cooperative level to see if it's okay to render
		if( FAILED( hr = m_pd3dDevice->TestCooperativeLevel() ) )
		{
			// If the device was lost, do not render until we get it back
			if( hr == D3DERR_DEVICELOST )
			{
				hr = S_OK;
				goto exit;
			}
			
			// Check if the device needs to be resized.
			if( hr == D3DERR_DEVICENOTRESET )
			{
				if( FAILED( hr = ResizeD3DEnvironment() ) )
				{
					goto exit;
				}
			}
			
			// return hr
			goto exit;
		}

		if( !g_curRomInfo.bForceScreenClear )	
			Clear(CLEAR_DEPTH_BUFFER);

		if( m_bWindowed )
		{
			RECT dstrect={0,windowSetting.toolbarHeight,windowSetting.uDisplayWidth,windowSetting.toolbarHeight+windowSetting.uDisplayHeight};
			RECT srcrect={0,0,windowSetting.uDisplayWidth,windowSetting.uDisplayHeight};
			hr = m_pd3dDevice->Present( &srcrect, &dstrect, NULL, NULL );
		}
		else
		{
			hr = m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
		}

#ifdef _DEBUG
		if( pauseAtNext && eventToPause == NEXT_FRAME )
		{
			TRACE0("Update screen");
		}
#endif

	}
exit:
	
	Unlock();

	status.bScreenIsDrawn = false;
	if( g_curRomInfo.bForceScreenClear )	needCleanScene = true;
}

//*****************************************************************************
//
//*****************************************************************************
extern void WriteConfiguration(void);
bool CDXGraphicsContext::Initialize(HWND hWnd, HWND hWndStatus,
									 uint32 dwWidth, uint32 dwHeight,
									 bool bWindowed )
{
	HRESULT hr;

	if( g_GraphicsInfo.hStatusBar )
	{
		SetWindowText(g_GraphicsInfo.hStatusBar,"Initializing DirectX Device, please wait");
	}

	Lock();

	// Create the Direct3D object
	m_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
	if( m_pD3D == NULL )
	{
		Unlock();
		DisplayD3DErrorMsg( D3DAPPERR_NODIRECT3D, MSGERR_APPMUSTEXIT );
		return false;
	}

	CGraphicsContext::Initialize(hWnd, hWndStatus, dwWidth, dwHeight, bWindowed );
	
	hr = Create3D( bWindowed );

	// Clear/Update a few times to ensure that all of the buffers are cleared
	if ( m_pd3dDevice )
	{
		Clear(CLEAR_COLOR_AND_DEPTH_BUFFER);
		m_pd3dDevice->BeginScene();
		m_pd3dDevice->EndScene();
		UpdateFrame();
	}

	D3DVIEWPORT9 vp = {
		0, 
			windowSetting.toolbarHeightToUse, 
			windowSetting.uDisplayWidth, 
			windowSetting.uDisplayHeight, 0, 1
	};
	gD3DDevWrapper.SetViewport(&vp);
	
	Unlock();

	g_pD3DDev->GetDeviceCaps(&g_D3DDeviceCaps);

	if( g_GraphicsInfo.hStatusBar )
	{
		SetWindowText(g_GraphicsInfo.hStatusBar,"DirectX device is ready");
	}

	return hr==S_OK;
}

// This is a static function, will be called when the plugin DLL is initialized
void CDXGraphicsContext::InitDeviceParameters()
{
	SetWindowText(m_hWndStatus, "Initialize DirectX Device");

	// Create Direct3D object
	LPDIRECT3D9 pD3D;
	pD3D = Direct3DCreate9( D3D_SDK_VERSION );
	if( pD3D == NULL )
	{
        DisplayD3DErrorMsg( D3DAPPERR_NODIRECT3D, MSGERR_APPMUSTEXIT );
		return;
	}

	//Get display mode for default adapter
	pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_displayMode);

	// Get device caps
	pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &m_d3dCaps);

	// Determine the maximum FSAA
	for( m_maxFSAA = 16; m_maxFSAA >= 2; m_maxFSAA-- )
	{
		if (SUCCEEDED(pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, false, D3DMULTISAMPLE_TYPE(D3DMULTISAMPLE_NONE + m_maxFSAA), NULL)))
		{
			if (m_maxFSAA < 2)
				m_maxFSAA = 0;
			break;
		}
		else
		{
			continue;
		}
	}

	if( options.DirectXMaxFSAA != m_maxFSAA )
	{
		options.DirectXMaxFSAA = m_maxFSAA;
		WriteConfiguration();
	}

	// Check Anisotropy Filtering maximum
	m_maxAnisotropy = m_d3dCaps.MaxAnisotropy;

	if( options.DirectXMaxAnisotropy != m_maxAnisotropy )
	{
		options.DirectXMaxAnisotropy = m_maxAnisotropy;
		WriteConfiguration();
	}


	// Release the Direct3D object
	pD3D->Release();
}


//*****************************************************************************
//
//*****************************************************************************
HRESULT CDXGraphicsContext::Create3D( bool bWindowed )
{
    HRESULT hr;
	
 	m_bWindowed = (bWindowed==true);		// Make user Toggle manually for now!

    // Initialize the 3D environment for the app
    if( FAILED( hr = InitializeD3D() ) )
    {
        SAFE_RELEASE( m_pD3D );
        return DisplayD3DErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }
	
    // The app is ready to go
    m_bReady = true;
	
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: InitializeD3D()
// Desc:
//-----------------------------------------------------------------------------

HRESULT CDXGraphicsContext::InitializeD3D()
{
    // Prepare window for possible windowed/fullscreen change
    AdjustWindowForChange();

	windowSetting.statusBarHeightToUse = 0;
	windowSetting.toolbarHeightToUse = 0;

    // Set up the presentation parameters
	//Clear out our presentation struct for use
    ZeroMemory( &m_d3dpp, sizeof(m_d3dpp) );

    m_d3dpp.Windowed               = m_bWindowed;
    m_d3dpp.BackBufferCount        = 1;
	m_d3dpp.EnableAutoDepthStencil = true; /*m_bUseDepthBuffer;*/
	m_d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	m_d3dpp.hDeviceWindow          = m_hWnd;
	m_d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
	m_d3dpp.BackBufferFormat	   = D3DFMT_X8R8G8B8;

	m_FSAAIsEnabled = false;
	if (options.DirectXAntiAliasingValue != 0 && m_maxFSAA > 0)
	{
		m_d3dpp.MultiSampleType = (D3DMULTISAMPLE_TYPE)(D3DMULTISAMPLE_NONE + min(m_maxFSAA, (int)options.DirectXAntiAliasingValue));
		m_FSAAIsEnabled = true;
		//TRACE1("Start with FSAA=%d X", pDeviceInfo->MultiSampleType-D3DMULTISAMPLE_NONE);
	}

	if( currentRomOptions.N64FrameBufferEmuType != FRM_BUF_NONE && m_FSAAIsEnabled )
	{
		m_FSAAIsEnabled = false;
		m_d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
		m_d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
		TRACE0("FSAA is turned off in order to use BackBuffer emulation");
		SetWindowText(g_GraphicsInfo.hStatusBar, "FSAA is turned off in order to use BackBuffer emulation");
	}


	if( !m_FSAAIsEnabled )
		m_d3dpp.SwapEffect = m_bWindowed ? D3DSWAPEFFECT_COPY : D3DSWAPEFFECT_FLIP;	// Always use COPY for window mode
	else
		m_d3dpp.SwapEffect		= D3DSWAPEFFECT_DISCARD;	// Anti-Aliasing mode

	windowSetting.uDisplayWidth = m_bWindowed ? windowSetting.uWindowDisplayWidth : windowSetting.uFullScreenDisplayWidth;
	windowSetting.uDisplayHeight = m_bWindowed ? windowSetting.uWindowDisplayHeight : windowSetting.uFullScreenDisplayHeight;

	m_d3dpp.FullScreen_RefreshRateInHz = m_bWindowed ? 0 : windowSetting.uFullScreenRefreshRate;

	if (m_d3dpp.FullScreen_RefreshRateInHz > m_displayMode.RefreshRate && !m_bWindowed)
	{
		m_d3dpp.FullScreen_RefreshRateInHz = m_displayMode.RefreshRate;
		windowSetting.uFullScreenRefreshRate = m_displayMode.RefreshRate;
	}

	m_d3dpp.BackBufferWidth = m_bWindowed ? 0 : windowSetting.uDisplayWidth;
	m_d3dpp.BackBufferHeight = m_bWindowed ? 0 : windowSetting.uDisplayHeight;

    // Create the device
	if(!SUCCEEDED(m_pD3D->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL, 
		m_hWnd, 
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, 
		&m_d3dpp,	&m_pd3dDevice )))
	{
		if(!SUCCEEDED(m_pD3D->CreateDevice(
				D3DADAPTER_DEFAULT,
				D3DDEVTYPE_HAL, 
				m_hWnd, 
				D3DCREATE_SOFTWARE_VERTEXPROCESSING, 
				&m_d3dpp,	&m_pd3dDevice )))
		{
			MsgInfo("Failed to initialize Direct3D");
			return E_FAIL;
		}
	}

	if(m_pd3dDevice )
    {
		g_pD3DDev = m_pd3dDevice;
		gD3DDevWrapper.SetD3DDev(m_pd3dDevice);

        // Store device Caps
        m_pd3dDevice->GetDeviceCaps( &m_d3dCaps );

        if( m_bWindowed )
        {
            SetWindowPos( m_hWnd, HWND_NOTOPMOST,
						m_rcWindowBounds.left, m_rcWindowBounds.top,
						( m_rcWindowBounds.right - m_rcWindowBounds.left ),
						( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
						SWP_SHOWWINDOW );
        }
		
        // Store render target surface desc
        LPDIRECT3DSURFACE9 pBackBuffer;
        m_pd3dDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
        pBackBuffer->GetDesc( &m_d3dsdBackBuffer );
        pBackBuffer->Release();
		
		if ( CRender::IsAvailable() )
		{
			CRender::GetRender()->InitDeviceObjects();
		}
        m_bActive = true;
        return S_OK;
    }
	else
	{
		if( status.ToToggleFullScreen || !m_bWindowed )
			SetWindowText(g_GraphicsInfo.hStatusBar,"Can not initialize DX9, check your Direct settings");
		else
			MsgInfo("Can not initialize DX9, check your Direct settings");
	}
    return E_FAIL;
}


//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::ResizeD3DEnvironment()
{
    HRESULT hr;
	
    // Release all vidmem objects
    if( FAILED( hr = CleanDeviceObjects() ) )
        return hr;
	
    // Reset the device
    if( IsResultGood( hr = m_pd3dDevice->Reset( &m_d3dpp ), true ) )
	{
		while ( hr == D3DERR_DEVICELOST )
		{
			Pause(true);
			hr = m_pd3dDevice->Reset( &m_d3dpp );
		}
	}
	else
	{
        return hr;
	}
	
    // Store render target surface desc
    LPDIRECT3DSURFACE9 pBackBuffer;
    m_pd3dDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
    pBackBuffer->GetDesc( &m_d3dsdBackBuffer );
    pBackBuffer->Release();
	
    // Initialize the app's device-dependent objects
	if ( CRender::IsAvailable() )
	{
		CRender::GetRender()->InitDeviceObjects();
	}
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: ToggleFullScreen()
// Desc: Called when user toggles between fullscreen mode and windowed mode
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::DoToggleFullscreen()
{
    // Need device change if going windowed and the current device
    // can only be fullscreen
	
    m_bReady = false;
	
    // Toggle the windowed state
    m_bWindowed = !m_bWindowed;
	
    // Prepare window for windowed/fullscreen change
    AdjustWindowForChange();

	Lock();
	m_pd3dDevice->Reset( &m_d3dpp );
	CRender::GetRender()->CleanUp();
	CleanUp();

    m_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
    if( m_pD3D == NULL )
	{
		TRACE0("Error to create m_pD3D");
		Unlock();
        return DisplayD3DErrorMsg( D3DAPPERR_NODIRECT3D, MSGERR_APPMUSTEXIT );
	}
	InitializeD3D();

	Unlock();

	CRender::g_pRender->SetViewport(0, 0, windowSetting.uViWidth, windowSetting.uViHeight, 0x3FF);

    // When moving from fullscreen to windowed mode, it is important to
    // adjust the window size after resetting the device rather than
    // beforehand to ensure that you get the window size you want.  For
    // example, when switching from 640x480 fullscreen to windowed with
    // a 1000x600 window on a 1024x768 desktop, it is impossible to set
    // the window size to 1000x600 until after the display mode has
    // changed to 1024x768, because windows cannot be larger than the
    // desktop.
    if( m_bWindowed )
    {
		SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        SetWindowPos( m_hWnd, HWND_NOTOPMOST,
			m_rcWindowBounds.left, m_rcWindowBounds.top,
			( m_rcWindowBounds.right - m_rcWindowBounds.left ),
			( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
			SWP_SHOWWINDOW );
    }
    m_bReady = true;
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: AdjustWindowForChange()
// Desc: Prepare the window for a possible change between windowed mode and
//       fullscreen mode.  This function is virtual and thus can be overridden
//       to provide different behavior, such as switching to an entirely
//       different window for fullscreen mode (as in the MFC sample apps).
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::AdjustWindowForChange()
{
    if( m_bWindowed )
    {
        // Set windowed-mode style - but disable resizing
        SetWindowLong( m_hWnd, GWL_STYLE, m_dwWindowStyle & (~(WS_THICKFRAME|WS_MAXIMIZEBOX)) );
		if ( IsWindow(m_hWndStatus) )
		{
			SetWindowLong( m_hWndStatus, GWL_STYLE, m_dwStatusWindowStyle & (~SBARS_SIZEGRIP));
		}
    }
    else
    {
        // Set fullscreen-mode style
        SetWindowLong( m_hWnd, GWL_STYLE, WS_POPUP|WS_VISIBLE|WS_EX_TOPMOST );
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: DisplayD3DErrorMsg()
// Desc: Displays error messages in a message box
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::DisplayD3DErrorMsg( HRESULT hr, uint32 dwType )
{
    TCHAR strMsg[512];
	
    switch( hr )
    {
	case D3DAPPERR_NODIRECT3D:
		strcpy( strMsg, "Could not initialize Direct3D. You may\n"
			"want to check that the latest version of\n"
			"DirectX is correctly installed on your\n"
			"system." );
		break;
		
	case D3DAPPERR_NOCOMPATIBLEDEVICES:
		strcpy( strMsg, "Could not find any compatible Direct3D\n"
			"devices." );
		break;
		
	case D3DAPPERR_NOWINDOWABLEDEVICES:
		strcpy( strMsg, "This program cannot run in a desktop\n"
			"window with the current display settings.\n"
			"Please change your desktop settings to a\n"
			"16- or 32-bit display mode and re-run this\n"
			"sample." );
		break;
		
	case D3DAPPERR_NOHARDWAREDEVICE:
		strcpy( strMsg, "No hardware-accelerated Direct3D devices\n"
			"were found." );
		break;
		
	case D3DAPPERR_HALNOTCOMPATIBLE:
		strcpy( strMsg, "This program requires functionality that\n"
			"is not available on your Direct3D hardware\n"
			"accelerator." );
		break;
		
	case D3DAPPERR_NOWINDOWEDHAL:
		strcpy( strMsg, "Your Direct3D hardware accelerator cannot\n"
			"render into a window." );
		break;
		
	case D3DAPPERR_NODESKTOPHAL:
		strcpy( strMsg, "Your Direct3D hardware accelerator cannot\n"
			"render into a window with the current\n"
			"desktop display settings." );
		break;
		
	case D3DAPPERR_NOHALTHISMODE:
		strcpy( strMsg, "This program requires functionality that is\n"
			"not available on your Direct3D hardware\n"
			"accelerator with the current desktop display\n"
			"settings." );
		break;
		
	case D3DAPPERR_RESIZEFAILED:
		strcpy( strMsg, "Could not reset the Direct3D device." );
		break;
		
	case D3DAPPERR_NONZEROREFCOUNT:
		strcpy( strMsg, "A D3D object has a non-zero reference\n"
			"count (meaning things were not properly\n"
			"cleaned up)." );
		break;
		
	case E_OUTOFMEMORY:
		strcpy( strMsg, "Not enough memory." );
		break;
		
	case D3DERR_OUTOFVIDEOMEMORY:
		strcpy( strMsg, "Not enough video memory." );
		break;
		
	default:
		strcpy( strMsg, "Generic application error. Enable\n"
			"debug output for detailed information." );
    }
	
	// TODO: Use IDS_D3DERROR resource
	// IDS_D3DERROR has one %s for the error message
	//wsprintf(szMsg, CResourceString(IDS_D3DERROR), szError);
	
	
    if( MSGERR_APPMUSTEXIT == dwType )
    {
        strcat( strMsg, "\n\nThis program will now exit." );
    }
    else
    {
        if( MSGWARN_SWITCHEDTOREF == dwType )
            strcat( strMsg, "\n\nSwitching to the reference rasterizer,\n"
			"a software device that implements the entire\n"
			"Direct3D feature set, but runs very slowly." );
    }

	OutputDebugString( strMsg );
	if( status.ToToggleFullScreen || !CGraphicsContext::g_pGraphicsContext->IsWindowed() )
		SetWindowText(g_GraphicsInfo.hStatusBar,strMsg);
	else
		MessageBox(NULL,strMsg,"str",MB_OK|MB_ICONERROR);

    return hr;
}

//-----------------------------------------------------------------------------
// Name: CleanUp()
// Desc: Cleanup scene objects
//-----------------------------------------------------------------------------
VOID CDXGraphicsContext::CleanUp()
{
	CGraphicsContext::CleanUp();
	
    if( m_pd3dDevice )
    {
        CleanDeviceObjects();

		SAFE_RELEASE(m_pd3dDevice);
		SAFE_RELEASE(m_pD3D);
    }
}


int CDXGraphicsContext::ToggleFullscreen()
{
    // Toggle the fullscreen/window mode
    if( m_bActive && m_bReady )
    {
        if( FAILED( DoToggleFullscreen() ) )
        {
            DisplayD3DErrorMsg( D3DAPPERR_RESIZEFAILED, MSGERR_APPMUSTEXIT );
        }
		CRender::GetRender()->Initialize();
    }

	if( !m_bWindowed )
	{
		m_pd3dDevice->ShowCursor( false );
	}
	else
	{
		m_pd3dDevice->ShowCursor( true );
	}

	return m_bWindowed?0:1;
}


//-----------------------------------------------------------------------------
// Name: Pause()
// Desc: Called in to toggle the pause state of the app.
//-----------------------------------------------------------------------------
void CDXGraphicsContext::Pause( bool bPause )
{
    static uint32 dwAppPausedCount = 0L;
	
    dwAppPausedCount += ( bPause ? +1 : -1 );
    m_bReady          = ( dwAppPausedCount ? false : true );
}

//-----------------------------------------------------------------------------
// Name: CleanDeviceObjects()
// Desc: Called when the device-dependent objects are about to be lost.
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::CleanDeviceObjects()
{
	if ( CRender::IsAvailable() )
	{
		CRender::GetRender()->ClearDeviceObjects();
	}

	// Kill all textures?
	gTextureManager.RecycleAllTextures();
	gTextureManager.CleanUp();

	return S_OK;
}

bool CDXGraphicsContext::IsResultGood(HRESULT hr, bool displayError)
{
	if(FAILED(hr))
	{
		if( displayError )
		{
			const char *errmsg = DXGetErrorString(hr);
			TRACE1("D3D Error: %s", errmsg);
			//ErrorMsg(errmsg);
		}
		return false;
	}
	else
		return true;
}

HRESULT CD3DDevWrapper::SetRenderState(D3DRENDERSTATETYPE State,DWORD Value)
{
	if( m_pD3DDev != NULL )
	{
		if (m_savedRenderStates[State] != Value || !m_savedRenderStates[State])
		{
			m_savedRenderStates[State] = Value;
			return m_pD3DDev->SetRenderState(State, Value);
		}
	}

	return S_OK;
}

HRESULT CD3DDevWrapper::SetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value)
{
	if( m_pD3DDev != NULL )
	{
		if (m_savedTextureStageStates[Stage][Type] != Value || !m_savedTextureStageStates[Stage][Type])
		{
			switch( Type )
			{
			case D3DTSS_COLORARG1: 
			case D3DTSS_COLORARG2: 
			case D3DTSS_COLORARG0: 
			case D3DTSS_ALPHAARG1: 
			case D3DTSS_ALPHAARG2: 
			case D3DTSS_ALPHAARG0: 
				if( Value != D3DTA_IGNORE )
				{
					m_savedTextureStageStates[Stage][Type] = Value;
					return m_pD3DDev->SetTextureStageState(Stage, Type, Value);
				}
				else
					return S_OK;
				break;
			default:
				m_savedTextureStageStates[Stage][Type] = Value;
				return m_pD3DDev->SetTextureStageState(Stage, Type, Value);
				break;
			}
		}
	}

	return S_OK;
}

HRESULT CD3DDevWrapper::SetPixelShader(IDirect3DPixelShader9* pShader)
{
	if( m_pD3DDev != NULL )
	{
		if( m_savedPixelShader != pShader )
		{
			m_savedPixelShader = pShader;
			return m_pD3DDev->SetPixelShader(pShader);
		}
	}

	return S_OK;
}

HRESULT CD3DDevWrapper::SetPixelShaderConstant(DWORD Register, float* pfdata)
{
	if( m_pD3DDev != NULL )
	{
		//If the shaders were the last shaders to be used ignore the call.
		if(m_savedPixelShaderConstants[Register][0] != pfdata[0] || m_savedPixelShaderConstants[Register][1] != pfdata[1] ||
		   m_savedPixelShaderConstants[Register][2] != pfdata[2] || m_savedPixelShaderConstants[Register][3] != pfdata[3] )
		{
			m_savedPixelShaderConstants[Register][0] = pfdata[0];
			m_savedPixelShaderConstants[Register][1] = pfdata[1];
			m_savedPixelShaderConstants[Register][2] = pfdata[2];
			m_savedPixelShaderConstants[Register][3] = pfdata[3];
			
			return m_pD3DDev->SetPixelShaderConstantF(Register,pfdata,1);
		}
	}

	return S_OK;
}

HRESULT CD3DDevWrapper::SetViewport(D3DVIEWPORT9* pViewport)
{
	if( m_pD3DDev != NULL )
	{
		if( pViewport->Width <= 0 )	pViewport->Width = 1;
		if( pViewport->Height <= 0 )	pViewport->Height = 1;

		//Preliminary support for pillarboxing
		if(windowSetting.uScreenScaleMode == 1)
		{
			float scaleFactor = (4.0 * windowSetting.uDisplayHeight) / (3.0 * windowSetting.uDisplayWidth);
			int offset = (windowSetting.uDisplayWidth - windowSetting.uDisplayHeight * 4 / 3) / 2;

			pViewport->X = pViewport->X * scaleFactor + offset;
			pViewport->Width = pViewport->Width * scaleFactor;
		}
		else if(windowSetting.uScreenScaleMode == 2)
		{
			if (pViewport->Width < windowSetting.uDisplayWidth * 0.9)
			{
				float scaleFactor = (4.0 * windowSetting.uDisplayHeight) / (3.0 * windowSetting.uDisplayWidth);
                int offset = (windowSetting.uDisplayWidth - windowSetting.uDisplayHeight * 4 / 3 - pViewport->Width * scaleFactor / 2) / 2 ;

				pViewport->X = pViewport->X * scaleFactor + offset;
			}
		}

		try
		{
			return m_pD3DDev->SetViewport(pViewport);
		}
		catch(...)
		{
		}
	}

	return S_OK;
}

HRESULT CD3DDevWrapper::SetTexture(DWORD Stage,IDirect3DBaseTexture9* pTexture)
{
	if( m_pD3DDev != NULL )
	{
		//if (m_savedTexturePointers[Stage] != pTexture )
		{
			m_savedTexturePointers[Stage] = pTexture;
			return m_pD3DDev->SetTexture( Stage, pTexture );
		}
	}

	return S_OK;
}

HRESULT CD3DDevWrapper::SetFVF(DWORD FVF)
{
	if( m_pD3DDev != NULL )
	{
		//if( m_savedFVF != FVF )
		{
			m_savedFVF = FVF;
			return m_pD3DDev->SetFVF(FVF);
		}
	}

	return S_OK;
}

void CD3DDevWrapper::SetD3DDev(LPDIRECT3DDEVICE9 pD3DDev)
{
	m_pD3DDev = pD3DDev;
	Initalize();
}

void CD3DDevWrapper::Initalize(void)
{
	m_savedPixelShader = NULL;
	m_savedFVF = ~0;
	memset(&m_savedRenderStates, 0xEE, sizeof(m_savedRenderStates));
	memset(&m_savedPixelShaderConstants, 0xEE, sizeof(m_savedPixelShaderConstants));
	memset(&m_savedTextureStageStates, 0xEE, sizeof(m_savedTextureStageStates));
	memset(&m_savedTexturePointers, 0xEE, sizeof(m_savedTexturePointers));

	for( int i=0; i<8; i++ )
	{
		m_savedTextureStageStates[i][D3DTSS_TEXCOORDINDEX] = ~0;
	}
}
