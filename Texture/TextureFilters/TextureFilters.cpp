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
#include "..\..\stdafx.h"
#include <string>
#include <fstream>
#include <iostream>
#include "TextureFilters.h"
#include "..\..\SimpleIni.h"
#include "BMGDll.h"

void EnhanceTexture(TxtrCacheEntry *pEntry)
{
	if( pEntry->dwEnhancementFlag == options.textureEnhancement )
	{
		// The texture has already been enhanced
		return;
	}
	else if( options.textureEnhancement == TEXTURE_NO_ENHANCEMENT ) 
	{
		//Texture enhancement has being turned off
		//Delete any allocated memory for the enhanced texture
		SAFE_DELETE(pEntry->pEnhancedTexture);
		//Set the enhancement flag so the texture wont be processed again
		pEntry->dwEnhancementFlag = TEXTURE_NO_ENHANCEMENT;
		return;
	}

	DrawInfo srcInfo;	
	//Start the draw update
	if(!pEntry->pTexture->StartUpdate(&srcInfo))
	{
		//If we get here we were unable to start the draw update
		//Delete any allocated memory for the enhanced texture
		SAFE_DELETE(pEntry->pEnhancedTexture);
		return;
	}

	//Set our enhancement flag
	pEntry->dwEnhancementFlag = options.textureEnhancement;

	// Don't enhance for large textures
	if( srcInfo.dwCreatedWidth + srcInfo.dwCreatedHeight > 1024/2 )
	{
		//End the draw update
		pEntry->pTexture->EndUpdate(&srcInfo);
		//Delete any data allocated for the enhanced texture
		SAFE_DELETE(pEntry->pEnhancedTexture);
		//Set the enhancement flag so the texture wont be processed again - WHUT THATS WRONG, texture will be continually processed
		pEntry->dwEnhancementFlag = TEXTURE_NO_ENHANCEMENT;
		return;
	}

	//Create the surface for the texture
	CTexture* pSurfaceHandler = new CTexture(srcInfo.dwCreatedWidth * 2, srcInfo.dwCreatedHeight * 2);

	DrawInfo destInfo;
	if(pSurfaceHandler)
	{
		//Open up the surface handler for updating
		if( pSurfaceHandler->StartUpdate(&destInfo))
		{
			switch(options.textureEnhancement)
			{
				case TEXTURE_2XSAI_ENHANCEMENT:
					Super2xSaI((uint32*)(srcInfo.lpSurface),(uint32*)(destInfo.lpSurface), srcInfo.dwCreatedWidth, srcInfo.dwCreatedHeight, srcInfo.dwCreatedWidth);
					break;
				case TEXTURE_HQ2X_ENHANCEMENT:
					hq2x((uint8*)(srcInfo.lpSurface), srcInfo.lPitch, (uint8*)(destInfo.lpSurface), destInfo.lPitch, srcInfo.dwCreatedWidth, srcInfo.dwCreatedHeight);
					break;
				case TEXTURE_HQ2XS_ENHANCEMENT:
					hq2xS((uint8*)(srcInfo.lpSurface), srcInfo.lPitch, (uint8*)(destInfo.lpSurface), destInfo.lPitch, srcInfo.dwCreatedWidth, srcInfo.dwCreatedHeight);
					break;
				default:
					break;
			}
			//Tell it that we have finished updating the surface
			pSurfaceHandler->EndUpdate(&destInfo);	
		}

		pSurfaceHandler->m_bIsEnhancedTexture = true;
	}

	pEntry->pTexture->EndUpdate(&srcInfo);

	pEntry->pEnhancedTexture = pSurfaceHandler;
}


/************************************************************************/
/*                                                                      */
/************************************************************************/

/****
 All code bellow, CLEAN ME
****/
enum TextureType
{
	NO_TEXTURE,
	RGB_PNG,
	RGB_WITH_ALPHA_TOGETHER_PNG,
};

struct ExtTxtrInfo{
	unsigned int width;
	unsigned int height;
	int fmt;
	int siz;
	int crc32;
	int pal_crc32;
	char *foldername;
	char *filename;
	char *filename_a;
	TextureType type;
	bool bSeparatedAlpha;
	int scaleShift;
	// cached texture
	unsigned char	*pHiresTextureRGB;
	unsigned char	*pHiresTextureAlpha;
};
void CacheHiresTexture( ExtTxtrInfo &ExtTexInfo );

CSortedList<uint64,ExtTxtrInfo> gTxtrDumpInfos;
CSortedList<uint64,ExtTxtrInfo> gHiresTxtrInfos;

extern void GetPluginDir( char * Directory );
extern char * right(char * src, int nchars);

int GetImageInfoFromFile(char* pSrcFile, IMAGE_INFO *pSrcInfo)
{
    unsigned char sig[8];
    FILE *f;

	//Open up the file so we can read from it
    f = fopen(pSrcFile, "rb");

	//If the file returns empty then spit out an error
    if (f == NULL)
    {
		TRACE1("GetImageInfoFromFile() error: couldn't open file '%s'", pSrcFile);
		return 1;
    }
	//Read the first 8 bytes from the image
    if (fread(sig, 1, 8, f) != 8)
    {
		//If we couldn't read the first 8 bytes then send spit out an error
		TRACE1("GetImageInfoFromFile() error: couldn't read first 8 bytes of file '%s'", pSrcFile);
		fclose(f);// We failed to read the first 8 bytes, so ensure that we have flosed the file read stream
		return 1;
    }
	//Close the file stream
    fclose(f);

	//Now lets do some header anaylsis
	if(sig[0] == 137 && sig[1] == 'P' && sig[2] == 'N' && sig[3] == 'G' && sig[4] == '\r' && sig[5] == '\n' &&
               sig[6] == 26 && sig[7] == '\n') // PNG
    {
		//Define out BMGImageStruct where we will store our info
        struct BMGImageStruct img;
		//Set our new struct to the proper size
        memset(&img, 0, sizeof(BMGImageStruct));
		//Read the information we need from the PNG into our BMGImageStruct
        BMG_Error code = ReadPNGInfo(pSrcFile, &img);

        if( code == BMG_OK )
        {
			//Store the image width in our own image info
            pSrcInfo->Width = img.width;
			//Store the iamge height in our own image info
            pSrcInfo->Height = img.height;
			//Store the bits per pixel as our depth
            pSrcInfo->Depth = img.bits_per_pixel;
			//Store the amount of mip levels as 1, since png's do not contain mip map data
            pSrcInfo->MipLevels = 1;
			//Set our image format based on the amount of bits per pixel there is
            if(img.bits_per_pixel == 32)
                pSrcInfo->Format = D3DFMT_A8R8G8B8;
            else if(img.bits_per_pixel == 8)
                pSrcInfo->Format = D3DFMT_P8;
            // Resource and File Format ignored
            FreeBMGImage(&img);
            return 0;
        }
		//If we get here it means we failed to read the PNG file
        TRACE2("Couldn't read PNG file '%s'; error = %i", pSrcFile, code);
        return 1;
    }
	//If we get here it means the image where checking is not a known format
	TRACE1("GetImageInfoFromFile : unknown file format (%s)", pSrcFile);
    return 1;
}

