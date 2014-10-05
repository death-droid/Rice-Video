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
#include "Texture\TextureFilters\TextureFilters.h"

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
	"N64 Default Texture Filter",					FORCE_DEFAULT_FILTER,
	"Force Nearest Filter (faster, low quality)",	FORCE_POINT_FILTER,
	"Force Linear Filter (slower, better quality)",	FORCE_LINEAR_FILTER,
};

const SettingInfo TextureEnhancementSettings[] =
{
	"N64 original texture (No enhancement)",	TEXTURE_NO_ENHANCEMENT,
	"2xSaI",									TEXTURE_2XSAI_ENHANCEMENT,
	"HQ2x",										TEXTURE_HQ2X_ENHANCEMENT,
	"HQ2xS",									TEXTURE_HQ2XS_ENHANCEMENT,
};

void WriteConfiguration(void);
void GenerateCurrentRomOptions();

int DialogToStartRomIsRunning = PSH_ROM_SETTINGS;
int DialogToStartRomIsNotRunning = PSH_OPTIONS;
HWND hPropSheetHwnd = NULL;

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

	frameBufferOptions.bSupportRenderTextures	= false;
	frameBufferOptions.bCheckRenderTextures		= false;
	frameBufferOptions.bRenderTextureWriteBack	= false;
	frameBufferOptions.bLoadRDRAMIntoRenderTexture = false;

	frameBufferOptions.bProcessCPUWrite			= false;
	frameBufferOptions.bProcessCPURead			= false;
	frameBufferOptions.bAtEachFrameUpdate		= false;
	frameBufferOptions.bIgnoreRenderTextureIfHeightUnknown = false;

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
	ini.SetLongValue("WindowSetting", "ScreenScaleMode", (uint32)windowSetting.uScreenScaleMode);

	//Now rendering modes
	ini.SetLongValue("RenderSetting", "DirectXAntiAliasingValue", (uint32)options.DirectXAntiAliasingValue);
	ini.SetLongValue("RenderSetting", "DirectXAnisotropyValue", (uint32)options.DirectXAnisotropyValue);
	ini.SetLongValue("RenderSetting", "NormalAlphaBlender", defaultRomOptions.bNormalBlender);
	ini.SetLongValue("RenderSetting", "EnableFog", options.bEnableFog);
	ini.SetLongValue("RenderSetting", "WinFrameMode", options.bWinFrameMode);
	ini.SetLongValue("RenderSetting", "MipMaps", options.bMipMaps);

	//Now texture settings
	ini.SetLongValue("Texture Settings", "CacheHiResTextures" , (uint32)options.bCacheHiResTextures);
	ini.SetLongValue("Texture Settings", "ForceTextureFilter", (uint32)options.forceTextureFilter);
	ini.SetLongValue("Texture Settings", "LoadHiResTextures", (uint32)options.bLoadHiResTextures);
	ini.SetLongValue("Texture Settings", "DumpTexturesToFiles", (uint32)options.bDumpTexturesToFiles);
	ini.SetLongValue("Texture Settings", "TextureEnhancement", (uint32)options.textureEnhancement);
	ini.SetLongValue("Texture Settings", "TextureEnhancementControl", (uint32)options.textureEnhancementControl);

	//Now framebuffer Settings
	ini.SetLongValue("FrameBufferSettings", "FrameBufferType", defaultRomOptions.N64FrameBufferEmuType);
	ini.SetLongValue("FrameBufferSettings", "FrameBufferWriteBackControl", defaultRomOptions.N64FrameBufferWriteBackControl);
	ini.SetLongValue("FrameBufferSettings", "RenderToTexture", defaultRomOptions.N64RenderToTextureEmuType);
	
	//Now just some misc settings
	ini.SetLongValue("MiscSettings", "EnableHacks", options.bEnableHacks);
	ini.SetLongValue("MiscSettings", "ScreenUpdateSetting", defaultRomOptions.screenUpdateSetting);
	//Ok lets save the settings
	ini.SaveFile(name);
	ini.Reset();
}

