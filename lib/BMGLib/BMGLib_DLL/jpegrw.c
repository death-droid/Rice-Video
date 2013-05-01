/*
//  source code for the BMGLib JPEG functions
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


#include "jpegrw.h"
#include "BMGUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "jpeglib.h"   /* size_t must be defined before this file is included */

/****************************************************************************/
/* error handling structures for JPEGS  */
struct win_jpeg_error_mgr {
  struct jpeg_error_mgr pub;	/* public fields */
  jmp_buf setjmp_buffer;		/* for return to caller */
};

/* JPEG error handler */
typedef struct win_jpeg_error_mgr *error_ptr;

/****************************************************************************/
void win_jpeg_error_exit( j_common_ptr cinfo )
{
  error_ptr myerr = (error_ptr)cinfo->err;
  (*cinfo->err->output_message)(cinfo);
  longjmp( myerr->setjmp_buffer, (int)errLib);
}


/*
    ReadJPEG - Reads the contents of a JPEG file and stores the contents into
              BMGImageStruct

    Inputs:
        filename    - the name of the file to be opened

    Outputs:
        img         - the BMGImageStruct containing the image data

    Returns:
        BMGError - if the file could not be read or a resource error occurred
        BMG_OK   - if the file was read and the data was stored in img

    Limitations:
        Will only read RGB and Gray scale images
*/
BMGError ReadJPEG( const char *filename,
              struct BMGImageStruct *img )
{
    struct jpeg_decompress_struct cinfo;
    struct win_jpeg_error_mgr jerr;
    unsigned char *lpBits;
    int DIBLineWidth;
    JSAMPARRAY buffer;
    int i, error;
    FILE *infile;
	BMGError tmp;

	SetLastBMGError( BMG_OK );

    /* overide the error_exit handler */
    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = win_jpeg_error_exit;

    /* if this is true some weird error has already occured  */
    error = setjmp(jerr.setjmp_buffer);
    if ( error > 0 )
    {
        if ( error != (int)errInvalidBMGImage && error != (int)errFileOpen )
            jpeg_destroy_decompress( &cinfo );
        if ( infile != NULL )
            fclose( infile );
        FreeBMGImage( img );
		SetLastBMGError( (BMGError)error );
        return (BMGError)error;
    }

    if ( img == NULL )
        longjmp( jerr.setjmp_buffer, (int)errInvalidBMGImage );

    infile = fopen( filename, "rb" );
    if ( infile == NULL )
        longjmp( jerr.setjmp_buffer, (int)errFileOpen );

    /* initialize the jpeg decompression object */
    jpeg_create_decompress( &cinfo );

    /* specify the data source */
    jpeg_stdio_src( &cinfo, infile );

    /* read file parameters */
    jpeg_read_header( &cinfo, TRUE );

    img->width = cinfo.image_width;
    img->height = cinfo.image_height;
    img->bits_per_pixel = (unsigned char)(8 * cinfo.num_components);

	tmp = AllocateBMGImage( img );
    if ( tmp != BMG_OK )
        longjmp( jerr.setjmp_buffer, (int)tmp );
    /* set parameters for decompression
    add here if needed  */

    /* start decompressor */
    jpeg_start_decompress( &cinfo );

    /* we don't support non-RGB color spaces */
    if ( cinfo.out_color_space != JCS_RGB &&
         cinfo.out_color_space != JCS_GRAYSCALE )
    {
        longjmp( jerr.setjmp_buffer, (int)errUnsupportedFileFormat );
    }

    /* if this is a grayscale image, initialize the palette */
    if ( cinfo.jpeg_color_space == JCS_GRAYSCALE )
    {
        lpBits = img->palette;
        for ( i = 0; i < img->palette_size;
              i++, lpBits += img->bytes_per_palette_entry )
        {
            memset( (void *)lpBits, i, 3 );
        }
    }

    /* point to the place where the pixel data starts and determine the
    number of bytes in each DIB scan line */
    DIBLineWidth = cinfo.image_width * cinfo.num_components;

