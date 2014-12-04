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
#include "float.h"

extern FiddledVtx * g_pVtxBase;

#define ENABLE_CLIP_TRI
#define X_CLIP_MAX	0x1
#define X_CLIP_MIN	0x2
#define Y_CLIP_MAX	0x4
#define Y_CLIP_MIN	0x8
#define Z_CLIP_MAX	0x10
#define Z_CLIP_MIN	0x20

#ifdef ENABLE_CLIP_TRI

inline void RSP_Vtx_Clipping(int i)
{
	g_clipFlag[i] = 0;
	g_clipFlag2[i] = 0;
	if( g_vecProjected[i].w > 0 )
	{
		{
			float scaleFactor = 1.0f;
			if(windowSetting.uScreenScaleMode == 1)
				scaleFactor = (3.0f * windowSetting.uDisplayWidth) / (4.0f * windowSetting.uDisplayHeight);

			if( g_vecProjected[i].x > scaleFactor )   g_clipFlag2[i] |= X_CLIP_MAX;
			if( g_vecProjected[i].x < -scaleFactor )  g_clipFlag2[i] |= X_CLIP_MIN;
			if( g_vecProjected[i].y > 1 )	g_clipFlag2[i] |= Y_CLIP_MAX;
			if( g_vecProjected[i].y < -1 )	g_clipFlag2[i] |= Y_CLIP_MIN;
			//if( g_vecProjected[i].z > 1.0f )	g_clipFlag2[i] |= Z_CLIP_MAX;
			//if( gRSP.bNearClip && g_vecProjected[i].z < -1.0f )	g_clipFlag2[i] |= Z_CLIP_MIN;
		}

	}
}

#else
inline void RSP_Vtx_Clipping(int i) {}
#endif

/*
 *	Global variables
 */
RSP_Options gRSP;
RDP_Options gRDP;

static int norms[3];

__declspec(align(16)) v4	g_vtxNonTransformed[MAX_VERTS];
__declspec(align(16)) v4	g_vecProjected[MAX_VERTS];
__declspec(align(16)) v4	g_vtxTransformed[MAX_VERTS];

//uint32		g_dwVtxFlags[MAX_VERTS];			// Z_POS Z_NEG etc
v2		g_fVtxTxtCoords[MAX_VERTS];
uint32		g_dwVtxDifColor[MAX_VERTS];
uint32		g_clipFlag[MAX_VERTS]; //Unused? Remove?
uint32		g_clipFlag2[MAX_VERTS];
RenderTexture g_textures[MAX_TEXTURES];
float		g_fFogCoord[MAX_VERTS];

TLITVERTEX			g_vtxBuffer[1000];
TLITVERTEX			g_clippedVtxBuffer[2000];
int					g_clippedVtxCount=0;
TLITVERTEX			g_texRectTVtx[4];
unsigned int		g_vtxIndex[1000];
unsigned int		g_minIndex, g_maxIndex;

float				gRSPfFogMin;
float				gRSPfFogMax;
float				gRSPfFogDivider;

uint32			gRSPnumLights;
Light	gRSPlights[16];

__declspec(align(16)) Matrix4x4	gRSPworldProject;
__declspec(align(16)) Matrix4x4	gRSPmodelViewTop;



void InitRenderBase()
{
	gRSPfFogMin = gRSPfFogMax = 0.0f;
	windowSetting.fMultX = windowSetting.fMultY = 2.0f;
	windowSetting.vpLeftW = windowSetting.vpTopW = 0;
	windowSetting.vpRightW = windowSetting.vpWidthW = 640;
	windowSetting.vpBottomW = windowSetting.vpHeightW = 480;
	gRSP.maxZ = 0;
	gRSP.nVPLeftN = gRSP.nVPTopN = 0;
	gRSP.nVPRightN = 640;
	gRSP.nVPBottomN = 640;
	gRSP.nVPWidthN = 640;
	gRSP.nVPHeightN = 640;
	gRDP.scissor.left=gRDP.scissor.top=0;
	gRDP.scissor.right=gRDP.scissor.bottom=640;
	
	gRSP.curTile=gRSPnumLights=gRSP.ambientLightIndex= 0;
	gRSP.projectionMtxTop = gRSP.modelViewMtxTop = 0;
	gRDP.fogColor = gRDP.primitiveColor = gRDP.envColor = gRDP.primitiveDepth = gRDP.primLODMin = gRDP.primLODFrac = gRDP.LODFrac = 0;
	gRDP.fPrimitiveDepth = 0;
	gRSP.numVertices = 0;
	gRSP.maxVertexID = 0;
	gRDP.bFogEnableInBlender=false;
	gRSP.shadeMode=SHADE_SMOOTH;
	gRDP.keyR=gRDP.keyG=gRDP.keyB=gRDP.keyA=gRDP.keyRGB=gRDP.keyRGBA = 0;
	gRDP.fKeyA = 0;

	gRSP.fTexScaleX = 1/32.0f;
	gRSP.fTexScaleY = 1/32.0f;
	gRSP.bTextureEnabled = FALSE;

	gRSP.clip_ratio_left = 0;
	gRSP.clip_ratio_top = 0;
	gRSP.clip_ratio_right = 640;
	gRSP.clip_ratio_bottom = 480;
	gRSP.clip_ratio_negx = 1;
	gRSP.clip_ratio_negy = 1;
	gRSP.clip_ratio_posx = 1;
	gRSP.clip_ratio_posy = 1;
	gRSP.real_clip_scissor_left = 0;
	gRSP.real_clip_scissor_top = 0;
	gRSP.real_clip_scissor_right = 640;
	gRSP.real_clip_scissor_bottom = 480;
	windowSetting.clipping.left = 0;
	windowSetting.clipping.top = 0;
	windowSetting.clipping.right = 640;
	windowSetting.clipping.bottom = 480;
	windowSetting.clipping.width = 640;
	windowSetting.clipping.height = 480;
	windowSetting.clipping.needToClip = false;
	gRSP.real_clip_ratio_negx = 1;
	gRSP.real_clip_ratio_negy = 1;
	gRSP.real_clip_ratio_posx = 1;
	gRSP.real_clip_ratio_posy = 1;

	gRDP.tnl._u32	= 0;
	gRDP.otherMode.L		= 0;
	gRDP.otherMode.H		= 0;
	gRDP.fillColor		= 0xFFFFFFFF;
	gRDP.originalFillColor	=0;

	gRSP.bNearClip	= false;
	gRSP.bRejectVtx	= false;

	gRDP.texturesAreReloaded = false;
	gRDP.textureIsChanged = false;
	gRDP.colorsAreReloaded = false;

	memset(&gRDP.otherMode,0,sizeof(RDP_OtherMode));
	memset(&gRDP.tiles,0,sizeof(Tile)*8);

	int i;

	for( i=0; i<MAX_VERTS; i++ )
		g_clipFlag[i] = 0;
	for( i=0; i<MAX_VERTS; i++ )
		g_vtxNonTransformed[i].w = 1;

}