void ReadConfiguration(void)
{
	char name[1024];
	GetPluginDir(name);
	strcat(name, CONFIG_FILE);

	options.bEnableHacks = TRUE;

	defaultRomOptions.screenUpdateSetting = SCREEN_UPDATE_AT_VI_CHANGE;
	//defaultRomOptions.screenUpdateSetting = SCREEN_UPDATE_AT_VI_UPDATE_AND_DRAWN;

	defaultRomOptions.N64FrameBufferEmuType = FRM_BUF_NONE;
	defaultRomOptions.N64FrameBufferWriteBackControl = FRM_BUF_WRITEBACK_NORMAL;
	defaultRomOptions.N64RenderToTextureEmuType = TXT_BUF_NONE;
	FILE *f = fopen(name, "rb");
	if(!f)
	{
		options.bEnableFog = TRUE;
		options.bWinFrameMode = FALSE;
		options.bMipMaps = TRUE;
		options.forceTextureFilter = 0;
		options.bLoadHiResTextures = FALSE;
		// set caching by default to "off"
		options.bCacheHiResTextures = FALSE;
		options.bDumpTexturesToFiles = FALSE;
		options.textureEnhancement = 0;
		options.textureEnhancementControl = 0;
		options.DirectXAntiAliasingValue = 0;
		options.DirectXAnisotropyValue = 0;

		defaultRomOptions.N64FrameBufferEmuType = FRM_BUF_NONE;
		defaultRomOptions.N64FrameBufferWriteBackControl = FRM_BUF_WRITEBACK_NORMAL;
		defaultRomOptions.N64RenderToTextureEmuType = TXT_BUF_NONE;

		defaultRomOptions.bNormalBlender = FALSE;
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

		options.DirectXAntiAliasingValue = ini.GetLongValue("RenderSetting", "DirectXAntiAliasingValue");
		options.DirectXAnisotropyValue = ini.GetLongValue("RenderSetting", "DirectXAnisotropyValue");
		defaultRomOptions.bNormalBlender = ini.GetBoolValue("RenderSetting", "NormalAlphaBlender");
		options.bEnableFog = ini.GetBoolValue("RenderSetting", "EnableFog");
		options.bWinFrameMode = ini.GetBoolValue("RenderSetting", "WinFrameMode");
		options.bMipMaps = ini.GetBoolValue("RenderSetting", "MipMaps");

		ini.Reset();
	}
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
	currentRomOptions.bNormalBlender			=g_curRomInfo.dwNormalBlender;

	options.enableHackForGames = NO_HACK_FOR_GAME;

	//URGHK Wish there was some way to clean all of this up
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

///////////////////////////////////////
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

	switch(message)
	{
	case WM_INITDIALOG:
		//General config op
		SendDlgItemMessage(hDlg, IDC_FOG,		    BM_SETCHECK, options.bEnableFog	   ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_WINFRAME_MODE, BM_SETCHECK, options.bWinFrameMode ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_ALPHA_BLENDER, BM_SETCHECK, defaultRomOptions.bNormalBlender ? BST_CHECKED : BST_UNCHECKED, 0);

		//--------------------------------------------------------------
		// Begin Resolution handling code
		//--------------------------------------------------------------
		SendDlgItemMessage(hDlg, IDC_RESOLUTION_FULL_SCREEN_MODE, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hDlg, IDC_RESOLUTION_WINDOW_MODE,	  CB_RESETCONTENT, 0, 0);
		for(int maxres=0; maxres<CGraphicsContext::m_numOfResolutions; maxres++ ) // Really need to rethink this -CLEAN ME -FIX ME
		{
			sprintf(generalText, "%d x %d", CGraphicsContext::m_FullScreenResolutions[maxres][0], CGraphicsContext::m_FullScreenResolutions[maxres][1]);
			SendDlgItemMessage(hDlg, IDC_RESOLUTION_FULL_SCREEN_MODE, CB_INSERTSTRING, maxres, (LPARAM) generalText);
			SendDlgItemMessage(hDlg, IDC_RESOLUTION_WINDOW_MODE,	  CB_INSERTSTRING, maxres, (LPARAM) generalText);

			if( windowSetting.uFullScreenDisplayWidth  == CGraphicsContext::m_FullScreenResolutions[maxres][0] &&
				windowSetting.uFullScreenDisplayHeight == CGraphicsContext::m_FullScreenResolutions[maxres][1] )
			{
				SendDlgItemMessage(hDlg, IDC_RESOLUTION_FULL_SCREEN_MODE, CB_SETCURSEL, maxres, 0);
			}

			if ( windowSetting.uWindowDisplayWidth  == CGraphicsContext::m_FullScreenResolutions[maxres][0] &&
				 windowSetting.uWindowDisplayHeight == CGraphicsContext::m_FullScreenResolutions[maxres][1])
			{
				SendDlgItemMessage(hDlg, IDC_RESOLUTION_WINDOW_MODE, CB_SETCURSEL, maxres, 0);
			}
		}

		SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_RESETCONTENT, 0, 0);
		SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_INSERTSTRING, 0, (LPARAM) "Stretch (Default)");
		SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_INSERTSTRING, 1, (LPARAM) "Pillarbox");
		SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_INSERTSTRING, 2, (LPARAM) "Extend");
		SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_SETCURSEL, windowSetting.uScreenScaleMode, 0);
		//--------------------------------------------------------------
		// End resolution handling code
		//--------------------------------------------------------------

		//--------------------------------------------------------------
		//	Begin framebuffer options
		//--------------------------------------------------------------
		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_RESETCONTENT, 0, 0);
		for (i = 0; i<sizeof(frameBufferSettings) / sizeof(char*); i++)
		{
			SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_INSERTSTRING, i, (LPARAM)frameBufferSettings[i]);
		}
		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_SETCURSEL, defaultRomOptions.N64FrameBufferEmuType, 0);

		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_WRITE_BACK_CONTROL, CB_RESETCONTENT, 0, 0);
		for (i = 0; i<sizeof(frameBufferWriteBackControlSettings) / sizeof(char*); i++)
		{
			SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_WRITE_BACK_CONTROL, CB_INSERTSTRING, i, (LPARAM)frameBufferWriteBackControlSettings[i]);
		}
		SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_WRITE_BACK_CONTROL, CB_SETCURSEL, defaultRomOptions.N64FrameBufferWriteBackControl, 0);

		SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_RESETCONTENT, 0, 0);
		for (i = 0; i<sizeof(renderToTextureSettings) / sizeof(char*); i++)
		{
			SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_INSERTSTRING, i, (LPARAM)renderToTextureSettings[i]);
		}
		SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_SETCURSEL, defaultRomOptions.N64RenderToTextureEmuType, 0);
		//--------------------------------------------------------------
		//	End framebuffer options
		//--------------------------------------------------------------

		//--------------------------------------------------------------
		// Begin texture enhancement code
		//--------------------------------------------------------------
		SendDlgItemMessage(hDlg, IDC_TEXTURE_ENHANCEMENT, CB_RESETCONTENT, 0, 0);
		for (i = 0; i<sizeof(TextureEnhancementSettings) / sizeof(SettingInfo); i++)
		{
			SendDlgItemMessage(hDlg, IDC_TEXTURE_ENHANCEMENT, CB_INSERTSTRING, i, (LPARAM)TextureEnhancementSettings[i].description);				
		}

		SendDlgItemMessage(hDlg, IDC_FORCE_TEXTURE_FILTER, CB_RESETCONTENT, 0, 0);
		for (i = 0; i<sizeof(ForceTextureFilterSettings) / sizeof(SettingInfo); i++)
		{
			SendDlgItemMessage(hDlg, IDC_FORCE_TEXTURE_FILTER, CB_INSERTSTRING, i, (LPARAM)ForceTextureFilterSettings[i].description);
		}

		SendDlgItemMessage(hDlg, IDC_TEXTURE_ENHANCEMENT,   CB_SETCURSEL, options.textureEnhancement, 0);
		SendDlgItemMessage(hDlg, IDC_FORCE_TEXTURE_FILTER,  CB_SETCURSEL, options.forceTextureFilter, 0);
		SendDlgItemMessage(hDlg, IDC_MIPMAPS,				BM_SETCHECK,  options.bMipMaps			   ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOAD_HIRES_TEXTURE,    BM_SETCHECK,  options.bLoadHiResTextures   ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_CACHE_HIRES_TEXTURE,   BM_SETCHECK,  options.bCacheHiResTextures  ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_DUMP_TEXTURE_TO_FILES, BM_SETCHECK,  options.bDumpTexturesToFiles ? BST_CHECKED : BST_UNCHECKED, 0);
		//--------------------------------------------------------------
		// End texture enhancement code
		//--------------------------------------------------------------

		//--------------------------------------------------------------
		// Start of Direct X related settings
		//--------------------------------------------------------------
		SendDlgItemMessage(hDlg, IDC_SLIDER_FSAA, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 8));
		SendDlgItemMessage(hDlg, IDC_SLIDER_FSAA, TBM_SETPAGESIZE, 0, (LPARAM) 4);

		SendDlgItemMessage(hDlg, IDC_SLIDER_ANISO, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 16));
		SendDlgItemMessage(hDlg, IDC_SLIDER_ANISO, TBM_SETPAGESIZE, 0, (LPARAM)4);

		sprintf(generalText, "Full Screen Anti-Aliasing: %d X", options.DirectXAntiAliasingValue);
		SetWindowText(GetDlgItem(hDlg, IDC_ANTI_ALIASING_TEXT), generalText);

		sprintf(generalText, "Anisotropic Filtering: %d X", options.DirectXAnisotropyValue);
		SetWindowText(GetDlgItem(hDlg, IDC_ANISOTROPIC_TEXT), generalText);

		SendDlgItemMessage(hDlg, IDC_SLIDER_FSAA, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)options.DirectXAntiAliasingValue);
		SendDlgItemMessage(hDlg, IDC_SLIDER_ANISO, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)options.DirectXAnisotropyValue);
		//--------------------------------------------------------------
		// End of Direct X related settings
		//--------------------------------------------------------------

		if( status.bGameIsRunning )
		{
			EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_WINDOW_MODE), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_FSAA), FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_SLIDER_ANISO), FALSE);
		}
        return(TRUE);


	case WM_HSCROLL:
		switch (LOWORD(wParam))
		{
		case TB_ENDTRACK:
		case TB_THUMBTRACK:
		case TB_PAGEDOWN:
		case TB_PAGEUP:
			if (lParam == (LONG)GetDlgItem(hDlg, IDC_SLIDER_FSAA))
			{
				sprintf(generalText, "Full Screen Anti-Aliasing: %d X", SendMessage(GetDlgItem(hDlg, IDC_SLIDER_FSAA), TBM_GETPOS, 0, 0));
				SetWindowText(GetDlgItem(hDlg, IDC_ANTI_ALIASING_TEXT), generalText);
			}
			else if (lParam == (LONG)GetDlgItem(hDlg, IDC_SLIDER_ANISO))
			{
				sprintf(generalText, "Anisotropic Filtering: %d X", SendMessage(GetDlgItem(hDlg, IDC_SLIDER_ANISO), TBM_GETPOS, 0, 0));
				SetWindowText(GetDlgItem(hDlg, IDC_ANISOTROPIC_TEXT), generalText);
			}
			break;
		}

		break;

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

				if(status.bGameIsRunning)
					DialogToStartRomIsRunning = PSH_OPTIONS;
				else
					DialogToStartRomIsNotRunning = PSH_OPTIONS;

				break;
			default:
				return 0;
			}
		}
		return(TRUE);

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			options.bEnableFog = (SendDlgItemMessage(hDlg, IDC_FOG, BM_GETCHECK, 0, 0) == BST_CHECKED);
			options.bWinFrameMode = (SendDlgItemMessage(hDlg, IDC_WINFRAME_MODE, BM_GETCHECK, 0, 0) == BST_CHECKED);
			defaultRomOptions.bNormalBlender = (SendDlgItemMessage(hDlg, IDC_ALPHA_BLENDER, BM_GETCHECK, 0, 0) == BST_CHECKED);
			
			//Begin Resolutioon Handling
			windowSetting.uScreenScaleMode = SendDlgItemMessage(hDlg, IDC_SCALE_MODE, CB_GETCURSEL, 0, 0);

			i = SendDlgItemMessage(hDlg, IDC_RESOLUTION_WINDOW_MODE, CB_GETCURSEL, 0, 0);
			windowSetting.uWindowDisplayWidth = CGraphicsContext::m_FullScreenResolutions[i][0];
			windowSetting.uWindowDisplayHeight = CGraphicsContext::m_FullScreenResolutions[i][1];

			i = SendDlgItemMessage(hDlg, IDC_RESOLUTION_FULL_SCREEN_MODE, CB_GETCURSEL, 0, 0);
			windowSetting.uFullScreenDisplayWidth = CGraphicsContext::m_FullScreenResolutions[i][0];
			windowSetting.uFullScreenDisplayHeight = CGraphicsContext::m_FullScreenResolutions[i][1];

			windowSetting.uDisplayWidth = windowSetting.uWindowDisplayWidth;
			windowSetting.uDisplayHeight = windowSetting.uWindowDisplayHeight;
			//End Resolution Handling

			options.DirectXAntiAliasingValue = SendDlgItemMessage(hDlg, IDC_SLIDER_FSAA, TBM_GETPOS, 0, 0);
			if (options.DirectXAntiAliasingValue == 1)
				options.DirectXAntiAliasingValue = 0;
			options.DirectXAnisotropyValue = SendDlgItemMessage(hDlg, IDC_SLIDER_ANISO, TBM_GETPOS, 0, 0);

			//--------------------------------------------------------------
			// Begin texture enhancement code
			//--------------------------------------------------------------
			
			options.textureEnhancement = TextureEnhancementSettings[SendDlgItemMessage(hDlg, IDC_TEXTURE_ENHANCEMENT, CB_GETCURSEL, 0, 0)].setting;
			options.forceTextureFilter = ForceTextureFilterSettings[SendDlgItemMessage(hDlg, IDC_FORCE_TEXTURE_FILTER, CB_GETCURSEL, 0, 0)].setting;
			options.bMipMaps			 = (SendDlgItemMessage(hDlg, IDC_MIPMAPS, BM_GETCHECK, 0, 0) == BST_CHECKED);
			options.bLoadHiResTextures   = (SendDlgItemMessage(hDlg, IDC_LOAD_HIRES_TEXTURE,    BM_GETCHECK, 0, 0) == BST_CHECKED);
			options.bDumpTexturesToFiles = (SendDlgItemMessage(hDlg, IDC_DUMP_TEXTURE_TO_FILES, BM_GETCHECK, 0, 0) == BST_CHECKED);
			options.bCacheHiResTextures  = (SendDlgItemMessage(hDlg, IDC_CACHE_HIRES_TEXTURE,   BM_GETCHECK, 0, 0) == BST_CHECKED);

			if (status.bGameIsRunning) //BACKTOME, NEEDS RETHINKING -- CLEAN ME
			{
				if (options.bLoadHiResTextures)
				{
					InitHiresTextures();
					gTextureManager.RecheckHiresForAllTextures();
				}
				else
					CloseHiresTextures();
	
				if (options.bCacheHiResTextures)
					InitHiresCache();
				else
					ClearHiresCache();

				if (options.bDumpTexturesToFiles)
					InitTextureDump();
				else
					CloseTextureDump();
			}

			//--------------------------------------------------------------
			// End texture enhancement code
			//--------------------------------------------------------------

			defaultRomOptions.N64FrameBufferEmuType = SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_SETTING, CB_GETCURSEL, 0, 0);
			defaultRomOptions.N64FrameBufferWriteBackControl = SendDlgItemMessage(hDlg, IDC_FRAME_BUFFER_WRITE_BACK_CONTROL, CB_GETCURSEL, 0, 0);
			defaultRomOptions.N64RenderToTextureEmuType = SendDlgItemMessage(hDlg, IDC_RENDER_TO_TEXTURE_SETTING, CB_GETCURSEL, 0, 0);

			WriteConfiguration();
			EndDialog(hDlg, TRUE);

			return(TRUE);

		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return(TRUE);
		case IDC_ABOUT:
			DllAbout(hDlg);
			break;
		}
    }

    return FALSE;
}

