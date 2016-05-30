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


#ifndef __RICE_RDP_GFX_H__
#define __RICE_RDP_GFX_H__
#include "ucodedefs.h"

#define	RSP_SPNOOP				0	// handle 0 gracefully 
#define	RSP_MTX					1
#define RSP_RESERVED0			2	// unknown 
#define RSP_MOVEMEM				3	// move a block of memory (up to 4 words) to dmem 
#define	RSP_VTX					4
#define RSP_RESERVED1			5	// unknown 
#define	RSP_DL					6
#define RSP_RESERVED2			7	// unknown 
#define RSP_RESERVED3			8	// unknown 
#define RSP_SPRITE2D   			9	// sprite command 
#define RSP_SPRITE2D_BASE		9	// sprite command


#define	RSP_1ST					0xBF
#define	RSP_TRI1				(RSP_1ST-0)
#define RSP_CULLDL				(RSP_1ST-1)
#define	RSP_POPMTX				(RSP_1ST-2)
#define	RSP_MOVEWORD			(RSP_1ST-3)
#define	RSP_TEXTURE				(RSP_1ST-4)
#define	RSP_SETOTHERMODE_H		(RSP_1ST-5)
#define	RSP_SETOTHERMODE_L		(RSP_1ST-6)
#define RSP_ENDDL				(RSP_1ST-7)
#define RSP_SETGEOMETRYMODE		(RSP_1ST-8)
#define RSP_CLEARGEOMETRYMODE	(RSP_1ST-9)
#define RSP_LINE3D				(RSP_1ST-10)
#define RSP_RDPHALF_1			(RSP_1ST-11)
#define RSP_RDPHALF_2			(RSP_1ST-12)
#define RSP_RDPHALF_CONT		(RSP_1ST-13)

#define RSP_MODIFYVTX			(RSP_1ST-13)
#define RSP_TRI2				(RSP_1ST-14)
#define RSP_BRANCH_Z			(RSP_1ST-15)
#define RSP_LOAD_UCODE			(RSP_1ST-16)

#define RSP_SPRITE2D_SCALEFLIP    (RSP_1ST-1)
#define RSP_SPRITE2D_DRAW         (RSP_1ST-2)

#define RSP_ZELDAVTX				1
#define RSP_ZELDAMODIFYVTX			2
#define RSP_ZELDACULLDL				3
#define RSP_ZELDABRANCHZ			4
#define RSP_ZELDATRI1				5
#define RSP_ZELDATRI2				6
#define RSP_ZELDALINE3D				7
#define	RSP_ZELDARDPHALF_2			0xf1
#define	RSP_ZELDASETOTHERMODE_H		0xe3
#define	RSP_ZELDASETOTHERMODE_L		0xe2
#define	RSP_ZELDARDPHALF_1			0xe1
#define	RSP_ZELDASPNOOP				0xe0
#define	RSP_ZELDAENDDL				0xdf
#define	RSP_ZELDADL					0xde
#define	RSP_ZELDALOAD_UCODE			0xdd
#define	RSP_ZELDAMOVEMEM			0xdc
#define	RSP_ZELDAMOVEWORD			0xdb
#define	RSP_ZELDAMTX				0xda
#define RSP_ZELDAGEOMETRYMODE		0xd9
#define	RSP_ZELDAPOPMTX				0xd8
#define	RSP_ZELDATEXTURE			0xd7
#define	RSP_ZELDASUBMODULE			0xd6

// 4 is something like a conditional DL
#define RSP_DMATRI	0x05
#define G_DLINMEM	0x07