void SetFogMinMax(float fMin, float fMax, float fMul, float fOffset)
{
	if( fMin > fMax )
	{
		float temp = fMin;
		fMin = fMax;
		fMax = temp;
	}

	{
		gRSPfFogMin = max(0,fMin/500-1);
		gRSPfFogMax = fMax/500-1;
	}

	gRSPfFogDivider = 255/(gRSPfFogMax-gRSPfFogMin);
	CRender::g_pRender->SetFogMinMax(fMin, fMax);
}

void InitVertexTextureConstants()
{
	float scaleX;
	float scaleY;

	RenderTexture &tex0 = g_textures[gRSP.curTile];
	CTexture *surf = tex0.m_pCTexture;
	Tile &tile0 = gRDP.tiles[gRSP.curTile];

	scaleX = gRSP.fTexScaleX;
	scaleY = gRSP.fTexScaleY;

	gRSP.tex0scaleX = scaleX * tile0.fShiftScaleS/tex0.m_fTexWidth;
	gRSP.tex0scaleY = scaleY * tile0.fShiftScaleT/tex0.m_fTexHeight;

	gRSP.tex0OffsetX = tile0.fhilite_sl/tex0.m_fTexWidth;
	gRSP.tex0OffsetY = tile0.fhilite_tl/tex0.m_fTexHeight;

	if( CRender::g_pRender->IsTexel1Enable() )
	{
		RenderTexture &tex1 = g_textures[(gRSP.curTile+1)&7];
		CTexture *surf = tex1.m_pCTexture;
		Tile &tile1 = gRDP.tiles[(gRSP.curTile+1)&7];

		gRSP.tex1scaleX = scaleX * tile1.fShiftScaleS/tex1.m_fTexWidth;
		gRSP.tex1scaleY = scaleY * tile1.fShiftScaleT/tex1.m_fTexHeight;

		gRSP.tex1OffsetX = tile1.fhilite_sl/tex1.m_fTexWidth;
		gRSP.tex1OffsetY = tile1.fhilite_tl/tex1.m_fTexHeight;
	}

	gRSP.texGenXRatio = tile0.fShiftScaleS;
	gRSP.texGenYRatio = gRSP.fTexScaleX/gRSP.fTexScaleY*tex0.m_fTexWidth/tex0.m_fTexHeight*tile0.fShiftScaleT;
}


void ComputeLOD()
{
	TLITVERTEX &v0 = g_vtxBuffer[0];
	TLITVERTEX &v1 = g_vtxBuffer[1];
	RenderTexture &tex0 = g_textures[gRSP.curTile];

	float d,dt;
	float x = (v0.x - v1.x)/ windowSetting.fMultX;
	float y = (v0.y - v1.y)/ windowSetting.fMultY;
	d = sqrtf(x*x+y*y);

	float s0 = v0.tcord[0].u * tex0.m_fTexWidth;
	float t0 = v0.tcord[0].v * tex0.m_fTexHeight;
	float s1 = v1.tcord[0].u * tex0.m_fTexWidth;
	float t1 = v1.tcord[0].v * tex0.m_fTexHeight;

	dt = sqrtf((s0-s1)*(s0-s1)+(t0-t1)*(t0-t1));

	float lod = dt/d;
	int ilod = (int)lod;
	double intptr;
	float frac = log10f(lod)/log10f(2.0f);
	//DEBUGGER_IF_DUMP(pauseAtNext,{DebuggerAppendMsg("LOD frac = %f", frac);});
	int lod_tile = min((int)(log10f((float)ilod)/log10f(2.0f)), gRSP.curTile + floorf(frac));
	frac = max((float)modf(lod / pow(2.,lod_tile),&intptr), gRDP.primLODMin / 255.0f);
	//DEBUGGER_IF_DUMP(pauseAtNext,{DebuggerAppendMsg("LOD = %f, frac = %f", lod, frac);});
	gRDP.LODFrac = (uint32)(frac*255);
	CRender::g_pRender->SetCombinerAndBlender();
}

bool bHalfTxtScale=false;
extern uint32 lastSetTile;

