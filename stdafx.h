// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(_STDAFX_H_)
#define _STDAFX_H_

#define _CRT_SECURE_NO_DEPRECATE

#pragma once

#define EXPORT				__declspec(dllexport)
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0601
#include <stdio.h>
#define WINVER 0x0601
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

#include <process.h>

#include <d3d9.h>
#include <d3dx9.h>
#include <d3d9types.h>
#include <D3DCommon.h>
#include "./Utility/dxerr.h"
#include <vector>
#include "./Utility/DaedalusVtx.h"
#include "./math/Vector2.h"
#include "./math/Vector3.h"
#include "./math/Vector4.h"
#include "./math/Matrix4x4.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }// Microdev check me.
#endif


#ifndef SAFE_CHECK
#define SAFE_CHECK(a)	if( (a) == NULL ) {ErrorMsg("Creater out of memory"); throw new std::exception();}
#endif

#include "typedefs.h"
#include "gfx.h"
#include "Video.h"
#include "Config.h"
#include "resource.h"
#include "./Debugger/Debugger.h"
#include "./Parser/RSP_S2DEX.h"
#include "./Parser/RSP_Parser.h"

#include "./Texture/TextureManager.h"
#include "./Texture/ConvertImage.h"
#include "./Texture/Texture.h"

#include "./Combiner/CombinerDefs.h"
#include "./Combiner/DecodedMux.h"

#include "./Combiner/blender.h"

#include "./Combiner/combiner.h"
#include "./Combiner/DirectXCombiner/DirectXCombiner.h"

#include "./Device/RenderTexture.h"
#include "./Device/FrameBuffer.h"

#include "./Device/GraphicsContext.h"
#include "./Device/DirectXDevice/DXGraphicsContext.h"

#include "./Render/RenderBase.h"
#include "./Render/ExtendedRender.h"
#include "./Render/Render.h"
#include "./Render/DirectX/D3DRender.h"

#include "resource.h"

#include "./Utility/CritSect.h"

extern WindowSettingStruct windowSetting;

void __cdecl MsgInfo (char * Message, ...);
void __cdecl ErrorMsg (char * Message, ...);

#define MI_INTR_DP          0x00000020  
#define MI_INTR_SP          0x00000001  

extern uint32 g_dwRamSize;

extern uint32 * g_pu32RamBase;
extern signed char* g_ps8RamBase;
extern unsigned char *g_pu8RamBase;

extern GFX_INFO g_GraphicsInfo;

extern char *project_name;

