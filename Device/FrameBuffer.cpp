/*
Copyright (C) 2005 Rice1964

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

// ===========================================================================

#include "..\stdafx.h"
#include "..\Parser\UcodeDefs.h"
#include "..\Parser\RSP_Parser.h"

extern TMEMLoadMapInfo g_tmemLoadAddrMap[0x200];	// Totally 4KB TMEM;

// 0 keeps the most recent CI info
// 1 keeps the frame buffer CI info which is being displayed now
// 2 keeps the older frame buffer CI info. This can be used if we are using triple buffer


/* Overview of framebuffer implementation
1) Check if backbuffer has changed, via different detection techniques
2) If changed, we copy the MicroCodeCommandcard's backbuffer to main RAM
3) This is slow due to the reading process, not the writing
*/

RecentCIInfo g_RecentCIInfo[5];
RecentCIInfo *g_uRecentCIInfoPtrs[5] =
{
	&g_RecentCIInfo[0],
	&g_RecentCIInfo[1],
	&g_RecentCIInfo[2],
	&g_RecentCIInfo[3],
	&g_RecentCIInfo[4],
};

int numOfRecentCIInfos = 5;

RecentViOriginInfo g_RecentVIOriginInfo[5];
uint32 dwBackBufferSavedAtFrame=0;

RenderTextureInfo gRenderTextureInfos[20];
int numOfTxtBufInfos = sizeof(gRenderTextureInfos)/sizeof(RenderTextureInfo);
RenderTextureInfo *g_pRenderTextureInfo = NULL;

FrameBufferManager* g_pFrameBufferManager = NULL;

bool LastCIIsNewCI=false;

FrameBufferManager::FrameBufferManager() :
	m_isRenderingToTexture(false),
	m_lastTextureBufferIndex(-1),
	m_curRenderTextureIndex(-1)
{
}

FrameBufferManager::~FrameBufferManager()
{
}

void FrameBufferManager::CloseUp()
{
	for( int i=0; i<numOfTxtBufInfos; i++ )
	{
		SAFE_DELETE(gRenderTextureInfos[i].pRenderTexture);
	}
}

void FrameBufferManager::Initialize()
{
	m_isRenderingToTexture = false;
	m_lastTextureBufferIndex = -1;
	m_curRenderTextureIndex = -1;
	
	status.bCIBufferIsRendered = false;
	status.bN64IsDrawingTextureBuffer = false;
	status.bHandleN64RenderTexture = false;
	status.bN64FrameBufferIsUsed = false;

	memset(&gRenderTextureInfos[0], 0, sizeof(RenderTextureInfo)*numOfTxtBufInfos);
}
// ===========================================================================

uint16 ConvertRGBATo555(uint8 r, uint8 g, uint8 b, uint8 a)
{
	uint8 ar = a>=0x20?1:0;
	return ((r>>3)<<RGBA5551_RedShift) | ((g>>3)<<RGBA5551_GreenShift) | ((b>>3)<<RGBA5551_BlueShift) | ar;//(a>>7);
}

uint16 ConvertRGBATo555(uint32 color32)
{
	return uint16((((color32>>19)&0x1F)<<RGBA5551_RedShift) | (((color32>>11)&0x1F)<<RGBA5551_GreenShift) | (((color32>>3)&0x1F)<<RGBA5551_BlueShift) | ((color32>>31)));
}

void FrameBufferManager::UpdateRecentCIAddr(SetImgInfo &ciinfo)
{
	if( ciinfo.dwAddr == g_uRecentCIInfoPtrs[0]->dwAddr )
		return;

	RecentCIInfo *temp;

	int i;

	for( i=1; i<numOfRecentCIInfos; i++ )
	{
		if( ciinfo.dwAddr == g_uRecentCIInfoPtrs[i]->dwAddr )
		{
			temp = g_uRecentCIInfoPtrs[i];

			for( int j=i; j>0; j-- )
			{
				g_uRecentCIInfoPtrs[j] = g_uRecentCIInfoPtrs[j-1];
			}
			break;
		}
	}

	if( i >= numOfRecentCIInfos )
	{
		temp = g_uRecentCIInfoPtrs[4];
		g_uRecentCIInfoPtrs[4] = g_uRecentCIInfoPtrs[3];
		g_uRecentCIInfoPtrs[3] = g_uRecentCIInfoPtrs[2];
		g_uRecentCIInfoPtrs[2] = g_uRecentCIInfoPtrs[1];
		g_uRecentCIInfoPtrs[1] = g_uRecentCIInfoPtrs[0];
		temp->dwCopiedAtFrame = 0;
		temp->bCopied = false;
	}

	g_uRecentCIInfoPtrs[0] = temp;

	// Fix me here for Mario Tennis
	temp->dwLastWidth = windowSetting.uViWidth;
	temp->dwLastHeight = windowSetting.uViHeight;

	temp->dwFormat = ciinfo.dwFormat;
	temp->dwAddr = ciinfo.dwAddr;
	temp->dwSize = ciinfo.dwSize;
	temp->dwWidth = ciinfo.dwWidth;
	temp->dwHeight = gRDP.scissor.bottom;
	temp->dwMemSize = (temp->dwWidth*temp->dwHeight/2)<<temp->dwSize;
	temp->bCopied = false;
	temp->lastUsedFrame = status.gDlistCount;
	temp->lastSetAtUcode = status.gUcodeCount;
}


/************************************************************************/
/* Mark the ciinfo entry that the ciinfo is used by VI origin register  */
/* in another word, this is a real frame buffer, not a fake frame buffer*/
/* Fake frame buffers are never really used by VI origin				*/
/************************************************************************/
void FrameBufferManager::SetAddrBeDisplayed(uint32 addr)
{
	uint32 viwidth = *g_GraphicsInfo.VI_WIDTH_REG;
	addr &= (g_dwRamSize-1);

	int i;

	for( i=0; i<numOfRecentCIInfos; i++ )
	{
		if( g_uRecentCIInfoPtrs[i]->dwAddr+2*viwidth == addr )
		{
			g_uRecentCIInfoPtrs[i]->bUsedByVIAtFrame = status.gDlistCount;
		}
		else if( addr >= g_uRecentCIInfoPtrs[i]->dwAddr && addr < g_uRecentCIInfoPtrs[i]->dwAddr+0x1000 )
		{
			g_uRecentCIInfoPtrs[i]->bUsedByVIAtFrame = status.gDlistCount;
		}
	}

	for( i=0; i<numOfRecentCIInfos; i++ )
	{
		if( g_RecentVIOriginInfo[i].addr == addr )
		{
			g_RecentVIOriginInfo[i].FrameCount = status.gDlistCount;
			return;
		}
	}

	for( i=0; i<numOfRecentCIInfos; i++ )
	{
		if( g_RecentVIOriginInfo[i].addr == 0 )
		{
			// Never used
			g_RecentVIOriginInfo[i].addr = addr;
			g_RecentVIOriginInfo[i].FrameCount = status.gDlistCount;
			return;
		}
	}

	int index=0;
	uint32 minFrameCount = 0xffffffff;

	for( i=0; i<numOfRecentCIInfos; i++ )
	{
		if( g_RecentVIOriginInfo[i].FrameCount < minFrameCount )
		{
			index = i;
			minFrameCount = g_RecentVIOriginInfo[i].FrameCount;
		}
	}

	g_RecentVIOriginInfo[index].addr = addr;
	g_RecentVIOriginInfo[index].FrameCount = status.gDlistCount;
}