// RDP commands:
#define	RDP_NOOP			0xc0
#define	RDP_SETCIMG			0xff
#define	RDP_SETZIMG			0xfe
#define	RDP_SETTIMG			0xfd
#define	RDP_SETCOMBINE		0xfc
#define	RDP_SETENVCOLOR		0xfb
#define	RDP_SETPRIMCOLOR	0xfa
#define	RDP_SETBLENDCOLOR	0xf9
#define	RDP_SETFOGCOLOR		0xf8
#define	RDP_SETFILLCOLOR	0xf7
#define	RDP_FILLRECT		0xf6
#define	RDP_SETTILE			0xf5
#define	RDP_LOADTILE		0xf4
#define	RDP_LOADBLOCK		0xf3
#define	RDP_SETTILESIZE		0xf2
#define	RDP_LOADTLUT		0xf0
#define	RDP_RDPSETOTHERMODE	0xef
#define	RDP_SETPRIMDEPTH	0xee
#define	RDP_SETSCISSOR		0xed
#define	RDP_SETCONVERT		0xec
#define	RDP_SETKEYR			0xeb
#define	RDP_SETKEYGB		0xea
#define	RDP_FULLSYNC		0xe9
#define	RDP_TILESYNC		0xe8
#define	RDP_PIPESYNC		0xe7
#define	RDP_LOADSYNC		0xe6
#define RDP_TEXRECT_FLIP	0xe5
#define RDP_TEXRECT			0xe4

// Overloaded for sprite microcode
#define G_GBI1_SPRITE2D_SCALEFLIP    0xbe
#define G_GBI1_SPRITE2D_DRAW         0xbd
#define G_GBI1_SPRITE2D_BASE	9

#define RSP_ZELDA_MTX_MODELVIEW		0x00
#define RSP_ZELDA_MTX_PROJECTION	0x04
#define RSP_ZELDA_MTX_MUL			0x00
#define RSP_ZELDA_MTX_LOAD			0x02
#define RSP_ZELDA_MTX_PUSH			0x00
#define RSP_ZELDA_MTX_NOPUSH		0x01



//
// RSP_SETOTHERMODE_L sft: shift count

#define	RSP_SETOTHERMODE_SHIFT_ALPHACOMPARE		0
#define	RSP_SETOTHERMODE_SHIFT_ZSRCSEL			2
#define	RSP_SETOTHERMODE_SHIFT_RENDERMODE		3
#define	RSP_SETOTHERMODE_SHIFT_BLENDER			16

//
// RSP_SETOTHERMODE_H sft: shift count

#define	RSP_SETOTHERMODE_SHIFT_BLENDMASK		0	// unsupported 
#define	RSP_SETOTHERMODE_SHIFT_ALPHADITHER		4
#define	RSP_SETOTHERMODE_SHIFT_RGBDITHER		6

#define	RSP_SETOTHERMODE_SHIFT_COMBKEY			8
#define	RSP_SETOTHERMODE_SHIFT_TEXTCONV			9
#define	RSP_SETOTHERMODE_SHIFT_TEXTFILT			12
#define	RSP_SETOTHERMODE_SHIFT_TEXTLUT			14
#define	RSP_SETOTHERMODE_SHIFT_TEXTLOD			16
#define	RSP_SETOTHERMODE_SHIFT_TEXTDETAIL		17
#define	RSP_SETOTHERMODE_SHIFT_TEXTPERSP		19
#define	RSP_SETOTHERMODE_SHIFT_CYCLETYPE		20
#define	RSP_SETOTHERMODE_SHIFT_COLORDITHER		22	// unsupported in HW 2.0 
#define	RSP_SETOTHERMODE_SHIFT_PIPELINE			23

// RSP_SETOTHERMODE_H gPipelineMode 
#define	RSP_PIPELINE_MODE_1PRIMITIVE		(1 << RSP_SETOTHERMODE_SHIFT_PIPELINE)
#define	RSP_PIPELINE_MODE_NPRIMITIVE		(0 << RSP_SETOTHERMODE_SHIFT_PIPELINE)

// RSP_SETOTHERMODE_H gSetCycleType 
#define	CYCLE_TYPE_1		0
#define	CYCLE_TYPE_2		1
#define	CYCLE_TYPE_COPY		2
#define	CYCLE_TYPE_FILL		3

// RSP_SETOTHERMODE_H gSetTextureLUT 
#define TLUT_FMT_NONE			(0 << RSP_SETOTHERMODE_SHIFT_TEXTLUT)
#define TLUT_FMT_UNKNOWN		(1 << RSP_SETOTHERMODE_SHIFT_TEXTLUT)
#define TLUT_FMT_RGBA16			(2 << RSP_SETOTHERMODE_SHIFT_TEXTLUT)
#define TLUT_FMT_IA16			(3 << RSP_SETOTHERMODE_SHIFT_TEXTLUT)

// RSP_SETOTHERMODE_H gSet

