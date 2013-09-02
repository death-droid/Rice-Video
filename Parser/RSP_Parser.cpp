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

#include "..\stdafx.h"

#include "ucode.h"

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                    uCode Config                      //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
#define MAX_UCODE_INFO	16
UcodeInfo ucodeInfo[MAX_UCODE_INFO];

RDPInstruction LoadedUcodeMap[256];
LPCSTR LoadedUcodeNameMap[256];

OSTask *g_pOSTask = NULL;
UcodeInfo lastUcodeInfo;
UcodeInfo UsedUcodes[MAX_UCODE_INFO];
const uint32 maxUsedUcodes = sizeof(UsedUcodes)/sizeof(UcodeInfo);

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                     Ucodes                           //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

UcodeMap *ucodeMaps[] = 
{
	&ucodeMap0,				// ucode 0 - Mario
	&ucodeMap1,				// ucode 1 - GBI1
	&ucodeMap2,				// ucode 2 - Golden Eye
	&ucodeMap3,				// ucode 3 - S2DEX GBI2
	&ucodeMap4,				// ucode 4 - Wave Racer
	&ucodeMap5,				// ucode 5 - BGI2
	&ucodeMap6,				// ucode 6 - DKR
	&ucodeMap7,				// ucode 7 - S2DEX
	NULL,					// ucode 8 - ucode 0 with sprite2D, for Demo Puzzle Master 64
	&ucodeMap9,				// ucode 9 - Perfect Dark
	NULL,					// ucode 10 - Conker 
	&ucodeMap11,			// ucode 11 - Gemini
	NULL,					// ucode 12 - Silicon Valley, Spacestation
	NULL,					// ucode 13 - modified ucode S2DEX
	NULL,					// ucode 14 - OgreBattle Background
	NULL,					// ucode 15 - ucode 0 with sprite2D
	&ucodeMap16,			// ucode 16 - Star War, Shadow of Empire
	NULL,					// ucode 17 - Star Wars - Rogue Squadron, 
	NULL,					// ucode 18 - World Driver Championship
	NULL,					// ucode 19 - Last Legion UX
	&ucodeMap1,				// ucode 20 - ZSortp
};

uint32 vertexMultVals[] =
{
	10, // ucode 0 - Mario
	2,	// ucode 1 - GBI1
	10,	// ucode 2 - Golden Eye
	2,	// ucode 3 - S2DEX GBI2
	5,	// ucode 4 - Wave Racer
	2,	// ucode 5 - BGI2
	10, // ucode 6 - DKR
	2,	// ucode 7 - S2DEX
	10,	// ucode 8 - ucode 0 with sprite2D, for Demo Puzzle Master 64
	10, // ucode 9 - Perfect Dark
	2,	// ucode 10 - Conker
	10, // ucode 11 - Gemini
	2,	// ucode 12 - Silicon Valley, Spacestation
	2,	// ucode 13 - modified ucode S2DEX
	2,	// ucode 14 - OgreBattle Background
	10,	// ucode 15 - ucode 0 with sprite2D
	5,	// ucode 16 - Star War, Shadow of Empire
	2,	// ucode 17 - Star Wars - Rogue Squadron, 
	2,	// ucode 18 - World Driver Championship, check me here
	2,	// ucode 19 - Last Legion UX, check me here
	2,	// ucode 20 - ZSortp
};

CHAR gLastMicrocodeString[ 300 ] = "";

