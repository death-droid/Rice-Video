/*
//  source code for the ImageLib BMP functions
//
//  Copyright (C) 2001 M. Scott Heiman
//  All Rights Reserved
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

#include "BMGDLL.h"
#include "BMGUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

const unsigned short BMP_ID = 0x4D42;

/*
    ReadBMP - reads the image data from a BMP files and stores it in a
              BMGImageStruct.

    Inputs:
        filename    - the name of the file to be opened

    Outputs:
        img         - the BMGImageStruct containing the image data

    Returns:
        BMGError - if the file could not be read or a resource error occurred
        BMG_OK   - if the file was read and the data was stored in img

    Limitations:
        will not read BMP files using BI_RLE8, BI_RLE4, or BI_BITFIELDS
*/
BMGError ReadBMP( const char *filename,
              struct BMGImageStruct *img )
{
    FILE *file;
    jmp_buf err_jmp;
    int error;
	BMGError tmp;
    unsigned char *p, *q; /*, *q_end; */
/*    unsigned int cnt; */
    int i;
/*    int EOBMP; */

    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;
/*
    DWORD mask[3];
*/

    unsigned int DIBScanWidth;
    unsigned int bit_size, rawbit_size;
    unsigned char *rawbits = NULL;

    /* error handler */
    error = setjmp( err_jmp );
    if ( error != 0 )
    {
        if ( file != NULL )
            fclose( file );
        if ( rawbits != NULL )
            free( rawbits );
		if( img != NULL)
			FreeBMGImage( img );
        return (BMGError)error;
    }

    if ( img == NULL )
        longjmp( err_jmp, (int)errInvalidBMGImage );

    file = fopen( filename, "rb" );
    if  ( file == NULL )
        longjmp( err_jmp, (int)errFileOpen );

    /* read the file header */
    if ( fread( (void *)&bmfh, sizeof(BITMAPFILEHEADER), 1, file ) != 1 )
        longjmp( err_jmp, (int)errFileRead );

    /* confirm that this is a BMP file */
    if ( bmfh.bfType != BMP_ID )
        longjmp( err_jmp, (int)errUnsupportedFileFormat );

    /* read the bitmap info header */
    if ( fread( (void *)&bmih, sizeof(BITMAPINFOHEADER), 1, file ) != 1 )
        longjmp( err_jmp, (int)errFileRead );

    /* abort if this is an unsupported format */
    if ( bmih.biCompression != BI_RGB )
            longjmp( err_jmp, (int)errUnsupportedFileFormat );

    img->bits_per_pixel = (unsigned char)bmih.biBitCount;
    img->width  = bmih.biWidth;
    img->height = bmih.biHeight;
    if ( img->bits_per_pixel <= 8 )
    {
        img->palette_size = (unsigned short)bmih.biClrUsed;
        img->bytes_per_palette_entry = 4U;
    }

	tmp = AllocateBMGImage( img );
    if ( tmp != BMG_OK )
        longjmp( err_jmp, (int)tmp );

    /* read palette if necessary */
    if ( img->bits_per_pixel <= 8 )
    {
        if ( fread( (void *)img->palette, sizeof(RGBQUAD), img->palette_size,
                file ) != (unsigned int)img->palette_size )
        {
            longjmp( err_jmp, (int)errFileRead );
        }
    }

    /* dimensions */
    DIBScanWidth = ( img->bits_per_pixel * img->width + 7 ) / 8;
    if ( DIBScanWidth %4 )
        DIBScanWidth += 4 - DIBScanWidth % 4;

    bit_size = img->scan_width * img->height;

    /* allocate memory for the raw bits */
    if ( bmih.biCompression != BI_RGB )
        rawbit_size = bmfh.bfSize - bmfh.bfOffBits;
    else
        rawbit_size = DIBScanWidth * img->height;

    rawbits = (unsigned char *)calloc( rawbit_size, 1 );
    if ( rawbits == NULL )
        longjmp( err_jmp, (int)errMemoryAllocation );

    if ( fread( (void *)rawbits, sizeof(unsigned char), rawbit_size, file )
                   != rawbit_size )
    {
        longjmp( err_jmp, (int)errFileRead );
    }

    if ( bmih.biCompression == BI_RGB )
    {
        p = rawbits;
        for ( q = img->bits; q < img->bits + bit_size;
                         q += img->scan_width, p += DIBScanWidth )
        {
            memcpy( (void *)q, (void *)p, img->scan_width );
        }
    }

    /* swap rows if necessary */
    if ( bmih.biHeight < 0 )
    {
        for ( i = 0; i < (int)(img->height) / 2; i++ )
        {
            p = img->bits + i * img->scan_width;
            q = img->bits + ((img->height) - i - 1 ) * img->scan_width;
            memcpy( (void *)rawbits, (void *)p, img->scan_width );
            memcpy( (void *)p, (void *)q, img->scan_width );
            memcpy( (void *)q, (void *)rawbits, img->scan_width );
        }
    }

    fclose( file );
    free( rawbits );
    return BMG_OK;
}

/*
    ReadBMPInfo - reads the header data from a BMP files and stores it in a
              BMGImageStruct.

    Inputs:
        filename    - the name of the file to be opened

    Outputs:
        img         - the BMGImageStruct containing the image data

    Returns:
        BMGError - if the file could not be read or a resource error occurred
        BMG_OK   - if the file was read and the data was stored in img

*/
BMGError ReadBMPInfo( const char *filename,
              struct BMGImageStruct *img )
{
    FILE *file;
    jmp_buf err_jmp;
    int error;

    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;

    /* error handler */
    error = setjmp( err_jmp );
    if ( error != 0 )
    {
        if ( file != NULL )
            fclose( file );
		if ( img != NULL)
			FreeBMGImage( img );
        return (BMGError)error;
    }

    if ( img == NULL )
        longjmp( err_jmp, (int)errInvalidBMGImage );

    file = fopen( filename, "rb" );
    if  ( file == NULL )
        longjmp( err_jmp, (int)errFileOpen );

        /* read the file header */
    if ( fread( (void *)&bmfh, sizeof(BITMAPFILEHEADER), 1, file ) != 1 )
        longjmp( err_jmp, (int)errFileRead );

    /* confirm that this is a BMP file */
    if ( bmfh.bfType != BMP_ID )
        longjmp( err_jmp, (int)errUnsupportedFileFormat );

    /* read the bitmap info header */
    if ( fread( (void *)&bmih, sizeof(BITMAPINFOHEADER), 1, file ) != 1 )
        longjmp( err_jmp, (int)errFileRead );

    /* abort if this is an unsupported format */
    if ( bmih.biCompression != BI_RGB )
            longjmp( err_jmp, (int)errUnsupportedFileFormat );

    img->bits_per_pixel = (unsigned char)bmih.biBitCount;
    img->width  = bmih.biWidth;
    img->height = bmih.biHeight;
    if ( img->bits_per_pixel <= 8 )
    {
        img->palette_size = (unsigned short)bmih.biClrUsed;
        img->bytes_per_palette_entry = 4U;
    }

    fclose( file );
    return BMG_OK;
}