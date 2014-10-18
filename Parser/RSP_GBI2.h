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
#ifndef RSP_GBI2_H_
#define RSP_GBI2_H_

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI2_Vtx(MicroCodeCommand command)
{
	uint32 addr = RSPSegmentAddr(command.vtx2.addr);
	int vend	= command.vtx2.vend/2;
	int n		= command.vtx2.n;
	int v0		= vend - n;

	LOG_UCODE("    Vtx: Address 0x%08x, vEnd: %d, v0: %d, Num: %d", addr, vend, v0, n);

	if( vend > 64 )
	{
		DebuggerAppendMsg("Warning, attempting to load into invalid vertex positions, v0=%d, n=%d", v0, n);
		return;
	}

	if ((addr + (n*16)) > g_dwRamSize)
	{
		DebuggerAppendMsg("ProcessVertexData: Address out of range (0x%08x)", addr);
	}
	else
	{
		ProcessVertexData(addr, v0, n);

#ifdef _DEBUG
		status.dwNumVertices += n;
		DisplayVertexInfo(addr, v0, n);
#endif
	}
}

void RSP_GBI2_EndDL(MicroCodeCommand command)
{
	RDP_GFX_PopDL();
}

void RSP_GBI2_CullDL(MicroCodeCommand command)
{
#ifdef _DEBUG
	if( !debuggerEnableCullFace )
	{
		return;	//Disable Culling
	}
#endif
	if( g_curRomInfo.bDisableCulling )
	{
		return;	//Disable Culling
	}

	uint32 i;
	uint32 dwVFirst = (((command.inst.cmd0)) & 0xfff) / gRSP.vertexMult;
	uint32 dwVLast  = (((command.inst.cmd1)) & 0xfff) / gRSP.vertexMult;

	LOG_UCODE("    Culling using verts %d to %d", dwVFirst, dwVLast);

	// Mask into range
	dwVFirst &= 0x1f;
	dwVLast &= 0x1f;

	if( dwVLast < dwVFirst )	return;
	if( !gRSP.bRejectVtx )	return;

	for (i = dwVFirst; i <= dwVLast; i++)
	{
		//if (g_dwVtxFlags[i] == 0)
		if (g_clipFlag[i] == 0)
		{
			LOG_UCODE("    Vertex %d is visible, returning", i);
			return;
		}
	}

	status.dwNumDListsCulled++;

	LOG_UCODE("    No vertices were visible, culling");

	RDP_GFX_PopDL();
}

