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
#include "_BldNum.h"
#include "SimpleIni.h"

#define INI_FILE		"RiceVideo.ini"
#define CONFIG_FILE     "RiceVideo.cfg"
char *project_name =	"RiceVideo";

// Disable the config dialog box to allow Vtune call graph feature to work
#define ENABLE_CONFIG_DIALOG

char *frameBufferSettings[] =
{
	"None (default)",
	"Hide Framebuffer Effects",
	"Basic Framebuffer",
	"Basic & Write Back",
	"Write Back & Reload",
	"Write Back Every Frame",
	"With Emulator",
	"Basic Framebuffer & With Emulator",
	"With Emulator Read Only",
	"With Emulator Write Only",
};

char *frameBufferWriteBackControlSettings[] =
{
	"Every Frame (default)",
	"Every 2 Frames",
	"Every 3 Frames",
	"Every 4 Frames",
	"Every 5 Frames",
	"Every 6 Frames",
	"Every 7 Frames",
	"Every 8 Frames",
};

char *renderToTextureSettings[] =
{
	"None (default)",
	"Hide Render-to-texture Effects",
	"Basic Render-to-texture",
	"Basic & Write Back",
	"Write Back & Reload",
};

const char *screenUpdateSettings[] =
{
	"At VI origin update",
	"At VI origin change",
	"At CI change",
	"At the 1st CI change",
	"At the 1st drawing",
	"Before clear the screen",
	"At VI origin update after screen is drawn (default)",
};

WindowSettingStruct windowSetting;
GlobalOptions options;
RomOptions defaultRomOptions;
RomOptions currentRomOptions;
FrameBufferOptions frameBufferOptions;

char	szIniFileName[300];
char	szIniSettingsFileName[300];

CSimpleIniA ini;
CSimpleIniA perRomIni;

//=======================================================
const SettingInfo ForceTextureFilterSettings[] =
{
	"N64 Default Texture Filter",	FORCE_DEFAULT_FILTER,
	"Force Nearest Filter (faster, low quality)",	FORCE_POINT_FILTER,
	"Force Linear Filter (slower, better quality)",	FORCE_LINEAR_FILTER,
};

const SettingInfo TextureEnhancementSettings[] =
{
	"N64 original texture (No enhancement)",	TEXTURE_NO_ENHANCEMENT,
	"2xSaI", TEXTURE_2XSAI_ENHANCEMENT,
	"HQ2x", TEXTURE_HQ2X_ENHANCEMENT,
	"HQ2xS", TEXTURE_HQ2XS_ENHANCEMENT,
};

const char*	strDXDeviceDescs[] = { "HAL", "REF" };


/*
*	Constants
*/
const SettingInfo OnScreenDisplaySettings[] =
{
	"Display Nothing",							ONSCREEN_DISPLAY_NOTHING,
	"Display DList Per Second",					ONSCREEN_DISPLAY_DLIST_PER_SECOND,
	"Display Frame Per Second",					ONSCREEN_DISPLAY_FRAME_PER_SECOND,
	"Display Debug Information Only",			ONSCREEN_DISPLAY_DEBUG_INFORMATION_ONLY,
	"Display Messages From CPU Core Only",		ONSCREEN_DISPLAY_TEXT_FROM_CORE_ONLY,
	"Display DList Per Second With Core Msgs",	ONSCREEN_DISPLAY_DLIST_PER_SECOND_WITH_CORE_MSG,
	"Display Frame Per Second With Core Msgs",	ONSCREEN_DISPLAY_FRAME_PER_SECOND_WITH_CORE_MSG,
	"Display Debug Information With Core Msgs",	ONSCREEN_DISPLAY_DEBUG_INFORMATION_WITH_CORE_MSG,
};

void WriteConfiguration(void);
void GenerateCurrentRomOptions();

HWND	g_hwndTT=NULL;
HWND	g_hwndDlg=NULL;
HHOOK	g_hhk = NULL;

int DialogToStartRomIsRunning = PSH_ROM_SETTINGS;
int DialogToStartRomIsNotRunning = PSH_OPTIONS;
HWND hPropSheetHwnd = NULL;

extern "C" bool __stdcall EnumChildProc(HWND hwndCtrl, LPARAM lParam);
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
VOID OnWMNotify(LPARAM lParam);
bool CreateDialogTooltip(void);
bool EnumChildWndTooltip(void);

inline void ShowItem(HWND hDlg, UINT item, bool flag)
{
	HWND itemwnd = GetDlgItem(hDlg, item);
	ShowWindow(itemwnd,flag?SW_SHOW:SW_HIDE);
}

//////////////////////////////////////////////////////////////////////////
void GenerateFrameBufferOptions(void)
{
	frameBufferOptions.bUpdateCIInfo			= false;

	frameBufferOptions.bCheckBackBufs			= false;
	frameBufferOptions.bWriteBackBufToRDRAM		= false;
	frameBufferOptions.bLoadBackBufFromRDRAM	= false;

	frameBufferOptions.bIgnore					= true;

	frameBufferOptions.bSupportRenderTextures			= false;
	frameBufferOptions.bCheckRenderTextures			= false;
	frameBufferOptions.bRenderTextureWriteBack			= false;
	frameBufferOptions.bLoadRDRAMIntoRenderTexture		= false;

	frameBufferOptions.bProcessCPUWrite			= false;
	frameBufferOptions.bProcessCPURead			= false;
	frameBufferOptions.bAtEachFrameUpdate		= false;
	frameBufferOptions.bIgnoreRenderTextureIfHeightUnknown		= false;

	switch( currentRomOptions.N64FrameBufferEmuType )
	{
	case FRM_BUF_NONE:
		break;
	case FRM_BUF_COMPLETE:
		frameBufferOptions.bAtEachFrameUpdate		= true;
		frameBufferOptions.bProcessCPUWrite			= true;
		frameBufferOptions.bProcessCPURead			= true;
		frameBufferOptions.bUpdateCIInfo			= true;
		break;
	case FRM_BUF_WRITEBACK_AND_RELOAD:
		frameBufferOptions.bLoadBackBufFromRDRAM	= true;
	case FRM_BUF_BASIC_AND_WRITEBACK:
		frameBufferOptions.bWriteBackBufToRDRAM		= true;
	case FRM_BUF_BASIC:
		frameBufferOptions.bCheckBackBufs			= true;
	case FRM_BUF_IGNORE:
		frameBufferOptions.bUpdateCIInfo			= true;
		break;
	case FRM_BUF_BASIC_AND_WITH_EMULATOR:
		// Banjo Kazooie
		frameBufferOptions.bCheckBackBufs			= true;
	case FRM_BUF_WITH_EMULATOR:
		frameBufferOptions.bUpdateCIInfo			= true;
		frameBufferOptions.bProcessCPUWrite			= true;
		frameBufferOptions.bProcessCPURead			= true;
		break;
	case FRM_BUF_WITH_EMULATOR_READ_ONLY:
		frameBufferOptions.bUpdateCIInfo			= true;
		frameBufferOptions.bProcessCPURead			= true;
		break;
	case FRM_BUF_WITH_EMULATOR_WRITE_ONLY:
		frameBufferOptions.bUpdateCIInfo			= true;
		frameBufferOptions.bProcessCPUWrite			= true;
		break;
	}

	///comebacktome
	switch( currentRomOptions.N64RenderToTextureEmuType )
	{
	case TXT_BUF_NONE:
		frameBufferOptions.bSupportRenderTextures			= false;
		break;
	case TXT_BUF_WRITE_BACK_AND_RELOAD://checkme does nothing
		frameBufferOptions.bLoadRDRAMIntoRenderTexture		= true;
	case TXT_BUF_WRITE_BACK:
		frameBufferOptions.bRenderTextureWriteBack			= true;
	case TXT_BUF_NORMAL:
		frameBufferOptions.bCheckRenderTextures			= true;
		frameBufferOptions.bIgnore					= false;
	case TXT_BUF_IGNORE:
		frameBufferOptions.bUpdateCIInfo			= true;
		frameBufferOptions.bSupportRenderTextures			= true;
		break;
	}

	if( currentRomOptions.screenUpdateSetting >= SCREEN_UPDATE_AT_CI_CHANGE )
	{
		frameBufferOptions.bUpdateCIInfo = true;
	}
}
//////////////////////////////////////////////////////////////////////////

//========================================================================
extern void GetPluginDir( char * Directory );

void WriteConfiguration(void)
{
	char name[1024];
	GetPluginDir(name);
	strcat(name, CONFIG_FILE);
	FILE *f = fopen(name, "rb");
	if (!f)
	{
		windowSetting.uWindowDisplayWidth=640;
		windowSetting.uWindowDisplayHeight=480;
		windowSetting.uFullScreenDisplayWidth=640;
		windowSetting.uFullScreenDisplayHeight=480;
		windowSetting.uScreenScaleMode = 0;
	}
	else
		fclose(f);
   
	//Lets set window settings
	ini.SetLongValue("WindowSetting", "WindowedWidth", windowSetting.uWindowDisplayWidth);
	ini.SetLongValue("WindowSetting", "WindowedHeight", windowSetting.uWindowDisplayHeight);
	ini.SetLongValue("WindowSetting", "FullscreenWidth", windowSetting.uFullScreenDisplayWidth);
	ini.SetLongValue("WindowSetting", "FullscreenHeight", windowSetting.uFullScreenDisplayHeight);
	ini.SetLongValue("WindowSetting", "FullScreenFrequency", (uint32)windowSetting.uFullScreenRefreshRate);
	ini.SetLongValue("WindowSetting", "ScreenScaleMode", (uint32)windowSetting.uScreenScaleMode);

	//Now rendering modes
	ini.SetLongValue("RenderSetting", "DirectXDevice", (uint32)options.DirectXDevice);
	ini.SetLongValue("RenderSetting", "DirectXAntiAliasingValue", (uint32)options.DirectXAntiAliasingValue);
	ini.SetLongValue("RenderSetting", "DirectXMaxFSAA", (uint32)options.DirectXMaxFSAA);
	ini.SetLongValue("RenderSetting", "DirectXMaxAnisotropy", (uint32)options.DirectXMaxAnisotropy);
	ini.SetLongValue("RenderSetting", "DirectXAnisotropyValue", (uint32)options.DirectXAnisotropyValue);
	ini.SetLongValue("RenderSetting", "NormalAlphaBlender", defaultRomOptions.bNormalBlender);
	ini.SetLongValue("RenderSetting", "EnableFog", options.bEnableFog);
	ini.SetLongValue("RenderSetting", "WinFrameMode", options.bWinFrameMode);
	ini.SetLongValue("RenderSetting", "MipMaps", options.bMipMaps);
	ini.SetLongValue("RenderSetting", "EnableVertexShader", options.bEnableVertexShader);
	ini.SetLongValue("RenderSetting", "ForceSoftwareTnL", options.bForceSoftwareTnL);

	//Now texture settings
	ini.SetLongValue("Texture Settings", "CacheHiResTextures" , (uint32)options.bCacheHiResTextures);
	ini.SetLongValue("Texture Settings", "ForceTextureFilter", (uint32)options.forceTextureFilter);
	ini.SetLongValue("Texture Settings", "LoadHiResTextures", (uint32)options.bLoadHiResTextures);
	ini.SetLongValue("Texture Settings", "DumpTexturesToFiles", (uint32)options.bDumpTexturesToFiles);
	ini.SetLongValue("Texture Settings", "TextureEnhancement", (uint32)options.textureEnhancement);
	ini.SetLongValue("Texture Settings", "TextureEnhancementControl", (uint32)options.textureEnhancementControl);
	ini.SetLongValue("Texture Settings", "DoubleSizeForSmallTxtrBuf", (uint32)defaultRomOptions.bDoubleSizeForSmallTxtrBuf);

	//Now framebuffer Settings
	ini.SetLongValue("FrameBufferSettings", "FrameBufferType", defaultRomOptions.N64FrameBufferEmuType);
	ini.SetLongValue("FrameBufferSettings", "FrameBufferWriteBackControl", defaultRomOptions.N64FrameBufferWriteBackControl);
	ini.SetLongValue("FrameBufferSettings", "RenderToTexture", defaultRomOptions.N64RenderToTextureEmuType);
	
	//Now just some misc settings
	ini.SetLongValue("MiscSettings", "EnableHacks", options.bEnableHacks);
	ini.SetLongValue("MiscSettings", "ScreenUpdateSetting", defaultRomOptions.screenUpdateSetting);
	ini.SetLongValue("MiscSettings", "FPSColor", options.FPSColor);
	ini.SetLongValue("MiscSettings","EnableSSE", options.bEnableSSE);
	ini.SetLongValue("MiscSettings","DisplayTooltip", options.bDisplayTooltip);
	ini.SetLongValue("MiscSettings","HideAdvancedOptions", options.bHideAdvancedOptions);
	ini.SetLongValue("MiscSettings","DisplayOnscreenFPS", options.bDisplayOnscreenFPS);

	//Ok lets save the settings
	ini.SaveFile(name);
	ini.Reset();
}

bool isMMXSupported() 
{ 
	int IsMMXSupported; 
	__asm 
	{ 
		mov eax,1	// CPUID level 1 
		cpuid		// EDX = feature flag 
		and edx,0x800000		// test bit 23 of feature flag 
		mov IsMMXSupported,edx	// != 0 if MMX is supported 
	} 
	if (IsMMXSupported != 0) 
		return true; 
	else 
		return false; 
} 