    /* point to the bottom row of the DIB data.  DIBs are stored bottom-to-top,
    JPEGS are stored top-to-bottom. */
    lpBits = img->bits + img->scan_width * (cinfo.image_height - 1);

    /* create the JPEGs scan line buffer
        we don't need to deallocate this since the JPEG memory manager is used
        to allocate the memory.  The memory will be freed when
        jpeg_destroy_decompress is called */
    buffer = (*cinfo.mem->alloc_sarray)
     ((j_common_ptr)&cinfo, JPOOL_IMAGE, DIBLineWidth, 1);

    /* loop through the decompression object and store the data in the DIB */
    while ( cinfo.output_scanline < cinfo.output_height )
    {
        jpeg_read_scanlines( &cinfo, buffer, 1 );
        if ( cinfo.jpeg_color_space == JCS_GRAYSCALE )
        {
            memcpy( (void *)lpBits, (void *)(*buffer),
               DIBLineWidth*sizeof(unsigned char));
        }
        else
        {
            for ( i = 0; i < DIBLineWidth; i += 3 )
            {
                int i1 = i + 1;
                int i2 = i + 2;
                lpBits[i ] = (*buffer)[i2];
                lpBits[i1] = (*buffer)[i1];
                lpBits[i2] = (*buffer)[i ];
            }
        }
        lpBits -= img->scan_width;
    }

    /* finish decompression */
    jpeg_finish_decompress( &cinfo );

    /* release decompression object */
    jpeg_destroy_decompress( &cinfo );

    fclose( infile );
    return BMG_OK;
}

