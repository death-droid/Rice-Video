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

CTextureManager gTextureManager;
// Returns the first prime greater than or equal to nFirst
inline LONG GetNextPrime(LONG nFirst)
{
	LONG nCurrent;

	LONG i;

	nCurrent = nFirst;

	// Just make sure it's odd
	if ((nCurrent % 2) == 0)
		nCurrent++;

	for (;;)
	{
		LONG nSqrtCurrent;
		BOOL bIsComposite;

		// nSqrtCurrent = nCurrent^0.5 + 1 (round up)
		nSqrtCurrent = (LONG)sqrt((double)nCurrent) + 1;


		bIsComposite = FALSE;
		
		// Test all odd numbers from 3..nSqrtCurrent
		for (i = 3; i <= nSqrtCurrent; i+=2)
		{
			if ((nCurrent % i) == 0)
			{
				bIsComposite = TRUE;
				break;
			}
		}

		if (!bIsComposite)
		{			
			return nCurrent;
		}

		// Select next odd candidate...
		nCurrent += 2;
	}

}



///////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////
CTextureManager::CTextureManager() :
	m_pHead(NULL),
	m_pCacheTxtrList(NULL),
	m_numOfCachedTxtrList(809)
{
	m_numOfCachedTxtrList = GetNextPrime(800);

	m_pCacheTxtrList = new TxtrCacheEntry *[m_numOfCachedTxtrList];
	SAFE_CHECK(m_pCacheTxtrList);

	for (uint32 i = 0; i < m_numOfCachedTxtrList; i++)
		m_pCacheTxtrList[i] = NULL;

	memset(&m_blackTextureEntry, 0, sizeof(TxtrCacheEntry));
	memset(&m_PrimColorTextureEntry, 0, sizeof(TxtrCacheEntry));
	memset(&m_EnvColorTextureEntry, 0, sizeof(TxtrCacheEntry));
	memset(&m_LODFracTextureEntry, 0, sizeof(TxtrCacheEntry));
	memset(&m_PrimLODFracTextureEntry, 0, sizeof(TxtrCacheEntry));
}

CTextureManager::~CTextureManager()
{
	CleanUp();

	delete []m_pCacheTxtrList;
	m_pCacheTxtrList = NULL;	
}


//
//  Delete all textures.
//
bool CTextureManager::CleanUp()
{
	RecycleAllTextures();

	while (m_pHead)
	{
		TxtrCacheEntry * pVictim = m_pHead;
		m_pHead = pVictim->pNext;

		delete pVictim;
	}

	if( m_blackTextureEntry.pTexture )		delete m_blackTextureEntry.pTexture;	
	if( m_PrimColorTextureEntry.pTexture )	delete m_PrimColorTextureEntry.pTexture;
	if( m_EnvColorTextureEntry.pTexture )	delete m_EnvColorTextureEntry.pTexture;
	if( m_LODFracTextureEntry.pTexture )	delete m_LODFracTextureEntry.pTexture;
	if( m_PrimLODFracTextureEntry.pTexture )	delete m_PrimLODFracTextureEntry.pTexture;
	memset(&m_blackTextureEntry, 0, sizeof(TxtrCacheEntry));
	memset(&m_PrimColorTextureEntry, 0, sizeof(TxtrCacheEntry));
	memset(&m_EnvColorTextureEntry, 0, sizeof(TxtrCacheEntry));
	memset(&m_LODFracTextureEntry, 0, sizeof(TxtrCacheEntry));
	memset(&m_PrimLODFracTextureEntry, 0, sizeof(TxtrCacheEntry));

	return true;
}

bool CTextureManager::TCacheEntryIsLoaded(TxtrCacheEntry *pEntry)
{
  for (int i = 0; i < MAX_TEXTURES; i++)
    if (g_textures[i].pTextureEntry == pEntry)
      return true;

  return false;
}

// Purge any textures whos last usage was over 5 seconds ago
// Check here
void CTextureManager::PurgeOldTextures()
{
	if (m_pCacheTxtrList == NULL)
		return;

	static const uint32 dwFramesToKill = 5*30;			// 5 secs at 30 fps
	static const uint32 dwFramesToDelete = 30*30;		// 30 secs at 30 fps
	
	for ( uint32 i = 0; i < m_numOfCachedTxtrList; i++ )
	{
		TxtrCacheEntry * pEntry;
		TxtrCacheEntry * pNext;
		
		pEntry = m_pCacheTxtrList[i];
		while (pEntry)
		{
			pNext = pEntry->pNext;
			
			if ( status.gDlistCount - pEntry->FrameLastUsed > dwFramesToKill && !TCacheEntryIsLoaded(pEntry))
			{
				RemoveTexture(pEntry);
			}
			pEntry = pNext;
		}
	}
	
	
	// Remove any old textures that haven't been recycled in 1 minute or so
	// Normally these would be reused
	TxtrCacheEntry * pPrev;
	TxtrCacheEntry * pCurr;
	TxtrCacheEntry * pNext;
	
	pPrev = NULL;
	pCurr = m_pHead;
	
	while (pCurr)
	{
		pNext = pCurr->pNext;
		
		if ( status.gDlistCount - pCurr->FrameLastUsed > dwFramesToDelete && !TCacheEntryIsLoaded(pCurr) )
		{
			if (pPrev != NULL) pPrev->pNext        = pCurr->pNext;
			else			   m_pHead = pCurr->pNext;
			
			delete pCurr;
			pCurr = pNext;	
		}
		else
		{
			pPrev = pCurr;
			pCurr = pNext;
		}
	}
}