bool isSSESupported() 
{
	int SSESupport;
	// And finally, check the CPUID for Streaming SIMD Extensions support.
	__asm
	{
		mov		eax, 1			// Put a "1" in eax to tell CPUID to get the feature bits
		cpuid					// Perform CPUID (puts processor feature info into EDX)
		and		edx, 02000000h	// Test bit 25, for Streaming SIMD Extensions existence.
		mov		SSESupport, edx	// SIMD Extensions).  Set return value to 1 to indicate,
	}
	
	if (SSESupport != 0) 
		return true; 
	else 
		return false; 
} 

void ReadConfiguration(void)
{

	char name[1024];
	GetPluginDir(name);
	strcat(name, CONFIG_FILE);

	options.bEnableHacks = TRUE;
	options.bEnableSSE = TRUE;
	options.bEnableVertexShader = FALSE;

	defaultRomOptions.screenUpdateSetting = SCREEN_UPDATE_AT_VI_CHANGE;
	//defaultRomOptions.screenUpdateSetting = SCREEN_UPDATE_AT_VI_UPDATE_AND_DRAWN;

	status.isMMXSupported = isMMXSupported();
	status.isSSESupported = isSSESupported();
	status.isVertexShaderSupported = false;

	defaultRomOptions.N64FrameBufferEmuType = FRM_BUF_NONE;
	defaultRomOptions.N64FrameBufferWriteBackControl = FRM_BUF_WRITEBACK_NORMAL;
	defaultRomOptions.N64RenderToTextureEmuType = TXT_BUF_NONE;
	FILE *f = fopen(name, "rb");
	if(!f)
	{
		options.bEnableFog = TRUE;
		options.bWinFrameMode = FALSE;
		options.bMipMaps = TRUE;
		options.bForceSoftwareTnL = TRUE;
		options.bEnableSSE = TRUE;
		options.bEnableVertexShader = FALSE;
		options.forceTextureFilter = 0;
		options.bLoadHiResTextures = FALSE;
		// set caching by default to "off"
		options.bCacheHiResTextures = FALSE;
		options.bDumpTexturesToFiles = FALSE;
		options.textureEnhancement = 0;
		options.textureEnhancementControl = 0;
		options.bDisplayTooltip = FALSE;
		options.bHideAdvancedOptions = TRUE;
		options.bDisplayOnscreenFPS = FALSE;
		options.DirectXAntiAliasingValue = 0;
		options.DirectXDevice = 0;	// HAL device
		options.DirectXAnisotropyValue = 0;
		options.DirectXMaxFSAA = 16;
		options.FPSColor = 0xFFFFFFFF;
		options.DirectXMaxAnisotropy = 16;

		defaultRomOptions.N64FrameBufferEmuType = FRM_BUF_NONE;
		defaultRomOptions.N64FrameBufferWriteBackControl = FRM_BUF_WRITEBACK_NORMAL;
		defaultRomOptions.N64RenderToTextureEmuType = TXT_BUF_NONE;

		defaultRomOptions.bNormalBlender = FALSE;
		defaultRomOptions.bNormalCombiner = FALSE;
		defaultRomOptions.bDoubleSizeForSmallTxtrBuf = FALSE;
		windowSetting.uFullScreenRefreshRate = 0;	// 0 is the default value, means to use Window default frequency
		windowSetting.uScreenScaleMode = 0;

		WriteConfiguration();
		return;
	}
	else
	{
		fclose(f);
		ini.LoadFile(name);
		
		windowSetting.uWindowDisplayWidth = (uint16)ini.GetLongValue("WindowSetting", "WindowedWidth", 640);
		windowSetting.uWindowDisplayHeight = (uint16)ini.GetLongValue("WindowSetting", "WindowedHeight", 480);
		windowSetting.uDisplayWidth = windowSetting.uWindowDisplayWidth;
		windowSetting.uDisplayHeight = windowSetting.uWindowDisplayHeight;
		windowSetting.uFullScreenDisplayWidth = (uint16)ini.GetLongValue("WindowSetting", "FullscreenWidth", 640);
		windowSetting.uFullScreenDisplayHeight = (uint16)ini.GetLongValue("WindowSetting", "FullScreenHeight", 480);
		windowSetting.uFullScreenRefreshRate = ini.GetLongValue("WindowSetting", "FullScreenFrequency");
		windowSetting.uScreenScaleMode = ini.GetLongValue("WindowSetting", "ScreenScaleMode", 0);

		defaultRomOptions.N64FrameBufferEmuType = ini.GetLongValue("FrameBufferSettings", "FrameBufferSetting");
		defaultRomOptions.N64FrameBufferWriteBackControl = ini.GetLongValue("FrameBufferSettings", "FrameBufferWriteBackControl");
		defaultRomOptions.N64RenderToTextureEmuType = ini.GetLongValue("FrameBufferSettings", "RenderToTexture");
		

		options.textureEnhancement = ini.GetLongValue("Texture Settings","TextureEnhancement");
		options.textureEnhancementControl = ini.GetLongValue("Texture Settings","TextureEnhancementControl");
		options.forceTextureFilter = ini.GetLongValue("Texture Settings","ForceTextureFilter");
		options.bLoadHiResTextures = ini.GetBoolValue("Texture Settings","LoadHiResTextures");
		options.bCacheHiResTextures = ini.GetBoolValue("Texture Settings","CacheHiResTextures");
		options.bDumpTexturesToFiles = ini.GetBoolValue("Texture Settings","DumpTexturesToFiles");
		defaultRomOptions.bDoubleSizeForSmallTxtrBuf = ini.GetBoolValue("Texture Settings", "DoubleSizeForSmallTxtrBuf");

		options.DirectXDevice = ini.GetLongValue("RenderSetting", "DirectXDevice");
		options.DirectXAntiAliasingValue = ini.GetLongValue("RenderSetting", "DirectXAntiAliasingValue");
		options.DirectXAnisotropyValue = ini.GetLongValue("RenderSetting", "DirectXAnisotropyValue");
		options.DirectXMaxFSAA = ini.GetLongValue("RenderSetting", "DirectXMaxFSAA");
		options.DirectXMaxAnisotropy = ini.GetLongValue("RenderSetting", "DirectXMaxAnisotropy");
		defaultRomOptions.bNormalBlender = ini.GetBoolValue("RenderSetting", "NormalAlphaBlender");
		options.bEnableFog = ini.GetBoolValue("RenderSetting", "EnableFog");
		options.bWinFrameMode = ini.GetBoolValue("RenderSetting", "WinFrameMode");
		options.bMipMaps = ini.GetBoolValue("RenderSetting", "MipMaps");
		options.bForceSoftwareTnL = ini.GetLongValue("RenderSetting", "ForceSoftwareTnL");
		
		options.bEnableVertexShader = ini.GetBoolValue("RenderSetting", "EnableVertexShader");
		options.bEnableVertexShader = FALSE;

		options.bDisplayTooltip = ini.GetBoolValue("MiscSettings", "DisplayTooltip");
		options.bHideAdvancedOptions = ini.GetBoolValue("MiscSettings", "HideAdvancedOptions");
		options.bDisplayOnscreenFPS = ini.GetBoolValue("MiscSettings", "DisplayOnscreenFPS");
		options.bEnableSSE = ini.GetBoolValue("MiscSettings", "EnableSSE");		
		options.FPSColor = ini.GetLongValue("MiscSettings", "FPSColor");
		ini.Reset();
	}

	status.isSSEEnabled = status.isSSESupported && options.bEnableSSE;
	if( status.isSSEEnabled )
		ProcessVertexData = ProcessVertexDataSSE;
	else
		ProcessVertexData = ProcessVertexDataNoSSE;

	status.isVertexShaderEnabled = status.isVertexShaderSupported && options.bEnableVertexShader;
	status.bUseHW_T_L = false;
}
	
//---------------------------------------------------------------------------------------
BOOL InitConfiguration(void)
{
	//Initialize this DLL
	GetPluginDir(szIniFileName);
	strcat(szIniFileName, INI_FILE);

	//Load the per rom configs into memory
	perRomIni.LoadFile(szIniFileName);

	ReadConfiguration();
	return TRUE;
}


void GenerateCurrentRomOptions()
{
	currentRomOptions.N64FrameBufferEmuType		=g_curRomInfo.dwFrameBufferOption;	
	currentRomOptions.N64FrameBufferWriteBackControl		=defaultRomOptions.N64FrameBufferWriteBackControl;	
	currentRomOptions.N64RenderToTextureEmuType	=g_curRomInfo.dwRenderToTextureOption;	
	currentRomOptions.screenUpdateSetting		=g_curRomInfo.dwScreenUpdateSetting;
	currentRomOptions.bNormalCombiner			=g_curRomInfo.dwNormalCombiner;
	currentRomOptions.bNormalBlender			=g_curRomInfo.dwNormalBlender;

	options.enableHackForGames = NO_HACK_FOR_GAME;

	if ((strncmp(g_curRomInfo.szGameName, "BANJO TOOIE", 11) == 0))
	{
		options.enableHackForGames = HACK_FOR_BANJO_TOOIE;
	}
	else if ((strncmp(g_curRomInfo.szGameName, "DR.MARIO", 8) == 0))
	{
		options.enableHackForGames = HACK_FOR_DR_MARIO;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "Pilot", 5) == 0))
	{
		options.enableHackForGames = HACK_FOR_PILOT_WINGS;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "YOSHI", 5) == 0))
	{
		options.enableHackForGames = HACK_FOR_YOSHI;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "NITRO", 5) == 0))
	{
		options.enableHackForGames = HACK_FOR_NITRO;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "TONY HAWK", 9) == 0))
	{
		options.enableHackForGames = HACK_FOR_TONYHAWK;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "THPS", 4) == 0))
	{
		options.enableHackForGames = HACK_FOR_TONYHAWK;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "SPIDERMAN", 9) == 0))
	{
		options.enableHackForGames = HACK_FOR_TONYHAWK;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "NASCAR", 6) == 0))
	{
		options.enableHackForGames = HACK_FOR_NASCAR;
	}
	else if ((strstr(g_curRomInfo.szGameName, "ZELDA") != 0) && (strstr(g_curRomInfo.szGameName, "MASK") != 0))
	{
		options.enableHackForGames = HACK_FOR_ZELDA_MM;
	}
	else if ((strstr(g_curRomInfo.szGameName, "ZELDA") != 0))
	{
		options.enableHackForGames = HACK_FOR_ZELDA;
	}
	else if ((strstr(g_curRomInfo.szGameName, "Ogre") != 0))
	{
		options.enableHackForGames = HACK_FOR_OGRE_BATTLE;
	}
	else if ((strstr(g_curRomInfo.szGameName, "TWINE") != 0))
	{
		options.enableHackForGames = HACK_FOR_TWINE;
	}
	else if ((strstr(g_curRomInfo.szGameName, "Squadron") != 0))
	{
		options.enableHackForGames = HACK_FOR_ROGUE_SQUADRON;
	}
	else if ((strstr(g_curRomInfo.szGameName, "Baseball") != 0) && (strstr(g_curRomInfo.szGameName, "Star") != 0))
	{
		options.enableHackForGames = HACK_FOR_ALL_STAR_BASEBALL;
	}
	else if ((strstr(g_curRomInfo.szGameName, "Tigger") != 0) && (strstr(g_curRomInfo.szGameName, "Honey") != 0))
	{
		options.enableHackForGames = HACK_FOR_TIGER_HONEY_HUNT;
	}
	else if ((strstr(g_curRomInfo.szGameName, "Bust") != 0) && (strstr(g_curRomInfo.szGameName, "Move") != 0))
	{
		options.enableHackForGames = HACK_FOR_BUST_A_MOVE;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "MarioTennis",11) == 0))
	{
		options.enableHackForGames = HACK_FOR_MARIO_TENNIS;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "SUPER BOWLING",13) == 0))
	{
		options.enableHackForGames = HACK_FOR_SUPER_BOWLING;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "CONKER",6) == 0))
	{
		options.enableHackForGames = HACK_FOR_CONKER;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "MK_MYTHOLOGIES",14) == 0))
	{
		options.enableHackForGames = HACK_REVERSE_Y_COOR;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "Fighting Force",14) == 0))
	{
		options.enableHackForGames = HACK_REVERSE_XY_COOR;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "GOLDENEYE",9) == 0))
	{
		options.enableHackForGames = HACK_FOR_GOLDEN_EYE;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "F-ZERO",6) == 0))
	{
		options.enableHackForGames = HACK_FOR_FZERO;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "Command&Conquer",15) == 0))
	{
		options.enableHackForGames = HACK_FOR_COMMANDCONQUER;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "READY 2 RUMBLE",14) == 0))
	{
		options.enableHackForGames = HACK_FOR_RUMBLE;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "READY to RUMBLE",15) == 0))
	{
		options.enableHackForGames = HACK_FOR_RUMBLE;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "South Park Rally",16) == 0))
	{
		options.enableHackForGames = HACK_FOR_SOUTH_PARK_RALLY;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "Extreme G 2",11) == 0))
	{
		options.enableHackForGames = HACK_FOR_EXTREME_G2;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "MarioGolf64",11) == 0))
	{
		options.enableHackForGames = HACK_FOR_MARIO_GOLF;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "MLB FEATURING",13) == 0))
	{
		options.enableHackForGames = HACK_FOR_MLB;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "POLARISSNOCROSS",15) == 0))
	{
		options.enableHackForGames = HACK_FOR_POLARISSNOCROSS;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "TOP GEAR RALLY",14) == 0))
	{
		options.enableHackForGames = HACK_FOR_TOPGEARRALLY;
	}
	else if ((_strnicmp(g_curRomInfo.szGameName, "DUKE NUKEM",10) == 0))
	{
		options.enableHackForGames = HACK_FOR_DUKE_NUKEM;
	}
	else if ((_stricmp(g_curRomInfo.szGameName, "MARIOKART64") == 0))
	{
		options.enableHackForGames = HACK_FOR_MARIO_KART;
	}

	if( currentRomOptions.N64FrameBufferEmuType == 0 )		currentRomOptions.N64FrameBufferEmuType = defaultRomOptions.N64FrameBufferEmuType;
	else currentRomOptions.N64FrameBufferEmuType--;
	if( currentRomOptions.N64RenderToTextureEmuType == 0 )		currentRomOptions.N64RenderToTextureEmuType = defaultRomOptions.N64RenderToTextureEmuType;
	else currentRomOptions.N64RenderToTextureEmuType--;
	if( currentRomOptions.screenUpdateSetting == 0 )		currentRomOptions.screenUpdateSetting = defaultRomOptions.screenUpdateSetting;
	if( currentRomOptions.bNormalCombiner == 0 )			currentRomOptions.bNormalCombiner = defaultRomOptions.bNormalCombiner;
	else currentRomOptions.bNormalCombiner--;
	if( currentRomOptions.bNormalBlender == 0 )			currentRomOptions.bNormalBlender = defaultRomOptions.bNormalBlender;
	else currentRomOptions.bNormalBlender--;

	GenerateFrameBufferOptions();

	if( options.enableHackForGames == HACK_FOR_MARIO_GOLF || options.enableHackForGames == HACK_FOR_MARIO_TENNIS )
	{
		frameBufferOptions.bIgnoreRenderTextureIfHeightUnknown = true;
	}
}

