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

#include "stdafx.h"


//Attention
// If using CUR as an argument, use it as Arg2, not Arg1. I don't know why, 
// Geforce2 seems to be picky about this

// LERP and MULTIPLYADD are actually implemented in 2 stages in video chip
// they can only be used with SEL() before it, or use 1 stage only

// SEL(SPE) only is not good for alpha channel
// In fact, Specular color does not have alpha channel

// Modifier D3DTA_COMPLEMENT does not work
// ADDSMOOTH does not work

// When using MOD with TEX and other, TEX must be the first argument, Arg1
// When MOD the DIF and FAC, using MOD(FAC,DIF) instead of MOD(DIF,FAC)

// Don't MOD(TEX,DIF) at Alpha channel, I don't know why this does not work
// probably there is not alpha blending for DIFFUSE at alpha channel

// Modifier COMPLEMENT and ALPHAREPLICATE only works as the first argument of the MOD operate
// Modifier ALPHAREPLICATE works
// Combined modifier of COMPLEMENT and ALPHAREPLICATE also works

#define MUX_T0	MUX_TEXEL0
#define MUX_T1	MUX_TEXEL1
#define MUX_DIF	MUX_SHADE
#define MUX_COM	MUX_COMBINED
#define MUX_CUR	MUX_COMBINED
#define MUX_PRI	MUX_PRIM

#define MUX_T0A		(MUX_TEXEL0|MUX_ALPHAREPLICATE)
#define MUX_T1A		(MUX_TEXEL1|MUX_ALPHAREPLICATE)
#define MUX_DIFA	(MUX_SHADE|MUX_ALPHAREPLICATE)
#define MUX_COMA	(MUX_COMBINED|MUX_ALPHAREPLICATE)
#define MUX_CURA	(MUX_COMBINED|MUX_ALPHAREPLICATE)
#define MUX_PRIA	(MUX_PRIM|MUX_ALPHAREPLICATE)
#define MUX_ENVA	(MUX_ENV|MUX_ALPHAREPLICATE)

#define MUX_T0C		(MUX_TEXEL0|MUX_COMPLEMENT)
#define MUX_T1C		(MUX_TEXEL1|MUX_COMPLEMENT)
#define MUX_DIFC	(MUX_SHADE|MUX_COMPLEMENT)
#define MUX_COMC	(MUX_COMBINED|MUX_COMPLEMENT)
#define MUX_CURC	(MUX_COMBINED|MUX_COMPLEMENT)
#define MUX_PRIC	(MUX_PRIM|MUX_COMPLEMENT)
#define MUX_ENVC	(MUX_ENV|MUX_COMPLEMENT)

#define MUX_T0AC	(MUX_TEXEL0|MUX_COMPLEMENT|MUX_ALPHAREPLICATE)
#define MUX_T1AC	(MUX_TEXEL1|MUX_COMPLEMENT|MUX_ALPHAREPLICATE)
#define MUX_DIFAC	(MUX_SHADE|MUX_COMPLEMENT|MUX_ALPHAREPLICATE)
#define MUX_COMAC	(MUX_COMBINED|MUX_COMPLEMENT|MUX_ALPHAREPLICATE)
#define MUX_CURAC	(MUX_COMBINED|MUX_COMPLEMENT|MUX_ALPHAREPLICATE)
#define MUX_PRIAC	(MUX_PRIM|MUX_COMPLEMENT|MUX_ALPHAREPLICATE)
#define MUX_ENVAC	(MUX_ENV|MUX_COMPLEMENT|MUX_ALPHAREPLICATE)

#define ONEARGS(op, arg1)	{CM_##op, MUX_##arg1}
#define TWOARGS(op, arg1,arg2)	{CM_##op, MUX_##arg1, MUX_##arg2}
#define TRIARGS(op, arg1,arg2,arg3)	{CM_##op, MUX_##arg1, MUX_##arg2, MUX_##arg3}
#define SEL(arg1)		ONEARGS(REPLACE,arg1)
#define MOD(arg1,arg2)	TWOARGS(MODULATE,arg1,arg2)
#define ADD(arg1,arg2)	TWOARGS(ADD,arg1,arg2)
#define SUB(arg1,arg2)	TWOARGS(SUBTRACT,arg1,arg2)
#define ADDSMOOTH(arg1,arg2)	TWOARGS(ADDSMOOTH,arg1,arg2)
#define LERP(arg1,arg2,arg3)	TRIARGS(INTERPOLATE,arg1,arg2,arg3)
#define MULADD(arg1,arg2,arg3)	TRIARGS(MULTIPLYADD,arg1,arg2,arg3)
#define SKIP	SEL(CUR)