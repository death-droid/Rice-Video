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

#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

#define uint16 unsigned __int16
#define uint32 unsigned __int32
#define uint64 unsigned __int64


typedef unsigned char				uint8;

typedef signed char					s8;	
typedef signed short				s16;
typedef long						s32;
typedef unsigned long				u32;
typedef unsigned char				u8;

//Fix me, these macro should not be used anymore in DirectX 8

#define RGBA_GETALPHA(rgb)      ((rgb) >> 24)
#define RGBA_GETRED(rgb)        (((rgb) >> 16) & 0xff)
#define RGBA_GETGREEN(rgb)      (((rgb) >> 8) & 0xff)
#define RGBA_GETBLUE(rgb)       ((rgb) & 0xff)

typedef D3DXMATRIX Matrix;
typedef D3DVECTOR Vector3;
typedef D3DLOCKED_RECT LockRectType;

#define COLOR_RGBA D3DCOLOR_RGBA


typedef struct
{
	unsigned int	dwFormat:3;
	unsigned int	dwSize:2;
	unsigned int	dwWidth:10;
	uint32			dwAddr;
	uint32			bpl;
} SetImgInfo;

typedef struct 
{
	// Set by RDP_SetTile
	unsigned int dwFormat	:3;		// e.g. RGBA, YUV etc
	unsigned int dwSize		:2;		// e.g 4/8/16/32bpp
	unsigned int dwLine		:9;		// Ummm...
	unsigned int dwPalette	:4;		// 0..15 - a palette index?
	uint32 dwTMem;					// Texture memory location

	unsigned int bClampS	:1;
	unsigned int bClampT	:1;
	unsigned int bMirrorS	:1;
	unsigned int bMirrorT	:1;

	unsigned int dwMaskS	:4;
	unsigned int dwMaskT	:4;
	unsigned int dwShiftS	:4;
	unsigned int dwShiftT	:4;

	// Set by RDP_SetTileSize
	int sl;		// Upper left S		- 8:3
	int tl;		// Upper Left T		- 8:3
	int sh;		// Lower Right S
	int th;		// Lower Right T

	int   hilite_sl;
	int	  hilite_tl;
	int   hilite_sh;
	int	  hilite_th;

	float fsl;		// Upper left S		- 8:3
	float ftl;		// Upper Left T		- 8:3
	float fsh;		// Lower Right S
	float fth;		// Lower Right T

	float   fhilite_sl;
	float	fhilite_tl;
	float   fhilite_sh;
	float	fhilite_th;

	uint32 dwDXT;

	uint32 dwPitch;

	uint32 dwWidth;
	uint32 dwHeight;

	float fShiftScaleS;
	float fShiftScaleT;

	uint32   lastTileCmd;
	bool  bSizeIsValid;

	bool bForceWrapS;
	bool bForceWrapT;
	bool bForceClampS;
	bool bForceClampT;

} Tile;


typedef struct
{
	float u;
	float v;
} TexCord;

typedef struct VECTOR2
{
	float x;
	float y;
	VECTOR2( float newx, float newy )	{x=newx; y=newy;}
	VECTOR2()	{x=0; y=0;}
} VECTOR2;

typedef struct
{
	short x;
	short y;
} IVector2;

typedef struct 
{
	short x;
	short y;
	short z;
} IVector3;

#define RICE_FVF_TLITVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2 ) 
typedef struct {
	float x,y,z;
	float rhw;
	union {
		D3DCOLOR  dcDiffuse;
		struct {
			uint8 b;
			uint8 g;
			uint8 r;
			uint8 a;
		};
	};
	D3DCOLOR  dcSpecular;
	TexCord tcord[2];
} TLITVERTEX, *LPTLITVERTEX;

#define RICE_FVF_UTLITVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2 ) 
typedef struct {
	float x,y,z;
	union {
		D3DCOLOR  dcDiffuse;
		struct {
			uint8 b;
			uint8 g;
			uint8 r;
			uint8 a;
		};
	};
	D3DCOLOR  dcSpecular;
	TexCord tcord[2];
} UTLITVERTEX, *LPUTLITVERTEX;


#define RICE_FVF_LITVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR ) 
typedef struct {
	float x,y,z;
	float rhw;
	union {
		D3DCOLOR  dcDiffuse;
		struct {
			uint8 b;
			uint8 g;
			uint8 r;
			uint8 a;
		};
	};
	D3DCOLOR  dcSpecular;
} LITVERTEX, *LPLITVERTEX;



typedef struct {
	float	x,y,z;
	float	rhw;
	D3DCOLOR dcDiffuse;
} FILLRECTVERTEX, *LPFILLRECTVERTEX;

#include "./Utility/IColor.h"