bool FrameBufferManager::HasAddrBeenDisplayed(uint32 addr, uint32 width)
{
	addr &= (g_dwRamSize-1);

	int i;

	for( i=0; i<numOfRecentCIInfos; i++ )
	{
		if( g_uRecentCIInfoPtrs[i]->dwAddr == 0 )
			continue;

		if( g_uRecentCIInfoPtrs[i]->dwAddr == addr )
		{
			if( status.gDlistCount-g_uRecentCIInfoPtrs[i]->bUsedByVIAtFrame < 20 )
				//if( g_uRecentCIInfoPtrs[i]->bUsedByVIAtFrame != 0 )
			{
				return true;
			}
			else
			{
				TXTRBUF_DUMP(TRACE0("This is a new buffer address, the addr is never a displayed buffer"););
				return false;
			}
		}
	}

	for( i=0; i<numOfRecentCIInfos; i++ )
	{
		if( g_RecentVIOriginInfo[i].addr != 0 )
		{
			if( g_RecentVIOriginInfo[i].addr > addr && 
				(g_RecentVIOriginInfo[i].addr - addr)%width == 0 &&
				(g_RecentVIOriginInfo[i].addr - addr)/width <= 4)
			{
				if( status.gDlistCount-g_RecentVIOriginInfo[i].FrameCount < 20 )
					//if( g_RecentVIOriginInfo[i].FrameCount != 0 )
				{
					return true;
				}
				else
				{
					TXTRBUF_DUMP(DebuggerAppendMsg("This is a new buffer address, the addr is never a displayed buffer"););
					return false;
				}
			}
		}
	}

	if( status.gDlistCount > 20 )
		return false;
	else
	{
		TXTRBUF_DUMP({DebuggerAppendMsg("This is a new buffer address, the addr is never a displayed buffer");});
		return true;
	}
}

int FrameBufferManager::FindRecentCIInfoIndex(uint32 addr)
{
	for( int i=0; i<numOfRecentCIInfos; i++ )
	{
		if( g_uRecentCIInfoPtrs[i]->dwAddr <= addr && addr < g_uRecentCIInfoPtrs[i]->dwAddr+g_uRecentCIInfoPtrs[i]->dwMemSize )
		{
			return i;
		}
	}
	return -1;
}

bool FrameBufferManager::IsDIaRenderTexture()
{
	// Knowing g_CI and g_ZI

	//if( g_CI.dwWidth )

	bool foundSetScissor=false;
	bool foundFillRect=false;
	bool foundSetFillColor=false;
	bool foundSetCImg=false;
	bool foundTxtRect=false;
	int ucodeLength=10;
	int height;
	uint32 newFillColor;

	uint32 dwPC = gDlistStack.address[gDlistStackPointer];		// This points to the next instruction

	for( int i=0; i<10; i++ )
	{
		uint32 w0 = *(uint32 *)(g_pu8RamBase + dwPC + i*8);
		uint32 w1 = *(uint32 *)(g_pu8RamBase + dwPC + 4 + i*8);

		if( (w0>>24) == RDP_SETSCISSOR )
		{
			height   = ((w1>>0 )&0xFFF)/4;
			foundSetScissor = true;
			continue;
		}

		if( (w0>>24) == RDP_SETFILLCOLOR )
		{
			height   = ((w1>>0 )&0xFFF)/4;
			foundSetFillColor = true;
			newFillColor = w1;
			continue;
		}

		if( (w0>>24) == RDP_FILLRECT )
		{
			uint32 x0   = ((w1>>12)&0xFFF)/4;
			uint32 y0   = ((w1>>0 )&0xFFF)/4;
			uint32 x1   = ((w0>>12)&0xFFF)/4;
			uint32 y1   = ((w0>>0 )&0xFFF)/4;

			if( x0 == 0 && y0 == 0 )
			{
				if( x1 == g_CI.dwWidth )
				{
					height = y1;
					foundFillRect = true;
					continue;
				}

				if( x1 == g_CI.dwWidth - 1 )
				{
					height = y1+1;
					foundFillRect = true;
					continue;
				}
			}
		}	

		if( (w0>>24) == RDP_TEXRECT )
		{
			foundTxtRect = true;
			break;
		}

		if( (w0>>24) == RDP_SETCIMG )
		{
			foundSetCImg = true;
			break;
		}
	}

	/*
	bool foundSetScissor=false;
	bool foundFillRect=false;
	bool foundSetFillColor=false;
	bool foundSetCImg=false;
	bool foundTxtRect=false;
	int ucodeLength=10;
	uint32 newFillColor;
	*/

	if( foundFillRect )
	{
		if( foundSetFillColor )
		{
			if( newFillColor != 0xFFFCFFFC )
				return true;	// this is a render_texture
			else
				return false;
		}

		if( gRDP.fillColor != 0x00FFFFF7 )
			return true;	// this is a render_texture
		else
			return false;	// this is a normal ZImg
	}
	else if( foundSetFillColor && newFillColor == 0xFFFCFFFC && foundSetCImg )
	{
		return false;
	}
	else
		return true;


	if( !foundSetCImg )
		return true;

	if( foundSetScissor )
		return true;
}

// Return -1 if the addr is not in recent frame buffer addr
// Return 1 if the addr is in the frame buffer which is currently being displayed
// this function will not return 0 which means the addr is within the current rendering frame buffer
//     this should not happen 
int FrameBufferManager::IsAddrInRecentFrameBuffers(uint32 addr)
{
	if( (addr >= g_uRecentCIInfoPtrs[1]->dwAddr && addr < g_uRecentCIInfoPtrs[1]->dwAddr+g_uRecentCIInfoPtrs[1]->dwMemSize) ||
		(addr >= g_uRecentCIInfoPtrs[0]->dwAddr && addr < g_uRecentCIInfoPtrs[0]->dwAddr+g_uRecentCIInfoPtrs[0]->dwMemSize && status.bHandleN64RenderTexture))
		return 1;
	return -1;
}

int FrameBufferManager::CheckAddrInBackBuffers(uint32 addr, uint32 memsize, bool copyToRDRAM)
{
	int r = FindRecentCIInfoIndex(addr);

	if( r >= 0 )
	{
		// Also check if the address is overwritten by a recent render_texture
		//int t = CheckAddrInRenderTextures(addr,false);
		int t =-1;
		for( int i=0; i<numOfTxtBufInfos; i++ )
		{
			uint32 bufHeight = gRenderTextureInfos[i].knownHeight ? gRenderTextureInfos[i].N64Height : gRenderTextureInfos[i].maxUsedHeight;
			uint32 bufMemSize = gRenderTextureInfos[i].CI_Info.dwSize*gRenderTextureInfos[i].N64Width*bufHeight;
			if( addr >=gRenderTextureInfos[i].CI_Info.dwAddr && addr < gRenderTextureInfos[i].CI_Info.dwAddr+bufMemSize)
			{
				if( g_uRecentCIInfoPtrs[r]->lastSetAtUcode < gRenderTextureInfos[i].updateAtUcodeCount )
				{
					t = i;
					break;
				}
			}
		}

		if( t >= 0 )
			return -1;
	}

	if( r >= 0 && status.gDlistCount - g_uRecentCIInfoPtrs[r]->lastUsedFrame <= 3  && g_uRecentCIInfoPtrs[r]->bCopied == false )
	{
		DEBUGGER_IF_DUMP((logTextureBuffer&&r==1),TRACE2("Hit current front buffer at %08X, size=0x%X", addr, memsize));
		DEBUGGER_IF_DUMP((logTextureBuffer&&r==0),TRACE2("Hit current back buffer at %08X, size=0x%X", addr, memsize));
		DEBUGGER_IF_DUMP((logTextureBuffer&&r>1),TRACE2("Hit old back buffer at %08X, size=0x%X", addr, memsize));

		SaveBackBuffer(r, NULL, true);
	}		

	return r;
}


uint8 CIFindIndex(uint16 val)
{
	for( int i=0; i<=0xFF; i++ )
	{
		if( val == g_wRDPTlut[i] )
		{
			return (uint8)i;
		}
	}
	return 0;
}