//*****************************************************************************
//
//*****************************************************************************
static UcodeData g_UcodeData[] = 
{
	//crc_size,	crc_800;

	{0, 0x150c3ce8, 0x150c3ce8, "RSP SW Version: 2.0D, 04-01-96",}, // Super Mario 64
	{4, 0x2b94276f, 0x2b94276f, "RSP SW Version: 2.0D, 04-01-96",}, // Wave Race 64 (v1.0)
	{16,0xb1870454, 0xb1870454, "RSP SW Version: 2.0D, 04-01-96",}, // Star Wars - Shadows of the Empire (v1.0), 
	{0, 0x51671ae4, 0x51671ae4, "RSP SW Version: 2.0D, 04-01-96",}, // Pilot Wings 64, 
	{0, 0x67b5ac55, 0x67b5ac55, "RSP SW Version: 2.0D, 04-01-96",}, // Wibble, 
	{0, 0x64dc8104, 0x64dc8104, "RSP SW Version: 2.0D, 04-01-96",}, // Dark Rift, 
	{0, 0x309f363d, 0x309f363d, "RSP SW Version: 2.0D, 04-01-96",}, // Killer Instinct Gold, 
	{0, 0xfcb57e57, 0xfcb57e57, "RSP SW Version: 2.0D, 04-01-96",}, // Blast Corps, 
	{0, 0xb420f35a, 0xb420f35a, "RSP SW Version: 2.0D, 04-01-96",}, // Blast Corps, 
	{0, 0x6e26c1df, 0x7c98e9c2, "RSP SW Version: 2.0D, 04-01-96",}, 
	{2, 0xc02ac7bc, 0xc02ac7bc, "RSP SW Version: 2.0G, 09-30-96",}, // GoldenEye 007, 
	{0, 0xe5fee3bc, 0xe5fee3bc, "RSP SW Version: 2.0G, 09-30-96",}, // Aero Fighters Assault, 
	{8, 0xe4bb5ad8, 0x80129845, "RSP SW Version: 2.0G, 09-30-96",}, // Puzzle Master 64, 
	{0, 0x72109ec6, 0x72109ec6, "RSP SW Version: 2.0H, 02-12-97",}, // Duke Nukem 64, 
	{0, 0xf24a9a04, 0xf24a9a04, "RSP SW Version: 2.0H, 02-12-97",}, // Tetrisphere, 
	{15,0x700de42e, 0x700de42e, "RSP SW Version: 2.0H, 02-12-97",}, // Wipeout 64 (uses GBI1 too!), 
	{15,0x1b304a74, 0x1b304a74, "RSP SW Version: 2.0H, 02-12-97",}, // Flying Dragon, 
	{15,0xe4bb5ad8, 0xa7b2f704, "RSP SW Version: 2.0H, 02-12-97",}, // Silicon Valley, 
	{15,0xe4bb5ad8, 0x88202781, "RSP SW Version: 2.0H, 02-12-97",}, // Glover, 
	{0, 0xe466b5bd, 0xe466b5bd, "Unknown 0xe466b5bd, 0xe466b5bd",}, // Dark Rift, 
	{9, 0x7064a163, 0x7064a163, "Unknown 0x7064a163, 0x7064a163",}, // Perfect Dark (v1.0), 
	{0, 0x6522df69, 0x71bd078d, "Unknown 0x6522df69, 0x71bd078d",}, // Tetris 
	{0, 0x6522df69, 0x1b0c23a8, "Unknown 0x6522df69, 0x1b0c23a8",}, // Pachinko Nichi 

	// GBI1
	
	{1, 0x45ca328e, 0x45ca328e, "RSP MicroCodeCommand ucode F3DLX         0.95 Yoshitaka Yasumoto Nintendo.",}, // Mario Kart 64, 
	{1, 0x98e3b909, 0x98e3b909, "RSP MicroCodeCommand ucode F3DEX         0.95 Yoshitaka Yasumoto Nintendo.",},	// Mario Kart 64
	{1, 0x5d446090, 0x5d446090, "RSP MicroCodeCommand ucode F3DLP.Rej     0.96 Yoshitaka Yasumoto Nintendo.",0,1}, // Jikkyou J. League Perfect Striker, 
	{1, 0x244f5ca3, 0x244f5ca3, "RSP MicroCodeCommand ucode F3DEX         1.00 Yoshitaka Yasumoto Nintendo.",}, // F-1 Pole Position 64, 
	{1, 0x6a022585, 0x6a022585, "RSP MicroCodeCommand ucode F3DEX.NoN     1.00 Yoshitaka Yasumoto Nintendo.",1}, // Turok - The Dinosaur Hunter (v1.0), 
	{1, 0x150706be, 0x150706be, "RSP MicroCodeCommand ucode F3DLX.NoN     1.00 Yoshitaka Yasumoto Nintendo.",1}, // Extreme-G, 
	{1, 0x503f2c53, 0x503f2c53, "RSP MicroCodeCommand ucode F3DEX.NoN     1.21 Yoshitaka Yasumoto Nintendo.",1}, // Bomberman 64, 
	{1, 0xc705c37c, 0xc705c37c, "RSP MicroCodeCommand ucode F3DLX         1.21 Yoshitaka Yasumoto Nintendo.",}, // Fighting Force 64, Wipeout 64
	{1, 0xa2146075, 0xa2146075, "RSP MicroCodeCommand ucode F3DLX.NoN     1.21 Yoshitaka Yasumoto Nintendo.",1}, // San Francisco Rush - Extreme Racing, 
	{1, 0xb65aa2da, 0xb65aa2da, "RSP MicroCodeCommand ucode L3DEX         1.21 Yoshitaka Yasumoto Nintendo.",}, // Wipeout 64, 
	{1, 0x0c8e5ec9, 0x0c8e5ec9, "RSP MicroCodeCommand ucode F3DEX         1.21 Yoshitaka Yasumoto Nintendo.",}, // 
	{1, 0xe30795f2, 0xa53df3c4, "RSP MicroCodeCommand ucode F3DLP.Rej     1.21 Yoshitaka Yasumoto Nintendo.",0,1},

	{1, 0xaebeda7d, 0xaebeda7d, "RSP MicroCodeCommand ucode F3DLX.Rej     1.21 Yoshitaka Yasumoto Nintendo.",0,1}, // Jikkyou World Soccer 3, 
	{1, 0x0c8e5ec9, 0x0c8e5ec9, "RSP MicroCodeCommand ucode F3DEX         1.23 Yoshitaka Yasumoto Nintendo" ,},  // Wave Race 64 (Rev. 2) - Shindou Rumble Edition (JAP) 
	{1, 0xc705c37c, 0xc705c37c, "RSP MicroCodeCommand ucode F3DLX         1.23 Yoshitaka Yasumoto Nintendo.",}, // GT
	{1, 0x2a61350d, 0x2a61350d, "RSP MicroCodeCommand ucode F3DLX         1.23 Yoshitaka Yasumoto Nintendo.",}, // Toy Story2
	{1, 0x0c8e5ec9, 0x0c8e5ec9, "RSP MicroCodeCommand ucode F3DEX         1.23 Yoshitaka Yasumoto Nintendo.",}, // Wave Race 64 Shindou Edition
	{12,0xfc6529aa, 0xfc6529aa, "RSP MicroCodeCommand ucode F3DEX         1.23 Yoshitaka Yasumoto Nintendo.",}, // Superman - The Animated Series, 
	{1, 0xa56cf996, 0xa56cf996, "RSP MicroCodeCommand ucode L3DEX         1.23 Yoshitaka Yasumoto Nintendo.",}, // Flying Dragon, 
	{1, 0xcc83b43f, 0xcc83b43f, "RSP MicroCodeCommand ucode F3DEX.NoN     1.23 Yoshitaka Yasumoto Nintendo.",1}, // AeroGauge, 
	{1, 0xca8927a0, 0xca8927a0, "RSP MicroCodeCommand ucode F3DLX.Rej     1.23 Yoshitaka Yasumoto Nintendo.",0,1},	// Puzzle Bobble 64, 
	{1, 0x25689c75, 0xbe481ae8, "RSP MicroCodeCommand ucode F3DLP.Rej     1.23 Yoshitaka Yasumoto Nintendo.",0,1},
	{1, 0xd2d747b7, 0xd2d747b7, "RSP MicroCodeCommand ucode F3DLX.NoN     1.23 Yoshitaka Yasumoto Nintendo.",1}, // Penny Racers, 
	{1, 0xa849c858, 0x5bd32b5a, "RSP MicroCodeCommand ucode F3DTEX/A      1.23 Yoshitaka Yasumoto Nintendo.",}, // Tamagotchi 

	{7, 0xecd8b772, 0xecd8b772, "RSP MicroCodeCommand ucode S2DEX  1.06 Yoshitaka Yasumoto Nintendo.",}, // Yoshi's Story, 
	{7, 0xf59132f5, 0xf59132f5, "RSP MicroCodeCommand ucode S2DEX  1.07 Yoshitaka Yasumoto Nintendo.",}, // Bakuretsu Muteki Bangaioh, 
	{7, 0x961dd811, 0x961dd811, "RSP MicroCodeCommand ucode S2DEX  1.03 Yoshitaka Yasumoto Nintendo.",}, // GT

	{5, 0x3e083afa, 0x722f97cc, "RSP MicroCodeCommand ucode F3DEX.NoN   fifo 2.03  Yoshitaka Yasumoto 1998 Nintendo.",1}, // F-Zero X, 
	{5, 0xa8050bd1, 0xa8050bd1, "RSP MicroCodeCommand ucode F3DEX       fifo 2.03  Yoshitaka Yasumoto 1998 Nintendo.",}, // F-Zero X, 
	{5, 0x4e8055f0, 0x4e8055f0, "RSP MicroCodeCommand ucode F3DLX.Rej   fifo 2.03  Yoshitaka Yasumoto 1998 Nintendo.",0,1}, // F-Zero X, 
	{5, 0xabf001f5, 0xabf001f5, "RSP MicroCodeCommand ucode F3DFLX.Rej  fifo 2.03F Yoshitaka Yasumoto 1998 Nintendo.",0,1}, // F-Zero X, 
	{5, 0xadb4b686, 0xadb4b686, "RSP MicroCodeCommand ucode F3DEX       fifo 2.04  Yoshitaka Yasumoto 1998 Nintendo.",}, // Top Gear Rally 2, 
	{5, 0x779e2a9b, 0x779e2a9b, "RSP MicroCodeCommand ucode F3DEX.NoN   fifo 2.04  Yoshitaka Yasumoto 1998 Nintendo.",1}, // California Speed, 
	{5, 0xa8cb3e09, 0xa8cb3e09, "RSP MicroCodeCommand ucode L3DEX       fifo 2.04  Yoshitaka Yasumoto 1998 Nintendo.",}, // In-Fisherman Bass Hunter 64, 
	{5, 0x2a1341d6, 0x2a1341d6, "RSP MicroCodeCommand ucode F3DEX       fifo 2.04H Yoshitaka Yasumoto 1998 Nintendo.",}, // Kirby 64 - The Crystal Shards, 
	{5, 0x3e083afa, 0x89a8e0ed, "RSP MicroCodeCommand ucode F3DEX.NoN   fifo 2.05  Yoshitaka Yasumoto 1998 Nintendo.",1}, // Carmageddon 64 (uncensored), 
	{5, 0x4964b75d, 0x4964b75d, "RSP MicroCodeCommand ucode F3DEX.NoN   fifo 2.05  Yoshitaka Yasumoto 1998 Nintendo.",1}, 
	{5, 0x39e3e95a, 0x39e3e95a, "RSP MicroCodeCommand ucode F3DEX       fifo 2.05  Yoshitaka Yasumoto 1998 Nintendo."}, // Knife Edge - Nose Gunner, 
	{5, 0xd2913522, 0xd2913522, "RSP MicroCodeCommand ucode F3DAM       fifo 2.05  Yoshitaka Yasumoto 1998 Nintendo."}, // Hey You, Pikachu!, 
	{5, 0x3e083afa, 0xc998443f, "RSP MicroCodeCommand ucode F3DEX       xbus 2.05  Yoshitaka Yasumoto 1998 Nintendo."}, //Triple play
	{5, 0xf4184a7d, 0xf4184a7d, "RSP MicroCodeCommand ucode F3DEX       fifo 2.06  Yoshitaka Yasumoto 1998 Nintendo.",}, // Hey You, Pikachu!, 
	{5, 0x595a88de, 0x595a88de, "RSP MicroCodeCommand ucode F3DEX.Rej   fifo 2.06  Yoshitaka Yasumoto 1998 Nintendo.",0,1}, // Bio Hazard 2, 
	{5, 0x0259f764, 0x0259f764, "RSP MicroCodeCommand ucode F3DLX.Rej   fifo 2.06  Yoshitaka Yasumoto 1998 Nintendo.",0,1}, // Mario Party, 
	{5, 0xe1a5477a, 0xe1a5477a, "RSP MicroCodeCommand ucode F3DEX.NoN   xbus 2.06  Yoshitaka Yasumoto 1998 Nintendo.",1}, // Command & Conquer, 
	{5, 0x4cfa0a19, 0x4cfa0a19, "RSP MicroCodeCommand ucode F3DZEX.NoN  fifo 2.06H Yoshitaka Yasumoto 1998 Nintendo.",1}, // The Legend of Zelda - Ocarina of Time (v1.0), 
	{5, 0x2cbd9514, 0x5f40b9f5, "RSP MicroCodeCommand ucode F3DZEX.NoN  fifo 2.06H Yoshitaka Yasumoto 1998 Nintendo.",1}, 

	{5, 0x3e083afa, 0x882680f4, "RSP MicroCodeCommand ucode L3DEX       fifo 2.07  Yoshitaka Yasumoto 1998 Nintendo."},	// Polaris Sno

	{5, 0xdeb1cac0, 0xdeb1cac0, "RSP MicroCodeCommand ucode F3DEX.NoN   fifo 2.07  Yoshitaka Yasumoto 1998 Nintendo.",1}, // Knockout Kings 2000, 
	{5, 0xf4184a7d, 0xf4184a7d, "RSP MicroCodeCommand ucode F3DEX       fifo 2.07  Yoshitaka Yasumoto 1998 Nintendo.",}, // Xena Warrior Princess - Talisman of Fate, Army Men - Air Combat, Destruction Derby
	{5, 0x4b013e60, 0x4b013e60, "RSP MicroCodeCommand ucode F3DEX       xbus 2.07  Yoshitaka Yasumoto 1998 Nintendo.",}, // Lode Runner 3-D, 
	{5, 0xd1a63836, 0xd1a63836, "RSP MicroCodeCommand ucode L3DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.",}, // Hey You, Pikachu!, 
	{5, 0x97193667, 0x97193667, "RSP MicroCodeCommand ucode F3DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.",}, // Top Gear Hyper-Bike, 
	{5, 0x92149ba8, 0x92149ba8, "RSP MicroCodeCommand ucode F3DEX       fifo 2.08  Yoshitaka Yasumoto/Kawasedo 1999.",}, // Paper Mario, 
	{5, 0xae0fb88f, 0xae0fb88f, "RSP MicroCodeCommand ucode F3DEX       xbus 2.08  Yoshitaka Yasumoto 1999 Nintendo.",}, // WWF WrestleMania 2000, 
	{5, 0xc572f368, 0xc572f368, "RSP MicroCodeCommand ucode F3DLX.Rej   xbus 2.08  Yoshitaka Yasumoto 1999 Nintendo.",}, // WWF No Mercy, 
	{5, 0x3e083afa, 0x74252492, "RSP MicroCodeCommand ucode F3DEX.NoN   xbus 2.08  Yoshitaka Yasumoto 1999 Nintendo.",1}, 

	{5, 0x9c2edb70, 0xea98e740, "RSP MicroCodeCommand ucode F3DEX.NoN   fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.",1}, // LEGO Racers, 
	{5, 0x79e004a6, 0x79e004a6, "RSP MicroCodeCommand ucode F3DLX.Rej   fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.",0,1}, // Mario Party 2, 
	{5, 0xaa6ab3ca, 0xaa6ab3ca, "RSP MicroCodeCommand ucode F3DEX.Rej   fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.",0,1}, // V-Rally Edition 99, 
	{5, 0x2c597e0f, 0x2c597e0f, "RSP MicroCodeCommand ucode F3DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.",}, // Cruis'n Exotica,
	{10, 0x4e5f3e3b, 0x4e5f3e3b,"RSP MicroCodeCommand ucode F3DEXBG.NoN fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.",1}, // Conker The Bad Fur Day 
	{5, 0x61f31862, 0x61f31862, "RSP MicroCodeCommand ucode F3DEX.NoN   fifo 2.08H Yoshitaka Yasumoto 1999 Nintendo.",1}, // Pokemon Snap, 
	{5, 0x005f5b71, 0x005f5b71, "RSP MicroCodeCommand ucode F3DZEX.NoN  fifo 2.08I Yoshitaka Yasumoto/Kawasedo 1999.",1}, // The Legend of Zelda 2 - Majora's Mask, 

	{3, 0x41839d1e, 0x41839d1e, "RSP MicroCodeCommand ucode S2DEX       fifo 2.05  Yoshitaka Yasumoto 1998 Nintendo.",}, // Chou Snobow Kids, 
	{3, 0x2cbd9514, 0xc639dbb9, "RSP MicroCodeCommand ucode S2DEX       xbus 2.06  Yoshitaka Yasumoto 1998 Nintendo.",},
	{3, 0xec89e273, 0xec89e273, "RSP MicroCodeCommand ucode S2DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.",}, // V-Rally Edition 99, 
	{3, 0x9429b7d6, 0x9429b7d6, "RSP MicroCodeCommand ucode S2DEX       xbus 2.08  Yoshitaka Yasumoto 1999 Nintendo.",}, // Star Craft, 
	//{14,0x5a72397b, 0xec89e273, "RSP MicroCodeCommand ucode S2DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.",}, // OgreBattle Background, 
	{3, 0x2cbd9514, 0xec89e273, "RSP MicroCodeCommand ucode S2DEX       fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo.",}, // Zelda MM, 

	{6, 0x6aef74f8, 0x6aef74f8, "Unknown 0x6aef74f8, 0x6aef74f8",}, // Diddy Kong Racing (v1.0), 
	{6, 0x4c4eead8, 0x4c4eead8, "Unknown 0x4c4eead8, 0x4c4eead8",}, // Diddy Kong Racing (v1.1), 

	{1, 0xed421e9a, 0xed421e9a, "Unknown 0xed421e9a, 0xed421e9a",}, // Kuiki Uhabi Suigo, 
	{5, 0x37751932, 0x55c0fd25, "Unknown 0x37751932, 0x55c0fd25",}, // Bio Hazard 2, 
	{11,0xbe0b83e7, 0xbe0b83e7,"Unknown 0xbe0b83e7, 0xbe0b83e7",},  // Jet Force Gemini, 

	{17, 0x02e882cf, 0x2ad17281, "Unknown 0x02e882cf, 0x2ad17281",}, // Indiana Jones, 
	{17, 0x1f7d9118, 0xdab2199b, "Unknown 0x1f7d9118, 0xdab2199b",}, // Battle Naboo, 
	{17, 0x74583614, 0x74583614, "Unknown 0x74583614, 0x74583614",}, // Star Wars - Rogue Squadron, 
	{17, 0xe37e2f49, 0x1eb63fd8, "Unknown 0xe37e2f49, 0x1eb63fd8",}, // Star Wars - Rogue Squadron, 
	{17, 0x8ce1af3d, 0xb2760ea2, "Unknown 0x8ce1af3d, 0xb2760ea2",}, // Star Wars - Rogue Squadron, 

	{18, 0x7b685972, 0x57b8095a, "Unknown 0x7b685972, 0x57b8095a",}, // World Driver Championship
	{18, 0xe92dbb9b, 0x57b8095a, "Unknown 0xe92dbb9b, 0x57b8095a",}, // World Driver Championship
	{18, 0xe6c9acc1, 0x65f80845, "Unknown 0xe6c9acc1, 0x65f80845",}, // World Driver Championship
	{18, 0x6522df69, 0x720b88a0, "Unknown 0x6522df69, 0x720b88a0",}, // World Driver Championship
	{18, 0x6522df69, 0xf1e8ba9e, "Unknown 0x6522df69, 0xf1e8ba9e",}, // World Driver Championship

	{19, 0xa486bed3, 0xa486bed3, "Unknown 0xa486bed3, 0xa486bed3",}, // Last Legion UX, 
	{19, 0x6b519381, 0xfebacfd8, "Unknown in Toukan Road",},	// I don't know which ucode

	{20, 0x6d2a01b1, 0x6d2a01b1, "RSP MicroCodeCommand ucode ZSortp 0.33 Yoshitaka Yasumoto Nintendo.",}, // Mia Hamm Soccer 64, 
};