#define RDP_TFILTER_POINT		(0 << RSP_SETOTHERMODE_SHIFT_TEXTFILT)
#define RDP_TFILTER_AVERAGE		(3 << RSP_SETOTHERMODE_SHIFT_TEXTFILT)
#define RDP_TFILTER_BILERP		(2 << RSP_SETOTHERMODE_SHIFT_TEXTFILT)

// RSP_SETOTHERMODE_L gSetAlphaCompare 
#define	RDP_ALPHA_COMPARE_NONE			(0 << RSP_SETOTHERMODE_SHIFT_ALPHACOMPARE)
#define	RDP_ALPHA_COMPARE_THRESHOLD		(1 << RSP_SETOTHERMODE_SHIFT_ALPHACOMPARE)
#define	RDP_ALPHA_COMPARE_DITHER		(3 << RSP_SETOTHERMODE_SHIFT_ALPHACOMPARE)

// RSP_SETOTHERMODE_L gSetRenderMode 
#define	Z_COMPARE			0x0010
#define	Z_UPDATE			0x0020
#define	ZMODE_DEC			0x0c00


//
// flags for RSP_SETGEOMETRYMODE
//
#define G_ZBUFFER				0x00000001
#define G_TEXTURE_ENABLE		0x00000002	// Microcode use only 
#define G_SHADE					0x00000004	// enable Gouraud interp 
//
#define G_SHADING_SMOOTH		0x00000200	// flat or smooth shaded 
#define G_CULL_FRONT			0x00001000
#define G_CULL_BACK				0x00002000
#define G_CULL_BOTH				0x00003000	// To make code cleaner 
#define G_FOG					0x00010000
#define G_LIGHTING				0x00020000
#define G_TEXTURE_GEN			0x00040000
#define G_TEXTURE_GEN_LINEAR	0x00080000
#define G_LOD					0x00100000	// NOT IMPLEMENTED 

//
// G_SETIMG fmt: set image formats
//
#define TXT_FMT_RGBA	0
#define TXT_FMT_YUV	1
#define TXT_FMT_CI		2
#define TXT_FMT_IA		3
#define TXT_FMT_I		4

//
// G_SETIMG siz: set image pixel size
//
#define TXT_SIZE_4b		0
#define TXT_SIZE_8b		1
#define TXT_SIZE_16b	2
#define TXT_SIZE_32b	3

//
// Texturing macros
//

#define	RDP_TXT_LOADTILE	7
#define	RDP_TXT_RENDERTILE	0

#define	RDP_TXT_NOMIRROR	0
#define	RDP_TXT_WRAP		0
#define	RDP_TXT_MIRROR		0x1
#define	RDP_TXT_CLAMP		0x2
#define	RDP_TXT_NOMASK		0
#define	RDP_TXT_NOLOD		0



//
// MOVEMEM indices
//
// Each of these indexes an entry in a dmem table
// which points to a 1-4 word block of dmem in
// which to store a 1-4 word DMA.
//
//
#define RSP_GBI1_MV_MEM_VIEWPORT	0x80
#define RSP_GBI1_MV_MEM_LOOKATY		0x82
#define RSP_GBI1_MV_MEM_LOOKATX		0x84
#define RSP_GBI1_MV_MEM_L0			0x86
#define RSP_GBI1_MV_MEM_L1			0x88
#define RSP_GBI1_MV_MEM_L2			0x8a
#define RSP_GBI1_MV_MEM_L3			0x8c
#define RSP_GBI1_MV_MEM_L4			0x8e
#define RSP_GBI1_MV_MEM_L5			0x90
#define RSP_GBI1_MV_MEM_L6			0x92
#define RSP_GBI1_MV_MEM_L7			0x94
#define RSP_GBI1_MV_MEM_TXTATT		0x96
#define RSP_GBI1_MV_MEM_MATRIX_1	0x9e	// NOTE: this is in moveword table 
#define RSP_GBI1_MV_MEM_MATRIX_2	0x98
#define RSP_GBI1_MV_MEM_MATRIX_3	0x9a
#define RSP_GBI1_MV_MEM_MATRIX_4	0x9c

