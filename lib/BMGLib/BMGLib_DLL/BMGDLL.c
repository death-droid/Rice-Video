/*
//  source code for the BMGLib DLL
//  This DLL encapsulates the libTIFF library, the libJPEG library, the 
//  libPNG library, and other functions
//
//  Copyright (C) 1998, 2000, 2001, 2002 M. Scott Heiman
//  All Rights Reserved
//  libTIFF is Copyright (C) Sam Leffler and SGI
//  libJPEG is Copyright (C) 1991-1998, Thomas G. Lane and is part of the
//  	Independent JPEG Group's software.
//  libPNG is Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
//    (libpng versions 0.5, May 1995, through 0.89c, May 1996)
//    Copyright (c) 1996, 1997 Andreas Dilger
//    (libpng versions 0.90, December 1996, through 0.96, May 1997)
//    Copyright (c) 1998, 1999 Glenn Randers-Pehrson
//    (libpng versions 0.97, January 1998, through 1.0.5, October 15, 1999)
//  zLib Copyright (C) 1995-1998 Jean-loup Gailly.
//  GeoTIFF is Copyright (c) 1999, Frank Warmerdam
//  libPROJ (used by GeoTIFF) is Copytight (c) 2000, Frank Warmerdam
//  libUnGif is Copyright (c) 1997,  Eric S. Raymond
//
// You may use the software for any purpose you see fit.  You may modify
// it, incorporate it in a commercial application, use it for school,
// even turn it in as homework.  You must keep the Copyright in the
// header and source files.  This software is not in the "Public Domain".
// You may use this software at your own risk.  I have made a reasonable
// effort to verify that this software works in the manner I expect it to;
// however,...
//
// THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS" AND
// WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE, INCLUDING
// WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR FITNESS FOR A
// PARTICULAR PURPOSE. IN NO EVENT SHALL MICHAEL S. HEIMAN BE LIABLE TO
// YOU OR ANYONE ELSE FOR ANY DIRECT, SPECIAL, INCIDENTAL, INDIRECT OR
// CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING
// WITHOUT LIMITATION, LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE,
// OR THE CLAIMS OF THIRD PARTIES, WHETHER OR NOT MICHAEL S. HEIMAN HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
// POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <setjmp.h>
#include "BMGUtils.h"
#include "BMGDLL.h"

/****************************************************************************/
/* local functions */
BMGError CreateRGBAArray( struct BMGImageStruct img,
                          unsigned int *width,
                          unsigned int *height,
                          unsigned char **bits,
                          int bgra );

/****************************************************************************/
BITMAPINFO CreateBMI( DWORD dwWidth,  /* width */
                      DWORD dwHeight, /* height */
                      WORD wBitCount, /* bit count */
                      int compression )  /* compression type */
{
	return InternalCreateBMI( dwWidth, dwHeight, wBitCount, compression );
}

