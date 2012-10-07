// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(_STDAFX_H_)
#define _STDAFX_H_

#define _CRT_SECURE_NO_DEPRECATE

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define EXPORT				__declspec(dllexport)
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0400
#include <stdio.h>
#ifdef _XBOX
#include <xtl.h>
#include <xfont.h>
#else
#define WINVER 0x0400
#include <windows.h>
#include <windowsx.h>			// Button_* etc
#include <shlwapi.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <ShellAPI.h>
#include <winnt.h>			// For 32x32To64 multiplies
#endif
#include <math.h>			// For sqrt()
#include <iostream>
#include <fstream>
#include <istream>

#if _MSC_VER > 1200
#include "xmmintrin.h"
#endif


#include <process.h>

#ifdef DIRECTX8
#define DIRECTX_VERSION		8
#else
#define DIRECTX_VERSION		9
#endif

#if DIRECTX_VERSION > 8
#include <d3d9.h>
#include <d3dx9.h>
#include <d3d9types.h>
#include <D3dx9math.h>
#include <dxerr.h>
#define MYLPDIRECT3DTEXTURE			LPDIRECT3DTEXTURE9
#define MYD3DADAPTER_IDENTIFIER		D3DADAPTER_IDENTIFIER9
#define MYIDirect3DSurface			IDirect3DSurface9
#define MYLPDIRECT3DBASETEXTURE		LPDIRECT3DBASETEXTURE9
#define MYLPDIRECT3DSURFACE			LPDIRECT3DSURFACE9
#define MYD3DVIEWPORT				D3DVIEWPORT9
#define MYD3DCAPS					D3DCAPS9
#define MYLPDIRECT3DDEVICE			LPDIRECT3DDEVICE9
#define MYLPDIRECT3D				LPDIRECT3D9
#define MYIDirect3DBaseTexture		IDirect3DBaseTexture9
#define D3DRS_ZBIAS					D3DRS_DEPTHBIAS
#define SetVertexShaderConstant		SetVertexShaderConstantF
#else
#include <d3d8.h>
#include <d3dx8.h>
#include <d3d8types.h>
#include <D3dx8math.h>
#define MYLPDIRECT3DTEXTURE			LPDIRECT3DTEXTURE8
#define MYD3DADAPTER_IDENTIFIER		D3DADAPTER_IDENTIFIER8
#define MYIDirect3DSurface			IDirect3DSurface8
#define MYLPDIRECT3DBASETEXTURE		LPDIRECT3DBASETEXTURE8
#define MYLPDIRECT3DSURFACE			LPDIRECT3DSURFACE8
#define MYD3DVIEWPORT				D3DVIEWPORT8
#define MYD3DCAPS					D3DCAPS8
#define MYLPDIRECT3DDEVICE			LPDIRECT3DDEVICE8
#define MYLPDIRECT3D				LPDIRECT3D8
#define MYIDirect3DBaseTexture		IDirect3DBaseTexture8
#endif

#include <vector>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#endif

#ifndef SAFE_CHECK
#define SAFE_CHECK(a)	if( (a) == NULL ) {ErrorMsg("Creater out of memory"); throw new std::exception();}
#endif

#include "typedefs.h"
#include "gfx.h"
#include "Video.h"
#include "Config.h"
#include "resource.h"
#include "Debugger.h"
#include "RSP_S2DEX.h"
#include "RSP_Parser.h"

#include "TextureManager.h"
#include "ConvertImage.h"
#include "Texture.h"
#include "DirectXTexture.h"

#include "CombinerDefs.h"
#include "DecodedMux.h"
#include "DirectXDecodedMux.h"

#include "blender.h"
#include "DirectXBlender.h"

#include "combiner.h"
#include "DirectXCombiner.h"

#include "RenderTexture.h"
#include "FrameBuffer.h"

#include "GraphicsContext.h"
#include "DXGraphicsContext.h"
#include "DeviceBuilder.h"

#include "RenderBase.h"
#include "ExtendedRender.h"
#include "Render.h"
#include "D3DRender.h"

#include "resource.h"

#include "icolor.h"

#include "CSortedList.h"
#include "CritSect.h"
#include "Timing.h"


extern WindowSettingStruct windowSetting;

void __cdecl MsgInfo (char * Message, ...);
void __cdecl ErrorMsg (char * Message, ...);

#define MI_INTR_DP          0x00000020  
#define MI_INTR_SP          0x00000001  

extern uint32 g_dwRamSize;

extern uint32 * g_pRDRAMu32;
extern signed char* g_pRDRAMs8;
extern unsigned char *g_pRDRAMu8;

extern GFX_INFO g_GraphicsInfo;

#ifdef _XBOX
extern XFONT *g_defaultTrueTypeFont;
#endif


extern char *project_name;
#endif