FiddledVtx * g_pVtxBase=NULL;

SetImgInfo g_TI = { TXT_FMT_RGBA, TXT_SIZE_16b, 1, 0 };
SetImgInfo g_CI = { TXT_FMT_RGBA, TXT_SIZE_16b, 1, 0 };
SetImgInfo g_ZI = { TXT_FMT_RGBA, TXT_SIZE_16b, 1, 0 };
RenderTextureInfo g_ZI_saves[2];

DListStack	gDlistStack[MAX_DL_STACK_SIZE];
int		gDlistStackPointer= -1;

TMEMLoadMapInfo g_tmemLoadAddrMap[0x200];	// Totally 4KB TMEM
TMEMLoadMapInfo g_tmemInfo0;				// Info for Tmem=0
TMEMLoadMapInfo g_tmemInfo1;				// Info for Tmem=0x100

char *pszImgSize[4] = {"4", "8", "16", "32"};
const char *textluttype[4] = {"RGB16", "I16?", "RGBA16", "IA16"};
uint16	g_wRDPTlut[0x200];
uint32	g_dwRDPPalCrc[16];

#include "..\Device\FrameBuffer.h"

//Normal ucodes
#include "RSP_GBI0.h"
#include "RSP_GBI1.h"
#include "RSP_GBI2.h"
#include "RSP_GBI2_ext.h"
#include "RSP_GBI_Others.h"
#include "RSP_GBI_Sprite2D.h"
#include "RDP_Texture.h"

//Custom ucodes
#include "RSP_WRUS.h" //Wave Race 64
#include "RSP_LL.h" //Last Legion
#include "RSP_DKR.h" //Diddy Kong Racing
#include "RSP_SOTE.h" //Shadow Of The Empire
#include "RSP_Conker.h" //Conkers Bad Fur Day
#include "RSP_GE.h" //Golden Eye
#include "RSP_PD.h" //Perfect Dark
#include "RSP_RS.h" //Rouge Squadron

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                  Init and Reset                      //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

void DLParser_Init()
{
	int i;

	status.gRDPTime = 0;
	status.gDlistCount = 0;
	status.gUcodeCount = 0;
	status.frameReadByCPU = FALSE;
	status.frameWriteByCPU = FALSE;
	status.bN64IsDrawingTextureBuffer = false;
	status.bHandleN64RenderTexture = false;

	status.bUcodeIsKnown = FALSE;
	status.lastPurgeTimeTime = status.gRDPTime;

	status.curRenderBuffer = NULL;
	status.curDisplayBuffer = NULL;
	status.curVIOriginReg = NULL;

	status.lastPurgeTimeTime = 0;		// Time textures were last purged

	status.UseLargerTile[0] = false;
	status.LargerTileRealLeft[0] = status.LargerTileRealLeft[1] = 0;
	memset(&g_ZI_saves, 0, sizeof(RenderTextureInfo)*2);

	for( i=0; i<8; i++ )
	{
		memset(&gRDP.tiles[i], 0, sizeof(Tile));
	}
	memset(g_tmemLoadAddrMap, 0, sizeof(g_tmemLoadAddrMap));

	for( i=0; i<MAX_UCODE_INFO; i++ )
	{
		memset(&ucodeInfo[i], 0, sizeof(UcodeInfo));
	}

	status.bUseModifiedUcodeMap = false;
	status.ucodeHasBeenSet = false;
	status.bAllowLoadFromTMEM = true;
	
	char name[200];
	strcpy(name, g_curRomInfo.szGameName);

	memset(&lastUcodeInfo, 0, sizeof(UcodeInfo));
	memset(&UsedUcodes, 0, sizeof(UsedUcodes));
	memset(&g_TmemFlag, 0, sizeof(g_TmemFlag));
	memset(&g_RecentCIInfo, 0, sizeof(RecentCIInfo)*5);
	memset(&g_RecentVIOriginInfo, 0, sizeof(RecentViOriginInfo)*5);
	memset(&g_ZI_saves, 0, sizeof(RenderTextureInfo)*2);
	memset(&g_ZI, 0, sizeof(SetImgInfo));
	memset(&g_CI, 0, sizeof(SetImgInfo));
	memset(&g_TI, 0, sizeof(SetImgInfo));

	status.UseLargerTile[0] = status.UseLargerTile[1] = false;
	status.LargerTileRealLeft[0] = status.LargerTileRealLeft[1] = 0;
}


void RDP_GFX_Reset()
{
	gDlistStackPointer=-1;
	status.bUcodeIsKnown = FALSE;
	gTextureManager.RecycleAllTextures();
}


void RDP_Cleanup()
{
	if( status.bHandleN64RenderTexture )
	{
		g_pFrameBufferManager->CloseRenderTexture(false);
	}
}

