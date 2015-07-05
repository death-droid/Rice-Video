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
//Clean me
#ifndef RSP_GBI1_H_
#define RSP_GBI1_H_

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_Vtx(MicroCodeCommand command)
{
	uint32 addr = RSPSegmentAddr(command.vtx1.addr);
	uint32 v0  = command.vtx1.v0;
	uint32 n   = command.vtx1.n;

	LOG_UCODE("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", addr, v0, n, command.vtx1.len);

	//This happens in wetrix
	if (addr > g_dwRamSize)
	{
		TRACE0("     Address out of range - ignoring load");
		return;
	}

	if ((v0 + n) > 80)
	{
		TRACE5("Warning, invalid vertex positions, N=%d, v0=%d, Addr=0x%08X, Cmd=%08X-%08X",
			n, v0, addr, command.inst.cmd0, command.inst.cmd1);
		return;
	}

	ProcessVertexData(addr, v0, n);
#ifdef _DEBUG
	status.dwNumVertices += n;
	DisplayVertexInfo(addr, v0, n);
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_ModifyVtx(MicroCodeCommand command)
{
	uint32 offset   = command.modifyvtx.offset;
	uint32 dwVert   = command.modifyvtx.vtx;
	uint32 dwValue  = command.modifyvtx.value;

	//Stops crash after swining in mario golf
	if( dwVert > 80 )
	{
		RSP_RDP_NOIMPL("RSP_GBI1_ModifyVtx: Invalid vertex number: %d", dwVert, 0);
		return;
	}

	ModifyVertexInfo(offset, dwVert, dwValue);
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_Tri2(MicroCodeCommand command)
{
	// While the next command pair is Tri2, add vertices
	uint32 dwPC = gDlistStack.address[gDlistStackPointer];
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);

	bool bTrisAdded = false;

	do {
		// Vertex indices are multiplied by 10 for GBI0, by 2 for GBI1
		uint32 dwV0 = command.gbi1tri2.v0 >> 1;
		uint32 dwV1 = command.gbi1tri2.v1 >> 1;
		uint32 dwV2 = command.gbi1tri2.v2 >> 1;

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);

		uint32 dwV3 = command.gbi1tri2.v3 >> 1;
		uint32 dwV4 = command.gbi1tri2.v4 >> 1;
		uint32 dwV5 = command.gbi1tri2.v5 >> 1;

		bTrisAdded |= AddTri(dwV3, dwV4, dwV5);

		command.inst.cmd0 = *pCmdBase++;
		command.inst.cmd1 = *pCmdBase++;
		dwPC += 8;
#ifdef _DEBUG
	} while (!(pauseAtNext && eventToPause==NEXT_TRIANGLE) && command.inst.cmd == (uint8)RSP_TRI2);
#else
	} while( command.inst.cmd == (uint8)RSP_TRI2);
#endif


	gDlistStack.address[gDlistStackPointer] = dwPC - 8;


	if (bTrisAdded)	
	{
		CRender::g_pRender->DrawTriangles();
	}

	DEBUG_TRIANGLE(TRACE0("Pause at GBI1 TRI1"));
}

//*****************************************************************************
// When the depth is less than the z value provided, branch to given address
//*****************************************************************************
void RSP_GBI1_BranchZ(MicroCodeCommand command)
{
	uint32 vtx = command.branchz.vtx;
	float vtxdepth = g_vecProjected[vtx].ProjectedPos.z / g_vecProjected[vtx].ProjectedPos.w;

#ifdef _DEBUG
	if( debuggerEnableZBuffer==FALSE || vtxdepth <= (s32)command.inst.cmd1 )
#else
	if( vtxdepth <= (s32)(command.branchz.value) )
#endif
	{
		uint32 dwAddr = RSPSegmentAddr(gRDPHalf1);

		LOG_UCODE("BranchZ to DisplayList 0x%08x", dwAddr);
		gDlistStack.address[gDlistStackPointer] = dwAddr;
	}
}

