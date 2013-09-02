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

void RSP_GBI0_Mtx(MicroCodeCommand command)
{	
	uint32 addr = RSPSegmentAddr((command.mtx1.addr));

	LOG_UCODE("    Command: %s %s %s Length %d Address 0x%08x",
		command.mtx1.projection == 1 ? "Projection" : "ModelView",
		command.mtx1.load == 1 ? "Load" : "Mul",	
		command.mtx1.push == 1 ? "Push" : "NoPush",
		command.mtx1.len, addr);

	if (addr + 64 > g_dwRamSize)
	{
		TRACE1("Mtx: Address invalid (0x%08x)", addr);
		return;
	}

	LoadMatrix(addr);
	
	if (command.mtx1.projection)
	{
		CRender::g_pRender->SetProjection(matToLoad, command.mtx1.push, command.mtx1.load);
	}
	else
	{
		CRender::g_pRender->SetWorldView(matToLoad, command.mtx1.push, command.mtx1.load);
	}

#ifdef _DEBUG
	char *loadstr = command.mtx1.load?"Load":"Mul";
	char *pushstr = command.mtx1.push?"Push":"Nopush";
	int projlevel = CRender::g_pRender->GetProjectMatrixLevel();
	int worldlevel = CRender::g_pRender->GetWorldViewMatrixLevel();
	if( pauseAtNext && eventToPause == NEXT_MATRIX_CMD )
	{
		pauseAtNext = false;
		debuggerPause = true;
		if (command.mtx1.projection)
		{
			TRACE3("Pause after %s and %s Matrix: Projection, level=%d\n", loadstr, pushstr, projlevel );
		}
		else
		{
			TRACE3("Pause after %s and %s Matrix: WorldView level=%d\n", loadstr, pushstr, worldlevel);
		}
	}
	else
	{
		if( pauseAtNext && logMatrix ) 
		{
			if (command.mtx1.projection)
			{
				TRACE3("Matrix: %s and %s Projection level=%d\n", loadstr, pushstr, projlevel);
			}
			else
			{
				TRACE3("Matrix: %s and %s WorldView\n level=%d", loadstr, pushstr, worldlevel);
			}
		}
	}
#endif
}

void RSP_GBI0_Vtx(MicroCodeCommand command)
{
	uint32 addr = RSPSegmentAddr(command.vtx0.addr);
	uint32 v0 = command.vtx0.v0;
	uint32 n = command.vtx0.n + 1;

	LOG_UCODE("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", addr, v0, n, command.vtx0.len);

	if ((v0 + n) > 80)
	{
		TRACE3("Warning, invalid vertex positions, N=%d, v0=%d, Addr=0x%08X", n, v0, addr);
		n = 32 - v0;
	}

	// Check that address is valid...
	if ((addr + n*16) > g_dwRamSize)
	{
		TRACE1("Vertex Data: Address out of range (0x%08x)", addr);
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

void RSP_GBI0_DL(MicroCodeCommand command)
{	
	uint32 addr = RSPSegmentAddr((command.dlist.addr)) & (g_dwRamSize-1);

	LOG_UCODE("    Address=0x%08x Push: 0x%02x", addr, command.dlist.param);
	if( addr > g_dwRamSize )
	{
		RSP_RDP_NOIMPL("Error: DL addr = %08X out of range, PC=%08X", addr, gDlistStack[gDlistStackPointer].pc );
		addr &= (g_dwRamSize-1);
		DebuggerPauseCountN( NEXT_DLIST );
	}

	if( command.dlist.param == RSP_DLIST_PUSH )
		gDlistStackPointer++;

	gDlistStack[gDlistStackPointer].pc = addr;
	gDlistStack[gDlistStackPointer].countdown = MAX_DL_COUNT;

		LOG_UCODE("Level=%d", gDlistStackPointer+1);
		LOG_UCODE("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
}

void RSP_GBI0_Tri4(MicroCodeCommand command)
{
	// While the next command pair is Tri2, add vertices
	uint32 dwPC = gDlistStack[gDlistStackPointer].pc;

	bool bTrisAdded = false;

	do {
		//Tri #1
		u32 v0 = command.tri4.v0;
		u32 v1 = command.tri4.v1;
		u32 v2 = command.tri4.v2;

		bTrisAdded |= AddTri(v0, v1, v2);

		//Tri #2
		u32 v3 = command.tri4.v3;
		u32 v4 = command.tri4.v4;
		u32 v5 = command.tri4.v5;

		bTrisAdded |= AddTri(v3, v4, v5, true);

		//Tri #3
		u32 v6 = command.tri4.v6;
		u32 v7 = command.tri4.v7;
		u32 v8 = command.tri4.v8;

		bTrisAdded |= AddTri(v6, v7, v8, true);

		//Tri #4
		u32 v9  = command.tri4.v9;
		u32 v10 = command.tri4.v10;
		u32 v11 = command.tri4.v11;

		bTrisAdded |= AddTri(v9, v10, v11, true);

		command.inst.cmd0 = *(u32 *)(g_pu8RamBase + dwPC+0);
		command.inst.cmd1 = *(u32 *)(g_pu8RamBase + dwPC+4);
		dwPC += 8;

#ifdef _DEBUG
	} while (!(pauseAtNext && eventToPause==NEXT_TRIANGLE) && (command.inst.cmd0>>24) == (uint8)RSP_TRI2);
#else
	} while (((command.inst.cmd0)>>24) == (uint8)RSP_TRI2);
#endif


	gDlistStack[gDlistStackPointer].pc = dwPC-8;


	if (bTrisAdded)	
	{
		CRender::g_pRender->DrawTriangles();
	}
	
	DEBUG_TRIANGLE(TRACE0("Pause at GBI0 TRI4"));

//	gDKRVtxCount=0;
}