void RSP_GBI2_MoveWord(MicroCodeCommand command)
{
	switch (command.mw2.type)
	{
	case RSP_MOVE_WORD_MATRIX:
		RSP_RDP_InsertMatrix(command);
		break;

	case RSP_MOVE_WORD_NUMLIGHT:
		{
			uint32 dwNumLights = command.mw2.value / 24;
			gRSP.ambientLightIndex = dwNumLights;
			SetNumLights(dwNumLights);
		}
		break;

	case RSP_MOVE_WORD_CLIP: //Test whether this really actually does anything
		{
			switch (command.mw2.offset)
			{
			case RSP_MV_WORD_OFFSET_CLIP_RNX:
			case RSP_MV_WORD_OFFSET_CLIP_RNY:
			case RSP_MV_WORD_OFFSET_CLIP_RPX:
			case RSP_MV_WORD_OFFSET_CLIP_RPY:
				CRender::g_pRender->SetClipRatio(command.mw2.offset, command.mw2.value);
			default:
				LOG_UCODE("     RSP_MOVE_WORD_CLIP  ?   : 0x%08x", command.inst.cmd1);
				break;
			}
		}
		break;

	case RSP_MOVE_WORD_SEGMENT:
		{
			uint32 dwSeg     = command.mw2.offset >> 2;
			uint32 dwAddr    = command.mw2.value & 0x00FFFFFF;			// Hack - convert to physical

			LOG_UCODE("      RSP_MOVE_WORD_SEGMENT Segment[%d] = 0x%08x",	dwSeg, dwAddr);

			gRSP.segments[dwSeg] = dwAddr;

		}
		break;
	case RSP_MOVE_WORD_FOG:
		{
			uint16 wMult = (uint16)((command.mw2.value >> 16) & 0xFFFF);
			uint16 wOff  = (uint16)((command.mw2.value      ) & 0xFFFF);

			float fMult = (float)(short)wMult;
			float fOff = (float)(short)wOff;

			float rng = 128000.0f / fMult;
			float fMin = 500.0f - (fOff*rng/256.0f);
			float fMax = rng + fMin;

			FOG_DUMP(TRACE4("Set Fog: Min=%f, Max=%f, Mul=%f, Off=%f", fMin, fMax, fMult, fOff));
			//if( fMult <= 0 || fMin > fMax || fMax < 0 || fMin > 1000 )
			if( fMult <= 0 || fMax < 0 )
			{
				// Hack
				fMin = 996;
				fMax = 1000;
				fMult = 0;
				fOff = 1;
			}

			SetFogMinMax(fMin, fMax, fMult, fOff);
			FOG_DUMP(TRACE3("Set Fog: Min=%f, Max=%f, Data=0x%08X", fMin, fMax, command.mw2.value));
		}
		break;
	case RSP_MOVE_WORD_LIGHTCOL:
		{
			uint32 dwLight = command.mw2.offset / 0x18;
			uint32 field_offset = (command.mw2.offset & 0x7);

			LOG_UCODE("    RSP_MOVE_WORD_LIGHTCOL/0x%08x: 0x%08x", command.mw2.offset, command.mw2.value);

			if (field_offset == 0)
			{
				SetLightCol(dwLight, ((command.mw2.value>>24)&0xFF), ((command.mw2.value>>16)&0xFF), ((command.mw2.value>>8)&0xFF) );
			}

		}
		break;

	case RSP_MOVE_WORD_PERSPNORM:
		LOG_UCODE("     RSP_MOVE_WORD_PERSPNORM 0x%04x", (short)command.inst.cmd1);
		break;

	case RSP_MOVE_WORD_POINTS:
		LOG_UCODE("     2nd cmd of Force Matrix");
		break;

	default:
		{
			LOG_UCODE("      Ignored!!");

		}
		break;
	}
}

void RSP_GBI2_Tri1(MicroCodeCommand command)
{

	// While the next command pair is Tri1, add vertices
	uint32 dwPC = gDlistStack[gDlistStackPointer].pc;
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);

	bool bTrisAdded = false;

	do
	{
		uint32 dwV0 = command.gbi2tri1.v0/gRSP.vertexMult;
		uint32 dwV1 = command.gbi2tri1.v1/gRSP.vertexMult;
		uint32 dwV2 = command.gbi2tri1.v2/gRSP.vertexMult;

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);

		command.inst.cmd0 = *pCmdBase++;
		command.inst.cmd1 = *pCmdBase++;

		dwPC += 8;

#ifdef _DEBUG
	} while (!(pauseAtNext && eventToPause==NEXT_TRIANGLE) && command.inst.cmd == (uint8)RSP_ZELDATRI1);
#else
	} while( command.inst.cmd == (uint8)RSP_ZELDATRI1);
#endif

	gDlistStack[gDlistStackPointer].pc = dwPC-8;

	if (bTrisAdded)	
	{
		CRender::g_pRender->DrawTriangles();
	}

	DEBUG_TRIANGLE(TRACE0("Pause at GBI2 TRI1"));
}

