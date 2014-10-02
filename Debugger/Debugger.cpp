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

#include "..\stdafx.h"
#include "..\_BldNum.h"

#ifdef _DEBUG

void DumpMatrix2(const Matrix &mtx, const char* prompt);

bool debuggerWinOpened = false;
bool debuggerDrawRenderTexture = false;
int debuggerDrawRenderTextureNo = 0;

bool logCombiners = false;
bool logWarning = true;
bool logTriangles = false;
bool logMatrix = false;
bool logVertex = false;
bool logTextures = false;
bool logTextureBuffer = false;
bool logToScreen = true;
bool logToFile = false;
bool logUcodes = false;
bool logMicrocode = false;
bool logFog = false;
bool logDetails = false;

FILE *logFp = NULL;

bool debuggerEnableTexture=true;
bool debuggerEnableZBuffer=true;
bool debuggerEnableCullFace=true;
bool debuggerEnableTestTris=true;
bool debuggerEnableAlphaTest=true;
bool debuggerContinueWithUnknown=false;

bool debuggerPause = false;
bool pauseAtNext = false;
int  eventToPause = 0;
int  debuggerPauseCount = 0;
int  countToPause = 0;

bool debuggerDropCombiners=false;
bool debuggerDropGeneralCombiners=false;
bool debuggerDropDecodedMux=false;
bool debuggerDropCombinerInfos=false;

char msgBuf[0x20000+2];
bool msgBufUpdated = false;
static HWND myDialogWnd = NULL;
extern FiddledVtx * g_pVtxBase;
HWND hWndDlg = NULL;

bool debuggerWinActive = false;
CTexture *surfTlut = NULL;

uint32 CachedTexIndex = 0;

char* otherNexts[] = {
	"Frame",
	"Flush Tri",
	"TextRect",
	"Triangle",
	"Set CImg",
	"ObjTxt Cmd",
	"Obj BG",
	"Sprite2D",
	"FillRect",
	"DList",
	"Ucode",
	"Texture Buffer",
	"Matrix Cmd",
	"Vertex Cmd",
	"New Texture",
	"Set Texture",
	"Mux",
	"Set Light",
	"Set Mode Cmd",
	"Set Prim Color",
	"Texture Cmd",
	"Unknown Ops",
	"Scale Image",
	"LoadTlut",
	"Ucode Switching",
};
int numberOfNextOthers = sizeof(otherNexts)/sizeof(char*);

char* thingsToDump[] = {
	"Cur Texture RGBA",
	"Cur+1 Texture RGBA",
	"Colors",
	"Memory At",
	"Mux",
	"Simple Mux",
	"Other Modes",
	"Texture #",
	"Tile #",
	"VI Regs",
	"Cur Txt to file",
	"Cur+1 Txt to file",
	"Cur Texture RGB",
	"Cur Texture Alpha",
	"Cur+1 Texture RGB",
	"Cur+1 Texture Alpha",
	"Light Info",
	"Tlut",
	"Obj Tlut",
	"Vertexes",
	"Cached Texture",
	"Next Texture",
	"Prev Texture",
	"Dlist At",
	"Matrix At",
	"Combined Matrix",
	"World Top Matrix",
	"Projection Matrix",
	"World Matrix #",
	"Frame Buffer in RDRAM",
	"BackBuffer",
	"TexBuffer #",
};
int numberOfThingsToDump = sizeof(thingsToDump)/sizeof(char*);

enum {
	DUMP_CUR_TEXTURE_RGBA,
	DUMP_CUR_1_TEXTURE_RGBA,
	DUMP_COLORS,
	DUMP_CONTENT_AT,
	DUMP_CUR_MUX,
	DUMP_SIMPLE_MUX,
	DUMP_OTHER_MODE,
	DUMP_TEXTURE_AT,
	DUMP_TILE_AT,
	DUMP_VI_REGS,
	DUMP_CUR_TEXTURE_TO_FILE,
	DUMP_CUR_1_TEXTURE_TO_FILE,
	DUMP_CUR_TEXTURE_RGB,
	DUMP_CUR_TEXTURE_ALPHA,
	DUMP_CUR_1_TEXTURE_RGB,
	DUMP_CUR_1_TEXTURE_ALPHA,
	DUMP_LIGHT,
	DUMP_TLUT,
	DUMP_OBJ_TLUT,
	DUMP_VERTEXES,
	DUMP_CACHED_TEX,
	DUMP_NEXT_TEX,
	DUMP_PREV_TEX,
	DUMP_DLIST_AT,
	DUMP_MATRIX_AT,
	DUMP_COMBINED_MATRIX,
	DUMP_WORLD_TOP_MATRIX,
	DUMP_PROJECTION_MATRIX,
	DUMP_WORLD_MATRIX_AT,
	DUMP_FRAME_BUFFER,
	DUMP_BACKBUFFER,
	DUMP_TEXBUFFER_AT,
};

