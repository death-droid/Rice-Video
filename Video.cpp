/*
Copyright (C) 2002-2009 Rice1964

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
#include "_BldNum.h"

PluginStatus status;
char generalText[256];

GFX_INFO g_GraphicsInfo;

uint32 g_dwRamSize = 0x800000;
uint32* g_pu32RamBase = NULL;
signed char *g_ps8RamBase = NULL;
unsigned char *g_pu8RamBase = NULL;

CCritSect g_CritialSection;
//---------------------------------------------------------------------------------------

BOOL APIENTRY DllMain(HINSTANCE hinstDLL,  // DLL module handle
                      uint32 fdwReason,              // reason called
                      LPVOID lpvReserved)           // reserved
{ 
	windowSetting.myhInst = hinstDLL;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH: 
		//Once we have been attached to a process, init our configuration
		InitConfiguration();
		break;
	case DLL_THREAD_ATTACH: 
		break; 
	case DLL_THREAD_DETACH: 
		break; 
	case DLL_PROCESS_DETACH:
		//Process has been detached
		break; 
	} 
	return TRUE; 
} 

void GetPluginDir( char * Directory ) 
{
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	GetModuleFileName(windowSetting.myhInst,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );
	strcpy(Directory,drive);
	strcat(Directory,dir);
	if(Directory[strlen(Directory) - 1] != '\\') 
		strcat(Directory, "\\");
}

//-------------------------------------------------------------------------------------
FUNC_TYPE(void) NAME_DEFINE(GetDllInfo) ( PLUGIN_INFO * PluginInfo )
{
	sprintf(PluginInfo->Name, "%s %s %s",project_name, BUILD_NUMBER, FILE_VERSION);

	PluginInfo->Version        = 0x0103;
	PluginInfo->Type           = PLUGIN_TYPE_GFX;
	PluginInfo->NormalMemory   = FALSE;
	PluginInfo->MemoryBswaped  = TRUE;
}

//---------------------------------------------------------------------------------------

FUNC_TYPE(void) NAME_DEFINE(DllAbout) ( HWND hParent )
{
	char temp[300];
	sprintf(temp, "%s %s (%s)\nDirectX 9\nBased on Rice's 6.1.1 beta 10 source code", project_name, FILE_VERSION, BUILD_NUMBER) ;
	MsgInfo(temp);
}


//---------------------------------------------------------------------------------------
FUNC_TYPE(void) NAME_DEFINE(DllConfig) ( HWND hParent )
{
	CreateOptionsDialogs(hParent);
}


void ChangeWinSize( void ) 
{
	WINDOWPLACEMENT wndpl;
    RECT rc1, swrect;
	
    wndpl.length = sizeof(wndpl);
	GetWindowPlacement( g_GraphicsInfo.hWnd, &wndpl);

	if ( g_GraphicsInfo.hStatusBar != NULL ) 
	{
		GetClientRect( g_GraphicsInfo.hStatusBar, &swrect );
	    SetRect( &rc1, 0, 0, windowSetting.uDisplayWidth, (windowSetting.uDisplayWidth >> 2) * 3 + swrect.bottom );
	} 
	else 
	{
	    SetRect( &rc1, 0, 0, windowSetting.uDisplayWidth, (windowSetting.uDisplayWidth >> 2) * 3 );
	}

    AdjustWindowRectEx( &rc1,GetWindowLong( g_GraphicsInfo.hWnd, GWL_STYLE ), GetMenu( g_GraphicsInfo.hWnd ) != NULL, GetWindowLong( g_GraphicsInfo.hWnd, GWL_EXSTYLE ) ); 
    MoveWindow( g_GraphicsInfo.hWnd, wndpl.rcNormalPosition.left, wndpl.rcNormalPosition.top, rc1.right - rc1.left, rc1.bottom - rc1.top, TRUE );
	Sleep(100);
}
//---------------------------------------------------------------------------------------

FUNC_TYPE(void) NAME_DEFINE(ChangeWindow) ()
{
	
	if( status.ToToggleFullScreen )
		status.ToToggleFullScreen = FALSE;
	else
		status.ToToggleFullScreen = TRUE;

	status.bDisableFPS = true;
	windowSetting.bDisplayFullscreen = !windowSetting.bDisplayFullscreen;
	g_CritialSection.Lock();
	windowSetting.bDisplayFullscreen = CGraphicsContext::Get()->ToggleFullscreen();

	if( g_GraphicsInfo.hStatusBar != NULL )
		ShowWindow(g_GraphicsInfo.hStatusBar, windowSetting.bDisplayFullscreen ? SW_HIDE : SW_SHOW);

	ShowCursor(windowSetting.bDisplayFullscreen ? FALSE : TRUE);

	CGraphicsContext::Get()->Clear(CLEAR_COLOR_AND_DEPTH_BUFFER);
	CGraphicsContext::Get()->UpdateFrame();
	g_CritialSection.Unlock();
	status.bDisableFPS = false;
	status.ToToggleFullScreen = FALSE;
}

//---------------------------------------------------------------------------------------

FUNC_TYPE(void) NAME_DEFINE(DrawScreen) (void)
{
}

//---------------------------------------------------------------------------------------

FUNC_TYPE(void) NAME_DEFINE(MoveScreen) (int xpos, int ypos)
{ 
}

void Ini_GetRomOptions(LPGAMESETTING pGameSetting);
void Ini_StoreRomOptions(LPGAMESETTING pGameSetting);
void GenerateCurrentRomOptions();

extern void InitExternalTextures(void);
bool StartVideo(void)
{
	SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &windowSetting.screenSaverStatus, 0);
	if( windowSetting.screenSaverStatus )	
		SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, 0, 0);		// Disable screen saver

	g_CritialSection.Lock();

	memcpy(&g_curRomInfo.romheader, g_GraphicsInfo.HEADER, sizeof(ROMHeader));
	ROM_ByteSwap_3210( &g_curRomInfo.romheader, sizeof(ROMHeader) );
	ROM_GetRomNameFromHeader(g_curRomInfo.szGameName, &g_curRomInfo.romheader);
	Ini_GetRomOptions(&g_curRomInfo);

	char *p = g_curRomInfo.szGameName + (lstrlen(g_curRomInfo.szGameName) -1);// -1 to skip null
	while (p >= g_curRomInfo.szGameName)
	{
		if( *p == ':' || *p == '\\' || *p == '/' )
			*p = '-';
		p--;
	}

	//Grab any rom options that are set only for this rim
	GenerateCurrentRomOptions();

	//Lets figure out what the tv system is
	status.dwTvSystem = CountryCodeToTVSystem(g_curRomInfo.romheader.nCountryID);

	//Grab any external textures.
	InitExternalTextures();
	
	//Change the window size to our required one.
	ChangeWinSize();
		
	try {
		CGraphicsContext::g_pGraphicsContext = new CDXGraphicsContext();
		g_pFrameBufferManager = new DXFrameBufferManager;
		CGraphicsContext::InitWindowInfo();
		
		windowSetting.bDisplayFullscreen = FALSE;
		bool res = CGraphicsContext::Get()->Initialize(g_GraphicsInfo.hWnd, g_GraphicsInfo.hStatusBar, 640, 480, !windowSetting.bDisplayFullscreen);
		
		if(!res)
		{
			g_CritialSection.Unlock();
			return false;
		}

		CRender::g_pRender = new D3DRender();
		CRender::GetRender()->Initialize();
		
		DLParser_Init();
		
		status.bGameIsRunning = true;
	}
	catch(...)
	{
		ErrorMsg("Error to start video");
		throw 0;
	}

	g_CritialSection.Unlock();
	return true;
}

extern void CloseExternalTextures(void);
void StopVideo()
{
	if( CGraphicsContext::Get()->IsWindowed() == false )
	{
		status.ToToggleFullScreen = TRUE;
		CGraphicsContext::Get()->ToggleFullscreen();
		status.ToToggleFullScreen = FALSE;
	}

	g_CritialSection.Lock();
	status.bGameIsRunning = false;


	try {
		// Kill all textures?
		gTextureManager.RecycleAllTextures();
		gTextureManager.CleanUp();
		RDP_Cleanup();

		CloseExternalTextures();

		SAFE_DELETE(CRender::g_pRender);
		CRender::g_pRender = NULL;

		CGraphicsContext::Get()->CleanUp();
		SAFE_DELETE(CGraphicsContext::g_pGraphicsContext);
		SAFE_DELETE(g_pFrameBufferManager);
	}
	catch(...)
	{
		TRACE0("Some exceptions during RomClosed");
	}

	g_CritialSection.Unlock();
	status.gDlistCount = 0;

	if( windowSetting.screenSaverStatus )	
		SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, 0, 0);	// Enable screen saver

	DEBUGGER_ONLY({delete surfTlut;});
}

//---------------------------------------------------------------------------------------
FUNC_TYPE(void) NAME_DEFINE(RomClosed) (void)
{
	TRACE0("To stop video");
	Ini_StoreRomOptions(&g_curRomInfo);

	StopVideo();

	TRACE0("Video is stopped");
}

FUNC_TYPE(bool) NAME_DEFINE(RomOpen) (void)
{
	if( g_CritialSection.IsLocked() )
	{
		g_CritialSection.Unlock();
		TRACE0("g_CritialSection is locked when game is starting, unlock it now.");
	}

	status.bDisableFPS=false;

#ifdef _DEBUG
	if( debuggerPause )
	{
		debuggerPause = FALSE;
		Sleep(100);
	}
#endif
	if (!StartVideo())
		return 0;

	return 1;
}

void SetVIScales()
{
	if( g_curRomInfo.VIHeight>0 && g_curRomInfo.VIWidth>0 )
	{
		windowSetting.fViWidth = windowSetting.uViWidth = g_curRomInfo.VIWidth;
		windowSetting.fViHeight = windowSetting.uViHeight = g_curRomInfo.VIHeight;
	}
	else if( g_curRomInfo.UseCIWidthAndRatio && g_CI.dwWidth )
	{
		windowSetting.fViWidth = windowSetting.uViWidth = g_CI.dwWidth;
		windowSetting.fViHeight = windowSetting.uViHeight = g_curRomInfo.UseCIWidthAndRatio == USE_CI_WIDTH_AND_RATIO_FOR_NTSC ? g_CI.dwWidth/4*3 : g_CI.dwWidth/11*9;
	}
	else
	{
		uint32 width = *g_GraphicsInfo.VI_WIDTH_REG;

		uint32 ScaleX = *g_GraphicsInfo.VI_X_SCALE_REG & 0xFFF;
		uint32 ScaleY = *g_GraphicsInfo.VI_Y_SCALE_REG & 0xFFF;

		float fScaleX = (float)ScaleX / 1024.0f;
		float fScaleY = (float)ScaleY / 2048.0f;

		uint32 HStartReg = *g_GraphicsInfo.VI_H_START_REG;
		uint32 VStartReg = *g_GraphicsInfo.VI_V_START_REG;

		uint32 hstart = HStartReg >> 16;
		uint32 hend   = HStartReg & 0xFFFF;

		uint32 vstart = VStartReg >> 16;
		uint32 vend   = VStartReg & 0xFFFF;

		if (hend == hstart)
		{
			hend = (uint32)(width / fScaleX);
		}

		windowSetting.fViWidth  = (hend - hstart) * fScaleX;
		windowSetting.fViHeight = (vend - vstart) * fScaleY;

		if (width > 0x300)
			windowSetting.fViHeight *= 2.0f;

		windowSetting.uViWidth = (unsigned short)(windowSetting.fViWidth / 4);
		windowSetting.fViWidth = windowSetting.uViWidth *= 4;

		windowSetting.uViHeight = (unsigned short)(windowSetting.fViHeight / 4);
		windowSetting.fViHeight = windowSetting.uViHeight *= 4;
	}
	SetScreenMult(windowSetting.uDisplayWidth/windowSetting.fViWidth, windowSetting.uDisplayHeight/windowSetting.fViHeight);
}

//---------------------------------------------------------------------------------------
FUNC_TYPE(void) NAME_DEFINE(UpdateScreen) (void)
{
	g_CritialSection.Lock();

	if( status.bHandleN64RenderTexture )
		g_pFrameBufferManager->CloseRenderTexture(true);
	
	g_pFrameBufferManager->SetAddrBeDisplayed(*g_GraphicsInfo.VI_ORIGIN_REG);

	if( status.gDlistCount == 0 )
	{
		// CPU frame buffer update
		uint32 width = *g_GraphicsInfo.VI_WIDTH_REG;
		if( (*g_GraphicsInfo.VI_ORIGIN_REG & (g_dwRamSize-1) ) > width*2 && *g_GraphicsInfo.VI_H_START_REG != 0 && width != 0 )
		{
			SetVIScales();
			CRender::GetRender()->DrawFrameBuffer(true);
			CGraphicsContext::Get()->UpdateFrame();
		}
		g_CritialSection.Unlock();
		return;
	}

	TXTRBUF_DETAIL_DUMP(TRACE1("VI ORIG is updated to %08X", *g_GraphicsInfo.VI_ORIGIN_REG));

	if( *g_GraphicsInfo.VI_ORIGIN_REG != status.curVIOriginReg )
	{

		status.curVIOriginReg = *g_GraphicsInfo.VI_ORIGIN_REG;

		if (status.toCaptureScreen)
		{
			status.toCaptureScreen = false;
			// Capture screen here
			CRender::g_pRender->CaptureScreen(status.screenCaptureFilename);
		}

		CGraphicsContext::Get()->UpdateFrame();
		DEBUGGER_IF_DUMP( pauseAtNext, TRACE1("Update Screen: VIORIG=%08X", *g_GraphicsInfo.VI_ORIGIN_REG));
	}


	g_CritialSection.Unlock();
}

//---------------------------------------------------------------------------------------

FUNC_TYPE(void) NAME_DEFINE(ViStatusChanged) (void)
{
	g_CritialSection.Lock();
	SetVIScales();
	CRender::g_pRender->UpdateClipRectangle();
	g_CritialSection.Unlock();
}

//---------------------------------------------------------------------------------------
FUNC_TYPE(void) NAME_DEFINE(ViWidthChanged) (void)
{
	g_CritialSection.Lock();
	SetVIScales();
	CRender::g_pRender->UpdateClipRectangle();
	g_CritialSection.Unlock();
}

FUNC_TYPE(BOOL) NAME_DEFINE(InitiateGFX)(GFX_INFO Gfx_Info)
{
#ifdef _DEBUG
	OpenDialogBox();
#endif

	memset(&status, 0, sizeof(status));
	memcpy(&g_GraphicsInfo, &Gfx_Info, sizeof(GFX_INFO));
	
	g_pu8RamBase			= Gfx_Info.RDRAM;
	g_pu32RamBase			= (uint32*)Gfx_Info.RDRAM;
	g_ps8RamBase			= (signed char *)Gfx_Info.RDRAM;
	
	windowSetting.fViWidth = 320;
	windowSetting.fViHeight = 240;
	status.ToToggleFullScreen = FALSE;
	status.bDisableFPS=false;

	InitConfiguration();
	CGraphicsContext::InitWindowInfo();
	CGraphicsContext::InitDeviceParameters();

	return(TRUE);
}


void __cdecl MsgInfo (char * Message, ...)
{
	char Msg[400];
	va_list ap;

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );

	sprintf(generalText, "%s %s",project_name, BUILD_NUMBER);
	MessageBox(NULL,Msg,generalText,MB_OK|MB_ICONINFORMATION);
}

void __cdecl ErrorMsg (char * Message, ...)
{
	char Msg[400];
	va_list ap;
	
	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	
	sprintf(generalText, "%s %s",project_name, BUILD_NUMBER);
	if( status.ToToggleFullScreen || (CGraphicsContext::g_pGraphicsContext && !CGraphicsContext::g_pGraphicsContext->IsWindowed()) )
		SetWindowText(g_GraphicsInfo.hStatusBar,Msg);
	else
		MessageBox(NULL,Msg,generalText,MB_OK|MB_ICONERROR);
}

//---------------------------------------------------------------------------------------

FUNC_TYPE(void) NAME_DEFINE(CloseDLL) (void)
{ 
	if( status.bGameIsRunning )
	{
		RomClosed();
	}

#ifdef _DEBUG
	CloseDialogBox();
#endif
}

FUNC_TYPE(void) NAME_DEFINE(ProcessRDPList)(void)
{
	try
	{
		RDP_DLParser_Process();
	}
	catch (...)
	{
		TRACE0("Unknown Error in ProcessRDPList");
	}
}	

FUNC_TYPE(void) NAME_DEFINE(ProcessDList)(void)
{
	g_CritialSection.Lock();
	if( status.toShowCFB )
	{
		CRender::GetRender()->DrawFrameBuffer(true);
		status.toShowCFB = false;
	}

	try
	{
		DLParser_Process();
	}
	catch (...)
	{
		TRACE0("Unknown Error in ProcessDList");
	}

	g_CritialSection.Unlock();
}	

//---------------------------------------------------------------------------------------

void TriggerDPInterrupt(void)
{
	*(g_GraphicsInfo.MI_INTR_REG) |= 0x20;
	g_GraphicsInfo.CheckInterrupts();
}

// Plugin spec 1.3 functions
FUNC_TYPE(void) NAME_DEFINE(ShowCFB) (void)
{
	status.toShowCFB = true;
}

//REWRITE ME --FIX ME -- CLEAN ME
FUNC_TYPE(void) NAME_DEFINE(CaptureScreen) ( char * Directory )
{
	if( status.bGameIsRunning && status.gDlistCount > 0 )
	{
		if( !PathFileExists(Directory) && !CreateDirectory(Directory, NULL) )
		{

			//DisplayError("Can not create new folder: %s", pathname);
			return;
		}

		strcpy(status.screenCaptureFilename, Directory);
		if( Directory[strlen(Directory)-1] != '\\' && Directory[strlen(Directory)-1] != '/'  )
		{
			strcat(status.screenCaptureFilename,"\\");
		}
		
		strcat(status.screenCaptureFilename, g_curRomInfo.szGameName);

		char tempname[MAX_PATH];
		for( int i=0; ; i++)
		{
			sprintf(tempname, "%s-%d.bmp", status.screenCaptureFilename, i);
			if( !PathFileExists(tempname) )
			{
				sprintf(tempname, "%s-%d.png", status.screenCaptureFilename, i);
				if( !PathFileExists(tempname) )
				{
					sprintf(tempname, "%s-%d.png", status.screenCaptureFilename, i);
					break;
				}
			}
		}

		strcpy(status.screenCaptureFilename, tempname);
		status.toCaptureScreen = true;
	}
}