void RSP_GBI2_Tri2(MicroCodeCommand command)
{
	// While the next command pair is Tri2, add vertices
	uint32 dwPC = gDlistStack[gDlistStackPointer].pc;
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);

	bool bTrisAdded = false;

	do {

		uint32 dwV0 = command.gbi2tri2.v0;
		uint32 dwV1 = command.gbi2tri2.v1;
		uint32 dwV2 = command.gbi2tri2.v2;

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);

		uint32 dwV5 = command.gbi2tri2.v5;
		uint32 dwV4 = command.gbi2tri2.v4;
		uint32 dwV3 = command.gbi2tri2.v3;

		bTrisAdded |= AddTri(dwV3, dwV4, dwV5);

		LOG_UCODE("    ZeldaTri2: 0x%08x 0x%08x", command.inst.cmd0, command.inst.cmd1);
		LOG_UCODE("           V0: %d, V1: %d, V2: %d", dwV0, dwV1, dwV2);
		LOG_UCODE("           V3: %d, V4: %d, V5: %d", dwV3, dwV4, dwV5);

		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		dwPC += 8;

#ifdef _DEBUG
	} while (!(pauseAtNext && eventToPause==NEXT_TRIANGLE) && command.inst.cmd == (uint8)RSP_ZELDATRI2);
#else
	} while ( command.inst.cmd == (uint8)RSP_ZELDATRI2 );//&& status.dwNumTrisRendered < 50);
#endif


	gDlistStack[gDlistStackPointer].pc = dwPC-8;

	if (bTrisAdded)	
	{
		CRender::g_pRender->DrawTriangles();
	}

	DEBUG_TRIANGLE(TRACE0("Pause at GBI2 TRI2"));

}

void RSP_GBI2_Line3D(MicroCodeCommand command)
{
	uint32 dwPC = gDlistStack[gDlistStackPointer].pc;
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);

	bool bTrisAdded = false;

	do {
		uint32 dwV0 = command.gbi2line3d.v0/gRSP.vertexMult;
		uint32 dwV1 = command.gbi2line3d.v1/gRSP.vertexMult;
		uint32 dwV2 = command.gbi2line3d.v2/gRSP.vertexMult;

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);

		uint32 dwV3 = command.gbi2line3d.v3/gRSP.vertexMult;
		uint32 dwV4 = command.gbi2line3d.v4/gRSP.vertexMult;
		uint32 dwV5 = command.gbi2line3d.v5/gRSP.vertexMult;

		bTrisAdded |= AddTri(dwV3, dwV4, dwV5);

		LOG_UCODE("    ZeldaTri3: 0x%08x 0x%08x", command.inst.cmd0, command.inst.cmd1);
		LOG_UCODE("           V0: %d, V1: %d, V2: %d", dwV0, dwV1, dwV2);
		LOG_UCODE("           V3: %d, V4: %d, V5: %d", dwV3, dwV4, dwV5);		

		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		dwPC += 8;

#ifdef _DEBUG
	} while (!(pauseAtNext && eventToPause==NEXT_TRIANGLE) && command.inst.cmd == (uint8)RSP_LINE3D);
#else
	} while ( command.inst.cmd == (uint8)RSP_LINE3D);
#endif

	gDlistStack[gDlistStackPointer].pc = dwPC-8;


	if (bTrisAdded)	
	{
		CRender::g_pRender->DrawTriangles();
	}

	DEBUG_TRIANGLE(TRACE0("Pause at GBI2 Line3D"));
}

