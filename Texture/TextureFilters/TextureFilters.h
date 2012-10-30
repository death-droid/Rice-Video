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




#ifndef __FILTERS_H__
#define __FILTERS_H__

// this is a subfolder of which the content will always be reloaded if a savestate is loaded 
// or if there is a switch between window and fullscreen mode
#define WIP_FOLDER "WIP"

#define DWORD_MAKE(r, g, b, a)   ((uint32) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define WORD_MAKE(r, g, b, a)   ((uint16) (((a) << 12) | ((r) << 8) | ((g) << 4) | (b)))

void Super2xSaI( uint32 *srcPtr, uint32 *destPtr, uint32 width, uint32 height, uint32 pitch);

void hq2x(uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height);
void hq2xS(uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height);

void InitHiresTextures(bool bWIPFolder = false);


#endif