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
	int vend	= command.vtx2.vend >> 1;
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

//*****************************************************************************
//
//*****************************************************************************
//0016A710: DB020000 00000018 CMD Zelda_MOVEWORD  Mem[2][00]=00000018 Lightnum=0
//001889F0: DB020000 00000030 CMD Zelda_MOVEWORD  Mem[2][00]=00000030 Lightnum=2
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

			SetNumLights(dwNumLights);
		}
		break;

	/*case RSP_MOVE_WORD_CLIP: Seems unused
		{
			LOG_UCODE("     RSP_MOVE_WORD_CLIP  ?   : 0x%08x", command.inst.cmd1);
		}
		break;*/

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

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI2_Tri1(MicroCodeCommand command)
{
	// While the next command pair is Tri1, add vertices
	uint32 dwPC = gDlistStack.address[gDlistStackPointer];
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);

	bool bTrisAdded = false;

	do
	{
		uint32 dwV0 = command.gbi2tri1.v0 >> 1;
		uint32 dwV1 = command.gbi2tri1.v1 >> 1;
		uint32 dwV2 = command.gbi2tri1.v2 >> 1;

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);

		command.inst.cmd0 = *pCmdBase++;
		command.inst.cmd1 = *pCmdBase++;

		dwPC += 8;

#ifdef _DEBUG
	} while (!(pauseAtNext && eventToPause==NEXT_TRIANGLE) && command.inst.cmd == (uint8)RSP_ZELDATRI1);
#else
	} while( command.inst.cmd == (uint8)RSP_ZELDATRI1);
#endif

	gDlistStack.address[gDlistStackPointer] = dwPC - 8;

	if (bTrisAdded)	
	{
		CRender::g_pRender->DrawTriangles();
	}

	DEBUG_TRIANGLE(TRACE0("Pause at GBI2 TRI1"));
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI2_Tri2(MicroCodeCommand command)
{
	// While the next command pair is Tri2, add vertices
	uint32 dwPC = gDlistStack.address[gDlistStackPointer];
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);

	bool bTrisAdded = false;

	do {

		uint32 dwV0 = command.gbi2tri2.v0;
		uint32 dwV1 = command.gbi2tri2.v1;
		uint32 dwV2 = command.gbi2tri2.v2;

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);

		uint32 dwV4 = command.gbi2tri2.v4;
		uint32 dwV3 = command.gbi2tri2.v3;
		uint32 dwV5 = command.gbi2tri2.v5;

		bTrisAdded |= AddTri(dwV3, dwV4, dwV5);

		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		dwPC += 8;

#ifdef _DEBUG
	} while (!(pauseAtNext && eventToPause==NEXT_TRIANGLE) && command.inst.cmd == (uint8)RSP_ZELDATRI2);
#else
	} while ( command.inst.cmd == (uint8)RSP_ZELDATRI2 );
#endif


	gDlistStack.address[gDlistStackPointer] = dwPC - 8;

	if (bTrisAdded)	
	{
		CRender::g_pRender->DrawTriangles();
	}

	DEBUG_TRIANGLE(TRACE0("Pause at GBI2 TRI2"));

}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI2_Line3D(MicroCodeCommand command)
{
	// While the next command pair is Tri2, add vertices
	uint32 dwPC = gDlistStack.address[gDlistStackPointer];
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);

	bool tris_added = false;

	do {
		uint32 dwV0 = command.gbi2line3d.v0 >> 1;
		uint32 dwV1 = command.gbi2line3d.v1 >> 1;
		uint32 dwV2 = command.gbi2line3d.v2 >> 1;

		tris_added |= AddTri(dwV0, dwV1, dwV2);

		uint32 dwV3 = command.gbi2line3d.v3 >> 1;
		uint32 dwV4 = command.gbi2line3d.v4 >> 1;
		uint32 dwV5 = command.gbi2line3d.v5 >> 1;

		tris_added |= AddTri(dwV3, dwV4, dwV5);

		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;
		dwPC += 8;

#ifdef _DEBUG
	} while (!(pauseAtNext && eventToPause==NEXT_TRIANGLE) && command.inst.cmd == (uint8)RSP_LINE3D);
#else
	} while ( command.inst.cmd == (uint8)RSP_LINE3D);
#endif

	gDlistStack.address[gDlistStackPointer] = dwPC - 8;

	if (tris_added)
	{
		CRender::g_pRender->DrawTriangles();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI2_Texture(MicroCodeCommand command)
{

	CRender::g_pRender->SetTextureEnable(command.texture.enable_gbi2);

	float fTextureScaleS = (float)(command.texture.scaleS) / (65536.0f * 32.0f);
	float fTextureScaleT = (float)(command.texture.scaleT) / (65536.0f * 32.0f);

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

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI2_PopMtx(MicroCodeCommand command)
{
	LOG_UCODE("    Command: (%s)",	command.inst.cmd1 ? "Projection" : "ModelView");

	// Banjo Tooie, pops more than one matrix
	u32 num = command.inst.cmd1>>6;

	// Just pop the worldview matrix
	CRender::g_pRender->PopWorldView(num);

}

//*****************************************************************************
//
//*****************************************************************************
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

//*****************************************************************************
//
//*****************************************************************************
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

	//Load Matrix from address
	if (command.mtx2.projection)
	{
		// So far only Extreme-G seems to Push/Pop projection matrices	
		CRender::g_pRender->SetProjection(addr, command.mtx2.load);
	}
	else
	{
		CRender::g_pRender->SetWorldView(addr, command.mtx2.nopush==0, command.mtx2.load);

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

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI2_MoveMem(MicroCodeCommand command)
{
	uint32 addr = RSPSegmentAddr(command.inst.cmd1);
	uint32 type = (command.inst.cmd0     ) & 0xFE;

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
		gDlistStack.address[gDlistStackPointer] += 8;
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
		break;
	case RSP_GBI2_MV_MEM__POINT:
		LOG_UCODE("Zelda Move Point");
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

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI2_SetOtherModeL(MicroCodeCommand command)
{
	// Mask is constructed slightly differently
	const uint32 mask = (uint32)((s32)(0x80000000) >> command.othermode.len) >> command.othermode.sft;

	gRDP.otherMode.L = (gRDP.otherMode.L & ~mask) | command.othermode.data;
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI2_SetOtherModeH(MicroCodeCommand command)
{
	// Mask is constructed slightly differently
	const uint32 mask = (uint32)((s32)(0x80000000) >> command.othermode.len) >> command.othermode.sft;

	gRDP.otherMode.H = (gRDP.otherMode.H & ~mask) | command.othermode.data;
}

//*****************************************************************************
// Kirby 64, SSB and Cruisn' Exotica use this
//*****************************************************************************
void RSP_GBI2_DL_Count(MicroCodeCommand command)
{
	// This cmd is likely to execute number of ucode at the given address
	uint32 address = RSPSegmentAddr(command.inst.cmd1);

	// For SSB and Kirby, otherwise we'll end up scrapping the pc
	if (address == 0)
	{
		return;
	}

	gDlistStackPointer++;
	gDlistStack.address[gDlistStackPointer] = address;
	gDlistStack.limit = (command.inst.cmd0) & 0xFFFF;
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