void RDP_SetUcodeMap(int ucode)
{
	status.bUseModifiedUcodeMap = false;
	switch( ucode )
	{
	case 0:	// Mario and demos
		break;
	case 1:	// F3DEX GBI1
	case 20:
		break;
	case 2: // Golden Eye
		break;
	case 3:	// S2DEX GBI2
		break;
	case 4: // Wave racer
		break;
	case 5:	// F3DEX GBI2
		break;
	case 6: // DKR, Jet Force Gemini, Mickey
		break;
	case 11: // DKR, Jet Force Gemini, Mickey
		break;
	case 7: // S2DEX GBI1
		break;
	case 8: // Ucode 0 with Sprite2D, Puzzle Master 64
		memcpy( &LoadedUcodeMap, &ucodeMap0, sizeof(UcodeMap));
		LoadedUcodeMap[RSP_SPRITE2D_BASE] = RSP_GBI_Sprite2D_PuzzleMaster64;
		LoadedUcodeMap[RSP_SPRITE2D_SCALEFLIP] = RSP_GBI1_Sprite2DScaleFlip;
		LoadedUcodeMap[RSP_SPRITE2D_DRAW] = RSP_GBI0_Sprite2DDraw;
		status.bUseModifiedUcodeMap = true;
		break;
	case 9:	// Perfect Dark
		break;
	case 10: // Conker BFD
		memcpy( &LoadedUcodeMap, &ucodeMap5, sizeof(UcodeMap));
		LoadedUcodeMap[1]=RSP_Vtx_Conker;
		LoadedUcodeMap[0x10]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x11]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x12]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x13]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x14]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x15]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x16]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x17]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x18]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x19]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x1a]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x1b]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x1c]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x1d]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x1e]=RSP_Tri4_Conker;
		LoadedUcodeMap[0x1f]=RSP_Tri4_Conker;
		LoadedUcodeMap[0xdb]=RSP_MoveWord_Conker;
		LoadedUcodeMap[0xdc]=RSP_MoveMem_Conker;
		status.bUseModifiedUcodeMap = true;
		break;
	case 12: // Silicon Velley, Space Station
		memcpy( &LoadedUcodeMap, &ucodeMap1, sizeof(UcodeMap));
		LoadedUcodeMap[0x01]=RSP_GBI0_Mtx;
		status.bUseModifiedUcodeMap = true;
		break;
	case 13: // modified S2DEX
		memcpy( &LoadedUcodeMap, &ucodeMap7, sizeof(UcodeMap));
		//LoadedUcodeMap[S2DEX_BG_1CYC] = ucodeMap1[S2DEX_BG_1CYC];
		LoadedUcodeMap[S2DEX_OBJ_RECTANGLE] = ucodeMap1[S2DEX_OBJ_RECTANGLE];
		LoadedUcodeMap[S2DEX_OBJ_SPRITE] = ucodeMap1[S2DEX_OBJ_SPRITE];
		//LoadedUcodeMap[S2DEX_OBJ_RENDERMODE] = ucodeMap1[S2DEX_OBJ_RENDERMODE];
		//LoadedUcodeMap[S2DEX_OBJ_RECTANGLE_R] = ucodeMap1[S2DEX_OBJ_RECTANGLE_R];
		LoadedUcodeMap[S2DEX_RDPHALF_0] = ucodeMap1[S2DEX_RDPHALF_0];
		status.bUseModifiedUcodeMap = true;
		break;
	case 14: // OgreBattle Background
		memcpy( &LoadedUcodeMap, &ucodeMap5, sizeof(UcodeMap));
		LoadedUcodeMap[0xda] = DLParser_OgreBatter64BG;
		LoadedUcodeMap[0xdc] = RSP_S2DEX_OBJ_MOVEMEM;
		status.bUseModifiedUcodeMap = true;
		break;
	case 15: // Ucode 0 with Sprite2D
		memcpy( &LoadedUcodeMap, &ucodeMap0, sizeof(UcodeMap));
		LoadedUcodeMap[RSP_SPRITE2D_BASE] = RSP_GBI_Sprite2DBase;
		LoadedUcodeMap[RSP_SPRITE2D_SCALEFLIP] = RSP_GBI1_Sprite2DScaleFlip;
		LoadedUcodeMap[RSP_SPRITE2D_DRAW] = RSP_GBI0_Sprite2DDraw;
		status.bUseModifiedUcodeMap = true;
		break;
	case 16: // Star War, Shadow Of Empire
		break;
	case 17:	//Indiana Jones, does not work anyway
		memcpy( &LoadedUcodeMap, &ucodeMap1, sizeof(UcodeMap));
		LoadedUcodeMap[0]=DLParser_Ucode8_0x0;
		//LoadedUcodeMap[1]=RSP_RDP_Nothing;
		LoadedUcodeMap[2]=DLParser_RS_Color_Buffer;
		LoadedUcodeMap[3]=DLParser_RS_MoveMem;
		LoadedUcodeMap[4]=DLParser_RS_Vtx_Buffer;
		LoadedUcodeMap[5]=DLParser_Ucode8_0x05;
		LoadedUcodeMap[6]=DLParser_Ucode8_DL;
		LoadedUcodeMap[7]=DLParser_Ucode8_JUMP;
		LoadedUcodeMap[8]=RSP_RDP_Nothing;
		LoadedUcodeMap[9]=RSP_RDP_Nothing;
		LoadedUcodeMap[10]=RSP_RDP_Nothing;
		LoadedUcodeMap[11]=RSP_RDP_Nothing;
		LoadedUcodeMap[0x80]=DLParser_RS_Block;
		LoadedUcodeMap[0xb4]=DLParser_Ucode8_0xb4;
		LoadedUcodeMap[0xb5]=DLParser_Ucode8_0xb5;
		LoadedUcodeMap[0xb8]=DLParser_Ucode8_EndDL;
		LoadedUcodeMap[0xbc]=DLParser_Ucode8_0xbc;
		LoadedUcodeMap[0xbd]=DLParser_Ucode8_0xbd;
		LoadedUcodeMap[0xbe]=DLParser_RS_0xbe;
		LoadedUcodeMap[0xbF]=DLParser_Ucode8_0xbf;
		LoadedUcodeMap[0xe4]=DLParser_TexRect_Last_Legion;
		status.bUseModifiedUcodeMap = true;
		break;
	case 18: // World Driver Championship
		memcpy( &LoadedUcodeMap, &ucodeMap1, sizeof(UcodeMap));
		LoadedUcodeMap[0xe]=DLParser_RSP_DL_WorldDriver;
		LoadedUcodeMap[0x2]=DLParser_RSP_Pop_DL_WorldDriver;
		LoadedUcodeMap[0xdf]=DLParser_RSP_Pop_DL_WorldDriver;
		LoadedUcodeMap[0x6]=RSP_RDP_Nothing;
		status.bUseModifiedUcodeMap = true;
		break;
	case 19: // Last Legion UX
		memcpy( &LoadedUcodeMap, &ucodeMap1, sizeof(UcodeMap));
		LoadedUcodeMap[0x80]=DLParser_RSP_Last_Legion_0x80;
		LoadedUcodeMap[0x00]=DLParser_RSP_Last_Legion_0x00;
		LoadedUcodeMap[0xe4]=DLParser_TexRect_Last_Legion;
		status.bUseModifiedUcodeMap = true;
		break;
	default:
		memcpy( &LoadedUcodeMap, &ucodeMap5, sizeof(UcodeMap));
		status.bUseModifiedUcodeMap = true;
		break;
	}

#ifdef _DEBUG
	if( logMicrocode )
		TRACE1("Using ucode %d", ucode);
#endif
}

void RSP_SetUcode(int ucode, uint32 ucStart, uint32 ucDStart, uint32 ucSize)
{
	//If ucode has been set and current ucode matches with one where parsing then return
	if( status.ucodeHasBeenSet && gRSP.ucode == ucode )
		return;

	//Set it so we know the ucode has been
	status.ucodeHasBeenSet = true;

	//If ucode seems invalid, just ucode 5 as default
	if( ucode < 0 )
		ucode = 5;

	//Set the ucode map to the specified ucode
	RDP_SetUcodeMap(ucode);

	//If where using a modified ucode map, then lets use that instead of our current one
	if( status.bUseModifiedUcodeMap )
		currentUcodeMap = &LoadedUcodeMap[0];
	else
		currentUcodeMap = *ucodeMaps[ucode];

	//Grab our vertex multiplier from our table
	gRSP.vertexMult = vertexMultVals[ucode];

	//if( gRSP.ucode != ucode )	DebuggerAppendMsg("Set to ucode: %d", ucode);
	gRSP.ucode = ucode;

	lastUcodeInfo.used = true;
	if( ucStart == 0 )
	{
		lastUcodeInfo.ucStart = g_pOSTask->t.ucode;
		lastUcodeInfo.ucDStart = g_pOSTask->t.ucode_data;
		lastUcodeInfo.ucSize = g_pOSTask->t.ucode_size;
	}
	else
	{
		lastUcodeInfo.ucStart = ucStart;
		lastUcodeInfo.ucDStart = ucDStart;
		lastUcodeInfo.ucSize = ucSize;
	}
}

//*****************************************************************************
//
//*****************************************************************************
static uint32 DLParser_IdentifyUcodeFromString( const CHAR * str_ucode )
{
	//Lets try and indentify what ucode where using via a string
	const CHAR str_ucode0[] = "RSP SW Version: 2.0";
	const CHAR str_ucode1[] = "RSP MicroCodeCommand ucode ";

	if ( _strnicmp( str_ucode, str_ucode0, strlen(str_ucode0) ) == 0 )
	{
		return 0;
	}

	if ( _strnicmp( str_ucode, str_ucode1, strlen(str_ucode1) ) == 0 )
	{
		if( strstr(str_ucode,"1.") != 0 )
		{
			if( strstr(str_ucode,"S2DEX") != 0 )
			{
				return 7;
			}
			else
				return 1;
		}
		else if( strstr(str_ucode,"2.") != 0 )
		{
			if( strstr(str_ucode,"S2DEX") != 0 )
			{
				return 3;
			}
			else
				return 5;
		}
	}

	return 5;
}

//*****************************************************************************
//
//*****************************************************************************
static uint32 DLParser_IdentifyUcode( uint32 crc_size, uint32 crc_800, char* str )
{
	for ( uint32 i = 0; i < sizeof(g_UcodeData)/sizeof(UcodeData); i++ )
	{
#ifdef _DEBUG
		if ( crc_800 == g_UcodeData[i].crc_800 )
		{
			if( strlen(str)==0 || strcmp(g_UcodeData[i].ucode_name, str) == 0 ) 
			{
				TRACE0(g_UcodeData[i].ucode_name);
			}
			else
			{
				DebuggerAppendMsg("Incorrect description for this ucode:\n%x, %x, %s",crc_800, crc_size, str);
			}
			status.bUcodeIsKnown = TRUE;
			gRSP.bNearClip = !g_UcodeData[i].non_nearclip;
			gRSP.bRejectVtx = g_UcodeData[i].reject;
			DebuggerAppendMsg("Identify ucode = %d, crc = %08X, %s", g_UcodeData[i].ucode, crc_800, str);
			return g_UcodeData[i].ucode;
		}
#else
		if ( crc_800 == g_UcodeData[i].crc_800 )
		{
			status.bUcodeIsKnown = TRUE;
			gRSP.bNearClip = !g_UcodeData[i].non_nearclip;
			gRSP.bRejectVtx = g_UcodeData[i].reject;
			return g_UcodeData[i].ucode;
		}
#endif
	}

#ifdef _DEBUG
	{
		static bool warned = false;
		if( warned == false )
		{
			warned = true;
			TRACE0("Can not identify ucode for this game");
		}
	}
#endif
	gRSP.bNearClip = false;
	gRSP.bRejectVtx = false;
	status.bUcodeIsKnown = FALSE;
	return ~0;
}