void TexRectToFrameBuffer_8b(uint32 dwXL, uint32 dwYL, uint32 dwXH, uint32 dwYH, float t0u0, float t0v0, float t0u1, float t0v1, uint32 dwTile)
{
	// Copy the framebuffer texture into the N64 framebuffer memory
	// Used in Yoshi

	/*
	uint32 maxW = g_pRenderTextureInfo->CI_Info.dwWidth;
	uint32 maxH = maxW*3/4;
	if( status.dwTvSystem == TV_SYSTEM_PAL )
	{
	maxH = maxW*9/11;
	}
	*/

	uint32 maxW = g_pRenderTextureInfo->N64Width;
	uint32 maxH = g_pRenderTextureInfo->N64Height;

	uint32 maxOff = maxW*maxH;

	TMEMLoadMapInfo &info = g_tmemLoadAddrMap[gRDP.tiles[dwTile].dwTMem];
	uint32 dwWidth = dwXH-dwXL;
	uint32 dwHeight = dwYH-dwYL;

	float xScale = (t0u1-t0u0)/dwWidth;
	float yScale = (t0v1-t0v0)/dwHeight;

	uint8* dwSrc = g_pu8RamBase + info.dwLoadAddress;
	uint8* dwDst = g_pu8RamBase + g_pRenderTextureInfo->CI_Info.dwAddr;

	uint32 dwSrcPitch = gRDP.tiles[dwTile].dwPitch;
	uint32 dwDstPitch = g_pRenderTextureInfo->CI_Info.dwWidth;

	uint32 dwSrcOffX = gRDP.tiles[dwTile].hilite_sl;
	uint32 dwSrcOffY = gRDP.tiles[dwTile].hilite_tl;

	uint32 dwLeft = dwXL;
	uint32 dwTop = dwYL;

	dwWidth = min(dwWidth,maxW-dwLeft);
	dwHeight = min(dwHeight, maxH-dwTop);
	if( maxH <= dwTop )	return;

	for (uint32 y = 0; y < dwHeight; y++)
	{
		uint32 dwByteOffset = (uint32)(((y*yScale+dwSrcOffY) * dwSrcPitch) + dwSrcOffX);

		for (uint32 x = 0; x < dwWidth; x++)
		{
			if( (((y+dwTop)*dwDstPitch+x+dwLeft)^0x3) > maxOff )
			{
#ifdef _DEBUG
				TRACE0("Warning: Offset exceeds limit");
#endif
				continue;
			}
			dwDst[((y+dwTop)*dwDstPitch+x+dwLeft)^0x3] = dwSrc[(uint32)(dwByteOffset+x*xScale) ^ 0x3];
		}
	}

	TXTRBUF_DUMP(DebuggerAppendMsg("TexRect To FrameBuffer: X0=%d, Y0=%d, X1=%d, Y1=%d,\n\t\tfS0=%f, fT0=%f, fS1=%f, fT1=%f ",
		dwXL, dwYL, dwXH, dwYH, t0v0, t0v0, t0u1, t0v1););
}

void TexRectToN64FrameBuffer_16b(uint32 x0, uint32 y0, uint32 width, uint32 height, uint32 dwTile)
{
	// Copy the framebuffer texture into the N64 RDRAM framebuffer memory structure

	DrawInfo srcInfo;	
	if( g_textures[dwTile].m_pCTexture->StartUpdate(&srcInfo) == false )
	{
		DebuggerAppendMsg("Fail to lock texture:TexRectToN64FrameBuffer_16b" );
		return;
	}

	uint32 n64CIaddr = g_CI.dwAddr;
	uint32 n64CIwidth = g_CI.dwWidth;

	for (uint32 y = 0; y < height; y++)
	{
		uint32* pSrc = (uint32*)((uint8*)srcInfo.lpSurface + y * srcInfo.lPitch);
		uint16* pN64Buffer = (uint16*)(g_pu8RamBase+(n64CIaddr&(g_dwRamSize-1)))+(y+y0)*n64CIwidth;

		for (uint32 x = 0; x < width; x++)
		{
			pN64Buffer[x+x0] = ConvertRGBATo555(pSrc[x]);
		}
	}

	g_textures[dwTile].m_pCTexture->EndUpdate(&srcInfo);
}


extern uint32 dwAsmCRC;
uint32 CalculateRDRAMCRC(void *pPhysicalAddress, uint32 left, uint32 top, uint32 width, uint32 height, uint32 size, uint32 pitchInBytes)
{
	try
	{
		//If where not loading or dumping textures then lets use a speedy hash
		if (!options.bLoadHiResTextures && !options.bDumpTexturesToFiles)
		{
			//Code by CornN64
			dwAsmCRC = (uint32)pPhysicalAddress;
			register uint32 *pStart = (uint32*)(pPhysicalAddress);
			register uint32 *pEnd = pStart;
			
			uint32 pitch = pitchInBytes >> 2;
			pStart += (top * pitch) + (((left << size) + 1) >> 3);
			pEnd += ((top + height) * pitch) + ((((left + width) << size) + 1) >> 3);
			
			uint32 SizeInDWORD = (uint32)(pEnd - pStart);
			uint32 pinc = SizeInDWORD >> 2;
			
			if (pinc < 1) pinc = 1;
			if (pinc > 23) pinc = 23;
			do
			{
				dwAsmCRC = ((dwAsmCRC << 1) | (dwAsmCRC >> 31)) ^ *pStart;	//This combines to a single instruction in ARM assembler EOR ...,ROR #31 :)
				pStart += pinc;
			}while (pStart < pEnd);
		}
		else
		{
			dwAsmCRC = 0;
			const uint32 bytesPerLine = ((width << size) + 1) / 2;

			uint8* pStart = (uint8*)(pPhysicalAddress);
			pStart += (top * pitchInBytes) + (((left << size) + 1) >> 1);

			int y = height - 1;

			while (y >= 0)
			{
				uint32 esi = 0;
				int x = bytesPerLine - 4;
				while (x >= 0)
				{
					esi = *(uint32*)(pStart + x);
					esi ^= x;

					dwAsmCRC = (dwAsmCRC << 4) + ((dwAsmCRC >> 28) & 15);
					dwAsmCRC += esi;
					x -= 4;
				}
				esi ^= y;
				dwAsmCRC += esi;
				pStart += pitchInBytes;
				y--;
			}
		}
	}
	catch (...)
	{
		TRACE0("Exception in texture CRC calculation");
	}
	return dwAsmCRC;
}

BYTE CalculateMaxCI(void *pPhysicalAddress, uint32 left, uint32 top, uint32 width, uint32 height, uint32 size, uint32 pitchInBytes )
{
	uint32 x, y;
	BYTE *buf;
	BYTE val = 0;

	if( TXT_SIZE_8b == size )
	{
		for( y = 0; y<height; y++ )
		{
			buf = (BYTE*)pPhysicalAddress + left + pitchInBytes * (y+top);
			for( x=0; x<width; x++ )
			{
				if( buf[x] > val )	val = buf[x];
				if( val == 0xFF )
					return 0xFF;
			}
		}
	}
	else
	{
		BYTE val1,val2;
		left >>= 1;
		width >>= 1;
		for( y = 0; y<height; y++ )
		{
			buf = (BYTE*)pPhysicalAddress + left + pitchInBytes * (y+top);
			for( x=0; x<width; x++ )
			{
				val1 = buf[x]>>4;
				val2 = buf[x]&0xF;
				if( val1 > val )	val = val1;
				if( val2 > val )	val = val2;
				if( val == 0xF )
					return 0xF;
			}
		}
	}

	return val;
}

bool FrameBufferManager::FrameBufferInRDRAMCheckCRC()
{
	RecentCIInfo &p = *(g_uRecentCIInfoPtrs[0]);
	uint8 *pFrameBufferBase = (uint8*)(g_pu8RamBase+p.dwAddr);
	uint32 pitch = (p.dwWidth << p.dwSize ) >> 1;
	uint32 crc = CalculateRDRAMCRC(pFrameBufferBase, 0, 0, p.dwWidth, p.dwHeight, p.dwSize, pitch);
	if( crc != p.dwCRC )
	{
		p.dwCRC = crc;
		TRACE0("Frame Buffer CRC mismitch, it is modified by CPU");
		return false;
	}
	else
	{
		return true;
	}
}

void FrameBufferManager::UpdateFrameBufferBeforeUpdateFrame()
{
	if( (frameBufferOptions.bLoadBackBufFromRDRAM && !FrameBufferInRDRAMCheckCRC() ) )		
		// Checks if frame buffer has been modified by CPU
		// Only happens to Dr. Mario
	{
		if (CRender::IsAvailable())
		{
			RecentCIInfo &p = *(g_uRecentCIInfoPtrs[0]);
			CRender::GetRender()->DrawFrameBuffer(false, 0,0,p.dwWidth,p.dwHeight);
			ClearN64FrameBufferToBlack();
		}
	}
}