void Ini_GetRomOptions(LPGAMESETTING pGameSetting)
{
	CHAR szCRC[50+1];
	// Generate the CRC-ID for this rom:
	wsprintf(szCRC, "%08x%08x-%02x", pGameSetting->romheader.dwCRC1,  pGameSetting->romheader.dwCRC2, pGameSetting->romheader.nCountryID);

	pGameSetting->bDisableCulling		= perRomIni.GetBoolValue(szCRC, "bDisableCulling", false);
	pGameSetting->bIncTexRectEdge		= perRomIni.GetBoolValue(szCRC, "bIncTexRectEdge", false);
	pGameSetting->bZHack				= perRomIni.GetBoolValue(szCRC, "bZHack", false);
	pGameSetting->bTextureScaleHack		= perRomIni.GetBoolValue(szCRC, "bTextureScaleHack", false);
	pGameSetting->bPrimaryDepthHack		= perRomIni.GetBoolValue(szCRC, "bPrimaryDepthHack", false);
	pGameSetting->bTexture1Hack			= perRomIni.GetBoolValue(szCRC, "bTexture1Hack", false);
	pGameSetting->bFastLoadTile			= perRomIni.GetBoolValue(szCRC, "bFastLoadTile", false);
	
	pGameSetting->VIWidth				= perRomIni.GetLongValue(szCRC, "VIWidth", -1);
	pGameSetting->VIHeight				= perRomIni.GetLongValue(szCRC, "VIHeight", -1);
	pGameSetting->UseCIWidthAndRatio	= perRomIni.GetLongValue(szCRC, "UseCIWidthAndRatio", NOT_USE_CI_WIDTH_AND_RATIO);
	pGameSetting->bTxtSizeMethod2		= perRomIni.GetBoolValue(szCRC, "bTxtSizeMethod2", false);
	pGameSetting->bEnableTxtLOD			= perRomIni.GetBoolValue(szCRC, "bEnableTxtLOD", false);

	pGameSetting->bEmulateClear			= perRomIni.GetBoolValue(szCRC, "bEmulateClear", false);
	pGameSetting->bForceScreenClear		= perRomIni.GetBoolValue(szCRC, "bForceScreenClear", false);

	pGameSetting->bDisableBlender		= perRomIni.GetBoolValue(szCRC, "bDisableBlender", false);
	pGameSetting->bForceDepthBuffer		= perRomIni.GetBoolValue(szCRC, "bForceDepthBuffer", false);
	pGameSetting->bDisableObjBG			= perRomIni.GetBoolValue(szCRC, "bDisableObjBG", false);

	pGameSetting->dwNormalCombiner		= perRomIni.GetLongValue(szCRC, "dwNormalCombiner", 0);
	pGameSetting->dwNormalBlender		= perRomIni.GetLongValue(szCRC, "dwNormalBlender", 0);
	pGameSetting->dwFrameBufferOption	= perRomIni.GetLongValue(szCRC, "dwFrameBufferOption", 0);
	pGameSetting->dwRenderToTextureOption	= perRomIni.GetLongValue(szCRC, "dwRenderToTextureOption", 0);
	pGameSetting->dwScreenUpdateSetting	= perRomIni.GetLongValue(szCRC, "dwScreenUpdateSetting", 0);
}

void Ini_StoreRomOptions(LPGAMESETTING pGameSetting)
{
	CHAR szCRC[50+1];
	// Generate the CRC-ID for this rom:
	wsprintf(szCRC, "%08x%08x-%02x", pGameSetting->romheader.dwCRC1,  pGameSetting->romheader.dwCRC2, pGameSetting->romheader.nCountryID);

	perRomIni.SetLongValue(szCRC, "bDisableCulling", pGameSetting->bDisableCulling);
	perRomIni.SetLongValue(szCRC, "bEmulateClear", pGameSetting->bEmulateClear);
	perRomIni.SetLongValue(szCRC, "dwNormalBlender", pGameSetting->dwNormalBlender);
	perRomIni.SetLongValue(szCRC, "bDisableBlender", pGameSetting->bDisableBlender);
	perRomIni.SetLongValue(szCRC, "bForceScreenClear", pGameSetting->bForceScreenClear);
	perRomIni.SetLongValue(szCRC, "dwNormalCombiner", pGameSetting->dwNormalCombiner);
	perRomIni.SetLongValue(szCRC, "bForceDepthBuffer", pGameSetting->bForceDepthBuffer);
	perRomIni.SetLongValue(szCRC, "bDisableObjBG", pGameSetting->bDisableObjBG);
	perRomIni.SetLongValue(szCRC, "dwFrameBufferOption", pGameSetting->dwFrameBufferOption);
	perRomIni.SetLongValue(szCRC, "dwRenderToTextureOption", pGameSetting->dwRenderToTextureOption);
	perRomIni.SetLongValue(szCRC, "dwScreenUpdateSetting", pGameSetting->dwScreenUpdateSetting);
	perRomIni.SetLongValue(szCRC, "bIncTexRectEdge", pGameSetting->bIncTexRectEdge);
	perRomIni.SetLongValue(szCRC, "bZHack", pGameSetting->bZHack);
	perRomIni.SetLongValue(szCRC, "bTextureScaleHack", pGameSetting->bTextureScaleHack);
	perRomIni.SetLongValue(szCRC, "bPrimaryDepthHack", pGameSetting->bPrimaryDepthHack);
	perRomIni.SetLongValue(szCRC, "bTexture1Hack", pGameSetting->bTexture1Hack);
	perRomIni.SetLongValue(szCRC, "bFastLoadTile", pGameSetting->bFastLoadTile);
	perRomIni.SetLongValue(szCRC, "VIWidth", pGameSetting->VIWidth);
	perRomIni.SetLongValue(szCRC, "VIHeight", pGameSetting->VIHeight);
	perRomIni.SetLongValue(szCRC, "UseCIWidthAndRatio", pGameSetting->UseCIWidthAndRatio);
	perRomIni.SetLongValue(szCRC, "bTxtSizeMethod2", pGameSetting->bTxtSizeMethod2);
	perRomIni.SetLongValue(szCRC, "bEnableTxtLOD", pGameSetting->bEnableTxtLOD);
	perRomIni.SaveFile(szIniFileName);

	//Ensure that we now have the up to date settings stored in memory by flushing out the current ones and reloading the file
	perRomIni.Reset();
	perRomIni.LoadFile(szIniFileName);
	TRACE0("Rom option is changed and saved");
}



// Tooltip functions from Glide64 project, original from MSDN library

typedef struct {
	int id;
	char *title;
	char *text;
} ToolTipMsg;


