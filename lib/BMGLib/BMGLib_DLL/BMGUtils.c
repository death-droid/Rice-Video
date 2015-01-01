/*
//  source code for the BMGLib Utility functions
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

#include <malloc.h>
#include "BMGUtils.h"

/* error strings for all BMG errors */
char BMGErrorStrings[17][128] = {
"No Error",
"Corrupted file or invalid file format",
"Invalid bits per pixel for this file format",
"Memory allocation error",
"Invalid requested image size",
"Invalid bitmap handle",
"Windows API Error",  /* this will be overwritten */
"Unable to open file",
"Unsupported file format option",
"Invalid pointer to a BMG image",
"Unsupported file extension",
"Error reading file",
"Error writing to the output file",
"Invalid pointer to a GeoTIFF structure",
"The background image is undefined",
"The background image is too small",
"Corrupt File"
};

/* stores last BMG error */
static BMGError LastBMGError;

/* sets the last BMG error */
void SetLastBMGError( BMGError err )
{
	LastBMGError = err;
}

/* returns the last error state */
BMGError GetLastBMGError(void)
{
	return LastBMGError;
}
/* gets the error message */
void GetLastBMGErrorMessage( const char **msg )
{
	if ( LastBMGError == errWindowsAPI )
	{
		  LPVOID lpMsgBuf;

		  FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
		  );

		/* copy the string. */
		  strcpy( BMGErrorStrings[(int)LastBMGError], (char *)lpMsgBuf );

		/* Free the buffer. */
		  LocalFree( lpMsgBuf );
	}

	*msg = BMGErrorStrings[(int)LastBMGError];
}

/****************************************************************************
// Converts a 16 BPP image to a 24 BPP image 
// returns 1 if successfull, 0 otherwise */
BMGError Convert16to24( struct BMGImageStruct *img )
{
	unsigned int i;
	unsigned int new_scan_width;
	unsigned char *new_bits;

	/* this function will only work with 16 BBP images */
	if ( img->bits_per_pixel != 16 )
		return errInvalidPixelFormat;

	/* calculate the new scan width */
	new_scan_width = 3 * img->width;
	if ( new_scan_width % 4 && img->opt_for_bmp )
		new_scan_width += 4 - new_scan_width % 4;

	/* allocate memory for the new pixel values */
	new_bits = (unsigned char *)calloc( new_scan_width * img->height, sizeof(unsigned char) );
	if ( new_bits == NULL )
		return errMemoryAllocation;

	/* convert the 16 BPP pixel values to the equivalent 24 BPP values  */
	for ( i = 0; i < img->height; i++ )
	{
		unsigned char *p24;
		unsigned short *p16 = (unsigned short *)(img->bits + i * img->scan_width);
		unsigned char *start = new_bits + i * new_scan_width;
		unsigned char *end = start + new_scan_width;
		for ( p24 = start; p24 < end; p24 += 3, p16++ ) 
		{
			p24[0] = (unsigned char)( (*p16 & 0x001F) << 3 );
			p24[1] = (unsigned char)( (*p16 & 0x03E0) >> 2 );
			p24[2] = (unsigned char)( (*p16 & 0x7C00) >> 7 );
		}
	}

	free( img->bits );
	img->bits = new_bits;
	img->scan_width = new_scan_width;
	img->bits_per_pixel = 24;

	return BMG_OK;
}