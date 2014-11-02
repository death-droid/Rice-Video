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

void RSP_GBI1_Tri2(MicroCodeCommand command)
{
	// While the next command pair is Tri2, add vertices
	uint32 dwPC = gDlistStack[gDlistStackPointer].pc;
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);

	bool bTrisAdded = false;

	do {
		// Vertex indices are multiplied by 10 for Mario64, by 2 for MarioKart
		uint32 dwV0 = command.gbi1tri2.v0/gRSP.vertexMult;
		uint32 dwV1 = command.gbi1tri2.v1/gRSP.vertexMult;
		uint32 dwV2 = command.gbi1tri2.v2/gRSP.vertexMult;

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);

		uint32 dwV3 = command.gbi1tri2.v3/gRSP.vertexMult;
		uint32 dwV4 = command.gbi1tri2.v4/gRSP.vertexMult;
		uint32 dwV5 = command.gbi1tri2.v5/gRSP.vertexMult;

		bTrisAdded |= AddTri(dwV3, dwV4, dwV5);

		command.inst.cmd0 = *pCmdBase++;
		command.inst.cmd1 = *pCmdBase++;
		dwPC += 8;
#ifdef _DEBUG
	} while (!(pauseAtNext && eventToPause==NEXT_TRIANGLE) && command.inst.cmd == (uint8)RSP_TRI2);
#else
	} while( command.inst.cmd == (uint8)RSP_TRI2);
#endif


	gDlistStack[gDlistStackPointer].pc = dwPC-8;


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
	float vtxdepth = g_vecProjected[vtx].z/g_vecProjected[vtx].w;

#ifdef _DEBUG
	if( debuggerEnableZBuffer==FALSE || vtxdepth <= (s32)command.inst.cmd1 || g_curRomInfo.bForceDepthBuffer )
#else
	if( vtxdepth <= (s32)(command.branchz.value) || g_curRomInfo.bForceDepthBuffer )
#endif
	{
		uint32 dwAddr = RSPSegmentAddr(gRDPHalf1);

		LOG_UCODE("BranchZ to DisplayList 0x%08x", dwAddr);
		gDlistStack[gDlistStackPointer].pc = dwAddr;
		gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;
	}
}

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
	if (dwAddr + 64 > g_dwRamSize)
	{
		DebuggerAppendMsg("ForceMtx: Address invalid (0x%08x)", dwAddr);
		return;
	}

	// Load matrix from dwAddr
	LoadMatrix(dwAddr);

	CRender::g_pRender->SetWorldProjectMatrix(matToLoad);

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

			v4 & t = g_vecProjected[dwV];

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
	if( dwLight >= 16 )
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
//	s8 range = light->range;

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

	short scale[4];
	short trans[4];

	// dwAddr is offset into RD_RAM of 8 x 16bits of data...
	scale[0] = *(short *)(g_pu8RamBase + ((dwAddr+(0*2))^0x2));
	scale[1] = *(short *)(g_pu8RamBase + ((dwAddr+(1*2))^0x2));
//	scale[2] = *(short *)(g_pu8RamBase + ((dwAddr+(2*2))^0x2));
//	scale[3] = *(short *)(g_pu8RamBase + ((dwAddr+(3*2))^0x2));

	trans[0] = *(short *)(g_pu8RamBase + ((dwAddr+(4*2))^0x2));
	trans[1] = *(short *)(g_pu8RamBase + ((dwAddr+(5*2))^0x2));
//	trans[2] = *(short *)(g_pu8RamBase + ((dwAddr+(6*2))^0x2));
//	trans[3] = *(short *)(g_pu8RamBase + ((dwAddr+(7*2))^0x2));


	int nCenterX = trans[0]/4;
	int nCenterY = trans[1]/4;
	int nWidth   = scale[0]/4;
	int nHeight  = scale[1]/4;

	// Check for some strange games
	if( nWidth < 0 )	nWidth = -nWidth;
	if( nHeight < 0 )	nHeight = -nHeight;

	int nLeft = nCenterX - nWidth;
	int nTop  = nCenterY - nHeight;
	int nRight= nCenterX + nWidth;
	int nBottom= nCenterY + nHeight;

	//LONG maxZ = scale[2];
	int maxZ = 0x3FF;

	CRender::g_pRender->SetViewport(nLeft, nTop, nRight, nBottom, maxZ);


	LOG_UCODE("        Scale: %d %d = %d,%d", scale[0], scale[1],  nWidth, nHeight);
	LOG_UCODE("        Trans: %d %d = %d,%d", trans[0], trans[1], nCenterX, nCenterY);
}

// S2DEX uses this - 0xc1

void RSP_GBI1_SpNoop(MicroCodeCommand command)
{

/*	if( (command+1)->inst.cmd == 0x00 && gRSP.ucode >= 17 )
	{
		RSP_RDP_NOIMPL("Double SPNOOP, Skip remain ucodes, PC=%08X, Cmd1=%08X", gDlistStack[gDlistStackPointer].pc, command.inst.cmd1);
		RDP_GFX_PopDL();
		//if( gRSP.ucode < 17 ) TriggerDPInterrupt();
	}*/
}

