/*
Copyright (C) 2002-2009 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either 
2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef _RICE_CONFIG_H_
#define _RICE_CONFIG_H_

#include "typedefs.h"

enum {
	FRM_BUF_NONE,
	FRM_BUF_IGNORE,
	FRM_BUF_BASIC,
	FRM_BUF_BASIC_AND_WRITEBACK,
	FRM_BUF_WRITEBACK_AND_RELOAD,
	FRM_BUF_COMPLETE,
	FRM_BUF_WITH_EMULATOR,
	FRM_BUF_BASIC_AND_WITH_EMULATOR,
	FRM_BUF_WITH_EMULATOR_READ_ONLY,
	FRM_BUF_WITH_EMULATOR_WRITE_ONLY,
};

enum {
	FRM_BUF_WRITEBACK_NORMAL,
	FRM_BUF_WRITEBACK_1_2,
	FRM_BUF_WRITEBACK_1_3,
	FRM_BUF_WRITEBACK_1_4,
	FRM_BUF_WRITEBACK_1_5,
	FRM_BUF_WRITEBACK_1_6,
	FRM_BUF_WRITEBACK_1_7,
	FRM_BUF_WRITEBACK_1_8,
};

enum {
	TXT_BUF_NONE,
	TXT_BUF_IGNORE,
	TXT_BUF_NORMAL,
	TXT_BUF_WRITE_BACK,
	TXT_BUF_WRITE_BACK_AND_RELOAD	,
};

enum {
	FORCE_DEFAULT_FILTER,
	FORCE_POINT_FILTER,
	FORCE_LINEAR_FILTER,
	FORCE_BILINEAR_FILTER,
};

enum {
	TEXTURE_NO_ENHANCEMENT,
	TEXTURE_2XSAI_ENHANCEMENT,
	TEXTURE_HQ2X_ENHANCEMENT,
	TEXTURE_HQ2XS_ENHANCEMENT,
	TEXTURE_EXTERNAL,
};

enum HACK_FOR_GAMES
{
	NO_HACK_FOR_GAME,
	HACK_FOR_BANJO_TOOIE,
	HACK_FOR_DR_MARIO,
	HACK_FOR_ZELDA,
	HACK_FOR_MARIO_TENNIS,
	HACK_FOR_BANJO,
	HACK_FOR_PD,
	HACK_FOR_GE,
	HACK_FOR_PILOT_WINGS,
	HACK_FOR_YOSHI,
	HACK_FOR_NITRO,
	HACK_FOR_TONYHAWK,
	HACK_FOR_NASCAR,
	HACK_FOR_SUPER_BOWLING,
	HACK_FOR_CONKER,
	HACK_FOR_ALL_STAR_BASEBALL,
	HACK_FOR_TIGER_HONEY_HUNT,
	HACK_REVERSE_XY_COOR,
	HACK_REVERSE_Y_COOR,
	HACK_FOR_GOLDEN_EYE,
	HACK_FOR_FZERO,
	HACK_FOR_COMMANDCONQUER,
	HACK_FOR_RUMBLE,
	HACK_FOR_SOUTH_PARK_RALLY,
	HACK_FOR_BUST_A_MOVE,
	HACK_FOR_OGRE_BATTLE,
	HACK_FOR_TWINE,
	HACK_FOR_EXTREME_G2,
	HACK_FOR_ROGUE_SQUADRON,
	HACK_FOR_MARIO_GOLF,
	HACK_FOR_MLB,
	HACK_FOR_POLARISSNOCROSS,
	HACK_FOR_TOPGEARRALLY,
	HACK_FOR_DUKE_NUKEM,
	HACK_FOR_ZELDA_MM,
	HACK_FOR_MARIO_KART,
};

enum {
	NOT_USE_CI_WIDTH_AND_RATIO,
	USE_CI_WIDTH_AND_RATIO_FOR_NTSC,
	USE_CI_WIDTH_AND_RATIO_FOR_PAL,
};

struct GlobalOptions{
	bool	bEnableHacks;
	bool	bEnableFog;
	bool	bWinFrameMode;
	bool	bMipMaps;

	uint32	forceTextureFilter;
	uint32	textureEnhancement;
	uint32	textureEnhancementControl;
	bool	bDumpTexturesToFiles;
	bool	bLoadHiResTextures;
	bool	bCacheHiResTextures;

	uint32	DirectXAntiAliasingValue;
	uint32	DirectXAnisotropyValue;

	HACK_FOR_GAMES	enableHackForGames;
} ;

extern GlobalOptions options;

struct FrameBufferOptions{
	bool	bUpdateCIInfo;

	bool	bCheckBackBufs;			// Check texture again against the recent backbuffer addresses
	bool	bWriteBackBufToRDRAM;	// If a recent backbuffer is used, write its content back to RDRAM
	bool	bLoadBackBufFromRDRAM;	// Load content from RDRAM and draw into backbuffer
	bool	bIgnore;				// Ignore all rendering into texture buffers

	bool	bSupportRenderTextures;		// Support render-to-texture
	bool	bCheckRenderTextures;			// Check texture again against the the last render_texture addresses
	bool	bRenderTextureWriteBack;		// Write back render_texture into RDRAM
	bool	bLoadRDRAMIntoRenderTexture;	// Load RDRAM content and render into render_texture

	bool	bAtEachFrameUpdate;		// Reload and write back at each frame buffer and CI update

	bool	bProcessCPUWrite;
	bool	bProcessCPURead;

	bool	bFillRectNextTextureBuffer;
	bool	bIgnoreRenderTextureIfHeightUnknown;
};

extern FrameBufferOptions frameBufferOptions;

BOOL InitConfiguration(void);

struct RomOptions{
	uint32	N64FrameBufferEmuType;
	uint32	N64FrameBufferWriteBackControl;
	uint32	N64RenderToTextureEmuType;
};

extern RomOptions defaultRomOptions;
extern RomOptions currentRomOptions;

extern char	szIniFileName[300];
void OutputSectionDetails(uint32 i, FILE * fh);
int FindIniEntry(uint32 dwCRC1, uint32 dwCRC2, uint8 nCountryID, LPCTSTR szName); 

struct ROMHeader
{
	uint8  x1, x2, x3, x4;
	uint32 dwClockRate;
	uint32 dwBootAddressOffset;
	uint32 dwRelease;
	uint32 dwCRC1;
	uint32 dwCRC2;
	uint64   qwUnknown1;
	char  szName[20];
	uint32 dwUnknown2;
	uint16  wUnknown3;
	uint8  nUnknown4;
	uint8  nManufacturer;
	uint16  wCartID;
	s8    nCountryID;
	uint8  nUnknown5;
};
#pragma pack()

typedef struct 
{
	// Other info from the rom. This is for convenience
	TCHAR	szGameName[50+1];
	s8	nCountryID;

	// Copy of the ROM header
	ROMHeader	romheader;

	// With changeable default values
	uint32	dwFrameBufferOption;
	uint32	dwRenderToTextureOption;

	// Less useful options
	BOOL	bIncTexRectEdge;
	BOOL	bZHack;
	BOOL	bTextureScaleHack;
	BOOL	bPrimaryDepthHack;
	BOOL	bTexture1Hack;
	BOOL 	bDisableCulling;
	int		VIWidth;
	int		VIHeight;
	uint32	UseCIWidthAndRatio;

	BOOL	bTxtSizeMethod2;
	BOOL	bEnableTxtLOD;
} GameSetting, *LPGAMESETTING;

extern GameSetting g_curRomInfo;

void ROM_GetRomNameFromHeader(TCHAR * szName, ROMHeader * pHdr);
void ROM_ByteSwap_3210(void *v, uint32 dwLen);

#define TV_SYSTEM_NTSC		1
#define TV_SYSTEM_PAL		0
uint32 CountryCodeToTVSystem(uint32 countryCode);

enum {
    PSH_OPTIONS,
    PSH_ROM_SETTINGS,
};

extern int DialogToStartRomIsRunning;
extern int DialogToStartRomIsNotRunning;

void CreateOptionsDialogs(HWND hParent);

#endif