uint32 DLParser_CheckUcode(uint32 ucStart, uint32 ucDStart, uint32 ucSize, uint32 ucDSize)
{
	if( options.enableHackForGames == HACK_FOR_ROGUE_SQUADRON )
	{
		return 17;
	}

	// Check the used ucode table first
	int usedUcodeIndex = 0;
	for( usedUcodeIndex=0; usedUcodeIndex<maxUsedUcodes; usedUcodeIndex++ )
	{
		if( UsedUcodes[usedUcodeIndex].used == false )
		{
			break;
		}

		if( UsedUcodes[usedUcodeIndex].ucStart == ucStart && UsedUcodes[usedUcodeIndex].ucSize == ucSize &&
			UsedUcodes[usedUcodeIndex].ucDStart == ucDStart /*&& UsedUcodes[usedUcodeIndex].ucDSize == ucDSize*/ )
		{
#ifdef _DEBUG
			if( gRSP.ucode != UsedUcodes[usedUcodeIndex].ucode && logMicrocode)
			{
				DebuggerAppendMsg("Check, ucode = %d, crc = %08X, %s", UsedUcodes[usedUcodeIndex].ucode, 
					UsedUcodes[usedUcodeIndex].crc_800 , UsedUcodes[usedUcodeIndex].rspstr);
			}
#endif
			lastUcodeInfo.ucStart = ucStart;
			lastUcodeInfo.used = true;
			lastUcodeInfo.ucDStart = ucDStart;
			lastUcodeInfo.ucSize = ucSize;
			return UsedUcodes[usedUcodeIndex].ucode;
		}
	}

	uint32 base = ucDStart & 0x1fffffff;
	CHAR str[300] = "";
	if( base < g_dwRamSize+0x1000 )
	{
		for ( uint32 i = 0; i < 0x1000; i++ )
		{

			if ( g_ps8RamBase[ base + ((i+0) ^ 3) ] == 'R' &&
				g_ps8RamBase[ base + ((i+1) ^ 3) ] == 'S' &&
				g_ps8RamBase[ base + ((i+2) ^ 3) ] == 'P' )
			{
				CHAR * p = str;
				while ( g_ps8RamBase[ base + (i ^ 3) ] >= ' ')
				{
					*p++ = g_ps8RamBase[ base + (i ^ 3) ];
					i++;
				}
				*p++ = 0;
				break;
			}
		}
	}

	//if ( strcmp( str, gLastMicrocodeString ) != 0 )
	{
		uint32 size = ucDSize;
		base = ucStart & 0x1fffffff;

		uint32 crc_size = ComputeCRC32( 0, &g_pu8RamBase[ base ], 8);//size );
		uint32 crc_800 = ComputeCRC32( 0, &g_pu8RamBase[ base ], 0x800 );
		uint32 ucode;
		ucode = DLParser_IdentifyUcode( crc_size, crc_800, str );
		if ( ucode == ~0 )
		{
#ifdef _DEBUG
			static bool warned=false;
			//if( warned == false )
			{
				CHAR message[300];

				sprintf(message, "Unable to find ucode to use for\n\n"
								  "%s\n"
								  "CRCSize: 0x%08x\n\n"
								  "CRC800: 0x%08x\n"
								  "Please mail rice1964@yahoo.com with the contents of c:\\ucodes.txt",
						str, crc_size, crc_800);
				TRACE0(message);
				ErrorMsg(message);
				warned = true;
			}
#endif
			ucode = DLParser_IdentifyUcodeFromString(str);
			if ( ucode == ~0 )
			{
				ucode=5;
			}
		}

		//DLParser_SetuCode( ucode );
		
#ifdef _DEBUG
		{
			static bool warned=false;
			if( warned == false )
			{
				warned = true;
				if( strlen(str) == 0 )
					DebuggerAppendMsg("Can not find RSP string in the DLIST, CRC800: 0x%08x, CRCSize: 0x%08x", crc_800, crc_size);
				else
					TRACE0(str);
			}
		}
#endif
		strcpy( gLastMicrocodeString, str );

		if( usedUcodeIndex >= MAX_UCODE_INFO )
		{
			usedUcodeIndex = rand()%MAX_UCODE_INFO;
		}

		UsedUcodes[usedUcodeIndex].ucStart = ucStart;
		UsedUcodes[usedUcodeIndex].ucSize = ucSize;
		UsedUcodes[usedUcodeIndex].ucDStart = ucDStart;
		UsedUcodes[usedUcodeIndex].ucDSize = ucDSize;
		UsedUcodes[usedUcodeIndex].ucode = ucode;
		UsedUcodes[usedUcodeIndex].crc_800 = crc_800;
		UsedUcodes[usedUcodeIndex].crc_size = crc_size;
		UsedUcodes[usedUcodeIndex].used = true;
		strcpy( UsedUcodes[usedUcodeIndex].rspstr, str );

		TRACE2("New ucode has been detected:\n%s, ucode=%d", str, ucode);
	
		return ucode;
	}
}

extern int dlistMtxCount;
extern bool bHalfTxtScale;

void DLParser_Process(OSTask * pTask)
{
	dlistMtxCount = 0;
	bHalfTxtScale = false;

	if ( CRender::g_pRender == NULL)
	{
		TriggerDPInterrupt();
		return;
	}

	status.bScreenIsDrawn = true;

	g_pOSTask = pTask;

	DebuggerPauseCountN( NEXT_DLIST );
	status.gRDPTime = timeGetTime();
	status.gDlistCount++;

	if ( lastUcodeInfo.ucStart != (uint32)(pTask->t.ucode) )
	{
		uint32 ucode = DLParser_CheckUcode(pTask->t.ucode, pTask->t.ucode_data, pTask->t.ucode_size, pTask->t.ucode_data_size);
		RSP_SetUcode(ucode, pTask->t.ucode, pTask->t.ucode_data, pTask->t.ucode_size);
		DEBUGGER_PAUSE_AND_DUMP(NEXT_SWITCH_UCODE,{DebuggerAppendMsg("Pause at switching ucode");});
	}

	// Initialize stack
	status.bN64FrameBufferIsUsed = false;
	gDlistStackPointer=0;
	gDlistStack[gDlistStackPointer].pc = (uint32)pTask->t.data_ptr;
	gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;
	
	DEBUGGER_PAUSE_AT_COND_AND_DUMP_COUNT_N((gDlistStack[gDlistStackPointer].pc == 0 && pauseAtNext && eventToPause==NEXT_UNKNOWN_OP),
			{DebuggerAppendMsg("Start Task without DLIST: ucode=%08X, data=%08X", (uint32)pTask->t.ucode, (uint32)pTask->t.ucode_data);});


	// Check if we need to purge
	if (status.gRDPTime - status.lastPurgeTimeTime > 5000)
	{
		gTextureManager.PurgeOldTextures();
		status.lastPurgeTimeTime = status.gRDPTime;
	}

	status.dwNumDListsCulled = 0;
	status.dwNumTrisRendered = 0;
	status.dwNumTrisClipped = 0;
	status.dwNumVertices = 0;
	status.dwBiggestVertexIndex = 0;

	if( g_curRomInfo.bForceScreenClear && CGraphicsContext::needCleanScene )
	{
		CRender::g_pRender->ClearBuffer(true,true);
		CGraphicsContext::needCleanScene = false;
	}

	SetVIScales();
	CRender::g_pRender->RenderReset();
	CRender::g_pRender->BeginRendering();
	CRender::g_pRender->SetViewport(0, 0, windowSetting.uViWidth, windowSetting.uViHeight, 0x3FF);
	CRender::g_pRender->SetFillMode(options.bWinFrameMode? RICE_FILLMODE_WINFRAME : RICE_FILLMODE_SOLID);

	try
	{
		// The main loop
		while( gDlistStackPointer >= 0 )
		{
#ifdef _DEBUG
			DEBUGGER_PAUSE_COUNT_N(NEXT_UCODE);
			if( debuggerPause )
			{
				DebuggerPause();
				CRender::g_pRender->SetFillMode(options.bWinFrameMode? RICE_FILLMODE_WINFRAME : RICE_FILLMODE_SOLID);
			}
#endif

#ifdef _DEBUG
			if (gDlistStack[gDlistStackPointer].pc > g_dwRamSize)
			{
				DebuggerAppendMsg("Error: dwPC is %08X", gDlistStack[gDlistStackPointer].pc );
				break;
			}
#endif

			status.gUcodeCount++;

			MicroCodeCommand *p_command = (MicroCodeCommand*)&g_pu32RamBase[(gDlistStack[gDlistStackPointer].pc>>2)];

			gDlistStack[gDlistStackPointer].pc += 8;
			currentUcodeMap[p_command->inst.cmd0 >>24](*p_command);

			if ( gDlistStackPointer >= 0 && --gDlistStack[gDlistStackPointer].countdown < 0 )
			{
				LOG_UCODE("**EndDLInMem");
				gDlistStackPointer--;
			}
		}

	}
	catch(...)
	{
		TRACE0("Unknown exception happens in ProcessDList");
		TriggerDPInterrupt();
	}

	CRender::g_pRender->EndRendering();
		
}

//*****************************************************************************
// Reads the next command from the display list, updates the PC.
//*****************************************************************************
inline void DLParser_FetchNextCommand(MicroCodeCommand * p_command)
{
	// Current PC is the last value on the stack
	*p_command = *(MicroCodeCommand*)&g_pu32RamBase[(gDlistStack[gDlistStackPointer].pc>>2)];

	gDlistStack[gDlistStackPointer].pc += 8;

}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                   Util Functions                     //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////


void RDP_NOIMPL_Real(LPCTSTR op, uint32 word0, uint32 word1) 
{
#ifdef _DEBUG
	if( logWarning )
	{
		TRACE0("Stack Trace");
		for( int i=0; i<gDlistStackPointer; i++ )
		{
			DebuggerAppendMsg("  %08X", gDlistStack[i].pc);
		}
		uint32 dwPC = gDlistStack[gDlistStackPointer].pc-8;
		DebuggerAppendMsg("PC=%08X",dwPC);
		DebuggerAppendMsg(op, word0, word1);
	}
	DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_UNKNOWN_OP, {TRACE0("Paused at unimplemented ucode\n");})
#endif
}

void RDP_NOIMPL_WARN(LPCTSTR op)
{
#ifdef _DEBUG
	if(logWarning)
	{
		TRACE0(op);
	}
#endif
}


void RSP_GBI1_Noop(MicroCodeCommand command)
{
}


void RDP_GFX_PopDL()
{
	LOG_UCODE("Returning from DisplayList: level=%d", gDlistStackPointer+1);
	LOG_UCODE("############################################");
	LOG_UCODE("/\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\ /\\");
	LOG_UCODE("");

	gDlistStackPointer--;
}

uint32 CalcalateCRC(uint32* srcPtr, uint32 srcSize)
{
	uint32 crc=0;
	for( uint32 i=0; i<srcSize; i++ )
	{
		crc += srcPtr[i];
	}
	return crc;
}