void RSP_GBI1_Reserved(MicroCodeCommand command)
{		
	RSP_RDP_NOIMPL("RDP: Reserved (0x%08x 0x%08x)", (command.inst.cmd0), (command.inst.cmd1));
}

void RSP_GBI1_MoveMem(MicroCodeCommand command)
{
	uint32 type    = ((command.inst.cmd0)>>16)&0xFF;
	uint32 addr = RSPSegmentAddr((command.inst.cmd1));

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
				uint32 dwLight = (type-RSP_GBI1_MV_MEM_L0)/2;
				LOG_UCODE("    RSP_GBI1_MV_MEM_L%d", dwLight);
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
			gDlistStack[gDlistStackPointer].pc += 24;
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

void RSP_GBI1_RDPHalf_1(MicroCodeCommand command)		
{
	gRDPHalf1 = command.inst.cmd1;
}

void RSP_GBI1_Line3D(MicroCodeCommand command)
{
	uint32 dwPC = gDlistStack[gDlistStackPointer].pc;
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
			uint32 dwV3  = command.gbi1line3d.v3/gRSP.vertexMult;		
			uint32 dwV0  = command.gbi1line3d.v0/gRSP.vertexMult;
			uint32 dwV1  = command.gbi1line3d.v1/gRSP.vertexMult;
			uint32 dwV2  = command.gbi1line3d.v2/gRSP.vertexMult;

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

		gDlistStack[gDlistStackPointer].pc = dwPC-8;

		if (bTrisAdded)	
		{
			CRender::g_pRender->DrawTriangles();
		}
	}
}

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

	//Re-implment culling properly, FIXME CLEANME
	gRDP.tnl.TriCull = gGeometryMode.GBI1_CullFront | gGeometryMode.GBI1_CullBack;
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

static const char * sc_szBlClr[4] = { "In", "Mem", "Bl", "Fog" };
static const char * sc_szBlA1[4] = { "AIn", "AFog", "AShade", "0" };
static const char * sc_szBlA2[4] = { "1-A", "AMem", "1", "?" };

//TODO CLEAN OTHERMODEL and OTHERMODEH
void RSP_GBI1_SetOtherModeL(MicroCodeCommand command)
{
	uint32 dwShift = ((command.inst.cmd0)>>8)&0xFF;
	uint32 dwLength= ((command.inst.cmd0)   )&0xFF;
	uint32 dwData  = (command.inst.cmd1);

	uint32 dwMask = ((1<<dwLength)-1)<<dwShift;

	uint32 modeL = gRDP.otherMode.L;
	modeL = (modeL&(~dwMask)) | dwData;

	MicroCodeCommand tempgfx;
	tempgfx.inst.cmd0 = gRDP.otherMode.H;
	tempgfx.inst.cmd1 = modeL;
	DLParser_RDPSetOtherMode(tempgfx);
}


void RSP_GBI1_SetOtherModeH(MicroCodeCommand command)
{
	uint32 dwShift = ((command.inst.cmd0)>>8)&0xFF;
	uint32 dwLength= ((command.inst.cmd0)   )&0xFF;
	uint32 dwData  = (command.inst.cmd1);

	uint32 dwMask = ((1<<dwLength)-1)<<dwShift;
	uint32 dwModeH = gRDP.otherMode.H;

	dwModeH = (dwModeH&(~dwMask)) | dwData;
	MicroCodeCommand tempgfx;
	tempgfx.inst.cmd0 = dwModeH;
	tempgfx.inst.cmd1 = gRDP.otherMode.L;
	DLParser_RDPSetOtherMode(tempgfx );
}


