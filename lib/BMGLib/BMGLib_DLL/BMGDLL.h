#ifndef _BMG_LIB_
#define _BMG_LIB_
/*
//  header file for the BMGLib DLL
//  This DLL encapsulates the libTIFF library, libJPEG library,
//  libPNG library, and the GeoTIFF library.
//
//  Copyright 2000, 2001 M. Scott Heiman
//  All Rights Reserved
//  libTIFF is Copyright Sam Leffler and SGI
//  libJPEG is Copyright (C) 1991-1998, Thomas G. Lane and is part of the
//      Independent JPEG Group's software.
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
*/

#ifdef BUILD_BMG_DLL
#   define BMG_EXPORT __declspec( dllexport )
#else
#   define BMG_EXPORT
#endif

#include "BMGImage.h"
#include "pngrw.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* the following functions will read/write image files using raw data */
extern
BMGError BMG_EXPORT ReadRGB( const char *filename,
                        struct BMGImageStruct *img );

extern
BMGError BMG_EXPORT WriteRGB( const char *filename,
                         struct BMGImageStruct img );

extern
BMGError BMG_EXPORT ReadTGA( const char *filename,
                        struct BMGImageStruct *img );

extern
BMGError BMG_EXPORT WriteTGA( const char *filename,
                        struct BMGImageStruct img );

extern
BMGError BMG_EXPORT ReadBMP( const char *filename,
                        struct BMGImageStruct *img );
extern
BMGError BMG_EXPORT ReadBMPInfo( const char *filename,
                        struct BMGImageStruct *img );

extern
BMGError BMG_EXPORT WriteBMP( const char *filename,
                         struct BMGImageStruct img );

extern
BMGError BMG_EXPORT ReadCEL( const char *filename,
                        struct BMGImageStruct *img );

extern
BMGError BMG_EXPORT ReadGIF( const char *filename,
                        struct BMGImageStruct *img );

extern
BMGError BMG_EXPORT ReadPSD( const char *filename, 
						struct BMGImageStruct *img );

extern
BMGError BMG_EXPORT ReadIFF( const char *filename, 
						struct BMGImageStruct *img );

extern
BMGError BMG_EXPORT ReadPCX( const char *filename, 
						struct BMGImageStruct *img );

#if defined(__cplusplus)
 }
#endif

#endif
