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

#include "stdafx.h"

/************************************************************************/
/* 2X filter                                                            */
/************************************************************************/

#define DWORD_MAKE(r, g, b, a)   ((uint32) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define WORD_MAKE(r, g, b, a)   ((uint16) (((a) << 12) | ((r) << 8) | ((g) << 4) | (b)))

// Basic 2x R8G8B8A8 filter with interpolation

void Texture2x_32(uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height)
{
  uint32 *pDst1, *pDst2;
  uint32 *pSrc, *pSrc2;
  uint32 nWidth = width;
  uint32 nHeight = height;

  uint32 b1;
  uint32 g1;
  uint32 r1;
  uint32 a1;
  uint32 b2;
  uint32 g2;
  uint32 r2;
  uint32 a2;
  uint32 b3;
  uint32 g3;
  uint32 r3;
  uint32 a3;
  uint32 b4;
  uint32 g4;
  uint32 r4;
  uint32 a4;

  uint32 xSrc;
  uint32 ySrc;

  for (ySrc = 0; ySrc < nHeight; ySrc++)
  {
    pSrc = (uint32*)(((uint8*)srcPtr)+ySrc*srcPitch);
    pSrc2 = (uint32*)(((uint8*)srcPtr)+(ySrc+1)*srcPitch);
    pDst1 = (uint32*)(((uint8*)dstPtr)+(ySrc*2)*dstPitch);
    pDst2 = (uint32*)(((uint8*)dstPtr)+(ySrc*2+1)*dstPitch);

    for (xSrc = 0; xSrc < nWidth; xSrc++)
    {
      b1 = (pSrc[xSrc]>>0)&0xFF;
      g1 = (pSrc[xSrc]>>8)&0xFF;
      r1 = (pSrc[xSrc]>>16)&0xFF;
      a1 = (pSrc[xSrc]>>24)&0xFF;

      if( xSrc<nWidth-1 )
      {
        b2 = (pSrc[xSrc+1]>>0)&0xFF;
        g2 = (pSrc[xSrc+1]>>8)&0xFF;
        r2 = (pSrc[xSrc+1]>>16)&0xFF;
        a2 = (pSrc[xSrc+1]>>24)&0xFF;
      }

      if( ySrc<nHeight-1 )
      {
        b3 = (pSrc2[xSrc]>>0)&0xFF;
        g3 = (pSrc2[xSrc]>>8)&0xFF;
        r3 = (pSrc2[xSrc]>>16)&0xFF;
        a3 = (pSrc2[xSrc]>>24)&0xFF;
        if( xSrc<nWidth-1 )
        {
          b4 = (pSrc2[xSrc+1]>>0)&0xFF;
          g4 = (pSrc2[xSrc+1]>>8)&0xFF;
          r4 = (pSrc2[xSrc+1]>>16)&0xFF;
          a4 = (pSrc2[xSrc+1]>>24)&0xFF;
        }
      }

#include "TextureFilters_texture2x.h"
    }
  }
}


// Basic 2x R4G4B4A4 filter with interpolation
void Texture2x_16(uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height)
{
  uint16 *pDst1, *pDst2;
  uint16 *pSrc, *pSrc2;
  uint32 nWidth = width;
  uint32 nHeight = height;

  uint16 b1;
  uint16 g1;
  uint16 r1;
  uint16 a1;
  uint16 b2;
  uint16 g2;
  uint16 r2;
  uint16 a2;
  uint16 b3;
  uint16 g3;
  uint16 r3;
  uint16 a3;
  uint16 b4;
  uint16 g4;
  uint16 r4;
  uint16 a4;

  uint16 xSrc;
  uint16 ySrc;

  for (ySrc = 0; ySrc < nHeight; ySrc++)
  {
    pSrc = (uint16*)(((uint8*)srcPtr)+ySrc*srcPitch);
    pSrc2 = (uint16*)(((uint8*)srcPtr)+(ySrc+1)*srcPitch);
    pDst1 = (uint16*)(((uint8*)dstPtr)+(ySrc*2)*dstPitch);
    pDst2 = (uint16*)(((uint8*)dstPtr)+(ySrc*2+1)*dstPitch);

    for (xSrc = 0; xSrc < nWidth; xSrc++)
    {
      b1 = (pSrc[xSrc]>> 0)&0xF;
      g1 = (pSrc[xSrc]>> 4)&0xF;
      r1 = (pSrc[xSrc]>> 8)&0xF;
      a1 = (pSrc[xSrc]>>12)&0xF;

      if( xSrc<nWidth-1 )
      {
        b2 = (pSrc[xSrc+1]>> 0)&0xF;
        g2 = (pSrc[xSrc+1]>> 4)&0xF;
        r2 = (pSrc[xSrc+1]>> 8)&0xF;
        a2 = (pSrc[xSrc+1]>>12)&0xF;
      }

      if( ySrc<nHeight-1 )
      {
        b3 = (pSrc2[xSrc]>> 0)&0xF;
        g3 = (pSrc2[xSrc]>> 4)&0xF;
        r3 = (pSrc2[xSrc]>> 8)&0xF;
        a3 = (pSrc2[xSrc]>>12)&0xF;
        if( xSrc<nWidth-1 )
        {
          b4 = (pSrc2[xSrc+1]>> 0)&0xF;
          g4 = (pSrc2[xSrc+1]>> 4)&0xF;
          r4 = (pSrc2[xSrc+1]>> 8)&0xF;
          a4 = (pSrc2[xSrc+1]>>12)&0xF;
        }
      }

#include "TextureFilters_texture2x.h"
    }
  }
}

void Texture2x(uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height, bool bPixelSize4)
{
	if(bPixelSize4)
		Texture2x_32((uint8*)(srcPtr), srcPitch, (uint8*)(dstPtr), dstPitch, width, height);
	else
		Texture2x_16((uint8*)(srcPtr), srcPitch, (uint8*)(dstPtr), dstPitch, width, height);
}