void RSP_GBI1_Texture(MicroCodeCommand command)
{
	CRender::g_pRender->SetTextureEnable(command.texture.enable_gbi0);

	float fTextureScaleS = (float)(command.texture.scaleS) / (65536.0f * 32.0f);
	float fTextureScaleT = (float)(command.texture.scaleT) / (65536.0f * 32.0f);

	/*if( (((command.inst.cmd1)>>16)&0xFFFF) == 0xFFFF ) FIXME CLEANME Check for any side effects
	{
		fTextureScaleS = 1/32.0f;
	}
	else if( (((command.inst.cmd1)>>16)&0xFFFF) == 0x8000 )
	{
		fTextureScaleS = 1/64.0f;
	}
#ifdef _DEBUG
	else if( ((command.inst.cmd1>>16)&0xFFFF) != 0 )
	{
		//DebuggerAppendMsg("Warning, texture scale = %08X is not integer", (word1>>16)&0xFFFF);
	}
#endif

	if( (((command.inst.cmd1)    )&0xFFFF) == 0xFFFF )
	{
		fTextureScaleT = 1/32.0f;
	}
	else if( (((command.inst.cmd1)    )&0xFFFF) == 0x8000 )
	{
		fTextureScaleT = 1/64.0f;
	}
#ifdef _DEBUG
	else if( (command.inst.cmd1&0xFFFF) != 0 )
	{
		//DebuggerAppendMsg("Warning, texture scale = %08X is not integer", (word1)&0xFFFF);
	}
#endif */


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
	switch (command.mw1.type)
	{
	case RSP_MOVE_WORD_MATRIX:
		RSP_RDP_InsertMatrix(command);
		break;
	case RSP_MOVE_WORD_NUMLIGHT:
		{
			uint32 dwNumLights = (((command.mw1.value)-0x80000000)/32)-1;
			LOG_UCODE("    RSP_MOVE_WORD_NUMLIGHT: Val:%d", dwNumLights);

			gRSP.ambientLightIndex = dwNumLights;
			SetNumLights(dwNumLights);
		}
		break;
	case RSP_MOVE_WORD_CLIP: //CHECKME, CLEANME, FIXME, unsure what this does
		{
			switch (command.mw1.offset)
			{
			case RSP_MV_WORD_OFFSET_CLIP_RNX:
			case RSP_MV_WORD_OFFSET_CLIP_RNY:
			case RSP_MV_WORD_OFFSET_CLIP_RPX:
			case RSP_MV_WORD_OFFSET_CLIP_RPY:
				CRender::g_pRender->SetClipRatio(command.mw1.offset, command.mw1.value);
				break;
			default:
				LOG_UCODE("    RSP_MOVE_WORD_CLIP  ?   : 0x%08x", command.inst.cmd1);
				break;
			}
		}
		break;
	case RSP_MOVE_WORD_SEGMENT:
		{
			uint32 dwSegment = (command.mw1.offset >> 2) & 0xF;
			uint32 dwBase = (command.mw1.value)&0x00FFFFFF;
			LOG_UCODE("    RSP_MOVE_WORD_SEGMENT Seg[%d] = 0x%08x", dwSegment, dwBase);
			gRSP.segments[dwSegment] = dwBase;
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
			uint32 dwLight = command.mw1.offset >> 5;
			uint32 field_offset = (command.mw1.offset & 0x7);

			LOG_UCODE("    RSP_MOVE_WORD_LIGHTCOL/0x%08x: 0x%08x", command.mw1.offset, command.mw1.value);
			
			if(field_offset == 0)
			{
				SetLightCol(dwLight, ((command.mw2.value>>24)&0xFF), ((command.mw2.value>>16)&0xFF), ((command.mw2.value>>8)&0xFF));
			}
		}
		break;
	case RSP_MOVE_WORD_POINTS:
		{
			uint32 vtx = command.mw1.offset/40;
			uint32 where = command.mw1.offset - vtx*40;
			ModifyVertexInfo(where, vtx, command.mw1.value);
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

void RSP_GBI1_PopMtx(MicroCodeCommand command)
{
	LOG_UCODE("    Command: (%s)",	command.popmtx.projection ? "Projection" : "ModelView");

	// Do any of the other bits do anything?
	// So far only Extreme-G seems to Push/Pop projection matrices

	if (command.popmtx.projection)
	{
		CRender::g_pRender->PopProjection();
	}
	else
	{
		CRender::g_pRender->PopWorldView();
	}
#ifdef _DEBUG
	if( pauseAtNext && eventToPause == NEXT_MATRIX_CMD )
	{
		pauseAtNext = false;
		debuggerPause = true;
		DebuggerAppendMsg("Pause after Pop Matrix: %s\n", command.popmtx.projection ? "Proj":"World");
	}
	else
	{
		if( pauseAtNext && logMatrix ) 
		{
			DebuggerAppendMsg("Pause after Pop Matrix: %s\n", command.popmtx.projection ? "Proj":"World");
		}
	}
#endif
}

void RSP_GBI1_CullDL(MicroCodeCommand command)
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
	uint32 dwVFirst = ((command.inst.cmd0) & 0xFFF) / gRSP.vertexMult;
	uint32 dwVLast  = (((command.inst.cmd1)) & 0xFFF) / gRSP.vertexMult;

	LOG_UCODE("    Culling using verts %d to %d", dwVFirst, dwVLast);

	// Mask into range
	dwVFirst &= 0x1f;
	dwVLast &= 0x1f;

	if( dwVLast < dwVFirst )	return;
	if( !gRSP.bRejectVtx )	return;

	for (i = dwVFirst; i <= dwVLast; i++)
	{
		if (g_clipFlag[i] == 0)
		{
			LOG_UCODE("    Vertex %d is visible, continuing with display list processing", i);
			return;
		}
	}

	status.dwNumDListsCulled++;

	LOG_UCODE("    No vertices were visible, culling rest of display list");

	RDP_GFX_PopDL();
}


void RSP_GBI1_Tri1(MicroCodeCommand command)
{
	bool bTrisAdded = false;

	// While the next command pair is Tri1, add vertices
	uint32 dwPC = gDlistStack[gDlistStackPointer].pc;
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

	gDlistStack[gDlistStackPointer].pc = dwPC-8;

	if (bTrisAdded)	
	{
		CRender::g_pRender->DrawTriangles();
	}

	DEBUG_TRIANGLE(TRACE0("Pause at GBI0 TRI1"));
}

#endif //RSP_GBI1_H_