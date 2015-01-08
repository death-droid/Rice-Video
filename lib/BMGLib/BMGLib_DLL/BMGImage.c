/*
// source code for the BMGImage functions
//
// Copyright (C) 2001 Michael S. Heiman
//
// You may use the software for any purpose you see fit. You may modify
// it, incorporate it in a commercial application, use it for school,
// even turn it in as homework. You must keep the Copyright in the
// header and source files. This software is not in the "Public Domain".
// You may use this software at your own risk. I have made a reasonable
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

#include <malloc.h>
#include <memory.h>
#include <setjmp.h>
#include "BMGUtils.h"

/* initializes a BMGImage to default values */
void InitBMGImage( struct BMGImageStruct *img )
{
    img->width = img->height = 0;
    img->bits_per_pixel = 0;
    img->palette_size = 0;
    img->bytes_per_palette_entry = 0;
    img->bits = NULL;
    img->palette = NULL;
    img->opt_for_bmp = 0;
    img->scan_width = 0;
    img->transparency_index = -1;
}

/* frees memory allocated to a BMGImage */
void FreeBMGImage( struct BMGImageStruct *img )
{
    if ( img->bits != NULL )
    {
        free( img->bits );
        img->bits = NULL;
    }
    if ( img->palette != NULL )
    {
        free( img->palette );
        img->palette = NULL;
    }
    img->bits_per_pixel = 0;
    img->palette_size = 0;
    img->bytes_per_palette_entry = 0;
    img->width = img->height = 0;
    img->opt_for_bmp = 0;
    img->scan_width = 0;
    img->transparency_index = -1;
}

/* allocates memory for the bits & palette.  Assigned values to scan_line
   & bits_per_palette_entry as well.  Assumes opt_for_bmp has been set before
   this function is called. Assumes that all images with bits_per_pixel <= 8
   require a palette.
 */
BMGError AllocateBMGImage( struct BMGImageStruct *img )
{
    unsigned int mempal;


    /* make sure that all REQUIRED parameters are valid */
    if ( img->width * img->height <= 0 )
	{
        return errInvalidSize;
	}

    switch( img->bits_per_pixel )
    {
        case  1:
        case  4:
        case  8:
        case 16:
        case 24:
        case 32:
            break;
        default:
            return errInvalidPixelFormat;
    }

    /* delete old memory */
    if ( img->bits != NULL )
    {
        free( img->bits );
        img->bits = NULL;
    }
    if ( img->palette != NULL )
    {
        free( img->palette );
        img->palette = NULL;
    }

    /* allocate memory for the palette */
    if ( img->bits_per_pixel <= 8 )
    {
        if ( img->opt_for_bmp > 0 )
            img->bytes_per_palette_entry = 4U;
        else
        {
            /* we only support 3-byte and 4-byte palettes */
            if ( img->bytes_per_palette_entry <= 3U )
                img->bytes_per_palette_entry = 3U;
            else
                img->bytes_per_palette_entry = 4U;
        }
        /*
           use bits_per_pixel to determine palette_size if none was
           specified
        */
        if ( img->palette_size == 0 )
            img->palette_size = (unsigned short)(1 << img->bits_per_pixel);

        mempal = img->bytes_per_palette_entry * img->palette_size;
        img->palette = (unsigned char *)calloc( mempal, sizeof(unsigned char) );
        if ( img->palette == NULL )
		{
            return errMemoryAllocation;
		}
    }
    else
    {
        img->bytes_per_palette_entry = 0;
        img->palette_size = 0;
    }

    /*
       set the scan width.  Bitmaps optimized for windows have scan widths that
       are evenly divisible by 4.
    */
    img->scan_width = ( img->bits_per_pixel * img->width + 7 ) / 8;
    if ( img->opt_for_bmp && img->scan_width % 4 )
        img->scan_width += 4 - img->scan_width % 4;

    /* allocate memory for the bits */
    mempal = img->scan_width * img->height;
    if ( mempal > 0 )
    {
        img->bits = (unsigned char *)calloc( mempal, sizeof( unsigned char) );
        if ( img->bits == NULL )
        {
            if ( img->palette != NULL )
            {
                free( img->palette );
                img->palette = NULL;
            }
            return errMemoryAllocation;
        }
    }
    else
	{
        return errInvalidSize;
	}

    return BMG_OK;
}