/********************************************************************************************************************
 * Truncates the current list with information about hires textures and scans the hires folder for hires textures and 
 * creates a list with records of properties of the hires textures.
 * parameter:
 * foldername: the folder that should be scaned for valid hires textures.
 * infos: a pointer that will point to the list containing the records with the infos about the found hires textures.
 *        In case of enabled caching, these records will also contain the actual textures.
 * extraCheck: ?
 * bRecursive: flag that indicates if also subfolders should be scanned for hires textures
 * bCacheTextures: flag that indicates if the identified hires textures should also be cached
 * bMainFolder: indicates if the folder is the main folder that will be scanned. That way, texture counting does not
 *              start at 1 each time a subfolder is accessed. (microdev: I know that is not important but it really 
 *              bugged me ;-))
 * return:
 * infos: the list with the records of the identified hires textures. Be aware that these records also contains the 
 *        actual textures if caching is enabled.
 ********************************************************************************************************************/
void FindAllTexturesFromFolder(char *foldername, CSortedList<uint64,ExtTxtrInfo> &infos, bool extraCheck, bool bRecursive, bool bCacheTextures = false, bool bMainFolder = true)
{
	// check if folder actually exists
	if(!PathIsDirectory(foldername) )
		return;

	// the path of the texture
	char			texturefilename[_MAX_PATH];
	// the folder path which is scanned for textures
	char			searchpath[_MAX_PATH];
	// counter to count the identified hires textures
	static int      count;
	// handle to the first file found with a particular name (pattern search)
	HANDLE			findfirst;
	// the record containing the actual file data
	WIN32_FIND_DATA libaa;
	// 
	IMAGE_INFO	imgInfo;
	//
	IMAGE_INFO	imgInfo2;
	// simple ini
	CSimpleIniA ini;

	// prepare message
	sprintf(generalText,"Processing folder: %s", foldername);
	// create box for displaying it on screen
	RECT rect2={0,200,windowSetting.uDisplayWidth,300};
	//and.. well, you guess it - display it
	OutputText(generalText,&rect2);

	// set the search path to the current folder name
	strcpy(searchpath, foldername);
	// add the search pattern
	strcat(searchpath, "*.*");

	// and fetch the first file corresponding to this pattern
	findfirst = FindFirstFile(searchpath, &libaa);
	// ehmm... there is no first file
	if( findfirst == INVALID_HANDLE_VALUE )
	{
		// thus forget it - empty folder
		return;
	}

	// we are in the main folder ==> reset counter (I love it)
	if (bMainFolder) 
		count = 0;

	// vars for: DRAM-crc, texture format, texture size (in bit), and the palette-crc
	uint32 crc, fmt, siz, palcrc32;
	//ASCII representation (hex) of DRAM-CRC and palette-CRC
	char crcstr[16], crcstr2[16];

	do
	{
		// the array is empty, break the current operation
		if( libaa.cFileName == NULL )
			break;

		// the current file is a hidden one
		if( libaa.cFileName[0] == '.' )
			// these files we don't need
			continue;

		// let's process the next file
		count++;

		// get the folder name
		strcpy(texturefilename, foldername);
		// and append the file name
		strcat(texturefilename, libaa.cFileName);

		// assemble the message
		sprintf(generalText,"Texture %d is checked: %s", count, libaa.cFileName);
		// display it in the status bar
		SetWindowText(g_GraphicsInfo.hStatusBar,generalText);
		// prepare a rectangle for on-screen display
		RECT rect={0,300,windowSetting.uDisplayWidth,320};
		// and also display it on-screen
		OutputText(generalText,&rect, DT_LEFT);

		//Check if the current file is a directory and if recursive scanning is enabled
		if( PathIsDirectory(texturefilename) && bRecursive )
		{
			// add file-separator
			strcat(texturefilename, "\\");
			// scan detected subfolder for hires textures (recursive call)
			FindAllTexturesFromFolder(texturefilename, infos, extraCheck, bRecursive, bCacheTextures, false);
			continue;
		}

		// well, the current file is actually no file (probably a directory & recursive scanning is not enabled)
		if( strstr(libaa.cFileName,g_curRomInfo.szGameName) == 0 )
			// go on with the next one
			continue;

		// now we've got a file. Let's check if it is a supported texture format
		if( GetImageInfoFromFile(texturefilename, &imgInfo) != 0 )
		{
			// ehmm, wait a second... no!
			TRACE1("Cannot get image info for file: %s", libaa.cFileName);
			return;
		}

		// init texture type var
		TextureType type = NO_TEXTURE;
		// init alpha flag
		bool bSeparatedAlpha = false;

		// detect the texture type by it's extention
		// microdev: this is not the smartest way. Should be done by header analysis if possible
		if( _stricmp(right(libaa.cFileName, 13),  "_ciByRGBA.png"  ) == 0 || 
			_stricmp(right(libaa.cFileName, 16), "_allciByRGBA.png") == 0 ||
			_stricmp(right(libaa.cFileName,  8), "_all.png")         == 0)
		{
			type = RGB_WITH_ALPHA_TOGETHER_PNG;
		}
		// detect the texture type by it's extention
		else if( _stricmp(right(libaa.cFileName,8), "_rgb.png") == 0)
		{
			// store type to the record
			type = RGB_PNG;

			char filename2[256];
			// assemble the file name for the separate alpha channel file
			strcpy(filename2,texturefilename);
			strcpy(filename2 + strlen(filename2) -8,"_a.png");

			// check if this file actually exists
			if( PathFileExists(filename2) )
			{
				// check if the file with this name is actually a texture (well an alpha channel one)
				if( GetImageInfoFromFile(filename2, &imgInfo2) != 0 )
				{
					// nope, it isn't => go on with next file
					TRACE1("Cannot get image info for file: %s", filename2);
					continue;
				}
				// yes it is a texture file. Check if the size of the alpha channel is the same as the one of the texture
				if( extraCheck && (imgInfo2.Width != imgInfo.Width || imgInfo2.Height != imgInfo.Height) )
				{
					// nope, it isn't => go on with next file
					TRACE1("RGB and alpha texture size mismatch: %s", filename2);
					continue;
				}

				// Indicate that the file has a sperated alpha channel
				bSeparatedAlpha = true; 
			}
		}
		// if a known texture format has been detected...
		if( type != NO_TEXTURE )
		{
		/*
			Try to read image information here.

			(CASTLEVANIA2)#(58E2333F)#(2)#(0)#(D7A5C6D9)_ciByRGBA.png
			(------1-----)#(---2----)#(3)#(4)#(---5----)_ciByRGBA.png

			1. Internal ROM name
			2. The DRAM CRC
			3. The image pixel size (8b=0, 16b=1, 24b=2, 32b=3)
			4. The texture format (RGBA=0, YUV=1, CI=2, IA=3, I=4)
			5. The palette CRC

			<internal Rom name>#<DRAM CRC>#<24bit>#<RGBA>#<PAL CRC>_ciByRGBA.png
		*/
			// get the actual file name
			strcpy(texturefilename, libaa.cFileName);
			// place the pointer before the DRAM-CRC (first occurance of '#')
			char *ptr = strchr(texturefilename,'#');
			// terminate the string ('0' means end of string - or in this case begin of string)
			*ptr++ = 0;


			// extract the information from the file name - this file does not have a palette crc; information is:
			// <DRAM(or texture)-CRC><texture type><texture format>
			// o gosh, commenting source code is really boring - but necessary!! Thus do it! (and don't use drugs ;-))
			if (sscanf(ptr, "%8c#%d#%d#%8c", crcstr, &fmt, &siz, crcstr2) == 4)
				palcrc32 = strtoul(crcstr2, NULL, 16);
			else if (sscanf(ptr, "%8c#%d#%d", crcstr, &fmt, &siz) == 3)
				palcrc32 = 0xFFFFFFFF;
			else
				return;

			// transform the ascii presentation of the hex value to an unsigned integer
			crc = strtoul(crcstr,NULL,16);

			// for the detection of an existing item
			int foundIdx = -1;
			// for the detection of the WIP folder
			bool bWIPFolder = false;
			// loop through the list of records of already fetched hires textures
			for( int k=0; k<infos.size(); k++)
			{
				// check if texture already exists in the list
				// microdev: that's why I somehow love documenting code: that makes the implementation of a WIP folder check
				// fucking easy :-)
				if( infos[k].crc32 == crc && infos[k].pal_crc32 == palcrc32 )
				{
					// indeeed, the texture already exists
					// microdev: MAYBE ADD CODE TO MOVE IT TO A 'DUBLICATE' FOLDER TO EASE WORK OF RETEXTURERS
					// check if the WIP folder is currently treated
					if(strstr(foldername, '\\'+ WIP_FOLDER) != 0)
					{
						foundIdx = k;
						// indeed! => indicate that
						bWIPFolder = true;
					}
					else
						//ini.SetValue("Duplicates", infos[k].filename, _strdup(libaa.cFileName)); //Todo, finish me
						break;
				}

			}

			// if the texture is not yet in the list or if it exists with another type or the current folder is the WIP folder
			if(	foundIdx < 0 || type != infos[foundIdx].type || bWIPFolder)
			{
				// create a new entry
				ExtTxtrInfo *newinfo;

				// if WIP folder check is active, and a texture already exists,
				if(foundIdx >= 0 && type == infos[foundIdx].type && bWIPFolder)
				{
					// modify the existing entry
					newinfo = &infos[foundIdx];
					// free memory for the existing texture
					SAFE_DELETE(newinfo->pHiresTextureRGB);
					SAFE_DELETE(newinfo->pHiresTextureAlpha);
				}
				else
					// otherwise create a new one
					newinfo = new ExtTxtrInfo;

				// store the width
				newinfo->width = imgInfo.Width;
				// store the height
				newinfo->height = imgInfo.Height;
				//allocate space for the name of the folder it has been found in
				newinfo->foldername = new char[strlen(foldername)+1];
				// store the name of the folder it has been found in
				strcpy(newinfo->foldername,foldername);
				//store the name of the texture
				newinfo->filename = _strdup(libaa.cFileName);
				//assume by default theres no alpha texture
				newinfo->filename_a = NULL;
				// store the format
				newinfo->fmt	= fmt;
				// store the size (bit-size not texture size)
				newinfo->siz	= siz;
				// store DRAM (texture) CRC
				newinfo->crc32 =	crc;
				// store PAL (palette) CRC (the actual one or the dummy value ('FFFFFFFF'))
				newinfo->pal_crc32 = palcrc32;
				// store the texture type
				newinfo->type = type;
				// indicate if there is a separate alpha file that has to be loaded
				newinfo->bSeparatedAlpha = bSeparatedAlpha;

				// store the extention of the texture and also the extention of the alpha channel (if existing)
				if(bSeparatedAlpha)
				{
					char filename2[_MAX_PATH];
					strcpy(filename2, libaa.cFileName);
					strcpy(filename2+strlen(filename2)-8,"_a.png");
					newinfo->filename_a = _strdup(filename2);
				}

				// generate the key for the record describing the hires texture.
				// This key is used to find it back in the list
				// The key format is: <DRAM(texture)-CRC-8byte><PAL(palette)-CRC-6byte(2bytes have been truncated to have space for format and size)><format-1byte><size-1byte>
				uint64 crc64 = newinfo->crc32;
				crc64 <<= 32;
				crc64 |= newinfo->pal_crc32&0xFFFFFFFF;

				// if caching has been enabled, also cache the actual texture
				if(bCacheTextures)
				{
					// generate status message
					sprintf(generalText,"Texture %d is loading: %s", count, libaa.cFileName);
					// display status message in the status bar fo the window
					SetWindowText(g_GraphicsInfo.hStatusBar,generalText);
					// prepare a rectancle for displaying onscreen message
					RECT rect={0,300,windowSetting.uDisplayWidth,320};
					// display onscreen message
					OutputText(generalText,&rect, DT_LEFT);
					// cache the actual texture to memory (and of course the alpha channel as well, if existing)
					CacheHiresTexture(*newinfo);
				}

				// a new entry only has to be added to the list if it was not an already existing one
				if(!(foundIdx >= 0 && type == infos[foundIdx].type && bWIPFolder))
					// add the new record to the list 
					infos.add(crc64,*newinfo);
			}
		}
	// loop through all files of the current directory
	} while(FindNextFile(findfirst, &libaa));
}