void InitVertex(uint32 dwV, uint32 vtxIndex, bool bTexture)
{
	VTX_DUMP(TRACE2("Init vertex (%d) to vtx buf[%d]:", dwV, vtxIndex));

	TLITVERTEX &v = g_vtxBuffer[vtxIndex];

	VTX_DUMP(TRACE4("  Trans: x=%f, y=%f, z=%f, w=%f",  g_vtxTransformed[dwV].x,g_vtxTransformed[dwV].y,g_vtxTransformed[dwV].z,g_vtxTransformed[dwV].w));
	float scaleFactor = 1.0f;
	if(windowSetting.uScreenScaleMode == 2)
		scaleFactor = (4.0f * windowSetting.uDisplayHeight) / (3.0f * windowSetting.uDisplayWidth);
	v.x = g_vecProjected[dwV].x*gRSP.vtxXMul+gRSP.vtxXAdd*scaleFactor;
	v.y = g_vecProjected[dwV].y*gRSP.vtxYMul+gRSP.vtxYAdd;
	v.z = (g_vecProjected[dwV].z + 1.0f) * 0.5f;	// DirectX minZ=0, maxZ=1
	v.rhw = g_vecProjected[dwV].w;
	VTX_DUMP(TRACE4("  Proj : x=%f, y=%f, z=%f, rhw=%f",  v.x,v.y,v.z,v.rhw));

	if( gRDP.tnl.Fog )
	{
		uint32	fogFct = 0xFF-(uint8)((g_fFogCoord[dwV]-gRSPfFogMin)*gRSPfFogDivider);
		v.dcSpecular = (fogFct<<24);
	}
	VTX_DUMP(TRACE2("  (U,V): %f, %f",  g_fVtxTxtCoords[dwV].x,g_fVtxTxtCoords[dwV].y));

	v.dcDiffuse = g_dwVtxDifColor[dwV];
	if( gRDP.otherMode.key_en )
	{
		v.dcDiffuse &= 0x00FFFFFF;
		v.dcDiffuse |= (gRDP.keyA<<24);
	}
	else if( gRDP.otherMode.aa_en && gRDP.otherMode.clr_on_cvg==0 )
	{
		v.dcDiffuse |= 0xFF000000;
	}

	if( options.bWinFrameMode )
	{
		v.dcDiffuse = g_dwVtxDifColor[dwV];
	}

	if( bTexture )
	{
		// If the vert is already lit, then there is no normal (and hence we can't generate tex coord)
		// Only scale if not generated automatically
		if (gRDP.tnl.TexGen && gRDP.tnl.Light)
		{
			// Correction for texGen result
			float u0,u1,v0,v1;
			RenderTexture &tex0 = g_textures[gRSP.curTile];
			u0 = g_fVtxTxtCoords[dwV].x * 32 * 1024 * gRSP.fTexScaleX / tex0.m_fTexWidth;
			v0 = g_fVtxTxtCoords[dwV].y * 32 * 1024 * gRSP.fTexScaleY / tex0.m_fTexHeight;
			u0 *= (gRDP.tiles[gRSP.curTile].fShiftScaleS);
			v0 *= (gRDP.tiles[gRSP.curTile].fShiftScaleT);

			if( CRender::g_pRender->IsTexel1Enable() )
			{
				RenderTexture &tex1 = g_textures[(gRSP.curTile+1)&7];
				u1 = g_fVtxTxtCoords[dwV].x * 32 * 1024 * gRSP.fTexScaleX / tex1.m_fTexWidth;
				v1 = g_fVtxTxtCoords[dwV].y * 32 * 1024 * gRSP.fTexScaleY / tex1.m_fTexHeight;
				u1 *= gRDP.tiles[(gRSP.curTile+1)&7].fShiftScaleS;
				v1 *= gRDP.tiles[(gRSP.curTile+1)&7].fShiftScaleT;
				CRender::g_pRender->SetVertexTextureUVCoord(v, u0, v0, u1, v1);
			}
			else
			{
				CRender::g_pRender->SetVertexTextureUVCoord(v, u0, v0);
			}
		}
		else
		{
			float tex0u = g_fVtxTxtCoords[dwV].x *gRSP.tex0scaleX - gRSP.tex0OffsetX ;
			float tex0v = g_fVtxTxtCoords[dwV].y *gRSP.tex0scaleY - gRSP.tex0OffsetY ;

			if( CRender::g_pRender->IsTexel1Enable() )
			{
				float tex1u = g_fVtxTxtCoords[dwV].x *gRSP.tex1scaleX - gRSP.tex1OffsetX ;
				float tex1v = g_fVtxTxtCoords[dwV].y *gRSP.tex1scaleY - gRSP.tex1OffsetY ;

				CRender::g_pRender->SetVertexTextureUVCoord(v, tex0u, tex0v, tex1u, tex1v);
				VTX_DUMP(TRACE2("  (tex0): %f, %f",  tex0u,tex0v));
				VTX_DUMP(TRACE2("  (tex1): %f, %f",  tex1u,tex1v));
			}
			else
			{
				CRender::g_pRender->SetVertexTextureUVCoord(v, tex0u, tex0v);
				VTX_DUMP(TRACE2("  (tex0): %f, %f",  tex0u,tex0v));
			}
		}
	}

	if( g_curRomInfo.bEnableTxtLOD && vtxIndex == 1 && gRDP.otherMode.text_lod )
	{
		if( CRender::g_pRender->IsTexel1Enable() && CRender::g_pRender->m_pColorCombiner->m_pDecodedMux->isUsed(MUX_LODFRAC) )
		{
			ComputeLOD();
		}
		else
		{
			gRDP.LODFrac = 0;
		}
	}

	VTX_DUMP(TRACE2("  DIF(%08X), SPE(%08X)",   v.dcDiffuse, v.dcSpecular));
}

uint32 LightVert(v3 & norm)
{
	// Do ambient
	float r = gRSPlights[gRSP.ambientLightIndex].colour.r;
	float g = gRSPlights[gRSP.ambientLightIndex].colour.g;
	float b = gRSPlights[gRSP.ambientLightIndex].colour.b;

	for (unsigned int l=0; l < gRSPnumLights; l++)
	{
		// Regular directional light
		float fCosT = norm.Dot(gRSPlights[l].direction);
		
		if (fCosT > 0.0f)
		{
			r += gRSPlights[l].colour.fr * fCosT;
			g += gRSPlights[l].colour.fg * fCosT;
			b += gRSPlights[l].colour.fb * fCosT;
		}
	}

	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	return ((0xff000000)|(((uint32)r)<<16)|(((uint32)g)<<8)|((uint32)b));
}

uint32 LightPointVert(v4 & w)
{
	// Do ambient
	float r = gRSPlights[gRSP.ambientLightIndex].colour.r;
	float g = gRSPlights[gRSP.ambientLightIndex].colour.g;
	float b = gRSPlights[gRSP.ambientLightIndex].colour.b;

	for (unsigned int l = 0; l < gRSPnumLights; l++)
	{
		if (gRSPlights[l].SkipIfZero)
		{
			// Regular directional light
			v3 distance_vec(gRSPlights[l].Position.x - w.x, gRSPlights[l].Position.y - w.y, gRSPlights[l].Position.z - w.z);

			float light_qlen = distance_vec.LengthSq();
			float light_llen = sqrtf(light_qlen);

			float at = gRSPlights[l].ca + gRSPlights[l].la * light_llen + gRSPlights[l].qa * light_qlen;
			if (at > 0.0f)
			{
				float fCosT = 1.0f / at;
				r += gRSPlights[l].colour.fr * fCosT;
				g += gRSPlights[l].colour.fg * fCosT;
				b += gRSPlights[l].colour.fb * fCosT;
			}
		}
	}

	if (r > 255) r = 255;
	if (g > 255) g = 255;
	if (b > 255) b = 255;
	return ((0xff000000) | (((uint32)r) << 16) | (((uint32)g) << 8) | ((uint32)b));
}

inline void ReplaceAlphaWithFogFactor(int i)
{
	if( gRDP.tnl.Fog )
	{
		// Use fog factor to replace vertex alpha
		if( g_vecProjected[i].z > 1 )
			*(((uint8*)&(g_dwVtxDifColor[i]))+3) = 0xFF;
		if( g_vecProjected[i].z < 0 )
			*(((uint8*)&(g_dwVtxDifColor[i]))+3) = 0;
		else
			*(((uint8*)&(g_dwVtxDifColor[i]))+3) = (uint8)(g_vecProjected[i].z*255);	
	}
}