void CTextureManager::RecycleAllTextures()
{
	if (m_pCacheTxtrList == NULL)
		return;
	
	uint32 dwCount = 0;
	uint32 dwTotalUses = 0;

	for (uint32 i = 0; i < m_numOfCachedTxtrList; i++)
	{
		while (m_pCacheTxtrList[i])
		{
			TxtrCacheEntry *pTVictim = m_pCacheTxtrList[i];
			m_pCacheTxtrList[i] = pTVictim->pNext;
			
			dwTotalUses += pTVictim->dwUses;
			dwCount++;
			RecycleTexture(pTVictim);
		}
	}
}

void CTextureManager::RecheckHiresForAllTextures()
{
	if (m_pCacheTxtrList == NULL)
		return;

	for (uint32 i = 0; i < m_numOfCachedTxtrList; i++)
	{
		while (m_pCacheTxtrList[i])
		{
			TxtrCacheEntry *pTVictim = m_pCacheTxtrList[i];
			m_pCacheTxtrList[i] = pTVictim->pNext;
			pTVictim->bExternalTxtrChecked = false;

		}
	}
}


// Add to the recycle list
void CTextureManager::RecycleTexture(TxtrCacheEntry *pEntry)
{
	if (pEntry->pTexture == NULL)
	{
		// No point in saving!
		delete pEntry;
	}
	else
	{
		// Add to the list
		pEntry->pNext = m_pHead;
		// Reset the texture enhancement flag
		pEntry->dwEnhancementFlag = TEXTURE_NO_ENHANCEMENT;

		SAFE_DELETE(pEntry->pEnhancedTexture);

		m_pHead = pEntry;
	}
}

// Search for a texture of the specified dimensions to recycle
TxtrCacheEntry * CTextureManager::ReviveTexture( uint32 width, uint32 height )
{
	TxtrCacheEntry * pPrev;
	TxtrCacheEntry * pCurr;
	
	pPrev = NULL;
	pCurr = m_pHead;
	
	while (pCurr)
	{
		if (pCurr->ti.WidthToCreate == width &&
			pCurr->ti.HeightToCreate == height)
		{
			// Remove from list
			if (pPrev != NULL)
				pPrev->pNext = pCurr->pNext;
			else			   
				m_pHead = pCurr->pNext;
			
			return pCurr;
		}
		
		pPrev = pCurr;
		pCurr = pCurr->pNext;
	}
	
	return NULL;
}

uint32 CTextureManager::Hash(uint32 dwValue)
{
	// Divide by four, because most textures will be on a 4 byte boundry, so bottom four
	// bits are null
	return (dwValue>>2) % m_numOfCachedTxtrList;
}

void CTextureManager::AddTexture(TxtrCacheEntry *pEntry)
{	
	uint32 dwKey = Hash(pEntry->ti.Address);
	
	if (m_pCacheTxtrList == NULL)
		return;
	
	TxtrCacheEntry **p = &m_pCacheTxtrList[dwKey];
	
	// Add to head (not tail, for speed - new textures are more likely to be accessed next)
	pEntry->pNext = m_pCacheTxtrList[dwKey];
	m_pCacheTxtrList[dwKey] = pEntry;
}

TxtrCacheEntry * CTextureManager::GetTxtrCacheEntry(TxtrInfo * pti)
{
	TxtrCacheEntry *pEntry;
	
	if (m_pCacheTxtrList == NULL)
		return NULL;
	
	// See if it is already in the hash table
	uint32 dwKey = Hash(pti->Address);

	for (pEntry = m_pCacheTxtrList[dwKey]; pEntry; pEntry = pEntry->pNext)
	{
		if ( pEntry->ti == *pti )
		{
			return pEntry;
		}
	}

	return NULL;
}

void CTextureManager::RemoveTexture(TxtrCacheEntry * pEntry)
{
	TxtrCacheEntry * pPrev;
	TxtrCacheEntry * pCurr;
	
	if (m_pCacheTxtrList == NULL)
		return;
	
	// See if it is already in the hash table
	uint32 dwKey = Hash(pEntry->ti.Address);
	
	pPrev = NULL;
	pCurr = m_pCacheTxtrList[dwKey];
	
	while (pCurr)
	{
		// Check that the attributes match
		if ( pCurr == pEntry )
		{
			if (pPrev != NULL) 
				pPrev->pNext = pCurr->pNext;
			else
			   m_pCacheTxtrList[dwKey] = pCurr->pNext;

			RecycleTexture(pEntry);

			break;
		}

		pPrev = pCurr;
		pCurr = pCurr->pNext;
	}
	
}
	
