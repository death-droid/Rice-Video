#ifndef _BMG_IMAGE_H_
#define _BMG_IMAGE_H_
/*
//  header file for the BMGImage functions
//
//  Copyright 2000, 2001 M. Scott Heiman
*/

#include <windows.h>

#ifdef BUILD_BMG_DLL
#	define BMG_EXPORT __declspec( dllexport )
#else
#	define BMG_EXPORT
#endif

enum BMG_Error 
{ 
	BMG_OK = 0,
    errLib = 1,
    errInvalidPixelFormat = 2,
    errMemoryAllocation = 3,
	errInvalidSize = 4,
    errInvalidBitmapHandle = 5,
    errWindowsAPI = 6,
	errFileOpen = 7,
    errUnsupportedFileFormat = 8,
    errInvalidBMGImage = 9,
	errInvalidFileExtension = 10,
    errFileRead = 11,
    errFileWrite = 12,
	errInvalidGeoTIFFPointer = 13,
    errUndefinedBGImage = 14,
    errBGImageTooSmall = 15,
	errCorruptFile = 16,
	errIncorrectFormat = 17
};

typedef enum BMG_Error BMGError;

#pragma pack(push,1)
struct BMGImageStruct
{
    unsigned int width;
    unsigned int height;
    unsigned char bits_per_pixel;
    unsigned char *bits;
    unsigned short palette_size;
    unsigned char bytes_per_palette_entry;
    unsigned char *palette;
    unsigned int scan_width;
    int opt_for_bmp; /*= 1 if memory has been sized for HBITMAP, 0 otherwise*/
    short transparency_index;
};
#pragma pack(pop)

#if defined(__cplusplus)
extern "C" {
#endif

/* initializes a BMGImage to default values */
extern
void BMG_EXPORT InitBMGImage( struct BMGImageStruct *img );

/* frees memory allocated to a BMGImage */
extern
void BMG_EXPORT FreeBMGImage( struct BMGImageStruct *img );

/* allocates memory (bits & palette) for a BMGImage.
   returns 1 if successfull, 0 otherwise.
   width, height, bits_per_pixel, palette_size, & opt_for_bmp must have valid
   values before this function is called.
   Assumes that all images with bits_per_pixel <= 8 requires a palette.
   will set bits_per_palette_entry, scan_width, bits, & palette */
extern
BMGError BMG_EXPORT AllocateBMGImage( struct BMGImageStruct *img );

#if defined(__cplusplus)
 }
#endif

#endif