void RSP_GBI2_Texture(MicroCodeCommand command)
{
	bool bEnable = command.texture.enable_gbi2;

	CRender::g_pRender->SetTextureEnable( bEnable );

	//Since the texture isnt enabled lets stop it from computing the rest
	float fTextureScaleS = (float)(command.texture.scaleS) / (65536.0f * 32.0f);
	float fTextureScaleT = (float)(command.texture.scaleT) / (65536.0f * 32.0f);

	if( (((command.inst.cmd1)>>16)&0xFFFF) == 0xFFFF ) //TEST FOR SIDE EFFECTS FIXME CLEANME
	{
		fTextureScaleS = 1/32.0f;
	}
	else if( (((command.inst.cmd1)>>16)&0xFFFF) == 0x8000 )
	{
		fTextureScaleS = 1/64.0f;
	}
	if( (((command.inst.cmd1)    )&0xFFFF) == 0xFFFF )
	{
		fTextureScaleT = 1/32.0f;
	}
	else if( (((command.inst.cmd1)    )&0xFFFF) == 0x8000 )
	{
		fTextureScaleT = 1/64.0f;
	}

	CRender::g_pRender->SetTextureScale(command.texture.tile, fTextureScaleS, fTextureScaleT);

	LOG_TEXTURE(
	{
		DebuggerAppendMsg("SetTexture: Level: %d Tile: %d %s\n", command.texture.level, command.texture.tile, command.texture.enable_gbi2 ? "enabled":"disabled");
		DebuggerAppendMsg("            ScaleS: %f, ScaleT: %f\n", fTextureScaleS*32.0f, fTextureScaleT*32.0f);
	});

	DEBUGGER_PAUSE_COUNT_N(NEXT_SET_TEXTURE);

	LOG_UCODE("    Level: %d Tile: %d %s", command.texture.level, command.texture.tile, command.texture.enable_gbi2 ? "enabled":"disabled");
	LOG_UCODE("    ScaleS: %f, ScaleT: %f", fTextureScaleS*32.0f, fTextureScaleT*32.0f);
}



void RSP_GBI2_PopMtx(MicroCodeCommand command)
{
	LOG_UCODE("    Command: (%s)",	command.inst.cmd1 ? "Projection" : "ModelView");

	// Banjo Tooie, pops more than one matrix
	u32 num = command.inst.cmd1>>6;

	CRender::g_pRender->PopWorldView(num);

}

void RSP_GBI2_GeometryMode(MicroCodeCommand command)
{
	gGeometryMode._u32	&= command.inst.cmd0;
	gGeometryMode._u32  |= command.inst.cmd1;

	gRDP.tnl._u32 = 0;

	gRDP.tnl.Light		= gGeometryMode.GBI2_Lighting;
	gRDP.tnl.TexGen		= gGeometryMode.GBI2_TexGen;
	gRDP.tnl.TexGenLin	= gGeometryMode.GBI2_TexGenLin;
	gRDP.tnl.Fog		= gGeometryMode.GBI2_Fog;
	gRDP.tnl.Shade		= !(gGeometryMode.GBI2_TexGenLin);
	gRDP.tnl.Zbuffer	= gGeometryMode.GBI2_Zbuffer;

	gRDP.tnl.TriCull = gGeometryMode.GBI2_CullFront | gGeometryMode.GBI2_CullBack; //We need to redo the way we handle culling
	gRDP.tnl.CullBack	= gGeometryMode.GBI2_CullBack;
	gRDP.tnl.PointLight = gGeometryMode.GBI2_PointLight;

	CRender::g_pRender->ZBufferEnable(gRDP.tnl.Zbuffer);

	CRender::g_pRender->SetFogEnable( gRDP.tnl.Fog );
	if (gRDP.tnl.Shade)
		CRender::g_pRender->SetShadeMode(SHADE_SMOOTH);
	else
		CRender::g_pRender->SetShadeMode(SHADE_FLAT);
}


int dlistMtxCount=0;
extern uint32 dwConkerVtxZAddr;

void RSP_GBI2_Mtx(MicroCodeCommand command)
{	
	dwConkerVtxZAddr = 0;	// For Conker BFD

	uint32 addr = RSPSegmentAddr(command.mtx2.addr);

	if( command.mtx2.param == 0 && command.mtx2.len == 0 )
	{
		DLParser_Bomberman2TextRect(command);
		return;
	}

	LOG_UCODE("    Mtx: %s %s %s Length %d Address 0x%08x",
		command.mtx2.projection ? "Projection" : "ModelView",
		command.mtx2.load ? "Load" : "Mul",	
		command.mtx2.nopush==0 ? "Push" : "No Push",
		command.mtx2.len, addr);


	LoadMatrix(addr);

	//Load Matrix from address
	if (command.mtx2.projection)
	{
		// So far only Extreme-G seems to Push/Pop projection matrices	
		CRender::g_pRender->SetProjection(matToLoad, command.mtx2.nopush==0, command.mtx2.load);
	}
	else
	{
		CRender::g_pRender->SetWorldView(matToLoad, command.mtx2.nopush==0, command.mtx2.load);

		if( options.enableHackForGames == HACK_FOR_SOUTH_PARK_RALLY )
		{
			dlistMtxCount++;
			if( dlistMtxCount == 2 )
			{
				CRender::g_pRender->ClearZBuffer(1.0f);
			}
		}
	}
}