TxtrCacheEntry * CTextureManager::CreateNewCacheEntry(uint32 dwAddr, uint32 dwWidth, uint32 dwHeight)
{
	TxtrCacheEntry * pEntry = NULL;

	// Find a used texture
	pEntry = ReviveTexture(dwWidth, dwHeight);

	if (pEntry == NULL)
	{
		// Couldn't find on - recreate!
		pEntry = new TxtrCacheEntry;
		if (pEntry == NULL)
		{
			return NULL;
		}

		pEntry->pTexture = new CTexture(dwWidth, dwHeight);
		if (pEntry->pTexture == NULL || pEntry->pTexture->GetTexture() == NULL)
			TRACE2("Warning, unable to create %d x %d texture!", dwWidth, dwHeight);
	}
	
	// Initialize
	pEntry->ti.Address = dwAddr;
	pEntry->pNext = NULL;
	pEntry->dwUses = 0;
	pEntry->dwTimeLastUsed = status.gRDPTime;
	pEntry->dwCRC = 0;
	pEntry->FrameLastUsed = status.gDlistCount;
	pEntry->FrameLastUpdated = 0;
	pEntry->lastEntry = NULL;
	pEntry->bExternalTxtrChecked = false;
	pEntry->maxCI = -1;

	// Add to the hash table
	AddTexture(pEntry);
	return pEntry;	
}

// If already in table, return
// Otherwise, create surfaces, and load texture into memory
uint32 dwAsmHeight;
uint32 dwAsmPitch;
uint32 dwAsmdwBytesPerLine;
uint32 dwAsmCRC;
uint8* pAsmStart;
TxtrCacheEntry *g_lastTextureEntry=NULL;
bool lastEntryModified = false;

