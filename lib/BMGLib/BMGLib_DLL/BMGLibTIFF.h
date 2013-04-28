#ifndef _BMG_LIBTIFF_H_
#define _BMG_LIBTIFF_H_
/*
//  header file defining BMGLib BMGlibTIFF functions 
//
//  Copyright 2000, 2001 Scott Heiman
//  libTIFF is Copyright Sam Leffler and SGI
//  zLib Copyright (C) 1995-1998 Jean-loup Gailly.
//
//**************** WARNING **********************************************
// Any project using the BMGLibTIFF library must add 
// "c:\projects\BMGLib\BMGLib_DLL"
//**************** WARNING **********************************************
*/

#include "tiffrw.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef BUILD_BMG_DLL
#   define BMG_EXPORT __declspec( dllexport )
#else
#   define BMG_EXPORT
#endif

#pragma message ("Exporting BMGLibTIFF unique functions" )
/* saves the contents of an HBITMAP to a file.  The extension of the file name
// determines the file type.  returns 1 if successfull, 0 otherwise */
extern
BMGError BMG_EXPORT SaveBitmapToTIFFile( HBITMAP hBitmap,      /* bitmap to be saved */
                                    const char *filename, /* name of output file */
                                    struct TIFFInfoStruct *info );

/* Creates an HBITMAP to an image file.  The extension of the file name
// determines the file type.  returns an HBITMAP if successfull, NULL
// otherwise */
extern
HBITMAP BMG_EXPORT CreateBitmapFromTIFFile( const char *filename,
                                            struct TIFFInfoStruct *info,
                                            int blend );

#if defined(__cplusplus)
 }
#endif

#endif