void ProcessVertexData(uint32 dwAddr, uint32 dwV0, uint32 dwNum)
{

	UpdateCombinedMatrix();

	// This function is called upon SPvertex
	// - do vertex matrix transform
	// - do vertex lighting
	// - do texture cooridinate transform if needed
	// - calculate normal vector

	// Output:  - g_vecProjected[i]				-> transformed vertex x,y,z
	//			- g_vecProjected[i].w						-> saved vertex 1/w
	//			- g_dwVtxFlags[i]				-> flags
	//			- g_dwVtxDifColor[i]			-> vertex color
	//			- g_fVtxTxtCoords[i]				-> vertex texture cooridinates

	FiddledVtx * pVtxBase = (FiddledVtx*)(g_pu8RamBase + dwAddr);
	g_pVtxBase = pVtxBase;

	for (uint32 i = dwV0; i < dwV0 + dwNum; i++)
	{
		FiddledVtx & vert = pVtxBase[i - dwV0];

		v4 w(float(vert.x), float(vert.y), float(vert.z), 1.0f);

		g_vtxTransformed[i] = gRSPworldProject.Transform(w);

		g_vecProjected[i].w = 1.0f / g_vtxTransformed[i].w;
		g_vecProjected[i].x = g_vtxTransformed[i].x * g_vecProjected[i].w;
		g_vecProjected[i].y = g_vtxTransformed[i].y * g_vecProjected[i].w;

		if ((g_curRomInfo.bPrimaryDepthHack || options.enableHackForGames == HACK_FOR_NASCAR) && gRDP.otherMode.depth_source)
		{
			g_vecProjected[i].z = gRDP.fPrimitiveDepth;
			g_vtxTransformed[i].z = gRDP.fPrimitiveDepth*g_vtxTransformed[i].w;
		}
		else
		{
			g_vecProjected[i].z = g_vtxTransformed[i].z * g_vecProjected[i].w;
		}

		if (gRDP.tnl.Fog)
		{
			g_fFogCoord[i] = g_vecProjected[i].z;
			if (g_vecProjected[i].w < 0 || g_vecProjected[i].z < 0 || g_fFogCoord[i] < gRSPfFogMin)
				g_fFogCoord[i] = gRSPfFogMin;
		}

		VTX_DUMP(
		{
			uint32 *dat = (uint32*)(&vert);
			DebuggerAppendMsg("vtx %d: %08X %08X %08X %08X", i, dat[0], dat[1], dat[2], dat[3]);
			DebuggerAppendMsg("      : %f, %f, %f, %f",
				g_vtxTransformed[i].x, g_vtxTransformed[i].y, g_vtxTransformed[i].z, g_vtxTransformed[i].w);
			DebuggerAppendMsg("      : %f, %f, %f, %f",
				g_vecProjected[i].x, g_vecProjected[i].y, g_vecProjected[i].z, g_vecProjected[i].w);
		});

		RSP_Vtx_Clipping(i);

		if (gRDP.tnl.Light)
		{
			
			v3 model_normal((float)vert.norm_x, (float)vert.norm_y, (float)vert.norm_z);
			v3 vecTransformedNormal;
			vecTransformedNormal = gRSPmodelViewTop.TransformNormal(model_normal);
			vecTransformedNormal.Normalise();

			if (gRDP.tnl.PointLight)
			{
				g_dwVtxDifColor[i] = LightPointVert(w);
			}
			else
			{
				g_dwVtxDifColor[i] = LightVert(vecTransformedNormal);
			}

			*(((uint8*)&(g_dwVtxDifColor[i])) + 3) = vert.rgba_a;	// still use alpha from the vertex

			if (gRDP.tnl.TexGen)
			{
				const v3 & norm = vecTransformedNormal;
				if (gRDP.tnl.TexGenLin)
				{
					g_fVtxTxtCoords[i].x = acosf(norm.x) / 3.14f;
					g_fVtxTxtCoords[i].y = acosf(norm.y) / 3.14f;
				}
				else
				{
					g_fVtxTxtCoords[i].x = 0.5f * (1.0f + norm.x);
					g_fVtxTxtCoords[i].y = 0.5f * (1.0f - norm.y);
				}
			}
			else
			{
				g_fVtxTxtCoords[i].x = (float)vert.tu;
				g_fVtxTxtCoords[i].y = (float)vert.tv;
			}
		}
		else
		{
			if ((gRDP.tnl.Shade) == 0)	//Shade is disabled
			{
				//FLAT shade
				g_dwVtxDifColor[i] = gRDP.primitiveColor;
			}
			else
			{
				g_dwVtxDifColor[i] = COLOR_RGBA(vert.rgba_r, vert.rgba_g, vert.rgba_b, vert.rgba_a);
			}
			g_fVtxTxtCoords[i].x = (float)vert.tu;
			g_fVtxTxtCoords[i].y = (float)vert.tv;
		}

		if (options.bWinFrameMode)
		{
			g_dwVtxDifColor[i] = COLOR_RGBA(vert.rgba_r, vert.rgba_g, vert.rgba_b, vert.rgba_a);
		}

		ReplaceAlphaWithFogFactor(i);

		// Update texture coords n.b. need to divide tu/tv by bogus scale on addition to buffer

	}

	VTX_DUMP(TRACE2("Setting Vertexes: %d - %d\n", dwV0, dwV0 + dwNum - 1));
	DEBUGGER_PAUSE_AND_DUMP(NEXT_VERTEX_CMD, { TRACE0("Paused at Vertex Cmd"); });
}

bool PrepareTriangle(uint32 dwV0, uint32 dwV1, uint32 dwV2)
{
	bool textureFlag = (CRender::g_pRender->IsTextureEnabled());

	InitVertex(dwV0, gRSP.numVertices, textureFlag);
	InitVertex(dwV1, gRSP.numVertices+1, textureFlag);
	InitVertex(dwV2, gRSP.numVertices+2, textureFlag);

	gRSP.numVertices += 3;
	status.dwNumTrisRendered++;
	return true;
}

bool AddTri(u32 v0, u32 v1, u32 v2, bool bTri4)
{
	if(bTri4 && v0 == v1)
		return false; // Cull empty tris

	if (IsTriangleVisible(v0, v1, v2))
	{
		if (CRender::g_pRender->IsTextureEnabled())
		{
			PrepareTextures();
			InitVertexTextureConstants();
		}

		CRender::g_pRender->SetCombinerAndBlender();

		PrepareTriangle(v0, v1, v2);
		return true;
	}
	return false;
}

