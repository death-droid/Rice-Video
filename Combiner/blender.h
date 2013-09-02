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

#ifndef _BLENDER_H_
#define _BLENDER_H_

class CRender;

class CBlender
{
public:
	
	static void InitBlenderMode(void);
	static void NormalAlphaBlender(void);
	static void DisableAlphaBlender(void);
	
	static void BlendFunc(uint32 srcFunc, uint32 desFunc);

	static void Enable();
	static void Disable();
protected:
	CBlender(CRender *pRender) : m_pRender(pRender) {}
	CRender *m_pRender;
};

#endif



