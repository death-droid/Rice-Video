cv /*
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


MYLPDIRECT3DDEVICE g_pD3DDev = NULL;
CD3DDevWrapper    gD3DDevWrapper;
MYD3DCAPS g_D3DDeviceCaps;
#if DIRECTX_VERSION == 8
DWORD gVertexShader = 0;
#else
LPDIRECT3DVERTEXSHADER9 gVertexShader = NULL;
#endif


#ifdef _XBOX
extern int flickerlevel;
extern float XBINPUT_DEADZONE;
extern float XBINPUT_SUMMAND;
extern bool softlevel;
extern int vertex;
extern int texfilter;
#endif

int FormatToSize(D3DFORMAT fmt)
{
	switch(fmt)
	{
#ifdef _XBOX
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
	case D3DFMT_D24S8:
	case TEXTURE_FMT_A8R8G8B8:
#else
	case D3DFMT_R8G8B8:
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
	case D3DFMT_A2B10G10R10:
	case D3DFMT_G16R16:
	case D3DFMT_INDEX32:
	case D3DFMT_D32:
	case D3DFMT_D24S8:
	case D3DFMT_D24X8:
	case D3DFMT_D24X4S4:
	case TEXTURE_FMT_A8R8G8B8:
#endif
		return 32;
	default:
		/*
		case D3DFMT_R5G6B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
		case D3DFMT_A4R4G4B4:
		case D3DFMT_A8:
		case D3DFMT_R3G3B2:
		case D3DFMT_A8R3G3B2:
		case D3DFMT_X4R4G4B4:
		case D3DFMT_A8P8:
		case D3DFMT_P8:
		case D3DFMT_L8:
		case D3DFMT_A8L8:
		case D3DFMT_A4L4:
		case D3DFMT_INDEX16:
		case D3DFMT_D16_LOCKABLE:
		case D3DFMT_D15S1:
		case D3DFMT_D16:
		*/
		return 16;
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
int				CDXGraphicsContext::m_dwNumAdapters;
D3DAdapterInfo	CDXGraphicsContext::m_1stAdapters;
MYD3DCAPS		CDXGraphicsContext::m_d3dCaps;           // Caps for the device
bool			CDXGraphicsContext::m_bSupportAnisotropy;
#ifdef _XBOX
const	uint32		dwNumDeviceTypes = 1;
#else
const	uint32		dwNumDeviceTypes = 2;
#endif
extern const char*	strDXDeviceDescs[];
const	D3DDEVTYPE	DeviceTypes[] = { D3DDEVTYPE_HAL, D3DDEVTYPE_REF };


CDXGraphicsContext::CDXGraphicsContext() :
	m_pd3dDevice(NULL),
	m_pD3D(NULL),
#ifndef _XBOX
	m_hFont(NULL),
	m_pID3DFont(NULL),
	m_bFontIsCreated(false),
#endif
	m_dwAdapter(0),			// Default Adapter
	m_dwCreateFlags(0),
	m_dwMinDepthBits(16),
	m_dwMinStencilBits(0),
	m_desktopFormat(D3DFMT_A8R8G8B8),
	m_FSAAIsEnabled(false)
{
	m_strDeviceStats[0] = '\0';
}

//*****************************************************************************
//
//*****************************************************************************
CDXGraphicsContext::~CDXGraphicsContext()
{
#ifndef _XBOX
	if( m_hFont )	DeleteObject(m_hFont);
#endif
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

	//if( options.bDisplayOnscreenFPS && !status.bDisableFPS )
	{
		char str[256];
		RECT rect={windowSetting.uDisplayWidth-100,windowSetting.toolbarHeight+windowSetting.uDisplayHeight-100,windowSetting.uDisplayWidth-10,windowSetting.toolbarHeight+windowSetting.uDisplayHeight};
		if( (options.bDisplayOnscreenFPS == ONSCREEN_DISPLAY_DLIST_PER_SECOND ||
			options.bDisplayOnscreenFPS == ONSCREEN_DISPLAY_DLIST_PER_SECOND_WITH_CORE_MSG ) && windowSetting.dps > 0 )
		{
			sprintf(str,"%.1f dps", windowSetting.dps);
			DrawText(str, rect,2);
		}
		else if( (options.bDisplayOnscreenFPS == ONSCREEN_DISPLAY_FRAME_PER_SECOND ||
			options.bDisplayOnscreenFPS == ONSCREEN_DISPLAY_FRAME_PER_SECOND_WITH_CORE_MSG ) && windowSetting.fps > 0 )
		{
			sprintf(str,"%.1f fps", windowSetting.fps);
			DrawText(str, rect,2);
		}
	
		if( options.bDisplayOnscreenFPS >= ONSCREEN_DISPLAY_TEXT_FROM_CORE_ONLY && status.CPUCoreMsgIsSet && strlen(status.CPUCoreMsgToDisplay) > 0 && windowSetting.dps > 0 )
		{
			RECT rect2={10,windowSetting.toolbarHeight+windowSetting.uDisplayHeight-100,windowSetting.uDisplayWidth-100,windowSetting.toolbarHeight+windowSetting.uDisplayHeight};
			DrawText(status.CPUCoreMsgToDisplay, rect2,1);
		}
	}

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
#ifndef _XBOX
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
				// If we are windowed, read the desktop mode and use the same format for
				// the back buffer
				//if( m_bWindowed )
				//{
				//	D3DAdapterInfo* pAdapterInfo = &m_Adapters[m_dwAdapter];
				//	m_pD3D->GetAdapterDisplayMode( m_dwAdapter, &pAdapterInfo->d3ddmDesktop );
				//	m_d3dpp.BackBufferFormat = pAdapterInfo->d3ddmDesktop.Format;
				//}
				
				if( FAILED( hr = ResizeD3DEnvironment() ) )
				{
					goto exit;
				}
			}
			
			// return hr
			goto exit;
		}
#endif

		if( !g_curRomInfo.bForceScreenClear )	
			Clear(CLEAR_DEPTH_BUFFER);

#ifndef _XBOX
		if( m_bWindowed )
		{
			RECT dstrect={0,windowSetting.toolbarHeight,windowSetting.uDisplayWidth,windowSetting.toolbarHeight+windowSetting.uDisplayHeight};
			RECT srcrect={0,0,windowSetting.uDisplayWidth,windowSetting.uDisplayHeight};
			hr = m_pd3dDevice->Present( &srcrect, &dstrect, NULL, NULL );
		}
		else
#endif
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
#ifndef _XBOX
exit:
#endif
	
	Unlock();

	status.bScreenIsDrawn = false;
	if( g_curRomInfo.bForceScreenClear )	needCleanScene = true;
}