// Returns TRUE if it thinks the triangle is visible
// Returns FALSE if it is clipped
bool IsTriangleVisible(uint32 dwV0, uint32 dwV1, uint32 dwV2)
{
	DEBUGGER_ONLY_IF( (!debuggerEnableTestTris || !debuggerEnableCullFace), {return TRUE;});
    
#ifdef _DEBUG
	// Check vertices are valid!
	if (dwV0 >= MAX_VERTS || dwV1 >= MAX_VERTS || dwV2 >= MAX_VERTS)
		return false;
#endif

	// Here we AND all the flags. If any of the bits is set for all
	// 3 vertices, it means that all three x, y or z lie outside of
	// the current viewing volume.
	// Currently disabled - still seems a bit dodgy
	if (gRDP.tnl.TriCull)
	{
		v4 & v0 = g_vecProjected[dwV0];
		v4 & v1 = g_vecProjected[dwV1];
		v4 & v2 = g_vecProjected[dwV2];

		// Only try to clip if the tri is onscreen. For some reason, this
		// method doesnt' work well when the z value is outside of screenspace
		//if (v0.z < 1 && v1.z < 1 && v2.z < 1)
		{
			float V1 = v2.x - v0.x;
			float V2 = v2.y - v0.y;
			
			float W1 = v2.x - v1.x;
			float W2 = v2.y - v1.y;

			float fDirection = (V1 * W2) - (V2 * W1);
			fDirection = fDirection * v1.w * v2.w * v0.w;

			if (fDirection <= 0.0f)
			{
				if (gRDP.tnl.CullBack)
				{
					status.dwNumTrisClipped++;
					return false;
				}
			}
			else if (!gRDP.tnl.CullBack)
			{
				status.dwNumTrisClipped++;
				return false;
			}
		}
	}
	
#ifdef ENABLE_CLIP_TRI
	//if( gRSP.bRejectVtx && (g_clipFlag[dwV0]|g_clipFlag[dwV1]|g_clipFlag[dwV2]) )	
	//	return;
	if( g_clipFlag2[dwV0]&g_clipFlag2[dwV1]&g_clipFlag2[dwV2] )
	{
		//DebuggerAppendMsg("Clipped");
		return false;
	}
#endif

	return true;
}


void SetPrimitiveColor(uint32 dwCol, uint32 LODMin, uint32 LODFrac)
{
	gRDP.colorsAreReloaded = true;
	gRDP.primitiveColor = dwCol;
	gRDP.primLODMin = LODMin;
	gRDP.primLODFrac = LODFrac;
	if( gRDP.primLODFrac < gRDP.primLODMin )
	{
		gRDP.primLODFrac = gRDP.primLODMin;
	}

	gRDP.fvPrimitiveColor[0] = ((dwCol>>16)&0xFF)/255.0f;		//r
	gRDP.fvPrimitiveColor[1] = ((dwCol>>8)&0xFF)/255.0f;		//g
	gRDP.fvPrimitiveColor[2] = ((dwCol)&0xFF)/255.0f;			//b
	gRDP.fvPrimitiveColor[3] = ((dwCol>>24)&0xFF)/255.0f;		//a
}

void SetPrimitiveDepth(uint32 z, uint32 dwDZ)
{
	gRDP.primitiveDepth = z&0x7FFF;
	//if( gRDP.primitiveDepth != 0 && gRDP.primitiveDepth != 0xFFFF ) 
	{
		gRDP.fPrimitiveDepth = (float)(gRDP.primitiveDepth)/(float)0x8000;

		//gRDP.fPrimitiveDepth = gRDP.fPrimitiveDepth*2-1;	
		/*
		z=0xFFFF	->	1	the farest
		z=0			->	-1	the nearest
		*/
	}

	//how to use dwDZ?

#ifdef _DEBUG
	if( (pauseAtNext && (eventToPause == NEXT_VERTEX_CMD || eventToPause == NEXT_FLUSH_TRI )) )//&& logTriangles ) 
	{
		DebuggerAppendMsg("Set prim Depth: %f, (%08X, %08X)", gRDP.fPrimitiveDepth, z, dwDZ); 
	}
#endif
}

void SetVertexXYZ(uint32 vertex, float x, float y, float z)
{
	g_vecProjected[vertex].x = x;
	g_vecProjected[vertex].y = y;
	g_vecProjected[vertex].z = z;

	g_vtxTransformed[vertex].x = x*g_vtxTransformed[vertex].w;
	g_vtxTransformed[vertex].y = y*g_vtxTransformed[vertex].w;
	g_vtxTransformed[vertex].z = z*g_vtxTransformed[vertex].w;
}

void ModifyVertexInfo(uint32 where, uint32 vertex, uint32 val)
{
	switch (where)
	{
	case RSP_MV_WORD_OFFSET_POINT_RGBA:		// Modify RGBA
		{
			uint32 r = (val>>24)&0xFF;
			uint32 g = (val>>16)&0xFF;
			uint32 b = (val>>8)&0xFF;
			uint32 a = val&0xFF;
			g_dwVtxDifColor[vertex] = COLOR_RGBA(r, g, b, a);
			LOG_UCODE("Modify vert %d color, 0x%08x", vertex, g_dwVtxDifColor[vertex]);
		}
		break;
	case RSP_MV_WORD_OFFSET_POINT_XYSCREEN:		// Modify X,Y
		{
			uint16 x = (uint16)(val>>16) / 4;
			uint16 y = (uint16)(val & 0xFFFF) / 4;
			// Should do viewport transform

			x -= windowSetting.uViWidth/2;
			y = windowSetting.uViHeight/2-y;

			if( options.bEnableHacks && ((*g_GraphicsInfo.VI_X_SCALE_REG)&0xF) != 0 )
			{
				// Tarzan
				// I don't know why Tarzan is different
				SetVertexXYZ(vertex, x/windowSetting.fViWidth, y/windowSetting.fViHeight, g_vecProjected[vertex].z);
			}
			else
			{
				// Toy Story 2 and other games
				SetVertexXYZ(vertex, x*2/windowSetting.fViWidth, y*2/windowSetting.fViHeight, g_vecProjected[vertex].z);
			}

			LOG_UCODE("Modify vert %d: x=%d, y=%d", vertex, x, y);
			VTX_DUMP(TRACE3("Modify vert %d: (%d,%d)", vertex, x, y));
		}
		break;
	case RSP_MV_WORD_OFFSET_POINT_ZSCREEN:		// Modify C
		{
			int z = val>>16;

			SetVertexXYZ(vertex, g_vecProjected[vertex].x, g_vecProjected[vertex].y, (((float)z/0x03FF)+0.5f)/2.0f );
			LOG_UCODE("Modify vert %d: z=%d", vertex, z);
			VTX_DUMP(TRACE2("Modify vert %d: z=%d", vertex, z));
		}
		break;
	case RSP_MV_WORD_OFFSET_POINT_ST:		// Texture
		{
			short tu = short(val>>16);
			short tv = short(val & 0xFFFF);
			float ftu = tu / 32.0f;
			float ftv = tv / 32.0f;
			LOG_UCODE("      Setting vertex %d tu/tv to %f, %f", vertex, (float)tu, (float)tv);
			CRender::g_pRender->SetVtxTextureCoord(vertex, ftu/gRSP.fTexScaleX, ftv/gRSP.fTexScaleY);
		}
		break;
	default:
	//	RSP_RDP_NOIMPL("RSP_GBI1_ModifyVtx: Setting unk value: 0x%02x, 0x%08x", dwWhere, dwValue);
		break;
	}
	DEBUGGER_PAUSE_AND_DUMP(NEXT_VERTEX_CMD,{TRACE0("Paused at ModVertex Cmd");});
}