//*****************************************************************************
// AST, Yoshi's World, Scooby Doo use this
//*****************************************************************************
void RSP_GBI1_LoadUCode(MicroCodeCommand command)
{
	u32 code_base = (command.inst.cmd1 & 0x1fffffff);
	u32 code_size = 0x1000;
	u32 data_base = gRDPHalf1 & 0x1fffffff;         // Preceeding RDP_HALF1 sets this up
	u32 data_size = (command.inst.cmd0 & 0xFFFF) + 1;

	DLParser_InitMicrocode(code_base, code_size, data_base, data_size);

	DEBUGGER_PAUSE_AND_DUMP(NEXT_SWITCH_UCODE,{DebuggerAppendMsg("Pause at loading ucode");});
}


//NOT GBI1 SPECFIFIC MOVEME FIXME CLEANME
void RSP_GFX_Force_Matrix(uint32 dwAddr)
{
	gRSP.mWorldProjectValid = true;
	gRSP.mWPmodified = true;//Signal that Worldproject matrix is changed

	MatrixFromN64FixedPoint(gRSP.mWorldProject, dwAddr);

	DEBUGGER_PAUSE_AND_DUMP(NEXT_MATRIX_CMD,{TRACE0("Paused at ModMatrix Cmd");});
}


void DisplayVertexInfo(uint32 dwAddr, uint32 dwV0, uint32 dwN)
{
#ifdef _DEBUG
		s8 *pcSrc = (s8 *)(g_pu8RamBase + dwAddr);
		short *psSrc = (short *)(g_pu8RamBase + dwAddr);

		for (uint32 dwV = dwV0; dwV < dwV0 + dwN; dwV++)
		{
			float x = (float)psSrc[0^0x1];
			float y = (float)psSrc[1^0x1];
			float z = (float)psSrc[2^0x1];

			//uint32 wFlags = g_dwVtxFlags[dwV]; //(uint16)psSrc[3^0x1];
			uint32 wFlags = 0;

			uint8 a = pcSrc[12^0x3];
			uint8 b = pcSrc[13^0x3];
			uint8 c = pcSrc[14^0x3];
			uint8 d = pcSrc[15^0x3];
			
			//LONG nTU = (LONG)(short)(psSrc[4^0x1]<<4);
			//LONG nTV = (LONG)(short)(psSrc[5^0x1]<<4);

			//float tu = (float)(nTU>>4);
			//float tv = (float)(nTV>>4);
			float tu = (float)(short)(psSrc[4^0x1]);
			float tv = (float)(short)(psSrc[5^0x1]);

			v4 & t = g_vecProjected[dwV].ProjectedPos;

			psSrc += 8;			// Increase by 16 bytes
			pcSrc += 16;

			LOG_UCODE(" #%02d Flags: 0x%04x Pos: {% 6f,% 6f,% 6f} Tex: {%+7.2f,%+7.2f}, Extra: %02x %02x %02x %02x (transf: {% 6f,% 6f,% 6f})",
				dwV, wFlags, x, y, z, tu, tv, a, b, c, d, t.x, t.y, t.z );
		}
#endif
}

//NOT GBI1 SPECFIFIC MOVEME FIXME CLEANME
void RSP_MoveMemLight(uint32 dwLight, const N64Light *light)
{
	if( dwLight >= 12 )
	{
		DebuggerAppendMsg("Warning: invalid light # = %d", dwLight);
		return;
	}

	u8 r = light->r;
	u8 g = light->g;
	u8 b = light->b;

	s8 dir_x = light->dir_x;
	s8 dir_y = light->dir_y;
	s8 dir_z = light->dir_z;

	bool valid = (dir_x | dir_y | dir_z) != 0;

	LIGHT_DUMP(TRACE4("  Light[%d] RGB[%d, %d, %d]", dwLight, r, g, b));
//	LIGHT_DUMP(TRACE4("  x[%d] y[%d] z[%d] %s direction", x, y, z, valid ? "Valid" : "Invalid"));
	
	//Set the light color
	SetLightCol(dwLight, r, g, b);

	//Set direction
	SetLightDirection(dwLight, dir_x, dir_y, dir_z);
}