void RSP_GBI2_MoveMem(MicroCodeCommand command)
{
	uint32 addr = RSPSegmentAddr((command.inst.cmd1));
	uint32 type    = ((command.inst.cmd0)     ) & 0xFE;

	//uint32 dwLen = ((command.inst.cmd0) >> 16) & 0xFF;
	//uint32 dwOffset = ((command.inst.cmd0) >> 8) & 0xFFFF;

	switch (type)
	{
	case RSP_GBI2_MV_MEM__VIEWPORT:
		{
			RSP_MoveMemViewport(addr);
		}
		break;
	case RSP_GBI2_MV_MEM__LIGHT:
		{
			 uint32 dwOffset2 = ((command.inst.cmd0) >> 5) & 0x7F8;
	
			uint32 dwLight = (dwOffset2)/24;
			if (dwLight < 2)
			{
				return;
			}

			dwLight -= 2;
			N64Light *light = (N64Light*)(g_pu8RamBase + addr);
			RSP_MoveMemLight(dwLight, light);

			SetLightPosition(dwLight, light->x1, light->y1, light->z1, 1.0f);
			SetLightEx(dwLight, light->ca, light->la, light->qa);

			LOG_UCODE("    Light %d:", dwLight);
			break;
		}
		break;
	case RSP_GBI2_MV_MEM__MATRIX:
		LOG_UCODE("Force Matrix: addr=%08X", addr);
		// Rayman 2, Donald Duck, Tarzan, all wrestling games use this
		RSP_GFX_Force_Matrix(addr);
		// ForceMatrix takes two cmds
		gDlistStack[gDlistStackPointer].pc += 8;
		break;
	case RSP_GBI2_MV_MEM_O_L0:
	case RSP_GBI2_MV_MEM_O_L1:
	case RSP_GBI2_MV_MEM_O_L2:
	case RSP_GBI2_MV_MEM_O_L3:
	case RSP_GBI2_MV_MEM_O_L4:
	case RSP_GBI2_MV_MEM_O_L5:
	case RSP_GBI2_MV_MEM_O_L6:
	case RSP_GBI2_MV_MEM_O_L7:
		LOG_UCODE("Zelda Move Light");
		RDP_NOIMPL_WARN("Zelda Move Light");
		break;

	case RSP_GBI2_MV_MEM__POINT:
		LOG_UCODE("Zelda Move Point");
		RDP_NOIMPL_WARN("Zelda Move Point");
		break;

	case RSP_GBI2_MV_MEM_O_LOOKATY:
		RSP_RDP_NOIMPL("Not implemented ZeldaMoveMem LOOKATY, Cmd0=0x%08X, Cmd1=0x%08X", command.inst.cmd0, command.inst.cmd1);
		break;
	case 0x00:
	case 0x02:
		// Ucode for Evangelion.v64, the ObjMatrix cmd
			RSP_S2DEX_OBJ_MOVEMEM(command);
			break;
	default:
		LOG_UCODE("ZeldaMoveMem Type: Unknown");
		RSP_RDP_NOIMPL("Unknown ZeldaMoveMem Type, type=0x%X, Addr=%08X", type, addr);
		break;
	}
}