void RSP_GFX_InitGeometryMode()
{
	bool bCullFront		= (gRDP.geometryMode & G_CULL_FRONT) ? true : false;
	bool bCullBack		= (gRDP.geometryMode & G_CULL_BACK) ? true : false;

	if( bCullFront && bCullBack ) // should never cull front
		bCullFront = false;

	CRender::g_pRender->SetCullMode(bCullFront, bCullBack);
	
	BOOL bShade			= (gRDP.geometryMode & G_SHADE) ? TRUE : FALSE;
	BOOL bShadeSmooth	= (gRDP.geometryMode & G_SHADING_SMOOTH) ? TRUE : FALSE;
	if (bShade && bShadeSmooth)		
		CRender::g_pRender->SetShadeMode( SHADE_SMOOTH );
	else							
		CRender::g_pRender->SetShadeMode( SHADE_FLAT );
	
	CRender::g_pRender->SetFogEnable( gRDP.geometryMode & G_FOG ? true : false );
	SetTextureGen((gRDP.geometryMode & G_TEXTURE_GEN) ? true : false );
	SetLighting( (gRDP.geometryMode & G_LIGHTING ) ? true : false );
	CRender::g_pRender->ZBufferEnable( gRDP.geometryMode & G_ZBUFFER );
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                   DP Ucodes                          //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

void DLParser_SetKeyGB(MicroCodeCommand command)
{
	gRDP.keyB = ((command.inst.cmd1)>>8)&0xFF;
	gRDP.keyG = ((command.inst.cmd1)>>24)&0xFF;
	gRDP.keyA = (gRDP.keyR+gRDP.keyG+gRDP.keyB)/3;
	gRDP.fKeyA = gRDP.keyA/255.0f;
}
void DLParser_SetKeyR(MicroCodeCommand command)
{
	gRDP.keyR = ((command.inst.cmd1)>>8)&0xFF;
	gRDP.keyA = (gRDP.keyR+gRDP.keyG+gRDP.keyB)/3;
	gRDP.fKeyA = gRDP.keyA/255.0f;
}

void DLParser_SetConvert(MicroCodeCommand command)
{
	LOG_UCODE("SetConvert: (Ignored)");
}
void DLParser_SetPrimDepth(MicroCodeCommand command)
{
	uint32 dwZ  = ((command.inst.cmd1) >> 16) & 0xFFFF;
	uint32 dwDZ = ((command.inst.cmd1)      ) & 0xFFFF;

	LOG_UCODE("SetPrimDepth: 0x%08x 0x%08x - z: 0x%04x dz: 0x%04x",
		command.inst.cmd0, command.inst.cmd1, dwZ, dwDZ);
	
	SetPrimitiveDepth(dwZ, dwDZ);
	DEBUGGER_PAUSE(NEXT_SET_PRIM_COLOR);
}

void DLParser_RDPSetOtherMode(MicroCodeCommand command)
{
	if( gRDP.otherMode.H != ((command.inst.cmd0) & 0x0FFFFFFF) )
	{
		gRDP.otherMode.H = ((command.inst.cmd0) & 0x0FFFFFFF);

		uint32 dwTextFilt  = (gRDP.otherMode.H>>RSP_SETOTHERMODE_SHIFT_TEXTFILT)&0x3;
		CRender::g_pRender->SetTextureFilter(dwTextFilt<<RSP_SETOTHERMODE_SHIFT_TEXTFILT);
	}

	if( gRDP.otherMode.L != (command.inst.cmd1) )
	{
		if( (gRDP.otherMode.L&ZMODE_DEC) != ((command.inst.cmd1)&ZMODE_DEC) )
		{
			if( ((command.inst.cmd1)&ZMODE_DEC) == ZMODE_DEC )
				CRender::g_pRender->SetZBias( 2 );
			else
				CRender::g_pRender->SetZBias( 0 );
		}

		gRDP.otherMode.L = (command.inst.cmd1);

		BOOL bZCompare		= (gRDP.otherMode.L & Z_COMPARE)			? TRUE : FALSE;
		BOOL bZUpdate		= (gRDP.otherMode.L & Z_UPDATE)			? TRUE : FALSE;

		CRender::g_pRender->SetZCompare( bZCompare );
		CRender::g_pRender->SetZUpdate( bZUpdate );

		uint32 dwAlphaTestMode = (gRDP.otherMode.L >> RSP_SETOTHERMODE_SHIFT_ALPHACOMPARE) & 0x3;

		if ((dwAlphaTestMode) != 0)
			CRender::g_pRender->SetAlphaTestEnable( TRUE );
		else
			CRender::g_pRender->SetAlphaTestEnable( FALSE );
	}

	uint16 blender = gRDP.otherMode.blender;
	RDP_BlenderSetting &bl = *(RDP_BlenderSetting*)(&(blender));
	if( bl.c1_m1a==3 || bl.c1_m2a == 3 || bl.c2_m1a == 3 || bl.c2_m2a == 3 )
		gRDP.bFogEnableInBlender = true;
	else
		gRDP.bFogEnableInBlender = false;
}



void DLParser_RDPLoadSync(MicroCodeCommand command)	
{ 
	LOG_UCODE("LoadSync: (Ignored)"); 
}

void DLParser_RDPPipeSync(MicroCodeCommand command)	
{ 
	LOG_UCODE("PipeSync: (Ignored)"); 
}
void DLParser_RDPTileSync(MicroCodeCommand command)	
{ 
	LOG_UCODE("TileSync: (Ignored)"); 
}

void DLParser_RDPFullSync(MicroCodeCommand command)
{ 
	TriggerDPInterrupt();
}

void DLParser_SetScissor(MicroCodeCommand command)
{
	ScissorType tempScissor;
	// The coords are all in 8:2 fixed point
	tempScissor.x0   = ((command.inst.cmd0)>>12)&0xFFF;
	tempScissor.y0   = ((command.inst.cmd0)>>0 )&0xFFF;
	tempScissor.mode = ((command.inst.cmd1)>>24)&0x03;
	tempScissor.x1   = ((command.inst.cmd1)>>12)&0xFFF;
	tempScissor.y1   = ((command.inst.cmd1)>>0 )&0xFFF;

	tempScissor.left	= tempScissor.x0/4;
	tempScissor.top		= tempScissor.y0/4;
	tempScissor.right	= tempScissor.x1/4;
	tempScissor.bottom	= tempScissor.y1/4;

	if( options.bEnableHacks )
	{
		if( g_CI.dwWidth == 0x200 && tempScissor.right == 0x200 )
		{
			uint32 width = *g_GraphicsInfo.VI_WIDTH_REG & 0xFFF;

			if( width != 0x200 )
			{
				// Hack for RE2
				tempScissor.bottom = tempScissor.right*tempScissor.bottom/width;
				tempScissor.right = width;
			}

		}
	}

	if( gRDP.scissor.left != tempScissor.left || gRDP.scissor.top != tempScissor.top ||
		gRDP.scissor.right != tempScissor.right || gRDP.scissor.bottom != tempScissor.bottom ||
		gRSP.real_clip_scissor_left != tempScissor.left || gRSP.real_clip_scissor_top != tempScissor.top ||
		gRSP.real_clip_scissor_right != tempScissor.right || gRSP.real_clip_scissor_bottom != tempScissor.bottom)
	{
		memcpy(&(gRDP.scissor), &tempScissor, sizeof(ScissorType) );
		if( !status.bHandleN64RenderTexture )
			SetVIScales();

		if(  options.enableHackForGames == HACK_FOR_SUPER_BOWLING && g_CI.dwAddr%0x100 != 0 )
		{
			// right half screen
			gRDP.scissor.left += 160;
			gRDP.scissor.right += 160;
			CRender::g_pRender->SetViewport(160, 0, 320, 240, 0xFFFF);
		}

		CRender::g_pRender->UpdateClipRectangle();
		CRender::g_pRender->UpdateScissor();
	}

	LOG_UCODE("SetScissor: x0=%d y0=%d x1=%d y1=%d mode=%d",
		gRDP.scissor.left, gRDP.scissor.top,
		gRDP.scissor.right, gRDP.scissor.bottom,
		gRDP.scissor.mode);

	///TXTRBUF_DETAIL_DUMP(DebuggerAppendMsg("SetScissor: x0=%d y0=%d x1=%d y1=%d mode=%d",	gRDP.scissor.left, gRDP.scissor.top,
	//gRDP.scissor.right, gRDP.scissor.bottom, gRDP.scissor.mode););
}


void DLParser_FillRect(MicroCodeCommand command)
{
	if( status.bN64IsDrawingTextureBuffer && frameBufferOptions.bIgnore )
		return;

	// Removes annoying rect that appears in Conker and fillrects that cover screen in banjo tooie
	if (g_CI.dwFormat != TXT_FMT_RGBA)
	{
		TRACE0("	Ignoring Fillrect	");
		return;
	}

	u32 fill_colour = gRDP.originalFillColor;

	//Always clear zBuffer if depth buffer has been selected
	if(g_ZI.dwAddr == g_CI.dwAddr)
	{
		CRender::g_pRender->ClearBuffer(false,true);
		
		//Clear framebuffer, fixes jumpy camera in DK64, also the sun and flames glare in Zelda
		u32 * dst = (u32*)(g_pu8RamBase + g_CI.dwAddr);
		u32 * end = (u32*)(dst + (command.fillrect.y1*(g_CI.dwWidth >> 1)));

		do
		{
			*dst++ = fill_colour;
			*dst++ = fill_colour;
			*dst++ = fill_colour;
			*dst++ = fill_colour;
		} while(dst < end);

		TRACE0("Clearing ZBuffer");
		return;
	}

	if( options.enableHackForGames == HACK_FOR_MARIO_TENNIS )
	{
		uint32 dwPC = gDlistStack[gDlistStackPointer].pc;		// This points to the next instruction
		uint32 w2 = *(uint32 *)(g_pu8RamBase + dwPC);
		if( (w2>>24) == RDP_FILLRECT )
		{
			// Mario Tennis, a lot of FillRect ucodes, skip all of them
			while( (w2>>24) == RDP_FILLRECT )
			{
				dwPC += 8;
				w2 = *(uint32 *)(g_pu8RamBase + dwPC);
			}

			gDlistStack[gDlistStackPointer].pc = dwPC;
			return;
		}
	}


	// Note, in some modes, the right/bottom lines aren't drawn

	if( gRDP.otherMode.cycle_type >= CYCLE_TYPE_COPY )
	{
		command.fillrect.x1++;
		command.fillrect.y1++;
	}

	if( status.bHandleN64RenderTexture )
	{
		if( !status.bCIBufferIsRendered ) g_pFrameBufferManager->ActiveTextureBuffer();

		status.leftRendered = status.leftRendered<0 ? command.fillrect.x0 : min((int)command.fillrect.x0,status.leftRendered);
		status.topRendered = status.topRendered<0 ? command.fillrect.y0 : min((int)command.fillrect.y0,status.topRendered);
		status.rightRendered = status.rightRendered<0 ? command.fillrect.x1 : max((int)command.fillrect.x1,status.rightRendered);
		status.bottomRendered = status.bottomRendered<0 ? command.fillrect.y1 : max((int)command.fillrect.y1,status.bottomRendered);

		g_pRenderTextureInfo->maxUsedHeight = max(g_pRenderTextureInfo->maxUsedHeight,(int)command.fillrect.y1);

		if(command.fillrect.x0==0 && command.fillrect.y0==0 && (command.fillrect.x1 == g_pRenderTextureInfo->N64Width || command.fillrect.x1 == g_pRenderTextureInfo->N64Width-1 ))
		{
			if( g_pRenderTextureInfo->CI_Info.dwSize == TXT_SIZE_16b )
			{
				uint16 color = (uint16)fill_colour;
				uint32 pitch = g_pRenderTextureInfo->N64Width<<1;
				uint32 base = (uint32)(g_pu8RamBase + g_pRenderTextureInfo->CI_Info.dwAddr);
				for( uint32 i =command.fillrect.y0; i<command.fillrect.y1; i++ )
				{
					for( uint32 j=command.fillrect.x0; j<command.fillrect.x1; j++ )
					{
						*(uint16*)((base+pitch*i+j)^2) = color;
					}
				}
			}
			else
			{
				uint8 color = (uint8)fill_colour;
				uint32 pitch = g_pRenderTextureInfo->N64Width;
				uint32 base = (uint32)(g_pu8RamBase + g_pRenderTextureInfo->CI_Info.dwAddr);
				for( uint32 i=command.fillrect.y0; i<command.fillrect.y1; i++ )
				{
					for( uint32 j=command.fillrect.x0; j<command.fillrect.x1; j++ )
					{
						*(uint8*)((base+pitch*i+j)^3) = color;
					}
				}
			}
			status.bFrameBufferDrawnByTriangles = false;
		}
		else
		{
			status.bFrameBufferDrawnByTriangles = true;
		}
		status.bFrameBufferDrawnByTriangles = true;


		status.bFrameBufferIsDrawn = true;

		//if( x0==0 && y0==0 && (x1 == g_pRenderTextureInfo->N64Width || x1 == g_pRenderTextureInfo->N64Width-1 ) && gRDP.fillColor == 0)
		//{
		//	CRender::g_pRender->ClearBuffer(true,false);
		//}
		//else
		{
			if( gRDP.otherMode.cycle_type == CYCLE_TYPE_FILL )
			{
				CRender::g_pRender->FillRect(command.fillrect.x0, command.fillrect.y0, command.fillrect.x1, command.fillrect.y1, gRDP.fillColor);
			}
			else
			{
				D3DCOLOR primColor = GetPrimitiveColor();
				CRender::g_pRender->FillRect(command.fillrect.x0, command.fillrect.y0, command.fillrect.x1, command.fillrect.y1, primColor);
			}
		}

		DEBUGGER_PAUSE_AND_DUMP_COUNT_N( NEXT_FLUSH_TRI,{TRACE0("Pause after FillRect\n");});
		DEBUGGER_PAUSE_AND_DUMP_COUNT_N( NEXT_FILLRECT, {DebuggerAppendMsg("FillRect: X0=%d, Y0=%d, X1=%d, Y1=%d, Color=0x%08X", command.fillrect.x0, command.fillrect.y0, command.fillrect.x1, command.fillrect.y1, gRDP.originalFillColor);
		DebuggerAppendMsg("Pause after FillRect: Color=%08X\n", gRDP.originalFillColor);});
	}
	else
	{
		LOG_UCODE("    Filling Rectangle");
		if( frameBufferOptions.bSupportRenderTextures || frameBufferOptions.bCheckBackBufs )
		{
			if( !status.bCIBufferIsRendered ) g_pFrameBufferManager->ActiveTextureBuffer();

			status.leftRendered = status.leftRendered<0 ? command.fillrect.x0 : min((int)command.fillrect.x0,status.leftRendered);
			status.topRendered = status.topRendered<0 ? command.fillrect.y0 : min((int)command.fillrect.y0,status.topRendered);
			status.rightRendered = status.rightRendered<0 ? command.fillrect.x1 : max((int)command.fillrect.x1,status.rightRendered);
			status.bottomRendered = status.bottomRendered<0 ? command.fillrect.y1 : max((int)command.fillrect.y1,status.bottomRendered);
		}

		if( gRDP.otherMode.cycle_type == CYCLE_TYPE_FILL )
		{
			if( !status.bHandleN64RenderTexture || g_pRenderTextureInfo->CI_Info.dwSize == TXT_SIZE_16b )
			{
				CRender::g_pRender->FillRect(command.fillrect.x0, command.fillrect.y0, command.fillrect.x1, command.fillrect.y1, gRDP.fillColor);
			}
		}
		else
		{
			D3DCOLOR primColor = GetPrimitiveColor();
			//if( RGBA_GETALPHA(primColor) != 0 )
			{
				CRender::g_pRender->FillRect(command.fillrect.x0, command.fillrect.y0, command.fillrect.x1, command.fillrect.y1, primColor);
			}
		}
		DEBUGGER_PAUSE_AND_DUMP_COUNT_N( NEXT_FLUSH_TRI,{TRACE0("Pause after FillRect\n");});
		DEBUGGER_PAUSE_AND_DUMP_COUNT_N( NEXT_FILLRECT, {DebuggerAppendMsg("FillRect: X0=%d, Y0=%d, X1=%d, Y1=%d, Color=0x%08X", command.fillrect.x0, command.fillrect.y0, command.fillrect.x1, command.fillrect.y1, gRDP.originalFillColor);
		DebuggerAppendMsg("Pause after FillRect: Color=%08X\n", gRDP.originalFillColor);});
	}
}

//Nintro64 uses Sprite2d 
void RSP_RDP_Nothing(MicroCodeCommand command)
{
#ifdef _DEBUG
	if( logWarning )
	{
		TRACE0("Stack Trace");
		for( int i=0; i<gDlistStackPointer; i++ )
		{
			DebuggerAppendMsg("  %08X", gDlistStack[i].pc);
		}

		uint32 dwPC = gDlistStack[gDlistStackPointer].pc-8;
		DebuggerAppendMsg("PC=%08X",dwPC);
		DebuggerAppendMsg("Warning, unknown ucode PC=%08X: 0x%08x 0x%08x\n", dwPC, command.inst.cmd0, command.inst.cmd1);
	}
	DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_UNKNOWN_OP, {TRACE0("Paused at unknown ucode\n");})
	if( debuggerContinueWithUnknown )
	{
		return;
	}
#endif
		
	if( options.bEnableHacks )
		return;

	TriggerDPInterrupt();

	gDlistStackPointer=-1;
}