ToolTipMsg ttmsg[] = {
	{ 
		IDC_SLIDER_FSAA,
			"DirectX Full Screen Mode Anti-Aliasing Setting",
			"Please refer to your video card driver setting to determine the maximum supported FSAA value. The plugin will try to determine "
			"the highest supported FSAA mode, but it may not work well enough since highest FSAA setting is also dependent on the full scrren "
			"resolution. Using incorrect FSAA value will cause DirectX fail to initialize.\n\n"
			"FSAA usage is not compatible with frame buffer effects. Frame buffer may fail to work if FSAA is used."
	},
	{ 
		IDC_SLIDER_ANISO,
			"DirectX Anisotropy Filtering Setting",
			"Use this to set the amount of anisotropic filtering."
	},
	{ 
		IDC_RESOLUTION_WINDOW_MODE,
			"Window mode display resolution",
			"Sets the resolution of the windowed image"
	},
	{ 
		IDC_RESOLUTION_FULL_SCREEN_MODE,
			"Full screen mode display resolution",
			"Sets the fullscreen resolution"
	},
	{ 
		IDC_TEXTURE_ENHANCEMENT,
			"Texture enhancement",
			"Enhances the texture when loading the texture.\n\n"
			"- 2x        double the texture size\n"
			"- 2x texture rectangle only,    double the texture size, only for textRect, not for triangle primitives\n"
			"- 2xSai,    double the texture size and apply 2xSai algorithm\n"
			"- 2xSai for texture rectangle only\n"
			"- Sharpen,      apply sharpen filter (cool effects)\n"
			"- Sharpen more, do more sharpening"
	},
	{ 
		IDC_FORCE_TEXTURE_FILTER,
			"Force texture filter",
			"Force Nearest filter, or force bilinear filtering\n"
	},
	{ 
		IDC_FOG,
			"Enable/Disable Fog",
			"Enable or disable fog emulation by this option\n"
	},
	{ 
		IDC_SSE,
			"Enable/Disable SSE for Intel P3/P4 CPUs",
			"SSE (Intel Streaming SMID Extension) can speed up 3D transformation, vertex and matrix processing. "
			"It is only available with Intel P3 and P4 CPUs, not with AMD CPUs. P3 is actually much faster than P4 "
			"with SSE instructions\n"
	},
	{ 
		IDC_SKIP_FRAME,
			"Frame skipping",
			"If this option is on, the plugin will skip frames, to maintain speed. This could help to improve "
			"speed for some games, and could cause flickering for other games.\n"
	},
	{ 
		IDC_VERTEX_SHADER,
			"Vertex Shader",
			"If this option is on, the plugin will try to use vertex shaders if supported by the GPU. Using "
			"a vertex shader will transfer most CPU duty on vertex transforming and lighting to the GPU, "
			"which will greatly decrease the CPU duty and increase the game's speed.\n"
			"The plugin uses Vertex Shader Model 1.0 which is defined by DirectX 8.0. The plugin supports vertex shaders "
			"in DirectX mode only at this moment."
	},
	{ 
		IDC_ALPHA_BLENDER,
			"Force to use normal alpha blender",
			"Use this option if you have opaque/transparency problems with certain games.\n"
			"\nWhen a game is not running, it is the default value (for all games), available values are on/off.\n"
			"When a game is running, it is the game setting. Three available setting are on/off/as default."
	},
	{ 
		IDC_NORMAL_COMBINER,
			"Normal color combiner",
			"Forces plugin to use normal color combiner\n"
			"Normal color combiner is:\n"
			"- Texture * Shade,  if both texture and shade are used\n"
			"- Texture only,     if texture is used and shade is not used\n"
			"- shade only,       if texture is not used\n\n"
			"Try to use this option if you have ingame texture color problems, transparency problems, "
			"or black/white texture problems\n"
			"\nWhen a game is not running, it is the default value (for all games), available values are on/off.\n"
			"When a game is running, it is the game setting. Three available setting are on/off/as default."
	},
	{ 
		IDC_FORCE_BUFFER_CLEAR,
			"Force Buffer Clear",
			"Force to clear screen before drawing any primitives.\n"
			"This is in fact a hack, only for a few games, including KI Gold, need this.\n"
			"\nWhen a game is not running, it is the default value (for all games), available values are on/off.\n"
			"When a game is running, it is the game setting. Three available setting are on/off/as default."
	},
	{ 
		IDC_DISABLE_BG,
			"Disable BG primitives",
			"Disable this option for Zelda MM, otherwise its intro will be covered by a black layer (by drawing of a black BG texture).\n"
			"\nWhen a game is not running, it is the default value (for all games), available values are on/off.\n"
			"When a game is running, it is the game setting. Three available setting are on/off/as default."
	},
	{ 
		IDC_SCREEN_UPDATE_AT,
			"Control when the screen will be updated",
			"\n"
			"At VI origin update (default)\n"
			"At VI origin change\n"
			"At CI change\n"
			"At the 1st CI change\n"
			"At the 1st drawing\n"
			"\nWhen a game is not running, it is the default value (for all games).\n"
			"When a game is running, it is the game setting."
	},
	{ 
		IDC_SCREEN_UPDATE_LABEL,
			"Control when the screen will be updated",
			"This option is to prevent or reduce flicking in certain games by controlling when the screen will be updated\n\n"
			"At VI origin update (default)\n"
			"At VI origin change\n"
			"At CI change\n"
			"At the 1st CI change\n"
			"At the 1st drawing\n"
			"\nWhen a game is not running, it is the default value (for all games).\n"
			"When a game is running, it is the game setting."
	},
	{ 
		IDC_FRAME_BUFFER_SETTING,
			"N64 CPU frame buffer emulation",
			"The CPU frame buffer is referred to N64 drawing buffers in RDRAM."
			"Games could draw into a frame buffer other than a displayed render buffer and use the result as textures for further drawing into rendering buffer. "
			"It is very difficult to emulate N64 frame buffer through either DirectX or OpenGL\n\n"
			"- None (default), don't do any frame buffer emulating\n"
			"- Hide framebuffer effects,  ignore frame buffer drawing, at least such drawing won't draw to the current rendering buffer\n"
			"- Basic framebuffer, will check texture loading address to see if the address is within the frame buffer\n"
			"- Basic & Write back, will write the frame buffer back to RDRAM if a texture is loaded from it\n"
			"- Write back & Reload, will load frame buffer from RDRAM at each frame\n"
			"- Write Back Every Frame,       a complete emulation, very slow\n"
			"- With Emulator,  new 1964 will inform the plugin about CPU frame buffer memory direct read/write, for Dr. Mario\n"
	},
	{ 
		IDC_RENDER_TO_TEXTURE_SETTING,
			"Render-to-texture emulation",
			"- None (default), don't do any Render-to-texture emulation\n"
			"- Hide Render-to-texture effects,  ignore Render-to-texture drawing, at least such drawing won't draw to the current rendering buffer\n"
			"- Render-to-texture,    support self-render-texture\n"
			"- Basic Render-to-texture, will check texture loading address to see if the address is within the frame buffer\n"
			"- Basic & Write back, will write the Render-to-render_texture back when rendering is finished\n"
			"- Write back & Reload, will load Render-to-render_texture from RDRAM before the buffer is rendered.\n"
	},
	{ 
		IDC_FRAME_BUFFER_WRITE_BACK_CONTROL,
			"Frame Buffer Write Back Control",
			"Control the frequency of frame buffer writing back to RDRAM\n"
	},
	{ 
		IDC_OPTION_GROUP,
			"Default options or Rom specific settings",
			"\nWhen a game is not running, it is the default value (for all games).\n"
			"When a game is running, it is the game setting."
	},
	{ 
		IDC_EMULATE_CLEAR,
			"Emulate Memory Clear",
			"\nA few games need this option to work better, including DK64."
	},
	{ 
		IDC_SOFTWARE_TNL,
			"Force Software Transformation and Lighting",
			"\nThis option will force software T and L instead of available hardware T and L. "
			"It is needed for most newer ATI Radeons."
			"\n\nThe plugin will run slower with this option on. If you don't need it, don't leave it on."
	},
	{ 
		IDC_FULLSCREEN_FREQUENCY,
			"Monitor Refresh Frequency in Fullscreen Mode",
			"Select the frequency for your full screen mode.\n\n"
			"You should know what's the highest frequency your monitor can display for each screen resolution. If you select a higher frequency "
			"then your monitor can display, you will get black screen or full screen just does not work. At the time, you can press [ALT-Enter] key again to go back to windowed mode."
	},
	{ 
		IDC_TOOLTIP,
			"Display tooltips",
			"Enable/Disable tooltip display in the configuration dialog box\n\n"
	},
	{ 
		IDC_FORCE_DEPTH_COMPARE,
			"Force Using Depth Buffer",
			"Force to enable depth buffer compare and update.\n\n"
	},
	{ 
		IDC_DISABLE_BLENDER,
			"Disable Alpha Blender",
			"Enable / Disable Alpha Blender\n\n"
			"This option is different from the Normal Blender option. If this option is on, the alpha blender "
			"will be disabled completely. All transparency effects are disabled. "
	},
	{ 
		IDC_EDIT_WIDTH,
			"Manually Set the N64 Screen Resolution Width",
			"Manually set the N64 screen width, the value will overwrite the screen resolution auto detection"
	},
	{ 
		IDC_EDIT_HEIGHT,
			"Manually Set the N64 Screen Resolution Height",
			"Manually set the N64 screen height, the value will overwrite the screen resolution auto detection"
	},
	{ 
		IDC_INCREASE_TEXTRECT_EDGE,
			"Increase TextRect Edge by 1",
			"This is an advanced option. Try it if you see there are horizonal or vertical thin "
			"lines across big texture blocks in menu or logo."
	},
	{ 
		IDC_Z_HACK,
			"Hack the z value",
			"This is an advanced option. If enabled, range of vertex Z values will be adjusted "
			" so that vertexes before the near plane can be rendered without clipped."
	},
	{ 
		IDC_TEXTURE_SCALE_HACK,
			"Hack Texture Scale",
			"This is an advanced option. Don't bother if you have no idea what it is. It is only "
			"a hack for a few games."
	},
	{ 
		IDC_FASTER_LOADING_TILE, //Make this a forced feature?
			"Faster Texture Tile Loading Algorithm",
			"This is an advanced option. It may increase texture loading if textures are loaded "
			"by LoadTile ucodes."
	},
	{ 
		IDC_PRIMARY_DEPTH_HACK,
			"Primary Depth Hack",
			"This is an advanced option. This is a hack for a few games, don't bother with it."
	},
	{ 
		IDC_TEXTURE_1_HACK,
			"Texture 1 Hack",
			"This is an advanced option. This is a hack for a few games, don't bother with it."
	},
	{ 
		IDC_DISABLE_CULLING,
			"Disable DL Culling",
			"This is an advanced option. If enabled, it will disable the CullDL ucode."
	},
	{ 
		IDC_SHOW_FPS,
			"Display OnScreen FPS",
			"If enabled, current FPS (frame per second) will be displayed at the right-bottom corner of the screen "
			"in selected color"
	},
	{ 
		IDC_FPS_COLOR,
			"Onscreen FPS Display Text Color",
			"Color must be in 32bit HEX format, as AARRGGBB, AA=alpha, RR=red, GG=green, BB=Blue\n"
			"Data must be entered exactly in 8 hex numbers, or the entered value won't be accepted."
	},
	{ 
		IDC_AUTO_WRITE_BACK,
			"Automatically write overlapped texture back to RDRAM",
			"If enabled, such render-to-textures or saved back buffer textures will be written back "
			"to RDRAM if they are to be covered partially by new textures.\n"
	},
	{ 
		IDC_TXTR_BUF_DOUBLE_SIZE,
			"Double Texture Buffer Size for Small Render-to-Textures",
			"Enable this option to have better render-to-texture quality, of course this requires "
			"more video RAM."
	},
	{ 
		IDC_HIDE_ADVANCED_OPTIONS,
			"Hide Advanced Options",
			"If enabled, all advanced options will be hidden. Per game settings, default games settings "
			"and texture filter settings will be all hidden."
	},
	{ 
		IDC_WINFRAME_MODE,
			"Enable wireframes (WinFrame)",
			"If enabled, graphics will be drawn in wireframe mode instead of solid and texture mode."
	},
	{
		IDC_MIPMAPS,
			"Enables automatic mipmaping.",
			"This will allow textures to look nicer when viewed far away and increase performance in some cases."
	},
	{ 
	   IDC_LOAD_HIRES_TEXTURE,
	   "Load custom hi-res textures",
	   "Loads custom game textures, which were dumped from the game. This option is used extensively for custom "
	   "game graphics projects by fans."
	},
	{ 
	   IDC_CACHE_HIRES_TEXTURE,
	   "Cache custom hi-res textures",
	   "Caches all custom game textures before the game is started. This produces smoother gameplay - especially for "
	   "large textures.\n\n But be aware that the memory occupation might become very high for large texture packs."
	   "Furthermore it will delay the game start as the textures have to be actually loaded first."
	},
	{
	   IDC_DUMP_TEXTURE_TO_FILES,
	   "Dump textures to files",
	   "This option dumps the textures in the game to normal JPG/BMP/PNG files.\n\nThis can be very useful "
	   "for custom game art, high resolution textures, and other graphics modifications to games."
	},
	{
	   IDC_TXT_SIZE_METHOD_2,
	   "Alternate texture size calculation",
	   "This option uses a different method when calculating textures then what is normally used. "
	   "Use of this option is not normally needed."   
	},
   {
	   IDC_USE_CI_WIDTH_AND_RATIO,
	   "Sets CI width and ratio",
	   "No - doesnt set CI ratio and width\n"
	   "NTSC - sets CI width and ratio according to NTSC standards\n"
	   "PAL - sets CI width and ratio according to PAL specs\n"
	   },
   {
	   IDC_ENABLE_LOD,
	   "Enables Texture LOD (Level of Detail)",
	   "This is a advanced option. It doesnt need to be enabled in most cases."
	   },
};

int numOfTTMsgs = sizeof(ttmsg)/sizeof(ToolTipMsg);


// ** TOOLTIP CODE FROM MSDN LIBRARY SAMPLE WITH SEVERAL MODIFICATIONS **

// DoCreateDialogTooltip - creates a tooltip control for a dialog box, 
//     enumerates the child control windows, and installs a hook 
//     procedure to monitor the message stream for mouse messages posted 
//     to the control windows. 
// Returns TRUE if successful, or FALSE otherwise. 
// 
// Global variables 
// g_hinst - handle to the application instance. 
// g_hwndTT - handle to the tooltip control. 
// g_hwndDlg - handle to the dialog box. 
// g_hhk - handle to the hook procedure. 

bool CreateDialogTooltip(void) 
{
#ifdef ENABLE_CONFIG_DIALOG
    // Ensure that the common control DLL is loaded, and create
    // a tooltip control.
    InitCommonControls();
    g_hwndTT = CreateWindowEx(0, TOOLTIPS_CLASS, (LPSTR) NULL,
        TTS_ALWAYSTIP|/*TTS_BALLOON*/0x40, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, g_hwndDlg, (HMENU) NULL, windowSetting.myhInst, NULL);
	
    if (g_hwndTT == NULL)
        return FALSE;
    // Install a hook procedure to monitor the message stream for mouse
    // messages intended for the controls in the dialog box.
    g_hhk = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc,
        (HINSTANCE) NULL, GetCurrentThreadId());
	
    if (g_hhk == (HHOOK) NULL)
        return FALSE;
#endif
    return TRUE;
} 

bool EnumChildWndTooltip(void)
{
	return (!EnumChildWindows(g_hwndDlg, (WNDENUMPROC) EnumChildProc, 0));
}

// EmumChildProc - registers control windows with a tooltip control by
//     using the TTM_ADDTOOL message to pass the address of a 
//     TOOLINFO structure. 
// Returns TRUE if successful, or FALSE otherwise. 
// hwndCtrl - handle of a control window. 
// lParam - application-defined value (not used). 
extern "C"  bool __stdcall EnumChildProc(HWND hwndCtrl, LPARAM lParam) 
{ 
    TOOLINFO ti; 
	
    ti.cbSize = sizeof(TOOLINFO); 
    ti.uFlags = TTF_IDISHWND; 
    ti.hwnd = g_hwndDlg; 
    ti.uId = (UINT) hwndCtrl; 
    ti.hinst = 0; 
    ti.lpszText = LPSTR_TEXTCALLBACK; 
    SendMessage(g_hwndTT, TTM_ADDTOOL, 0, 
       (LPARAM) (LPTOOLINFO) &ti); 
    return TRUE; 
} 

// GetMsgProc - monitors the message stream for mouse messages intended 
//     for a control window in the dialog box. 
// Returns a message-dependent value. 
// nCode - hook code. 
// wParam - message flag (not used). 
// lParam - address of an MSG structure. 
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    MSG *lpmsg; 
	
    lpmsg = (MSG *) lParam; 
    //if (nCode < 0 || !(IsChild(g_hwndDlg, lpmsg->hwnd))) 
	if (nCode < 0 ) 
        return (CallNextHookEx(g_hhk, nCode, wParam, lParam)); 
	
    switch (lpmsg->message) { 
	case WM_MOUSEMOVE: 
	case WM_LBUTTONDOWN: 
	case WM_LBUTTONUP: 
	case WM_RBUTTONDOWN: 
	case WM_RBUTTONUP: 
		if( options.bDisplayTooltip && g_hwndTT != NULL) 
		{ 
			MSG msg; 

			int idCtrl = GetDlgCtrlID((HWND)lpmsg->hwnd);
			
			msg.lParam = lpmsg->lParam; 
			msg.wParam = lpmsg->wParam; 
			msg.message = lpmsg->message; 
			msg.hwnd = lpmsg->hwnd; 
			SendMessage(g_hwndTT, TTM_RELAYEVENT, 0, 
				(LPARAM) (LPMSG) &msg); 
			SendMessage(g_hwndTT, TTM_SETMAXTIPWIDTH, 0, 300);
			SendMessage(g_hwndTT, TTM_SETDELAYTIME, TTDT_INITIAL, (LPARAM)MAKELONG(500,0));
			SendMessage(g_hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, (LPARAM)MAKELONG(32000,0));

			for (int i=0; i<numOfTTMsgs; i++)
			{
				if (idCtrl == ttmsg[i].id )
				{
					SendMessage(g_hwndTT, /*TTM_SETTITLE*/(WM_USER + 32), 1, (LPARAM)ttmsg[i].title);
					break;
				}
			}
		} 
		break; 
	default: 
		break; 
    } 
    return (CallNextHookEx(g_hhk, nCode, wParam, lParam)); 
} 


