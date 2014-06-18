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

uint32 g_dwRamSize = 0x400000;
uint32* g_pu32RamBase = NULL;
signed char *g_ps8RamBase = NULL;
unsigned char *g_pu8RamBase = NULL;

CCritSect g_CritialSection;

//=======================================================
// User Options
RECT frameWriteByCPURect;
std::vector<RECT> frameWriteByCPURects;
RECT frameWriteByCPURectArray[20][20];
bool frameWriteByCPURectFlag[20][20];
std::vector<uint32> frameWriteRecord;

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
void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, uint32 dwTime)
{
	if( windowSetting.lastSecDlistCount != 0xFFFFFFFF )
	{
		if( windowSetting.dps < 0 )
			windowSetting.dps = (float)(status.gDlistCount-windowSetting.lastSecDlistCount);
		else
			windowSetting.dps = windowSetting.dps * 0.2f + (status.gDlistCount-windowSetting.lastSecDlistCount)*0.8f;
	}
	windowSetting.lastSecDlistCount = status.gDlistCount;

	if( windowSetting.lastSecFrameCount != 0xFFFFFFFF )
	{
		if( windowSetting.fps < 0 )
			windowSetting.fps = (float)(status.gFrameCount-windowSetting.lastSecFrameCount);
		else
			windowSetting.fps = windowSetting.fps * 0.2f + (status.gFrameCount-windowSetting.lastSecFrameCount)*0.8f;
	}
	windowSetting.lastSecFrameCount = status.gFrameCount;
}