TxtrCacheEntry * CTextureManager::GetTexture(TxtrInfo * pgti, bool fromTMEM, bool AutoExtendTexture)
{
	TxtrCacheEntry *pEntry;

	gRDP.texturesAreReloaded = true;

	dwAsmCRC = 0;
	uint32 dwPalCRC = 0;

	pEntry = GetTxtrCacheEntry(pgti);
	bool loadFromTextureBuffer=false;
	int txtBufIdxToLoadFrom = -1;
	if( (frameBufferOptions.bCheckRenderTextures&&!frameBufferOptions.bWriteBackBufToRDRAM) || (frameBufferOptions.bCheckBackBufs&&!frameBufferOptions.bWriteBackBufToRDRAM) )
	{
		txtBufIdxToLoadFrom = g_pFrameBufferManager->CheckAddrInRenderTextures(pgti->Address);
		if( txtBufIdxToLoadFrom >= 0 )
		{
			loadFromTextureBuffer = true;
			// Check if it is the same size,
			RenderTextureInfo &info = gRenderTextureInfos[txtBufIdxToLoadFrom];
			//if( info.pRenderTexture && info.CI_Info.dwAddr == pgti->Address && info.CI_Info.dwFormat == pgti->Format 
			if( info.pRenderTexture && info.CI_Info.dwFormat == pgti->Format 
				&& info.CI_Info.dwSize == pgti->Size )
			{
				info.txtEntry.ti = *pgti;
				return &info.txtEntry;
			}
		}
	}

	if( frameBufferOptions.bCheckBackBufs && g_pFrameBufferManager->CheckAddrInBackBuffers(pgti->Address, pgti->HeightToLoad*pgti->Pitch) >= 0 )
	{
		if( !frameBufferOptions.bWriteBackBufToRDRAM )
		{
			// Load the texture from recent back buffer
			txtBufIdxToLoadFrom = g_pFrameBufferManager->CheckAddrInRenderTextures(pgti->Address);
			if( txtBufIdxToLoadFrom >= 0 )
			{
				loadFromTextureBuffer = true;
				// Check if it is the same size,
				RenderTextureInfo &info = gRenderTextureInfos[txtBufIdxToLoadFrom];
				//if( info.pRenderTexture && info.CI_Info.dwAddr == pgti->Address && info.CI_Info.dwFormat == pgti->Format 
				if( info.pRenderTexture && info.CI_Info.dwFormat == pgti->Format 
					&& info.CI_Info.dwSize == pgti->Size )
				{
					info.txtEntry.ti = *pgti;
					return &info.txtEntry;
				}
			}
		}
	}

	if (pEntry && pEntry->dwTimeLastUsed == status.gRDPTime && status.gDlistCount != 0 && !status.bN64FrameBufferIsUsed )		// This is not good, Palatte may changes
	{
		// We've already calculated a CRC this frame!
		dwAsmCRC = pEntry->dwCRC;
	}
	else
	{
		if( loadFromTextureBuffer )
			dwAsmCRC = gRenderTextureInfos[txtBufIdxToLoadFrom].crcInRDRAM;
		else
			CalculateRDRAMCRC(pgti->pPhysicalAddress, pgti->LeftToLoad, pgti->TopToLoad, pgti->WidthToLoad, pgti->HeightToLoad, pgti->Size, pgti->Pitch);
	}

	int maxCI = 0;
	if (pgti->Format == TXT_FMT_CI || (pgti->Format == TXT_FMT_RGBA && pgti->Size <= TXT_SIZE_8b ))
	{
		//maxCI = pgti->Size == TXT_SIZE_8b ? 255 : 15;
		extern BYTE CalculateMaxCI(void *pPhysicalAddress, uint32 left, uint32 top, uint32 width, uint32 height, uint32 size, uint32 pitchInBytes );

		if( !pEntry || pEntry->dwCRC != dwAsmCRC || pEntry->maxCI < 0 )
		{
			maxCI = CalculateMaxCI(pgti->pPhysicalAddress, pgti->LeftToLoad, pgti->TopToLoad, pgti->WidthToLoad, pgti->HeightToLoad, pgti->Size, pgti->Pitch);
		}
		else
		{
			maxCI = pEntry->maxCI;
		}

		//Check PAL CRC
		uint8 * pStart;
		uint32 dwPalSize = 16;
		uint32 dwOffset;

		if( pgti->Size == TXT_SIZE_8b )
		{
			dwPalSize = 256;
			dwOffset = 0;
		}
		else
		{
			dwOffset = pgti->Palette << 4;
		}

		pStart = (uint8*)pgti->PalAddress+dwOffset*2;

		uint32 dwAsmCRCSave = dwAsmCRC;
		//dwPalCRC = CalculateRDRAMCRC(pStart, 0, 0, dwPalSize, 1, TXT_SIZE_16b, dwPalSize*2);
		dwPalCRC = CalculateRDRAMCRC(pStart, 0, 0, maxCI+1, 1, TXT_SIZE_16b, dwPalSize*2);
		dwAsmCRC = dwAsmCRCSave;
	}

	// Fix for textures where ti is identical. In this case just the first texture has been added to the Cache.
	// for further instances this texture has just been replaced instead of adding the additional texture to the same index
	// in the cachelist. This was causing the slowdowns. Thus we have to iterate through the bucket of the cache list and see
	// which of the textures that have been placed to it is the one we are looking for
	if(pEntry && pEntry->dwCRC == dwAsmCRC && pEntry->dwPalCRC != dwPalCRC &&
			(!loadFromTextureBuffer || gRenderTextureInfos[txtBufIdxToLoadFrom].updateAtFrame < pEntry->FrameLastUsed ))
	{
		bool bChecksumDoMatch=false;
		// iterate through all textures located in the same bucket
		while(pEntry->pNext)
		{
			// check the next texture in the same bucket
			pEntry = pEntry->pNext;
				// let's see if this one is the one we are actually looking for
				if(pEntry->dwCRC == dwAsmCRC && pEntry->dwPalCRC == dwPalCRC && (!loadFromTextureBuffer || gRenderTextureInfos[txtBufIdxToLoadFrom].updateAtFrame < pEntry->FrameLastUsed ))
				{
					// found it in the neighbourhood
					bChecksumDoMatch = true;
					break;
				}
		}
		// cannot find it 
		if(!bChecksumDoMatch){
			// try to load it
			pEntry = NULL;
		}
	} 


	if (pEntry)
	{
		if(pEntry->dwCRC == dwAsmCRC && pEntry->dwPalCRC == dwPalCRC &&
			(!loadFromTextureBuffer || gRenderTextureInfos[txtBufIdxToLoadFrom].updateAtFrame < pEntry->FrameLastUsed ) )
		{
			// Tile is ok, return
			pEntry->dwUses++;
			pEntry->dwTimeLastUsed = status.gRDPTime;
			pEntry->FrameLastUsed = status.gDlistCount;
			LOG_TEXTURE(TRACE0("   Use current texture:\n"));
			pEntry->lastEntry = g_lastTextureEntry;
			g_lastTextureEntry = pEntry;
			lastEntryModified = false;

			DEBUGGER_IF_DUMP((pauseAtNext && loadFromTextureBuffer) ,
			{DebuggerAppendMsg("Load cached texture from render_texture");}
			);

			return pEntry;
		}
		else
		{
			; //Do something
		}
	}

	if (pEntry == NULL)
	{
		// We need to create a new entry, and add it
		//  to the hash table.
		pEntry = CreateNewCacheEntry(pgti->Address, pgti->WidthToCreate, pgti->HeightToCreate);

		if (pEntry == NULL)
		{
			g_lastTextureEntry = pEntry;
			return NULL;
		}
	}

	pEntry->ti = *pgti;
	pEntry->dwCRC = dwAsmCRC;
	pEntry->dwPalCRC = dwPalCRC;
	pEntry->bExternalTxtrChecked = false;
	pEntry->maxCI = maxCI;

	try 
	{
		if (pEntry->pTexture != NULL)
		{

			if( pEntry->pTexture->m_dwCreatedTextureWidth < pgti->WidthToCreate )
				pEntry->ti.WidthToLoad = pEntry->pTexture->m_dwCreatedTextureWidth;

			if( pEntry->pTexture->m_dwCreatedTextureHeight < pgti->HeightToCreate )
				pEntry->ti.HeightToLoad = pEntry->pTexture->m_dwCreatedTextureHeight;

			SAFE_DELETE(pEntry->pEnhancedTexture);
			pEntry->dwEnhancementFlag = TEXTURE_NO_ENHANCEMENT;

			if( loadFromTextureBuffer )
			{
				g_pFrameBufferManager->LoadTextureFromRenderTexture(pEntry, txtBufIdxToLoadFrom);
				DEBUGGER_IF_DUMP((pauseAtNext && loadFromTextureBuffer) ,
				{DebuggerAppendMsg("Load texture from render_texture %d", txtBufIdxToLoadFrom);}
				);

				extern void ConvertTextureRGBAtoI(TxtrCacheEntry* pEntry, bool alpha);
				if( g_pRenderTextureInfo->CI_Info.dwFormat == TXT_FMT_I )
				{
					// Convert texture from RGBA to I
					ConvertTextureRGBAtoI(pEntry,false);
				}
				else if( g_pRenderTextureInfo->CI_Info.dwFormat == TXT_FMT_IA )
				{
					// Convert texture from RGBA to IA
					ConvertTextureRGBAtoI(pEntry,true);
				}
			}
			else
			{
				LOG_TEXTURE(TRACE0("   Load new texture from RDRAM:\n"));
				ConvertTexture(pEntry, fromTMEM);
				pEntry->FrameLastUpdated = status.gDlistCount;
				SAFE_DELETE(pEntry->pEnhancedTexture);
				pEntry->dwEnhancementFlag = TEXTURE_NO_ENHANCEMENT;
			}

			pEntry->ti.WidthToLoad = pgti->WidthToLoad;
			pEntry->ti.HeightToLoad = pgti->HeightToLoad;
			
			if( AutoExtendTexture )
			{
				ExpandTextureS(pEntry);
				ExpandTextureT(pEntry);
			}

			if( options.bDumpTexturesToFiles && !loadFromTextureBuffer )
			{
				DumpCachedTexture(*pEntry);
			}

#ifdef _DEBUG
			if( pauseAtNext && eventToPause == NEXT_NEW_TEXTURE )
			{
				CRender::g_pRender->SetCurrentTexture( 0, pEntry->pTexture, pEntry->ti.WidthToCreate, pEntry->ti.HeightToCreate, pEntry);
				CRender::g_pRender->DrawTexture(0);
				debuggerPause = true;
				TRACE0("Pause after loading a new texture");
				if( pEntry->ti.Format == TXT_FMT_YUV )
				{
					TRACE0("This is YUV texture");
				}
				DebuggerAppendMsg("W:%d, H:%d, RealW:%d, RealH:%d, D3DW:%d, D3DH: %d", pEntry->ti.WidthToCreate, pEntry->ti.HeightToCreate,
					pEntry->ti.WidthToLoad, pEntry->ti.HeightToLoad, pEntry->pTexture->m_dwCreatedTextureWidth, pEntry->pTexture->m_dwCreatedTextureHeight);
				DebuggerAppendMsg("CRC=%08X", pEntry->dwCRC);
				DebuggerPause();
				CRender::g_pRender->SetCurrentTexture( 0, NULL, 64, 64, NULL);
			}
#endif
		}
	}
	catch (...)
	{
		TRACE0("Exception in texture decompression");
		g_lastTextureEntry = NULL;
		return NULL;
	}

	pEntry->lastEntry = g_lastTextureEntry;
	g_lastTextureEntry = pEntry;
	lastEntryModified = true;
	return pEntry;
}