uint32 FrameBufferManager::ComputeCImgHeight(SetImgInfo &info, uint32 &height)
{
	uint32 dwPC = gDlistStack.address[gDlistStackPointer];		// This points to the next instruction

	for( int i=0; i<10; i++ )
	{
		uint32 w0 = *(uint32 *)(g_pu8RamBase + dwPC + i*8);
		uint32 w1 = *(uint32 *)(g_pu8RamBase + dwPC + 4 + i*8);

		if( (w0>>24) == RDP_SETSCISSOR )
		{
			height   = ((w1>>0 )&0xFFF)/4;
			TXTRBUF_DETAIL_DUMP(TRACE1("buffer height = %d", height));
			return RDP_SETSCISSOR;
		}

		if( (w0>>24) == RDP_FILLRECT )
		{
			uint32 x0   = ((w1>>12)&0xFFF)/4;
			uint32 y0   = ((w1>>0 )&0xFFF)/4;
			uint32 x1   = ((w0>>12)&0xFFF)/4;
			uint32 y1   = ((w0>>0 )&0xFFF)/4;

			if( x0 == 0 && y0 == 0 )
			{
				if( x1 == info.dwWidth )
				{
					height = y1;
					TXTRBUF_DETAIL_DUMP(TRACE1("buffer height = %d", height));
					return RDP_FILLRECT;
				}

				if( x1 == info.dwWidth - 1 )
				{
					height = y1+1;
					TXTRBUF_DETAIL_DUMP(TRACE1("buffer height = %d", height));
					return RDP_FILLRECT;
				}
			}
		}	

		if( (w0>>24) == RDP_SETCIMG )
		{
			goto step2;
		}

		if( (w0>>24) == RDP_SETCIMG )
		{
			goto step2;
		}
	}

	if( gRDP.scissor.left == 0 && gRDP.scissor.top == 0 && gRDP.scissor.right == info.dwWidth )
	{
		height = gRDP.scissor.bottom;
		TXTRBUF_DETAIL_DUMP(TRACE1("buffer height = %d", height));
		return RDP_SETSCISSOR+1;
	}

step2:
	TXTRBUF_DETAIL_DUMP(TRACE0("Not sure about buffer height"));

	height = info.dwWidth*3/4;
	if( status.dwTvSystem == TV_SYSTEM_PAL )
	{
		height = info.dwWidth*9/11;
	}

	if( gRDP.scissor.bottom < (int)height && gRDP.scissor.bottom != 0 )
	{
		height = gRDP.scissor.bottom;
	}

	if( info.dwAddr + height*info.dwWidth*info.dwSize >= g_dwRamSize )
	{
		height = info.dwWidth*3/4;
		if( status.dwTvSystem == TV_SYSTEM_PAL )
		{
			height = info.dwWidth*9/11;
		}

		if( gRDP.scissor.bottom < (int)height && gRDP.scissor.bottom != 0 )
		{
			height = gRDP.scissor.bottom;
		}

		if( info.dwAddr + height*info.dwWidth*info.dwSize >= g_dwRamSize )
		{
			height = ( g_dwRamSize - info.dwAddr ) / info.dwWidth;
		}
	}

	TXTRBUF_DETAIL_DUMP(TRACE1("render_texture height = %d", height));
	return 0;
}

int FrameBufferManager::CheckRenderTexturesWithNewCI(SetImgInfo &CIinfo, uint32 height, bool byNewTxtrBuf)
{
	int matchidx = -1;
	uint32 memsize = ((height*CIinfo.dwWidth)>>1)<<CIinfo.dwSize;

	for( int i=0; i<numOfTxtBufInfos; i++ )
	{
		RenderTextureInfo &info = gRenderTextureInfos[i];
		if( !info.isUsed )	continue;

		bool covered = false;

		if( info.CI_Info.dwAddr == CIinfo.dwAddr )
		{
			if( info.CI_Info.dwSize == CIinfo.dwSize &&
				info.CI_Info.dwWidth == CIinfo.dwWidth &&
				info.CI_Info.dwFormat == CIinfo.dwFormat &&
				info.N64Height == height 
				&& info.CI_Info.dwAddr == CIinfo.dwAddr 
				)
			{
				// This is the same texture at the same address
				if( byNewTxtrBuf )
				{
					matchidx = i;
					break;
				}
			}

			// At the same address, but not the same size
			//SAFE_DELETE(info.psurf);
			covered = true;
		}

		if( !covered )
		{
			uint32 memsize2 = ((info.N64Height*info.N64Width)>>1)<<info.CI_Info.dwSize;

			if( info.CI_Info.dwAddr > CIinfo.dwAddr && info.CI_Info.dwAddr < CIinfo.dwAddr + memsize)
				covered = true;
			else if( info.CI_Info.dwAddr+memsize2 > CIinfo.dwAddr && info.CI_Info.dwAddr+memsize2 < CIinfo.dwAddr + memsize)
				covered = true;
			else if( CIinfo.dwAddr > info.CI_Info.dwAddr && CIinfo.dwAddr < info.CI_Info.dwAddr + memsize2 )
				covered = true;
			else if( CIinfo.dwAddr+ memsize > info.CI_Info.dwAddr && CIinfo.dwAddr+ memsize < info.CI_Info.dwAddr + memsize2 )
				covered = true;
		}

		if( covered )
		{
			//SAFE_DELETE(info.psurf);
			if( info.pRenderTexture->IsBeingRendered() )
			{
				TRACE0("Error, covering a render_texture which is being rendered");
				TRACE3("New addrr=%08X, width=%d, height=%d", CIinfo.dwAddr, CIinfo.dwWidth, height );
				TRACE3("Old addrr=%08X, width=%d, height=%d", info.CI_Info.dwAddr, info.N64Width, info.N64Height );
			}
			info.isUsed = false;
			TXTRBUF_DUMP(TRACE5("Delete txtr buf %d at %08X, covered by new CI at %08X, Width=%d, Height=%d", 
				i, info.CI_Info.dwAddr, CIinfo.dwAddr, CIinfo.dwWidth, height ));
			SAFE_DELETE(info.pRenderTexture);
			info.txtEntry.pTexture = NULL;
			continue;
		}
	}

	return matchidx;
}

extern RecentCIInfo *g_uRecentCIInfoPtrs[5];
RenderTextureInfo newRenderTextureInfo;

int FrameBufferManager::FindASlot(void)
{
	int idx;

	// Find an empty slot
	bool found = false;
	for( int i=0; i<numOfTxtBufInfos; i++ )
	{
		if( !gRenderTextureInfos[i].isUsed && gRenderTextureInfos[i].updateAtFrame < status.gDlistCount )
		{
			found = true;
			idx = i;
			break;
		}
	}

	// If cannot find an empty slot, find the oldest slot and reuse the slot
	if( !found )
	{
		uint32 oldestCount=0xFFFFFFFF;
		uint32 oldestIdx = 0;
		for( int i=0; i<numOfTxtBufInfos; i++ )
		{
			if( gRenderTextureInfos[i].updateAtUcodeCount < oldestCount )
			{
				oldestCount = gRenderTextureInfos[i].updateAtUcodeCount;
				oldestIdx = i;
			}
		}

		idx = oldestIdx;
	}

	DEBUGGER_IF_DUMP((logTextureBuffer && gRenderTextureInfos[idx].pRenderTexture ),TRACE2("Delete txtr buf %d at %08X, to reuse it.", idx, gRenderTextureInfos[idx].CI_Info.dwAddr ));
	SAFE_DELETE(gRenderTextureInfos[idx].pRenderTexture) ;

	return idx;
}


void FrameBufferManager::SetRenderTexture(void)
{
	memcpy(&(newRenderTextureInfo.CI_Info), &g_CI, sizeof(SetImgInfo));

	newRenderTextureInfo.N64Width = newRenderTextureInfo.CI_Info.dwWidth;
	newRenderTextureInfo.knownHeight = ComputeCImgHeight(g_CI, newRenderTextureInfo.N64Height);

	status.bHandleN64RenderTexture = true;
	newRenderTextureInfo.maxUsedHeight = 0;

	newRenderTextureInfo.bufferWidth = newRenderTextureInfo.N64Width;
	newRenderTextureInfo.bufferHeight = newRenderTextureInfo.N64Height;

	newRenderTextureInfo.scaleX = newRenderTextureInfo.bufferWidth / float(newRenderTextureInfo.N64Width);
	newRenderTextureInfo.scaleY = newRenderTextureInfo.bufferHeight / float(newRenderTextureInfo.N64Height);

	status.bFrameBufferIsDrawn = false;
	status.bFrameBufferDrawnByTriangles = false;

	newRenderTextureInfo.updateAtFrame = status.gDlistCount;
	newRenderTextureInfo.updateAtUcodeCount = status.gUcodeCount;

	// Delay activation of the render_texture until the 1st rendering

	TXTRBUF_DUMP(TRACE1("Set render_texture: addr=%08X", g_CI.dwAddr));
	DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_RENDER_TEXTURE, 
	{DebuggerAppendMsg("Paused after setting render_texture:\nAddr: 0x%08x, Fmt: %s Size: %s Width: %d, Height:%d",
	g_CI.dwAddr, pszImgFormat[g_CI.dwFormat], pszImgSize[g_CI.dwSize], g_CI.dwWidth, g_pRenderTextureInfo->N64Height);});
}