extern void InitExternalTextures(void);
bool StartVideo(void)
{
	SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &windowSetting.screenSaverStatus, 0);
	if( windowSetting.screenSaverStatus )	
		SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, 0, 0);		// Disable screen saver

	windowSetting.timer = SetTimer(g_GraphicsInfo.hWnd, 100, 1000, (TIMERPROC)TimerProc);
	windowSetting.dps = windowSetting.fps = -1;
	windowSetting.lastSecDlistCount = windowSetting.lastSecFrameCount = 0xFFFFFFFF;

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
	
	//Now we have determined the tv system of the rom, lets set the frame ratio's
	if( status.dwTvSystem == TV_SYSTEM_NTSC )
		status.fRatio = 0.75f;
	else
		status.fRatio = 9/11.0f;
	
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
	windowSetting.dps = windowSetting.fps = -1;
	windowSetting.lastSecDlistCount = windowSetting.lastSecFrameCount = 0xFFFFFFFF;
	status.gDlistCount = status.gFrameCount = 0;

	KillTimer(g_GraphicsInfo.hWnd, windowSetting.timer);

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

	__try{
		uint32 dummy;
		dummy = g_GraphicsInfo.RDRAM[0x400000];
		dummy = g_GraphicsInfo.RDRAM[0x500000];
		dummy = g_GraphicsInfo.RDRAM[0x600000];
		dummy = g_GraphicsInfo.RDRAM[0x700000];
		dummy = g_GraphicsInfo.RDRAM[0x7FFFFC];
		g_dwRamSize = 0x800000;
	}
	__except(NULL, EXCEPTION_EXECUTE_HANDLER)
	{
		g_dwRamSize = 0x400000;
	}
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
		windowSetting.fViHeight = windowSetting.uViHeight = 
			g_curRomInfo.UseCIWidthAndRatio == USE_CI_WIDTH_AND_RATIO_FOR_NTSC ? g_CI.dwWidth/4*3 : g_CI.dwWidth/11*9;
	}
	else
	{
		float xscale, yscale;
		uint32 val = *g_GraphicsInfo.VI_X_SCALE_REG & 0xFFF;
		xscale = (float)val / (1<<10);
		uint32 start = *g_GraphicsInfo.VI_H_START_REG >> 16;
		uint32 end = *g_GraphicsInfo.VI_H_START_REG&0xFFFF;
		uint32 width = *g_GraphicsInfo.VI_WIDTH_REG;
		windowSetting.fViWidth = (end-start)*xscale;
		if( abs(windowSetting.fViWidth - width ) < 8 ) 
		{
			windowSetting.fViWidth = (float)width;
		}
		else
		{
			//DebuggerAppendMsg("fViWidth = %f, Width Reg=%d", windowSetting.fViWidth, width);
		}

		val = (*g_GraphicsInfo.VI_Y_SCALE_REG & 0xFFF);// - ((*g_GraphicsInfo.VI_Y_SCALE_REG>>16) & 0xFFF);
		if( val == 0x3FF )	val = 0x400;
		yscale = (float)val / (1<<10);
		start = *g_GraphicsInfo.VI_V_START_REG >> 16;
		end = *g_GraphicsInfo.VI_V_START_REG&0xFFFF;
		windowSetting.fViHeight = (end-start)/2*yscale;

		if( yscale == 0 )
		{
			windowSetting.fViHeight = windowSetting.fViWidth*status.fRatio;
		}
		else
		{
			if( *g_GraphicsInfo.VI_WIDTH_REG > 0x300 ) 
				windowSetting.fViHeight *= 2;

			if( windowSetting.fViWidth*status.fRatio > windowSetting.fViHeight && (*g_GraphicsInfo.VI_X_SCALE_REG & 0xFF) != 0 )
			{
				if( abs(int(windowSetting.fViWidth*status.fRatio - windowSetting.fViHeight)) < 8 )
				{
					windowSetting.fViHeight = windowSetting.fViWidth*status.fRatio;
				}
			}
			
			if( windowSetting.fViHeight<100 || windowSetting.fViWidth<100 )
			{
				//At sometime, value in VI_H_START_REG or VI_V_START_REG are 0
				windowSetting.fViWidth = (float)*g_GraphicsInfo.VI_WIDTH_REG;
				windowSetting.fViHeight = windowSetting.fViWidth*status.fRatio;
			}
		}

		windowSetting.uViWidth = (unsigned short)(windowSetting.fViWidth/4);
		windowSetting.fViWidth = windowSetting.uViWidth *= 4;

		windowSetting.uViHeight = (unsigned short)(windowSetting.fViHeight/4);
		windowSetting.fViHeight = windowSetting.uViHeight *= 4;
		uint16 optimizeHeight = uint16(windowSetting.uViWidth*status.fRatio);
		optimizeHeight &= ~3;

		uint16 optimizeHeight2 = uint16(windowSetting.uViWidth*3/4);
		optimizeHeight2 &= ~3;

		if( windowSetting.uViHeight != optimizeHeight && windowSetting.uViHeight != optimizeHeight2 )
		{
			if( abs(windowSetting.uViHeight-optimizeHeight) <= 8 )
				windowSetting.fViHeight = windowSetting.uViHeight = optimizeHeight;
			else if( abs(windowSetting.uViHeight-optimizeHeight2) <= 8 )
				windowSetting.fViHeight = windowSetting.uViHeight = optimizeHeight2;
		}


		if( gRDP.scissor.left == 0 && gRDP.scissor.top == 0 && gRDP.scissor.right != 0 )
		{
			if( (*g_GraphicsInfo.VI_X_SCALE_REG & 0xFF) != 0x0 && gRDP.scissor.right == windowSetting.uViWidth )
			{
				// Mario Tennis
				if( abs(int( windowSetting.fViHeight - gRDP.scissor.bottom )) < 8 )
				{
					windowSetting.fViHeight = windowSetting.uViHeight = gRDP.scissor.bottom;
				}
				else if( windowSetting.fViHeight < gRDP.scissor.bottom )
				{
					windowSetting.fViHeight = windowSetting.uViHeight = gRDP.scissor.bottom;
				}
				windowSetting.fViHeight = windowSetting.uViHeight = gRDP.scissor.bottom;
			}
			else if( gRDP.scissor.right == windowSetting.uViWidth - 1 && gRDP.scissor.bottom != 0 )
			{
				if( windowSetting.uViHeight != optimizeHeight && windowSetting.uViHeight != optimizeHeight2 )
				{
					if( status.fRatio != 0.75 && windowSetting.fViHeight > optimizeHeight/2 )
					{
						windowSetting.fViHeight = windowSetting.uViHeight = gRDP.scissor.bottom + gRDP.scissor.top + 1;
					}
				}
			}
			else if( gRDP.scissor.right == windowSetting.uViWidth && gRDP.scissor.bottom != 0  && status.fRatio != 0.75 )
			{
				if( windowSetting.uViHeight != optimizeHeight && windowSetting.uViHeight != optimizeHeight2 )
				{
					if( status.fRatio != 0.75 && windowSetting.fViHeight > optimizeHeight/2 )
					{
						windowSetting.fViHeight = windowSetting.uViHeight = gRDP.scissor.bottom + gRDP.scissor.top + 1;
					}
				}
			}
		}
	}
	SetScreenMult(windowSetting.uDisplayWidth/windowSetting.fViWidth, windowSetting.uDisplayHeight/windowSetting.fViHeight);
}