/********************************************************************************************************************
 * Checks if a folder is actually existant. If not, it tries to create this folder
 * parameter:
 * pathname: the name of the folder that should be checked or created if not existant
 * return:
 * return value: flag that indicates true if the folder is existant or could be created. If none was the case, 
 *               false will be returned
 ********************************************************************************************************************/
bool CheckAndCreateFolder(const char* pathname)
{
	// check if provided folder already exists
	if( !PathFileExists(pathname) )
	{
		// it didn't. Try creating it
		if( !CreateDirectory(pathname, NULL) )
		{
			// it didn't work (probably insuffient permissions or read-only media) ==> return false
			TRACE1("Can not create new folder: %s", pathname);
			return false;
		}
	}

	// success
	return true;
}

// microdev: THIS HAS TO BE CLEANED UP...

// Texture dumping filenaming
// GameName_FrameCount_CRC_Fmt_Siz.bmp
// File format:		BMP
// GameName:		N64 game internal name
// CRC:				32 bit, 8 hex digits
// Fmt:				0 - 4
// Siz:				0 - 3

char *subfolders[] = {
	"png_all\\",
	"ci_by_png\\",
};

void FindAllDumpedTextures(void)
{
	char	foldername[256];
	GetPluginDir(foldername);

	strcat(foldername,"texture_dump\\");

	CheckAndCreateFolder(foldername);

	strcat(foldername,g_curRomInfo.szGameName);
	strcat(foldername,"\\");

	gTxtrDumpInfos.clear();

	CheckAndCreateFolder(foldername);

	FindAllTexturesFromFolder(foldername,gTxtrDumpInfos, false, true);

	char	foldername2[256];
	for( int i=0; i<2; i++)
	{
		strcpy(foldername2,foldername);
		strcat(foldername2,subfolders[i]);
		CheckAndCreateFolder(foldername2);
	}
}