int FrameBufferManager::SetBackBufferAsRenderTexture(SetImgInfo &CIinfo, int ciInfoIdx)
{
/* MUDLORD:
OK, heres the drill!
* We  set the graphics card's back buffer's contents as a render_texure
* This is done due to how the current framebuffer implementation detects
  changes to the backbuffer memory pointer and then we do a texture
  copy. This might be slow since it doesnt use hardware auxillary buffers*/
         

	RenderTextureInfo tempRenderTextureInfo;

	memcpy(&(tempRenderTextureInfo.CI_Info), &CIinfo, sizeof(SetImgInfo));

	tempRenderTextureInfo.N64Width = g_uRecentCIInfoPtrs[ciInfoIdx]->dwLastWidth;
	tempRenderTextureInfo.N64Height = g_uRecentCIInfoPtrs[ciInfoIdx]->dwLastHeight;
	tempRenderTextureInfo.knownHeight = true;
	tempRenderTextureInfo.maxUsedHeight = 0;

	tempRenderTextureInfo.bufferWidth = windowSetting.uDisplayWidth;
	tempRenderTextureInfo.bufferHeight = windowSetting.uDisplayHeight;

	tempRenderTextureInfo.scaleX = tempRenderTextureInfo.bufferWidth / float(tempRenderTextureInfo.N64Width);
	tempRenderTextureInfo.scaleY = tempRenderTextureInfo.bufferHeight / float(tempRenderTextureInfo.N64Height);

	status.bFrameBufferIsDrawn = false;
	status.bFrameBufferDrawnByTriangles = false;

	tempRenderTextureInfo.updateAtFrame = status.gDlistCount;
	tempRenderTextureInfo.updateAtUcodeCount = status.gUcodeCount;

	// Checking against previous render_texture infos
	uint32 memsize = ((tempRenderTextureInfo.N64Height*tempRenderTextureInfo.N64Width)>>1)<<tempRenderTextureInfo.CI_Info.dwSize;
	int matchidx = CheckRenderTexturesWithNewCI(CIinfo,tempRenderTextureInfo.N64Height,false);
	int idxToUse = (matchidx >= 0) ? matchidx : FindASlot();

	if( gRenderTextureInfos[idxToUse].pRenderTexture == NULL || matchidx < 0 )
	{
		gRenderTextureInfos[idxToUse].pRenderTexture = 
				new CDXRenderTexture(tempRenderTextureInfo.bufferWidth, tempRenderTextureInfo.bufferHeight, &gRenderTextureInfos[idxToUse], AS_BACK_BUFFER_SAVE);
	}

	// Need to set all variables for gRenderTextureInfos[idxToUse]
	CRenderTexture *pRenderTexture = gRenderTextureInfos[idxToUse].pRenderTexture;
	memcpy(&gRenderTextureInfos[idxToUse], &tempRenderTextureInfo, sizeof(RenderTextureInfo) );
	gRenderTextureInfos[idxToUse].pRenderTexture = pRenderTexture;
	gRenderTextureInfos[idxToUse].isUsed = true;
	gRenderTextureInfos[idxToUse].txtEntry.pTexture = pRenderTexture->m_pTexture;
	gRenderTextureInfos[idxToUse].txtEntry.txtrBufIdx = idxToUse+1;

	TXTRBUF_DUMP(TRACE2("Set back buf as render_texture %d, addr=%08X", idxToUse, CIinfo.dwAddr));
	DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_RENDER_TEXTURE, 
	{DebuggerAppendMsg("Paused after setting render_texture:\nAddr: 0x%08x, Fmt: %s Size: %s Width: %d, Height:%d",
	CIinfo.dwAddr, pszImgFormat[CIinfo.dwFormat], pszImgSize[CIinfo.dwSize], CIinfo.dwWidth, g_pRenderTextureInfo->N64Height);});

	return idxToUse;
}
//close render_texture
void FrameBufferManager::CloseRenderTexture(bool toSave)
{
	if( m_curRenderTextureIndex < 0 )	return;

	status.bHandleN64RenderTexture = false;

	RestoreNormalBackBuffer();
	if( !toSave || !status.bFrameBufferIsDrawn || !status.bFrameBufferDrawnByTriangles )
	{
		TXTRBUF_DUMP(TRACE0("Closing render_texture without save"););
		SAFE_DELETE(gRenderTextureInfos[m_curRenderTextureIndex].pRenderTexture);
		gRenderTextureInfos[m_curRenderTextureIndex].isUsed = false;
		TXTRBUF_DUMP(TRACE1("Delete render_texture %d",m_curRenderTextureIndex););
	}
	else
	{
		TXTRBUF_DUMP(TRACE1("Closing render_texture %d", m_curRenderTextureIndex););
		StoreRenderTextureToRDRAM();

		if( frameBufferOptions.bRenderTextureWriteBack )
		{
			SAFE_DELETE(gRenderTextureInfos[m_curRenderTextureIndex].pRenderTexture);
			gRenderTextureInfos[m_curRenderTextureIndex].isUsed = false;
			TXTRBUF_DUMP(TRACE1("Delete render_texture %d after writing back to RDRAM",m_curRenderTextureIndex););
		}
		else
		{
			g_pRenderTextureInfo->crcInRDRAM = ComputeRenderTextureCRCInRDRAM(m_curRenderTextureIndex);
			g_pRenderTextureInfo->crcCheckedAtFrame = status.gDlistCount;
		}
	}

	SetScreenMult(windowSetting.uDisplayWidth/windowSetting.fViWidth, windowSetting.uDisplayHeight/windowSetting.fViHeight);
	CRender::g_pRender->UpdateClipRectangle();
	CRender::g_pRender->ApplyScissorWithClipRatio();

	DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_RENDER_TEXTURE, 
	{
		DebuggerAppendMsg("Paused after saving render_texture %d:\nAddr: 0x%08x, Fmt: %s Size: %s Width: %d", m_curRenderTextureIndex,
			g_pRenderTextureInfo->CI_Info.dwAddr, pszImgFormat[g_pRenderTextureInfo->CI_Info.dwFormat], pszImgSize[g_pRenderTextureInfo->CI_Info.dwSize], g_pRenderTextureInfo->CI_Info.dwWidth);
	});
}
//clears the framebuffer
void FrameBufferManager::ClearN64FrameBufferToBlack(uint32 left, uint32 top, uint32 width, uint32 height)
{
	RecentCIInfo &p = *(g_uRecentCIInfoPtrs[0]);
	uint16 *frameBufferBase = (uint16*)(g_pu8RamBase+p.dwAddr);
	uint32 pitch = p.dwWidth;

	if( width == 0 || height == 0 )
	{
		uint32 len = p.dwHeight*p.dwWidth*p.dwSize;
		if( p.dwSize == TXT_SIZE_4b ) len = (p.dwHeight*p.dwWidth)>>1;
		memset(frameBufferBase, 0, len);
	}
	else
	{
		for( uint32 y=0; y<height; y++)
		{
			for( uint32 x=0; x<width; x++ )
			{
				*(frameBufferBase+(y+top)*pitch+x+left) = 0;
			}
		}
	}
}

uint8 RevTlutTable[0x10000];
bool RevTlutTableNeedUpdate = false;
void InitTlutReverseLookup(void)
{
	if( RevTlutTableNeedUpdate )
	{
		memset(RevTlutTable, 0, 0x10000);
		for( int i=0; i<=0xFF; i++ )
		{
			RevTlutTable[g_wRDPTlut[i]] = uint8(i);
		}

		RevTlutTableNeedUpdate = false;
	}
}