// OnWMNotify - provides the tooltip control with the appropriate text 
//     to display for a control window. This function is called by 
//     the dialog box procedure in response to a WM_NOTIFY message. 
// lParam - second message parameter of the WM_NOTIFY message. 
VOID OnWMNotify(LPARAM lParam) 
{ 
	LPTOOLTIPTEXT lpttt; 
	int idCtrl; 
	
	if ((((LPNMHDR) lParam)->code) == TTN_NEEDTEXT) { 
		idCtrl = GetDlgCtrlID((HWND) ((LPNMHDR) lParam)->idFrom); 
		lpttt = (LPTOOLTIPTEXT) lParam; 

		for (int i=0; i<numOfTTMsgs; i++)
		{
			if (idCtrl == ttmsg[i].id )
			{
				lpttt->lpszText = ttmsg[i].text;
				return;
			}
		}
	} 
	return;
}

///////////////////////////////////////////////
//// Constructors / Deconstructors
///////////////////////////////////////////////

char * left(char * src, int nchars)
{
	static char dst[300];			// BUGFIX (STRMNNRM)
	strncpy(dst,src,nchars);
	dst[nchars]=0;
	return dst;
}

char * right(char *src, int nchars)
{
	static char dst[300];

	int srclen = strlen(src);
	if (nchars >= srclen)
	{
		strcpy(dst, src);
	}
	else
	{
		strncpy(dst, src + srclen - nchars, nchars);
		dst[nchars]=0;
	}
	
	return dst;
}


// Find the entry corresponding to the specified rom. 
// If the rom is not found, a new entry is created
// The resulting value is returned
void __cdecl DebuggerAppendMsg (const char * Message, ...);

GameSetting g_curRomInfo;

// Swap bytes from 80 37 12 40
// to              40 12 37 80
void ROM_ByteSwap_3210(void *v, uint32 dwLen)
{
	__asm
	{
		mov		esi, v
			mov		edi, v
			mov		ecx, dwLen

			add		edi, ecx

top:
		mov		al, byte ptr [esi + 0]
		mov		bl, byte ptr [esi + 1]
		mov		cl, byte ptr [esi + 2]
		mov		dl, byte ptr [esi + 3]

		mov		byte ptr [esi + 0], dl		//3
			mov		byte ptr [esi + 1], cl		//2
			mov		byte ptr [esi + 2], bl		//1
			mov		byte ptr [esi + 3], al		//0

			add		esi, 4
			cmp		esi, edi
			jne		top

	}
}


void ROM_GetRomNameFromHeader(TCHAR * szName, ROMHeader * pHdr)
{
	TCHAR * p;

	memcpy(szName, pHdr->szName, 20);
	szName[20] = '\0';

	p = szName + (lstrlen(szName) -1);		// -1 to skip null
	while (p >= szName && *p == ' ')
	{
		*p = 0;
		p--;
	}
}


uint32 CountryCodeToTVSystem(uint32 countryCode)
{
	uint32 system;
	switch(countryCode)
	{
	case 0:
	case '7':
	case 0x41:
	case 0x45:
	case 0x4A:
		system = TV_SYSTEM_NTSC;
		break;

	case 0x44:
	case 0x46:
	case 'I':
	case 0x50:
	case 'S':
	case 0x55:
	case 0x58:
	case 0x59:
	case 0x20:
	case 0x21:
	case 0x38:
	case 0x70:
		system = TV_SYSTEM_PAL;
		break;

		/* ??? */
	default:
		system = TV_SYSTEM_PAL;
		break;
	}

	return system;
}