//---------------------------------------------------------------------
void DumpVIRegisters(void)
{
	DebuggerAppendMsg("----VI Registers----\nSTATUS:\t%08X\nORIGIN:\t%08X\nWIDTH:\t%08X\n\
V_SYNC:\t%08X\nH_SYNC:\t%08X\nX_SCALE:\t%08X\nY_SCALE:\t%08X\n\
H_START:\t%08X\nV_START:\t%08X\nVI Width=%f(x %f), VI Height=%f(x %f)\n\n",
		*g_GraphicsInfo.VI_STATUS_REG, *g_GraphicsInfo.VI_ORIGIN_REG, *g_GraphicsInfo.VI_WIDTH_REG, *g_GraphicsInfo.VI_V_SYNC_REG,
		*g_GraphicsInfo.VI_H_SYNC_REG, *g_GraphicsInfo.VI_X_SCALE_REG, *g_GraphicsInfo.VI_Y_SCALE_REG,
		*g_GraphicsInfo.VI_H_START_REG, *g_GraphicsInfo.VI_V_START_REG, windowSetting.fViWidth,windowSetting.fMultX,
		windowSetting.fViHeight,windowSetting.fMultY);
	DebuggerAppendMsg("Scissor: x0=%d y0=%d x1=%d y1=%d mode=%d",
		gRDP.scissor.left, gRDP.scissor.top,
		gRDP.scissor.right, gRDP.scissor.bottom,
		gRDP.scissor.mode);
	DebuggerAppendMsg("Effective scissor: x0=%d y0=%d x1=%d y1=%d",
		gRSP.real_clip_scissor_left, gRSP.real_clip_scissor_top,
		gRSP.real_clip_scissor_right, gRSP.real_clip_scissor_bottom);
	DebuggerAppendMsg("Clipping: (%d) left=%f top=%f right=%f bottom=%f",
		gRSP.clip_ratio_posx, gRSP.real_clip_ratio_negx , gRSP.real_clip_ratio_negy,
		gRSP.real_clip_ratio_posx, gRSP.real_clip_ratio_posy);
	DebuggerAppendMsg("Viewport: left=%d top=%d right=%d bottom=%d",
		gRSP.nVPLeftN, gRSP.nVPTopN , gRSP.nVPRightN,
		gRSP.nVPBottomN);
	DebuggerAppendMsg("Current CImg: Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", 
		g_CI.dwAddr, pszImgFormat[g_CI.dwFormat], pszImgSize[g_CI.dwSize], g_CI.dwWidth);
	DebuggerAppendMsg("Current ZImg: Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", 
		g_ZI.dwAddr, pszImgFormat[g_ZI.dwFormat], pszImgSize[g_ZI.dwSize], g_ZI.dwWidth);
}

void DumpVertexArray(void)
{
	DebuggerAppendMsg("----Vertexes----\n");
	try{
		for( int i=0; i<32; i++ )
		{
			FiddledVtx &v = g_pVtxBase[i];
			DebuggerAppendMsg("[%d] x=%d,y=%d,z=%d -- r=%d,g=%d,b=%d,a=%d\n", i, v.x, v.y, v.z, 
				v.rgba.r, v.rgba.g, v.rgba.b, v.rgba.a);
			DebuggerAppendMsg("\tx=%f,y=%f,z=%f,rhw=%f\n", g_vecProjected[i].x, g_vecProjected[i].y, g_vecProjected[i].z, g_vecProjected[i].w);
		}
	}catch(...)
	{
		DebuggerAppendMsg("Invalid memory pointer of vertex array\n");
	}
}

void DumpHex(uint32 rdramAddr, int count);
uint32 StrToHex(char *HexStr);

void DumpTileInfo(uint32 dwTile)
{
	char *pszOnOff[2]     = {"Off", "On"};

	DebuggerAppendMsg("Tile: %d\nFmt: %s/%s Line:%d (Pitch %d) TMem:0x%04x Palette:%d\n",
		dwTile, pszImgFormat[gRDP.tiles[dwTile].dwFormat], pszImgSize[gRDP.tiles[dwTile].dwSize],
		gRDP.tiles[dwTile].dwLine, gRDP.tiles[dwTile].dwPitch, gRDP.tiles[dwTile].dwTMem, gRDP.tiles[dwTile].dwPalette);
	DebuggerAppendMsg("S: Clamp: %s Mirror:%s Mask:0x%x Shift:0x%x\n",
		pszOnOff[gRDP.tiles[dwTile].bClampS],pszOnOff[gRDP.tiles[dwTile].bMirrorS],
		gRDP.tiles[dwTile].dwMaskS, gRDP.tiles[dwTile].dwShiftS);
	DebuggerAppendMsg("T: Clamp: %s Mirror:%s Mask:0x%x Shift:0x%x\n",
		pszOnOff[gRDP.tiles[dwTile].bClampT],pszOnOff[gRDP.tiles[dwTile].bMirrorT],
		gRDP.tiles[dwTile].dwMaskT, gRDP.tiles[dwTile].dwShiftT);
	DebuggerAppendMsg("(%d,%d) -> (%d,%d), hilite [%d, %d]\n",
		gRDP.tiles[dwTile].sl, gRDP.tiles[dwTile].tl, gRDP.tiles[dwTile].sh, gRDP.tiles[dwTile].th,
		gRDP.tiles[dwTile].hilite_sl,gRDP.tiles[dwTile].hilite_tl);
}

void DumpTexture(int tex, TextureChannel channel = TXT_RGB )
{
	CRender::GetRender()->DrawTexture(tex, channel);
}

void DumpRenderTexture(int tex=-1)
{
	g_pFrameBufferManager->DisplayRenderTexture(tex);
}