char *pszImgFormat[8] = {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};
uint8 pnImgSize[4]   = {4, 8, 16, 32};
char *textlutname[4] = {"RGB16", "I16?", "RGBA16", "IA16"};

extern uint16 g_wRDPTlut[];
extern ConvertFunction	gConvertFunctions_FullTMEM[ 8 ][ 4 ];
extern ConvertFunction	gConvertFunctions[ 8 ][ 4 ];
extern ConvertFunction	gConvertTlutFunctions[ 8 ][ 4 ];
void CTextureManager::ConvertTexture(TxtrCacheEntry * pEntry, bool fromTMEM)
{
	static uint32 dwCount = 0;
	
	ConvertFunction pF;
	if( fromTMEM && status.bAllowLoadFromTMEM )//backtomenoww
	{
		pF = gConvertFunctions_FullTMEM[ pEntry->ti.Format ][ pEntry->ti.Size ];
	}
	else
	{
		if( gRDP.tiles[7].dwFormat == TXT_FMT_YUV )
		{
			if( gRDP.otherMode.text_tlut>=2 )
				pF = gConvertTlutFunctions[ TXT_FMT_YUV ][ pEntry->ti.Size ];
			else
				pF = gConvertFunctions[ TXT_FMT_YUV ][ pEntry->ti.Size ];
		}
		else
		{
			if( gRDP.otherMode.text_tlut>=2 )
				pF = gConvertTlutFunctions[ pEntry->ti.Format ][ pEntry->ti.Size ];
			else
				pF = gConvertFunctions[ pEntry->ti.Format ][ pEntry->ti.Size ];
		}
	}

	if( pF )
	{
		pF( pEntry->pTexture, pEntry->ti );
	
		LOG_TEXTURE(
		{
			DebuggerAppendMsg("Decompress 32bit Texture:\n\tFormat: %s\n\tImage Size:%d\n", 
				pszImgFormat[pEntry->ti.Format], pnImgSize[pEntry->ti.Size]);
			DebuggerAppendMsg("Palette Format: %s (%d)\n", textlutname[pEntry->ti.TLutFmt>>RSP_SETOTHERMODE_SHIFT_TEXTLUT], pEntry->ti.TLutFmt>>RSP_SETOTHERMODE_SHIFT_TEXTLUT);
		});
	}
	else
	{
		TRACE2("ConvertTexture: Unable to decompress %s/%dbpp", pszImgFormat[pEntry->ti.Format], pnImgSize[pEntry->ti.Size]);
	}

	dwCount++;
}