void RSP_RDP_InsertMatrix(MicroCodeCommand command)
{
	float fraction;

	UpdateCombinedMatrix();

	if ((command.inst.cmd0) & 0x20)
	{
		int x = ((command.inst.cmd0) & 0x1F) >> 1;
		int y = x >> 2;
		x &= 3;

		fraction = ((command.inst.cmd1)>>16)/65536.0f;
		gRSPworldProject.m[y][x] = (float)(int)gRSPworldProject.m[y][x];
		gRSPworldProject.m[y][x] += fraction;

		fraction = ((command.inst.cmd1)&0xFFFF)/65536.0f;
		gRSPworldProject.m[y][x+1] = (float)(int)gRSPworldProject.m[y][x+1];
		gRSPworldProject.m[y][x+1] += fraction;
	}
	else
	{
		int x = ((command.inst.cmd0) & 0x1F) >> 1;
		int y = x >> 2;
		x &= 3;

		fraction = (float)fabs(gRSPworldProject.m[y][x] - (int)gRSPworldProject.m[y][x]);
		gRSPworldProject.m[y][x] = (short)((command.inst.cmd1)>>16) + fraction;

		fraction = (float)fabs(gRSPworldProject.m[y][x+1] - (int)gRSPworldProject.m[y][x+1]);
		gRSPworldProject.m[y][x+1] = (short)((command.inst.cmd1)&0xFFFF) + fraction;
	}

	gRSP.bMatrixIsUpdated = false;
	gRSP.bCombinedMatrixIsUpdated = true;

#ifdef _DEBUG
	if( pauseAtNext && eventToPause == NEXT_MATRIX_CMD )
	{
		pauseAtNext = false;
		debuggerPause = true;
		DebuggerAppendMsg("Pause after insert matrix: %08X, %08X", command.inst.cmd0, command.inst.cmd1);
	}
	else
	{
		if( pauseAtNext && logMatrix ) 
		{
			DebuggerAppendMsg("insert matrix: %08X, %08X", command.inst.cmd0, command.inst.cmd1);
		}
	}
#endif
}

#define STORE_CI	{g_CI.dwAddr = dwNewAddr;g_CI.dwFormat = dwFmt;g_CI.dwSize = dwSiz;g_CI.dwWidth = dwWidth;g_CI.bpl=dwBpl;}