/********************************************************************************************************************
 * Truncates the current list with information about hires textures and scans the hires folder for hires textures and 
 * creates a list with records of properties of the hires textures.
 * parameter: 
 * none
 * return:
 * none
 ********************************************************************************************************************/
void FindAllHiResTextures(char* WIPFolderName = NULL)
{
	CSimpleIniA ini;
	char	foldername[256];
	// get the path of the plugin directory
	GetPluginDir(foldername);

	// add the relative path to the hires folder
	strcat(foldername,"hires_texture\\");
	// it does not exist? => create it
	CheckAndCreateFolder(foldername);

	// add the path to a sub-folder corresponding to the rom name 
	// HOOK IN: PACK SELECT
	strcat(foldername,g_curRomInfo.szGameName);
	strcat(foldername,"\\");
	if(WIPFolderName == NULL)
	{
		// trunkate the current list with hires texture infos
		gHiresTxtrInfos.clear();
	} 
	else
	{
		strcat(foldername,WIPFolderName);
		strcat(foldername,"\\");	
	}
	// check if there is a subfolder for this rom
	if( !PathFileExists(foldername) )
	{
		// no? bye, bye ;-)
		return;
	}
	else
	{
		// find all hires textures and also cache them if configured to do so
		FindAllTexturesFromFolder(foldername,gHiresTxtrInfos, true, true, options.bCacheHiResTextures != FALSE);
	}
}

void CloseHiresTextures(void)
{
	for( int i=0; i<gHiresTxtrInfos.size(); i++)
	{
		if( gHiresTxtrInfos[i].foldername )
			delete [] gHiresTxtrInfos[i].foldername;
		if( gHiresTxtrInfos[i].filename )
			delete []gHiresTxtrInfos[i].filename;
		if( gHiresTxtrInfos[i].filename_a )
			delete []gHiresTxtrInfos[i].filename_a;

		// don't forget to also free memory of cached textures
		if(gHiresTxtrInfos[i].pHiresTextureRGB)
			delete [] gHiresTxtrInfos[i].pHiresTextureRGB;
		if(gHiresTxtrInfos[i].pHiresTextureAlpha)
			delete [] gHiresTxtrInfos[i].pHiresTextureAlpha;
	}

	gHiresTxtrInfos.clear();
}

void CloseTextureDump(void)
{
	for( int i=0; i<gTxtrDumpInfos.size(); i++)
	{
		if( gTxtrDumpInfos[i].foldername )	
			delete [] gTxtrDumpInfos[i].foldername;
		if( gTxtrDumpInfos[i].filename )
			delete []gTxtrDumpInfos[i].filename;
		if( gTxtrDumpInfos[i].filename_a )
			delete []gTxtrDumpInfos[i].filename_a;
	}

	gTxtrDumpInfos.clear();
}


void CloseExternalTextures(void)
{
	static char* currentRomName = new char[200];
	// this condition has been added to avoid reloading the game each time a savestate has been loaded
	// The trick is quite simple: it is checked if the internal rom name has changed
	// If it has changed, a new game has been started - then hires has to be reloaded otherwise not
	if((currentRomName == NULL) || (strcmp(g_curRomInfo.szGameName, currentRomName) != 0))
	{
		strcpy(currentRomName, g_curRomInfo.szGameName);
		CloseHiresTextures();
		CloseTextureDump();
	}
}

/********************************************************************************************************************
 * Scans the hires folder for hires textures and creates a list with records of properties of the hires textures.
 * in case of enabled hires caching also the actual hires textures will be added to the record. Before textures will
 * be loaded, existing list of texture information will be truncated.
 * parameter: 
 * bWIPFolder: Indicates if all textures should be inited or just the WIP folder. Just the content of the WIP folder 
 *             will be reloaded if a savestate has been loaded or if there has been a switch between window and full-
 *             screen mode. 
 * return:
 * none
 ********************************************************************************************************************/