void DumpTextureToFile(int tex, TextureChannel channel = TXT_RGB)
{
	//CRender::GetRender()->SaveTextureToFile(tex, channel,false);
	//DUMP WITH DIRECTX
}

void DumpTlut(uint16* palAddr)
{
	for( uint32 i=0; i<0x100; i+=8 )
	{
		DebuggerAppendMsg("0x%4X 0x%4X 0x%4X 0x%4X 0x%4X 0x%4X 0x%4X 0x%4X ", 
			g_wRDPTlut[i], g_wRDPTlut[i+1], g_wRDPTlut[i+2], g_wRDPTlut[i+2], 
			g_wRDPTlut[i+4], g_wRDPTlut[i+5], g_wRDPTlut[i+6], g_wRDPTlut[i+7]);
	}
}

void DumpDlistAt(uint32 dwPC)
{
	uint32 word0, word1, opcode;
	LPCSTR *name;


	DebuggerAppendMsg("\n\n");
	//if( dwPC>100 ) dwPC -= 40;
	for( uint32 i=0; i<20; i++)
	{
		word0 = g_pu32RamBase[(dwPC>>2)+0];
		word1 = g_pu32RamBase[(dwPC>>2)+1];
		opcode = word0>>24;
		DebuggerAppendMsg("%08X: %08X, %08X - %s", dwPC, word0, word1, name[opcode] );
		dwPC+=8;
	}
	DebuggerAppendMsg("\n\n");
}