# define RSP_GBI2_MV_MEM__VIEWPORT	8
# define RSP_GBI2_MV_MEM__LIGHT		10
# define RSP_GBI2_MV_MEM__POINT		12
# define RSP_GBI2_MV_MEM__MATRIX	14		/* NOTE: this is in moveword table */
# define RSP_GBI2_MV_MEM_O_LOOKATX	(0*24)
# define RSP_GBI2_MV_MEM_O_LOOKATY	(1*24)
# define RSP_GBI2_MV_MEM_O_L0		(2*24)
# define RSP_GBI2_MV_MEM_O_L1		(3*24)
# define RSP_GBI2_MV_MEM_O_L2		(4*24)
# define RSP_GBI2_MV_MEM_O_L3		(5*24)
# define RSP_GBI2_MV_MEM_O_L4		(6*24)
# define RSP_GBI2_MV_MEM_O_L5		(7*24)
# define RSP_GBI2_MV_MEM_O_L6		(8*24)
# define RSP_GBI2_MV_MEM_O_L7		(9*24)


//
// MOVEWORD indices
//
// Each of these indexes an entry in a dmem table
// which points to a word in dmem in dmem where
// an immediate word will be stored.
//
//
#define RSP_MOVE_WORD_MATRIX	0x00	// NOTE: also used by movemem 
#define RSP_MOVE_WORD_NUMLIGHT	0x02
#define RSP_MOVE_WORD_CLIP		0x04
#define RSP_MOVE_WORD_SEGMENT	0x06
#define RSP_MOVE_WORD_FOG		0x08
#define RSP_MOVE_WORD_LIGHTCOL	0x0a
#define	RSP_MOVE_WORD_POINTS	0x0c
#define	RSP_MOVE_WORD_PERSPNORM	0x0e

//
// These are offsets from the address in the dmem table
// 
#define RSP_MV_WORD_OFFSET_NUMLIGHT			0x00
#define RSP_MV_WORD_OFFSET_CLIP_RNX			0x04
#define RSP_MV_WORD_OFFSET_CLIP_RNY			0x0c
#define RSP_MV_WORD_OFFSET_CLIP_RPX			0x14
#define RSP_MV_WORD_OFFSET_CLIP_RPY			0x1c
#define RSP_MV_WORD_OFFSET_FOG				0x00	
#define RSP_MV_WORD_OFFSET_POINT_RGBA		0x10
#define RSP_MV_WORD_OFFSET_POINT_ST			0x14
#define RSP_MV_WORD_OFFSET_POINT_XYSCREEN	0x18
#define	RSP_MV_WORD_OFFSET_POINT_ZSCREEN		0x1c



// flags to inhibit pushing of the display list (on branch)
#define RSP_DLIST_PUSH		0x00
#define RSP_DLIST_NOPUSH		0x01


//
// RSP_MTX: parameter flags
//
#define	RSP_MATRIX_MODELVIEW		0x00
#define	RSP_MATRIX_PROJECTION	0x01

#define	RSP_MATRIX_MUL			0x00
#define	RSP_MATRIX_LOAD			0x02

#define RSP_MATRIX_NOPUSH		0x00
#define RSP_MATRIX_PUSH			0x04



struct OSTask_t
{
	uint32	type;
	uint32	flags;

	uint32	ucode_boot;
	uint32	ucode_boot_size;

	uint32	ucode;
	uint32	ucode_size;

	uint32	ucode_data;
	uint32	ucode_data_size;

	uint32	dram_stack;
	uint32	dram_stack_size;

	uint32	output_buff;
	uint32	output_buff_size;

	uint32	data_ptr;
	uint32	data_size;

	uint32	yield_data_ptr;
	uint32	yield_data_size;
};

union OSTask {
	OSTask_t		t;
	uint64	force_structure_alignment;
};

#define MAX_DL_STACK_SIZE	32
#define MAX_DL_COUNT		1000000

struct UcodeData
{
	uint32		ucode;
	uint32		crc_size;
	uint32		crc_800;
	const CHAR * ucode_name;
	bool		non_nearclip;
	bool		reject;
};

enum LoadType
{
	BY_NEVER_SET,
	BY_LOAD_BLOCK,
	BY_LOAD_TILE,
	BY_LOAD_TLUT,
};