extern u32 gDKRCMatrixIndex;
extern u32 gDKRVtxCount;
extern bool gDKRBillBoard;

void ProcessVertexDataDKR(uint32 dwAddr, uint32 dwV0, uint32 dwNum)
{
	UpdateCombinedMatrix();

	uint32 pVtxBase = uint32(g_pu8RamBase + dwAddr);
	g_pVtxBase = (FiddledVtx*)pVtxBase;

	Matrix4x4 &matWorldProject(gRSP.modelviewMtxs[gDKRCMatrixIndex]);

	uint32 i;

	bool addbase=false;
	if ((!gDKRBillBoard) || (gDKRCMatrixIndex != 2) )
		addbase = false;
	else
		addbase = true;

	if( addbase && gDKRVtxCount == 0 && dwNum > 1 )
	{
		gDKRVtxCount++;
	}

	LOG_UCODE("    ProcessVertexDataDKR, CMatrix = %d, Add base=%s", gDKRCMatrixIndex, gDKRBillBoard?"true":"false");
	VTX_DUMP(TRACE2("DKR Setting Vertexes\nCMatrix = %d, Add base=%s", gDKRCMatrixIndex, gDKRBillBoard?"true":"false"));

	uint32 end = dwV0 + dwNum;
	for (i = dwV0; i < end; i++)
	{
		v4 w;

		w.x = (float)*(short*)((pVtxBase + 0) ^ 2);
		w.y = (float)*(short*)((pVtxBase + 2) ^ 2);
		w.z = (float)*(short*)((pVtxBase + 4) ^ 2);
		w.w = 1.0f;

		g_vtxTransformed[i] = matWorldProject.Transform(w);

		if( gDKRVtxCount == 0 && dwNum==1 )
		{
			gRSP.DKRBaseVec.x = g_vtxTransformed[i].x;
			gRSP.DKRBaseVec.y = g_vtxTransformed[i].y;
			gRSP.DKRBaseVec.z = g_vtxTransformed[i].z;
			gRSP.DKRBaseVec.w = g_vtxTransformed[i].w;
		}
		else if( addbase )
		{
			g_vtxTransformed[i].x += gRSP.DKRBaseVec.x;
			g_vtxTransformed[i].y += gRSP.DKRBaseVec.y;
			g_vtxTransformed[i].z += gRSP.DKRBaseVec.z;
			g_vtxTransformed[i].w  = gRSP.DKRBaseVec.w;
		}

		g_vecProjected[i].w = 1.0f / g_vtxTransformed[i].w;
		g_vecProjected[i].x = g_vtxTransformed[i].x * g_vecProjected[i].w;
		g_vecProjected[i].y = g_vtxTransformed[i].y * g_vecProjected[i].w;
		g_vecProjected[i].z = g_vtxTransformed[i].z * g_vecProjected[i].w;

		gDKRVtxCount++;

		VTX_DUMP(TRACE5("vtx %d: %f, %f, %f, %f", i, 
			g_vtxTransformed[i].x,g_vtxTransformed[i].y,g_vtxTransformed[i].z,g_vtxTransformed[i].w));

		if( gRDP.tnl.Fog )
		{
			g_fFogCoord[i] = g_vecProjected[i].z;
			if( g_vecProjected[i].w < 0 || g_vecProjected[i].z < 0 || g_fFogCoord[i] < gRSPfFogMin )
				g_fFogCoord[i] = gRSPfFogMin;
		}

		RSP_Vtx_Clipping(i);

		short wA = *(short*)((pVtxBase + 6) ^ 2);
		short wB = *(short*)((pVtxBase + 8) ^ 2);

		// Assign true vert colour after lighting/fogging
		g_dwVtxDifColor[i] = COLOR_RGBA((s8)(wA >> 8), (s8)(wA), (s8)(wB >> 8), (s8)(wB));

		ReplaceAlphaWithFogFactor(i);

		g_fVtxTxtCoords[i].x = g_fVtxTxtCoords[i].y = 1;

		pVtxBase += 10;
	}


	DEBUGGER_PAUSE_AND_DUMP(NEXT_VERTEX_CMD,{DebuggerAppendMsg("Paused at DKR Vertex Cmd, v0=%d, vn=%d, addr=%08X", dwV0, dwNum, dwAddr);});
}