//check render_texture memory addresses
int	FrameBufferManager::CheckAddrInRenderTextures(uint32 addr, bool checkcrc)
{
	for( int i=0; i<numOfTxtBufInfos; i++ )
	{
		if( !gRenderTextureInfos[i].isUsed )	
			continue;

		if( gRenderTextureInfos[i].pRenderTexture->IsBeingRendered() )
			continue;

		uint32 bufHeight = gRenderTextureInfos[i].knownHeight ? gRenderTextureInfos[i].N64Height : gRenderTextureInfos[i].maxUsedHeight;
		uint32 bufMemSize = gRenderTextureInfos[i].CI_Info.dwSize*gRenderTextureInfos[i].N64Width*bufHeight;
		if( addr >=gRenderTextureInfos[i].CI_Info.dwAddr && addr < gRenderTextureInfos[i].CI_Info.dwAddr+bufMemSize)
		{
			if(checkcrc)
			{
				// Check the CRC in RDRAM
				if( gRenderTextureInfos[i].crcCheckedAtFrame < status.gDlistCount )
				{
					uint32 crc = ComputeRenderTextureCRCInRDRAM(i);
					if( gRenderTextureInfos[i].crcInRDRAM != crc )
					{
						// RDRAM has been modified by CPU core
						TRACE3("Buf %d CRC in RDRAM changed from %08X to %08X", i, gRenderTextureInfos[i].crcInRDRAM, crc );
						TXTRBUF_DUMP(TRACE2("Delete txtr buf %d at %08X, crcInRDRAM failed.", i, gRenderTextureInfos[i].CI_Info.dwAddr ));
						SAFE_DELETE(gRenderTextureInfos[i].pRenderTexture);
						gRenderTextureInfos[i].isUsed = false;
						continue;
					}
					else
					{
						gRenderTextureInfos[i].crcCheckedAtFrame = status.gDlistCount;
					}
				}
			}

			TXTRBUF_DUMP(TRACE2("Loading texture addr = %08X from txtr buf %d", addr, i));
			return i;
		}
	}

	return -1;
}
//load texture from render_texture buffer
void FrameBufferManager::LoadTextureFromRenderTexture(TxtrCacheEntry* pEntry, int infoIdx)
{
	if( infoIdx < 0 || infoIdx >= numOfTxtBufInfos )
	{
		infoIdx = CheckAddrInRenderTextures(pEntry->ti.Address);
	}

	if( infoIdx >= 0 && gRenderTextureInfos[infoIdx].isUsed && gRenderTextureInfos[infoIdx].pRenderTexture )
	{
		TXTRBUF_DUMP(TRACE1("Loading from render_texture %d", infoIdx));
		gRenderTextureInfos[infoIdx].pRenderTexture->LoadTexture(pEntry);
	}
}

void FrameBufferManager::RestoreNormalBackBuffer()
{
	if( m_curRenderTextureIndex >= 0 && m_curRenderTextureIndex < numOfTxtBufInfos )
	{
		if( gRenderTextureInfos[m_curRenderTextureIndex].pRenderTexture )
			gRenderTextureInfos[m_curRenderTextureIndex].pRenderTexture->SetAsRenderTarget(false);
		m_isRenderingToTexture = false;
		m_lastTextureBufferIndex = m_curRenderTextureIndex;
	}

	if( !status.bFrameBufferIsDrawn || !status.bFrameBufferDrawnByTriangles )
	{
		gRenderTextureInfos[m_curRenderTextureIndex].isUsed = false;
		TXTRBUF_DUMP(TRACE2("Delete txtr buf %d at %08X, it is never rendered", m_curRenderTextureIndex, gRenderTextureInfos[m_curRenderTextureIndex].CI_Info.dwAddr ));
		SAFE_DELETE(gRenderTextureInfos[m_curRenderTextureIndex].pRenderTexture);
	}
}

uint32 FrameBufferManager::ComputeRenderTextureCRCInRDRAM(int infoIdx)
{
	if( infoIdx >= numOfTxtBufInfos || infoIdx < 0 || !gRenderTextureInfos[infoIdx].isUsed )
		return 0;

	RenderTextureInfo &info = gRenderTextureInfos[infoIdx];
	uint32 height = info.knownHeight ? info.N64Height : info.maxUsedHeight;
	uint8 *pAddr = (uint8*)(g_pu8RamBase+info.CI_Info.dwAddr);
	uint32 pitch = (info.N64Width << info.CI_Info.dwSize ) >> 1;

	return CalculateRDRAMCRC(pAddr, 0, 0, info.N64Width, height, info.CI_Info.dwSize, pitch);
}
//activates texture buffer for drawing
void FrameBufferManager::ActiveTextureBuffer(void)
{
	status.bCIBufferIsRendered = true;

	if( status.bHandleN64RenderTexture )
	{
		// Checking against previous render_texture infos
		int matchidx = -1;

		uint32 memsize = ((newRenderTextureInfo.N64Height*newRenderTextureInfo.N64Width)>>1)<<newRenderTextureInfo.CI_Info.dwSize;

		matchidx = CheckRenderTexturesWithNewCI(g_CI,newRenderTextureInfo.N64Height,true);

		int idxToUse=-1;
		if( matchidx >= 0 )
		{
			// Reuse the matched slot
			idxToUse = matchidx;
		}
		else
		{
			idxToUse = FindASlot();
		}

		if( gRenderTextureInfos[idxToUse].pRenderTexture == NULL || matchidx < 0 )
		{
			int w = newRenderTextureInfo.bufferWidth;
			if( newRenderTextureInfo.knownHeight == RDP_SETSCISSOR && newRenderTextureInfo.CI_Info.dwAddr == g_ZI.dwAddr )
			{
				w = gRDP.scissor.right;
			}

			gRenderTextureInfos[idxToUse].pRenderTexture = 
				new CDXRenderTexture(w, newRenderTextureInfo.bufferHeight, &gRenderTextureInfos[idxToUse], AS_RENDER_TARGET);
		}

		// Need to set all variables for gRenderTextureInfos[idxToUse]
		CRenderTexture *pRenderTexture = gRenderTextureInfos[idxToUse].pRenderTexture;
		memcpy(&gRenderTextureInfos[idxToUse], &newRenderTextureInfo, sizeof(RenderTextureInfo) );
		gRenderTextureInfos[idxToUse].pRenderTexture = pRenderTexture;
		gRenderTextureInfos[idxToUse].isUsed = true;
		gRenderTextureInfos[idxToUse].txtEntry.pTexture = pRenderTexture->m_pTexture;
		gRenderTextureInfos[idxToUse].txtEntry.txtrBufIdx = idxToUse+1;

		g_pRenderTextureInfo = &gRenderTextureInfos[idxToUse];

		// Active the render_texture
		if( m_curRenderTextureIndex >= 0 && gRenderTextureInfos[m_curRenderTextureIndex].isUsed && gRenderTextureInfos[m_curRenderTextureIndex].pRenderTexture )
		{
			gRenderTextureInfos[m_curRenderTextureIndex].pRenderTexture->SetAsRenderTarget(false);
			m_isRenderingToTexture = false;
		}

		if( gRenderTextureInfos[idxToUse].pRenderTexture->SetAsRenderTarget(true) )
		{
			m_isRenderingToTexture = true;

			//Clear(CLEAR_COLOR_AND_DEPTH_BUFFER,0x80808080,1.0f);
			if( frameBufferOptions.bFillRectNextTextureBuffer )
				CGraphicsContext::g_pGraphicsContext->Clear(CLEAR_COLOR_BUFFER,gRDP.fillColor,1.0f);
			else if( options.enableHackForGames == HACK_FOR_MARIO_TENNIS && g_pRenderTextureInfo->N64Width > 64 && g_pRenderTextureInfo->N64Width < 300 )
			{
				CGraphicsContext::g_pGraphicsContext->Clear(CLEAR_COLOR_BUFFER,0,1.0f);
			}
			else if( options.enableHackForGames == HACK_FOR_MARIO_TENNIS && g_pRenderTextureInfo->N64Width < 64 && g_pRenderTextureInfo->N64Width > 32 )
			{
				CGraphicsContext::g_pGraphicsContext->Clear(CLEAR_COLOR_BUFFER,0,1.0f);
			}

			m_curRenderTextureIndex = idxToUse;

			//SetScreenMult(1, 1);
			SetScreenMult(gRenderTextureInfos[m_curRenderTextureIndex].scaleX, gRenderTextureInfos[m_curRenderTextureIndex].scaleY);
			CRender::g_pRender->UpdateClipRectangle();
			D3DVIEWPORT9 vp = {0,0,gRenderTextureInfos[idxToUse].bufferWidth,gRenderTextureInfos[idxToUse].bufferHeight,0,1};
			gD3DDevWrapper.SetViewport(&vp);

			// If needed, draw RDRAM into the render_texture
			if( frameBufferOptions.bLoadRDRAMIntoRenderTexture )
			{
				CRender::GetRender()->LoadTxtrBufFromRDRAM();
			}
		}
		else
		{
			TRACE1("Error to set Render Target: %d", idxToUse);
			TRACE1("Addr = %08X", gRenderTextureInfos[idxToUse].CI_Info.dwAddr);
			TRACE2("Width = %d, Height=%d", gRenderTextureInfos[idxToUse].N64Width, gRenderTextureInfos[idxToUse].N64Height);
		}	


		TXTRBUF_DUMP(TRACE2("Rendering to render_texture %d, addr=%08X", idxToUse, g_CI.dwAddr));
		DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_RENDER_TEXTURE, 
		{DebuggerAppendMsg("Paused after activating render_texture:\nAddr: 0x%08x, Fmt: %s Size: %s Width: %d, Height:%d",
		g_CI.dwAddr, pszImgFormat[g_CI.dwFormat], pszImgSize[g_CI.dwSize], g_CI.dwWidth, g_pRenderTextureInfo->N64Height);});
	}
	else
	{
		UpdateRecentCIAddr(g_CI);
		CheckRenderTexturesWithNewCI(g_CI,gRDP.scissor.bottom,false);
	}
}