struct LoadCmdInfo
{
	LoadType	loadtype;
	unsigned int sl		:10;	// Upper left S		- 8:3
	unsigned int tl		:10;	// Upper Left T		- 8:3
	unsigned int sh		:10;	// Lower Right S
	unsigned int th		:10;	// Lower Right T
	unsigned int dxt	:12;
};

struct RDP_BlenderSetting
{
	unsigned int	c2_m2b:2;
	unsigned int	c1_m2b:2;
	unsigned int	c2_m2a:2;
	unsigned int	c1_m2a:2;
	unsigned int	c2_m1b:2;
	unsigned int	c1_m1b:2;
	unsigned int	c2_m1a:2;
	unsigned int	c1_m1a:2;
};

//Todo, move these out into another file
inline uint32 pixels2bytes(uint32 pixels, uint32 size)
{
    return ((pixels << size) + 1) >> 1;
}

inline u32 bytes2pixels(uint32 bytes, uint32 size)
{
    return (bytes << 1) >> size;
}

struct SImageDescriptor
{
    uint32 Format;		// "RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"
    uint32 Size;		// "4bpp", "8bpp", "16bpp", "32bpp"
    uint32 Width;		// Num Pixels
    uint32 Address;	// Location

    inline uint32 GetPitch() const
    {
        return ((Width << Size) >> 1);
    }

    //Get Bpl -> ( Width << Size >> 1 )
    inline uint32 GetAddress(uint32 x, uint32 y) const
    {
        return Address + y * ((Width << Size) >> 1) + ((x << Size) >> 1);
    }

    // Optimised version when Size == G_IM_SIZ_16b
    inline uint32 GetPitch16bpp() const
    {
        return Width << 1;
    }
    inline uint32 GetAddress16bpp(uint32 x, uint32 y) const
    {
        return Address + ((y * Width + x) << 1);
    }

};

struct RDP_GeometryMode
{
    union
    {
        uint32	_u32;

        struct
        {
            uint32 GBI1_Zbuffer : 1;		// 0x1
            uint32 GBI1_Texture : 1;		// 0x2
            uint32 GBI1_Shade : 1;			// 0x4
            uint32 GBI1_pad0 : 6;			// 0x0
            uint32 GBI1_ShadingSmooth : 1;	// 0x200
            uint32 GBI1_pad1 : 2;			// 0x0
            uint32 GBI1_CullFront : 1;		// 0x1000
            uint32 GBI1_CullBack : 1;		// 0x2000
            uint32 GBI1_pad2 : 2;			// 0x0
            uint32 GBI1_Fog : 1;			// 0x10000
            uint32 GBI1_Lighting : 1;		// 0x20000
            uint32 GBI1_TexGen : 1;		// 0x40000
            uint32 GBI1_TexGenLin : 1;		// 0x80000
            uint32 GBI1_Lod : 1;			// 0x100000
            uint32 GBI1_pad3 : 11;			// 0x0
        };
        struct
        {
            uint32 GBI2_Zbuffer : 1;		// 0x1
            uint32 GBI2_pad0 : 8;			// 0x0
            uint32 GBI2_CullBack : 1;		// 0x200
            uint32 GBI2_CullFront : 1;		// 0x400
            uint32 GBI2_pad1 : 5;			// 0x0
            uint32 GBI2_Fog : 1;			// 0x10000
            uint32 GBI2_Lighting : 1;		// 0x20000
            uint32 GBI2_TexGen : 1;		// 0x40000
            uint32 GBI2_TexGenLin : 1;		// 0x80000
            uint32 GBI2_Lod : 1;			// 0x100000
            uint32 GBI2_ShadingSmooth : 1;	// 0x200000
            uint32 GBI2_PointLight : 1;	// 0x400000
            uint32 GBI2_pad2 : 9;			// 0x0
        };
    };
};

struct RDP_OtherMode
{
	union
	{
        uint64			_u64;

		struct
		{
			// Low bits
            uint32		alpha_compare : 2;			// 0..1
            uint32		depth_source : 1;			// 2..2

            uint32		aa_en : 1;					// 3
            uint32		z_cmp : 1;					// 4
            uint32		z_upd : 1;					// 5
            uint32		im_rd : 1;					// 6
            uint32		clr_on_cvg : 1;				// 7