//We need to keep these rom setting page, they should ALWAYS overide any other option
LRESULT APIENTRY RomSettingProc(HWND hDlg, unsigned message, LONG wParam, LONG lParam)
{
	int i;
	uint32 state;

	switch(message)
	{
	case WM_INITDIALOG:
		// Tri-state variables
		state = g_curRomInfo.dwNormalBlender==2 ? BST_CHECKED : (g_curRomInfo.dwNormalBlender==1?BST_UNCHECKED:BST_INDETERMINATE);
		SendDlgItemMessage(hDlg, IDC_ALPHA_BLENDER, BM_SETSTYLE, BS_AUTO3STATE, TRUE);
		SendDlgItemMessage(hDlg, IDC_ALPHA_BLENDER, BM_SETCHECK, state, 0);


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
	PROPSHEETPAGE	psp[2]; //Change this array size if you change the number of pages.
	PROPSHEETHEADER psh;
	memset(&psp,0,sizeof(PROPSHEETPAGE)*2);
	memset(&psh,0,sizeof(PROPSHEETHEADER));

	psp[PSH_OPTIONS].dwSize			= sizeof(PROPSHEETPAGE);
	psp[PSH_OPTIONS].dwFlags		= PSP_USETITLE;
	psp[PSH_OPTIONS].hInstance		= windowSetting.myhInst;
	psp[PSH_OPTIONS].pszTemplate	= "OPTIONS";
	psp[PSH_OPTIONS].pszIcon		= NULL;
	psp[PSH_OPTIONS].pfnDlgProc		= (DLGPROC)OptionsDialogProc;
	psp[PSH_OPTIONS].pszTitle		= "General Options";
	psp[PSH_OPTIONS].lParam			= 0;

	if (status.bGameIsRunning )
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

	hPropSheetHwnd = (HWND) PropertySheet(&psh);	// Start the Property Sheet

#endif
}