void InitHiresTextures(bool bWIPFolder)
{
	CSimpleIniA ini;
	if( options.bLoadHiResTextures )
	{
		// create a box for displaying the message on screen
		RECT rect={0,100,windowSetting.uDisplayWidth,200};
		// write text
		if(options.bCacheHiResTextures)
			OutputText("Texture caching option is enabled",&rect);
		else
			OutputText("Texture loading option is enabled",&rect);
		// create a box for displaying the message on screen
		RECT rect2={0,150,windowSetting.uDisplayWidth,250};

		// in case of loading save state or changing between window & fulscreen mode
		if(bWIPFolder){
		// write text
			OutputText("Checking WIP folder for hires textures",&rect2);
			SetWindowText(g_GraphicsInfo.hStatusBar,"Checking WIP folder for hires textures");
		}
		else
		{
		// write text
			OutputText("Finding all hires textures",&rect2);
			SetWindowText(g_GraphicsInfo.hStatusBar,"Finding all hires textures");
		}
		// check if all textures should be updated or just the ones located in the WIP folder
		if(!bWIPFolder)
		{
			// Generate our cache directory filename that where after
			char	inifilename[1024];
			// get the path of the plugin directory
			GetPluginDir(inifilename);
			// add the relative path to the hires folder
			strcat(inifilename,"hires_texture\\");
			//Grab our proper folder name
			strcat(inifilename, g_curRomInfo.szGameName);
			strcat(inifilename, "\\Cache.ini");

			//If we have already loaded this pack in then dont bother trying to get all the infomration again
			if (PathFileExists(inifilename))
			{
				ini.LoadFile(inifilename); //Loud in our cache file
				CSimpleIniA::TNamesDepend sections; //A place to store all the sections we find
				ini.GetAllSections(sections); // Store all our sections
				CSimpleIniA::TNamesDepend::const_iterator i; //Define out iterator so we can look through the sections we grabbed

				//Define a new place to store our incoming new info
				ExtTxtrInfo *newinfo;

				//Since where just looping through
				for (i = sections.begin(); i != sections.end(); ++i)
				{
		
					//Where creating a new newinfo everytime we loop
					newinfo = new ExtTxtrInfo;

					//Grab the information we need straight out of the ini file, this is ten times faster than loading the information out of
					//each and every darn png, specially with the community texture pack
					newinfo->width		= ini.GetLongValue(i->pItem,     "width", 0);
					newinfo->height		= ini.GetLongValue(i->pItem,    "height", 0);
					newinfo->fmt		= ini.GetLongValue(i->pItem,    "format", 0);
					newinfo->siz		= ini.GetLongValue(i->pItem,  "bit-size", 0);
					newinfo->crc32		= ini.GetLongValue(i->pItem,     "crc32", 0);
					newinfo->pal_crc32  = ini.GetLongValue(i->pItem, "pal_crc32", 0);
					newinfo->type		= (TextureType)ini.GetLongValue(i->pItem,	  "type", 0);
					newinfo->bSeparatedAlpha = ini.GetBoolValue(i->pItem, "seperated-alpha", 0);
					newinfo->foldername = _strdup((char *)ini.GetValue(i->pItem, "foldername", ""));
					newinfo->filename   = _strdup((char *)ini.GetValue(i->pItem,    "filename", ""));
					newinfo->filename_a   =  _strdup((char *)ini.GetValue(i->pItem,  "filename_alpha", "NULL"));
					if(newinfo->filename_a == "NULL")
						newinfo->filename_a = NULL;
					uint64 crc64 = newinfo->crc32;
					crc64 <<= 32;
					crc64 |= newinfo->pal_crc32&0xFFFFFFFF;
					gHiresTxtrInfos.add(crc64,*newinfo);
				}
			}			
			else
			{
				//Than find all hires textures
				FindAllHiResTextures();


				//And after we have foudn them all, loopthrough them and dump them to our cache file
				for( int i=0; i<gHiresTxtrInfos.size(); i++)
				{
					ini.SetLongValue(gHiresTxtrInfos[i].filename,			  "width", gHiresTxtrInfos[i].width);
					ini.SetLongValue(gHiresTxtrInfos[i].filename,			 "height", gHiresTxtrInfos[i].height);
					ini.SetLongValue(gHiresTxtrInfos[i].filename,			 "format", gHiresTxtrInfos[i].fmt);
					ini.SetLongValue(gHiresTxtrInfos[i].filename,		   "bit-size", gHiresTxtrInfos[i].siz);
					ini.SetLongValue(gHiresTxtrInfos[i].filename,			  "crc32", gHiresTxtrInfos[i].crc32);
					ini.SetLongValue(gHiresTxtrInfos[i].filename,		  "pal_crc32", gHiresTxtrInfos[i].pal_crc32);
					//ini.SetLongValue(newinfo->filename,			  "crc64", crc64);
					ini.SetLongValue(gHiresTxtrInfos[i].filename,	           "type", gHiresTxtrInfos[i].type);
					ini.SetBoolValue(gHiresTxtrInfos[i].filename, "seperated-alpha", gHiresTxtrInfos[i].bSeparatedAlpha);
					ini.SetValue(gHiresTxtrInfos[i].filename,        "filename", gHiresTxtrInfos[i].filename);
					ini.SetValue(gHiresTxtrInfos[i].filename,  "filename_alpha",gHiresTxtrInfos[i].filename_a);
					ini.SetValue(gHiresTxtrInfos[i].filename, "foldername", gHiresTxtrInfos[i].foldername);
				}
				//Save our cache file
				ini.SaveFile(inifilename);
			}
		}
		else
		{
			// just update the textures found in the WIP folder
			FindAllHiResTextures(WIP_FOLDER);
		}
	}
}

// load all hires textures into cache
void InitHiresCache(void)
{
	if( options.bCacheHiResTextures )
	{
		for( int i=0; i<gHiresTxtrInfos.size(); i++)
		{

			// generate status message
			sprintf(generalText,"Loading Texture Nr. %d", (i+1));
			SetWindowText(g_GraphicsInfo.hStatusBar,generalText);
			RECT rect={0,300,windowSetting.uDisplayWidth,320};
			OutputText(generalText,&rect);

			CacheHiresTexture(gHiresTxtrInfos[i]);
		}
	}
}

// clear hires texture cache
void ClearHiresCache(void)
{
	if( !options.bCacheHiResTextures )
	{
		for( int i=0; i<gHiresTxtrInfos.size(); i++)
		{
			SAFE_DELETE(gHiresTxtrInfos[i].pHiresTextureRGB);
			SAFE_DELETE(gHiresTxtrInfos[i].pHiresTextureAlpha);
		}
	}
}

void InitTextureDump(void)
{
	if( options.bDumpTexturesToFiles )
	{
		RECT rect={0,100,windowSetting.uDisplayWidth,200};
		OutputText("Texture dump option is enabled",&rect);
		RECT rect2={0,150,windowSetting.uDisplayWidth,250};
		OutputText("Finding all dumpped textures",&rect2);
		SetWindowText(g_GraphicsInfo.hStatusBar,"Finding all dumped textures");
		FindAllDumpedTextures();
	}
}

/********************************************************************************************************************
 * Inits the hires textures. For doing so, all hires textures info & the cached textures (for dumping and the hires ones) 
 * are deleted. Afterwards they are reloaded from file system. This only takes place if a new rom has been loaded.
 * parameter: 
 * none
 * return:
 * none
 ********************************************************************************************************************/

void InitExternalTextures(void)
{
	// this one has to be static for remembering the current rom name
	static char* currentRomName = new char[200];
	// this condition has been added to avoid reloading the game each time a savestate has been loaded
	// The trick is quite simple: it is checked if the internal rom name has changed
	// If it has changed, a new game has been started - then hires has to be reloaded otherwise not
	if((currentRomName == NULL) || (strcmp(g_curRomInfo.szGameName, currentRomName) != 0))
	{
		// ok, rom name has changed ==> remember it
		strcpy(currentRomName, g_curRomInfo.szGameName);
		// remove all hires & dump textures from cache
		CloseExternalTextures();
		// reload and recache hires textures
		InitHiresTextures();
		// prepare list of already dumped textures (for avoiding to redump them). Available hires textures will
		// also be excluded from dumping
		InitTextureDump();
	} 
	else
	{
		InitHiresTextures(true);
	}
}


/********************************************************************************************************************
 * Determines the scale factor for resizing the original texture to the hires replacement. The scale factor is a left 
 * shift. That means scale factor 1 = size(original texture)*2= size(hires texture), 
 * factor 2 = size(original texture)*4= size(hires texture), etc. (I'm not yet sure why it has to be 2^x. Most probably 
 * because of block size. Has to be further determined.
 * parameter: 
 * info: the record describing the external texture
 * entry: the original texture in the texture cache
 * return:
 * info.scaleShift: the value for left shift the original texture size to the corresponding hires texture size
 * return value: the value for left shift the original texture size to the corresponding hires texture size. 
 *               The function returns -1 if the dimensions of the hires texture are not a power of two of the
 *               original texture.
 ********************************************************************************************************************/
int FindScaleFactor(ExtTxtrInfo &info, TxtrCacheEntry &entry)
{
	// init scale shift
	info.scaleShift = 0;

	// check if the original texture dimensions (x and y) scaled with the current shift is still smaller or of the same size as the hires one
	while(info.height >= entry.ti.HeightToLoad*(1<<info.scaleShift)  && info.width >= entry.ti.WidthToLoad*(1<<info.scaleShift))
	{
		// check if the original texture dimensions (x and y)scaled with the current shift have the same size as the hires one
		if(info.height == entry.ti.HeightToLoad*(1<<info.scaleShift)  && info.width == entry.ti.WidthToLoad*(1<<info.scaleShift))
			// found appropriate scale shift, return it
			return info.scaleShift;
		info.scaleShift++;
	}

	// original texture dimensions (x or y or both) scaled with the last scale shift have become larger than the dimensions
	// of the hires texture. That means the dimensions of the hires replacement are not power of 2 of the original texture.
	// Therefore indicate invalid scale shift
	info.scaleShift = -1;
	return info.scaleShift;
}