void DLParser_SetCImg(MicroCodeCommand command)
{
	uint32 dwFmt		= command.img.fmt;
	uint32 dwSiz		= command.img.siz;
	uint32 dwWidth		= command.img.width + 1;
	uint32 dwNewAddr	= RSPSegmentAddr(command.img.addr) & 0x00FFFFFF ;
	uint32 dwBpl		= dwWidth << dwSiz >> 1;

	TXTRBUF_DETAIL_DUMP(DebuggerAppendMsg("SetCImg: Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", dwNewAddr, pszImgFormat[dwFmt], pszImgSize[dwSiz], dwWidth););

	if( dwFmt == TXT_FMT_YUV || dwFmt == TXT_FMT_IA )
	{
		WARNING(TRACE4("Check me:  SetCImg Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", 
			g_CI.dwAddr, pszImgFormat[dwFmt], pszImgSize[dwSiz], dwWidth));
	}

	LOG_UCODE("    Image: 0x%08x", RSPSegmentAddr(command.inst.cmd1));
	LOG_UCODE("    Fmt: %s Size: %s Width: %d",
		pszImgFormat[dwFmt], pszImgSize[dwSiz], dwWidth);

	if( g_CI.dwAddr == dwNewAddr && g_CI.dwFormat == dwFmt && g_CI.dwSize == dwSiz && g_CI.dwWidth == dwWidth )
	{
		TXTRBUF_OR_CI_DETAIL_DUMP({
			TRACE0("Set CIMG to the same address, no change, skipped");
			DebuggerAppendMsg("Pause after SetCImg: Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", 
				g_CI.dwAddr, pszImgFormat[dwFmt], pszImgSize[dwSiz], dwWidth);
		});
		return;
	}

	if( status.bVIOriginIsUpdated == true && currentRomOptions.screenUpdateSetting==SCREEN_UPDATE_AT_1ST_CI_CHANGE )
	{
		status.bVIOriginIsUpdated=false;
		CGraphicsContext::Get()->UpdateFrame();
		TXTRBUF_OR_CI_DETAIL_DUMP(TRACE0("Screen Update at 1st CI change"););
	}

	if( options.enableHackForGames == HACK_FOR_SUPER_BOWLING )
	{
		if( dwNewAddr%0x100 == 0 )
		{
			if( dwWidth < 320 )
			{
				// Left half screen
				gRDP.scissor.left = 0;
				gRDP.scissor.right = 160;
				CRender::g_pRender->SetViewport(0, 0, 160, 240, 0xFFFF);
				CRender::g_pRender->UpdateClipRectangle();
				CRender::g_pRender->UpdateScissor();
			}
			else
			{
				gRDP.scissor.left = 0;
				gRDP.scissor.right = 320;
				CRender::g_pRender->SetViewport(0, 0, 320, 240, 0xFFFF);
				CRender::g_pRender->UpdateClipRectangle();
				CRender::g_pRender->UpdateScissor();
			}
		}
		else
		{
			// right half screen
			gRDP.scissor.left = 160;
			gRDP.scissor.right = 320;
			gRSP.nVPLeftN = 160;
			gRSP.nVPRightN = 320;
			CRender::g_pRender->UpdateClipRectangle();
			CRender::g_pRender->UpdateScissor();
			CRender::g_pRender->SetViewport(160, 0, 320, 240, 0xFFFF);
		}
	}


	if( !frameBufferOptions.bUpdateCIInfo )
	{
		STORE_CI;
		status.bCIBufferIsRendered = false;
		status.bN64IsDrawingTextureBuffer = false;

		TXTRBUF_DUMP(TRACE4("SetCImg : Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", 
			g_CI.dwAddr, pszImgFormat[dwFmt], pszImgSize[dwSiz], dwWidth));

		DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_SET_CIMG, 
		{
			DebuggerAppendMsg("Pause after SetCImg: Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", 
				dwNewAddr, pszImgFormat[dwFmt], pszImgSize[dwSiz], dwWidth);
		}
		);
		return;
	}

	SetImgInfo newCI;
	newCI.bpl = dwBpl;
	newCI.dwAddr = dwNewAddr;
	newCI.dwFormat = dwFmt;
	newCI.dwSize = dwSiz;
	newCI.dwWidth = dwWidth;

	g_pFrameBufferManager->Set_CI_addr(newCI);
}

void DLParser_SetZImg(MicroCodeCommand command)
{
	LOG_UCODE("    Image: 0x%08x", RSPSegmentAddr(command.inst.cmd1));

	uint32 dwFmt   = command.img.fmt;
	uint32 dwSiz   = command.img.siz;
	uint32 dwWidth = command.img.width + 1;
	uint32 dwAddr = RSPSegmentAddr(command.img.addr);

	if( dwAddr != g_ZI_saves[0].CI_Info.dwAddr )
	{
		g_ZI_saves[1].CI_Info.dwAddr	= g_ZI.dwAddr;
		g_ZI_saves[1].CI_Info.dwFormat	= g_ZI.dwFormat;
		g_ZI_saves[1].CI_Info.dwSize	= g_ZI.dwSize;
		g_ZI_saves[1].CI_Info.dwWidth	= g_ZI.dwWidth;
		g_ZI_saves[1].updateAtFrame = g_ZI_saves[0].updateAtFrame;

		g_ZI_saves[0].CI_Info.dwAddr	= g_ZI.dwAddr	= dwAddr;
		g_ZI_saves[0].CI_Info.dwFormat	= g_ZI.dwFormat = dwFmt;
		g_ZI_saves[0].CI_Info.dwSize	= g_ZI.dwSize	= dwSiz;
		g_ZI_saves[0].CI_Info.dwWidth	= g_ZI.dwWidth	= dwWidth;
		g_ZI_saves[0].updateAtFrame		= status.gDlistCount;
	}
	else
	{
		g_ZI.dwAddr	= dwAddr;
		g_ZI.dwFormat = dwFmt;
		g_ZI.dwSize	= dwSiz;
		g_ZI.dwWidth	= dwWidth;
	}

	DEBUGGER_IF_DUMP((pauseAtNext) ,
	{DebuggerAppendMsg("SetZImg: Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", 
	g_ZI.dwAddr, pszImgFormat[dwFmt], pszImgSize[dwSiz], dwWidth);}
	);

	DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_SET_CIMG, 
		{
			DebuggerAppendMsg("Pause after SetZImg: Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", 
				g_ZI.dwAddr, pszImgFormat[dwFmt], pszImgSize[dwSiz], dwWidth);
		}
	);
}

bool IsUsedAsDI(uint32 addr)
{
	if( addr == g_ZI_saves[0].CI_Info.dwAddr )
		return true;
	else if( addr == g_ZI_saves[1].CI_Info.dwAddr && status.gDlistCount - g_ZI_saves[1].updateAtFrame < 10 
		&& g_ZI_saves[1].CI_Info.dwAddr != 0 )
		return true;
	else
		return false;
}

void DLParser_SetCombine(MicroCodeCommand command)
{
	uint32 dwMux0 = (command.inst.cmd0)&0x00FFFFFF;
	uint32 dwMux1 = (command.inst.cmd1);
	CRender::g_pRender->SetMux(dwMux0, dwMux1);
}

void DLParser_SetFillColor(MicroCodeCommand command)
{
	gRDP.fillColor = Convert555ToRGBA(command.setcolor.fillcolor);
	gRDP.originalFillColor = (command.setcolor.color);

	LOG_UCODE("    Color5551=0x%04x = 0x%08x", (uint16)command.inst.cmd1, gRDP.fillColor);

}

void DLParser_SetFogColor(MicroCodeCommand command)
{
	CRender::g_pRender->SetFogColor( command.setcolor.r, command.setcolor.g, command.setcolor.b, command.setcolor.a );
	FOG_DUMP(TRACE1("Set Fog color: %08X", command.setcolor.color));
}

void DLParser_SetBlendColor(MicroCodeCommand command)
{
	CRender::g_pRender->SetAlphaRef(command.setcolor.a);
}


void DLParser_SetPrimColor(MicroCodeCommand command)
{
	SetPrimitiveColor( COLOR_RGBA(command.setcolor.r, command.setcolor.g, command.setcolor.b, command.setcolor.a), 
		command.setcolor.prim_min_level, command.setcolor.prim_level);
}

void DLParser_SetEnvColor(MicroCodeCommand command)
{
	SetEnvColor( COLOR_RGBA(command.setcolor.r, command.setcolor.g, command.setcolor.b, command.setcolor.a) );
}


void RDP_DLParser_Process(void)
{
	status.gRDPTime = timeGetTime();
	status.gDlistCount++;

	uint32 start = *(g_GraphicsInfo.DPC_START_REG);
	uint32 end = *(g_GraphicsInfo.DPC_END_REG);

	gDlistStackPointer=0;
	gDlistStack[gDlistStackPointer].pc = start;
	gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;

	// Check if we need to purge
	if (status.gRDPTime - status.lastPurgeTimeTime > 5000)
	{
		gTextureManager.PurgeOldTextures();
		status.lastPurgeTimeTime = status.gRDPTime;
	}
	
	// Lock the graphics context here.
	CRender::g_pRender->SetFillMode(RICE_FILLMODE_SOLID);

	SetVIScales();

	CRender::g_pRender->RenderReset();
	CRender::g_pRender->BeginRendering();
	CRender::g_pRender->SetViewport(0, 0, windowSetting.uViWidth, windowSetting.uViHeight, 0x3FF);

	while( gDlistStack[gDlistStackPointer].pc < end )
	{
		MicroCodeCommand *p_command = (MicroCodeCommand*)&g_pu32RamBase[(gDlistStack[gDlistStackPointer].pc>>2)];
		gDlistStack[gDlistStackPointer].pc += 8;
		currentUcodeMap[p_command->inst.cmd0 >>24](*p_command);
	}

	CRender::g_pRender->EndRendering();
}

//You will never see these any HLE emulation, and since where a HLE plugin ignore them completely
void DLParser_TriRSP(MicroCodeCommand command)
{
	LOG_UCODE("DLParser_TriRSP: (Ignored)");
}

static int crc_table_empty = 1;
static ULONG crc_table[256];
static void make_crc_table(void);

static void make_crc_table()
{
  ULONG c;
  int n, k;
  ULONG poly;            /* polynomial exclusive-or pattern */
  /* terms of polynomial defining this crc (except x^32): */
  static const uint8 p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

  /* make exclusive-or pattern from polynomial (0xedb88320L) */
  poly = 0L;
  for (n = 0; n < sizeof(p)/sizeof(uint8); n++)
    poly |= 1L << (31 - p[n]);
 
  for (n = 0; n < 256; n++)
  {
    c = (ULONG)n;
    for (k = 0; k < 8; k++)
      c = (c & 1) ? (poly ^ (c >> 1)) : c >> 1;
    crc_table[n] = c;
  }
  crc_table_empty = 0;
}

/* ========================================================================= */
#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

/* ========================================================================= */
ULONG ComputeCRC32(ULONG crc, const uint8 *buf, UINT len)
{
    if (buf == NULL) return 0L;

    if (crc_table_empty)
      make_crc_table();

    crc = crc ^ 0xffffffffL;
    while (len >= 8)
    {
      DO8(buf);
      len -= 8;
    }
    if (len) do {
      DO1(buf);
    } while (--len);
    return crc ^ 0xffffffffL;
}

Matrix matToLoad;
void LoadMatrix(uint32 addr)
{
	if (addr + 64 > g_dwRamSize)
	{
		TRACE1("Mtx: Address invalid (0x%08x)", addr);
		return;
	}

	int i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++) 
		{
			int     hi = *(short *)(g_pu8RamBase + ((addr+(i<<3)+(j<<1)     )^0x2));
			uint16  lo = *(uint16  *)(g_pu8RamBase + ((addr+(i<<3)+(j<<1) + 32)^0x2));
			matToLoad.m[i][j] = (float)((hi<<16) | (lo))/ 65536.0f;
		}
	}


#ifdef _DEBUG
	LOG_UCODE(
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\r\n"
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\r\n"
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\r\n"
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\r\n",
		matToLoad.m[0][0], matToLoad.m[0][1], matToLoad.m[0][2], matToLoad.m[0][3],
		matToLoad.m[1][0], matToLoad.m[1][1], matToLoad.m[1][2], matToLoad.m[1][3],
		matToLoad.m[2][0], matToLoad.m[2][1], matToLoad.m[2][2], matToLoad.m[2][3],
		matToLoad.m[3][0], matToLoad.m[3][1], matToLoad.m[3][2], matToLoad.m[3][3]);
#endif // _DEBUG
}