//-----------------------------------------------------------------------------
// Name: FindDepthStencilFormat()
// Desc: Finds a depth/stencil format for the given device that is compatible
//       with the render target format and meets the needs of the app.
//-----------------------------------------------------------------------------
BOOL CDXGraphicsContext::FindDepthStencilFormat( UINT iAdapter, D3DDEVTYPE DeviceType,
											  D3DFORMAT TargetFormat,
											  D3DFORMAT* pDepthStencilFormat )
{
#ifdef _XBOX
	*pDepthStencilFormat = D3DFMT_D16;
	return TRUE;
#else
	if( m_dwMinDepthBits <= 16 && m_dwMinStencilBits == 0 )
	{
		if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
			TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D16 ) ) )
		{
			if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
				TargetFormat, TargetFormat, D3DFMT_D16 ) ) )
			{
				
				*pDepthStencilFormat = D3DFMT_D16;
				return TRUE;
			}
		}
	}
	
	if( m_dwMinDepthBits <= 15 && m_dwMinStencilBits <= 1 )
	{
		if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
			TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D15S1 ) ) )
		{
			if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
				TargetFormat, TargetFormat, D3DFMT_D15S1 ) ) )
			{
				*pDepthStencilFormat = D3DFMT_D15S1;
				return TRUE;
			}
		}
	}
	
	if( m_dwMinDepthBits <= 24 && m_dwMinStencilBits == 0 )
	{
		if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
			TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24X8 ) ) )
		{
			if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
				TargetFormat, TargetFormat, D3DFMT_D24X8 ) ) )
			{
				*pDepthStencilFormat = D3DFMT_D24X8;
				return TRUE;
			}
		}
	}
	
	if( m_dwMinDepthBits <= 24 && m_dwMinStencilBits <= 8 )
	{
		if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
			TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8 ) ) )
		{
			if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
				TargetFormat, TargetFormat, D3DFMT_D24S8 ) ) )
			{
				*pDepthStencilFormat = D3DFMT_D24S8;
				return TRUE;
			}
		}
	}
	
	if( m_dwMinDepthBits <= 24 && m_dwMinStencilBits <= 4 )
	{
		if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
			TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24X4S4 ) ) )
		{
			if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
				TargetFormat, TargetFormat, D3DFMT_D24X4S4 ) ) )
			{
				*pDepthStencilFormat = D3DFMT_D24X4S4;
				return TRUE;
			}
		}
	}
	
	if( m_dwMinDepthBits <= 32 && m_dwMinStencilBits == 0 )
	{
		if( SUCCEEDED( m_pD3D->CheckDeviceFormat( iAdapter, DeviceType,
			TargetFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D32 ) ) )
		{
			if( SUCCEEDED( m_pD3D->CheckDepthStencilMatch( iAdapter, DeviceType,
				TargetFormat, TargetFormat, D3DFMT_D32 ) ) )
			{
				*pDepthStencilFormat = D3DFMT_D32;
				return TRUE;
			}
		}
	}
	
	return FALSE;
#endif
}

//*****************************************************************************
//
//*****************************************************************************
extern void WriteConfiguration(void);
bool CDXGraphicsContext::Initialize(HWND hWnd, HWND hWndStatus,
									 uint32 dwWidth, uint32 dwHeight,
									 BOOL bWindowed )
{
	HRESULT hr;

#ifndef _XBOX
	if( g_GraphicsInfo.hStatusBar )
	{
		SetWindowText(g_GraphicsInfo.hStatusBar,"Initializing DirectX Device, please wait");
	}
#endif

	Lock();

	  // Create the Direct3D object
#if DIRECTX_VERSION == 8
	m_pD3D = Direct3DCreate8( D3D_SDK_VERSION );
#else
	m_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
#endif
	if( m_pD3D == NULL )
	{
		Unlock();
		DisplayD3DErrorMsg( D3DAPPERR_NODIRECT3D, MSGERR_APPMUSTEXIT );
		return false;
	}

	// Build a list of Direct3D adapters, modes and devices. The
    // ConfirmDevice() callback is used to confirm that only devices that
    // meet the app's requirements are considered.
#ifndef _XBOX
    if( FAILED( hr = BuildDeviceList() ) )
    {
        if ( m_pD3D )
		{
			m_pD3D->Release();
			m_pD3D = NULL;
		}
		Unlock();
        DisplayD3DErrorMsg( hr, MSGERR_APPMUSTEXIT );
		return false;
    }
#endif

	CGraphicsContext::Initialize(hWnd, hWndStatus, dwWidth, dwHeight, bWindowed );
	
	hr = Create3D( bWindowed );

	// Clear/Update a few times to ensure that all of the buffers are cleared
	if ( m_pd3dDevice )
	{
		for (int i=0; i<DirectXRenderBufferSettings[options.RenderBufferSetting].number+1; i++ )
		{
			Clear(CLEAR_COLOR_AND_DEPTH_BUFFER);
			m_pd3dDevice->BeginScene();
			m_pd3dDevice->EndScene();
			UpdateFrame();
		}
	}

	MYD3DVIEWPORT vp = {
		0, 
			windowSetting.toolbarHeightToUse, 
			windowSetting.uDisplayWidth, 
			windowSetting.uDisplayHeight, 0, 1
	};
	gD3DDevWrapper.SetViewport(&vp);
	
	Unlock();

	g_pD3DDev->GetDeviceCaps(&g_D3DDeviceCaps);
	CGraphicsContext::Get()->m_supportTextureMirror = false;
	if( g_D3DDeviceCaps.TextureAddressCaps & D3DPTADDRESSCAPS_MIRROR )
	{
		CGraphicsContext::Get()->m_supportTextureMirror = true;
	}
	
#ifndef _XBOX
	// Force to use Software T&L
#if DIRECTX_VERSION == 8
	if( options.bForceSoftwareTnL )
		gD3DDevWrapper.SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING,TRUE);
	else
		gD3DDevWrapper.SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING,FALSE);
#else
	if( options.bForceSoftwareTnL )
		g_pD3DDev->SetSoftwareVertexProcessing(TRUE);
	else
		g_pD3DDev->SetSoftwareVertexProcessing(FALSE);
#endif

	if( g_GraphicsInfo.hStatusBar )
	{
		SetWindowText(g_GraphicsInfo.hStatusBar,"DirectX device is ready");
	}
#endif

	return hr==S_OK;
}


//-----------------------------------------------------------------------------
// Name: SortModesCallback()
// Desc: Callback function for sorting display modes (used by BuildDeviceList).
//-----------------------------------------------------------------------------
static int _cdecl SortModesCallback( const VOID* arg1, const VOID* arg2 )
{
#ifndef _XBOX
	D3DDISPLAYMODE* p1 = (D3DDISPLAYMODE*)arg1;
	D3DDISPLAYMODE* p2 = (D3DDISPLAYMODE*)arg2;

	if( p1->Format > p2->Format )   return -1;
	if( p1->Format < p2->Format )   return +1;
	if( p1->Width  < p2->Width )    return -1;
	if( p1->Width  > p2->Width )    return +1;
	if( p1->Height < p2->Height )   return -1;
	if( p1->Height > p2->Height )   return +1;
#endif

	return 0;
}

