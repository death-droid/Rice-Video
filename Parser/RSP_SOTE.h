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
#ifndef RSP_SOTE_H__
#define RSP_SOTE_H__

//*****************************************************************************
//
//*****************************************************************************
void RSP_Vtx_ShadowOfEmpire(MicroCodeCommand command)
{
	uint32 dwAddr   = RSPSegmentAddr(command.inst.cmd1);
	uint32 dwLength = ((command.inst.cmd0))&0xFFFF;

	uint32 dwN= ((command.inst.cmd0 >> 4) & 0xFFF) / 33 + 1;
	uint32 dwV0 = 0;

	LOG_UCODE("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", dwAddr, dwV0, dwN, dwLength);

	if (dwV0 >= 32)
		dwV0 = 31;
	
	if ((dwV0 + dwN) > 32)
	{
		TRACE0("Warning, attempting to load into invalid vertex positions");
		dwN = 32 - dwV0;
	}

	ProcessVertexData(dwAddr, dwV0, dwN);

#ifdef _DEBUG
	status.dwNumVertices += dwN;
	DisplayVertexInfo(dwAddr, dwV0, dwN);
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void RSP_Quad3d_ShadowOfEmpire(MicroCodeCommand command)
{
	bool bTrisAdded = false;
	bool bTexturesAreEnabled = CRender::g_pRender->IsTextureEnabled();

	// While the next command pair is Tri2, add vertices
	uint32 dwPC = gDlistStack[gDlistStackPointer].pc;
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);

	do {
		uint32 dwV0 = ((command.inst.cmd1 >> 24) & 0xFF) / 5;
		uint32 dwV1 = ((command.inst.cmd1 >> 16) & 0xFF) / 5;
		uint32 dwV2 = ((command.inst.cmd1 >>  8) & 0xFF) / 5;

		uint32 dwV3 = ((command.inst.cmd1 >> 24) & 0xFF) / 5;
		uint32 dwV4 = ((command.inst.cmd1 >>  8) & 0xFF) / 5;
		uint32 dwV5 = ((command.inst.cmd1      ) & 0xFF) / 5;

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);
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
//
//*****************************************************************************
void RSP_Tri1_ShadowOfEmpire(MicroCodeCommand command)
{

	bool bTrisAdded = false;
	bool bTexturesAreEnabled = CRender::g_pRender->IsTextureEnabled();

	// While the next command pair is Tri1, add vertices
	uint32 dwPC = gDlistStack[gDlistStackPointer].pc;
	uint32 * pCmdBase = (uint32 *)(g_pu8RamBase + dwPC);
	
	do
	{
		uint32 dwV0 = ((command.inst.cmd1 >> 16) & 0xFF) / 5;
		uint32 dwV1 = ((command.inst.cmd1 >> 8) & 0xFF) / 5;
		uint32 dwV2 = (command.inst.cmd1 & 0xFF) / 5;

		bTrisAdded |= AddTri(dwV0, dwV1, dwV2);

		command.inst.cmd0 = *pCmdBase++;
		command.inst.cmd1 = *pCmdBase++;
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
}

#endif //RSP_SOTE_H__