extern uint32 dwPDCIAddr;
void ProcessVertexDataPD(uint32 dwAddr, uint32 dwV0, uint32 dwNum)
{
	UpdateCombinedMatrix();

	N64VtxPD * pVtxBase = (N64VtxPD*)(g_pu8RamBase + dwAddr);
	g_pVtxBase = (FiddledVtx*)pVtxBase;	// Fix me

	for (uint32 i = dwV0; i < dwV0 + dwNum; i++)
	{
		N64VtxPD &vert = pVtxBase[i - dwV0];
		v4 w(float(vert.x), float(vert.y), (float)vert.z, 1.0f);

		g_vtxTransformed[i] = gRSPworldProject.Transform(w);

		g_vecProjected[i].w = 1.0f / g_vtxTransformed[i].w;
		g_vecProjected[i].x = g_vtxTransformed[i].x * g_vecProjected[i].w;
		g_vecProjected[i].y = g_vtxTransformed[i].y * g_vecProjected[i].w;
		g_vecProjected[i].z = g_vtxTransformed[i].z * g_vecProjected[i].w;

		g_fFogCoord[i] = g_vecProjected[i].z;
		if( g_vecProjected[i].w < 0 || g_vecProjected[i].z < 0 || g_fFogCoord[i] < gRSPfFogMin )
			g_fFogCoord[i] = gRSPfFogMin;

		RSP_Vtx_Clipping(i);

		uint8 *addr = g_pu8RamBase+dwPDCIAddr+ (vert.cidx&0xFF);
		uint32 a = addr[0];
		uint32 r = addr[3];
		uint32 g = addr[2];
		uint32 b = addr[1];

		if( gRDP.tnl.Light )
		{
			v3 model_normal((char)r, (char)g, (char)b);

			v3 vecTransformedNormal;
			vecTransformedNormal = gRSPworldProject.TransformNormal(model_normal);
			vecTransformedNormal.Normalise();

			g_dwVtxDifColor[i] = LightVert(vecTransformedNormal);

			*(((uint8*)&(g_dwVtxDifColor[i]))+3) = (uint8)a;	// still use alpha from the vertex
		}
		else
		{
			if( (gRDP.tnl.Shade) == 0)	//Shade is disabled
			{
				g_dwVtxDifColor[i] = gRDP.primitiveColor;
			}
			else	//FLAT shade
			{
				g_dwVtxDifColor[i] = COLOR_RGBA(r, g, b, a);
			}
		}

		if( options.bWinFrameMode )
		{
			g_dwVtxDifColor[i] = COLOR_RGBA(r, g, b, a);
		}

		ReplaceAlphaWithFogFactor(i);

		v2 & t = g_fVtxTxtCoords[i];
		if (gRDP.tnl.TexGen && gRDP.tnl.Light )
		{
			//TexGen(g_fVtxTxtCoords[i].x, g_fVtxTxtCoords[i].y); BACKTOMEANDFIXME
		}
		else
		{
			t.x = vert.s;
			t.y = vert.t; 
		}


		VTX_DUMP( 
		{
			uint32 *dat = (uint32*)(&vert);
			DebuggerAppendMsg("vtx %d: %d %d %d", i, vert.x,vert.y,vert.z); 
			DebuggerAppendMsg("      : %f, %f, %f, %f", 
				g_vtxTransformed[i].x,g_vtxTransformed[i].y,g_vtxTransformed[i].z,g_vtxTransformed[i].w);
			DebuggerAppendMsg("      : %X, %X, %X, %X", r,g,b,a);
			DebuggerAppendMsg("      : u=%f, v=%f", t.x, t.y);
		});
	}

	VTX_DUMP(TRACE2("Setting Vertexes: %d - %d\n", dwV0, dwV0+dwNum-1));
	DEBUGGER_PAUSE_AND_DUMP(NEXT_VERTEX_CMD,{TRACE0("Paused at Vertex Cmd");});
}

extern uint32 dwConkerVtxZAddr;
#define N64COL_GETR( col )		(uint8((col) >> 24))
#define N64COL_GETG( col )		(uint8((col) >> 16))
#define N64COL_GETB( col )		(uint8((col) >>  8))
#define N64COL_GETA( col )		(uint8((col)      ))

void ProcessVertexDataConker(uint32 dwAddr, uint32 dwV0, uint32 dwNum)
{
	UpdateCombinedMatrix();

	FiddledVtx * pVtxBase = (FiddledVtx*)(g_pu8RamBase + dwAddr);
	g_pVtxBase = pVtxBase;

	short *mn = (short*)(g_pu8RamBase + dwConkerVtxZAddr);

	uint32 i;
	for (i = dwV0; i < dwV0 + dwNum; i++)
	{
		FiddledVtx & vert = pVtxBase[i - dwV0];

		v4 w(float(vert.x), float(vert.y), (float)vert.z, 1.0f);

		g_vtxTransformed[i] = gRSPworldProject.Transform(w);

		g_vecProjected[i].w = 1.0f / g_vtxTransformed[i].w;
		g_vecProjected[i].x = g_vtxTransformed[i].x * g_vecProjected[i].w;
		g_vecProjected[i].y = g_vtxTransformed[i].y * g_vecProjected[i].w;
		g_vecProjected[i].z = g_vtxTransformed[i].z * g_vecProjected[i].w;

		g_fFogCoord[i] = g_vecProjected[i].z;

		if (g_vecProjected[i].w < 0 || g_vecProjected[i].z < 0 || g_fFogCoord[i] < gRSPfFogMin)
			g_fFogCoord[i] = gRSPfFogMin;

		//Initialize clipping flags for vertexs
		RSP_Vtx_Clipping(i);

		if (gRDP.tnl.Light)
		{
			uint32 r = gRSPlights[gRSP.ambientLightIndex].colour.r;
			uint32 g = gRSPlights[gRSP.ambientLightIndex].colour.g;
			uint32 b = gRSPlights[gRSP.ambientLightIndex].colour.b;

			v3 model_normal(mn[((i << 1) + 0) ^ 3], mn[((i << 1) + 1) ^ 3], vert.normz);
			v3 vecTransformedNormal = gRSPworldProject.TransformNormal(model_normal);
			const v3 & norm = vecTransformedNormal;

			v4 Pos;
			Pos.x = (g_vecProjected[i].x + gRDP.CoordMod[8]) * gRDP.CoordMod[12];
			Pos.y = (g_vecProjected[i].y + gRDP.CoordMod[9]) * gRDP.CoordMod[13];
			Pos.z = (g_vecProjected[i].z + gRDP.CoordMod[10])* gRDP.CoordMod[14];
			Pos.w = (g_vecProjected[i].w + gRDP.CoordMod[11])* gRDP.CoordMod[15];

			float fCosT;
			uint32 k;

			if (gRDP.tnl.PointLight)
			{
				for (k = 0; k < gRSPnumLights - 1; k++)
				{
					if (gRSPlights[k].SkipIfZero)
					{
						fCosT = norm.Dot(gRSPlights[k].direction);
						if (fCosT > 0.0f)
						{
							float pi = gRSPlights[k].Iscale /(Pos - gRSPlights[k].Position).LengthSq();
							
							if (pi < 1.0f)
								fCosT *= pi;

							r += gRSPlights[k].colour.r *fCosT;
							g += gRSPlights[k].colour.g *fCosT;
							b += gRSPlights[k].colour.b *fCosT;
						}
					}
				}

				fCosT = norm.Dot(gRSPlights[k].direction);
				if (fCosT > 0.0f)
				{
					r += gRSPlights[k].colour.r *fCosT;
					g += gRSPlights[k].colour.g *fCosT;
					b += gRSPlights[k].colour.b *fCosT;
				}
			}
			else
			{
				for (uint32 k = 0; k < gRSPnumLights; k++)
				{
					if (gRSPlights[k].SkipIfZero)
					{
						float pi = gRSPlights[k].Iscale / (Pos - gRSPlights[k].Position).LengthSq();
						if (pi > 1.0f) pi = 1.0f;

						r += gRSPlights[k].colour.r *pi;
						g += gRSPlights[k].colour.g *pi;
						b += gRSPlights[k].colour.b *pi;
					}
				}
			}

			if (r>255)
				r = 255;
			if (g>255)
				g = 255;
			if (b>255)
				b = 255;
			r *= vert.rgba_r;
			g *= vert.rgba_g;
			b *= vert.rgba_b;
			r >>= 8;
			g >>= 8;
			b >>= 8;
			g_dwVtxDifColor[i] = 0xFF000000;
			g_dwVtxDifColor[i] |= (r << 16);
			g_dwVtxDifColor[i] |= (g << 8);
			g_dwVtxDifColor[i] |= (b);

			*(((uint8*)&(g_dwVtxDifColor[i])) + 3) = vert.rgba_a;	// still use alpha from the vertex
			//TEXTURE

			// ENV MAPPING
			if (gRDP.tnl.TexGen)
			{
				const v3 & norm = vecTransformedNormal;
				if (gRDP.tnl.TexGenLin)
				{
					g_fVtxTxtCoords[i].x = acosf(norm.x) / 3.14f;
					g_fVtxTxtCoords[i].y = acosf(norm.y) / 3.14f;
				}
				else
				{
					g_fVtxTxtCoords[i].x = 0.5f * (1.0f + norm.x);
					g_fVtxTxtCoords[i].y = 0.5f * (1.0f - norm.y);
				}
			}
			else
			{
				g_fVtxTxtCoords[i].x = (float)vert.tu;
				g_fVtxTxtCoords[i].y = (float)vert.tv;
			}
		}
		else
		{
			g_fVtxTxtCoords[i].x = (float)vert.tu;
			g_fVtxTxtCoords[i].y = (float)vert.tv;
		}

		if (options.bWinFrameMode)
		{
			//g_vecProjected[i].z = 0;
			g_dwVtxDifColor[i] = COLOR_RGBA(vert.rgba_r, vert.rgba_g, vert.rgba_b, vert.rgba_a);
		}

		ReplaceAlphaWithFogFactor(i);

	}

	VTX_DUMP(TRACE2("Setting Vertexes: %d - %d\n", dwV0, dwV0 + dwNum - 1));
	DEBUGGER_PAUSE_AND_DUMP(NEXT_VERTEX_CMD, { DebuggerAppendMsg("Paused at Vertex Cmd"); });
}