void CTextureManager::ExpandTexture(TxtrCacheEntry * pEntry, uint32 sizeToLoad, uint32 sizeToCreate, uint32 sizeCreated,
	int arrayWidth, int flag, int mask, int mirror, int clamp, uint32 otherSize)
{
	if( sizeToLoad >= sizeCreated )	return;

	uint32 maskWidth = (1<<mask);

#ifdef _DEBUG
	// Some checks
	if( sizeToLoad > sizeToCreate || sizeToCreate > sizeCreated )	
		TRACE0("Something is wrong, check me here in ExpandTextureS");
#endif

	// Doing Mirror And/Or Wrap in S direction
	// Image has been loaded with width=WidthToLoad, we need to enlarge the image
	// to width = pEntry->ti.WidthToCreate by doing mirroring or wrapping

	DrawInfo di;
	if( !(pEntry->pTexture->StartUpdate(&di)) )
	{
		TRACE0("Cann't update the texture");
		return;
	}


	if( mask == 0 )
	{
		// Clamp
		Clamp((uint32*)di.lpSurface, sizeToLoad, sizeCreated, arrayWidth, otherSize, flag);
		pEntry->pTexture->EndUpdate(&di);
		return;
	}

#ifdef _DEBUG
	if( sizeToLoad > maskWidth )
	{
		TRACE0("Something is wrong, check me here in ExpandTextureS");
		pEntry->pTexture->EndUpdate(&di);
		return;
	}
	if( sizeToLoad == maskWidth && maskWidth == sizeToCreate && sizeToCreate != sizeCreated )
	{
		TRACE0("Something is wrong, check me here in ExpandTextureS");
		pEntry->pTexture->EndUpdate(&di);
		return;
	}
#endif

	if( sizeToLoad == maskWidth )
	{
		uint32 tempwidth = clamp ? sizeToCreate : sizeCreated;
		if( mirror )
			Mirror((uint32*)di.lpSurface, sizeToLoad, mask, tempwidth, arrayWidth, otherSize, flag);
		else
			Wrap((uint32*)di.lpSurface, sizeToLoad, mask, tempwidth, arrayWidth, otherSize, flag);

		if( tempwidth < sizeCreated )
			Clamp((uint32*)di.lpSurface, tempwidth, sizeCreated, arrayWidth, otherSize, flag);

		pEntry->pTexture->EndUpdate(&di);
		return;
	}


	if( sizeToLoad < sizeToCreate && sizeToCreate == maskWidth && maskWidth == sizeCreated )
	{
		// widthToLoad < widthToCreate = maskWidth
		Wrap((uint32*)di.lpSurface, sizeToLoad, mask, sizeCreated, arrayWidth, otherSize, flag);

		pEntry->pTexture->EndUpdate(&di);
		return;
	}

	if( sizeToLoad == sizeToCreate && sizeToCreate < maskWidth )
	{
#ifdef _DEBUG
		if( maskWidth < sizeToCreate )	TRACE0("Incorrect condition, check me");
#endif
		Clamp((uint32*)di.lpSurface, sizeToLoad, sizeCreated, arrayWidth, otherSize, flag);

		pEntry->pTexture->EndUpdate(&di);
		return;
	}

	if( sizeToLoad < sizeToCreate && sizeToCreate < maskWidth )
	{
#ifdef _DEBUG
		if( clamp )	TRACE0("Incorrect condition, check me");
		if( maskWidth < sizeCreated )	TRACE0("Incorrect condition, check me");
#endif
		Clamp((uint32*)di.lpSurface, sizeToLoad, sizeCreated, arrayWidth, otherSize, flag);
		pEntry->pTexture->EndUpdate(&di);
		return;
	}

	TRACE0("Check me, should not get here");
	pEntry->pTexture->EndUpdate(&di);
}

void CTextureManager::ExpandTextureS(TxtrCacheEntry * pEntry)
{
	TxtrInfo &ti =  pEntry->ti;
	uint32 textureWidth = pEntry->pTexture->m_dwCreatedTextureWidth;
	ExpandTexture(pEntry, ti.WidthToLoad, ti.WidthToCreate, textureWidth, textureWidth, S_FLAG, ti.maskS, ti.mirrorS, ti.clampS, ti.HeightToLoad);
}

void CTextureManager::ExpandTextureT(TxtrCacheEntry * pEntry)
{
	TxtrInfo &ti =  pEntry->ti;
	uint32 textureHeight = pEntry->pTexture->m_dwCreatedTextureHeight;
	uint32 textureWidth = pEntry->pTexture->m_dwCreatedTextureWidth;
	ExpandTexture(pEntry, ti.HeightToLoad, ti.HeightToCreate, textureHeight, textureWidth, T_FLAG, ti.maskT, ti.mirrorT, ti.clampT, ti.WidthToLoad);
}