/*
    WriteJPEG - Writes the contents of a BMGImageStruct to a JPEG file.

    Inputs:
        filename    - the name of the file to be opened
        img         - the BMGImageStruct containing the image data
        quality     - A compression factor ranging from ??? to 100.  The
                      higher the value, the less information lost in the
                      compression process.  The lower the value, the smaller
                      the output file.

    Returns:
        0 - if the file could not be read or a resource error occurred
        1 - if the file was read and the data was stored in img

    Limitations:
        Will not save 1, 4, & 16-BPP images.  Compression losses tend to
        distort the image and the smaller BPP images should be stored in
        other file formats
*/
BMGError WriteJPEG( const char *filename,
                    struct BMGImageStruct img,
                    int quality )
{
    struct jpeg_compress_struct cinfo;
    struct win_jpeg_error_mgr jerr;
    FILE * outfile = NULL;		/* target file */
    JSAMPARRAY buffer;
    int row_stride;		/* physical row width in image buffer */
    int j, j2;
    unsigned char *lpBits, *q;
    int error;
    int GrayScale;
	BMGError tmp;

	SetLastBMGError( BMG_OK );
    /* We have to set up the error handler first, in case the initialization
    * step fails.  (Unlikely, but it could happen if you are out of memory.)
    * This routine fills in the contents of struct jerr, and returns jerr's
    * address which we place into the link field in cinfo.
    */
    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = win_jpeg_error_exit;

    /* if this is true some weird error has already occured */
    error = setjmp(jerr.setjmp_buffer);
    if ( error > 0 )
    {
        if ( outfile != NULL )
            fclose( outfile );
        jpeg_destroy_compress(&cinfo);

		SetLastBMGError( (BMGError)error );
        return (BMGError)error;
    }

    /* convert 16 BPP images to 24 BPP images */
	if ( img.bits_per_pixel == 16 )
	{
		tmp = Convert16to24( &img ); 
		if (  tmp != BMG_OK )
			longjmp( jerr.setjmp_buffer, (int)tmp );
	}

    /* determine if this is a grayscale image */
    if ( img.palette != NULL )
    {
        error = 0;
        j = 0;
        lpBits = img.palette;
        /* count the number of palette entries where the red element ==
           the blue element == the green element */
        while ( j < img.palette_size && error == 0 )
        {
            if ( lpBits[0] == lpBits[1] && lpBits[0] == lpBits[2] )
            {
                lpBits += img.bytes_per_palette_entry;
                j++;
            }
            else
                error = 1;
        }

        GrayScale = j == img.palette_size;
    }
    /* an 8 bit image with no palette MUST be a grayscale image */
    else if ( img.palette == NULL && img.bits_per_pixel == 8 )
        GrayScale = 1;
    else
        GrayScale = 0;

    /* Step 1: allocate and initialize JPEG compression object */
    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(&cinfo);

    /* 1 & 4 BPP images should be stored in other "lossless" formats */
    if ( img.bits_per_pixel < 8  )
        longjmp( jerr.setjmp_buffer, (int)errInvalidPixelFormat );

    /* Step 2: specify data destination (eg, a file) */
    /* Note: steps 2 and 3 can be done in either order. */

    if ((outfile = fopen(filename, "wb")) == NULL)
        longjmp( jerr.setjmp_buffer, (int)errFileOpen );

    jpeg_stdio_dest(&cinfo, outfile);

    /* Step 3: set parameters for compression */

    /* First we supply a description of the input image.
    * Four fields of the cinfo struct must be filled in:
    */
    cinfo.image_width = img.width; 	/* image width and height, in pixels */
    cinfo.image_height = img.height;
    /* # of color components per pixel */
    cinfo.input_components = GrayScale ? 1 : 3;
    /* colorspace of input image */
    cinfo.in_color_space = GrayScale ? JCS_GRAYSCALE : JCS_RGB;
    /* Now use the library's routine to set default compression parameters.
    * (You must set at least cinfo.in_color_space before calling this,
    * since the defaults depend on the source color space.)
    */
    jpeg_set_defaults(&cinfo);
    /* Now you can set any non-default parameters you wish to.
    * Here we just illustrate the use of quality (quantization table) scaling:
    */
    jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

    /* Step 4: Start compressor */

    /* TRUE ensures that we will write a complete interchange-JPEG file.
    * Pass TRUE unless you are very sure of what you're doing.
    */
    jpeg_start_compress(&cinfo, TRUE);

    /* Step 5: while (scan lines remain to be written) */
    /*           jpeg_write_scanlines(...); */

    /* point to the bottom row of the DIB data.  DIBs are stored bottom-to-top,
    JPEGS are stored top-to-bottom. */
    lpBits = img.bits + img.scan_width * ( img.height - 1 );

    /* JSAMPLEs per row in image_buffer */
    row_stride = cinfo.image_width * cinfo.input_components;
    /* we don't need to deallocate this since the JPEG memory manager is used
     to allocate the memory.  The memory will be freed when
     jpeg_destroy_decompress is called */
    buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

     /* bytes_per_pixel*/
     error = img.bits_per_pixel / 8;

    while ( cinfo.next_scanline < cinfo.image_height )
    {
        if ( GrayScale )
        {
            memcpy( (void *)(*buffer), (void *)lpBits,
				row_stride*sizeof(unsigned char));
        }
        else
        {
            switch ( img.bits_per_pixel )
            {
                /* 256 color palette */
                case 8:
                    q = buffer[0];
                    for ( j = 0; j < (int)img.width; j++, q += 3 )
                    {
                        j2 = img.bytes_per_palette_entry * lpBits[j];
                        q[2] = img.palette[j2  ];
                        q[1] = img.palette[j2+1];
                        q[0] = img.palette[j2+2];
                    }
                    break;
                /* true color or 24-bit color with alpha component
                   the alpha component is lost */
                case 24:
                case 32:
                    q = lpBits;
                    for ( j = 0; j < row_stride; j += 3, q += error )
                    {
                        buffer[0][j+2] = q[0];
                        buffer[0][j+1] = q[1];
                        buffer[0][j  ] = q[2];
                    }
                    break;
            }
        }

        (void) jpeg_write_scanlines(&cinfo, buffer, 1 );
        lpBits -= img.scan_width;
    }

    /* Step 6: Finish compression */

    jpeg_finish_compress(&cinfo);
    /* After finish_compress, we can close the output file. */
    fclose(outfile);
    outfile = NULL;

    /* Step 7: release JPEG compression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_compress(&cinfo);

    return BMG_OK;
}