void DumpMatrixAt(uint32 dwPC)
{
	Matrix mat;
	uint32 dwI;
	uint32 dwJ;
	const float fRecip = 1.0f / 65536.0f;

	for (dwI = 0; dwI < 4; dwI++) {
		for (dwJ = 0; dwJ < 4; dwJ++) {

			short nDataHi = *(short *)(g_pu8RamBase + ((dwPC+(dwI<<3)+(dwJ<<1)     )^0x2));
			uint16  nDataLo = *(uint16  *)(g_pu8RamBase + ((dwPC+(dwI<<3)+(dwJ<<1) + 32)^0x2));

			mat.m[dwI][dwJ] = (float)(((LONG)nDataHi<<16) | (nDataLo))*fRecip;
		}
	}
	TRACE0("Dump Matrix in Memory");
	DebuggerAppendMsg(
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
		mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
}

// High
static const char *alphadithertypes[4]	= {"Pattern", "NotPattern", "Noise", "Disable"};
static const char *rgbdithertype[4]		= {"MagicSQ", "Bayer", "Noise", "Disable"};
static const char *convtype[8]			= {"Conv", "?", "?", "?",   "?", "FiltConv", "Filt", "?"};
static const char *filtertype[4]		= {"Point", "?", "Bilinear", "Average"};
static const char *cycletype[4]			= {"1Cycle", "2Cycle", "Copy", "Fill"};
static const char *detailtype[4]		= {"Clamp", "Sharpen", "Detail", "?"};
static const char *alphacomptype[4]		= {"None", "Threshold", "?", "Dither"};
static const char * szCvgDstMode[4]		= { "Clamp", "Wrap", "Full", "Save" };
static const char * szZMode[4]			= { "Opa", "Inter", "XLU", "Decal" };
static const char * szZSrcSel[2]		= { "Pixel", "Primitive" };
static const char * sc_szBlClr[4]		= { "In", "Mem", "Bl", "Fog" };
static const char * sc_szBlA1[4]		= { "AIn", "AFog", "AShade", "0" };
static const char * sc_szBlA2[4]		= { "1-A", "AMem", "1", "0" };

void DumpOtherMode()
{
	uint16 blender = gRDP.otherMode.blender;
	RDP_BlenderSetting &bl = *(RDP_BlenderSetting*)(&(blender));
	DebuggerAppendMsg( "Other Modes");
	DebuggerAppendMsg( "\talpha_compare:\t%s", alphacomptype[ gRDP.otherMode.alpha_compare ]);
	DebuggerAppendMsg( "\tdepth_source:\t%s", szZSrcSel[ gRDP.otherMode.depth_source ]);
	DebuggerAppendMsg( "\taa_en:\t\t%d", gRDP.otherMode.aa_en );
	DebuggerAppendMsg( "\tz_cmp:\t\t%d", gRDP.otherMode.z_cmp );
	DebuggerAppendMsg( "\tz_upd:\t\t%d", gRDP.otherMode.z_upd );
	DebuggerAppendMsg( "\tim_rd:\t\t%d", gRDP.otherMode.im_rd );
	DebuggerAppendMsg( "\tclr_on_cvg:\t%d", gRDP.otherMode.clr_on_cvg );
	DebuggerAppendMsg( "\tcvg_dst:\t\t%s", szCvgDstMode[ gRDP.otherMode.cvg_dst ] );
	DebuggerAppendMsg( "\tzmode:\t\t%s", szZMode[ gRDP.otherMode.zmode ] );
	DebuggerAppendMsg( "\tcvg_x_alpha:\t%d", gRDP.otherMode.cvg_x_alpha );
	DebuggerAppendMsg( "\talpha_cvg_sel:\t%d", gRDP.otherMode.alpha_cvg_sel );
	DebuggerAppendMsg( "\tforce_bl:\t\t%d", gRDP.otherMode.force_bl );
	DebuggerAppendMsg( "\ttex_edge:\t\t%d", gRDP.otherMode.tex_edge );
	DebuggerAppendMsg( "\tblender:\t\t%04x - Cycle1:\t%s * %s + %s * %s\n\t\t\tCycle2:\t%s * %s + %s * %s", gRDP.otherMode.blender,
		sc_szBlClr[bl.c1_m1a], sc_szBlA1[bl.c1_m1b], sc_szBlClr[bl.c1_m2a], sc_szBlA2[bl.c1_m2b],
		sc_szBlClr[bl.c2_m1a], sc_szBlA1[bl.c2_m1b], sc_szBlClr[bl.c2_m2a], sc_szBlA2[bl.c2_m2b]);
	DebuggerAppendMsg( "\tblend_mask:\t%d", gRDP.otherMode.blend_mask );
	DebuggerAppendMsg( "\talpha_dither:\t%s", alphadithertypes[ gRDP.otherMode.alpha_dither ] );
	DebuggerAppendMsg( "\trgb_dither:\t\t%s", rgbdithertype[ gRDP.otherMode.rgb_dither ] );
	DebuggerAppendMsg( "\tcomb_key:\t%s", gRDP.otherMode.key_en ? "Key" : "None" );
	DebuggerAppendMsg( "\ttext_conv:\t\t%s", convtype[ gRDP.otherMode.text_conv ] );
	DebuggerAppendMsg( "\ttext_filt:\t\t%s", filtertype[ gRDP.otherMode.text_filt ] );
	DebuggerAppendMsg( "\ttext_tlut:\t\t%s", textluttype[ gRDP.otherMode.text_tlut ] );
	DebuggerAppendMsg( "\ttext_lod:\t\t%s", gRDP.otherMode.text_lod ? "Yes": "No" );
	DebuggerAppendMsg( "\ttext_detail:\t\t%s", gRDP.otherMode.text_detail ? "Yes": "No" );
	DebuggerAppendMsg( "\ttext_sharpen:\t\t%s", gRDP.otherMode.text_sharpen ? "Yes": "No" );
	DebuggerAppendMsg( "\ttext_persp:\t%s", gRDP.otherMode.text_persp ? "On" : "Off" );
	DebuggerAppendMsg( "\tcycle_type:\t%s", cycletype[ gRDP.otherMode.cycle_type ] );
	DebuggerAppendMsg( "\tpipeline:\t\t%s", gRDP.otherMode.atomic_prim ? "1Primitive" : "NPrimitive" );
	DebuggerAppendMsg("\n\nSP render flags:");
	DebuggerAppendMsg("\tCull mode: %s%s", gRDP.tnl.TriCull?"Cull Front":"", gRDP.tnl.CullBack?" Cull Back":"");
	DebuggerAppendMsg("\tShade mode: %d", gRSP.shadeMode);
	DebuggerAppendMsg("\tFog: %s", gRDP.tnl.Fog?"enabled":"disabled");
	DebuggerAppendMsg("\tZbuffer in SP: %s", gRDP.tnl.Zbuffer?"enabled":"disabled");
	DebuggerAppendMsg("\tLighting: %s", gRDP.tnl.Light?"enabled":"disabled");
	DebuggerAppendMsg("\\Number of lights: %d", gRSPnumLights);
	DebuggerAppendMsg("\tTexture Gen: %s", gRDP.tnl.TexGen?"enabled":"disabled");
	DebuggerAppendMsg("\tTexture Gen Linear: %s", (gRDP.tnl.TexGenLin)?"enabled":"disabled");
}

void DumpCachedTexture(uint32 index)
{
	TxtrCacheEntry *p = gTextureManager.GetCachedTexture(index);
	if( p != NULL )
	{
		char filename[80];
		sprintf(filename,"\\Texture%d_rgb", index);
		//D3DXSaveTextureToFile(filename, D3DXIFF_PNG, *(p->pTexture)->GetTexture(), NULL);

		DebuggerAppendMsg("Display cached texture #%d of %d\n", index, gTextureManager.GetNumOfCachedTexture());
		DebuggerAppendMsg("W:%d, H:%d, RealW:%d, RealH:%d, D3DW:%d, D3DH: %d", p->ti.WidthToCreate, p->ti.HeightToCreate,
			p->ti.WidthToLoad, p->ti.HeightToLoad, p->pTexture->m_dwCreatedTextureWidth, p->pTexture->m_dwCreatedTextureHeight);
	}
	else
	{
		DebuggerAppendMsg("No cached texture at index = %d\n", index);
	}
}

uint32 GetInputHex()
{
	char buf[30];
	GetDlgItemText(hWndDlg,IDC_BREAK_AT_COUNT, buf, 29);
	return StrToHex(buf);
}

extern uint32 gObjTlutAddr;
void DumpInfo(int thingToDump)
{
	uint32 i;
	switch(thingToDump)
	{
	case DUMP_COLORS:
		DebuggerAppendMsg("----Colors----\nPrim Color:\t%08X\nEnv Color:\t%08X\n"
			"Fill Color:\t%08X\nFog Color:\t%08X\n"
			"Prim Depth:\t%f\nPrim LOD Frac:\t%08X\n",
		GetPrimitiveColor(), GetEnvColor(), gRDP.fillColor,
		CRender::GetRender()->GetFogColor(), GetPrimitiveDepth(), GetLODFrac());
		break;
	case DUMP_CUR_MUX:
		CRender::GetRender()->m_pColorCombiner->DisplayMuxString();
		break;
	case DUMP_LIGHT:
		DebuggerAppendMsg("----Light Colors----\nNumber of Lights: %d\n",GetNumLights());
		for( i=0; i<GetNumLights()+2; i++)
		{
		//	DebuggerAppendMsg("Light %d:\t%08X, (%d,%d,%d)\n", i, gRSPn64lights[i].dwRGBA,gRSPn64lights[i].x,gRSPn64lights[i].y,gRSPn64lights[i].z );
		}
		break;
	case DUMP_TEXTURE_AT:
		{
			int txt = GetInputHex();
			if( txt < 8 )	DumpTexture(txt);
		}
		break;
	case DUMP_CUR_TEXTURE_RGBA:
		DumpTexture(gRSP.curTile, TXT_RGBA);
		break;
	case DUMP_CUR_1_TEXTURE_RGBA:
		DumpTexture((1+gRSP.curTile)%7, TXT_RGBA);
		break;
	case DUMP_CUR_TEXTURE_RGB:
		DumpTexture(gRSP.curTile, TXT_RGB);
		break;
	case DUMP_CUR_1_TEXTURE_RGB:
		DumpTexture((1+gRSP.curTile)%7, TXT_RGB);
		break;
	case DUMP_CUR_TEXTURE_TO_FILE:
		DumpTextureToFile(0,TXT_RGB);
		DumpTextureToFile(0,TXT_ALPHA);
		DumpTextureToFile(0,TXT_RGBA);
		break;
	case DUMP_CUR_1_TEXTURE_TO_FILE:
		DumpTextureToFile(1,TXT_RGB);
		DumpTextureToFile(1,TXT_ALPHA);
		DumpTextureToFile(1,TXT_RGBA);
		break;
	case DUMP_CUR_TEXTURE_ALPHA:
		DumpTexture(0, TXT_ALPHA);
		break;
	case DUMP_CUR_1_TEXTURE_ALPHA:
		DumpTexture(1, TXT_ALPHA);
		break;
	case DUMP_TLUT:
		DumpTlut(g_wRDPTlut);
		break;
	case DUMP_OBJ_TLUT:
		DumpTlut((uint16*)(g_pu8RamBase+gObjTlutAddr));
		break;
	case DUMP_TILE_AT:
		{
			int tile =GetInputHex();
			if( tile < 8 )	DumpTileInfo(tile);
		}
		break;
	case DUMP_VERTEXES:
		DumpVertexArray();
		break;
	case DUMP_VI_REGS:
		DumpVIRegisters();
		break;
	case DUMP_SIMPLE_MUX:
		CRender::GetRender()->m_pColorCombiner->DisplaySimpleMuxString();
		break;
	case DUMP_OTHER_MODE:
		DumpOtherMode();
		break;
	case DUMP_FRAME_BUFFER:
		CRender::GetRender()->DrawFrameBuffer(true);
		break;
	case DUMP_CONTENT_AT:
		{
			uint32 addr=GetInputHex();
			if( addr < g_dwRamSize )
			{
				DumpHex(addr, min(32, g_dwRamSize-addr));
			}
		}
		break;
	case DUMP_DLIST_AT:
		{
			uint32 addr=GetInputHex();
			addr &= (g_dwRamSize-1);
			DumpDlistAt(addr);
		}
		break;
	case DUMP_MATRIX_AT:
		{
			uint32 addr=GetInputHex();
			addr &= (g_dwRamSize-1);
			DumpMatrixAt(addr);
		}
		break;
	case DUMP_NEXT_TEX:
		CachedTexIndex++;
		if( CachedTexIndex >= gTextureManager.GetNumOfCachedTexture() )
		{
			CachedTexIndex = 0;
		}
		DumpCachedTexture(CachedTexIndex);
		break;
	case DUMP_PREV_TEX:		
		CachedTexIndex--;
		if( CachedTexIndex < 0 || CachedTexIndex >= gTextureManager.GetNumOfCachedTexture() )
			CachedTexIndex = 0;
		DumpCachedTexture(CachedTexIndex);
		break;
	case DUMP_CACHED_TEX:
		DumpCachedTexture(CachedTexIndex);
		break;
	case DUMP_TEXBUFFER_AT:
		{
			int bufno = GetInputHex();
			if( bufno < 20 ) DumpRenderTexture(bufno);
		}
		break;
	case DUMP_COMBINED_MATRIX:
		DumpMatrix2(gRSPworldProject,"Combined Matrix");
		break;
	case DUMP_WORLD_TOP_MATRIX:
		DumpMatrix2(gRSP.modelviewMtxs[gRSP.modelViewMtxTop],"World Top Matrix");
		break;
	case DUMP_WORLD_MATRIX_AT:
		{
			int mtxno = GetInputHex();
			if( mtxno < 4 ) DumpMatrix2(gRSP.modelviewMtxs[0],"World Matrix #");
		}
		break;
	case DUMP_PROJECTION_MATRIX:
		DumpMatrix2(gRSP.projectionMtxs[gRSP.projectionMtxTop],"Projection Top Matrix");
		break;
	}
}


void SetLogToFile(bool log)
{
	if( log )
	{
		if( logFp == NULL )
		{
			logFp = fopen("\\RiceVideo.log", "at");
		}
	}
	else
	{
		if( logFp != NULL )
		{
			fclose(logFp);
			logFp = NULL;
		}
	}
}

//=====================================================================================
LRESULT APIENTRY DebuggerDialog(HWND hDlg, unsigned message, LONG wParam, LONG lParam)
{
	int i;
	int thingToDump;
	char temp[300];
	hWndDlg = hDlg;
	switch(message)
	{
	case WM_INITDIALOG:
		myDialogWnd = hDlg;
		sprintf(temp,"%s Debug %s",project_name, BUILD_NUMBER) ;
		SetWindowText(myDialogWnd, temp );
		SendDlgItemMessage(hDlg, IDC_LOG_COMBINERS, BM_SETCHECK, logCombiners ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_TRIANGLES, BM_SETCHECK, logTriangles ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_TEXTURE_BUFFER, BM_SETCHECK, logTextureBuffer ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_VTX, BM_SETCHECK, logVertex ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_TO_SCREEN, BM_SETCHECK, logToScreen ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_TO_FILE, BM_SETCHECK, logToFile ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_MATRIX, BM_SETCHECK, logMatrix ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_ENABLE_TEXTURE, BM_SETCHECK, debuggerEnableTexture ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_ZBUFFER, BM_SETCHECK, debuggerEnableZBuffer ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_CULLFACE, BM_SETCHECK, debuggerEnableCullFace ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_TEST_TRIS, BM_SETCHECK, debuggerEnableTestTris ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_ALPHA_TEST, BM_SETCHECK, debuggerEnableAlphaTest ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_WARNING, BM_SETCHECK, logWarning ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_TEXTURES, BM_SETCHECK, logTextures ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_UCODE, BM_SETCHECK, logUcodes ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_MICROCODE, BM_SETCHECK, logMicrocode ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_CONT_W_UNKNOWN, BM_SETCHECK, debuggerContinueWithUnknown ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_LOG_FOG, BM_SETCHECK, logFog ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_DETAILS, BM_SETCHECK, logDetails ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(hDlg, IDC_WINFRAME, BM_SETCHECK, options.bWinFrameMode ? BST_CHECKED : BST_UNCHECKED, 0);

		SetDlgItemText(hDlg, IDC_MSG, "");
		SetDlgItemText(hDlg, IDC_BREAK_AT_COUNT, "");
		SetDlgItemText(hDlg, IDC_BREAK_AT_MUX, "");
		
		for( i=0; i<numberOfNextOthers; i++ )
		{ 
			SendDlgItemMessage(hDlg, IDC_NEXT_EVENT_TYPE, CB_INSERTSTRING, i, (LPARAM) otherNexts[i]);
		}
		SendDlgItemMessage(hDlg, IDC_NEXT_EVENT_TYPE, CB_SETCURSEL, NEXT_FLUSH_TRI, 0);

		for( i=0; i<numberOfThingsToDump; i++ )
		{ 
			SendDlgItemMessage(hDlg, IDC_THING_TO_DUMP, CB_INSERTSTRING, i, (LPARAM) thingsToDump[i]);
		}
		SendDlgItemMessage(hDlg, IDC_THING_TO_DUMP, CB_SETCURSEL, DUMP_CUR_TEXTURE_RGBA, 0);

		SetLogToFile(logToFile);

		msgBuf[0] = '\0';
		return(true);
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_LOG_COMBINERS:
			logCombiners = (SendDlgItemMessage(hDlg, IDC_LOG_COMBINERS, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_LOG_TRIANGLES:
			logTriangles = (SendDlgItemMessage(hDlg, IDC_LOG_TRIANGLES, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_LOG_TEXTURE_BUFFER:
			logTextureBuffer = (SendDlgItemMessage(hDlg, IDC_LOG_TEXTURE_BUFFER, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_LOG_VTX:
			logVertex = (SendDlgItemMessage(hDlg, IDC_LOG_VTX, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_LOG_TO_SCREEN:
			logToScreen = (SendDlgItemMessage(hDlg, IDC_LOG_TO_SCREEN, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_LOG_TO_FILE:
			logToFile = (SendDlgItemMessage(hDlg, IDC_LOG_TO_FILE, BM_GETCHECK, 0, 0) == BST_CHECKED);
			SetLogToFile(logToFile);
			return(true);
			break;
		case IDC_LOG_MATRIX:
			logMatrix = (SendDlgItemMessage(hDlg, IDC_LOG_MATRIX, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_ENABLE_TEXTURE:
			debuggerEnableTexture = (SendDlgItemMessage(hDlg, IDC_ENABLE_TEXTURE, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_ZBUFFER:
			debuggerEnableZBuffer = (SendDlgItemMessage(hDlg, IDC_ZBUFFER, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_CULLFACE:
			debuggerEnableCullFace = (SendDlgItemMessage(hDlg, IDC_CULLFACE, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_TEST_TRIS:
			debuggerEnableTestTris = (SendDlgItemMessage(hDlg, IDC_TEST_TRIS, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_ALPHA_TEST:
			debuggerEnableAlphaTest = (SendDlgItemMessage(hDlg, IDC_ALPHA_TEST, BM_GETCHECK, 0, 0) == BST_CHECKED);
			if( !debuggerEnableAlphaTest && CRender::g_pRender )	
				CRender::g_pRender->SetAlphaTestEnable(false);
			return(true);
			break;
		case IDC_LOG_WARNING:
			logWarning = (SendDlgItemMessage(hDlg, IDC_LOG_WARNING, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_LOG_FOG:
			logFog = (SendDlgItemMessage(hDlg, IDC_LOG_FOG, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_DETAILS:
			logDetails = (SendDlgItemMessage(hDlg, IDC_DETAILS, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_WINFRAME:
			options.bWinFrameMode = (SendDlgItemMessage(hDlg, IDC_WINFRAME, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_LOG_TEXTURES:
			logTextures = (SendDlgItemMessage(hDlg, IDC_LOG_TEXTURES, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_LOG_UCODE:
			logUcodes = (SendDlgItemMessage(hDlg, IDC_LOG_UCODE, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_LOG_MICROCODE:
			logMicrocode = (SendDlgItemMessage(hDlg, IDC_LOG_MICROCODE, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_CONT_W_UNKNOWN:
			debuggerContinueWithUnknown = (SendDlgItemMessage(hDlg, IDC_CONT_W_UNKNOWN, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return(true);
			break;
		case IDC_CLEAR:
			msgBuf[0] = '\0';
			SetDlgItemText(hDlg, IDC_MSG, "");
			break;
		case IDOK:
			EndDialog(hDlg, true);
			return(true);
		case IDCANCEL:
			EndDialog(hDlg, true);
			return(true);
		}

		if( !status.bGameIsRunning )
		{
			return(true);
		}

		switch(LOWORD(wParam))
		{
		case IDC_PAUSE:
			debuggerPause = true;
			break;
		case IDC_GO:
			pauseAtNext = false;
			debuggerPause = false;
			logTriangles = false;
			logVertex = false;
			logMatrix = false;
			logTextures = false;
			logUcodes = false;
			logMicrocode = false;

			SendDlgItemMessage(hDlg, IDC_LOG_TRIANGLES, BM_SETCHECK, BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_LOG_VTX, BM_SETCHECK, BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_LOG_TEXTURES, BM_SETCHECK, BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_LOG_UCODE, BM_SETCHECK, BST_UNCHECKED, 0);

			break;
		case IDC_NEXT:	//Next one of the others
			debuggerPause = false;
			pauseAtNext = true;
			eventToPause = SendDlgItemMessage(hDlg, IDC_NEXT_EVENT_TYPE, CB_GETCURSEL, 0, 0);
			{
				char text[30];
				GetDlgItemText(hDlg,IDC_BREAK_AT_COUNT, text, 29);
				if( strlen(text)>0 )
				{
					countToPause = atoi(text);
				}
				else
				{
					countToPause = 0;
				}
				if( countToPause > 0x1000 )	
					countToPause=0;
				debuggerPauseCount = countToPause;
			}
			break;
		case IDC_DUMP:
			thingToDump = SendDlgItemMessage(hDlg, IDC_THING_TO_DUMP, CB_GETCURSEL, 0, 0);
			DumpInfo(thingToDump);
			break;
		case IDC_UPDATE_SCREEN:
				CGraphicsContext::Get()->UpdateFrame();
			break;
		case IDC_CLEAR_BREAKPOINT:
			break;
		case IDC_SET_BREAKPOINT:
			break;
		case IDC_DROP_TEXTURE:
			gTextureManager.RecycleAllTextures();
			break;
		case IDC_DROP_COMBINER:
			debuggerDropCombiners = true;
			debuggerDropGeneralCombiners = true;
			debuggerDropDecodedMux = true;
			debuggerDropCombinerInfos = true;
			break;
		}
		return(true);
		break;
	case WM_CLOSE:
		EndDialog(hDlg, true);
		pauseAtNext = false;
		debuggerPause = false;
		logTriangles = false;
		logVertex = false;
		logMatrix = false;
		//debuggerWinOpened = false;
		return(true);
		break;
	}

	return(false);
}

void __cdecl DebugThreadFunc( void* )
{
	if( hWndDlg )
	{
		DestroyWindow(hWndDlg);
	}

	hWndDlg = CreateDialog(windowSetting.myhInst, "debugger", g_GraphicsInfo.hWnd, (DLGPROC)DebuggerDialog);
	ShowWindow(hWndDlg, SW_SHOW);
	debuggerWinOpened = true;

	MSG msg;

	while (GetMessage( &msg, hWndDlg, 0, 0 ) != 0)
	{ 
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DestroyWindow(hWndDlg);
}

void OpenDialogBox()
{
	if( !debuggerWinOpened )
	{
		HWND hWndDlg = CreateDialog(windowSetting.myhInst, "debugger", g_GraphicsInfo.hWnd, (DLGPROC)DebuggerDialog);
		ShowWindow(hWndDlg, SW_SHOW);
		debuggerWinOpened = true;
	}

	return;

	_beginthread( DebugThreadFunc, 255, NULL );
}

void CloseDialogBox()
{
	//if( debuggerWinOpened )
	if( hWndDlg )
	{
		DestroyWindow(hWndDlg);
		debuggerWinOpened = false;
	}
}


void __cdecl DebuggerAppendMsg(const char * Message, ...)
{
	if( !logToScreen && !logToFile )	return;

	char Msg[5000];
	char Buf2[5000];
	va_list ap;

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	
	if( Msg[strlen(Msg)-1]!='\n' ) strcat(Msg,"\n");

	uint32 i,j;
	for( i=0, j=0; i<strlen(Msg); i++, j++ )
	{
		if( Msg[i] == '\n' )
		{
			Buf2[j++] = '\r';
		}
		Buf2[j] = Msg[i];
	}
	Buf2[j] = 0;

	if( logToScreen )
	{
		HWND textBox = GetDlgItem(hWndDlg, IDC_MSG);
		//HWND textBox = GetDlgItem(myDialogWnd, IDC_MSG);

		INT nLength = SendMessage ( textBox, WM_GETTEXTLENGTH, 0, 0 ) ;
		SendMessage ( textBox, EM_SETSEL, -1, nLength );
		SendMessage ( textBox, EM_REPLACESEL, (WPARAM)false, (LPARAM)Buf2 );
		INT nNewLength = SendMessage ( textBox, WM_GETTEXTLENGTH, 0, 0 ) ;
		if( nLength == nNewLength )
		{
			SendMessage ( textBox, EM_SETSEL, 0, nLength ) ;
			SendMessage ( textBox, EM_REPLACESEL, false, (LPARAM) "\n"  ) ;
		}
	}

	if( logToFile )
	{
		fprintf(logFp, "%s\n", Msg);
	}
}


void DebuggerPause()
{
	char temp[300];
	sprintf(temp,"%s Debug %s:Paused ",project_name, BUILD_NUMBER) ;
	SetWindowText(myDialogWnd, temp );
	while( debuggerPause )
	{
		if( debuggerDrawRenderTexture )
		{
			g_pFrameBufferManager->DisplayRenderTexture(debuggerDrawRenderTextureNo);
			debuggerDrawRenderTexture = false;
		}
		Sleep(100);
	}
	sprintf(temp,"%s Debug %s",project_name, BUILD_NUMBER) ;
	SetWindowText(myDialogWnd, temp);
}

void __cdecl LOG_UCODE(LPCTSTR szFormat, ...)
{
	if( logUcodes)
	{
		char Msg[400];
		va_list va;
		va_start(va, szFormat);
		vsprintf( Msg, szFormat, va );
		va_end(va);
		DebuggerAppendMsg("%s\n", Msg);
	}
}

void DumpHex(uint32 rdramAddr, int count)
{
	rdramAddr &= 0xFFFFFFF0;
	uint32 *ptr = (uint32 *)((rdramAddr&(g_dwRamSize-1))+g_pu8RamBase);

	for( int i=0; i<(count+3)/4; i++)
	{
		DebuggerAppendMsg("%08X: %08X, %08X, %08X, %08X\n", 
			rdramAddr+i*16, ptr[i*4], ptr[i*4+1], ptr[i*4+2], ptr[i*4+3]);
	}
	DebuggerAppendMsg("\n");
}

uint32 StrToHex(char *HexStr)
{
	uint32		k;
	uint32	temp = 0;

	for(k = 0; k < strlen(HexStr); k++)
	{
		if(HexStr[k] <= '9' && HexStr[k] >= '0')
		{
			temp = temp << 4;
			temp += HexStr[k] - '0';
		}
		else if(HexStr[k] <= 'F' && HexStr[k] >= 'A')
		{
			temp = temp << 4;
			temp += HexStr[k] - 'A' + 10;
		}
		else if(HexStr[k] <= 'f' && HexStr[k] >= 'a')
		{
			temp = temp << 4;
			temp += HexStr[k] - 'a' + 10;
		}
		else
		{
			return(temp);
		}
	}

	return(temp);
}

void DEBUGGER_PAUSE_COUNT_N(uint32 val)
{
	if(pauseAtNext && eventToPause == val)
	{	
		if( debuggerPauseCount > 0 ) 
			debuggerPauseCount--;	
		
		if( debuggerPauseCount == 0 )	{
			pauseAtNext = false;	
			CGraphicsContext::Get()->UpdateFrame();
			debuggerPause = true;	
		}	
	}
}

void DEBUGGER_PAUSE_COUNT_N_WITHOUT_UPDATE(uint32 val)
{
	if(pauseAtNext && eventToPause == val)
	{	
		if( debuggerPauseCount > 0 ) 
			debuggerPauseCount--;	
		
		if( debuggerPauseCount == 0 )	{
			pauseAtNext = false;	
			debuggerPause = true;	
		}	
	}
}

void DumpMatrix2(const Matrix &mat, const char* prompt)
{
	DebuggerAppendMsg(prompt);
	DebuggerAppendMsg(
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n"
		" %#+12.5f %#+12.5f %#+12.5f %#+12.5f\n",
		mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);
}

void DumpMatrix(const Matrix &mat, const char* prompt)
{
	if( pauseAtNext && logMatrix )
	{
		DumpMatrix2(mat, prompt);
	}
}

#else
void __cdecl DebuggerAppendMsg(const char * Message, ...)
{
}

#endif