//Rethink me :S
/********************************************************************************************************************
 * Checks if a hires replacement for a texture is available.
 * parameter: 
 * infos: The list of external textures
 * entry: the original texture in the texture cache
 * return:
 * indexa: returns the index in "infos" where a hires replacement for a texture without 
 *         palette crc or a RGBA_PNG_FOR_ALL_CI texture has been found
 * return value: the index in "infos" where the corresponding hires texture has been found
 ********************************************************************************************************************/
int CheckTextureInfos( CSortedList<uint64,ExtTxtrInfo> &infos, TxtrCacheEntry &entry, int &indexa, bool bForDump = false )
{
	// determine if texture is a color-indexed (CI) texture
	bool bCI = (gRDP.otherMode.text_tlut>=2 || entry.ti.Format == TXT_FMT_CI || entry.ti.Format == TXT_FMT_RGBA) && entry.ti.Size <= TXT_SIZE_8b;

	// generate two alternative ids
	uint64 crc64a = entry.dwCRC;
	crc64a <<= 32;

	uint64 crc64b = crc64a;
	// crc64a = <DRAM-CRC-8bytes><FFFFFF><format-1byte><size-1byte>
	crc64a |= (0xFFFFFFFF);
	// crc64b = <DRAM-CRC-8bytes><palette-crc-6bytes (lowest 2 bytes are removed)><format-1byte><size-1byte>
	crc64b |= (entry.dwPalCRC&0xFFFFFFFF);

	// infos is the list containing the references to the detected external textures
	// get the number of items contained in this list
	int infosize = infos.size();
	int indexb=-1;
	// try to identify the external texture that
	// corresponds to the original texture
	indexa = infos.find(crc64a);		// For CI without pal CRC, and for RGBA_PNG_FOR_ALL_CI
	if( bCI )	
		// If this is a color index texture, search for it.
		indexb = infos.find(crc64b);	// For CI or PNG with pal CRC

	// did not found the ext. text.
	if( indexa >= infosize )	
		indexa = -1;
	// did not found the ext. text. w/ sep. alpha channel
	if( indexb >= infosize )	
		indexb = -1;

	int scaleShift = -1;

	// found texture with sep. alpha channel
	if( indexb >= 0 )
	{
		// determine the factor for scaling
		scaleShift = FindScaleFactor(infos[indexb], entry);

		// ok. the scale factor is supported. A valid replacement has been found
		if( scaleShift >= 0 )
			return indexb;
	}

	// if texture is 4bit, should be dumped and there is no match in the list of external textures
	if( bForDump && bCI && indexb < 0)
		// than return that there is no replacement & therefore texture can be dumped (microdev: not sure about that...)
		return -1;

	// texture has no separate alpha channel, try to find it in the ext. text. list
	if( indexa >= 0 )	
		scaleShift = FindScaleFactor(infos[indexa], entry);

	// ok. the scale factor is supported. A valid replacement has been found
	// this is a texture without ext. alpha channel
	if( scaleShift >= 0 )
		return indexa;
	// no luck at all. there is no valid replacement
	else
		return -1;
}

void DumpCachedTexture( TxtrCacheEntry &entry )
{

	CTexture *pSrcTexture = entry.pTexture;
	if( pSrcTexture )
	{
		// Check the vector table
		int ciidx;
		if( CheckTextureInfos(gHiresTxtrInfos, entry, ciidx, true) >= 0 )
			return;		// This texture already exists as a hires version, thus don't redump it.
		else if( CheckTextureInfos(gTxtrDumpInfos, entry, ciidx, true) >= 0 )
			return;		// This texture has been dumped

		char filename[256];
		char gamefolder[256];

		GetPluginDir(gamefolder);
		strcat(gamefolder,"texture_dump\\");
		strcat(gamefolder,g_curRomInfo.szGameName);
		strcat(gamefolder,"\\");

		//If the texture requires the extra pal CRC, then dump it as so
		if( (gRDP.otherMode.text_tlut>=2 || entry.ti.Format == TXT_FMT_CI || entry.ti.Format == TXT_FMT_RGBA) && entry.ti.Size <= TXT_SIZE_8b )
		{
			sprintf(filename, "%sci_by_png\\%s#%08X#%d#%d#%08X_ciByRGBA.png", gamefolder, g_curRomInfo.szGameName, entry.dwCRC, entry.ti.Format, entry.ti.Size,entry.dwPalCRC);
			D3DXSaveTextureToFile(filename, D3DXIFF_PNG, pSrcTexture->GetTexture(), NULL);
		}
		else
		{
			sprintf(filename, "%spng_all\\%s#%08X#%d#%d_all.png", gamefolder, g_curRomInfo.szGameName, entry.dwCRC, entry.ti.Format, entry.ti.Size);
			D3DXSaveTextureToFile(filename, D3DXIFF_PNG, pSrcTexture->GetTexture(), NULL);
		}

		ExtTxtrInfo newinfo;
		newinfo.width = entry.ti.WidthToLoad;
		newinfo.height = entry.ti.HeightToLoad;
		newinfo.fmt = entry.ti.Format;
		newinfo.siz = entry.ti.Size;
		newinfo.crc32 = entry.dwCRC;
		newinfo.pal_crc32 = entry.dwPalCRC;
		newinfo.foldername = NULL;
		newinfo.filename = NULL;
		newinfo.filename_a = NULL;
		newinfo.type = NO_TEXTURE;
		newinfo.bSeparatedAlpha = false;

		uint64 crc64 = newinfo.crc32;
		crc64 <<= 32;
		crc64 |= newinfo.pal_crc32&0xFFFFFFFF;
		gTxtrDumpInfos.add(crc64, newinfo);

	}
}
bool LoadRGBBufferFromPNGFile(char *filename, unsigned char **pbuf, int &width, int &height, int bits_per_pixel = 24 )
{
	struct BMGImageStruct img;
	memset(&img, 0, sizeof(BMGImageStruct));
	if( !PathFileExists(filename) )
	{
		TRACE1("File at '%s' doesn't exist in LoadRGBBufferFromPNGFile!", filename)
		return false;
	}

	// The following 2 lines fix the crashing in debug mode
	img.bits = NULL;
	img.palette = NULL;

	BMG_Error code = ReadPNG( filename, &img );
	if( code == BMG_OK )
	{
		*pbuf = NULL;

		*pbuf = new unsigned char[img.width*img.height*bits_per_pixel/8];

		if( *pbuf == NULL )
		{
			TRACE3("new[] returned NULL for image width=%i height=%i bpp=%i", img.width, img.height, bits_per_pixel);
			return false;
		}
		//Ok if the image bits per pixel is same as what we are after we dont have to do anything.
		if( img.bits_per_pixel == bits_per_pixel )
		{
			//Copy our image bits into our buffer
			memcpy(*pbuf, img.bits, img.width*img.height*bits_per_pixel/8);
		}
		//Bits per pixel are 24 but we want 32, convert it by adding an alpha channel to the data
		else if (img.bits_per_pixel == 24 && bits_per_pixel == 32)
        {
			unsigned char *pSrc = img.bits;
			unsigned char *pDst = *pbuf;
			for (int i = 0; i < (int)(img.width*img.height); i++)
			{
				//Copy  R
				*pDst++ = *pSrc++;
				//Copy G
				*pDst++ = *pSrc++;
				//Copy B
				*pDst++ = *pSrc++;
				//Set alpha as 0
				*pDst++ = 0xFF;
			}
		}
		// loaded image has alpha, needed image has to be without alpha channel
		else if (img.bits_per_pixel == 32 && bits_per_pixel == 24)
		{
			// pointer to source image data
			unsigned char *pSrc = img.bits;
			// buffer for destination image
			unsigned char *pDst = *pbuf;
			// copy data of the loaded image to the buffer by skipping the alpha byte
			for (unsigned int i = 0; i < img.width * img.height; i++)
			{
				// copy R
				*pDst++ = *pSrc++;
				// copy G
				*pDst++ = *pSrc++;
				// copy B
				*pDst++ = *pSrc++;
				// skip the alpha byte of the loaded image
				pSrc++;
			}
		}
		else if (img.bits_per_pixel == 8 && (bits_per_pixel == 24 || bits_per_pixel == 32))
		{
			// do palette lookup and convert 8bpp to 24/32bpp
			int destBytePP = bits_per_pixel / 8;
			int paletteBytePP = img.bytes_per_palette_entry;
			unsigned char *pSrc = img.bits;
			unsigned char *pDst = *pbuf;
			// clear the destination image data (just to clear alpha if bpp=32)
			memset(*pbuf, 0, img.width*img.height*destBytePP);
			for (int i = 0; i < (int)(img.width*img.height); i++)
			{
				unsigned char clridx = *pSrc++;
				unsigned char *palcolor = img.palette + clridx * paletteBytePP;
				pDst[0] = palcolor[2]; // red
				pDst[1] = palcolor[1]; // green
				pDst[2] = palcolor[0]; // blue
				pDst += destBytePP;
			}
		}
        else
        {
			TRACE3("PNG file '%s' is %i bpp but texture is %i bpp.", filename, img.bits_per_pixel, bits_per_pixel);
            delete [] *pbuf;
            *pbuf = NULL;
        }

		width = img.width;
		height = img.height;
		FreeBMGImage(&img);

		return true;
	}
	else
	{
		TRACE1("ReadPNG() returned error for '%s' in LoadRGBBufferFromPNGFile!", filename);
		*pbuf = NULL;
		return false;
	}
}

