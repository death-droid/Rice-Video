#ifndef _TIFF_RW_H_
#define _TIFF_RW_H_
/*
//  header file defining BMGLib libTIFF structures and functions 
//
//  Copyright 2000, 2001 Scott Heiman
//  libTIFF is Copyright Sam Leffler and SGI
//  zLib Copyright (C) 1995-1998 Jean-loup Gailly.
*/

#include "BMGImage.h"

/* enumeration types that support libTIFF */
enum TiffCompressionEnum { NONE, CCITTRLE, CCITTFAX3, CCITTFAX4, LZW, JPEG6,
                       JPEG_DCT, NeXT, CCITTRLEW, MACINTOSH, THUNDERSCAN,
                       PIXARFILM, PIXARLOG, ZIP, KODAK, JBIG };
enum TiffPhotometricEnum { MINISWHITE, MINISBLACK, RGB, PALETTE, MASK,
                       SEPARATED, YCBCR, CIELAB, CIE_LOGL, CIE_LOGLUV };
enum TiffOrientationEnum { TOPLEFT, BOTTOMLEFT };

typedef enum TiffCompressionEnum TiffCompression;
typedef enum TiffPhotometricEnum TiffPhotometric;
typedef enum TiffOrientationEnum TiffOrientation;

#pragma pack(push,1)
struct TIFFInfoStruct
{
    TiffCompression compression;
    TiffPhotometric photometric;
    TiffOrientation orientation;
    unsigned short predictor;
};
#pragma pack(pop)


#ifdef BUILD_BMG_DLL
#   define BMG_EXPORT __declspec( dllexport )
#else
#   define BMG_EXPORT
#endif


#if defined(__cplusplus)
extern "C" {
#endif

extern
BMGError BMG_EXPORT ReadTIFF( const char *filename,
                         struct BMGImageStruct *img,
                         struct TIFFInfoStruct *info );

extern
BMGError BMG_EXPORT WriteTIFF( const char *filename,
                          struct BMGImageStruct img,
                          struct TIFFInfoStruct *info );


#if defined(__cplusplus)
 }
#endif


#endif