LRESULT APIENTRY OptionsDialogProc(HWND hDlg, unsigned message, LONG wParam, LONG lParam)
{
	int i;
	int maxres;
	HWND item;

	switch(message)
	{
	case WM_INITDIALOG:
		g_hwndDlg = hDlg;
		EnumChildWndTooltip();

		SendDlgItemMessage(hDlg, IDC_FOG, BM_SETCHECK, options.bEnableFog ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_WINFRAME_MODE, BM_SETCHECK, options.bWinFrameMode ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_MIPMAPS, BM_SETCHECK, options.bMipMaps ? BST_CHECKED : BST_UNCHECKED, 0);

		if( status.isSSESupported )
		{
			SendDlgItemMessage(hDlg, IDC_SSE, BM_SETCHECK, options.bEnableSSE ? BST_CHECKED : BST_UNCHECKED, 0);
		}
		else
		{
			SendDlgItemMessage(hDlg, IDC_SSE, BM_SETCHECK, BST_UNCHECKED, 0);
			item = GetDlgItem(hDlg, IDC_SSE );
			EnableWindow(item, FALSE);
		}

		if( status.isVertexShaderSupported )
		{
			SendDlgItemMessage(hDlg, IDC_VERTEX_SHADER, BM_SETCHECK, options.bEnableVertexShader ? BST_CHECKED : BST_UNCHECKED, 0);
		}
		else
		{
			SendDlgItemMessage(hDlg, IDC_VERTEX_SHADER, BM_SETCHECK, BST_UNCHECKED, 0);
			item = GetDlgItem(hDlg, IDC_VERTEX_SHADER );
			EnableWindow(item, FALSE);
		}

		SendDlgItemMessage(hDlg, IDC_TOOLTIP, BM_SETCHECK, options.bDisplayTooltip ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_HIDE_ADVANCED_OPTIONS, BM_SETCHECK, options.bHideAdvancedOptions ? BST_CHECKED : BST_UNCHECKED, 0);

		SendDlgItemMessage(hDlg, IDC_RESOLUTION_FULL_SCREEN_MODE, CB_RESETCONTENT, 0, 0);
		for( maxres=0; maxres<CGraphicsContext::m_numOfResolutions; maxres++ )
		{
			sprintf(generalText, "%d x %d", CGraphicsContext::m_FullScreenResolutions[maxres][0], CGraphicsContext::m_FullScreenResolutions[maxres][1]);
			SendDlgItemMessage(hDlg, IDC_RESOLUTION_FULL_SCREEN_MODE, CB_INSERTSTRING, maxres, (LPARAM) generalText);
			if( windowSetting.uFullScreenDisplayWidth == CGraphicsContext::m_FullScreenResolutions[maxres][0] &&
				windowSetting.uFullScreenDisplayHeight == CGraphicsContext::m_FullScreenResolutions[maxres][1] )
			{
				SendDlgItemMessage(hDlg, IDC_RESOLUTION_FULL_SCREEN_MODE, CB_SETCURSEL, maxres, 0);
			}
		}

		if( CGraphicsContext::m_FullScreenResolutions[CGraphicsContext::m_numOfResolutions - 1][0] <= windowSetting.uWindowDisplayWidth ||
			CGraphicsContext::m_FullScreenResolutions[CGraphicsContext::m_numOfResolutions - 1][1] <= windowSetting.uWindowDisplayHeight )
		{
			windowSetting.uWindowDisplayWidth = 640;
			windowSetting.uWindowDisplayHeight = 480;
		}

		SendDlgItemMessage(hDlg, IDC_RESOLUTION_WINDOW_MODE, CB_RESETCONTENT, 0, 0);
		for( i=0; i<CGraphicsContext::m_numOfResolutions; i++ )
		{

			sprintf(generalText, "%d x %d", CGraphicsContext::m_FullScreenResolutions[i][0], CGraphicsContext::m_FullScreenResolutions[i][1]);
			SendDlgItemMessage(hDlg, IDC_RESOLUTION_WINDOW_MODE, CB_INSERTSTRING, i, (LPARAM) generalText);
			if( windowSetting.uWindowDisplayWidth == CGraphicsContext::m_FullScreenResolutions[i][0] &&
				windowSetting.uWindowDisplayHeight == CGraphicsContext::m_FullScreenResolutions[i][1] )
			{
				SendDlgItemMessage(hDlg, IDC_RESOLUTION_WINDOW_MODE, CB_SETCURSEL, i, 0);
			}
		}

		SendDlgItemMessage(hDlg, IDC_FULLSCREEN_FREQUENCY, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hDlg, IDC_FULLSCREEN_FREQUENCY, CB_INSERTSTRING, 0, (LPARAM) "Default Hz");
		for( i=0; i<40; i++ )
		{ 
			if( CGraphicsContext::m_FullScreenRefreshRates[i] == 0 )
				break;

			sprintf(generalText,"%d Hz", CGraphicsContext::m_FullScreenRefreshRates[i]);
			SendDlgItemMessage(hDlg, IDC_FULLSCREEN_FREQUENCY, CB_INSERTSTRING, i+1, (LPARAM) generalText);
			if( windowSetting.uFullScreenRefreshRate == CGraphicsContext::m_FullScreenRefreshRates[i] )
			{
				SendDlgItemMessage(hDlg, IDC_FULLSCREEN_FREQUENCY, CB_SETCURSEL, i+1, 0);
			}
		}

		if( windowSetting.uFullScreenRefreshRate == 0 )
		{
			SendDlgItemMessage(hDlg, IDC_FULLSCREEN_FREQUENCY, CB_SETCURSEL, 0, 0);
		}

		SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_INSERTSTRING, 0, (LPARAM) "Stretch (Default)");
		SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_INSERTSTRING, 1, (LPARAM) "Pillarbox");
		SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_INSERTSTRING, 2, (LPARAM) "Extend");
		SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_SETCURSEL, windowSetting.uScreenScaleMode, 0);


		if( status.bGameIsRunning )
		{
			item = GetDlgItem(hDlg, IDC_RESOLUTION_WINDOW_MODE);
			EnableWindow(item, FALSE);
		}

        return(TRUE);

    case WM_DESTROY:
		if (g_hhk) UnhookWindowsHookEx (g_hhk);
		g_hhk = NULL;
		g_hwndDlg = NULL;
		return 0;

    //Propertysheet handling
	case WM_NOTIFY:
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
            {
			case PSN_APPLY:
				SendMessage(hDlg, WM_COMMAND, IDOK, lParam);
                EndDialog(lpnm->hwndFrom, TRUE);
				break;

            case PSN_RESET :
                //Handle a Cancel button click, if necessary
                EndDialog(lpnm->hwndFrom, TRUE);
				break;
			case PSN_SETACTIVE :
				if( options.bHideAdvancedOptions )
				{
					ShowItem(hDlg, IDC_FOG, FALSE);
					ShowItem(hDlg, IDC_WINFRAME_MODE, FALSE);
					ShowItem(hDlg, IDC_SKIP_FRAME, FALSE);
					ShowItem(hDlg, IDC_SSE, FALSE);
					ShowItem(hDlg, IDC_VERTEX_SHADER, FALSE);
				}
				else
				{
					ShowItem(hDlg, IDC_FOG, TRUE);
					ShowItem(hDlg, IDC_WINFRAME_MODE, TRUE);
					ShowItem(hDlg, IDC_SKIP_FRAME, TRUE);
					ShowItem(hDlg, IDC_SSE, TRUE);
					ShowItem(hDlg, IDC_VERTEX_SHADER, TRUE);
				}

				if(status.bGameIsRunning)
					DialogToStartRomIsRunning = PSH_OPTIONS;
				else
					DialogToStartRomIsNotRunning = PSH_OPTIONS;

				break;
			default:
				OnWMNotify (lParam);
				return 0;
			}
		}
		return(TRUE);

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
 		case IDC_TOOLTIP: 
			options.bDisplayTooltip = (SendDlgItemMessage(hDlg, IDC_TOOLTIP, BM_GETCHECK, 0, 0) == BST_CHECKED);
			WriteConfiguration();
			break;
		case IDC_HIDE_ADVANCED_OPTIONS:
			options.bHideAdvancedOptions = (SendDlgItemMessage(hDlg, IDC_HIDE_ADVANCED_OPTIONS, BM_GETCHECK, 0, 0) == BST_CHECKED);
			if( options.bHideAdvancedOptions )
			{
				ShowItem(hDlg, IDC_FOG, FALSE);
				ShowItem(hDlg, IDC_WINFRAME_MODE, FALSE);
				ShowItem(hDlg, IDC_SKIP_FRAME, FALSE);
				ShowItem(hDlg, IDC_SSE, FALSE);
				ShowItem(hDlg, IDC_VERTEX_SHADER, FALSE);
			}
			else
			{
				ShowItem(hDlg, IDC_FOG, TRUE);
				ShowItem(hDlg, IDC_WINFRAME_MODE, TRUE);
				ShowItem(hDlg, IDC_SKIP_FRAME, TRUE);
				ShowItem(hDlg, IDC_SSE, TRUE);
				ShowItem(hDlg, IDC_VERTEX_SHADER, TRUE);
			}
			WriteConfiguration();
			break;
		case IDOK:
			options.bEnableFog = (SendDlgItemMessage(hDlg, IDC_FOG, BM_GETCHECK, 0, 0) == BST_CHECKED);
			options.bWinFrameMode = (SendDlgItemMessage(hDlg, IDC_WINFRAME_MODE, BM_GETCHECK, 0, 0) == BST_CHECKED);
			options.bDisplayTooltip = (SendDlgItemMessage(hDlg, IDC_TOOLTIP, BM_GETCHECK, 0, 0) == BST_CHECKED);
			options.bMipMaps = (SendDlgItemMessage(hDlg, IDC_MIPMAPS, BM_GETCHECK, 0, 0) == BST_CHECKED);
			options.bHideAdvancedOptions = (SendDlgItemMessage(hDlg, IDC_HIDE_ADVANCED_OPTIONS, BM_GETCHECK, 0, 0) == BST_CHECKED);

			options.bEnableSSE = (SendDlgItemMessage(hDlg, IDC_SSE, BM_GETCHECK, 0, 0) == BST_CHECKED);
			status.isSSEEnabled = status.isSSESupported && options.bEnableSSE;
			if( status.isSSEEnabled )
			{
				ProcessVertexData = ProcessVertexDataSSE;
			}
			else
			{
				ProcessVertexData = ProcessVertexDataNoSSE;
			}

			options.bEnableVertexShader = (SendDlgItemMessage(hDlg, IDC_VERTEX_SHADER, BM_GETCHECK, 0, 0) == BST_CHECKED);
			options.bEnableVertexShader = FALSE;

			status.isVertexShaderEnabled = status.isVertexShaderSupported && options.bEnableVertexShader;

			i = SendDlgItemMessage(hDlg, IDC_FULLSCREEN_FREQUENCY, CB_GETCURSEL, 0, 0);
			if( i<=0 )
			{
				windowSetting.uFullScreenRefreshRate = 0;
			}
			else
			{
				windowSetting.uFullScreenRefreshRate = CGraphicsContext::m_FullScreenRefreshRates[i-1];
			}

			windowSetting.uScreenScaleMode = SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_GETCURSEL, 0, 0);

			i = SendDlgItemMessage(hDlg, IDC_RESOLUTION_WINDOW_MODE, CB_GETCURSEL, 0, 0);
			windowSetting.uWindowDisplayWidth = CGraphicsContext::m_FullScreenResolutions[i][0];
			windowSetting.uWindowDisplayHeight = CGraphicsContext::m_FullScreenResolutions[i][1];

			i = SendDlgItemMessage(hDlg, IDC_RESOLUTION_FULL_SCREEN_MODE, CB_GETCURSEL, 0, 0);
			windowSetting.uFullScreenDisplayWidth = CGraphicsContext::m_FullScreenResolutions[i][0];
			windowSetting.uFullScreenDisplayHeight = CGraphicsContext::m_FullScreenResolutions[i][1];
			
			windowSetting.uDisplayWidth = windowSetting.uWindowDisplayWidth;
			windowSetting.uDisplayHeight = windowSetting.uWindowDisplayHeight;

			WriteConfiguration();
			EndDialog(hDlg, TRUE);

			return(TRUE);

		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return(TRUE);
		case IDC_ABOUT:
			DllAbout ( hDlg );
			break;
	    }
    }

    return FALSE;
}
LRESULT APIENTRY DirectXDialogProc(HWND hDlg, unsigned message, LONG wParam, LONG lParam)
{
	int i;
	HWND item;
	uint32 dwPos;
	int step;

	switch(message)
	{
	case WM_INITDIALOG:
		g_hwndDlg = hDlg;
		EnumChildWndTooltip();

		{
			D3DADAPTER_IDENTIFIER9 &identifier = CDXGraphicsContext::GetAdapterInfo().d3dAdapterIdentifier;
			LONG high = identifier.DriverVersion.HighPart;
			LONG low = identifier.DriverVersion.LowPart;

			sprintf(generalText, "Adapter: %s HAL, Driver Version: %d.%d.%d.%d", identifier.Description, (high>>16), (high&0xFFFF), (low>>16), (low&0xFFFF));
			SendDlgItemMessage(hDlg, IDC_DX_ADAPTER_TEXT, WM_SETTEXT, NULL, (LPARAM)generalText );
		}

		SendDlgItemMessage(hDlg, IDC_DX_DEVICE, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hDlg, IDC_DX_DEVICE, CB_INSERTSTRING, 0, (LPARAM) (strDXDeviceDescs[0]));
		SendDlgItemMessage(hDlg, IDC_DX_DEVICE, CB_INSERTSTRING, 1, (LPARAM) (strDXDeviceDescs[1]));
		SendDlgItemMessage(hDlg, IDC_DX_DEVICE, CB_SETCURSEL, options.DirectXDevice, 0);

		SendDlgItemMessage(hDlg, IDC_SOFTWARE_TNL, BM_SETCHECK, options.bForceSoftwareTnL ? BST_CHECKED : BST_UNCHECKED, 0);

		SendDlgItemMessage(hDlg, IDC_SHOW_FPS, CB_RESETCONTENT, 0, 0);
		for( i=0; i<sizeof(OnScreenDisplaySettings)/sizeof(SettingInfo); i++ )
		{
			SendDlgItemMessage(hDlg, IDC_SHOW_FPS, CB_INSERTSTRING, i, (LPARAM) (OnScreenDisplaySettings[i].description));
			if( options.bDisplayOnscreenFPS == OnScreenDisplaySettings[i].setting )
				SendDlgItemMessage(hDlg, IDC_SHOW_FPS, CB_SETCURSEL, i, 0);
		}

		item = GetDlgItem(hDlg, IDC_SOFTWARE_TNL );
		EnableWindow(item, TRUE);

		if( status.bGameIsRunning )
		{
			item = GetDlgItem(hDlg, IDC_SOFTWARE_TNL );
			EnableWindow(item, FALSE);
		}

		item = GetDlgItem(hDlg, IDC_SLIDER_FSAA);
		SendMessage(item,TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(0, options.DirectXMaxFSAA));
		step = max(options.DirectXMaxFSAA/4, 1);
		SendMessage(item,TBM_SETPAGESIZE,0,(LPARAM) step);

		step = max(options.DirectXMaxAnisotropy/4, 1);
		item = GetDlgItem(hDlg, IDC_SLIDER_ANISO);
		SendMessage(item,TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(0, options.DirectXMaxAnisotropy));
		SendMessage(item,TBM_SETPAGESIZE,0,(LPARAM) step);

		if( options.DirectXAntiAliasingValue > options.DirectXMaxFSAA )
		{
			options.DirectXAntiAliasingValue = options.DirectXMaxFSAA;
			WriteConfiguration();
		}

#ifndef _DEBUG
		ShowItem(hDlg, IDC_DX_DEVICE, FALSE);
		ShowItem(hDlg, IDC_DX_DEVICE_TEXT, FALSE);
#endif

		sprintf(generalText,"%08X", options.FPSColor);
		item = GetDlgItem(hDlg, IDC_FPS_COLOR);
		SetWindowText(item,generalText);


		item = GetDlgItem(hDlg, IDC_ANTI_ALIASING_TEXT);
		sprintf(generalText, "Full Screen Anti-Aliasing: %d X", options.DirectXAntiAliasingValue);
		SetWindowText(item,generalText);
		item = GetDlgItem(hDlg, IDC_SLIDER_FSAA);
		SendMessage(item, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)options.DirectXAntiAliasingValue);
		if( options.DirectXMaxFSAA == 0 || status.bGameIsRunning )
		{
			EnableWindow(item, FALSE);
		}

		item = GetDlgItem(hDlg, IDC_ANTI_ALIASING_MAX_TEXT);
		sprintf(generalText, "%d X", options.DirectXMaxFSAA);
		//TRACE1("Max FSAA=%d", options.DirectXMaxFSAA);
		SetWindowText(item,generalText);

		if( options.DirectXAnisotropyValue > options.DirectXMaxAnisotropy )
		{
			options.DirectXAnisotropyValue = options.DirectXMaxAnisotropy;
			WriteConfiguration();
		}

		item = GetDlgItem(hDlg, IDC_ANISOTROPIC_TEXT);
		sprintf(generalText, "Anisotropic Filtering: %d X", options.DirectXAnisotropyValue);
		SetWindowText(item,generalText);
		item = GetDlgItem(hDlg, IDC_SLIDER_ANISO);
		SendMessage(item, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)options.DirectXAnisotropyValue);
		if( options.DirectXMaxAnisotropy == 0 || status.bGameIsRunning )
		{
			EnableWindow(item, FALSE);
		}
		item = GetDlgItem(hDlg, IDC_ANISOTROPIC_MAX_TEXT);
		sprintf(generalText, "%d X", options.DirectXMaxAnisotropy);
		SetWindowText(item,generalText);

		return(TRUE);
    
    //Propertysheet handling
	case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;

			switch (lpnm->code)
			{
			case PSN_APPLY:
				SendMessage(hDlg, WM_COMMAND, IDOK, lParam);
				EndDialog(lpnm->hwndFrom, TRUE);
				break;

			case PSN_RESET :
				//Handle a Cancel button click, if necessary
				EndDialog(lpnm->hwndFrom, TRUE);
				break;
			case PSN_SETACTIVE :
				if( options.bHideAdvancedOptions )
				{
					ShowItem(hDlg, IDC_SHOW_FPS, FALSE);
					ShowItem(hDlg, IDC_FPS_COLOR, FALSE);
					ShowItem(hDlg, IDC_SETTING_LABEL2, FALSE);
					ShowItem(hDlg, IDC_SETTING_LABEL3, FALSE);
					ShowItem(hDlg, IDC_LABEL5, FALSE);
					ShowItem(hDlg, IDC_LABEL6, FALSE);
					ShowItem(hDlg, IDC_LABEL7, FALSE);
					ShowItem(hDlg, IDC_LABEL8, FALSE);
				}
				else
				{
					ShowItem(hDlg, IDC_SHOW_FPS, TRUE);
					ShowItem(hDlg, IDC_FPS_COLOR, TRUE);
					ShowItem(hDlg, IDC_SETTING_LABEL2, TRUE);
					ShowItem(hDlg, IDC_SETTING_LABEL3, TRUE);
					ShowItem(hDlg, IDC_LABEL5, TRUE);
					ShowItem(hDlg, IDC_LABEL6, TRUE);
					ShowItem(hDlg, IDC_LABEL7, TRUE);
					ShowItem(hDlg, IDC_LABEL8, TRUE);
				}

				if(status.bGameIsRunning)
					DialogToStartRomIsRunning = PSH_DIRECTX;
				else
					DialogToStartRomIsNotRunning = PSH_DIRECTX;

				break;
			default:
				OnWMNotify (lParam);
				return 0;
			}
		}
		return(TRUE);
	case WM_HSCROLL:
		switch (LOWORD(wParam)) 
		{
		case TB_ENDTRACK: 
		case TB_THUMBTRACK: 
		case TB_PAGEDOWN: 
		case TB_PAGEUP: 
			item = GetDlgItem(hDlg, IDC_SLIDER_FSAA);
			if( lParam == (LONG)item )
			{
				dwPos = SendMessage(item, TBM_GETPOS, 0, 0);
				item = GetDlgItem(hDlg, IDC_ANTI_ALIASING_TEXT);
				sprintf(generalText, "Full Screen Anti-Aliasing: %d X", dwPos);
				SetWindowText(item,generalText);
			}
			else
			{
				item = GetDlgItem(hDlg, IDC_SLIDER_ANISO);
				if( lParam == (LONG)item )
				{
					dwPos = SendMessage(item, TBM_GETPOS, 0, 0); 
					item = GetDlgItem(hDlg, IDC_ANISOTROPIC_TEXT);
					sprintf(generalText, "Anisotropic Filtering: %d X", dwPos);
					SetWindowText(item,generalText);
				}
			}
			break; 

		default: 
			break; 

		} 

		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
        case IDOK:
			options.bForceSoftwareTnL = (SendDlgItemMessage(hDlg, IDC_SOFTWARE_TNL, BM_GETCHECK, 0, 0) == BST_CHECKED);

			i = SendDlgItemMessage(hDlg, IDC_SHOW_FPS, CB_GETCURSEL, 0, 0);
			options.bDisplayOnscreenFPS = OnScreenDisplaySettings[i].setting;

			options.DirectXDevice = SendDlgItemMessage(hDlg, IDC_DX_DEVICE, CB_GETCURSEL, 0, 0);

			item = GetDlgItem(hDlg, IDC_SLIDER_FSAA);
			options.DirectXAntiAliasingValue = SendMessage(item, TBM_GETPOS, 0, 0);
			if( options.DirectXAntiAliasingValue == 1 )
				options.DirectXAntiAliasingValue = 0;
			item = GetDlgItem(hDlg, IDC_SLIDER_ANISO);
			options.DirectXAnisotropyValue = SendMessage(item, TBM_GETPOS, 0, 0); 

			item = GetDlgItem(hDlg, IDC_FPS_COLOR);
			GetWindowText(item,generalText,30);
			if( strlen(generalText) == 8 )
			{
				char str[30];
				sprintf(str,"0x%s",generalText);
				options.FPSColor = strtoul(str,0,16);
			}

			EndDialog(hDlg, TRUE);
			WriteConfiguration();

			return(TRUE);

		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return(TRUE);
	    }
    }


    return FALSE;
	return(TRUE);
}