// This is a static function, will be called when the plugin DLL is initialized
void CDXGraphicsContext::InitDeviceParameters()
{
#ifndef _XBOX
	SetWindowText(m_hWndStatus, "Initialize DirectX Device");
#endif

	// Create Direct3D object
	MYLPDIRECT3D pD3D;
#if DIRECTX_VERSION == 8
	pD3D = Direct3DCreate8( D3D_SDK_VERSION );
#else
	pD3D = Direct3DCreate9( D3D_SDK_VERSION );
#endif
	if( pD3D == NULL )
	{
        DisplayD3DErrorMsg( D3DAPPERR_NODIRECT3D, MSGERR_APPMUSTEXIT );
		return;
	}

#ifndef _XBOX
	// Get number of adapters
	int numAvailableAdapters = pD3D->GetAdapterCount();
#endif

	// For the 1st adapter, looking through each of its devices
	int iAdapter=0;
	D3DAdapterInfo* pAdapter  = &m_1stAdapters;
	pD3D->GetAdapterIdentifier( iAdapter, 0, &pAdapter->d3dAdapterIdentifier );
	pD3D->GetAdapterDisplayMode( iAdapter, &pAdapter->d3ddmDesktop );	// Get current display mode

	// Get number of devices for each adapters
	pAdapter->dwNumDevices    = dwNumDeviceTypes;
	pAdapter->dwCurrentDevice = 0;

	// Add devices to adapter
	for( UINT iDevice = 0; iDevice < dwNumDeviceTypes; iDevice++ )
	{
		// Fill in device info
		D3DDeviceInfo* pDevice	= &pAdapter->devices[iDevice];
		pDevice->DeviceType     = DeviceTypes[iDevice];
		pDevice->strDesc        = strDXDeviceDescs[iDevice];

		// for each devices in each adapter
		// Get device caps
		pD3D->GetDeviceCaps( iAdapter, DeviceTypes[iDevice], &pDevice->d3dCaps );

		pDevice->dwNumModes     = 0;
		pDevice->MultiSampleType = D3DMULTISAMPLE_NONE;
	}

	// Check FSAA maximum
	for( m_maxFSAA = 16; m_maxFSAA >= 2; m_maxFSAA-- )
	{
#if DIRECTX_VERSION == 8
		if( SUCCEEDED(pD3D->CheckDeviceMultiSampleType( D3DADAPTER_DEFAULT, 
			D3DDEVTYPE_HAL , D3DFMT_X8R8G8B8, FALSE, D3DMULTISAMPLE_TYPE(D3DMULTISAMPLE_NONE+m_maxFSAA) ) ) )
#else
		if( SUCCEEDED(pD3D->CheckDeviceMultiSampleType( D3DADAPTER_DEFAULT, 
			D3DDEVTYPE_HAL , D3DFMT_X8R8G8B8, FALSE, D3DMULTISAMPLE_TYPE(D3DMULTISAMPLE_NONE+m_maxFSAA), NULL ) ) )
#endif
		{
			break;
		}
		else
		{
			continue;
		}
	}

	if( m_maxFSAA < 2 )
	{
		m_maxFSAA = 0;
		TRACE0("Device does not support FSAA");
	}
	else
	{
		TRACE1("Device support FSAA up to %d X", m_maxFSAA);
	}
	if( options.DirectXMaxFSAA != m_maxFSAA )
	{
		options.DirectXMaxFSAA = m_maxFSAA;
		WriteConfiguration();
	}


	// Check Anisotropy Filtering maximum

	m_maxAnisotropy = pAdapter->devices[0].d3dCaps.MaxAnisotropy;
	TRACE1("Max Anisotropy: %d", m_maxAnisotropy);
	if( options.DirectXMaxAnisotropy != m_maxAnisotropy )
	{
		options.DirectXMaxAnisotropy = m_maxAnisotropy;
		WriteConfiguration();
	}

	status.isVertexShaderSupported = D3DSHADER_VERSION_MAJOR( pAdapter->devices[0].d3dCaps.VertexShaderVersion ) >= 1;
	status.isVertexShaderEnabled = status.isVertexShaderSupported && options.bEnableVertexShader;
	//status.isVertexShaderEnabled = false;	// Disable it for now
	//status.bUseHW_T_L = true;				// Debug hardware T&L so to debug vertex shader

	// Get available refresh rates

	// Get available color quality, 16 bit and 32 bit

	// Get available back buffer format

	// Get available depth buffer format

	// Release the Direct3D object
	pD3D->Release();
}


//*****************************************************************************
//
//*****************************************************************************
HRESULT CDXGraphicsContext::Create3D( BOOL bWindowed )
{
    HRESULT hr;
	
    // Get access to current adapter, device, and mode
    D3DAdapterInfo* pAdapterInfo = &m_Adapters[m_dwAdapter];
	D3DDeviceInfo*  pDeviceInfo  = &pAdapterInfo->devices[options.DirectXDevice];
	
 	m_bWindowed = (bWindowed==TRUE);		// Make user Toggle manually for now!
    pDeviceInfo->bWindowed = m_bWindowed;

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
    HRESULT hr;
	
    D3DAdapterInfo* pAdapterInfo = &m_Adapters[m_dwAdapter];
	D3DDeviceInfo*  pDeviceInfo  = &pAdapterInfo->devices[options.DirectXDevice];
    D3DModeInfo*    pModeInfo    = &pDeviceInfo->modes[FindCurrentDisplayModeIndex()];
	
    // Prepare window for possible windowed/fullscreen change
    AdjustWindowForChange();

	windowSetting.statusBarHeightToUse = 0;
	windowSetting.toolbarHeightToUse = 0;

    // Set up the presentation parameters
    ZeroMemory( &m_d3dpp, sizeof(m_d3dpp) );

#ifdef _XBOX
	m_d3dpp.Windowed               = FALSE;
	m_d3dpp.BackBufferCount        = 1;
	//m_d3dpp.MultiSampleType        = D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_QUINCUNX;
	//m_d3dpp.SwapEffect             = bufferSettings[curBufferSetting].swapEffect;
	m_d3dpp.SwapEffect             =  D3DSWAPEFFECT_COPY;
	m_d3dpp.EnableAutoDepthStencil = TRUE; /*m_bUseDepthBuffer;*/
	//m_d3dpp.AutoDepthStencilFormat = pModeInfo->DepthStencilFormat;
	m_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	m_d3dpp.hDeviceWindow          = m_hWnd;
	m_d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	m_d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	DWORD videoFlags = XGetVideoFlags();
	if(XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I)
	{
		if(videoFlags & XC_VIDEO_FLAGS_PAL_60Hz)		// PAL 60 user
			m_d3dpp.FullScreen_RefreshRateInHz = 60;
		else
			m_d3dpp.FullScreen_RefreshRateInHz = 50;
	}

	if( videoFlags&XC_VIDEO_FLAGS_HDTV_480p )
	{
		m_d3dpp.Flags = D3DPRESENTFLAG_PROGRESSIVE ;
	}

	m_d3dpp.BackBufferWidth = 640;
	m_d3dpp.BackBufferHeight = 480;
	m_d3dpp.BackBufferFormat = D3DFMT_X1R5G5B5;
	//m_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;

	windowSetting.uDisplayWidth = m_d3dpp.BackBufferWidth;
	windowSetting.uDisplayHeight = m_d3dpp.BackBufferHeight;

	m_desktopFormat = D3DFMT_X1R5G5B5;
	//m_desktopFormat = D3DFMT_A8R8G8B8;

	//freakdave
	if(vertex == 0){
		// Create the device
		hr = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
			NULL, D3DCREATE_PUREDEVICE, &m_d3dpp,
			&m_pd3dDevice );

	}


	if(vertex == 1){
		// Create the device
		hr = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
			NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_d3dpp,
			&m_pd3dDevice );

	}

	if(vertex == 2){
		// Create the device
		hr = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
			NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &m_d3dpp,
			&m_pd3dDevice );

	}

	if(vertex == 3){
		// Create the device
		hr = m_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
			NULL, D3DCREATE_MIXED_VERTEXPROCESSING, &m_d3dpp,
			&m_pd3dDevice );

	}