void RSP_GBI2_DL(MicroCodeCommand command)
{
	uint32 dwPush = ((command.inst.cmd0) >> 16) & 0xFF;
	uint32 dwAddr = RSPSegmentAddr((command.inst.cmd1));

	if( dwAddr > g_dwRamSize )
	{
		RSP_RDP_NOIMPL("Error: DL addr = %08X out of range, PC=%08X", dwAddr, gDlistStack[gDlistStackPointer].pc );
		dwAddr &= (g_dwRamSize-1);
		DebuggerPauseCountN( NEXT_DLIST );
	}

	LOG_UCODE("    DL: Push:0x%02x Addr: 0x%08x", dwPush, dwAddr);
	
	switch (dwPush)
	{
	case RSP_DLIST_PUSH:
		LOG_UCODE("    Pushing ZeldaDisplayList 0x%08x", dwAddr);
		gDlistStackPointer++;
		gDlistStack[gDlistStackPointer].pc = dwAddr;
		gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;

		break;
	case RSP_DLIST_NOPUSH:
		LOG_UCODE("    Jumping to ZeldaDisplayList 0x%08x", dwAddr);
		if( gDlistStack[gDlistStackPointer].pc == dwAddr+8 )	//Is this a loop
		{
			//Hack for Gauntlet Legends
			gDlistStack[gDlistStackPointer].pc = dwAddr+8;
		}
		else
			gDlistStack[gDlistStackPointer].pc = dwAddr;
		gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;
		break;
	}

	LOG_UCODE("");
	LOG_UCODE("\\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
	LOG_UCODE("#############################################");
}

void RSP_GBI2_SetOtherModeL(MicroCodeCommand command)
{
	uint32 dwShift = ((command.inst.cmd0)>>8)&0xFF;
	uint32 dwLength= ((command.inst.cmd0)   )&0xFF;
	uint32 dwData  = (command.inst.cmd1);

	// Mask is constructed slightly differently
	uint32 dwMask = (uint32)((s32)(0x80000000)>>dwLength)>>dwShift;
	dwData &= dwMask;

	uint32 modeL = gRDP.otherMode.L;
	modeL = (modeL&(~dwMask)) | dwData;

	MicroCodeCommand tempgfx;
	tempgfx.inst.cmd0 = gRDP.otherMode.H;
	tempgfx.inst.cmd1 = modeL;
	DLParser_RDPSetOtherMode(tempgfx );
}

void RSP_GBI2_SetOtherModeH(MicroCodeCommand command)
{
	uint32 dwLength= (((command.inst.cmd0))&0xFF)+1;
	uint32 dwShift = 32 - (((command.inst.cmd0)>>8)&0xFF) - dwLength;
	uint32 dwData  = (command.inst.cmd1);

	uint32 dwMask2 = ((1<<dwLength)-1)<<dwShift;
	uint32 dwModeH = gRDP.otherMode.H;
	dwModeH = (dwModeH&(~dwMask2)) | dwData;

	MicroCodeCommand tempgfx;
	tempgfx.inst.cmd0 = dwModeH;
	tempgfx.inst.cmd1 = gRDP.otherMode.L;
	DLParser_RDPSetOtherMode(tempgfx );
}

void RSP_GBI2_DL_Count(MicroCodeCommand command)
{
	// This cmd is likely to execute number of ucode at the given address
	uint32 dwAddr = RSPSegmentAddr((command.inst.cmd1));

	// For SSB and Kirby, otherwise we'll end up scrapping the pc
	if (dwAddr == 0)
	{
		return;
	}

	gDlistStackPointer++;
	gDlistStack[gDlistStackPointer].pc = dwAddr;
	gDlistStack[gDlistStackPointer].countdown = ((command.inst.cmd0) & 0xFFFF);
}

void RSP_GBI2_0x8(MicroCodeCommand command)
{
	if (((command.inst.cmd0) & 0x00FFFFFF) == 0x2F && ((command.inst.cmd1) & 0xFF000000) == 0x80000000)
	{
		// V-Rally 64
		RSP_S2DEX_SPObjLoadTxRectR(command);
	}
	else
	{
		RSP_RDP_Nothing(command);
	}
}
void RSP_GBI2_SubModule(MicroCodeCommand command)
{
	RSP_RDP_NOIMPL("RDP: RSP_GBI2_SubModule (0x%08x 0x%08x)", (command.inst.cmd0), (command.inst.cmd1));
}

#endif RSP_GBI2_H_