/*******************************************************************************
 CreateRGBAArray is a utility function that will create an RGBA array from any
 possible output of any of the supported files
*******************************************************************************/
BMGError CreateRGBAArray( struct BMGImageStruct img,
                          unsigned int *width,
                          unsigned int *height,
                          unsigned char **bits,
                          int bgra )
{
    jmp_buf err_jmp;
    unsigned char *p, *q, *r, *s;
    unsigned char *unpacked_bits = NULL;
    unsigned short pack;
    unsigned int scanwidth = 4 * img.width;
    unsigned int bit_size = scanwidth * img.height;
    int error;
    unsigned int i;
    unsigned int unpacked_size;
    unsigned int bytes_per_pixel;

    /* error handler */
    error = setjmp( err_jmp );
    if ( error != 0 )
    {
        if ( *bits != NULL )
		{
            free( *bits );
			*bits = NULL;
		}
        if ( unpacked_bits != NULL )
            free( unpacked_bits );
		SetLastBMGError( (BMGError)error );
        return (BMGError)error;
    }

    if ( img.bits == NULL )
        longjmp( err_jmp, (int)errInvalidBMGImage );

    *bits = (unsigned char *)calloc( bit_size, 1 );
    if ( *bits == NULL )
        longjmp( err_jmp, (int)errMemoryAllocation );

    /* if no palette is present then decompress the pixels */
    if ( img.palette == NULL )
    {
        switch ( img.bits_per_pixel )
        {
            case 15:
            case 16:
                for ( i = 0; i < img.height; i++ )
                {
                    p = img.bits + i * img.scan_width;
                    r = *bits +  i * scanwidth;
                    for ( q = r; q < r + scanwidth; q += 4, p += 2 )
                    {
                        pack = *((unsigned short *)p);
                        if ( bgra == 0 )
                        {
                            q[2] = (unsigned char)((pack & 0x001F) << 3);
                            q[1] = (unsigned char)((pack & 0x03E0) >> 2);
                            q[0] = (unsigned char)((pack & 0x7C00) >> 7);
                        }
                        else
                        {
                            q[0] = (unsigned char)((pack & 0x001F) << 3);
                            q[1] = (unsigned char)((pack & 0x03E0) >> 2);
                            q[2] = (unsigned char)((pack & 0x7C00) >> 7);
                        }
                        q[3] = 0xFF;
                    }
                }
                break;
            case 24:
            case 32:
                bytes_per_pixel = img.bits_per_pixel / 8;
                for ( i = 0; i < img.height; i++ )
                {
                    p = img.bits + i * img.scan_width;
                    r = *bits + i * scanwidth;
                    for ( q = r; q < r + scanwidth;
                          q += 4, p += bytes_per_pixel )
                    {
                    	if ( bgra == 0 )
                        {
                            q[0] = p[2];
                            q[1] = p[1];
                            q[2] = p[0];
                        }
                        else
                        	memcpy( (void *)q, (void *)p,
                            	3*sizeof(unsigned char) );

                        q[3] = bytes_per_pixel == 3 ? 0xFF : p[3];
                    }
                }
                break;
            default:
                longjmp( err_jmp, (int)errInvalidPixelFormat );
        }
    }
    /* if a palette is present then map the palette colors to the bits array */
    else
    {
        /* create an array for storing compressed index formats */
        unpacked_size = img.width * img.height;
        unpacked_bits = (unsigned char *)calloc( unpacked_size, 1 );
        if ( unpacked_bits == NULL )
            longjmp( err_jmp, (int)errMemoryAllocation );

        /* if the bits per pixel in the bits array is 1 or 4 then decompress
          the data into an 8 bit array */
        switch ( img.bits_per_pixel )
        {
            case 1:
                Convert1to8( img, unpacked_bits );
                break;
            case 4:
                Convert4to8( img, unpacked_bits );
                break;
            case 8:
                for ( i = 0; i < img.height; i++ )
                {
                    memcpy( (void *)(unpacked_bits + i * img.width),
                            (void *)(img.bits + i * img.scan_width),
                            img.width );
                }
                break;
            default:
                longjmp( err_jmp, (int)errInvalidPixelFormat );
        }

        /* map the palette into bits using the unpacked indices */
        for ( i = 0; i < img.height; i++ )
        {
            p = *bits + i * scanwidth;
            r = unpacked_bits + i * img.width;
            for ( q = r; q < r + img.width; q++, p += 4 )
            {
                s = img.palette + (*q)*img.bytes_per_palette_entry;
                if ( bgra == 0 )
                {
                    p[0] = s[2];
                    p[1] = s[1];
                    p[2] = s[0];
                }
                else
                	memcpy( (void *)p, (void *)s, 3*sizeof(unsigned char) );

                if ( img.transparency_index > - 1 )
                    p[3] = img.transparency_index == *q ? 0x00 : 0xFF;
                else
                    p[3] = 0xFF;
            }
        }

        /* clean up */
        free( unpacked_bits );
    }

    *width = img.width;
    *height = img.height;

    return BMG_OK;
}

/******************************************************************************/
extern
BMGError GetUnpackedArray( const char *filename,
                           unsigned int *width,
                           unsigned int *height,
                           unsigned char **bits,
                           int bgra )
{
    BMGError out;
    struct BMGImageStruct img;

	SetLastBMGError(BMG_OK);

    InitBMGImage( &img );

	out = GetDataFromFile( filename, &img, NULL );

    if ( out == BMG_OK )
    {
        out = CreateRGBAArray( img, width, height, bits, bgra );
    }

    FreeBMGImage( &img );

    return out;
}
/*******************************************************************************
// saves the contents of an HBITMAP to a file
// returns BMG_OK if successfull, or an error code otherwise */
BMGError SaveBitmapToFile( HBITMAP hBitmap, const char *filename, void *parameters )
{
    struct BMGImageStruct img;
    char ext[4], *period;
    int out = BMG_OK;

	SetLastBMGError( BMG_OK );
    InitBMGImage( &img );

    /* extract data from the bitmap.  We assume that 32 bit images have been
    // blended with the background (unless this is a DDB - see GetDataFromBitmap
	// for more details) */
    out = GetDataFromBitmap( hBitmap, &img, 1 );
    if ( out == BMG_OK )
    {
        /* determine the file type by using the extension */
        period = strrchr( filename, '.' ) + 1;
		if ( period == NULL )
		{
			SetLastBMGError(errInvalidFileExtension);
			return errInvalidFileExtension;
		}

		strcpy( ext, period );
        ext[0] = toupper( ext[0] );
        ext[1] = toupper( ext[1] );
        ext[2] = toupper( ext[2] );
        ext[3] = 0;

        if ( strcmp( ext, "PNG" ) == 0 )
        {
            out = WritePNG( filename, img );
        }
        else if ( strcmp( ext, "JPG" ) == 0 )
        {
            int quality =  parameters == NULL ? 100 : *((int *)parameters);

            out = WriteJPEG( filename, img, quality );

        }
        else if ( strcmp( ext, "BMP" ) == 0 )
        {
            out = WriteBMP( filename, img );
        }
        else
        {
			out = errInvalidFileExtension;
			SetLastBMGError( errInvalidFileExtension );
        }
    }

	FreeBMGImage( &img );

    return out;
}
/*******************************************************************************
// this function creates a bitmap from a file. Returns an HBITMAP if it
// succeeds, otherwise NULL */
HBITMAP CreateBitmapFromFile( const char *filename,
                              void *parameters,
                              int blend )
{
    BMGError out;
    struct BMGImageStruct img;
    HBITMAP hBitmap;

	SetLastBMGError( BMG_OK );
    InitBMGImage( &img );
    img.opt_for_bmp = 1;

	out = GetDataFromFile( filename, &img, parameters );

    /* create the bitmap from the image data */
    if ( out == BMG_OK )
        hBitmap = CreateBitmapFromData( img, blend );
	else
		hBitmap = NULL;

    FreeBMGImage( &img );

    return hBitmap;
}