typedef struct
{
	float x,y,z;
	float nx,ny,nz;
	union {
		D3DCOLOR  dcDiffuse;
		struct {
			uint8 b;
			uint8 g;
			uint8 r;
			uint8 a;
		};
	};
	float u,v;
}EXTERNAL_VERTEX, *LPSHADERVERTEX;


typedef struct
{
	union {
		struct {
			float x;
			float y;
			float z;
			float range;		// Range == 0  for directional light
								// Range != 0  for point light, Zelda MM
		};
	};

	union {
		struct {
			uint8 r;
			uint8 g;
			uint8 b;
			uint8 a;
		};
		uint32 col;
	};

	union {
		struct {
			float fr;
			float fg;
			float fb;
			float fa;
		};
		float fcolors[4];
	};

	union {
		struct {
			float tx;
			float ty;
			float tz;
			float tdummy;
		};
		struct {
			D3DXVECTOR3 td;
		};
	};

	union {
		struct {
			float ox;
			float oy;
			float oz;
			float odummy;
		};
		struct {
			D3DXVECTOR3 od;
		};
	};
} Light;



typedef struct
{
	uint32 dwColorOp;
	uint32 dwColorArg0;
	uint32 dwColorArg1;
	uint32 dwColorArg2;
	uint32 dwAlphaOp;
	uint32 dwAlphaArg0;
	uint32 dwAlphaArg1;
	uint32 dwAlphaArg2;
	uint32 dwResultArg;
	
	uint32 dwMinFilter;
	uint32 dwMagFilter;

	uint32 dwAddressUMode;
	uint32 dwAddressVMode;
	uint32 dwAddressW;

	uint32 dwTexCoordIndex;
	LPDIRECT3DBASETEXTURE9 pTexture; 
} D3DCombinerStage;

typedef struct
{
	char na;
	char nz;	// b
	char ny;	//g
	char nx;	//r
}NormalStruct;

typedef struct
{
	short y;
	short x;
	
	short flag;
	short z;
	
	short tv;
	short tu;
	
	union {
		struct _rgba {
			uint8 a;
			uint8 b;
			uint8 g;
			uint8 r;
		} rgba;
		NormalStruct norma;
	};
} FiddledVtx;

typedef struct
{
	short y;
	short x;
	
	uint8 a;
	uint8 b;
	short z;
	
	uint8 g;
	uint8 r;
	
} FiddledVtxDKR;

typedef struct 
{
	short y;
	short	x;
	uint16	cidx;
	short z;
	short t;
	short s;
} N64VtxPD;

class CTexture;
class CDirectXTexture;
struct TxtrCacheEntry;

typedef struct {
	LPDIRECT3DTEXTURE9 m_lpsTexturePtr;
	union {
		CTexture *			m_pCTexture;
	};
	
	uint32 m_dwTileWidth;
	uint32 m_dwTileHeight;
	float m_fTexWidth;
	float m_fTexHeight;		// Float to avoid converts when processing verts
	TxtrCacheEntry *pTextureEntry;
} RenderTexture;


typedef struct
{
	unsigned __int32	dwFormat;
	unsigned __int32	dwSize;
	unsigned __int32	dwWidth;
	unsigned __int32	dwAddr;

	unsigned __int32	dwLastWidth;
	unsigned __int32	dwLastHeight;

	unsigned __int32	dwHeight;
	unsigned __int32	dwMemSize;

	bool				bCopied;
	unsigned __int32	dwCopiedAtFrame;

	unsigned __int32	dwCRC;
	unsigned __int32	lastUsedFrame;
	unsigned __int32	bUsedByVIAtFrame;
	unsigned __int32	lastSetAtUcode;
} RecentCIInfo;

typedef struct
{
	uint32		addr;
	uint32		FrameCount;
} RecentViOriginInfo;

typedef enum {
	SHADE_DISABLED,
	SHADE_FLAT,
	SHADE_SMOOTH,
} RenderShadeMode;

typedef struct {
	char	*description;
	int		number;
	uint32	setting;
} BufferSettingInfo;

typedef struct {
	char	*description;
	uint32	setting;
} SettingInfo;

typedef union {
	uint8	g_Tmem8bit[0x1000];
	__int16	g_Tmem16bit[0x800];
	uint32	g_Tmem32bit[0x300];
	uint64	g_Tmem64bit[0x200];
} TmemType;


typedef struct {
	uint32 dwFormat;
	uint32 dwSize;
	BOOL  bSetBy;

	uint32 dwLoadAddress;
	uint32 dwTotalWords;
	uint32 dxt;
	BOOL  bSwapped;

	uint32 dwWidth;
	uint32 dwLine;

	int sl;
	int sh;
	int tl;
	int th;

	uint32 dwTmem;
} TMEMLoadMapInfo;

#endif