//NOT GBI1 SPECFIFIC MOVEME FIXME CLEANME
void RSP_MoveMemViewport(uint32 dwAddr)
{
	if( dwAddr+16 >= g_dwRamSize )
	{
		TRACE0("MoveMem Viewport, invalid memory");
		return;
	}

	// address is offset into RD_RAM of 8 x 16bits of data...
	N64Viewport *vp = (N64Viewport*)(g_pu8RamBase + dwAddr);

	v2 vec_scale(vp->scale_x * 0.25f, vp->scale_y * 0.25f);
	v2 vec_trans(vp->trans_x * 0.25f, vp->trans_y * 0.25f);

	// Check for some strange games
	if (vec_scale.x < 0)	vec_scale.x = -vec_scale.x;
	if (vec_scale.y < 0)	vec_scale.y = -vec_scale.y;

	int nLeft = vec_trans.x - vec_scale.x;
	int nTop = vec_trans.y - vec_scale.y;
	int nRight = vec_trans.x + vec_scale.x;
	int nBottom = vec_trans.y + vec_scale.y;

	int maxZ = 0x3FF;

	CRender::g_pRender->SetViewport(nLeft, nTop, nRight, nBottom, maxZ);

	LOG_UCODE("        Scale: %d %d = %d,%d", vp->scale_x, vp->scale_y, vec_scale.x, vec_scale.y);
	LOG_UCODE("        Trans: %d %d = %d,%d", vp->trans_x, vp->trans_y, vec_trans.x, vec_trans.y);
}

// S2DEX uses this - 0xc1

void RSP_GBI1_SpNoop(MicroCodeCommand command)
{

}

