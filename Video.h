/*
Copyright (C) 2002-2009 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either 
2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef _DLLINTERFACE_H_
#define _DLLINTERFACE_H_

#include "typedefs.h"
typedef struct {
	HINSTANCE myhInst;
	float	fViWidth, fViHeight;
	unsigned __int16		uViWidth, uViHeight;
	unsigned __int16		uDisplayWidth, uDisplayHeight;
	unsigned __int16		uFullScreenDisplayWidth, uFullScreenDisplayHeight;
	unsigned __int16		uWindowDisplayWidth, uWindowDisplayHeight;
	
	bool	bDisplayFullscreen;
	int		uScreenScaleMode;
	float	fMultX, fMultY;
	int		vpLeftW, vpTopW, vpRightW, vpBottomW, vpWidthW, vpHeightW;

	int		statusBarHeight, statusBarHeightToUse, toolbarHeight, toolbarHeightToUse;
	bool	screenSaverStatus;

	struct{
		uint32		left;
		uint32		top;
		uint32		right;
		uint32		bottom;
		uint32		width;
		uint32		height;
		bool		needToClip;
	}clipping;
}WindowSettingStruct;

extern WindowSettingStruct windowSetting;

enum CurScissorType
{
	RSP_SCISSOR,
	RDP_SCISSOR,
	UNKNOWN_SCISSOR,
};

struct PluginStatus{
	bool	bGameIsRunning;	  
	uint32	dwTvSystem;
	float	fRatio;

	BOOL	frameReadByCPU;
	BOOL	frameWriteByCPU;

	uint32	dwNumTrisRendered;
	uint32	dwNumDListsCulled;
	uint32	dwNumTrisClipped;
	uint32	dwNumVertices;

	uint32	gDlistCount;
	uint32	gFrameCount;
	uint32	gUcodeCount;
	uint32	gRDPTime;
	BOOL	ToToggleFullScreen;
	bool	bDisableFPS;

	uint32	curRenderBuffer;
	uint32	curVIOriginReg;
	CurScissorType  curScissor;

	uint32	lastPurgeTimeTime;		// Time textures were last purged

	bool	bCIBufferIsRendered;
	int		leftRendered,topRendered,rightRendered,bottomRendered;

	bool	toShowCFB;
	bool	toCaptureScreen;
	char	screenCaptureFilename[MAX_PATH];

	bool	bAllowLoadFromTMEM;

	// Frame buffer simulation related status variables
	bool	bN64FrameBufferIsUsed;		// Frame buffer is used in the frame
	bool	bN64IsDrawingTextureBuffer;	// The current N64 game is rendering into render_texture, to create self-rendering texture
	bool	bHandleN64RenderTexture;	// Do we need to handle of the N64 render_texture stuff?
	bool	bFrameBufferIsDrawn;		// flag to mark if the frame buffer is ever drawn
	bool	bFrameBufferDrawnByTriangles;	// flag to tell if the buffer is even drawn by Triangle cmds

	bool    bScreenIsDrawn;

};

extern PluginStatus status;
extern char generalText[];

void SetVIScales();
#endif