void SetLightCol(uint32 dwLight, u8 r, u8 g, u8 b)
{
	
	gRSPlights[dwLight].SkipIfZero = (r + g + b);
	gRSPlights[dwLight].colour.r = r;
	gRSPlights[dwLight].colour.g = g;
	gRSPlights[dwLight].colour.b = b;
	gRSPlights[dwLight].colour.a = 255;	// Ignore light alpha
	gRSPlights[dwLight].colour.fr = (float)gRSPlights[dwLight].colour.r;
	gRSPlights[dwLight].colour.fg = (float)gRSPlights[dwLight].colour.g;
	gRSPlights[dwLight].colour.fb = (float)gRSPlights[dwLight].colour.b;
	gRSPlights[dwLight].colour.fa = 255;	// Ignore light alpha

	//TRACE1("Set light %d color", dwLight);
//	LIGHT_DUMP(TRACE2("Set Light %d color: %08X", dwLight, dwCol));
}

void SetLightDirection(uint32 dwLight, float x, float y, float z)
{
	float w = sqrt(x*x + y*y + z*z);

	gRSPlights[dwLight].direction.x = x/w;
	gRSPlights[dwLight].direction.y = y/w;
	gRSPlights[dwLight].direction.z = z/w;

	DEBUGGER_PAUSE_AND_DUMP(NEXT_SET_LIGHT,TRACE4("Set Light %d dir: %.4f, %.4f, %.4f", dwLight, x, y, z));
}

void SetLightPosition(uint32 dwLight, float x, float y, float z, float w)
{
	gRSPlights[dwLight].Position.x = x;
	gRSPlights[dwLight].Position.y = y;
	gRSPlights[dwLight].Position.z = z;
	gRSPlights[dwLight].Position.w = w;
}

void SetLightCBFD(uint32 dwLight, short nonzero)
{
	gRSPlights[dwLight].Iscale = (float)(nonzero << 12);
	gRSPlights[dwLight].SkipIfZero = gRSPlights[dwLight].SkipIfZero&&nonzero;
}


void SetLightEx(uint32 dwLight, float ca, float la, float qa)
{
	gRSPlights[dwLight].ca = ca / 16.0f;
	gRSPlights[dwLight].la = la / 65535.0f;
	gRSPlights[dwLight].qa = qa / (8.0f*65535.0f);
}

void ForceMainTextureIndex(int dwTile) 
{
	if( dwTile == 1 && !(CRender::g_pRender->IsTexel0Enable()) && CRender::g_pRender->IsTexel1Enable() )
	{
		// Hack
		gRSP.curTile = 0;
	}
	else
		gRSP.curTile = dwTile;
}

//CheckMe
float HackZ2(float z)
{
	z = (z+9)/10;
	return z;
}
//CheckMe
float HackZ(float z)
{
	return HackZ2(z);

	if( z < 0.1 && z >= 0 )
		z = (.1f+z)/2;
	else if( z < 0 )
		//return (10+z)/100;
		z = (expf(z)/20);
	return z;
}
//CheckMe
void HackZ(std::vector<v3>& points)
{
	int size = points.size();
	for( int i=0; i<size; i++)
	{
		v3 &v = points[i];
		v.z = (float)HackZ(v.z);
	}
}
//CheckMe
void HackZAll()
{
	for( uint32 i=0; i<gRSP.numVertices; i++)
	{
		g_vtxBuffer[i].z = HackZ(g_vtxBuffer[i].z);
	}
}


extern Matrix4x4 reverseXY;
extern Matrix4x4 reverseY;

void UpdateCombinedMatrix()
{
	if( gRSP.bMatrixIsUpdated )
	{
		gRSPworldProject = gRSP.modelviewMtxs[gRSP.modelViewMtxTop] * gRSP.projectionMtxs[gRSP.projectionMtxTop];
		gRSP.bMatrixIsUpdated = false;
	}

}