/*
    Saves a RGB, BGR (24-bit) or RGBA, BGRA (32-bit) array of pixels to a file.
*/
BMGError SaveUnpackedArray( const char *filename,
                            unsigned char bytes_per_pixel,
                            unsigned int width,
                            unsigned int height,
                            unsigned char *bits,
							int bgra )
{
    struct BMGImageStruct img;
    unsigned char *p, *q, *r, *s, *r_end, *s_end;
    unsigned int scanwidth = bytes_per_pixel * width;
    char ext[4];
    char *period;
    BMGError out;

	SetLastBMGError( BMG_OK );
    /* create a BMGImage */
    InitBMGImage( &img );

    img.width = width;
    img.height = height;
    img.bits_per_pixel = 8 * bytes_per_pixel;

	out = AllocateBMGImage( &img );
    if ( out != BMG_OK )
    {
		SetLastBMGError( out );
        return out;
    }

	/* make sure that there is a period in the file name */
    period = strrchr( filename, '.' );
	if ( period == NULL )
	{
		out = errInvalidFileExtension;
		SetLastBMGError( out );
        return out;
	}


    /*
       save the array to the BMGImage. 
    */
//    p = bits + ( height - 1 ) * scanwidth;
    p = bits;
    r_end = img.bits + img.scan_width * img.height;
//    for ( r = img.bits; r < r_end; r += img.scan_width, p -= scanwidth )
    for ( r = img.bits; r < r_end; r += img.scan_width, p += scanwidth )
    {
		// memcpy will work for BGR & BGRA images
		if ( bgra == 1 )
		{
			memcpy( (void *)r, (void *)p, scanwidth );
		}
		// we need to swap red and blue bytes for RGB & RGBA images
		else
		{
			q = p;
			s_end = r + img.scan_width;
			for ( s = r; s < s_end; s += bytes_per_pixel, q += bytes_per_pixel )
			{
				s[0] = q[2];
				s[1] = q[1];
				s[2] = q[0];
				if ( bytes_per_pixel == 4 )
					s[3] = q[3];
			}
		}
    }

    /* determine file type */
	period += 1;
    strncpy( ext, period, 3 );
    ext[0] = toupper( ext[0] );
    ext[1] = toupper( ext[1] );
    ext[2] = toupper( ext[2] );
    ext[3] = 0;

    /* save the image to the file */
    if ( strcmp( ext, "PNG" ) == 0 )
    {
        out = WritePNG( filename, img );
    }
    else if ( strcmp( ext, "BMP" ) == 0 )
    {
        out = WriteBMP( filename, img );
    }
  
    else
    {
		out = errInvalidFileExtension;
		SetLastBMGError( errInvalidFileExtension );
    }

    FreeBMGImage( &img );
    return out;
}

BMGError GetDataFromFile( const char *filename, struct BMGImageStruct *img,
						  void *parameters )
{
    char ext[4];
    char *period;
    BMGError out;

	SetLastBMGError( BMG_OK );

    period = strrchr( filename, '.' );
    if ( period == NULL )
    	return errUnsupportedFileFormat;

    period++;
        
    strncpy( ext, period, 3 );
    ext[0] = toupper( ext[0] );
    ext[1] = toupper( ext[1] );
    ext[2] = toupper( ext[2] );
    ext[3] = 0;

    if ( strcmp( ext, "PNG" ) == 0 )
    {
        out = ReadPNG( filename, img );
    }
    else if ( strcmp( ext, "BMP" ) == 0 )
    {
        out = ReadBMP( filename, img );
    }
    else
    {
		out = errInvalidFileExtension;
		SetLastBMGError( errInvalidFileExtension );
    }

	return out;
}
