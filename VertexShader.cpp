/*
Copyright (C) 2005 Rice1964

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
#include "VertexShaderConstantDef.h"

// Vertex data definition

// Initialize vertex shader

// Turn vertex shader on/off

// Define the vertex shader, assemble it and create it
extern FiddledVtx* g_pVtxBase;
extern void TexGen(float &s, float &t);

void ProcessVertexDataExternal(uint32 dwAddr, uint32 dwV0, uint32 dwNum)
{
	FiddledVtx * g_pVtxBase = (FiddledVtx*)(g_pRDRAMu8 + dwAddr);
	uint32 i;
	for (i = dwV0; i < dwV0 + dwNum; i++)
	{
		FiddledVtx & vert = g_pVtxBase[i - dwV0];

		g_vtxForExternal[i].x = (float)vert.x;
		g_vtxForExternal[i].y = (float)vert.y;
		g_vtxForExternal[i].z = (float)vert.z;

		g_vtxForExternal[i].nx = (float)vert.norma.nx/128.0f;
		g_vtxForExternal[i].ny = (float)vert.norma.ny/128.0f;
		g_vtxForExternal[i].nz = (float)vert.norma.nz/128.0f;

		g_vtxForExternal[i].r = vert.rgba.r;
		g_vtxForExternal[i].g = vert.rgba.g;
		g_vtxForExternal[i].b = vert.rgba.b;
		g_vtxForExternal[i].a = vert.rgba.a;

		if (gRSP.bTextureGen && gRSP.bLightingEnable )
		{
			TexGen(g_vtxForExternal[i].u, g_vtxForExternal[i].v);
		}
		else
		{
			g_vtxForExternal[i].u = (float)vert.tu;
			g_vtxForExternal[i].v = (float)vert.tv; 
		}
	}
}

void UpdateVertexShaderConstant()
{
	//if( gRSP.bMatrixIsUpdated || gRSP.bCombinedMatrixIsUpdated )
	{
		//HRESULT res = g_pD3DDev->SetVertexShader( gVertexShader );
		UpdateCombinedMatrix();
		g_pD3DDev->SetVertexShaderConstant( CV_WORLDVIEWPROJ_0, (float*)&gRSPworldProjectTransported, 4 );
		g_pD3DDev->SetVertexShaderConstant( CV_WORLDVIEW_0, (float*)&gRSPmodelViewTopTranspose, 4 );
	}
}

void InitVertexShaderConstants()
{
	float f1[]={0.0f,0.0f,0.0f,0.0f};

	for( int i=0; i<90; i++ )
	{
		// Initialize all constant to 0
		g_pD3DDev->SetVertexShaderConstant( i, (float*)&f1, 1 );
	}

	g_pD3DDev->SetVertexShaderConstant( CV_ZERO, (float*)&f1, 1 );

	float f2[]={1.0f,1.0f,1.0f,1.0f};
	g_pD3DDev->SetVertexShaderConstant( CV_ONE, (float*)&f2, 1 );

	float f3[]={0.5f,0.5f,0.5f,0.5f};
	g_pD3DDev->SetVertexShaderConstant( CV_HALF, (float*)&f3, 1 );

	float f4[]={0.1f,0.1f,0.1f,0.1f};
	g_pD3DDev->SetVertexShaderConstant( CV_TENTH, (float*)&f4, 1 );

	float f5[]={0.9f,0.9f,0.9f,0.9f};
	g_pD3DDev->SetVertexShaderConstant( CV_NINE_TENTH, (float*)&f5, 1 );

	float f6[]={200.0f,200.0f,200.0f,200.0f};
	g_pD3DDev->SetVertexShaderConstant( CV_200, (float*)&f6, 1 );

}

void UpdateOptionsForVertexShader(float halfS, float halfT)
{
	float f0[]={0.0f,0.0f,0.0f,0.0f};
	float f1[]={1.0f,1.0f,1.0f,1.0f};

	float (*pf)[4];

	pf = gRSP.bFogEnabled ? &f1 : &f0;
	g_pD3DDev->SetVertexShaderConstant( FOG_IS_ENABLED, (float*)pf, 1 );

	pf = gRSP.bLightingEnable ? &f1 : &f0;
	g_pD3DDev->SetVertexShaderConstant( LIGHTING_ENABLED, (float*)pf, 1 );

	if(  gRDP.otherMode.key_en )
	{
		g_pD3DDev->SetVertexShaderConstant( FORCE_VTX_ALPHA, (float*)&f1, 1 );
		float f2[]={gRDP.keyA/255.0f,gRDP.keyA/255.0f,gRDP.keyA/255.0f,gRDP.keyA/255.0f};
		g_pD3DDev->SetVertexShaderConstant( VTX_ALPHA, (float*)&f2, 1 );
	}
	else if( gRDP.otherMode.aa_en && gRDP.otherMode.clr_on_cvg==0 )
	{
		g_pD3DDev->SetVertexShaderConstant( FORCE_VTX_ALPHA, (float*)&f1, 1 );
		g_pD3DDev->SetVertexShaderConstant( VTX_ALPHA, (float*)&f1, 1 );
	}
	else
		g_pD3DDev->SetVertexShaderConstant( FORCE_VTX_ALPHA, (float*)&f0, 1 );

	pf = g_curRomInfo.bZHack ? &f1 : &f0;
	g_pD3DDev->SetVertexShaderConstant( Z_HACK_ENABLE, (float*)pf, 1 );

	pf = ( (gRDP.geometryMode & G_SHADE) == 0 && gRSP.ucode < 5 ) ? &f1 : &f0;
	g_pD3DDev->SetVertexShaderConstant( USE_PRIMARY_COLOR, (float*)pf, 1 );

	if( (g_curRomInfo.bPrimaryDepthHack || options.enableHackForGames == HACK_FOR_NASCAR ) && gRDP.otherMode.depth_source )
		pf = &f1;
	else
		pf = &f0;
	g_pD3DDev->SetVertexShaderConstant( USE_PRIMARY_DEPTH, (float*)pf, 1 );


	// Fog
	float f = -gRSPfFogDivider/255.0f;
	float f6[]={f,f,f,f};
	g_pD3DDev->SetVertexShaderConstant( FOG_MUL, (float*)&f6, 1 );

	f = 1.0f + gRSPfFogMin * gRSPfFogDivider / 255.0f;
	float f7[]={f,f,f,f};
	g_pD3DDev->SetVertexShaderConstant( FOG_ADD, (float*)&f6, 1 );


	// Texture
	if( CRender::g_pRender->IsTextureEnabled() || gRSP.ucode == 6 )
	{
		float scale0x, scale1x=0, offset0x, offset1x=0;
		float scale0y, scale1y=0, offset0y, offset1y=0;
		if (gRSP.bTextureGen && gRSP.bLightingEnable)
		{
			RenderTexture &tex0 = g_textures[gRSP.curTile];
			scale0x = 32 * 1024 * gRSP.fTexScaleX / tex0.m_fTexWidth * (gRDP.tiles[gRSP.curTile].fShiftScaleS);
			scale0y = 32 * 1024 * gRSP.fTexScaleY / tex0.m_fTexHeight * (gRDP.tiles[gRSP.curTile].fShiftScaleT);
			if( CRender::g_pRender->IsTexel1Enable() )
			{
				RenderTexture &tex1 = g_textures[(gRSP.curTile+1)&7];
				scale1x = 32 * 1024 * gRSP.fTexScaleX / tex1.m_fTexWidth * (gRDP.tiles[(gRSP.curTile+1)&7].fShiftScaleS);
				scale1y = 32 * 1024 * gRSP.fTexScaleY / tex1.m_fTexHeight * (gRDP.tiles[(gRSP.curTile+1)&7].fShiftScaleT);
			}
			else
			{
				scale1x = scale1y = 0.0f;
			}

			offset0x = offset0y = 0.0f;
			offset1x = offset1y = 0.0f;
		}
		else
		{
			scale0x = gRSP.tex0scaleX;
			scale0y = gRSP.tex0scaleY;
			offset0x = -gRSP.tex0OffsetX;
			offset0y = -gRSP.tex0OffsetY;
			if( CRender::g_pRender->IsTexel1Enable() )
			{
				scale0x = gRSP.tex1scaleX;
				scale0y = gRSP.tex1scaleY;
				offset0x = -gRSP.tex1OffsetX;
				offset0y = -gRSP.tex1OffsetY;
			}
			else
			{
				scale1x = scale1y = 0.0f;
				offset1x = offset1y = 0.0f;
			}
		}

		float scales0[4] = {scale0x*halfS,scale0y*halfT,0,0};
		float scales1[4] = {scale1x*halfS,scale1y*halfT,0,0};
		float offset0[4] = {offset0x,offset0y,0,0};
		float offset1[4] = {offset1x,offset1y,0,0};

		g_pD3DDev->SetVertexShaderConstant( T0_SCALE_X_Y, (float*)&scales0, 1 );
		g_pD3DDev->SetVertexShaderConstant( T1_SCALE_X_Y, (float*)&scales1, 1 );
		g_pD3DDev->SetVertexShaderConstant( T0_OFFSET_X_Y, (float*)&offset0, 1 );
		g_pD3DDev->SetVertexShaderConstant( T1_OFFSET_X_Y, (float*)&offset1, 1 );
	}

}

char *shaderstr = 
"; Complex vertex shader to emulate N64 pipeline\n"\
"\n"\
"vs.1.1\n"\
"\n"\
"; Transform position to clip space and output it\n"\
"dp4 r7.x, v0, c[10]\n"\
"dp4 r7.y, v0, c[11]\n"\
"dp4 r7.z, v0, c[12]\n"\
"dp4 r7.w, v0, c[13]\n"\
"\n"\
"; Lighting\n"\
"; Transform normal to eye space\n"\
"dp3 r1.x, v1, c[14]\n"\
"dp3 r1.y, v1, c[15]\n"\
"dp3 r1.z, v1, c[16]\n"\
"\n"\
"; Normalize transformed normal\n"\
"dp3 r1.w, r1, r1\n"\
"rsq r1.w, r1.w\n"\
"mul r1, r1, r1.w\n"\
"\n"\
"; ** Light 0, the ambient light\n"\
"mov r2, c[20]\n"\
"\n"\
"; ** Light 1\n"\
"; Dot normal with light vector\n"\
"; This is the intensity of the diffuse component\n"\
"mov r3, c[0]\n"\
"dp3 r3.x, r1, c[30]\n"\
"; Calculate the diffuse & specular factors\n"\
"lit r4, r3\n"\
"\n"\
"; add the (diffuse color * diffuse light color * diffuse intensity(R4.y))\n"\
"mad r2, r4.y, c[21], r2\n"\
"\n"\
"; ** Light 2\n"\
"dp3 r3.x, r1, c[31]\n"\
"lit r4, r3\n"\
"mad r2, r4.y, c[22], r2\n"\
"\n"\
"; ** Light 3\n"\
"dp3 r3.x, r1, c[32]\n"\
"lit r4, r3\n"\
"mad r2, r4.y, c[23], r2\n"\
"\n"\
"; ** Light 4\n"\
"dp3 r3.x, r1, c[33]\n"\
"lit r4, r3\n"\
"mad r2, r4.y, c[24], r2\n"\
"\n"\
"; ** Light 5\n"\
"dp3 r3.x, r1, c[34]\n"\
"lit r4, r3\n"\
"mad r2, r4.y, c[25], r2\n"\
"\n"\
"; ** Light 6\n"\
"dp3 r3.x, r1, c[35]\n"\
"lit r4, r3\n"\
"mad r2, r4.y, c[26], r2\n"\
"\n"\
"; ** Light 7\n"\
"dp3 r3.x, r1, c[36]\n"\
"lit r4, r3\n"\
"mad r5, r4.y, c[27], r2\n"\
"; Light result is in r5\n"\
";Still use the vertex alpha\n"\
"mov r5.w, v2.w	\n"\
"\n"\
"; If Lighting is not enabled, then use color from either primitive color or from vertex color\n"\
"add r3, c[44], -v2\n"\
"mad r3, r3, c[43], v2\n"\
"add r2, r3, -r5\n"\
"mad r2, r2, c[45], r5\n"\
"\n"\
"; Force vertex alpha\n"\
"add r3, c[47], -r2\n"\
"mad oD0.w, r3, c[46], r2\n"\
"mov oD0.xyz, r2\n"\
"\n"\
"; Fog factor\n"\
"; Fog_factor = vertex.z, if w<0 or z<0, then factor=0\n"\
"slt r3, r7, c[0]\n"\
"mul r4, r3.z, r7\n"\
"mul r4, r3.w, r4\n"\
"mul r4, c[60], r4\n"\
"add r4, r4, c[61]\n"\
"mov oD1.w, r4.z\n"\
"mov oFog.x, r4.z\n"\
"\n"\
"\n"\
"; Texture coordinates\n"\
"mul r3.xy, v3.xy, c[50]\n"\
"add oT0.xy, r3.xy, c[52]\n"\
"mul r3.xy, v3.xy, c[51]\n"\
"add oT1.xy, r3.xy, c[53]\n"\
"\n"\
"; Optional transforms\n"\
"\n"\
"; viewport transform\n"\
"; z = (z+1)*0.5 = z*0.5+0.5\n"\
"mad r7.z, c[2], r7.z, c[2]\n"\
"mul r7.xy, r7.xy, c[5]\n"\
"mov oPos, r7\n"\
"\n"\
"; Hacks\n"\
"; z = z/10+0.9\n"\
"; mad r3, oPos, c[3], c[1]\n"\
"\n"\
"; To do\n"\
"; 1. Replace alpha with fog factor if fog is enabled\n"\
"\n";

char *shaderstr2 = 
"// Complex vertex shader to emulate N64 pipeline\n"\
"\n"\
"vs_1_1\n"\
"dcl_position v0\n"\
"dcl_normal v1\n"\
"dcl_color v2\n"\
"dcl_texcoord v3\n"\
"\n"\
"// Transform position to clip space and output it\n"\
"dp4 r7.x, v0, c[10]\n"\
"dp4 r7.y, v0, c[11]\n"\
"dp4 r7.z, v0, c[12]\n"\
"dp4 r7.w, v0, c[13]\n"\
"\n"\
"// Lighting\n"\
"// Transform normal to eye space\n"\
"dp3 r1.x, v1, c[14]\n"\
"dp3 r1.y, v1, c[15]\n"\
"dp3 r1.z, v1, c[16]\n"\
"\n"\
"// Normalize transformed normal\n"\
"dp3 r1.w, r1, r1\n"\
"rsq r1.w, r1.w\n"\
"mul r1, r1, r1.w\n"\
"\n"\
"// ** Light 0, the ambient light\n"\
"mov r2, c[20]\n"\
"\n"\
"// ** Light 1\n"\
"// Dot normal with light vector\n"\
"// This is the intensity of the diffuse component\n"\
"mov r3, c[0]\n"\
"dp3 r3.x, r1, c[30]\n"\
"// Calculate the diffuse & specular factors\n"\
"lit r4, r3\n"\
"\n"\
"// add the (diffuse color * diffuse light color * diffuse intensity(R4.y))\n"\
"mad r2, r4.y, c[21], r2\n"\
"\n"\
"// ** Light 2\n"\
"dp3 r3.x, r1, c[31]\n"\
"lit r4, r3\n"\
"mad r2, r4.y, c[22], r2\n"\
"\n"\
"// ** Light 3\n"\
"dp3 r3.x, r1, c[32]\n"\
"lit r4, r3\n"\
"mad r2, r4.y, c[23], r2\n"\
"\n"\
"// ** Light 4\n"\
"dp3 r3.x, r1, c[33]\n"\
"lit r4, r3\n"\
"mad r2, r4.y, c[24], r2\n"\
"\n"\
"// ** Light 5\n"\
"dp3 r3.x, r1, c[34]\n"\
"lit r4, r3\n"\
"mad r2, r4.y, c[25], r2\n"\
"\n"\
"// ** Light 6\n"\
"dp3 r3.x, r1, c[35]\n"\
"lit r4, r3\n"\
"mad r2, r4.y, c[26], r2\n"\
"\n"\
"// ** Light 7\n"\
"dp3 r3.x, r1, c[36]\n"\
"lit r4, r3\n"\
"mad r5, r4.y, c[27], r2\n"\
"// Light result is in r5\n"\
"//Still use the vertex alpha\n"\
"mov r5.w, v2.w	\n"\
"\n"\
"// If Lighting is not enabled, then use color from either primitive color or from vertex color\n"\
"add r3, c[44], -v2\n"\
"mad r3, r3, c[43], v2\n"\
"add r2, r3, -r5\n"\
"mad r2, r2, c[45], r5\n"\
"\n"\
"// Force vertex alpha\n"\
"add r3, c[47], -r2\n"\
"mad oD0.w, r3, c[46], r2\n"\
"mov oD0.xyz, r2\n"\
"\n"\
"// Fog factor\n"\
"// Fog_factor = vertex.z, if w<0 or z<0, then factor=0\n"\
"slt r3, r7, c[0]\n"\
"mul r4, r3.z, r7\n"\
"mul r4, r3.w, r4\n"\
"mul r4, c[60], r4\n"\
"add r4, r4, c[61]\n"\
"mov oD1.w, r4.z\n"\
"mov oFog, r4.z\n"\
"\n"\
"\n"\
"// Texture coordinates\n"\
"mul r3.xy, v3.xy, c[50]\n"\
"add oT0.xy, r3.xy, c[52]\n"\
"mul r3.xy, v3.xy, c[51]\n"\
"add oT1.xy, r3.xy, c[53]\n"\
"\n"\
"// Optional transforms\n"\
"\n"\
"// viewport transform\n"\
"// z = (z+1)*0.5 = z*0.5+0.5\n"\
"mad r7.z, c[2], r7.z, c[2]\n"\
"mul r7.xy, r7.xy, c[5]\n"\
"mov oPos, r7\n"\
"\n"\
"// Hacks\n"\
"// z = z/10+0.9\n"\
"// mad r3, oPos, c[3], c[1]\n"\
"\n"\
"// To do\n"\
"// 1. Replace alpha with fog factor if fog is enabled\n"\
"\n";

char *shaderstr3 = 
"vs_1_1              // version instruction\n"\
"dcl_position v0     // define position data in register v0\n"\
"m4x4 oPos, v0, c0   // transform vertices by view/projection matrix\n"\
"mov oD0, c4         // load constant color\n";

bool InitVertexShader()
{
#if DIRECTX_VERSION == 8
	// shader decl
	DWORD decl[] =
	{
		D3DVSD_STREAM(0),
		D3DVSD_REG(0, D3DVSDT_FLOAT3 ), // D3DVSDE_POSITION
		D3DVSD_REG(1, D3DVSDT_FLOAT3 ), // D3DVSDE_NORMAL 
		D3DVSD_REG(2, D3DVSDT_D3DCOLOR ), // D3DVSDE_DIFFUSE 
		D3DVSD_REG(3, D3DVSDT_FLOAT2 ), // D3DVSDE_TEXCOORD0 
		D3DVSD_END()
	};
#else
	D3DVERTEXELEMENT9 decl[] = 
	{
		{ 0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 12, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
		{ 0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,  0 },
		{ 0, 28, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		D3DDECL_END()
	};
#endif

	LPD3DXBUFFER pCode;
	LPD3DXBUFFER ppCompilationErrors;

	HRESULT res;

#ifdef _DEBUG
#if DIRECTX_VERSION == 8
	LPD3DXBUFFER ppConstants;
	res = D3DXAssembleShaderFromFile("shader.vsh", D3DXASM_DEBUG, &ppConstants, &pCode, &ppCompilationErrors);
#else
	res = D3DXAssembleShaderFromFileA("D:\\n64developing\\RiceVideo\\shaderdx9.vsh", NULL, NULL, D3DXSHADER_DEBUG, &pCode, &ppCompilationErrors);
	//res = D3DXAssembleShaderFromFileA("D:\\n64developing\\RiceVideo\\shaderdx9-2.vsh", NULL, NULL, D3DXSHADER_DEBUG, &pCode, &ppCompilationErrors);
#endif
#else
#if DIRECTX_VERSION == 8
	LPD3DXBUFFER ppConstants;
	res = D3DXAssembleShader(shaderstr, strlen(shaderstr), 0, &ppConstants, &pCode, &ppCompilationErrors);
#else
	res = D3DXAssembleShader(shaderstr2, strlen(shaderstr2), NULL, NULL, D3DXSHADER_DEBUG, &pCode, &ppCompilationErrors);
#endif
#endif


	if( CDXGraphicsContext::IsResultGood(res,true) )
	{
#if DIRECTX_VERSION == 8
		res = g_pD3DDev->CreateVertexShader( decl, (DWORD*)pCode->GetBufferPointer(), &gVertexShader, 0 );
#else
		res = g_pD3DDev->CreateVertexShader( (DWORD*)pCode->GetBufferPointer(), &gVertexShader );
		//FILE *fp = fopen("D:\\n64developing\\RiceVideo\\shaderdx9.vso","rb");
		//BYTE buf[4000];
		//int num = fread(buf, 1, 4000, fp);
		//res = g_pD3DDev->CreateVertexShader( (DWORD*)buf, &gVertexShader );
		//fclose(fp);
#endif
		pCode->Release();

		if( !CDXGraphicsContext::IsResultGood(res,true) )
			return false;

		res = g_pD3DDev->SetVertexShader( gVertexShader );
		InitVertexShaderConstants();
		return true;
	}
	else
	{
		if( ppCompilationErrors )
		{
			char* str = (char*)(ppCompilationErrors->GetBufferPointer());
			TRACE0(str);
		}
		return false;
	}
}