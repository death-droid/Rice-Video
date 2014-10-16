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
#include "Microcode.h"

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//                     Ucodes                           //
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

// Used to keep track of when we're processing the first display list
static bool gFirstCall = true;

static u32 gRDPHalf1 = 0;
static u32 gLastUcodeBase = 0;
const MicroCodeInstruction *gUcodeFunc = NULL;

void DLParser_InitMicrocode(u32 code_base, u32 code_size, u32 data_base, u32 data_size);

FiddledVtx * g_pVtxBase=NULL;
static RDP_GeometryMode gGeometryMode;

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

	status.lastPurgeTimeTime = status.gRDPTime;

	status.curRenderBuffer = NULL;
	status.curVIOriginReg = NULL;

	status.lastPurgeTimeTime = 0;		// Time textures were last purged

	memset(&g_ZI_saves, 0, sizeof(RenderTextureInfo)*2);

	for( i=0; i<8; i++ )
	{
		memset(&gRDP.tiles[i], 0, sizeof(Tile));
	}
	memset(g_tmemLoadAddrMap, 0, sizeof(g_tmemLoadAddrMap));

	status.bAllowLoadFromTMEM = true;
	
	char name[200];
	strcpy(name, g_curRomInfo.szGameName);

	GBIMicrocode_Reset();

	memset(&g_TmemFlag, 0, sizeof(g_TmemFlag));
	memset(&g_RecentCIInfo, 0, sizeof(RecentCIInfo)*5);
	memset(&g_RecentVIOriginInfo, 0, sizeof(RecentViOriginInfo)*5);
	memset(&g_ZI_saves, 0, sizeof(RenderTextureInfo)*2);
	memset(&g_ZI, 0, sizeof(SetImgInfo));
	memset(&g_CI, 0, sizeof(SetImgInfo));
	memset(&g_TI, 0, sizeof(SetImgInfo));

}


void RDP_GFX_Reset()
{
	gDlistStackPointer=-1;
	gTextureManager.RecycleAllTextures();
}


void RDP_Cleanup()
{
	if( status.bHandleN64RenderTexture )
	{
		g_pFrameBufferManager->CloseRenderTexture(false);
	}
}

extern int dlistMtxCount;
extern bool bHalfTxtScale;

//*****************************************************************************
//
//*****************************************************************************
void DLParser_InitMicrocode(u32 code_base, u32 code_size, u32 data_base, u32 data_size)
{
	UcodeInfo info = GBIMicrocode_DetectVersion(code_base, code_size, data_base, data_size);
	gRSP.vertexMult = info.stride;
	gLastUcodeBase = code_base;
	gUcodeFunc = info.func;
	// Used for fetching ucode names (Debug Only)
//#if defined(DAEDALUS_DEBUG_DISPLAYLIST) || defined(DAEDALUS_ENABLE_PROFILING)
	//gUcodeName = IS_CUSTOM_UCODE(ucode) ? gCustomInstructionName : gNormalInstructionName[ucode];
//#endif
}