void CTextureManager::Clamp(uint32 *array, uint32 size, uint32 tosize, uint32 arraySize, uint32 rows, int flag)
{
	if( flag == S_FLAG )	// s
	{
		if ((int) size <= 0 || (int) tosize < 0)
			return;

		for( uint32 y = 0; y<rows; y++ )
		{
			uint32* line = array+y*arraySize;
			uint32 val = line[size-1];
			for( uint32 x=size; x<tosize; x++ )
			{
				line[x] = val;
			}
		}
	}
	else	// t
	{
		if ((int) size <= 0 || (int) tosize < 0)
			return;

		uint32* linesrc = array+arraySize*(size-1);
		for( uint32 y = size; y<tosize; y++ )
		{
			uint32* linedst = array+arraySize*y;
			for( uint32 x=0; x<arraySize; x++ )
			{
				linedst[x] = linesrc[x];
			}
		}
	}
}

void CTextureManager::Wrap(uint32 *array, uint32 size, uint32 mask, uint32 tosize, uint32 arraySize, uint32 rows, int flag)
{
	uint32 maskval = (1<<mask)-1;

	if( flag == S_FLAG )	// s
	{
		for( uint32 y = 0; y<rows; y++ )
		{
			uint32* line = array+y*arraySize;
			for( uint32 x=size; x<tosize; x++ )
			{
				line[x] = line[(x&maskval)<size?(x&maskval):tosize-(x&maskval)];
			}
		}
	}
	else	// t
	{
		for( uint32 y = size; y<tosize; y++ )
		{
			uint32* linesrc = array+arraySize*(y>maskval?y&maskval:y-size);
			uint32* linedst = array+arraySize*y;
			for( uint32 x=0; x<arraySize; x++ )
			{
				linedst[x] = linesrc[x];
			}
		}
	}
}

void CTextureManager::Mirror(uint32 *array, uint32 size, uint32 mask, uint32 tosize, uint32 arraySize, uint32 rows, int flag)
{
	uint32 maskval1 = (1<<mask)-1;
	uint32 maskval2 = (1<<(mask+1))-1;
	if( flag == S_FLAG )	// s
	{
		for( uint32 y = 0; y<rows; y++ )
		{
			uint32* line = array+y*arraySize;
			// mirror the current row to the destination width
			for( uint32 x=size; x<tosize; x++ )
			{
				line[x] = (x&maskval2)<=maskval1 ? line[x&maskval1] : line[maskval2-(x&maskval2)];
			}
		}
	}
	else	// t
	{
		for( uint32 y = size; y<tosize; y++ )
		{
			uint32 srcy = (y&maskval2)<=maskval1 ? y&maskval1 : maskval2-(y&maskval2);
			uint32* linesrc = array+arraySize*srcy;
			uint32* linedst = array+arraySize*y;
			for( uint32 x=0; x<arraySize; x++ )
			{
				linedst[x] = linesrc[x];
			}
		}
	}

}


#ifdef _DEBUG
TxtrCacheEntry * CTextureManager::GetCachedTexture(uint32 tex)
{
	uint32 size = 0;
	for( uint32 i=0; i<m_numOfCachedTxtrList; i++ )
	{
		if( m_pCacheTxtrList[i] == NULL )
			continue;
		else
		{
			TxtrCacheEntry *pEntry;

			for (pEntry = m_pCacheTxtrList[i]; pEntry; pEntry = pEntry->pNext)
			{
				if( size == tex )
					return pEntry;
				else
					size++;
			}
		}
	}
	return NULL;
}
uint32 CTextureManager::GetNumOfCachedTexture()
{
	uint32 size = 0;
	for( uint32 i=0; i<m_numOfCachedTxtrList; i++ )
	{
		if( m_pCacheTxtrList[i] == NULL )
			continue;
		else
		{
			TxtrCacheEntry *pEntry;

			for (pEntry = m_pCacheTxtrList[i]; pEntry; pEntry = pEntry->pNext)
			{
				size++;
			}
		}
	}
	TRACE1("Totally %d texture cached", size);
	return size;
}
#endif