            uint32		cvg_dst : 2;				// 8..9
            uint32		zmode : 2;					// 10..11

            uint32		cvg_x_alpha : 1;			// 12
            uint32		alpha_cvg_sel : 1;			// 13
            uint32		force_bl : 1;				// 14
            uint32		tex_edge : 1;				// 15 - Not used

            uint32		blender : 16;				// 16..31

			// High bits
            uint32		blend_mask : 4;				// 0..3 - not supported
            uint32		alpha_dither : 2;			// 4..5
            uint32		rgb_dither : 2;				// 6..7
			
            uint32		key_en : 1;					// 8..8
            uint32		text_conv : 3;				// 9..11
            uint32		text_filt : 2;				// 12..13
            uint32		text_tlut : 2;				// 14..15

            uint32		text_lod : 1;				// 16..16
            uint32		text_sharpen : 1;			// 17..18
            uint32		text_detail : 1;			// 17..18
            uint32		text_persp : 1;				// 19..19
            uint32		cycle_type : 2;				// 20..21
            uint32		color_dither : 1;				// 22..22 - not supported
            uint32		atomic_prim : 1;			// 23..23

            uint32		pad : 8;					// 24..31 - padding

		};

		struct
		{
            uint32	L;
            uint32	H;
		};

	};
};

struct RDP_TexRect
{
    union
    {
        struct
        {
            u32 cmd3;
            u32 cmd2;
            u32 cmd1;
            u32 cmd0;
        };

        struct
        {
            // cmd3
            s32		dtdy : 16;
            s32		dsdx : 16;

            // cmd2
            s32		t : 16;
            s32		s : 16;

            // cmd1
            u32		y0 : 12;
            u32		x0 : 12;
            u32		tile_idx : 3;
            s32		pad1 : 5;

            // cmd0
            u32		y1 : 12;
            u32		x1 : 12;

            u32		cmd : 8;
        };
    };
};

struct RDP_MemRect
{
    union
    {
        struct
        {
            u32 cmd2;
            u32 cmd1;
            u32 cmd0;
        };

        struct
        {

            // cmd2
            s32		t : 16;
            s32		s : 16;

            // cmd1
            u32		pad3 : 2;
            u32		y0 : 10;
            u32		pad2 : 2;
            u32		x0 : 10;
            u32		tile_idx : 3;
            s32		pad1 : 5;

            // cmd0
            u32		pad5 : 2;
            u32		y1 : 10;
            u32		pad4 : 2;
            u32		x1 : 10;

            u32		cmd : 8;
        };

    };
};

struct RDP_Tile
{
    union
    {
        struct
        {
            u32		cmd1;
            u32		cmd0;
        };

        struct
        {
            // cmd1
            u32		shift_s : 4;
            u32		mask_s : 4;
            u32		mirror_s : 1;
            u32		clamp_s : 1;

            u32		shift_t : 4;
            u32		mask_t : 4;
            u32		mirror_t : 1;
            u32		clamp_t : 1;

            u32		palette : 4;
            u32		tile_idx : 3;

            u32		pad1 : 5;

            // cmd0
            u32		tmem : 9;
            u32		line : 9;
            u32		pad0 : 1;
            u32		size : 2;
            u32		format : 3;
            u32		cmd : 8;
        };

    };

    bool	operator==(const RDP_Tile & rhs) const
    {
        return cmd0 == rhs.cmd0 && cmd1 == rhs.cmd1;
    }
    bool	operator!=(const RDP_Tile & rhs) const
    {
        return cmd0 != rhs.cmd0 || cmd1 != rhs.cmd1;
    }
};

struct RDP_TileSize
{
    union
    {
        struct
        {
            u32		cmd1;
            u32		cmd0;
        };

        struct
        {
            // cmd1
            u32		bottom : 12;
            u32		right : 12;

            u32		tile_idx : 3;
            u32		pad1 : 5;

            // cmd0
            u32		top : 12;
            u32		left : 12;

            u32		cmd : 8;
        };
    };


    uint16 GetWidth() const
    {
        return ((right - left) / 4) + 1;
    }

    uint16 GetHeight() const
    {
        return ((bottom - top) / 4) + 1;
    }