/*******************************************************
 * Loads the hires equivaltent of a texture
 * parameter:
 * TxtrCacheEntry: The original texture in the texture cache
 * return:
 * none
 *******************************************************/
//Fix me, we have two problems with our current method
//One - If memory caching is enabled we have to load the texture from memory and convert it to data that we can use
//this is quite a time consuming process considering we do it plenty of time, and if the pack uses very high resolution
//Textures it will slowly bog it down further
//Two - Caching all the textures into memory requires a significant amount of memory as we do not do any compression on
//textures.

//Solution: Implement automatic DXT5 compression of textures and dump them to a .dds file. DDS has the added benefit.
//That it is GPU ready and doesnt require much processing power, speeding up the process.
//DXT compression will also make storing textures into memory more feasible.
void LoadHiresTexture( TxtrCacheEntry &entry )
{
	// check if the external texture has already been checked
	if( entry.bExternalTxtrChecked )
		return;

	// there is already an enhanced texture (e.g. a filtered one)
	if( entry.pEnhancedTexture )
	{
		// delete it from memory before loading the external one
		SAFE_DELETE(entry.pEnhancedTexture);
	}

	int ciidx;
	// search the index of the appropriate hires replacement texture
	// in the list containing the infos of the external textures
	// ciidx is not needed here (just needed for dumping)
	int idx = CheckTextureInfos(gHiresTxtrInfos, entry, ciidx, false);//backtomenow
	if( idx < 0 )
	{
		// there is no hires replacement => indicate that
		entry.bExternalTxtrChecked = true;
		return;
	}
	 
	// if caching has been disabled, the texture data
	// has to be loaded from file system first
	if(!options.bCacheHiResTextures)
		CacheHiresTexture(gHiresTxtrInfos[idx]);

	if( !gHiresTxtrInfos[idx].pHiresTextureRGB )
	{
		
		TRACE1("RGBBuffer creation failed for file '%s'.", gHiresTxtrInfos[idx].filename);
		return;
	}
	// check if the alpha channel has been loaded if the texture has a separate alpha channel
	else if( gHiresTxtrInfos[idx].bSeparatedAlpha && !gHiresTxtrInfos[idx].pHiresTextureAlpha )
	{
		TRACE1("Alpha buffer creation failed for file '%s'.", gHiresTxtrInfos[idx].filename_a);
		return;
	}

	int scale = 1 <<gHiresTxtrInfos[idx].scaleShift;

	int input_height_shift = gHiresTxtrInfos[idx].height - entry.ti.HeightToLoad * scale;
	int input_pitch_a = gHiresTxtrInfos[idx].width;
	int input_pitch_rgb = gHiresTxtrInfos[idx].width;
	gHiresTxtrInfos[idx].width = entry.ti.WidthToLoad * scale;
	gHiresTxtrInfos[idx].height = entry.ti.HeightToLoad * scale;

	entry.pEnhancedTexture = new CTexture(entry.ti.WidthToCreate*scale, entry.ti.HeightToCreate*scale);
	DrawInfo info;

	if( entry.pEnhancedTexture && entry.pEnhancedTexture->StartUpdate(&info) )
	{
		if( gHiresTxtrInfos[idx].type == RGB_PNG )
		{
			input_pitch_rgb *= 3;
			input_pitch_a *= 3;

			// Update the texture by using the buffer
			for( uint32 i=0; i<gHiresTxtrInfos[idx].height; i++)
			{
				unsigned char *pRGB = gHiresTxtrInfos[idx].pHiresTextureRGB + (input_height_shift + i) * input_pitch_rgb;
				unsigned char *pA = gHiresTxtrInfos[idx].pHiresTextureAlpha + (input_height_shift + i) * input_pitch_a;
				unsigned char* pdst = (unsigned char*)info.lpSurface + (gHiresTxtrInfos[idx].height - i - 1)*info.lPitch;

				for( unsigned int j=0; j<gHiresTxtrInfos[idx].width; j++)
				{
					*pdst++ = *pRGB++;		// R
					*pdst++ = *pRGB++;		// G
					*pdst++ = *pRGB++;		// B

					if( gHiresTxtrInfos[idx].bSeparatedAlpha )
					{
						*pdst++ = *pA;
						pA += 3;
					}
					else if( entry.ti.Format == TXT_FMT_I )
					{
						*pdst++ = *(pdst-1);
					}
					else
					{
						*pdst++ = 0xFF;
					}
				}
			}
		}
		else
		{
			// Update the texture by using the buffer
			input_pitch_rgb *= 4;
			for( int i=gHiresTxtrInfos[idx].height-1; i>=0; i--)
			{
				uint32 *pRGB = (uint32*)(gHiresTxtrInfos[idx].pHiresTextureRGB + (input_height_shift + i) * input_pitch_rgb);
				uint32 *pdst = (uint32*)((unsigned char*)info.lpSurface + (gHiresTxtrInfos[idx].height - i - 1)*info.lPitch);
				for( unsigned int j=0; j<gHiresTxtrInfos[idx].width; j++)
				{
					*pdst++ = *pRGB++;		// RGBA
				}
			}
		}

		if( entry.ti.WidthToCreate/entry.ti.WidthToLoad == 2 )
		{
			gTextureManager.Mirror((uint32*)info.lpSurface, gHiresTxtrInfos[idx].width, entry.ti.maskS+gHiresTxtrInfos[idx].scaleShift, gHiresTxtrInfos[idx].width*2, gHiresTxtrInfos[idx].width*2, gHiresTxtrInfos[idx].height, S_FLAG);
		}

		if( entry.ti.HeightToCreate/entry.ti.HeightToLoad == 2 )
		{
			gTextureManager.Mirror((uint32*)info.lpSurface, gHiresTxtrInfos[idx].height, entry.ti.maskT+gHiresTxtrInfos[idx].scaleShift, gHiresTxtrInfos[idx].height*2, entry.pEnhancedTexture->m_dwCreatedTextureWidth, gHiresTxtrInfos[idx].height, T_FLAG);
		}

		if( entry.ti.WidthToCreate*scale < entry.pEnhancedTexture->m_dwCreatedTextureWidth )
		{
			// Clamp
			gTextureManager.Clamp((uint32*)info.lpSurface, gHiresTxtrInfos[idx].width, entry.pEnhancedTexture->m_dwCreatedTextureWidth, entry.pEnhancedTexture->m_dwCreatedTextureWidth, gHiresTxtrInfos[idx].height, S_FLAG);
		}
		if( entry.ti.HeightToCreate*scale < entry.pEnhancedTexture->m_dwCreatedTextureHeight )
		{
			// Clamp
			gTextureManager.Clamp((uint32*)info.lpSurface, gHiresTxtrInfos[idx].height, entry.pEnhancedTexture->m_dwCreatedTextureHeight, entry.pEnhancedTexture->m_dwCreatedTextureWidth, gHiresTxtrInfos[idx].height, T_FLAG);
		}

		entry.pEnhancedTexture->EndUpdate(&info);


		entry.pEnhancedTexture->m_bIsEnhancedTexture = true;
		entry.dwEnhancementFlag = TEXTURE_EXTERNAL;
	}
	else
	{
		TRACE0("Cannot create a new texture");
	}

	// if caching has been disabled, remove
	// cached texture from memory
	if(!options.bCacheHiResTextures)
	{
		SAFE_DELETE(gHiresTxtrInfos[idx].pHiresTextureRGB);
		SAFE_DELETE(gHiresTxtrInfos[idx].pHiresTextureAlpha);
	}

}