TxtrCacheEntry * CTextureManager::GetBlackTexture(void)
{
	if( m_blackTextureEntry.pTexture == NULL )
	{
		m_blackTextureEntry.pTexture = new CTexture(4, 4);
		m_blackTextureEntry.ti.WidthToCreate = 4;
		m_blackTextureEntry.ti.HeightToCreate = 4;
		updateColorTexture(m_blackTextureEntry.pTexture,0x00000000);
	}
	return &m_blackTextureEntry;
}
TxtrCacheEntry * CTextureManager::GetPrimColorTexture(uint32 color)
{
	static uint32 mcolor = 0;
	if( m_PrimColorTextureEntry.pTexture == NULL )
	{
		m_PrimColorTextureEntry.pTexture = new CTexture(4, 4);
		m_PrimColorTextureEntry.ti.WidthToCreate = 4;
		m_PrimColorTextureEntry.ti.HeightToCreate = 4;
		updateColorTexture(m_PrimColorTextureEntry.pTexture,color);
		gRDP.texturesAreReloaded = true;
	}
	else if( mcolor != color )
	{
		updateColorTexture(m_PrimColorTextureEntry.pTexture,color);
		gRDP.texturesAreReloaded = true;
	}

	mcolor = color;
	return &m_PrimColorTextureEntry;
}
TxtrCacheEntry * CTextureManager::GetEnvColorTexture(uint32 color)
{
	static uint32 mcolor = 0;
	if( m_EnvColorTextureEntry.pTexture == NULL )
	{
		m_EnvColorTextureEntry.pTexture = new CTexture(4, 4);
		m_EnvColorTextureEntry.ti.WidthToCreate = 4;
		m_EnvColorTextureEntry.ti.HeightToCreate = 4;
		gRDP.texturesAreReloaded = true;

		updateColorTexture(m_EnvColorTextureEntry.pTexture,color);
	}
	else if( mcolor != color )
	{
		updateColorTexture(m_EnvColorTextureEntry.pTexture,color);
		gRDP.texturesAreReloaded = true;
	}

	mcolor = color;
	return &m_EnvColorTextureEntry;
}
TxtrCacheEntry * CTextureManager::GetLODFracTexture(uint8 fac)
{
	static uint8 mfac = 0;
	if( m_LODFracTextureEntry.pTexture == NULL )
	{
		m_LODFracTextureEntry.pTexture = new CTexture(4, 4);
		m_LODFracTextureEntry.ti.WidthToCreate = 4;
		m_LODFracTextureEntry.ti.HeightToCreate = 4;
		uint32 factor = fac;
		uint32 color = fac;
		color |= factor << 8;
		color |= color << 16;
		updateColorTexture(m_LODFracTextureEntry.pTexture,color);
		gRDP.texturesAreReloaded = true;
	}
	else if( mfac != fac )
	{
		uint32 factor = fac;
		uint32 color = fac;
		color |= factor << 8;
		color |= color << 16;
		updateColorTexture(m_LODFracTextureEntry.pTexture,color);
		gRDP.texturesAreReloaded = true;
	}

	mfac = fac;
	return &m_LODFracTextureEntry;
}

TxtrCacheEntry * CTextureManager::GetPrimLODFracTexture(uint8 fac)
{
	static uint8 mfac = 0;
	if( m_PrimLODFracTextureEntry.pTexture == NULL )
	{
		m_PrimLODFracTextureEntry.pTexture = new CTexture(4, 4);
		m_PrimLODFracTextureEntry.ti.WidthToCreate = 4;
		m_PrimLODFracTextureEntry.ti.HeightToCreate = 4;
		uint32 factor = fac;
		uint32 color = fac;
		color |= factor << 8;
		color |= color << 16;
		updateColorTexture(m_PrimLODFracTextureEntry.pTexture,color);
		gRDP.texturesAreReloaded = true;
	}
	else if( mfac != fac )
	{
		uint32 factor = fac;
		uint32 color = fac;
		color |= factor << 8;
		color |= color << 16;
		updateColorTexture(m_PrimLODFracTextureEntry.pTexture,color);
		gRDP.texturesAreReloaded = true;
	}

	mfac = fac;
	return &m_PrimLODFracTextureEntry;
}

TxtrCacheEntry * CTextureManager::GetConstantColorTexture(uint32 constant)
{
	switch( constant )
	{
	case MUX_PRIM:
		return GetPrimColorTexture(gRDP.primitiveColor);
		break;
	case MUX_ENV:
		return GetEnvColorTexture(gRDP.envColor);
		break;
	case MUX_LODFRAC:
		return GetLODFracTexture((uint8)gRDP.LODFrac);
		break;
	default:	// MUX_PRIMLODFRAC
		return GetPrimLODFracTexture((uint8)gRDP.primLODFrac);
		break;
	}
}

void CTextureManager::updateColorTexture(CTexture *ptexture, uint32 color)
{
	DrawInfo di;
	if( !(ptexture->StartUpdate(&di)) )
	{
		TRACE0("Cann't update the texture");
		return;
	}

	uint32 *buf = (uint32*)di.lpSurface;
	for( int i=0; i<16; i++ )
	{
		buf[i] = color;
	}

	ptexture->EndUpdate(&di);
}

void ConvertTextureRGBAtoI(TxtrCacheEntry* pEntry, bool alpha)
{
	DrawInfo srcInfo;	
	if( pEntry->pTexture->StartUpdate(&srcInfo) )
	{
		uint32 *buf;
		uint32 val;
		uint32 r,g,b,a,i;

		for(int nY = 0; nY < srcInfo.dwCreatedHeight; nY++)
		{
			buf = (uint32*)((uint8*)srcInfo.lpSurface+nY*srcInfo.lPitch);
			for(int nX = 0; nX < srcInfo.dwCreatedWidth; nX++)
			{
				val = buf[nX];
				b = (val>>0)&0xFF;
				g = (val>>8)&0xFF;
				r = (val>>16)&0xFF;
				i = (r+g+b)/3;
				a = alpha?(val&0xFF000000):(i<<24);
				buf[nX] = (a|(i<<16)|(i<<8)|i);
			}
		}
		pEntry->pTexture->EndUpdate(&srcInfo);	}
}
