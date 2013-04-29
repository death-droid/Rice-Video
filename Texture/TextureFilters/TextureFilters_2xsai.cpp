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
#include "TextureFilters.h"


/************************************************************************/
/* 2xSAI filters                                                        */
/************************************************************************/
static __inline int SAI_GetResult1_32( uint32 A, uint32 B, uint32 C, uint32 D, uint32 E )
{
	int x = 0;
	int y = 0;
	int r = 0;

	if (A == C) x += 1; else if (B == C) y += 1;
	if (A == D) x += 1; else if (B == D) y += 1;
	if (x <= 1) r += 1; 
	if (y <= 1) r -= 1;

	return r;
}

static __inline int SAI_GetResult2_32( uint32 A, uint32 B, uint32 C, uint32 D, uint32 E) 
{
	int x = 0; 
	int y = 0;
	int r = 0;

	if (A == C) x += 1; else if (B == C) y += 1;
	if (A == D) x += 1; else if (B == D) y += 1;
	if (x <= 1) r -= 1; 
	if (y <= 1) r += 1;

	return r;
}

static __inline int SAI_GetResult_32( uint32 A, uint32 B, uint32 C, uint32 D )
{
	int x = 0; 
	int y = 0;
	int r = 0;

	if (A == C) x += 1; else if (B == C) y += 1;
	if (A == D) x += 1; else if (B == D) y += 1;
	if (x <= 1) r += 1; 
	if (y <= 1) r -= 1;

	return r;
}

static __inline uint32 SAI_INTERPOLATE_32( uint32 A, uint32 B)
{
	if (A != B)
		return	((A & 0xFEFEFEFE) >> 1) + 
		((B & 0xFEFEFEFE) >> 1) |
		(A & B & 0x01010101);
	else
		return A;
}


static __inline uint32 SAI_Q_INTERPOLATE_32( uint32 A, uint32 B, uint32 C, uint32 D)
{
	uint32 x =	((A & 0xFCFCFCFC) >> 2) +
		((B & 0xFCFCFCFC) >> 2) +
		((C & 0xFCFCFCFC) >> 2) +
		((D & 0xFCFCFCFC) >> 2);
	uint32 y =	(((A & 0x03030303) +
		(B & 0x03030303) +
		(C & 0x03030303) +
		(D & 0x03030303)) >> 2) & 0x03030303;
	return x | y;
}

void Super2xSaI( uint32 *srcPtr, uint32 *destPtr, uint32 width, uint32 height, uint32 pitch)
{
	uint32 destWidth = width << 1;
	uint32 destHeight = height << 1;

	uint32 color4, color5, color6;
	uint32 color1, color2, color3;
	uint32 colorA0, colorA1, colorA2, colorA3;
	uint32 colorB0, colorB1, colorB2, colorB3;
	uint32 colorS1, colorS2;
	uint32 product1a, product1b, product2a, product2b;

#include"TextureFilters_2xsai.h"
}
