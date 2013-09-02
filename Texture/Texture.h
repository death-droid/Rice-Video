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


#ifndef __SURFACEHANDLER_H__
#define __SURFACEHANDLER_H__

/////////////// Define a struct to use as
///////////////  storage for all the surfaces
///////////////  created so far.
class CTexture;

typedef struct {
	unsigned int	dwWidth;			// Describes the width of the real texture area. Use lPitch to move between successive lines
	unsigned int	dwHeight;			// Describes the height of the real texture area
	unsigned int	dwCreatedWidth;		// Describes the width of the created texture area. Use lPitch to move between successive lines
	unsigned int	dwCreatedHeight;	// Describes the height of the created texture area
	LONG		lPitch;				// Specifies the number of bytes on each row (not necessarily bitdepth*width/8)
	VOID		*lpSurface;			// Pointer to the top left pixel of the image
} DrawInfo;


enum TextureFmt {
	TEXTURE_FMT_A8R8G8B8,
	TEXTURE_FMT_A4R4G4B4,
	TEXTURE_FMT_UNKNOWN,
};

enum TextureUsage {
	AS_NORMAL,
	AS_RENDER_TARGET,
	AS_BACK_BUFFER_SAVE,
};

class CTexture
{
public:
	virtual ~CTexture();
	uint32		m_dwWidth;			// The requested Texture w/h
	uint32		m_dwHeight;

	unsigned int		m_dwCreatedTextureWidth;	// What was actually created
	unsigned int		m_dwCreatedTextureHeight;

	float		m_fXScale;		// = m_dwCorrectedWidth/m_dwWidth
	float		m_fYScale;		// = m_dwCorrectedHeight/m_dwWidth

	bool		m_bIsEnhancedTexture;
	
	TextureUsage	m_Usage;

	LPDIRECT3DTEXTURE9 GetTexture() { return m_pTexture; }

	// Provides access to "surface"
	bool StartUpdate(DrawInfo *di);
	void EndUpdate(DrawInfo *di);
	
	CTexture(uint32 dwWidth, uint32 dwHeight, TextureUsage usage = AS_NORMAL);

protected:
	LPDIRECT3DTEXTURE9 CreateTexture(uint32 dwWidth, uint32 dwHeight, TextureUsage usage = AS_NORMAL);
	LPDIRECT3DTEXTURE9	m_pTexture;
};

#endif