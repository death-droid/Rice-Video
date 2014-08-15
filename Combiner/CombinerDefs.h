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

#ifndef _COMBINER_DEFS_H_
#define _COMBINER_DEFS_H_

#define MUX_MASK			0x1F
#define MUX_MASK_WITH_ALPHA	0x5F
#define MUX_MASK_WITH_NEG	0x3F
#define MUX_MASK_WITH_COMP	0x9F
enum
{
	MUX_0 = 0,
	MUX_1,
	MUX_COMBINED,
	MUX_TEXEL0,
	MUX_TEXEL1,
	MUX_PRIM,
	MUX_SHADE,
	MUX_ENV,
	MUX_COMBALPHA,
	MUX_T0_ALPHA,
	MUX_T1_ALPHA,
	MUX_PRIM_ALPHA,
	MUX_SHADE_ALPHA,
	MUX_ENV_ALPHA,
	MUX_LODFRAC,
	MUX_PRIMLODFRAC,
	MUX_K5,
	MUX_UNK,			//Use this if you want to factor to be set to 0

	// Don't change value of these three flags, then need to be within 1 uint8
	MUX_NEG				= 0x20,	//Support by NVidia register combiner
	MUX_ALPHAREPLICATE = 0x40,
	MUX_COMPLEMENT = 0x80,
	MUX_FORCE_0 = 0xFE,
	MUX_ERR = 0xFF,
};

typedef enum {
	COLOR_CHANNEL,
	ALPHA_CHANNEL,
} CombineChannel;

typedef struct {
	uint8 a;
	uint8 b;
	uint8 c;
	uint8 d;
} N64CombinerType;

#define G_CCMUX_TEXEL1		2
#define G_ACMUX_TEXEL1		2

#define D3DTA_IGNORE ~0
#define NOTUSED MUX_0

enum { TEX_0=0, TEX_1=1};

typedef struct {
	uint32 op;
	uint32 Arg1;
	uint32 Arg2;
	uint32 Arg0;
} StageOperate;

#endif



