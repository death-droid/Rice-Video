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

#ifndef _RICE_RENDER_BASE_H
#define _RICE_RENDER_BASE_H

/*
 *	Global variables defined in this file were moved out of Render class
 *  to make them be accessed faster
 */

#define RICE_MATRIX_STACK		20
#define MAX_TEXTURES			 8

enum FillMode
{
	RICE_FILLMODE_WINFRAME,
	RICE_FILLMODE_SOLID,
};

enum { MAX_VERTS = 80 };		// F3DLP.Rej supports up to 80 verts!

// All these arrays are moved out of the class CRender
// to be accessed in faster speed
extern DaedalusVtx4	g_vecProjected[MAX_VERTS];
extern uint32		g_dwVtxDifColor[MAX_VERTS];

extern RenderTexture g_textures[MAX_TEXTURES];

extern TLITVERTEX			g_vtxBuffer[1000];
extern unsigned int			g_vtxIndex[1000];

extern TLITVERTEX			g_clippedVtxBuffer[2000];
extern int					g_clippedVtxCount;

extern float				g_fFogCoord[MAX_VERTS];

extern TLITVERTEX			g_texRectTVtx[4];


//#define INIT_VERTEX_METHOD_2

/*
 *	Global variables
 */

/************************************************************************/
/*      Don't move                                                      */
/************************************************************************/

extern uint32	gRSPnumLights;
extern Light	gRSPlights[16];
extern float	gRSPfFogMin;
extern float	gRSPfFogMax;
extern float	gRSPfFogDivider;

/************************************************************************/
/*      Don't move                                                      */
/************************************************************************/
__declspec(align(16)) struct RSP_Options
{
	/************************************************************************/
	/*      Don't move above                                                */
	/************************************************************************/
	bool	bTextureEnabled;
	uint32	curTile;
	float	fTexScaleX;
	float	fTexScaleY;

	RenderShadeMode	shadeMode;

	uint32	numVertices;
	uint32  maxVertexID;

	int		nVPLeftN, nVPTopN, nVPRightN, nVPBottomN, nVPWidthN, nVPHeightN, maxZ;
	int		clip_ratio_left,	clip_ratio_top,	clip_ratio_right,	clip_ratio_bottom;
	int		real_clip_scissor_left,	real_clip_scissor_top,	real_clip_scissor_right,	real_clip_scissor_bottom;

	mutable Matrix4x4	mWorldProject;
	Matrix4x4			mTempMat;
	Matrix4x4			mProjectionMat;
	Matrix4x4			mModelViewStack[RICE_MATRIX_STACK];	//DKR reuses these and need at least 4 //Corn
	u32					mModelViewTop;
	u32					mMatStackSize;
	mutable bool		mWorldProjectValid;
	bool				mReloadProj;
	bool				mWPmodified;
	u32					mDKRMatIdx;

	bool	bLightIsUpdated;

	uint32		segments[16];

	v4		DKRBaseVec;

	int		vertexMult;

	// For DirectX only
	float	vtxXMul;
	float	vtxXAdd;
	float	vtxYMul;
	float	vtxYAdd;

	// Texture coordinates computation constants
	float	tex0scaleX;
	float	tex0scaleY;
	float	tex1scaleX;
	float	tex1scaleY;
	float	tex0OffsetX;
	float	tex0OffsetY;
	float	tex1OffsetX;
	float	tex1OffsetY;
	float	texGenYRatio;
	float	texGenXRatio;

};

__declspec(align(16)) struct RiceVideoVtx4
{

};

struct TnLMode
{
	union
	{
		struct
		{
			u32 Light : 1;			// 0x1
			u32 Texture : 1;		// 0x2
			u32 TexGen : 1;			// 0x4
			u32 TexGenLin : 1;		// 0x8
			u32 Fog : 1;			// 0x10
			u32 Shade : 1;			// 0x20
			u32 Zbuffer : 1;		// 0x40
			u32 TriCull : 1;		// 0x80
			u32 CullBack : 1;		// 0x100
			u32 PointLight : 1;		// 0x200
			u32 pad0 : 22;			// 0x0
		};
		u32	_u32;
	};
};

extern RSP_Options gRSP;


__declspec(align(16)) struct RDP_Options{
	bool	bFogEnableInBlender;

	uint32	fogColor;
	uint32	primitiveColor;
	uint32	envColor;
	uint32	primitiveDepth;
	uint32	primLODMin;
	uint32	primLODFrac;
	uint32	LODFrac;

	float	fPrimitiveDepth;
	float	fvFogColor[4];
	float	fvPrimitiveColor[4];
	float	fvEnvColor[4];

	uint32	fillColor;
	uint32	originalFillColor;

	TnLMode tnl;
	float CoordMod[16];

	RDP_OtherMode otherMode;

	Tile	tiles[8];
	ScissorType scissor;

	bool	textureIsChanged;
};