LRESULT APIENTRY TextureSettingDialogProc(HWND hDlg, unsigned message, LONG wParam, LONG lParam)
{
	int i;

	switch(message)
	{
	case WM_INITDIALOG:
		g_hwndDlg = hDlg;
		EnumChildWndTooltip();

		SendDlgItemMessage(hDlg, IDC_TEXTURE_ENHANCEMENT, CB_RESETCONTENT, 0, 0);
		for( i=0; i<sizeof(TextureEnhancementSettings)/sizeof(SettingInfo); i++ )
		{
			SendDlgItemMessage(hDlg, IDC_TEXTURE_ENHANCEMENT, CB_INSERTSTRING, i, (LPARAM) TextureEnhancementSettings[i].description);
			if( TextureEnhancementSettings[i].setting == options.textureEnhancement)
				SendDlgItemMessage(hDlg, IDC_TEXTURE_ENHANCEMENT, CB_SETCURSEL, i, 0);
		}

		SendDlgItemMessage(hDlg, IDC_FORCE_TEXTURE_FILTER, CB_RESETCONTENT, 0, 0);
		for( i=0; i<sizeof(ForceTextureFilterSettings)/sizeof(SettingInfo); i++ )
		{
			SendDlgItemMessage(hDlg, IDC_FORCE_TEXTURE_FILTER, CB_INSERTSTRING, i, (LPARAM) ForceTextureFilterSettings[i].description);
		}
		SendDlgItemMessage(hDlg, IDC_FORCE_TEXTURE_FILTER, CB_SETCURSEL, options.forceTextureFilter, 0);

		SendDlgItemMessage(hDlg, IDC_LOAD_HIRES_TEXTURE, BM_SETCHECK, options.bLoadHiResTextures ? BST_CHECKED : BST_UNCHECKED, 0);
		// fetch the value the user has set for caching from dialog
		SendDlgItemMessage(hDlg, IDC_CACHE_HIRES_TEXTURE, BM_SETCHECK, options.bCacheHiResTextures ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_DUMP_TEXTURE_TO_FILES, BM_SETCHECK, options.bDumpTexturesToFiles ? BST_CHECKED : BST_UNCHECKED, 0);

        return(TRUE);
    
    //Propertysheet handling
	case WM_NOTIFY:
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
            {
			case PSN_APPLY:
				SendMessage(hDlg, WM_COMMAND, IDOK, lParam);
                EndDialog(lpnm->hwndFrom, TRUE);
				break;

            case PSN_RESET :
                //Handle a Cancel button click, if necessary
                EndDialog(lpnm->hwndFrom, TRUE);
				break;
			case PSN_SETACTIVE:
				if(status.bGameIsRunning)
					DialogToStartRomIsRunning = PSH_TEXTURE;
				else
					DialogToStartRomIsNotRunning = PSH_TEXTURE;
				break;
			default:
				OnWMNotify (lParam);
				return 0;
			}
		}
		return(TRUE);

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
        case IDOK:
			i = SendDlgItemMessage(hDlg, IDC_TEXTURE_ENHANCEMENT, CB_GETCURSEL, 0, 0);
			options.textureEnhancement = TextureEnhancementSettings[i].setting;
			i = SendDlgItemMessage(hDlg, IDC_FORCE_TEXTURE_FILTER, CB_GETCURSEL, 0, 0);
			options.forceTextureFilter = ForceTextureFilterSettings[i].setting;

			{
				BOOL bLoadHiResTextures = options.bLoadHiResTextures;
				BOOL bCacheHiResTextures = options.bCacheHiResTextures;
				BOOL bDumpTexturesToFiles = options.bDumpTexturesToFiles;
				options.bLoadHiResTextures = (SendDlgItemMessage(hDlg, IDC_LOAD_HIRES_TEXTURE, BM_GETCHECK, 0, 0) == BST_CHECKED);
				options.bCacheHiResTextures = (SendDlgItemMessage(hDlg, IDC_CACHE_HIRES_TEXTURE, BM_GETCHECK, 0, 0) == BST_CHECKED);
				options.bDumpTexturesToFiles = (SendDlgItemMessage(hDlg, IDC_DUMP_TEXTURE_TO_FILES, BM_GETCHECK, 0, 0) == BST_CHECKED);

				if( status.bGameIsRunning && bLoadHiResTextures != options.bLoadHiResTextures)
				{
					void InitHiresTextures(bool bWIPFolder = false);
					void CloseHiresTextures(void);
					if( options.bLoadHiResTextures )
					{
						InitHiresTextures();
						gTextureManager.RecheckHiresForAllTextures();
					}
					else
					{
						CloseHiresTextures();
					}
				}
				// check if caching option has been changed
				if( status.bGameIsRunning && bLoadHiResTextures && bCacheHiResTextures != options.bCacheHiResTextures)
				{
					void InitHiresCache(void);
					void ClearHiresCache(void);
					if( options.bCacheHiResTextures )
					{
						// caching has been enabled
						InitHiresCache();
					}
					else
					{
						// caching has been disabled
						ClearHiresCache();
					}
				}

				if( status.bGameIsRunning && bDumpTexturesToFiles != options.bDumpTexturesToFiles)
				{
					void CloseTextureDump(void);
					void InitTextureDump(void);
					if( options.bDumpTexturesToFiles )
					{
						InitTextureDump();
					}
					else
					{
						CloseTextureDump();
					}
				}
			}

			EndDialog(hDlg, TRUE);
			WriteConfiguration();

			return(TRUE);

		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return(TRUE);
	    }
    }

    return FALSE;
}
LRESULT APIENTRY DefaultSettingDialogProc(HWND hDlg, unsigned message, LONG wParam, LONG lParam)
{
	int i;
	switch(message)
	{
	case WM_INITDIALOG:
		g_hwndDlg = hDlg;
		EnumChildWndTooltip();

		SendDlgItemMessage(hDlg, IDC_ALPHA_BLENDER, BM_SETCHECK, defaultRomOptions.bNormalBlender? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_NORMAL_COMBINER, BM_SETCHECK, defaultRomOptions.bNormalCombiner ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_TXTR_BUF_DOUBLE_SIZE, BM_SETCHECK, defaultRomOptions.bDoubleSizeForSmallTxtrBuf ? BST_CHECKED : BST_UNCHECKED, 0);

		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_RESETCONTENT, 0, 0);
		for( i=0; i<sizeof(frameBufferSettings)/sizeof(char*); i++ )
		{
			SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_INSERTSTRING, i, (LPARAM) frameBufferSettings[i]);
		}
		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_SETCURSEL, defaultRomOptions.N64FrameBufferEmuType, 0);

		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_WRITE_BACK_CONTROL, CB_RESETCONTENT, 0, 0);
		for( i=0; i<sizeof(frameBufferWriteBackControlSettings)/sizeof(char*); i++ )
		{
			SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_WRITE_BACK_CONTROL, CB_INSERTSTRING, i, (LPARAM) frameBufferWriteBackControlSettings[i]);
		}
		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_WRITE_BACK_CONTROL, CB_SETCURSEL, defaultRomOptions.N64FrameBufferWriteBackControl, 0);

		SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_RESETCONTENT, 0, 0);
		for( i=0; i<sizeof(renderToTextureSettings)/sizeof(char*); i++ )
		{
			SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_INSERTSTRING, i, (LPARAM) renderToTextureSettings[i]);
		}
		SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_SETCURSEL, defaultRomOptions.N64RenderToTextureEmuType, 0);
        return(TRUE);
    
    //Propertysheet handling
	case WM_NOTIFY:
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
            {
			case PSN_APPLY:
				SendMessage(hDlg, WM_COMMAND, IDOK, lParam);
                EndDialog(lpnm->hwndFrom, TRUE);
				break;

            case PSN_RESET :
                //Handle a Cancel button click, if necessary
                EndDialog(lpnm->hwndFrom, TRUE);
				break;
			case PSN_SETACTIVE:
				if(status.bGameIsRunning)
					DialogToStartRomIsRunning = PSH_DEFAULTS;
				else
					DialogToStartRomIsNotRunning = PSH_DEFAULTS;
				break;
			default:
				OnWMNotify (lParam);
				return 0;
			}
		}
		return(TRUE);

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			defaultRomOptions.bNormalBlender = (SendDlgItemMessage(hDlg, IDC_ALPHA_BLENDER, BM_GETCHECK, 0, 0) == BST_CHECKED);
			defaultRomOptions.bNormalCombiner = (SendDlgItemMessage(hDlg, IDC_NORMAL_COMBINER, BM_GETCHECK, 0, 0) == BST_CHECKED);
			defaultRomOptions.N64FrameBufferEmuType = SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_GETCURSEL, 0, 0);
			defaultRomOptions.N64FrameBufferWriteBackControl = SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_WRITE_BACK_CONTROL, CB_GETCURSEL, 0, 0);
			defaultRomOptions.N64RenderToTextureEmuType = SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_GETCURSEL, 0, 0);
			defaultRomOptions.bDoubleSizeForSmallTxtrBuf = (SendDlgItemMessage(hDlg, IDC_TXTR_BUF_DOUBLE_SIZE, BM_GETCHECK, 0, 0) == BST_CHECKED);

			WriteConfiguration();
			EndDialog(hDlg, TRUE);

			return(TRUE);

		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return(TRUE);
	    }
    }

	return FALSE;
}
LRESULT APIENTRY RomSettingProc(HWND hDlg, unsigned message, LONG wParam, LONG lParam)
{
	int i;
	uint32 state;

	switch(message)
	{
	case WM_INITDIALOG:
		g_hwndDlg = hDlg;
		EnumChildWndTooltip();

		// Tri-state variables
		state = g_curRomInfo.dwNormalBlender==2 ? BST_CHECKED : (g_curRomInfo.dwNormalBlender==1?BST_UNCHECKED:BST_INDETERMINATE);
		SendDlgItemMessage(hDlg, IDC_ALPHA_BLENDER, BM_SETSTYLE, BS_AUTO3STATE, TRUE);
		SendDlgItemMessage(hDlg, IDC_ALPHA_BLENDER, BM_SETCHECK, state, 0);

		state = g_curRomInfo.dwNormalCombiner ==2 ? BST_CHECKED : (g_curRomInfo.dwNormalCombiner ==1?BST_UNCHECKED:BST_INDETERMINATE);
		SendDlgItemMessage(hDlg, IDC_NORMAL_COMBINER, BM_SETSTYLE, BS_AUTO3STATE, TRUE);
		SendDlgItemMessage(hDlg, IDC_NORMAL_COMBINER, BM_SETCHECK, state, 0);

		// Normal bi-state variable
		SendDlgItemMessage(hDlg, IDC_DISABLE_BLENDER, BM_SETCHECK, g_curRomInfo.bDisableBlender?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_FORCE_DEPTH_COMPARE, BM_SETCHECK, g_curRomInfo.bForceDepthBuffer?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_FORCE_BUFFER_CLEAR, BM_SETCHECK, g_curRomInfo.bForceScreenClear?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_EMULATE_CLEAR, BM_SETCHECK, g_curRomInfo.bEmulateClear?BST_CHECKED:BST_UNCHECKED, 0);

		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_INSERTSTRING, 0, (LPARAM) "Default");
		for( i=0; i<sizeof(frameBufferSettings)/sizeof(char*); i++ )
		{
			SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_INSERTSTRING, i+1, (LPARAM) frameBufferSettings[i]);
		}
		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_SETCURSEL, g_curRomInfo.dwFrameBufferOption, 0);
		
		SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_INSERTSTRING, 0, (LPARAM) "Default");
		for( i=0; i<sizeof(renderToTextureSettings)/sizeof(char*); i++ )
		{
			SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_INSERTSTRING, i+1, (LPARAM) renderToTextureSettings[i]);
		}
		SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_SETCURSEL, g_curRomInfo.dwRenderToTextureOption, 0);

		SendDlgItemMessage(hDlg, IDC_SCREEN_UPDATE_AT, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hDlg, IDC_SCREEN_UPDATE_AT, CB_INSERTSTRING, 0, (LPARAM) "Default");
		for( i=0; i<sizeof(screenUpdateSettings)/sizeof(char*); i++ )
		{
			SendDlgItemMessage(hDlg, IDC_SCREEN_UPDATE_AT, CB_INSERTSTRING, i+1, (LPARAM) screenUpdateSettings[i]);
		}
		SendDlgItemMessage(hDlg, IDC_SCREEN_UPDATE_AT, CB_SETCURSEL, g_curRomInfo.dwScreenUpdateSetting, 0);

		SendDlgItemMessage(hDlg, IDC_USE_CI_WIDTH_AND_RATIO, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hDlg, IDC_USE_CI_WIDTH_AND_RATIO, CB_INSERTSTRING, 0, (LPARAM) "No");
		SendDlgItemMessage(hDlg, IDC_USE_CI_WIDTH_AND_RATIO, CB_INSERTSTRING, 1, (LPARAM) "NTSC");
		SendDlgItemMessage(hDlg, IDC_USE_CI_WIDTH_AND_RATIO, CB_INSERTSTRING, 2, (LPARAM) "PAL");
		SendDlgItemMessage(hDlg, IDC_USE_CI_WIDTH_AND_RATIO, CB_SETCURSEL, g_curRomInfo.UseCIWidthAndRatio, 0);

		if( status.bGameIsRunning )
		{
			ShowItem(hDlg, IDC_SCREEN_UPDATE_AT, TRUE);
			ShowItem(hDlg, IDC_FORCE_DEPTH_COMPARE, TRUE);
			ShowItem(hDlg, IDC_DISABLE_BLENDER, TRUE);
			ShowItem(hDlg, IDC_FORCE_BUFFER_CLEAR, TRUE);
		}

		// Less useful options
		SendDlgItemMessage(hDlg, IDC_DISABLE_BG, BM_SETCHECK, g_curRomInfo.bDisableObjBG?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_INCREASE_TEXTRECT_EDGE, BM_SETCHECK,	g_curRomInfo.bIncTexRectEdge?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_Z_HACK, BM_SETCHECK,	g_curRomInfo.bZHack?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_TEXTURE_SCALE_HACK, BM_SETCHECK,		g_curRomInfo.bTextureScaleHack?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_FASTER_LOADING_TILE, BM_SETCHECK,		g_curRomInfo.bFastLoadTile?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_PRIMARY_DEPTH_HACK, BM_SETCHECK,		g_curRomInfo.bPrimaryDepthHack?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_TEXTURE_1_HACK, BM_SETCHECK,			g_curRomInfo.bTexture1Hack?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_DISABLE_CULLING, BM_SETCHECK,			g_curRomInfo.bDisableCulling?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_TXT_SIZE_METHOD_2, BM_SETCHECK,	g_curRomInfo.bTxtSizeMethod2?BST_CHECKED:BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_ENABLE_LOD, BM_SETCHECK,	g_curRomInfo.bEnableTxtLOD?BST_CHECKED:BST_UNCHECKED, 0);

		if( g_curRomInfo.VIWidth > 0 )
		{
			sprintf(generalText,"%d",g_curRomInfo.VIWidth);
			SetDlgItemText(hDlg,IDC_EDIT_WIDTH,generalText);
		}
		if( g_curRomInfo.VIHeight > 0 )
		{
			sprintf(generalText,"%d",g_curRomInfo.VIHeight);
			SetDlgItemText(hDlg,IDC_EDIT_HEIGHT,generalText);
		}

        return(TRUE);
    
    //Propertysheet handling
	case WM_NOTIFY:
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
            {
			case PSN_APPLY:
				SendMessage(hDlg, WM_COMMAND, IDOK, lParam);
                EndDialog(lpnm->hwndFrom, TRUE);
				break;

            case PSN_RESET :
                //Handle a Cancel button click, if necessary
                EndDialog(lpnm->hwndFrom, TRUE);
				break;
			case PSN_SETACTIVE:
				if(status.bGameIsRunning)
					DialogToStartRomIsRunning = PSH_ROM_SETTINGS;
				break;
			default:
				OnWMNotify (lParam);
				return 0;
			}
		}
		return(TRUE);

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
        case IDOK:
			// Tri-stage options
			uint32 state;
			state = SendDlgItemMessage(hDlg, IDC_ALPHA_BLENDER, BM_GETCHECK, 0, 0);
			g_curRomInfo.dwNormalBlender = (state==BST_CHECKED?2:(state==BST_UNCHECKED?1:0));
			
			state = SendDlgItemMessage(hDlg, IDC_NORMAL_COMBINER, BM_GETCHECK, 0, 0);
			g_curRomInfo.dwNormalCombiner = (state==BST_CHECKED?2:(state==BST_UNCHECKED?1:0));

			// Bi-state options
			g_curRomInfo.bDisableBlender = (SendDlgItemMessage(hDlg, IDC_DISABLE_BLENDER, BM_GETCHECK, 0, 0)==BST_CHECKED);
			g_curRomInfo.bEmulateClear = (SendDlgItemMessage(hDlg, IDC_EMULATE_CLEAR, BM_GETCHECK, 0, 0)==BST_CHECKED);
			g_curRomInfo.bForceDepthBuffer = (SendDlgItemMessage(hDlg, IDC_FORCE_DEPTH_COMPARE, BM_GETCHECK, 0, 0)==BST_CHECKED);
			g_curRomInfo.bForceScreenClear = (SendDlgItemMessage(hDlg, IDC_FORCE_BUFFER_CLEAR, BM_GETCHECK, 0, 0)==BST_CHECKED);

			g_curRomInfo.dwFrameBufferOption = SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_GETCURSEL, 0, 0);
			g_curRomInfo.dwRenderToTextureOption = SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_GETCURSEL, 0, 0);
			g_curRomInfo.dwScreenUpdateSetting = SendDlgItemMessage(hDlg, IDC_SCREEN_UPDATE_AT, CB_GETCURSEL, 0, 0);
			g_curRomInfo.UseCIWidthAndRatio = SendDlgItemMessage(hDlg, IDC_USE_CI_WIDTH_AND_RATIO, CB_GETCURSEL, 0, 0);

			// Less useful variables
			g_curRomInfo.bDisableObjBG		= (SendDlgItemMessage(hDlg, IDC_DISABLE_BG, BM_GETCHECK, 0, 0)==BST_CHECKED);
			g_curRomInfo.bIncTexRectEdge	= (SendDlgItemMessage(hDlg, IDC_INCREASE_TEXTRECT_EDGE, BM_GETCHECK, 0, 0) == BST_CHECKED);
			g_curRomInfo.bZHack	= (SendDlgItemMessage(hDlg, IDC_Z_HACK, BM_GETCHECK, 0, 0) == BST_CHECKED);
			g_curRomInfo.bTextureScaleHack	= (SendDlgItemMessage(hDlg, IDC_TEXTURE_SCALE_HACK, BM_GETCHECK, 0, 0) == BST_CHECKED);
			g_curRomInfo.bFastLoadTile		= (SendDlgItemMessage(hDlg, IDC_FASTER_LOADING_TILE, BM_GETCHECK, 0, 0) == BST_CHECKED);
			g_curRomInfo.bPrimaryDepthHack	= (SendDlgItemMessage(hDlg, IDC_PRIMARY_DEPTH_HACK, BM_GETCHECK, 0, 0) == BST_CHECKED);
			g_curRomInfo.bTexture1Hack		= (SendDlgItemMessage(hDlg, IDC_TEXTURE_1_HACK, BM_GETCHECK, 0, 0) == BST_CHECKED);
			g_curRomInfo.bDisableCulling	= (SendDlgItemMessage(hDlg, IDC_DISABLE_CULLING, BM_GETCHECK, 0, 0) == BST_CHECKED);
			g_curRomInfo.bTxtSizeMethod2	= (SendDlgItemMessage(hDlg, IDC_TXT_SIZE_METHOD_2, BM_GETCHECK, 0, 0) == BST_CHECKED);
			g_curRomInfo.bEnableTxtLOD		= (SendDlgItemMessage(hDlg, IDC_ENABLE_LOD, BM_GETCHECK, 0, 0) == BST_CHECKED);

			GetDlgItemText(hDlg,IDC_EDIT_WIDTH,generalText,255);
			if( atol(generalText) > 100 )
			{
				g_curRomInfo.VIWidth = atol(generalText);
			}
			else
			{
				g_curRomInfo.VIWidth = 0;
			}
			

			GetDlgItemText(hDlg,IDC_EDIT_HEIGHT,generalText,255);
			if( atol(generalText) > 100 )
			{
				g_curRomInfo.VIHeight = atol(generalText);
			}
			else
			{
				g_curRomInfo.VIHeight = 0;
			}
					 
			GenerateCurrentRomOptions();
			Ini_StoreRomOptions(&g_curRomInfo);

			EndDialog(hDlg, TRUE);

			return(TRUE);

		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return(TRUE);
	    }
    }

    return FALSE;
}
LRESULT APIENTRY UnavailableProc(HWND hDlg, unsigned message, LONG wParam, LONG lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
        return(TRUE);
    
    //Propertysheet handling
	case WM_NOTIFY:
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
            {
			case PSN_APPLY:
				SendMessage(hDlg, WM_COMMAND, IDOK, lParam);
                EndDialog(lpnm->hwndFrom, TRUE);
				break;

            case PSN_RESET :
                //Handle a Cancel button click, if necessary
                EndDialog(lpnm->hwndFrom, TRUE);
				break;
			}
		}
		return(TRUE);

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
        case IDOK:
			EndDialog(hDlg, TRUE);
			return(TRUE);

		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return(TRUE);
	    }
    }

    return FALSE;
}