void DLParser_Process()
{
	dlistMtxCount = 0;
	bHalfTxtScale = false;

	if ( CRender::g_pRender == NULL)
	{
		return;
	}

	status.bScreenIsDrawn = true;

	DebuggerPauseCountN( NEXT_DLIST );
	status.gRDPTime = timeGetTime();
	status.gDlistCount++;

	OSTask * pTask = (OSTask *)(g_GraphicsInfo.DMEM + 0x0FC0);
	u32 code_base = (u32)pTask->t.ucode & 0x1fffffff;
	u32 code_size = pTask->t.ucode_size;
	u32 data_base = (u32)pTask->t.ucode_data & 0x1fffffff;
	u32 data_size = pTask->t.ucode_data_size;
	u32 stack_size = pTask->t.dram_stack_size >> 6;

	if (gLastUcodeBase != code_base)
	{
		DLParser_InitMicrocode(code_base, code_size, data_base, data_size);
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
	
	if( g_curRomInfo.bForceScreenClear && CGraphicsContext::needCleanScene )
	{
		CRender::g_pRender->ClearBuffer(true,true);
		CGraphicsContext::needCleanScene = false;
	}

	SetVIScales();
	CRender::g_pRender->RenderReset();
	CRender::g_pRender->ResetMatrices(stack_size);
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

			status.gUcodeCount++;

			MicroCodeCommand command;

			DLParser_FetchNextCommand(&command);

			gUcodeFunc[command.inst.cmd](command);

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
	}

	//Do this regardless
	TriggerDPInterrupt();

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
	if( gRDP.otherMode.H != ((command.inst.cmd0)) )
	{
		gRDP.otherMode.H = ((command.inst.cmd0));

		uint32 dwTextFilt  = (gRDP.otherMode.H>>RSP_SETOTHERMODE_SHIFT_TEXTFILT)&0x3;
		CRender::g_pRender->SetTextureFilter(dwTextFilt<<RSP_SETOTHERMODE_SHIFT_TEXTFILT);
	}
	//RDP_TFILTER_BILERP

	if( gRDP.otherMode.L != (command.inst.cmd1) )
	{

		gRDP.otherMode.L = (command.inst.cmd1);

		if (gRDP.otherMode.zmode == 3) //DKR doesnt seem to like the zbias being set for some odd reason :S
			CRender::g_pRender->SetZBias(2);
		else
			CRender::g_pRender->SetZBias(0);


		if ((gRDP.tnl.Zbuffer * gRDP.otherMode.z_cmp) | gRDP.otherMode.z_upd)
		{
			CRender::g_pRender->SetZCompare(true);
		}
		else
		{
			CRender::g_pRender->SetZCompare(false);
		}
		
		CRender::g_pRender->SetZUpdate( gRDP.otherMode.z_upd ? true : false );

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
	//We do this regardless now
	//This tends to be called after DLIST processing anyway
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
		//Clear the z buffer
		CRender::g_pRender->ClearBuffer(false,true);
		
		//Clear depth buffer, fixes jumpy camera in DK64, also the sun and flames glare in Zelda
		int x0 = command.fillrect.x0 + 1;
		int x1 = command.fillrect.x1 + 1;
		int y1 = command.fillrect.y1;
		int y0 = command.fillrect.y0;
		
		x0 = min(max(x0, gRDP.scissor.left), gRDP.scissor.right);
		x1 = min(max(x1, gRDP.scissor.left), gRDP.scissor.right);
		y1 = min(max(y1, gRDP.scissor.top), gRDP.scissor.bottom);
		y0 = min(max(y0, gRDP.scissor.top), gRDP.scissor.bottom);

		x0 >>= 1;
		x1 >>= 1;

		u32 zi_width_in_dwords = g_CI.dwWidth >> 1;
		u32 * dst = (u32*)(g_pu8RamBase + g_CI.dwAddr) + y0 * zi_width_in_dwords;

		for (int y = y0; y < y1; y++)
		{
			for (int x = x0; x < x1; x++)
			{
				dst[x] = fill_colour;
			}
			dst += zi_width_in_dwords;
		}

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
	gDlistStackPointer=-1;
}


void RSP_RDP_InsertMatrix(MicroCodeCommand command)
{
	UpdateCombinedMatrix();

	int x = ((command.inst.cmd0) & 0x1F) >> 1;
	int y = x >> 2;
	x &= 3;

	//Float
	if ((command.inst.cmd0) & 0x20)
	{
		gRSPworldProject.m[y][x]     = (float)(int)gRSPworldProject.m[y][x] + ((float) (command.inst.cmd1 >> 16) / 65536.0f);
		gRSPworldProject.m[y][x + 1] = (float)(int)gRSPworldProject.m[y][x + 1] + ((float) (command.inst.cmd1 & 0xFFFF) / 65536.0f);
	}
	else
	{
		//Integer
		gRSPworldProject.m[y][x]     = (float)(short)((command.inst.cmd1)>>16);
		gRSPworldProject.m[y][x + 1] = (float)(short)((command.inst.cmd1) & 0xFFFF);
	}

	gRSP.bMatrixIsUpdated = false;

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
	LOG_UCODE("    ZImage: 0x%08x", RSPSegmentAddr(command.inst.cmd1));

	g_ZI.dwAddr = RSPSegmentAddr(command.inst.cmd1);
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
		gUcodeFunc[p_command->inst.cmd0 >>24](*p_command);
	}

	CRender::g_pRender->EndRendering();
}

//You will never see these any HLE emulation, and since where a HLE plugin ignore them completely
void DLParser_TriRSP(MicroCodeCommand command)
{
	LOG_UCODE("DLParser_TriRSP: (Ignored)");
}

Matrix matToLoad;
void LoadMatrix(uint32 addr)
{
	if (addr + 64 > g_dwRamSize)
	{
		TRACE1("Mtx: Address invalid (0x%08x)", addr);
		return;
	}

	const float fRecip = 1.0f / 65536.0f;
	const N64mat *Imat = (N64mat *)(g_pu8RamBase + addr);

	for (int i = 0; i < 4; i++)
	{
		matToLoad.m[i][0] = ((Imat->h[i].x << 16) | Imat->l[i].x) * fRecip;
		matToLoad.m[i][1] = ((Imat->h[i].y << 16) | Imat->l[i].y) * fRecip;
		matToLoad.m[i][2] = ((Imat->h[i].z << 16) | Imat->l[i].z) * fRecip;
		matToLoad.m[i][3] = ((Imat->h[i].w << 16) | Imat->l[i].w) * fRecip;
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
