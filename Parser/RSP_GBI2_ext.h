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

// Some new GBI2 extension ucodes
void RSP_GBI2_DL_Count(MicroCodeCommand command)
{
	// This cmd is likely to execute number of ucode at the given address
	uint32 dwAddr = RSPSegmentAddr((command.inst.cmd1));
	{
		gDlistStackPointer++;
		gDlistStack[gDlistStackPointer].pc = dwAddr;
		gDlistStack[gDlistStackPointer].countdown = ((command.inst.cmd0)&0xFFFF);
	}
}


void RSP_GBI2_0x8(MicroCodeCommand command)
{
	if( ((command.inst.cmd0)&0x00FFFFFF) == 0x2F && ((command.inst.cmd1)&0xFF000000) == 0x80000000 )
	{
		// V-Rally 64
		RSP_S2DEX_SPObjLoadTxRectR(command);
	}
	else
	{
		RSP_RDP_Nothing(command);
	}
}