#define SAVE_CI	{g_CI.dwAddr = newCI.dwAddr;g_CI.dwFormat = newCI.dwFormat;g_CI.dwSize = newCI.dwSize;g_CI.dwWidth = newCI.dwWidth;g_CI.bpl=newCI.bpl;}
//sets CI address for framebuffer copies
void FrameBufferManager::Set_CI_addr(SetImgInfo &newCI)
{
	bool wasDrawingTextureBuffer = status.bN64IsDrawingTextureBuffer;
	status.bN64IsDrawingTextureBuffer = ( newCI.dwSize != TXT_SIZE_16b || newCI.dwFormat != TXT_FMT_RGBA || newCI.dwWidth < 200 || ( newCI.dwAddr != g_ZI.dwAddr && newCI.dwWidth != 512 && !g_pFrameBufferManager->HasAddrBeenDisplayed(newCI.dwAddr, newCI.dwWidth)) );
	status.bN64FrameBufferIsUsed = status.bN64IsDrawingTextureBuffer;

	if( !wasDrawingTextureBuffer && g_CI.dwAddr == g_ZI.dwAddr && status.bCIBufferIsRendered )
	{
		TXTRBUF_DUMP(TRACE0("ZI is rendered"));

		if( options.enableHackForGames != HACK_FOR_CONKER && g_uRecentCIInfoPtrs[0]->bCopied == false )
		{
			// Conker is not actually using a backbuffer
			g_pFrameBufferManager->UpdateRecentCIAddr(g_CI);
			if( status.leftRendered != -1 && status.topRendered != -1 && status.rightRendered != -1 && status.bottomRendered != -1 )
			{
				RECT rect={status.leftRendered,status.topRendered,status.rightRendered,status.bottomRendered};
				g_pFrameBufferManager->SaveBackBuffer(0,&rect);
			}
			else
			{
				g_pFrameBufferManager->SaveBackBuffer(0,NULL);
			}
		}
	}

	frameBufferOptions.bFillRectNextTextureBuffer = false;
	if( g_CI.dwAddr == newCI.dwAddr && status.bHandleN64RenderTexture && (g_CI.dwFormat != newCI.dwFormat || g_CI.dwSize != newCI.dwSize || g_CI.dwWidth != newCI.dwWidth ) )
	{
		// Mario Tennis player shadow
		g_pFrameBufferManager->CloseRenderTexture(true);
		if( options.enableHackForGames == HACK_FOR_MARIO_TENNIS )
			frameBufferOptions.bFillRectNextTextureBuffer = true;	// Hack for Mario Tennis
	}

	SAVE_CI;

	if( g_CI.dwAddr == g_ZI.dwAddr && !status.bN64IsDrawingTextureBuffer )
	{
		if( g_pFrameBufferManager->IsDIaRenderTexture() )
		{
			status.bN64IsDrawingTextureBuffer = true;
			status.bN64FrameBufferIsUsed = status.bN64IsDrawingTextureBuffer;
		}
	}

	status.bCIBufferIsRendered = false;
	status.leftRendered = status.topRendered = status.rightRendered = status.bottomRendered = -1;


	if( frameBufferOptions.bAtEachFrameUpdate && !status.bHandleN64RenderTexture )
	{
		if( status.curRenderBuffer != g_CI.dwAddr )
		{
			if( status.gDlistCount%(currentRomOptions.N64FrameBufferWriteBackControl+1) == 0 )
			{
				g_pFrameBufferManager->StoreBackBufferToRDRAM(status.curRenderBuffer, 
					newCI.dwFormat, newCI.dwSize, windowSetting.uViWidth, windowSetting.uViHeight,
					windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);
			}
		}

		status.curRenderBuffer = g_CI.dwAddr;
	}


	switch( currentRomOptions.N64RenderToTextureEmuType )
	{
	case TXT_BUF_NONE:
		if( status.bHandleN64RenderTexture )
			g_pFrameBufferManager->CloseRenderTexture(false);
		status.bHandleN64RenderTexture = false;	// Don't handle N64 render_texture stuffs
		if( !status.bN64IsDrawingTextureBuffer )
			g_pFrameBufferManager->UpdateRecentCIAddr(g_CI);
		break;
	default:
		if( status.bHandleN64RenderTexture )
		{
#ifdef _DEBUG
			if( pauseAtNext && eventToPause == NEXT_RENDER_TEXTURE )
			{
				pauseAtNext = TRUE;
				eventToPause = NEXT_RENDER_TEXTURE;
			}
#endif
			g_pFrameBufferManager->CloseRenderTexture(true);
		}

		status.bHandleN64RenderTexture = status.bN64IsDrawingTextureBuffer;
		if( status.bHandleN64RenderTexture )
		{
			if( options.enableHackForGames != HACK_FOR_BANJO_TOOIE )
			{
				g_pFrameBufferManager->SetRenderTexture();
			}
		}
		else
		{
#ifdef _DEBUG
			if( g_CI.dwWidth == 512 && pauseAtNext && (eventToPause==NEXT_OBJ_BG || eventToPause==NEXT_SET_CIMG) )
			{
				DebuggerAppendMsg("Warning SetCImg: new Addr=0x%08X, fmt:%s size=%sb, Width=%d\n", 
					g_CI.dwAddr, pszImgFormat[newCI.dwFormat], pszImgSize[newCI.dwSize], newCI.dwWidth);
			}
#endif
			//g_pFrameBufferManager->UpdateRecentCIAddr(g_CI);		// Delay this until the CI buffer is actally drawn
		}
		break;
	}

	TXTRBUF_DUMP(TRACE4("SetCImg : Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", 
		g_CI.dwAddr, pszImgFormat[newCI.dwFormat], pszImgSize[newCI.dwSize], newCI.dwWidth));

	DEBUGGER_PAUSE_AND_DUMP_NO_UPDATE(NEXT_SET_CIMG, 
	{
		DebuggerAppendMsg("Pause after SetCImg: Addr=0x%08X, Fmt:%s-%sb, Width=%d\n", 
			g_CI.dwAddr, pszImgFormat[newCI.dwFormat], pszImgSize[newCI.dwSize], newCI.dwWidth);
	}
	);
}


void FrameBufferManager::StoreRenderTextureToRDRAM(int infoIdx)
{
	if( !frameBufferOptions.bRenderTextureWriteBack )	return;

	if( infoIdx < 0 )
		infoIdx = m_lastTextureBufferIndex;

	if( !gRenderTextureInfos[infoIdx].pRenderTexture )
		return;

	if( gRenderTextureInfos[infoIdx].pRenderTexture->IsBeingRendered() )
	{
		TXTRBUF_DUMP(TRACE1("Cannot SaveTextureBuffer %d, it is being rendered", infoIdx));
		return;
	}

	gRenderTextureInfos[infoIdx].pRenderTexture->StoreToRDRAM(infoIdx);
}

