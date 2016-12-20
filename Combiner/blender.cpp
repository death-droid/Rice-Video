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

#ifdef _DEBUG
static const char * const kBlendCl[] = { "In", "Mem", "Bl", "Fog" };
static const char * const kBlendA1[] = { "AIn", "AFog", "AShade", "0" };
static const char * const kBlendA2[] = { "1-A", "AMem", "1", "?" };

static inline void DebugBlender(u32 cycle_type, u32 blender, u32 alpha_cvg_sel, u32 cvg_x_alpha)
{
	static u32 last_blender = 0;

	if (last_blender != blender)
	{
		TRACE0("********************************\n\n");
		TRACE2("Unknown Blender. alpha_cvg_sel: %d cvg_x_alpha: %d\n", alpha_cvg_sel, cvg_x_alpha);
		TRACE5("0x%04x: // %s * %s + %s * %s",
			blender,
			kBlendCl[(blender >> 14) & 0x3],
			kBlendA1[(blender >> 10) & 0x3],
			kBlendCl[(blender >> 6) & 0x3],
			kBlendA2[(blender >> 2) & 0x3]);

        if (cycle_type == CYCLE_TYPE_2)
		{
			TRACE4(" | %s * %s + %s * %s",
				kBlendCl[(blender >> 12) & 0x3],
				kBlendA1[(blender >> 8) & 0x3],
				kBlendCl[(blender >> 4) & 0x3],
				kBlendA2[(blender)& 0x3]);
		}
		TRACE0("\n********************************\n\n");
		last_blender = blender;
	}
}
#endif

//========================================================================
void CBlender::InitBlenderMode(void)					// Set Alpha Blender mode
{
/*Possible Blending Inputs:

    In  -   Input from color combiner
    Mem -   Input from current frame buffer
    Fog -   Fog generator
    BL  -   Blender

Possible Blending Factors:
    A-IN    -   Alpha from color combiner
    A-MEM   -   Alpha from current frame buffer
    (1-A)   -   
    A-FOG   -   Alpha of fog color
    A-SHADE -   Alpha of shade
    1   -   1
    0   -   0
*/

	uint32 blendmode = gRDP.otherMode.blender;
	uint32 cycle_type     = gRDP.otherMode.cycle_type;
	uint32 cvg_x_alpha   = gRDP.otherMode.cvg_x_alpha;
	uint32 alpha_cvg_sel = gRDP.otherMode.alpha_cvg_sel;
	
	uint32 active_mode = (cycle_type == CYCLE_TYPE_2) ? blendmode : (blendmode & 0xcccc);

	enum BlendType
	{
		kBlendModeOpaque,
		kBlendModeAlphaTrans,
		kBlendModeFade,
		kBlendModeOne,
		kBlendModeZeroOne,
		kBlendModeAlphaTransInvSrc,
		kBlendModeOneSrc
	};

	BlendType type = kBlendModeOpaque;

	// FIXME(strmnnrmn): lots of these need fog! (death-droid CHECKME)
	switch (active_mode)
	{
	case 0x0040: // In * AIn + Mem * 1-A
		// MarioKart (spinning logo).
		type = kBlendModeAlphaTrans;
		break;
	case 0x0050: // In * AIn + Mem * 1-A | In * AIn + Mem * 1-A
		// Extreme-G.
		type = kBlendModeAlphaTrans;
		break;
	case 0x0440: // In * AFog + Mem * 1-A
		// Bomberman64. alpha_cvg_sel: 1 cvg_x_alpha: 1
		type = kBlendModeAlphaTrans;
		break;
	case 0x04d0: // In * AFog + Fog * 1-A | In * AIn + Mem * 1-A
		// Conker.
		type = kBlendModeAlphaTrans;
		break;
	case 0x0150: // In * AIn + Mem * 1-A | In * AFog + Mem * 1-A
		// Spiderman.
		type = kBlendModeAlphaTrans;
		break;
	case 0x0c08: // In * 0 + In * 1
		// MarioKart (spinning logo)
		// This blend mode doesn't use the alpha value
		type = kBlendModeOpaque;
		break;
	case 0x0c18: // In * 0 + In * 1 | In * AIn + Mem * 1-A
		// StarFox main menu.
		type = kBlendModeAlphaTrans;
		break;
	case 0x0c40: // In * 0 + Mem * 1-A
		// Extreme-G.
		type = kBlendModeFade;
		break;
	case 0x0c48: // In * 0 + Mem * 1
		type = kBlendModeZeroOne;
		break;
	case 0x0f0a: // In * 0 + In * 1 | In * 0 + In * 1
		// Zelda OoT.
		type = kBlendModeOpaque;
		break;
	case 0x4c40: // Mem * 0 + Mem * 1-A
		//Waverace - alpha_cvg_sel: 0 cvg_x_alpha: 1
		type = kBlendModeFade;
		break;
	case 0x8410: // Bl * AFog + In * 1-A | In * AIn + Mem * 1-A
		// Paper Mario.
		type = kBlendModeAlphaTrans;
		break;
	case 0xc410: // Fog * AFog + In * 1-A | In * AIn + Mem * 1-A
		// Donald Duck (Dust)
		type = kBlendModeAlphaTrans;
		break;
	case 0xc440: // Fog * AFog + Mem * 1-A
		// Banjo Kazooie
		// Banjo Tooie sun glare
		// FIXME: blends fog over existing?
		type = kBlendModeAlphaTrans;
		break;
	case 0xc800: // Fog * AShade + In * 1-A
		//Bomberman64. alpha_cvg_sel: 0 cvg_x_alpha: 1
		type = kBlendModeOpaque;
		break;
	case 0xc810: // Fog * AShade + In * 1-A | In * AIn + Mem * 1-A
		// AeroGauge (ingame)
		type = kBlendModeAlphaTrans;
		break;
	// case 0x0321: // In * 0 + Bl * AMem
	// 	// Hmm - not sure about what this is doing. Zelda OoT pause screen.
	// 	type = kBlendModeAlphaTrans;
	// 	break;

	default:
#ifdef _DEBUG
		DebugBlender(cycle_type, active_mode, alpha_cvg_sel, cvg_x_alpha);
		TRACE1("		 Blend: SRCALPHA/INVSRCALPHA (default: 0x%04x)", active_mode);
#endif
		break;
	}

	//We only have alpha in the blender if alpha_cvg_selc is 0 or cvg_x_alpha is 1
	bool have_alpha = !alpha_cvg_sel || cvg_x_alpha;

	if (type == kBlendModeAlphaTrans && !have_alpha)
		type = kBlendModeOpaque;

	switch (type)
	{
	case kBlendModeOpaque:
		gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		break;
	case kBlendModeAlphaTrans:
		gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
		gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		break;
	case kBlendModeFade:
		gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_ZERO);
		gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		break;
	case kBlendModeOne:
		gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		break;
	case kBlendModeZeroOne:
		gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		break;
	case kBlendModeAlphaTransInvSrc:
		gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_INVSRCALPHA);
		gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);
		gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		break;
	case kBlendModeOneSrc:
		gD3DDevWrapper.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		gD3DDevWrapper.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);
		gD3DDevWrapper.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		break;
	}
}