    bool operator==(const RDP_TileSize & rhs) const
    {
        return cmd0 == rhs.cmd0 && cmd1 == rhs.cmd1;
    }
    bool operator!=(const RDP_TileSize & rhs) const
    {
        return cmd0 != rhs.cmd0 || cmd1 != rhs.cmd1;
    }
};

enum SetTileCmdType
{ 
	CMD_SETTILE, 
	CMD_SETTILE_SIZE, 
	CMD_LOADBLOCK, 
	CMD_LOADTILE, 
	CMD_LOADTLUT, 
	CMD_SET_TEXTURE,
	CMD_LOAD_OBJ_TXTR,
};


// The display list PC stack. Before this was an array of 10
// items, but this way we can nest as deeply as necessary. 

struct DListStack
{
	u32 address[MAX_DL_STACK_SIZE];
	s32 limit;
};

struct ScissorType
{
	int x0, y0, x1, y1, mode;
	int left, top, right, bottom;
};

// Mask down to 0x003FFFFF?
#define RSPSegmentAddr(seg) ( gRSP.segments[((seg)>>24)&0x0F] + ((seg)&0x00FFFFFF) )
#define RDRAM_UWORD(addr)	(*(uint32 *)((addr)+g_pu8RamBase))
#define RDRAM_SWORD(addr)	(*(s32 *)((addr)+g_pu8RamBase))
#define RDRAM_UHALF(addr)	(*(uint16 *)(((addr)^2)+g_pu8RamBase))
#define RDRAM_SHALF(addr)	(*(short *)(((addr)^2)+g_pu8RamBase))
#define RDRAM_UBYTE(addr)	(*(uint8 *)(((addr)^3)+g_pu8RamBase))
#define RDRAM_SBYTE(addr)	(*(s8 *)(((addr)^3)+g_pu8RamBase))
#define pRDRAM_UWORD(addr)	((uint32 *)((addr)+g_pu8RamBase))
#define pRDRAM_SWORD(addr)	((s32 *)((addr)+g_pu8RamBase))
#define pRDRAM_UHALF(addr)	((uint16 *)(((addr)^2)+g_pu8RamBase))
#define pRDRAM_SHALF(addr)	((short *)(((addr)^2)+g_pu8RamBase))
#define pRDRAM_UBYTE(addr)	((uint8 *)(((addr)^3)+g_pu8RamBase))
#define pRDRAM_SBYTE(addr)	((s8 *)(((addr)^3)+g_pu8RamBase))

extern uint16 g_wRDPTlut[];
extern const char *textluttype[4];

extern char *pszImgFormat[8];
extern char *pszImgSize[4];
extern uint8 pnImgSize[4];
extern char *textlutname[4];

extern SetImgInfo g_CI;
extern SetImgInfo g_ZI;
extern SetImgInfo g_TI;
extern TmemType g_Tmem;

extern DListStack	gDlistStack;

extern int gDlistStackPointer;

void DLParser_Init();
void RDP_GFX_Reset();
void RDP_Cleanup();
void DLParser_Process();
void RDP_DLParser_Process(void);

void PrepareTextures();
void RDP_InitRenderState();
void DisplayVertexInfo(uint32 dwAddr, uint32 dwV0, uint32 dwN);
void RSP_MoveMemLight(uint32 dwLight, const N64Light *light);
void RSP_MoveMemViewport(uint32 dwAddr);
void RDP_NOIMPL_WARN(LPCTSTR op);
void RSP_GFX_Force_Matrix(uint32 dwAddr);
inline void DLParser_FetchNextCommand(MicroCodeCommand *p_command); 
void RSP_GFX_InitGeometryMode();
void RSP_SetUcode(int ucode, uint32 ucStart=0, uint32 ucDStart=0, uint32 cdSize=0);
void RDP_GFX_PopDL();

void MatrixFromN64FixedPoint(Matrix4x4 & mat, u32 address);

ULONG ComputeCRC32(ULONG crc, const uint8 *buf, UINT len);

void TriggerDPInterrupt();

bool IsUsedAsDI(uint32 addr);

#if defined(_DEBUG)
void __cdecl LOG_UCODE(LPCTSTR szFormat, ...);
#else
// VC7
#define LOG_UCODE __noop
#endif
#endif	// __RICE_RDP_GFX_H__