//does FB copy to N64 RDAM structure
void FrameBufferManager::CopyBufferToRDRAM(uint32 addr, uint32 fmt, uint32 siz, uint32 width, uint32 height, uint32 bufWidth, uint32 bufHeight, uint32 startaddr, uint32 memsize, uint32 pitch, void *buffer, uint32 bufPitch)
{
	uint32 startline=0;
	if( startaddr == 0xFFFFFFFF )	startaddr = addr;

	startline = (startaddr-addr)/siz/pitch;
	if( startline >= height )
	{
		//TRACE0("Warning: check me");
		startline = height;
	}

	uint32 endline = height;
	if( memsize != 0xFFFFFFFF )
	{
		endline = (startaddr+memsize-addr)/siz;
		if( endline % pitch == 0 )
			endline /= pitch;
		else
			endline = endline/pitch+1;
	}
	if( endline > height )
	{
		endline = height;
	}

	if( memsize != 0xFFFFFFFF )
	{
		TXTRBUF_DUMP(DebuggerAppendMsg("Start at: 0x%X, from line %d to %d", startaddr-addr, startline, endline););
	}

	int indexes[600];
	{
		float sx;
		int sx0;
		float ratio = bufWidth/(float)width;
		for( uint32 j=0; j<width; j++ )
		{
			sx = j*ratio;
			sx0 = int(sx+0.5);
			indexes[j] = 4*sx0;
		}
	}

	if( siz == TXT_SIZE_16b )
	{
		uint16 *frameBufferBase = (uint16*)(g_pu8RamBase+addr);

		int  sy0;
		float ratio = bufHeight/(float)height;

		for( uint32 i=startline; i<endline; i++ )
		{
			sy0 = int(i*ratio+0.5);

			uint16 *pD = frameBufferBase + i * pitch;
			uint8 *pS0 = (uint8 *)buffer + sy0 * bufPitch;

			for( uint32 j=0; j<width; j++ )
			{
				// Point
				uint8 r = pS0[indexes[j]+2];
				uint8 g = pS0[indexes[j]+1];
				uint8 b = pS0[indexes[j]+0];
				uint8 a = pS0[indexes[j]+3];

				// Liner
				*(pD+(j^1)) = ConvertRGBATo555( r, g, b, a);

			}
		}
	}
	else if( siz == TXT_SIZE_8b && fmt == TXT_FMT_CI )
	{
		uint8 *frameBufferBase = (uint8*)(g_pu8RamBase+addr);

		uint16 tempword;
		InitTlutReverseLookup();

		for( uint32 i=startline; i<endline; i++ )
		{
			uint8 *pD = frameBufferBase + i * width;
			uint8 *pS = (uint8 *)buffer + i*bufHeight/height * bufPitch;
			for( uint32 j=0; j<width; j++ )
			{
				int pos = 4*(j*bufWidth/width);
				tempword = ConvertRGBATo555((pS[pos+2]),		// Red
					(pS[pos+1]),		// G
					(pS[pos+0]),		// B
					(pS[pos+3]));		// Alpha
				//*pD = CIFindIndex(tempword);
				*(pD+(j^3)) = RevTlutTable[tempword];
			}
		}
		DEBUGGER_IF_DUMP(pauseAtNext,{DebuggerAppendMsg("Copy %sb FrameBuffer to Rdram", pszImgSize[siz]);});
	}
	else if( siz == TXT_SIZE_8b && fmt == TXT_FMT_I )
	{
		uint8 *frameBufferBase = (uint8*)(g_pu8RamBase+addr);

		int sy0;
		float ratio = bufHeight/(float)height;

		for( uint32 i=startline; i<endline; i++ )
		{
			sy0 = int(i*ratio+0.5);

			uint8 *pD = frameBufferBase + i * width;
			uint8 *pS0 = (uint8 *)buffer + sy0 * bufPitch;

			for( uint32 j=0; j<width; j++ )
			{
				// Point
				uint32 r = pS0[indexes[j]+2];
				uint32 g = pS0[indexes[j]+1];
				uint32 b = pS0[indexes[j]+0];

				// Liner
				*(pD+(j^3)) = (uint8)((r+b+g)/3);

			}
		}
		DEBUGGER_IF_DUMP(pauseAtNext,{DebuggerAppendMsg("Copy %sb FrameBuffer to Rdram", pszImgSize[siz]);});
	}
}


#ifdef _DEBUG
//displays rendertexture, duh
void FrameBufferManager::DisplayRenderTexture(int infoIdx)
{
	if( infoIdx < 0 )
		infoIdx = m_lastTextureBufferIndex;

	if( gRenderTextureInfos[infoIdx].pRenderTexture )
	{
		if( gRenderTextureInfos[infoIdx].pRenderTexture->IsBeingRendered() )
		{
			TRACE1("Render texture %d is being rendered, cannot display", infoIdx);
		}
		else
		{
			gRenderTextureInfos[infoIdx].pRenderTexture->Display(infoIdx);
			TRACE1("Texture buffer %d:", infoIdx);
			TRACE1("Addr=%08X", gRenderTextureInfos[infoIdx].CI_Info.dwAddr);
			TRACE2("Width=%d, Created Height=%d", gRenderTextureInfos[infoIdx].N64Width,gRenderTextureInfos[infoIdx].N64Height);
			TRACE2("Fmt=%d, Size=%d", gRenderTextureInfos[infoIdx].CI_Info.dwFormat,gRenderTextureInfos[infoIdx].CI_Info.dwSize);
		}
	}
	else
	{
		TRACE1("Texture buffer %d is not used", infoIdx);
	}
}
#endif


//Saves backbuffer
//this is the core to the current framebuffer code
//We need to save backbuffer when changed by framebuffer
//so that we can use it for framebuffer effects
void FrameBufferManager::SaveBackBuffer(int ciInfoIdx, RECT* pSrcRect, bool forceToSaveToRDRAM)
{
	RecentCIInfo &ciInfo = *g_uRecentCIInfoPtrs[ciInfoIdx];

	if( ciInfoIdx == 1 )	// to save the current front buffer
	{
		CGraphicsContext::g_pGraphicsContext->UpdateFrame(true);
	}

	if( frameBufferOptions.bWriteBackBufToRDRAM || forceToSaveToRDRAM )
	{
		uint32 width = ciInfo.dwWidth;
		uint32 height = ciInfo.dwHeight;

		if( ciInfo.dwWidth == *g_GraphicsInfo.VI_WIDTH_REG && ciInfo.dwWidth != windowSetting.uViWidth )
		{
			width = windowSetting.uViWidth;
			height = windowSetting.uViHeight;
		}

		StoreBackBufferToRDRAM( ciInfo.dwAddr, ciInfo.dwFormat, ciInfo.dwSize, width, height, 
			windowSetting.uDisplayWidth, windowSetting.uDisplayHeight);

		g_uRecentCIInfoPtrs[ciInfoIdx]->bCopied = true;
		if( ciInfoIdx == 1 )	// to save the current front buffer
		{
			CGraphicsContext::g_pGraphicsContext->UpdateFrame(true);
		}
		return;
	}


	SetImgInfo tempinfo;
	tempinfo.dwAddr = ciInfo.dwAddr;
	tempinfo.dwFormat = ciInfo.dwFormat;
	tempinfo.dwSize = ciInfo.dwSize;
	tempinfo.dwWidth = ciInfo.dwWidth;

	int idx = SetBackBufferAsRenderTexture(tempinfo, ciInfoIdx);

	CopyBackBufferToRenderTexture(idx, ciInfo, pSrcRect);

	gRenderTextureInfos[idx].crcCheckedAtFrame = status.gDlistCount;
	gRenderTextureInfos[idx].crcInRDRAM = ComputeRenderTextureCRCInRDRAM(idx);

	DEBUGGER_IF_DUMP((logTextureBuffer&&pSrcRect==NULL),TRACE1("SaveBackBuffer at 0x%08X", ciInfo.dwAddr));
	DEBUGGER_IF_DUMP((logTextureBuffer&&pSrcRect),TRACE5("SaveBackBuffer at 0x%08X, {%d,%d -%d,%d)", ciInfo.dwAddr,
		pSrcRect->left,pSrcRect->top,pSrcRect->right,pSrcRect->bottom));
	DEBUGGER_IF_DUMP(( pauseAtNext && eventToPause == NEXT_RENDER_TEXTURE),{g_pFrameBufferManager->DisplayRenderTexture(idx);});

	g_uRecentCIInfoPtrs[ciInfoIdx]->bCopied = true;
}