/********************************************************************************************************************
 * loads the raw data of the hires from file system and caches it to memory
 * parameter: 
 * ExtTexInfo: The structure describing the hires texture
 * return:
 * ExtTexInfo: The structure describing the hires texture containing the hires texture data and describing properties
 ********************************************************************************************************************/
void CacheHiresTexture( ExtTxtrInfo &ExtTexInfo )
{

	// the buffer for the rgb texture file name
	char filename_rgb[256];
	// the buffer for the alpha channel file name
	char filename_alpha[256];

	// get the folder of the texture
	strcpy(filename_rgb, ExtTexInfo.foldername);
	strcat(filename_rgb, ExtTexInfo.filename);

	if (ExtTexInfo.filename_a) 
	{
		strcpy(filename_alpha, ExtTexInfo.foldername);
		strcat(filename_alpha, ExtTexInfo.filename_a);
	}
	else
	{
		strcpy(filename_alpha, "");
	}
	// init the pointer to the RGB texture data
	ExtTexInfo.pHiresTextureRGB = NULL;
	// init the pointer to the alpha channel data
	ExtTexInfo.pHiresTextureAlpha = NULL;
	// width and height of the loaded texture
	int width, height;
	// flags for indicating if texture loading was successful
	bool bResRGBA=false, bResA=false;
	// a color indexed texture has to (??? or color indexed format or the RGB format) and has to be 8 bit at most per pixel
	bool bCI = ((gRDP.otherMode.text_tlut>=2 || ExtTexInfo.fmt == TXT_FMT_CI || ExtTexInfo.fmt == TXT_FMT_RGBA) && ExtTexInfo.siz <= TXT_SIZE_8b );

	//We need to seperate these as they rely on not having a alpha channel / we need to strip it out by converting them to 24 bits
	if (ExtTexInfo.type == RGB_PNG)
	{
		// load the RGB texture to pHiresTextureRGB and set its proportions to the vars width and height  
		bResRGBA = LoadRGBBufferFromPNGFile(filename_rgb, &ExtTexInfo.pHiresTextureRGB, width, height);
		// if loading was successful and if there is a separate alpha channel
		if (bResRGBA && ExtTexInfo.bSeparatedAlpha)
			// load the alpha channel as well
			bResA = LoadRGBBufferFromPNGFile(filename_alpha, &ExtTexInfo.pHiresTextureAlpha, width, height);
	}
	else
	{
		bResRGBA = LoadRGBBufferFromPNGFile(filename_rgb, &ExtTexInfo.pHiresTextureRGB, width, height, 32);
	}

	// remember dimensions
	ExtTexInfo.width = width;
	ExtTexInfo.height = height;

	// If texture could not be loaded, or loaded texture is invalid (nulled)
	if( !bResRGBA || !ExtTexInfo.pHiresTextureRGB )
	{
		TRACE1("Cannot open %s", filename_rgb);
		// free the memory that has been alocated for the texture
		SAFE_DELETE(ExtTexInfo.pHiresTextureRGB);
		return;
	}
	// if texture has separate alpha channel but channel could not be loaded
	else if( ExtTexInfo.bSeparatedAlpha && !bResA )
	{
		TRACE1("Cannot open %s", filename_alpha);

		// free the memory that has been alocated for the texture
		SAFE_DELETE(ExtTexInfo.pHiresTextureRGB);
		SAFE_DELETE(ExtTexInfo.pHiresTextureAlpha);
		return;
	}
}