extern RDP_Options gRDP;

/*
*	Global functions
*/
void InitRenderBase();
void SetFogMinMax(float fMin, float fMax, float fMul, float fOffset);
void InitVertex(uint32 dwV, uint32 vtxIndex, bool bTexture);
void InitVertexTextureConstants();
bool AddTri(u32 v0, u32 v1, u32 v2, bool bTri4 = false);
bool PrepareTriangle(uint32 dwV0, uint32 dwV1, uint32 dwV2);
bool IsTriangleVisible(uint32 dwV0, uint32 dwV1, uint32 dwV2);
void ProcessVertexData(uint32 dwAddr, uint32 dwV0, uint32 dwNum);
void SetPrimitiveColor(uint32 dwCol, uint32 LODMin, uint32 LODFrac);
void SetPrimitiveDepth(uint32 z, uint32 dwDZ);
void SetVertexXYZ(uint32 vertex, float x, float y, float z);
void ModifyVertexInfo(uint32 where, uint32 vertex, uint32 val);
void ProcessVertexDataDKR(uint32 dwAddr, uint32 dwV0, uint32 dwNum);
void SetLightCol(uint32 dwLight, u8 r, u8 g, u8 b);
void SetLightDirection(uint32 dwLight, float x, float y, float z);
void SetLightPosition(uint32 dwLight, float x, float y, float z, float w);
void SetLightCBFD(uint32 dwLight, short nonzero);
void SetLightEx(uint32 dwLight, float ca, float la, float qa);

void ForceMainTextureIndex(int dwTile); 

void ClipVertexes();

inline float ViewPortTranslatef_x(float x) { return ( (x+1) * windowSetting.vpWidthW/2) + windowSetting.vpLeftW; }
inline float ViewPortTranslatef_y(float y) { return ( (1-y) * windowSetting.vpHeightW/2) + windowSetting.vpTopW; }
inline float ViewPortTranslatei_x(LONG x) { return x*windowSetting.fMultX; }
inline float ViewPortTranslatei_y(LONG y) { return y*windowSetting.fMultY; }
inline float ViewPortTranslatei_x(float x) { return x*windowSetting.fMultX; }
inline float ViewPortTranslatei_y(float y) { return y*windowSetting.fMultY; }

//*****************************************************************************
// We round these value here, so that when we scale up the coords to our screen
// coords we don't get any gaps.
//*****************************************************************************
inline void ConvertN64ToScreen(const v2 & n64_coords, v2 & answ)
{
	answ.x = roundf(ViewPortTranslatei_x(roundf(n64_coords.x)));
	answ.y = roundf(ViewPortTranslatei_y(roundf(n64_coords.y)));
}

inline float GetPrimitiveDepth() { return gRDP.fPrimitiveDepth; }
inline uint32 GetPrimitiveColor() { return gRDP.primitiveColor; }
inline float* GetPrimitiveColorfv() { return gRDP.fvPrimitiveColor; }
inline uint32 GetLODFrac() { return gRDP.LODFrac; }
inline void SetEnvColor(uint32 dwCol) 
{ 
	gRDP.envColor = dwCol; 
	gRDP.fvEnvColor[0] = ((dwCol>>16)&0xFF)/255.0f;		//r
	gRDP.fvEnvColor[1] = ((dwCol>>8)&0xFF)/255.0f;			//g
	gRDP.fvEnvColor[2] = ((dwCol)&0xFF)/255.0f;			//b
	gRDP.fvEnvColor[3] = ((dwCol>>24)&0xFF)/255.0f;		//a
}
inline uint32 GetEnvColor() { return gRDP.envColor; }
inline float* GetEnvColorfv() { return gRDP.fvEnvColor; }


inline void SetNumLights(uint32 dwNumLights) 
{ 
	gRSPnumLights = dwNumLights; 
	DEBUGGER_PAUSE_AND_DUMP(NEXT_SET_LIGHT,TRACE1("Set Num Of Light: %d", dwNumLights));
}
inline uint32 GetNumLights() { return gRSPnumLights; }
inline D3DCOLOR GetVertexDiffuseColor(uint32 ver) { return g_dwVtxDifColor[ver]; }
inline void SetScreenMult(float fMultX, float fMultY) { windowSetting.fMultX = fMultX; windowSetting.fMultY = fMultY; }

#endif