#else
    m_d3dpp.Windowed               = pDeviceInfo->bWindowed;
    m_d3dpp.BackBufferCount        = DirectXRenderBufferSettings[options.RenderBufferSetting].number;
	m_d3dpp.EnableAutoDepthStencil = TRUE; /*m_bUseDepthBuffer;*/
    m_d3dpp.AutoDepthStencilFormat = pModeInfo->DepthStencilFormat;
	m_d3dpp.AutoDepthStencilFormat = (D3DFORMAT)(DirectXDepthBufferSetting[options.DirectXDepthBufferSetting].number);
	m_d3dpp.hDeviceWindow          = m_hWnd;
	m_d3dpp.Flags				   = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	m_d3dpp.MultiSampleType        = pDeviceInfo->MultiSampleType;
	m_FSAAIsEnabled = false;
	if( pDeviceInfo->MultiSampleType != D3DMULTISAMPLE_NONE && m_maxFSAA > 0 )
	{
#if DIRECTX_VERSION == 8
		if( SUCCEEDED(m_pD3D->CheckDeviceMultiSampleType( D3DADAPTER_DEFAULT, 
			//D3DDEVTYPE_HAL , pAdapterInfo->d3ddmDesktop.Format, 
			D3DDEVTYPE_HAL , pModeInfo->Format, 
			m_bWindowed, pDeviceInfo->MultiSampleType ) ) )
#else
		if( SUCCEEDED(m_pD3D->CheckDeviceMultiSampleType( D3DADAPTER_DEFAULT, 
			//D3DDEVTYPE_HAL , pAdapterInfo->d3ddmDesktop.Format, 
			D3DDEVTYPE_HAL , pModeInfo->Format, 
			m_bWindowed, pDeviceInfo->MultiSampleType, NULL ) ) )
#endif
		{
			m_FSAAIsEnabled = true;
			TRACE1("Start with FSAA=%d X", pDeviceInfo->MultiSampleType-D3DMULTISAMPLE_NONE);
		}
		else
		{
			m_d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
			TRACE1("Cannot use Anti-Alias mode: %d", options.DirectXAntiAliasingValue);
		}
	}
	else
	{
		m_d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
		TRACE1("Cannot use Anti-Alias mode: %d", options.DirectXAntiAliasingValue);
	}

	if( currentRomOptions.N64FrameBufferEmuType != FRM_BUF_NONE && m_FSAAIsEnabled )
	{
		m_FSAAIsEnabled = false;
		m_d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
		TRACE0("FSAA is turned off in order to use BackBuffer emulation");
		SetWindowText(g_GraphicsInfo.hStatusBar, "FSAA is turned off in order to use BackBuffer emulation");
	}

    if( m_bWindowed )
    {
		if( !m_FSAAIsEnabled )
			m_d3dpp.SwapEffect		= D3DSWAPEFFECT_COPY;	// Always use COPY for window mode
		else
			m_d3dpp.SwapEffect		= D3DSWAPEFFECT_DISCARD;	// Anti-Aliasing mode
		windowSetting.uDisplayWidth = windowSetting.uWindowDisplayWidth;
		windowSetting.uDisplayHeight= windowSetting.uWindowDisplayHeight;

		m_d3dpp.BackBufferFormat	= pAdapterInfo->d3ddmDesktop.Format;
		m_d3dpp.FullScreen_RefreshRateInHz = 0;
    }
    else
    {
		if( !m_FSAAIsEnabled )
			m_d3dpp.SwapEffect		= (D3DSWAPEFFECT)DirectXRenderBufferSettings[options.RenderBufferSetting].setting;
		else
			m_d3dpp.SwapEffect		= D3DSWAPEFFECT_DISCARD;	// Anti-Aliasing mode
		windowSetting.uDisplayWidth = windowSetting.uFullScreenDisplayWidth;
		windowSetting.uDisplayHeight = windowSetting.uFullScreenDisplayHeight;
        //m_d3dpp.BackBufferFormat	= pAdapterInfo->d3ddmDesktop.Format;
        m_d3dpp.BackBufferFormat	= pModeInfo->Format;
		
		m_d3dpp.FullScreen_RefreshRateInHz = windowSetting.uFullScreenRefreshRate;
		if( m_d3dpp.FullScreen_RefreshRateInHz > pModeInfo->RefreshRate )
		{
			m_d3dpp.FullScreen_RefreshRateInHz = pModeInfo->RefreshRate;
			windowSetting.uFullScreenRefreshRate = pModeInfo->RefreshRate;
		}
    }
	m_desktopFormat = pAdapterInfo->d3ddmDesktop.Format;
	m_d3dpp.BackBufferWidth		= windowSetting.uDisplayWidth;
	m_d3dpp.BackBufferHeight	= windowSetting.uDisplayHeight;

	
    // Create the device
	hr = m_pD3D->CreateDevice( m_dwAdapter, pDeviceInfo->DeviceType,m_hWnd, pModeInfo->dwBehavior, &m_d3dpp,	&m_pd3dDevice );
	if( !SUCCEEDED(hr) && m_d3dpp.MultiSampleType == D3DMULTISAMPLE_NONE )
	{
		// Try again without FSAA
		TRACE0("Can not initialize DX8, try again without FSAA");
		SetWindowText(g_GraphicsInfo.hStatusBar,"Can not initialize DX8, try again without FSAA");
		m_d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
		hr = m_pD3D->CreateDevice( m_dwAdapter, pDeviceInfo->DeviceType,m_hWnd, pModeInfo->dwBehavior, &m_d3dpp,	&m_pd3dDevice );
	}
#endif

    if( SUCCEEDED(hr) && m_pd3dDevice )
    {
		g_pD3DDev = m_pd3dDevice;
		gD3DDevWrapper.SetD3DDev(m_pd3dDevice);
		
        // When moving from fullscreen to windowed mode, it is important to
        // adjust the window size after recreating the device rather than
        // beforehand to ensure that you get the window size you want.  For
        // example, when switching from 640x480 fullscreen to windowed with
        // a 1000x600 window on a 1024x768 desktop, it is impossible to set
        // the window size to 1000x600 until after the display mode has
        // changed to 1024x768, because windows cannot be larger than the
        // desktop.
		
        // Store device Caps
        m_pd3dDevice->GetDeviceCaps( &m_d3dCaps );

#ifdef _XBOX
		//freakdave
		if(vertex == 0){
			m_dwCreateFlags = D3DCREATE_PUREDEVICE;
		}

		if(vertex == 1){
			m_dwCreateFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		}

		if(vertex == 2){
			m_dwCreateFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
		}

		if(vertex == 3){
			m_dwCreateFlags = D3DCREATE_MIXED_VERTEXPROCESSING;
		}
#else
        m_dwCreateFlags = pModeInfo->dwBehavior;
		
        if( m_bWindowed )
        {
            SetWindowPos( m_hWnd, HWND_NOTOPMOST,
						m_rcWindowBounds.left, m_rcWindowBounds.top,
						( m_rcWindowBounds.right - m_rcWindowBounds.left ),
						( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
						SWP_SHOWWINDOW );
        }

        // Store device description
        if( pDeviceInfo->DeviceType == D3DDEVTYPE_REF )
            lstrcpy( m_strDeviceStats, "REF" );
        else if( pDeviceInfo->DeviceType == D3DDEVTYPE_HAL )
            lstrcpy( m_strDeviceStats, "HAL" );
        else if( pDeviceInfo->DeviceType == D3DDEVTYPE_SW )
            lstrcpy( m_strDeviceStats, "SW" );
		
        if( pModeInfo->dwBehavior & D3DCREATE_HARDWARE_VERTEXPROCESSING &&
            pModeInfo->dwBehavior & D3DCREATE_PUREDEVICE )
        {
            if( pDeviceInfo->DeviceType == D3DDEVTYPE_HAL )
                lstrcat( m_strDeviceStats, " (pure hw vp)" );
            else
                lstrcat( m_strDeviceStats, " (simulated pure hw vp)" );
        }
        else if( pModeInfo->dwBehavior & D3DCREATE_HARDWARE_VERTEXPROCESSING )
        {
            if( pDeviceInfo->DeviceType == D3DDEVTYPE_HAL )
                lstrcat( m_strDeviceStats, " (hw vp)" );
            else
                lstrcat( m_strDeviceStats, " (simulated hw vp)" );
        }
        else if( pModeInfo->dwBehavior & D3DCREATE_MIXED_VERTEXPROCESSING )
        {
            if( pDeviceInfo->DeviceType == D3DDEVTYPE_HAL )
                lstrcat( m_strDeviceStats, " (mixed vp)" );
            else
                lstrcat( m_strDeviceStats, " (simulated mixed vp)" );
        }
        else if( pModeInfo->dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING )
        {
            lstrcat( m_strDeviceStats, " (sw vp)" );
        }
		
        if( pDeviceInfo->DeviceType == D3DDEVTYPE_HAL )
        {
            lstrcat( m_strDeviceStats, ": " );
            lstrcat( m_strDeviceStats, pAdapterInfo->d3dAdapterIdentifier.Description );
        }
	
		if ( IsWindow(m_hWndStatus ) )
		{
			SetWindowText(m_hWndStatus, m_strDeviceStats);
		}
		TRACE0(m_strDeviceStats);
#endif

		
        // Store render target surface desc
        MYLPDIRECT3DSURFACE pBackBuffer;
#if DIRECTX_VERSION == 8
        m_pd3dDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
#else
        m_pd3dDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
#endif
        pBackBuffer->GetDesc( &m_d3dsdBackBuffer );
        pBackBuffer->Release();
		
        // Initialize the app's device-dependent objects
        hr = InitDeviceObjects();
        if( IsResultGood(hr,true) )
        {
			if ( CRender::IsAvailable() )
			{
				CRender::GetRender()->InitDeviceObjects();
			}
            m_bActive = true;
            return S_OK;
        }
		
        // Cleanup before we try again (- shouldn't get here)
        CleanDeviceObjects();
        SAFE_RELEASE( m_pd3dDevice );
    }
	else
	{
#ifndef _XBOX
		if( status.ToToggleFullScreen || !m_bWindowed )
			SetWindowText(g_GraphicsInfo.hStatusBar,"Can not initialize DX8, check your Direct settings");
		else
			MsgInfo("Can not initialize DX8, check your Direct settings");
#endif
	}
	
	/*
	extern MYLPDIRECT3DSURFACE g_pLockableBackBuffer;
	if( g_pLockableBackBuffer == NULL )
	{
		if( IsResultGood(g_pD3DDev->CreateDepthStencilSurface(windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, D3DFMT_D16_LOCKABLE, D3DMULTISAMPLE_NONE, &g_pLockableBackBuffer)) && g_pLockableBackBuffer )
		{
#if DIRECTX_VERSION == 8
			g_pD3DDev->SetRenderTarget(NULL, g_pLockableBackBuffer);
#else
			g_pD3DDev->SetRenderTarget(0, g_pLockableBackBuffer);
#endif
			TRACE0("Created and use lockable depth buffer");
		}
		else
		{
			if( g_pLockableBackBuffer )
			{
				g_pLockableBackBuffer->Release();
				g_pLockableBackBuffer = NULL;
			}
			TRACE0("Can not create lockable depth buffer");
		}
	}
	*/

    // If that failed, fall back to the reference rasterizer (removed)
	
    return hr;
}