void RSP_GBI1_Reserved(MicroCodeCommand command)
{		
	RSP_RDP_NOIMPL("RDP: Reserved (0x%08x 0x%08x)", (command.inst.cmd0), (command.inst.cmd1));
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_MoveMem(MicroCodeCommand command)
{
	uint32 type    = (command.inst.cmd0>>16)&0xFF;
	uint32 addr = RSPSegmentAddr(command.inst.cmd1);

	switch (type) 
	{
		case RSP_GBI1_MV_MEM_VIEWPORT:
			{
				LOG_UCODE("    RSP_GBI1_MV_MEM_VIEWPORT. Address: 0x%08x", addr);
				RSP_MoveMemViewport(addr);
			}
			break;
		case RSP_GBI1_MV_MEM_LOOKATY:
			LOG_UCODE("    RSP_GBI1_MV_MEM_LOOKATY");
			break;
		case RSP_GBI1_MV_MEM_LOOKATX:
			LOG_UCODE("    RSP_GBI1_MV_MEM_LOOKATX");
			break;
		case RSP_GBI1_MV_MEM_L0:
		case RSP_GBI1_MV_MEM_L1:
		case RSP_GBI1_MV_MEM_L2:
		case RSP_GBI1_MV_MEM_L3:
		case RSP_GBI1_MV_MEM_L4:
		case RSP_GBI1_MV_MEM_L5:
		case RSP_GBI1_MV_MEM_L6:
		case RSP_GBI1_MV_MEM_L7:
			{
				uint32 dwLight = (type-RSP_GBI1_MV_MEM_L0) >> 1;
				N64Light *light = (N64Light*)(g_pu8RamBase + addr);
				RSP_MoveMemLight(dwLight, light);
			}
			break;
		case RSP_GBI1_MV_MEM_TXTATT:
			LOG_UCODE("    RSP_GBI1_MV_MEM_TXTATT");
			break;
		case RSP_GBI1_MV_MEM_MATRIX_1:
			// Rayman 2, Donald Duck, Tarzan, all wrestling games use this
			RSP_GFX_Force_Matrix(addr);
			//Force matrix takes fource cmds
			gDlistStack.address[gDlistStackPointer] += 24;
			break;
		case RSP_GBI1_MV_MEM_MATRIX_2:
			break;
		case RSP_GBI1_MV_MEM_MATRIX_3:
			break;
		case RSP_GBI1_MV_MEM_MATRIX_4:
			break;
		default:
			RSP_RDP_NOIMPL("MoveMem: Unknown Move Type, cmd=%08X, %08X", command.inst.cmd0, command.inst.cmd1);
			break;
	}
}

void RSP_GBI1_RDPHalf_Cont(MicroCodeCommand command)	
{
	LOG_UCODE("RDPHalf_Cont: (Ignored)"); 
}
void RSP_GBI1_RDPHalf_2(MicroCodeCommand command)		
{ 
	LOG_UCODE("RDPHalf_2: (Ignored)"); 
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_RDPHalf_1(MicroCodeCommand command)		
{
	gRDPHalf1 = command.inst.cmd1;
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_Line3D(MicroCodeCommand command)
{
	uint32 dwPC = gDlistStack.address[gDlistStackPointer];
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);

	bool bTrisAdded = FALSE;

	if( command.gbi1line3d.v3 == 0 )
	{
		// Flying Dragon
		uint32 dwV0		= command.gbi1line3d.v0/gRSP.vertexMult;
		uint32 dwV1		= command.gbi1line3d.v1/gRSP.vertexMult;
		uint32 dwWidth  = command.gbi1line3d.v2;
		uint32 dwFlag	= command.gbi1line3d.v3/gRSP.vertexMult;	
		
		CRender::g_pRender->SetCombinerAndBlender();

		status.dwNumTrisRendered++;

		CRender::g_pRender->Line3D(dwV0, dwV1, dwWidth);
	}
	else
	{
		do {
			uint32 dwV0 = command.gbi1line3d.v0 / gRSP.vertexMult;
			uint32 dwV1 = command.gbi1line3d.v1 / gRSP.vertexMult;
			uint32 dwV2 = command.gbi1line3d.v2 / gRSP.vertexMult;
			uint32 dwV3 = command.gbi1line3d.v3 / gRSP.vertexMult;		

			LOG_UCODE("    Line3D: V0: %d, V1: %d, V2: %d, V3: %d", dwV0, dwV1, dwV2, dwV3);

			bTrisAdded |= AddTri(dwV0, dwV1, dwV2);
			bTrisAdded |= AddTri(dwV2, dwV3, dwV0);

			command.inst.cmd0 = *pCmdBase++;
			command.inst.cmd1 = *pCmdBase++;
			dwPC += 8;
#ifdef _DEBUG
		} while (command.inst.cmd == (uint8)RSP_LINE3D && !(pauseAtNext && eventToPause==NEXT_FLUSH_TRI));
#else
		} while (command.inst.cmd == (uint8)RSP_LINE3D);
#endif

		gDlistStack.address[gDlistStackPointer] = dwPC - 8;

		if (bTrisAdded)	
		{
			CRender::g_pRender->DrawTriangles();
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_GeometryMode(MicroCodeCommand command)
{
	uint32 dwMask = (command.inst.cmd1);
	if(command.inst.cmd & 1)
	{
		gGeometryMode._u32 |= dwMask;
		LOG_UCODE("Setting mask -> 0x%08x", dwMask);
	}
	else
	{
		gGeometryMode._u32 &= ~dwMask;
		LOG_UCODE("Clearing mask -> 0x%08x", dwMask);
	}
	
	gRDP.tnl._u32 = 0;

	gRDP.tnl.Light		= gGeometryMode.GBI1_Lighting;
	gRDP.tnl.TexGen		= gGeometryMode.GBI1_TexGen;
	gRDP.tnl.TexGenLin	= gGeometryMode.GBI1_TexGenLin;
	gRDP.tnl.Fog		= gGeometryMode.GBI1_Fog;
	gRDP.tnl.Shade		= gGeometryMode.GBI1_Shade;
	gRDP.tnl.Zbuffer	= gGeometryMode.GBI1_Zbuffer;

	// CULL_BACK has priority, Fixes Mortal Kombat 4
	gRDP.tnl.TriCull	= gGeometryMode.GBI1_CullFront | gGeometryMode.GBI1_CullBack;
	gRDP.tnl.CullBack	= gGeometryMode.GBI1_CullBack;

	CRender::g_pRender->ZBufferEnable(gRDP.tnl.Zbuffer);
	CRender::g_pRender->SetFogEnable(gRDP.tnl.Fog);
	if (gRDP.tnl.Shade)
		CRender::g_pRender->SetShadeMode(SHADE_SMOOTH);
	else
		CRender::g_pRender->SetShadeMode(SHADE_FLAT);
}

void RSP_GBI1_EndDL(MicroCodeCommand command)
{
	RDP_GFX_PopDL();
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_SetOtherModeL(MicroCodeCommand command)
{
	const u32 mask = ((1 << command.othermode.len) - 1) << command.othermode.sft;

	gRDP.otherMode.L = (gRDP.otherMode.L & ~mask) | command.othermode.data;
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_SetOtherModeH(MicroCodeCommand command)
{
	const u32 mask = ((1 << command.othermode.len) - 1) << command.othermode.sft;

	gRDP.otherMode.H = (gRDP.otherMode.H & ~mask) | command.othermode.data;
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_Texture(MicroCodeCommand command)
{
	CRender::g_pRender->SetTextureEnable(command.texture.enable_gbi0);

	float fTextureScaleS = (float)(command.texture.scaleS) / (65536.0f * 32.0f);
	float fTextureScaleT = (float)(command.texture.scaleT) / (65536.0f * 32.0f);

	CRender::g_pRender->SetTextureScale(command.texture.tile, fTextureScaleS, fTextureScaleT);

	// What happens if these are 0? Interpret as 1.0f?

	LOG_TEXTURE(
	{
		DebuggerAppendMsg("SetTexture: Level: %d Tile: %d %s\n", command.texture.level, command.texture.tile, command.texture.enable_gbi0 ? "enabled":"disabled");
		DebuggerAppendMsg("            ScaleS: %f, ScaleT: %f\n", fTextureScaleS*32.0f, fTextureScaleT*32.0f);
	});

	DEBUGGER_PAUSE_COUNT_N(NEXT_SET_TEXTURE);
	LOG_UCODE("    Level: %d Tile: %d %s", command.texture.level, command.texture.tile, command.texture.enable_gbi0 ? "enabled":"disabled");
	LOG_UCODE("    ScaleS: %f, ScaleT: %f", fTextureScaleS*32.0f, fTextureScaleT*32.0f);
}

extern void RSP_RDP_InsertMatrix(uint32 word0, uint32 word1);
void RSP_GBI1_MoveWord(MicroCodeCommand command)
{
	// Type of movement is in low 8bits of cmd0.
	u32 value = command.mw1.value;
	u32 offset = command.mw1.offset;

	switch (command.mw1.type)
	{
	case RSP_MOVE_WORD_MATRIX:
		RSP_RDP_InsertMatrix(command);
		break;
	case RSP_MOVE_WORD_NUMLIGHT:
		{
			uint32 dwNumLights = ((command.mw1.value-0x80000000) >> 5) - 1;
			LOG_UCODE("    RSP_MOVE_WORD_NUMLIGHT: Val:%d", dwNumLights);

			SetNumLights(dwNumLights);
		}
		break;
	/*case RSP_MOVE_WORD_CLIP: //Seems unused
		{
			LOG_UCODE("    RSP_MOVE_WORD_CLIP  ?   : 0x%08x", command.inst.cmd1);
		}
		break;*/
	case RSP_MOVE_WORD_SEGMENT:
		{
			uint32 segment = (offset >> 2) & 0xF;
			LOG_UCODE("    RSP_MOVE_WORD_SEGMENT Seg[%d] = 0x%08x", segment, value);
			gRSP.segments[segment] = value & 0x00FFFFFF;
		}
		break;
	case RSP_MOVE_WORD_FOG:
		{
			uint16 wMult = (uint16)(((command.mw1.value) >> 16) & 0xFFFF);
			uint16 wOff  = (uint16)(((command.mw1.value)      ) & 0xFFFF);

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

			LOG_UCODE("    RSP_MOVE_WORD_FOG/Mul=%d: Off=%d", wMult, wOff);
			FOG_DUMP(TRACE3("Set Fog: Min=%f, Max=%f, Data=%08X", fMin, fMax, command.mw1.value));
			SetFogMinMax(fMin, fMax, fMult, fOff);
		}
		break;
	case RSP_MOVE_WORD_LIGHTCOL:
		{
			uint32 light_idx = offset >> 5;
			uint32 field_offset = (offset & 0x7);

			LOG_UCODE("    RSP_MOVE_WORD_LIGHTCOL/0x%08x: 0x%08x", command.mw1.offset, command.mw1.value);
			
			if(field_offset == 0)
			{
				u8 r = ((value >> 24) & 0xFF);
				u8 g = ((value >> 16) & 0xFF);
				u8 b = ((value >>  8) & 0xFF);

				SetLightCol(light_idx, r, g, b);
			}
		}
		break;
	case RSP_MOVE_WORD_POINTS:
		{
			ModifyVertexInfo((offset % 40), (offset / 40), command.mw1.value);
		}
		break;
	case RSP_MOVE_WORD_PERSPNORM:
		LOG_UCODE("    RSP_MOVE_WORD_PERSPNORM");
		//if( word1 != 0x1A ) DebuggerAppendMsg("PerspNorm: 0x%04x", (short)word1);	
		break;
	default:
		RSP_RDP_NOIMPL("Unknown MoveWord, %08X, %08X", command.inst.cmd0, command.inst.cmd1);
		break;
	}

}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_Mtx(MicroCodeCommand command)
{
	uint32 addr = RSPSegmentAddr(command.mtx1.addr);

	LOG_UCODE("    Command: %s %s %s Length %d Address 0x%08x",
		command.mtx1.projection == 1 ? "Projection" : "ModelView",
		command.mtx1.load == 1 ? "Load" : "Mul",
		command.mtx1.push == 1 ? "Push" : "NoPush",
		command.mtx1.len, addr);

	//Load matrix from address
	if (command.mtx1.projection)
	{
		CRender::g_pRender->SetProjection(addr, command.mtx1.load);
	}
	else
	{
		CRender::g_pRender->SetWorldView(addr, command.mtx1.push, command.mtx1.load);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_PopMtx(MicroCodeCommand command)
{
	LOG_UCODE("    Command: (%s)",	command.popmtx.projection ? "Projection" : "ModelView");

	// Do any of the other bits do anything?
	// So far only Extreme-G seems to Push/Pop projection matrices
	if(command.inst.cmd1 == 0)
		CRender::g_pRender->PopWorldView();

}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_DL(MicroCodeCommand command)
{
	uint32 addr = RSPSegmentAddr((command.dlist.addr)) & (g_dwRamSize - 1);

	LOG_UCODE("    Address=0x%08x Push: 0x%02x", addr, command.dlist.param);
	if(addr > g_dwRamSize)
	{
		RSP_RDP_NOIMPL("Error: DL addr = %08X out of range, PC=%08X", addr, gDlistStack.address[gDlistStackPointer]);
		addr &= (g_dwRamSize - 1);
		DebuggerPauseCountN(NEXT_DLIST);
	}

	if(command.dlist.param == RSP_DLIST_PUSH)
		gDlistStackPointer++;

	gDlistStack.address[gDlistStackPointer] = addr;

	LOG_UCODE("Level=%d", gDlistStackPointer + 1);
	LOG_UCODE("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_CullDL(MicroCodeCommand command)
{
	uint32 first = command.culldl.first;
	uint32 last = command.culldl.end;;

	LOG_UCODE("    Culling using verts %d to %d", first, last);

	if( last < first )	return;

	uint32 flags = g_vecProjected[first].ClipFlags;
	for (uint32 i = first+1; i <= last; i++)
	{
		flags &= g_vecProjected[i].ClipFlags;
	}
	
	if(flags == 0)
	{
		LOG_UCODE("   Display list is visible");
		return;
	}
	status.dwNumDListsCulled++;

	LOG_UCODE("    No vertices were visible, culling rest of display list");

	RDP_GFX_PopDL();
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_GBI1_Tri1(MicroCodeCommand command)
{
	bool bTrisAdded = false;

	// While the next command pair is Tri1, add vertices
	uint32 dwPC = gDlistStack.address[gDlistStackPointer];
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase+  dwPC);
	
	do
	{
		// Vertex indices are multiplied by 10 for Mario64, by 2 for MarioKart
		uint32 dwV0 = command.gbi1tri1.v0/gRSP.vertexMult;
		uint32 dwV1 = command.gbi1tri1.v1/gRSP.vertexMult;
		uint32 dwV2 = command.gbi1tri1.v2/gRSP.vertexMult;

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);

		command.inst.cmd0= *pCmdBase++;
		command.inst.cmd1= *pCmdBase++;

		dwPC += 8;

#ifdef _DEBUG
	} while (!(pauseAtNext && eventToPause==NEXT_TRIANGLE) && command.inst.cmd == (uint8)RSP_TRI1);
#else
	} while (command.inst.cmd == (uint8)RSP_TRI1);
#endif

	gDlistStack.address[gDlistStackPointer] = dwPC - 8;

	if (bTrisAdded)	
	{
		CRender::g_pRender->DrawTriangles();
	}

	DEBUG_TRIANGLE(TRACE0("Pause at GBI0 TRI1"));
}

#endif //RSP_GBI1_H_