//---------------------------------------------------------------------------------------
FUNC_TYPE(void) NAME_DEFINE(UpdateScreen) (void)
{
	status.bVIOriginIsUpdated = false;
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

	if( status.toCaptureScreen )
	{
		status.toCaptureScreen = false;
		// Capture screen here
		CRender::g_pRender->CaptureScreen(status.screenCaptureFilename);
	}

	TXTRBUF_DETAIL_DUMP(TRACE1("VI ORIG is updated to %08X", *g_GraphicsInfo.VI_ORIGIN_REG));

	if( currentRomOptions.screenUpdateSetting == SCREEN_UPDATE_AT_VI_UPDATE )
	{
		CGraphicsContext::Get()->UpdateFrame();

		DEBUGGER_IF_DUMP( pauseAtNext, TRACE1("Update Screen: VIORIG=%08X", *g_GraphicsInfo.VI_ORIGIN_REG));
		DEBUGGER_PAUSE_COUNT_N_WITHOUT_UPDATE(NEXT_FRAME);
		DEBUGGER_PAUSE_COUNT_N_WITHOUT_UPDATE(NEXT_SET_CIMG);
		g_CritialSection.Unlock();
		return;
	}

	TXTRBUF_DETAIL_DUMP(TRACE1("VI ORIG is updated to %08X", *g_GraphicsInfo.VI_ORIGIN_REG));

	if( currentRomOptions.screenUpdateSetting == SCREEN_UPDATE_AT_VI_UPDATE_AND_DRAWN )
	{
		if( status.bScreenIsDrawn )
		{
			CGraphicsContext::Get()->UpdateFrame();
			DEBUGGER_IF_DUMP( pauseAtNext, TRACE1("Update Screen: VIORIG=%08X", *g_GraphicsInfo.VI_ORIGIN_REG));
		}
		else
		{
			DEBUGGER_IF_DUMP( pauseAtNext, TRACE1("Skip Screen Update: VIORIG=%08X", *g_GraphicsInfo.VI_ORIGIN_REG));
		}

		DEBUGGER_PAUSE_COUNT_N_WITHOUT_UPDATE(NEXT_FRAME);
		DEBUGGER_PAUSE_COUNT_N_WITHOUT_UPDATE(NEXT_SET_CIMG);
		g_CritialSection.Unlock();
		return;
	}

	if( currentRomOptions.screenUpdateSetting==SCREEN_UPDATE_AT_VI_CHANGE )
	{

		if( *g_GraphicsInfo.VI_ORIGIN_REG != status.curVIOriginReg )
		{
			if( *g_GraphicsInfo.VI_ORIGIN_REG < status.curDisplayBuffer || *g_GraphicsInfo.VI_ORIGIN_REG > status.curDisplayBuffer+0x2000  )
			{
				status.curDisplayBuffer = *g_GraphicsInfo.VI_ORIGIN_REG;
				status.curVIOriginReg = status.curDisplayBuffer;
				//status.curRenderBuffer = NULL;

				CGraphicsContext::Get()->UpdateFrame();
				DEBUGGER_IF_DUMP( pauseAtNext, TRACE1("Update Screen: VIORIG=%08X", *g_GraphicsInfo.VI_ORIGIN_REG));
				DEBUGGER_PAUSE_COUNT_N_WITHOUT_UPDATE(NEXT_FRAME);
				DEBUGGER_PAUSE_COUNT_N_WITHOUT_UPDATE(NEXT_SET_CIMG);
			}
			else
			{
				status.curDisplayBuffer = *g_GraphicsInfo.VI_ORIGIN_REG;
				status.curVIOriginReg = status.curDisplayBuffer;
				DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_FRAME, {DebuggerAppendMsg("Skip Screen Update, closed to the display buffer, VIORIG=%08X", *g_GraphicsInfo.VI_ORIGIN_REG);});
			}
		}
		else
		{
			DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_FRAME, {DebuggerAppendMsg("Skip Screen Update, the same VIORIG=%08X", *g_GraphicsInfo.VI_ORIGIN_REG);});
		}

		g_CritialSection.Unlock();
		return;
	}

	if( currentRomOptions.screenUpdateSetting >= SCREEN_UPDATE_AT_1ST_CI_CHANGE )
	{
		status.bVIOriginIsUpdated=true;
		DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_FRAME, {DebuggerAppendMsg("VI ORIG is updated to %08X", *g_GraphicsInfo.VI_ORIGIN_REG);});
		g_CritialSection.Unlock();
		return;
	}

	DEBUGGER_IF_DUMP( pauseAtNext, TRACE1("VI is updated, No screen update: VIORIG=%08X", *g_GraphicsInfo.VI_ORIGIN_REG));
	DEBUGGER_PAUSE_COUNT_N_WITHOUT_UPDATE(NEXT_FRAME);
	DEBUGGER_PAUSE_COUNT_N_WITHOUT_UPDATE(NEXT_SET_CIMG);

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
		TriggerDPInterrupt();
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
		DLParser_Process((OSTask *)(g_GraphicsInfo.DMEM + 0x0FC0));
	}
	catch (...)
	{
		TRACE0("Unknown Error in ProcessDList");
		TriggerDPInterrupt();
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