//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::ResizeD3DEnvironment()
{
#ifndef _XBOX
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
    MYLPDIRECT3DSURFACE pBackBuffer;
#if DIRECTX_VERSION == 8
    m_pd3dDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
#else
    m_pd3dDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
#endif
    pBackBuffer->GetDesc( &m_d3dsdBackBuffer );
    pBackBuffer->Release();
	
    // Initialize the app's device-dependent objects
	if ( CRender::IsAvailable() )
	{
		CRender::GetRender()->InitDeviceObjects();
	}
#endif
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: ToggleFullScreen()
// Desc: Called when user toggles between fullscreen mode and windowed mode
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::DoToggleFullscreen()
{
#ifndef _XBOX
    // Get access to current adapter, device, and mode
	D3DAdapterInfo* pAdapterInfo = &m_Adapters[m_dwAdapter];
	//D3DDeviceInfo*  pDeviceInfo  = &pAdapterInfo->devices[pAdapterInfo->dwCurrentDevice];
	D3DDeviceInfo*  pDeviceInfo  = &pAdapterInfo->devices[options.DirectXDevice];
    D3DModeInfo*    pModeInfo    = &pDeviceInfo->modes[FindCurrentDisplayModeIndex()];
	
    // Need device change if going windowed and the current device
    // can only be fullscreen

   if( !m_bWindowed && !pDeviceInfo->bCanDoWindowed )
	{
		return ForceWindowed();
	}
	
    m_bReady = false;
	
    // Toggle the windowed state
    m_bWindowed = !m_bWindowed;
    pDeviceInfo->bWindowed = m_bWindowed;
	
    // Prepare window for windowed/fullscreen change
    AdjustWindowForChange();
	
	Lock();
	CRender::GetRender()->CleanUp();

	CleanUp();
#if DIRECTX_VERSION == 8
    m_pD3D = Direct3DCreate8( D3D_SDK_VERSION );
#else
    m_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
#endif
    if( m_pD3D == NULL )
	{
		TRACE0("Error to create m_pD3D");
		Unlock();
        return DisplayD3DErrorMsg( D3DAPPERR_NODIRECT3D, MSGERR_APPMUSTEXIT );
	}
	InitializeD3D();
	Unlock();

	
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
        SetWindowPos( m_hWnd, HWND_NOTOPMOST,
			m_rcWindowBounds.left, m_rcWindowBounds.top,
			( m_rcWindowBounds.right - m_rcWindowBounds.left ),
			( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
			SWP_SHOWWINDOW );
    }
#endif
    m_bReady = true;
	
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ForceWindowed()
// Desc: Switch to a windowed mode, even if that means picking a new device
//       and/or adapter
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::ForceWindowed()
{
#ifndef _XBOX
    HRESULT hr;
    D3DAdapterInfo* pAdapterInfoCur = &m_Adapters[m_dwAdapter];
    //D3DDeviceInfo*  pDeviceInfoCur  = &pAdapterInfoCur->devices[pAdapterInfoCur->dwCurrentDevice];
	D3DDeviceInfo*  pDeviceInfoCur  = &pAdapterInfoCur->devices[options.DirectXDevice];
    BOOL bFoundDevice = FALSE;
	
    if( pDeviceInfoCur->bCanDoWindowed )
    {
        bFoundDevice = TRUE;
    }
    else
    {
        // Look for a windowable device on any adapter
        D3DAdapterInfo* pAdapterInfo;
        int dwAdapter;
        D3DDeviceInfo* pDeviceInfo;
        int dwDevice;
        for( dwAdapter = 0; dwAdapter < m_dwNumAdapters; dwAdapter++ )
        {
            pAdapterInfo = &m_Adapters[dwAdapter];
            for( dwDevice = 0; dwDevice < pAdapterInfo->dwNumDevices; dwDevice++ )
            {
                pDeviceInfo = &pAdapterInfo->devices[dwDevice];
                if( pDeviceInfo->bCanDoWindowed )
                {
                    m_dwAdapter = dwAdapter;
                    pDeviceInfoCur = pDeviceInfo;
                    pAdapterInfo->dwCurrentDevice = dwDevice;
                    bFoundDevice = TRUE;
                    break;
                }
            }
            if( bFoundDevice )
                break;
        }
    }
	
    if( !bFoundDevice )
        return E_FAIL;
	
    pDeviceInfoCur->bWindowed = true;
    m_bWindowed = true;
	
    // Now destroy the current 3D device objects, then reinitialize
	
    m_bReady = false;
	
    // Release all scene objects that will be re-created for the new device
    CleanDeviceObjects();
	
    // Release display objects, so a new device can be created
	LONG nRefCount = m_pd3dDevice->Release();
    if( nRefCount > 0L )
	{
        return DisplayD3DErrorMsg( D3DAPPERR_NONZEROREFCOUNT, MSGERR_APPMUSTEXIT );
	}
	
    // Create the new device
    if( FAILED( hr = InitializeD3D() ) )
        return DisplayD3DErrorMsg( hr, MSGERR_APPMUSTEXIT );

#endif
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
#ifndef _XBOX
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
        SetWindowLong( m_hWnd, GWL_STYLE, WS_POPUP|WS_SYSMENU|WS_VISIBLE );
    }
#endif
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: BuildDeviceList()
// Desc: From DX8 SDK Copyright (c) 1998-2000 Microsoft
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::BuildDeviceList()
{
#ifndef _XBOX
    BOOL bHALExists = FALSE;
    BOOL bHALIsWindowedCompatible = FALSE;
    BOOL bHALIsDesktopCompatible = FALSE;
    BOOL bHALIsSampleCompatible = FALSE;

	m_dwNumAdapters = 0;
	
    // Loop through all the adapters on the system (usually, there's just one
    // unless more than one graphics card is present).
    for( UINT iAdapter = 0; iAdapter < m_pD3D->GetAdapterCount(); iAdapter++ )
    {
        // Fill in adapter info
        D3DAdapterInfo* pAdapter  = &m_Adapters[m_dwNumAdapters];
        m_pD3D->GetAdapterIdentifier( iAdapter, 0, &pAdapter->d3dAdapterIdentifier );
        m_pD3D->GetAdapterDisplayMode( iAdapter, &pAdapter->d3ddmDesktop );
        pAdapter->dwNumDevices    = 0;
        pAdapter->dwCurrentDevice = 0;
		
		//
        // Enumerate all display modes on this adapter
		//
        D3DDISPLAYMODE modes[100];
		D3DFORMAT      formats[20];
        uint32 dwNumFormats      = 0;
        uint32 dwNumModes        = 0;
#if DIRECTX_VERSION == 8
        uint32 dwNumAdapterModes = m_pD3D->GetAdapterModeCount( iAdapter );
#else
        uint32 dwNumAdapterModes = m_pD3D->GetAdapterModeCount( iAdapter, D3DFMT_X8R8G8B8 );
#endif
		
        // Add the adapter's current desktop format to the list of formats
        formats[dwNumFormats++] = pAdapter->d3ddmDesktop.Format;
		
        for( UINT iMode = 0; iMode < dwNumAdapterModes; iMode++ )
        {
            // Get the display mode attributes
            D3DDISPLAYMODE DisplayMode;
#if DIRECTX_VERSION == 8
            m_pD3D->EnumAdapterModes( iAdapter, iMode, &DisplayMode );
#else
           m_pD3D->EnumAdapterModes( iAdapter, D3DFMT_X8R8G8B8, iMode, &DisplayMode );
#endif
			
            // Filter out low-resolution modes
            if( DisplayMode.Width  < 320 || DisplayMode.Height < 200 )
                continue;

			uint32 m;
			
            // Check if the mode already exists (to filter out refresh rates)
            for( m=0L; m<dwNumModes; m++ )
            {
                if( ( modes[m].Width  == DisplayMode.Width  ) &&
                    ( modes[m].Height == DisplayMode.Height ) &&
                    ( modes[m].Format == DisplayMode.Format ) )
				{
					if( modes[m].RefreshRate < DisplayMode.RefreshRate )
					{
						modes[m].RefreshRate = DisplayMode.RefreshRate;
					}
                    break;
				}
            }
			
            // If we found a new mode, add it to the list of modes
            if( m == dwNumModes )
            {
                modes[dwNumModes].Width       = DisplayMode.Width;
                modes[dwNumModes].Height      = DisplayMode.Height;
                modes[dwNumModes].Format      = DisplayMode.Format;
                modes[dwNumModes].RefreshRate = DisplayMode.RefreshRate;
                dwNumModes++;

				uint32 f;
				
                // Check if the mode's format already exists
                for( f=0; f<dwNumFormats; f++ )
                {
                    if( DisplayMode.Format == formats[f] )
                        break;
                }
				
                // If the format is new, add it to the list
                if( f== dwNumFormats )
                    formats[dwNumFormats++] = DisplayMode.Format;
            }
        }
		
        // Sort the list of display modes (by format, then width, then height)
        qsort( modes, dwNumModes, sizeof(D3DDISPLAYMODE), SortModesCallback );
		
        // Add devices to adapter
        for( UINT iDevice = 0; iDevice < dwNumDeviceTypes; iDevice++ )
        {
            // Fill in device info
            D3DDeviceInfo* pDevice;
            pDevice                 = &pAdapter->devices[pAdapter->dwNumDevices];
            pDevice->DeviceType     = DeviceTypes[iDevice];
            m_pD3D->GetDeviceCaps( iAdapter, DeviceTypes[iDevice], &pDevice->d3dCaps );
            pDevice->strDesc        = strDXDeviceDescs[iDevice];
            pDevice->dwNumModes     = 0;
            pDevice->bCanDoWindowed = false;
            pDevice->bWindowed      = false;
			if( options.DirectXAntiAliasingValue == 1 )
				options.DirectXAntiAliasingValue = 0;
			else if( options.DirectXAntiAliasingValue > 16 )
				options.DirectXAntiAliasingValue = 16;

			if( m_maxFSAA > 0 )
				pDevice->MultiSampleType = (D3DMULTISAMPLE_TYPE)(D3DMULTISAMPLE_NONE+min(m_maxFSAA,(int)options.DirectXAntiAliasingValue));
			else
				pDevice->MultiSampleType = D3DMULTISAMPLE_NONE;
			
            // Examine each format supported by the adapter to see if it will
            // work with this device and meets the needs of the application.
            BOOL  bFormatConfirmed[20];
            uint32 dwBehavior[20];
            D3DFORMAT fmtDepthStencil[20];
			
            for( uint32 f=0; f<dwNumFormats; f++ )
            {
                bFormatConfirmed[f] = FALSE;
                fmtDepthStencil[f] = D3DFMT_UNKNOWN;
				
                // Skip formats that cannot be used as render targets on this device
                if( FAILED( m_pD3D->CheckDeviceType( iAdapter, pDevice->DeviceType,
					formats[f], formats[f], FALSE ) ) )
                    continue;
				
                if( pDevice->DeviceType == D3DDEVTYPE_HAL )
                {
                    // This system has a HAL device
                    bHALExists = TRUE;
					
#if DIRECTX_VERSION == 8
                    if( pDevice->d3dCaps.Caps2 & D3DCAPS2_CANRENDERWINDOWED )
#else
					// Assume all DirectX9 can render window mode
                    //if( pDevice->d3dCaps.Caps2 & D3DCAPS2_CANRENDERWINDOWED )
#endif
                    {
                        // HAL can run in a window for some mode
                        bHALIsWindowedCompatible = TRUE;
						
                        if( f == 0 )
                        {
                            // HAL can run in a window for the current desktop mode
                            bHALIsDesktopCompatible = TRUE;
                        }
                    }
                }
				
                // Confirm the device/format for HW vertex processing
                if( pDevice->d3dCaps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT )
                {
					bFormatConfirmed[f] = FALSE;

					if( options.bForceSoftwareTnL )
					{
						dwBehavior[f] = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

						if( SUCCEEDED( ConfirmDevice( &pDevice->d3dCaps, dwBehavior[f], formats[f] ) ) )
						{
							bFormatConfirmed[f] = TRUE;
						}
						
						if ( FALSE == bFormatConfirmed[f] )
						{
							dwBehavior[f] = D3DCREATE_MIXED_VERTEXPROCESSING;

							if( SUCCEEDED( ConfirmDevice( &pDevice->d3dCaps, dwBehavior[f], formats[f] ) ) )
								bFormatConfirmed[f] = TRUE;
						}
					}

					if ( FALSE == bFormatConfirmed[f] )
					{
						if( pDevice->d3dCaps.DevCaps&D3DDEVCAPS_PUREDEVICE )
						{
							dwBehavior[f] = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;

							if( SUCCEEDED( ConfirmDevice( &pDevice->d3dCaps, dwBehavior[f], formats[f] ) ) )
								bFormatConfirmed[f] = TRUE;
						}
					}

					if ( FALSE == bFormatConfirmed[f] )
					{
						dwBehavior[f] = D3DCREATE_HARDWARE_VERTEXPROCESSING;

						if( SUCCEEDED( ConfirmDevice( &pDevice->d3dCaps, dwBehavior[f], formats[f] ) ) )
							bFormatConfirmed[f] = TRUE;
					}

					if ( FALSE == bFormatConfirmed[f] )
					{
						dwBehavior[f] = D3DCREATE_MIXED_VERTEXPROCESSING;

						if( SUCCEEDED( ConfirmDevice( &pDevice->d3dCaps, dwBehavior[f], formats[f] ) ) )
							bFormatConfirmed[f] = TRUE;
					}
                }
				
                // Confirm the device/format for SW vertex processing
                if( FALSE == bFormatConfirmed[f] )
                {
                    dwBehavior[f] = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
					
                    if( SUCCEEDED( ConfirmDevice( &pDevice->d3dCaps, dwBehavior[f], formats[f] ) ) )
                        bFormatConfirmed[f] = TRUE;
                }
				
                // Find a suitable depth/stencil buffer format for this device/format
                if( bFormatConfirmed[f] ) //&& m_bUseDepthBuffer
                {
                    if( !FindDepthStencilFormat( iAdapter, pDevice->DeviceType, formats[f], &fmtDepthStencil[f] ) )
                    {
                        bFormatConfirmed[f] = FALSE;
                    }
                }
            }
			
            // Add all enumerated display modes with confirmed formats to the
            // device's list of valid modes
            for( uint32 m=0L; m<dwNumModes; m++ )
            {
                for( uint32 f=0; f<dwNumFormats; f++ )
                {
                    if( modes[m].Format == formats[f] )
                    {
                        if( bFormatConfirmed[f] == TRUE )
                        {
                            // Add this mode to the device's list of valid modes
                            pDevice->modes[pDevice->dwNumModes].Width			= modes[m].Width;
                            pDevice->modes[pDevice->dwNumModes].Height			= modes[m].Height;
                            pDevice->modes[pDevice->dwNumModes].Format			= modes[m].Format;
							pDevice->modes[pDevice->dwNumModes].RefreshRate     = modes[m].RefreshRate;
                            pDevice->modes[pDevice->dwNumModes].dwBehavior		= dwBehavior[f];
                            pDevice->modes[pDevice->dwNumModes].DepthStencilFormat = fmtDepthStencil[f];
                            pDevice->dwNumModes++;
							
                            if( pDevice->DeviceType == D3DDEVTYPE_HAL )
                                bHALIsSampleCompatible = TRUE;
                        }
                    }
                }
            }
			
            // Check if the device is compatible with the desktop display mode
            // (which was added initially as formats[0])
#if DIRECTX_VERSION == 8
            if( bFormatConfirmed[0] && (pDevice->d3dCaps.Caps2 & D3DCAPS2_CANRENDERWINDOWED) )
#else
            if( bFormatConfirmed[0] ) //&& (pDevice->d3dCaps.Caps2 & D3DCAPS2_CANRENDERWINDOWED) )
#endif
            {
                pDevice->bCanDoWindowed = true;
                pDevice->bWindowed      = true;
            }
			
            // If valid modes were found, keep this device
            if( pDevice->dwNumModes > 0 )
                pAdapter->dwNumDevices++;
        }
		
        // If valid devices were found, keep this adapter
        if( pAdapter->dwNumDevices > 0 )
            m_dwNumAdapters++;
    }
	
    // Return an error if no compatible devices were found
    if( 0L == m_dwNumAdapters )
        return D3DAPPERR_NOCOMPATIBLEDEVICES;
	
    // Pick a default device that can render into a window
    // (This code assumes that the HAL device comes before the REF
    // device in the device array).
    for( int a=0; a<m_dwNumAdapters; a++ )
    {
        for( int d=0; d < m_Adapters[a].dwNumDevices; d++ )
        {
            if( m_Adapters[a].devices[d].bWindowed )
            {
                m_Adapters[a].dwCurrentDevice = d;
                m_dwAdapter = a;
				m_bWindowed = true;
				
                // Display a warning message
                if( m_Adapters[a].devices[d].DeviceType == D3DDEVTYPE_REF )
                {
                    if( !bHALExists )
                        DisplayD3DErrorMsg( D3DAPPERR_NOHARDWAREDEVICE, MSGWARN_SWITCHEDTOREF );
                    else if( !bHALIsSampleCompatible )
                        DisplayD3DErrorMsg( D3DAPPERR_HALNOTCOMPATIBLE, MSGWARN_SWITCHEDTOREF );
                    else if( !bHALIsWindowedCompatible )
                        DisplayD3DErrorMsg( D3DAPPERR_NOWINDOWEDHAL, MSGWARN_SWITCHEDTOREF );
                    else if( !bHALIsDesktopCompatible )
                        DisplayD3DErrorMsg( D3DAPPERR_NODESKTOPHAL, MSGWARN_SWITCHEDTOREF );
                    else // HAL is desktop compatible, but not sample compatible
                        DisplayD3DErrorMsg( D3DAPPERR_NOHALTHISMODE, MSGWARN_SWITCHEDTOREF );
                }
				
                return S_OK;
            }
        }
    }
	
    return D3DAPPERR_NOWINDOWABLEDEVICES;
#else
	return S_OK;
#endif
}

int	CDXGraphicsContext::FindCurrentDisplayModeIndex()
{
	D3DDISPLAYMODE dMode;
	D3DAdapterInfo &adapter = m_Adapters[m_dwAdapter];
	//D3DDeviceInfo &device = adapter.devices[adapter.dwCurrentDevice];
	D3DDeviceInfo &device = adapter.devices[options.DirectXDevice];
	int m;

	m_pD3D->GetAdapterDisplayMode(m_dwAdapter,&dMode);

	for( m=0; m<device.dwNumModes; m++ )
	{
		if( device.modes[m].Width==windowSetting.uFullScreenDisplayWidth && device.modes[m].Height==windowSetting.uFullScreenDisplayHeight 
			//&& device.modes[m].Format == dMode.Format 
			&& FormatToSize(device.modes[m].Format) == FormatToSize((D3DFORMAT)options.colorQuality) 
			)
		{
			return m;
		}
	}

	for( m=0; m<device.dwNumModes; m++ )
	{
		if( device.modes[m].Width==windowSetting.uFullScreenDisplayWidth && device.modes[m].Height==windowSetting.uFullScreenDisplayHeight )
		{
			return m;
		}
	}

	// Cannot find a matching mode
	TRACE0("Cannot find a matching mode");
	for( m=0; m<device.dwNumModes; m++ )
	{
		if( device.modes[m].Width==640 && device.modes[m].Height==480 )
		{
			return m;
		}
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Called during device initialization, this code checks the device
//       for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::ConfirmDevice( MYD3DCAPS* pCaps, uint32 dwBehavior,
										D3DFORMAT Format )
{
	
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: DisplayD3DErrorMsg()
// Desc: Displays error messages in a message box
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::DisplayD3DErrorMsg( HRESULT hr, uint32 dwType )
{
#ifdef _XBOX
	return S_OK;
#else
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
#endif
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
		
#ifndef _XBOX
		SAFE_RELEASE(m_pID3DFont);
		m_bFontIsCreated = false;
#endif
		SAFE_RELEASE(m_pd3dDevice);
		SAFE_RELEASE(m_pD3D);
    }
}


int CDXGraphicsContext::ToggleFullscreen()
{
#ifdef _XBOX
    return 1;
#else
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
		m_pd3dDevice->ShowCursor( FALSE );
	}
	else
	{
		m_pd3dDevice->ShowCursor( TRUE );
	}

	return m_bWindowed?0:1;
#endif
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
// Name: InitDeviceObjects()
// Desc: Initialize device-dependent objects. This is the place to create mesh
//       and texture objects.
//-----------------------------------------------------------------------------
HRESULT CDXGraphicsContext::InitDeviceObjects()
{
	// Set the device's states for rendering
	//return RDPInit();

	return S_OK;
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
#if DIRECTX_VERSION == 8
			char szError[200+1];
			D3DXGetErrorString(hr, szError, 200);
			TRACE1("D3D Error: %s", szError);
			ErrorMsg(szError);
#else
			const char *errmsg = DXGetErrorString(hr);
			TRACE1("D3D Error: %s", errmsg);
			//ErrorMsg(errmsg);
#endif
		}
		return false;
	}
	else
		return true;
}


void CDXGraphicsContext::SaveSurfaceToFile(char *filenametosave, MYLPDIRECT3DSURFACE surf, bool bShow)
{
#ifndef _XBOX
	char filename[256];
	strcpy(filename, filenametosave);

	D3DSURFACE_DESC desc;
	surf->GetDesc(&desc);
	CDirectXTexture *dsttxtr = new CDirectXTexture(desc.Width, desc.Height, AS_NORMAL);

	MYLPDIRECT3DSURFACE pDst;
	(MYLPDIRECT3DTEXTURE(dsttxtr->GetTexture()))->GetSurfaceLevel(0,&pDst);

	if( pDst )
	{
		HRESULT res;
		RECT dstrect = {0,0,desc.Width-1, desc.Height-1};
		res = D3DXLoadSurfaceFromSurface(pDst, NULL, &dstrect, surf, NULL, &dstrect, D3DX_FILTER_POINT, 0xFF000000);
		pDst->Release();

		if( res == S_OK )
		{
			CRender::g_pRender->SaveTextureToFile(*dsttxtr, filenametosave, TXT_RGB, bShow, false);
		}
	}

	delete dsttxtr;
#endif
}


bool CDXGraphicsContext::CreateFontObjects()
{
#ifndef _XBOX
#if DIRECTX_VERSION == 8
	m_hFont = (HFONT)GetStockObject(SYSTEM_FONT);
	D3DXCreateFont(m_pd3dDevice, m_hFont, &m_pID3DFont);
	m_bFontIsCreated = true;
#endif
#endif
	return true;
}

#include <WinGDI.h>
bool CDXGraphicsContext::DrawText(char *str, RECT &rect, int alignment)
{
#ifndef _XBOX
	if( !m_bFontIsCreated )
		CreateFontObjects();

	if( m_hFont && m_pID3DFont )
	{
		CRender::GetRender()->m_pColorCombiner->DisableCombiner();
#if DIRECTX_VERSION == 8
		m_pID3DFont->Begin();
		if( alignment == 0 )
		{
			m_pID3DFont->DrawText(str, strlen(str), &rect, 
				DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE,
				options.FPSColor);
		}
		else if( alignment == 1)
		{
			// Align to left
			m_pID3DFont->DrawText(str, strlen(str), &rect, 
				DT_LEFT | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE,
				options.FPSColor);
		}
		else
		{
			// Align to left
			m_pID3DFont->DrawText(str, strlen(str), &rect, 
				DT_RIGHT | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE,
				options.FPSColor);
		}
		m_pID3DFont->End();
#else
		if( alignment == 0 )
		{
			m_pID3DFont->DrawText(NULL, str, strlen(str), &rect, 
				DT_CENTER | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE,
				options.FPSColor);
		}
		else if( alignment == 1)
		{
			// Align to left
			m_pID3DFont->DrawText(NULL, str, strlen(str), &rect, 
				DT_LEFT | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE,
				options.FPSColor);
		}
		else
		{
			// Align to left
			m_pID3DFont->DrawText(NULL, str, strlen(str), &rect, 
				DT_RIGHT | DT_VCENTER | DT_NOCLIP | DT_NOPREFIX | DT_SINGLELINE,
				options.FPSColor);
		}
#endif
	}
#endif

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
#ifdef _XBOX
	if( m_pD3DDev != NULL && Stage < 4 )
#else
	if( m_pD3DDev != NULL )
#endif
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
#if DIRECTX_VERSION == 8
HRESULT CD3DDevWrapper::SetPixelShader(DWORD Handle)
{
	if( m_pD3DDev != NULL )
	{
		//if( m_savedPixelShader != Handle )
		{
			m_savedPixelShader = Handle;
			return m_pD3DDev->SetPixelShader(Handle);
		}
	}

	return S_OK;
}
#else
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
#endif

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
			
#if DIRECTX_VERSION == 8
			return m_pD3DDev->SetPixelShaderConstant(Register,pfdata,1);
#else
			return m_pD3DDev->SetPixelShaderConstantF(Register,pfdata,1);
#endif
		}
	}

	return S_OK;
}
HRESULT CD3DDevWrapper::SetViewport(MYD3DVIEWPORT* pViewport)
{
	if( m_pD3DDev != NULL )
	{
		if( pViewport->Width <= 0 )	pViewport->Width = 1;
		if( pViewport->Height <= 0 )	pViewport->Height = 1;

		m_savedViewport.X		= pViewport->X;
		m_savedViewport.Y		= pViewport->Y;
		m_savedViewport.Width	= pViewport->Width;
		m_savedViewport.Height	= pViewport->Height;
		m_savedViewport.MinZ	= pViewport->MinZ;
		m_savedViewport.MaxZ	= pViewport->MaxZ;
	
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
HRESULT CD3DDevWrapper::SetTexture(DWORD Stage,MYIDirect3DBaseTexture* pTexture)
{
#ifdef _XBOX
	if( m_pD3DDev != NULL && Stage < 4 )
#else
	if( m_pD3DDev != NULL )
#endif
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
#if DIRECTX_VERSION == 8 
			return m_pD3DDev->SetVertexShader(FVF);
#else
			return m_pD3DDev->SetFVF(FVF);
#endif
		}
	}

	return S_OK;
}

void CD3DDevWrapper::SetD3DDev(MYLPDIRECT3DDEVICE pD3DDev)
{
	m_pD3DDev = pD3DDev;
	Initalize();
}

void CD3DDevWrapper::Initalize(void)
{
#if DIRECTX_VERSION == 8
	m_savedPixelShader = ~0;
#else
	m_savedPixelShader = NULL;
#endif
	m_savedFVF = ~0;
	memset(&m_savedRenderStates, 0xEE, sizeof(m_savedRenderStates));
	memset(&m_savedPixelShaderConstants, 0xEE, sizeof(m_savedPixelShaderConstants));
	memset(&m_savedViewport, 0xEE, sizeof(m_savedViewport));
	memset(&m_savedTextureStageStates, 0xEE, sizeof(m_savedTextureStageStates));
	memset(&m_savedTexturePointers, 0xEE, sizeof(m_savedTexturePointers));

	for( int i=0; i<8; i++ )
	{
		m_savedTextureStageStates[i][D3DTSS_TEXCOORDINDEX] = ~0;
	}
}
