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
#ifndef RSP_DKR_H__
#define RSP_DKR_H__

u32 gDKRCMatrixIndex = 0;
u32 gDKRMatrixAddr = 0;
u32 gDKRVtxAddr = 0;
u32 gDKRVtxCount = 0;
bool gDKRBillBoard = false;

// DKR verts are extra 4 bytes
//*****************************************************************************
//
//*****************************************************************************
void RDP_GFX_DumpVtxInfoDKR(uint32 dwAddr, uint32 dwV0, uint32 dwN)
{
#ifdef _DEBUG
		uint32 dwV;
		LONG i;

		short * psSrc = (short *)(g_pu8RamBase + dwAddr);

		i = 0;
		for (dwV = dwV0; dwV < dwV0 + dwN; dwV++)
		{
			float x = (float)psSrc[(i + 0) ^ 1];
			float y = (float)psSrc[(i + 1) ^ 1];
			float z = (float)psSrc[(i + 2) ^ 1];

			//uint16 wFlags = CRender::g_pRender->m_dwVecFlags[dwV]; //(uint16)psSrc[3^0x1];

			uint16 wA = psSrc[(i + 3) ^ 1];
			uint16 wB = psSrc[(i + 4) ^ 1];

			uint8 a = wA>>8;
			uint8 b = (uint8)wA;
			uint8 c = wB>>8;
			uint8 d = (uint8)wB;

			v4 & t = g_vecProjected[dwV];


			LOG_UCODE(" #%02d Pos: {% 6f,% 6f,% 6f} Extra: %02x %02x %02x %02x (transf: {% 6f,% 6f,% 6f})",
				dwV, x, y, z, a, b, c, d, t.x, t.y, t.z );

			i+=5;
		}


		uint16 * pwSrc = (uint16 *)(g_pu8RamBase + dwAddr);
		i = 0;
		for (dwV = dwV0; dwV < dwV0 + dwN; dwV++)
		{
			LOG_UCODE(" #%02d %04x %04x %04x %04x %04x",
				dwV, pwSrc[(i + 0) ^ 1],
				pwSrc[(i + 1) ^ 1],
				pwSrc[(i + 2) ^ 1],
				pwSrc[(i + 3) ^ 1],
				pwSrc[(i + 4) ^ 1]);

			i += 5;
		}

#endif // _DEBUG
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_Vtx_DKR(MicroCodeCommand command)
{
	uint32 address = command.inst.cmd1 + gDKRVtxAddr;
	uint32 num_verts  = (command.inst.cmd0 >>19 )&0x1F;
	uint32 v0_idx = 0;

	//Increase num_verts by 1 for diddy kong racing, needed to avoid having a seperate method just for gemini
	if (options.enableHackForGames == HACK_FOR_DIDDY_KONG_RACING) num_verts++;

	if( command.inst.cmd0 & 0x00010000 )
	{
		if( gDKRBillBoard )
			gDKRVtxCount = 1;
	}
	else
	{
		gDKRVtxCount = 0;
	}

	v0_idx = ((command.inst.cmd0 >> 9) & 0x1F) + gDKRVtxCount;

	LOG_UCODE("    Address 0x%08x, v0: %d, Num: %d", address, v0_idx, num_verts);
	DEBUGGER_ONLY_IF( (pauseAtNext && (eventToPause==NEXT_VERTEX_CMD||eventToPause==NEXT_MATRIX_CMD)), {DebuggerAppendMsg("DKR Vtx: Cmd0=%08X, Cmd1=%08X", (command.inst.cmd0), (command.inst.cmd1));});

	if (v0_idx >= 32)		v0_idx = 31;
	
	if ((v0_idx + num_verts) > 32)
	{
		WARNING(TRACE0("Warning, attempting to load into invalid vertex positions"));
		num_verts = 32 - v0_idx;
	}

	// Check that address is valid...
	if ((address + (num_verts*16)) > g_dwRamSize)
	{
		WARNING(TRACE1("ProcessVertexData: Address out of range (0x%08x)", address));
	}
	else
	{
		ProcessVertexDataDKR(address, v0_idx, num_verts);

#ifdef _DEBUG
		status.dwNumVertices += num_verts;
		RDP_GFX_DumpVtxInfoDKR(address, v0_idx, num_verts);
#endif
	}
}

//*****************************************************************************
//
//*****************************************************************************
void RDP_GFX_DLInMem(MicroCodeCommand command)
{

	gDlistStackPointer++;
	gDlistStack[gDlistStackPointer].pc = command.inst.cmd1;
	gDlistStack[gDlistStackPointer].countdown = (command.inst.cmd0 >> 16) & 0xFF;
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_Mtx_DKR(MicroCodeCommand command)
{	
	uint32 address = (command.inst.cmd1)+RSPSegmentAddr(gDKRMatrixAddr);
	uint32 mtx_command = ((command.inst.cmd0)>>16)&0x3;
	//uint32 length  = ((command.inst.cmd0))    &0xFFFF;

	bool mul=false;

	if (mtx_command == 0)
	{
		//DKR : No mult
		gDKRCMatrixIndex = mtx_command = (command.inst.cmd0 >> 22) & 0x3;
	}
	else
	{
		//JFG : mult but only if bit is set
		mul = ((command.inst.cmd0 >> 23) & 0x1);
	}

	// Load matrix from address
	Matrix4x4 &mat = gRSP.modelviewMtxs[mtx_command];
	LoadMatrix(address);

	//Perform any required modifications to the matrix (aka if it needs multiply)
	if( mul )
	{
		mat = matToLoad* gRSP.modelviewMtxs[0];
	}
	else
	{
		mat = matToLoad;
	}

	DEBUGGER_PAUSE_AND_DUMP(NEXT_MATRIX_CMD,{TRACE0("Paused at DKR Matrix Cmd");});
}



//*****************************************************************************
//
//*****************************************************************************
void RSP_MoveWord_DKR(MicroCodeCommand command)
{
	switch ((command.inst.cmd0) & 0xFF)
	{
	case RSP_MOVE_WORD_NUMLIGHT:
		gDKRBillBoard = (command.inst.cmd1) & 0x1;
		LOG_UCODE("    gDKRBillBoard = %d", gDKRBillBoard);
		DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_MATRIX_CMD, {DebuggerAppendMsg("DKR Moveword, select gRSP.DKRBillBoard %s, cmd0=%08X, cmd1=%08X", gDKRBillBoard?"true":"false", (command.inst.cmd0), (command.inst.cmd1));});
		break;

	case RSP_MOVE_WORD_LIGHTCOL:
		gDKRCMatrixIndex = (command.inst.cmd1 >> 6) & 0x3;
		LOG_UCODE("    gDKRCMatrixIndex = %d", gDKRCMatrixIndex);
		DEBUGGER_PAUSE_AND_DUMP_COUNT_N(NEXT_MATRIX_CMD, {DebuggerAppendMsg("DKR Moveword, select matrix %d, cmd0=%08X, cmd1=%08X", gDKRCMatrixIndex, (command.inst.cmd0), (command.inst.cmd1));});
		break;

	default:
		RSP_GBI1_MoveWord(command);
		break;
	}

}

//*****************************************************************************
//
//*****************************************************************************
void DLParser_Set_Addr_DKR(MicroCodeCommand command)
{
	gDKRMatrixAddr = command.inst.cmd0 & 0x00FFFFFF;
	gDKRVtxAddr = RSPSegmentAddr(command.inst.cmd1 & 0x00FFFFFF);
	gDKRVtxCount=0;
}

//*****************************************************************************
//
//*****************************************************************************
//DKR: 00229BA8: 05710080 001E4AF0 CMD G_DMATRI  Triangles 9 at 801E4AF0
void RSP_DMA_Tri_DKR(MicroCodeCommand command)
{
	u32 dwAddr = RSPSegmentAddr(command.inst.cmd1);
	u32 dwNum = (command.inst.cmd0 >> 4) & 0x1F; //Count should never exceed 16

	//Unlike normal tri ucodes, this has the tri's stored in rdram
	TriDKR *tri = (TriDKR*)(g_pu8RamBase + dwAddr);

	TRI_DUMP(TRACE2("DMATRI, addr=%08X, Cmd0=%08X\n", dwAddr, (command.inst.cmd0)));

	bool bTrisAdded = false;

	for (u32 i = 0; i < dwNum; i++)
	{
		uint32 dwV0 = tri->v0;
		uint32 dwV1 = tri->v1;
		uint32 dwV2 = tri->v2;

		CRender::g_pRender->SetCullMode(!(tri->flag & 0x40), true);

		TRI_DUMP(TRACE5("DMATRI: %d, %d, %d (%08X-%08X)", dwV0,dwV1,dwV2,(command.inst.cmd0),(command.inst.cmd1)));

		DEBUG_DUMP_VERTEXES("DmaTri", dwV0, dwV1, dwV2);
		LOG_UCODE("   Tri: %d,%d,%d", dwV0, dwV1, dwV2);

		//Lets set the vtx texture coords always 
		CRender::g_pRender->SetVtxTextureCoord(dwV0, tri->s0, tri->t0);
		CRender::g_pRender->SetVtxTextureCoord(dwV1, tri->s1, tri->t1);
		CRender::g_pRender->SetVtxTextureCoord(dwV2, tri->s2, tri->t2);

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);

		tri++;
	}

	if (bTrisAdded)	
	{
		CRender::g_pRender->DrawTriangles();
	}

	gDKRVtxCount=0;
}
#endif //RSP_DKR_H__