//Test: Creating property pages for all options
void CreateOptionsDialogs(HWND hParent)
{
#ifdef ENABLE_CONFIG_DIALOG
	PROPSHEETPAGE	psp[6]; //Change this array size if you change the number of pages.
	PROPSHEETHEADER psh;
	memset(&psp,0,sizeof(PROPSHEETPAGE)*6);
	memset(&psh,0,sizeof(PROPSHEETHEADER));

	psp[PSH_OPTIONS].dwSize			= sizeof(PROPSHEETPAGE);
	psp[PSH_OPTIONS].dwFlags		= PSP_USETITLE;
	psp[PSH_OPTIONS].hInstance		= windowSetting.myhInst;
	psp[PSH_OPTIONS].pszTemplate	= "OPTIONS";
	psp[PSH_OPTIONS].pszIcon		= NULL;
	psp[PSH_OPTIONS].pfnDlgProc		= (DLGPROC)OptionsDialogProc;
	psp[PSH_OPTIONS].pszTitle		= "General Options";
	psp[PSH_OPTIONS].lParam			= 0;

	psp[PSH_DIRECTX].dwSize			= sizeof(PROPSHEETPAGE);
	psp[PSH_DIRECTX].dwFlags		= PSP_USETITLE;
	psp[PSH_DIRECTX].hInstance		= windowSetting.myhInst;
	psp[PSH_DIRECTX].pszTemplate	= "DIRECTX";
	psp[PSH_DIRECTX].pszIcon		= NULL;
	psp[PSH_DIRECTX].pfnDlgProc		= (DLGPROC)DirectXDialogProc;
	psp[PSH_DIRECTX].pszTitle		= "DirectX";
	psp[PSH_DIRECTX].lParam			= 0;

	psp[PSH_TEXTURE].dwSize			= sizeof(PROPSHEETPAGE);
	psp[PSH_TEXTURE].dwFlags		= PSP_USETITLE;
	psp[PSH_TEXTURE].hInstance		= windowSetting.myhInst;
	psp[PSH_TEXTURE].pszIcon		= NULL;
	psp[PSH_TEXTURE].pszTemplate	= "TEXTURES";
    psp[PSH_TEXTURE].pfnDlgProc		= (DLGPROC)TextureSettingDialogProc;
	psp[PSH_TEXTURE].pszTitle		= "Texture Enhancement";
	psp[PSH_TEXTURE].lParam			= 0;

	if( options.bHideAdvancedOptions )
	{
		psp[PSH_DEFAULTS].dwSize		= sizeof(PROPSHEETPAGE);
		psp[PSH_DEFAULTS].dwFlags		= PSP_USETITLE;
		psp[PSH_DEFAULTS].hInstance		= windowSetting.myhInst;
		psp[PSH_DEFAULTS].pszTemplate	= "HIDDEN";
		psp[PSH_DEFAULTS].pszIcon		= NULL;
		psp[PSH_DEFAULTS].pfnDlgProc	= (DLGPROC)UnavailableProc;
		psp[PSH_DEFAULTS].pszTitle		= "Game Default Options";
		psp[PSH_DEFAULTS].lParam		= 0;
	}
	else
	{
		psp[PSH_DEFAULTS].dwSize		= sizeof(PROPSHEETPAGE);
		psp[PSH_DEFAULTS].dwFlags		= PSP_USETITLE;
		psp[PSH_DEFAULTS].hInstance		= windowSetting.myhInst;
		psp[PSH_DEFAULTS].pszTemplate	= "DEFAULTS";
		psp[PSH_DEFAULTS].pszIcon		= NULL;
		psp[PSH_DEFAULTS].pfnDlgProc	= (DLGPROC)DefaultSettingDialogProc;
		psp[PSH_DEFAULTS].pszTitle		= "Game Default Options";
		psp[PSH_DEFAULTS].lParam		= 0;
	}

	if(options.bHideAdvancedOptions)
	{
		psp[PSH_ROM_SETTINGS].pszTemplate = "HIDDEN";
		psp[PSH_ROM_SETTINGS].pfnDlgProc = (DLGPROC) UnavailableProc;
	}
	else if (status.bGameIsRunning )
	{
		psp[PSH_ROM_SETTINGS].pszTemplate = "ROM_SETTINGS";
		psp[PSH_ROM_SETTINGS].pfnDlgProc = (DLGPROC) RomSettingProc;
	}
	else
	{
		psp[PSH_ROM_SETTINGS].pszTemplate = "NOT_AVAILABLE";
		psp[PSH_ROM_SETTINGS].pfnDlgProc = (DLGPROC) UnavailableProc;
	}
	psp[PSH_ROM_SETTINGS].dwSize		= sizeof(PROPSHEETPAGE);
	psp[PSH_ROM_SETTINGS].dwFlags		= PSP_USETITLE;
	psp[PSH_ROM_SETTINGS].hInstance		= windowSetting.myhInst;
	psp[PSH_ROM_SETTINGS].pszIcon		= NULL;
	psp[PSH_ROM_SETTINGS].pszTitle		= "Current Game Options";
	psp[PSH_ROM_SETTINGS].lParam		= 0;

	sprintf(generalText, "%s %s %s Configuration",project_name, BUILD_NUMBER, FILE_VERSION);

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;//| PSH_USEICONID ;
	psh.hwndParent = hParent;
	psh.hInstance = windowSetting.myhInst;
	//psh.pszIcon = MAKEINTRESOURCE(IDI_ICON2);
	psh.pszCaption = (LPSTR) generalText;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.ppsp = (LPCPROPSHEETPAGE) & psp;
	if(status.bGameIsRunning)
	{
		psh.nStartPage = DialogToStartRomIsRunning;
	}
	else
	{
		psh.nStartPage = DialogToStartRomIsNotRunning;
	}

	// Create tooltip handle
	if (!g_hhk)
	{
		g_hwndTT = NULL;
		g_hhk = NULL;
		CreateDialogTooltip();
	}

	hPropSheetHwnd = (HWND) PropertySheet(&psh);	// Start the Property Sheet

	//if (g_hhk) 
	//	UnhookWindowsHookEx (g_hhk);

	//g_hhk = NULL;
	//g_hwndDlg = NULL;
#endif
}

