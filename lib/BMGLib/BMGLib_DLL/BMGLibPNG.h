#ifndef _BMG_LIBPNG_H_
#define _BMG_LIBPNG_H_
/*
//  header file for the BMGLibPNG functions
//
//  Copyright 2000, 2001 M. Scott Heiman
//  All Rights Reserved
//  libPNG is Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
//    (libpng versions 0.5, May 1995, through 0.89c, May 1996)
//    Copyright (c) 1996, 1997 Andreas Dilger
//    (libpng versions 0.90, December 1996, through 0.96, May 1997)
//    Copyright (c) 1998, 1999 Glenn Randers-Pehrson
//    (libpng versions 0.97, January 1998, through 1.0.5, October 15, 1999)
*/

#include "pngrw.h"

#ifdef BUILD_BMG_DLL
#	define BMG_EXPORT __declspec( dllexport )
#else
#	define BMG_EXPORT
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#pragma message ("Exporting BMGLibPNG functions")
/* saves the contents of an HBITMAP to a file.  The extension of the file name
// determines the file type.  returns 1 if successfull, 0 otherwise */
extern
BMGError BMG_EXPORT SaveBitmapToPNGFile( HBITMAP hBitmap,      /* bitmap to be saved */
                                    const char *filename); /* name of output file */

/* Creates an HBITMAP to an image file.  The extension of the file name
// determines the file type.  returns an HBITMAP if successfull, NULL
// otherwise */
extern
HBITMAP BMG_EXPORT CreateBitmapFromPNGFile( const char *filename,
                                            int blend );

#if defined(__cplusplus)
 }
#endif

#endif