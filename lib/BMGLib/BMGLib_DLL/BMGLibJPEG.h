#ifndef _BMG_LIBJPEG_H_
#define _BMG_LIBJPEG_H_
/*
//  header file for the BMGLibJPEG functions
//
//  Copyright 2000, 2001 M. Scott Heiman
//  All Rights Reserved
//  libJPEG is Copyright (C) 1991-1998, Thomas G. Lane and is part of the
//      Independent JPEG Group's software.
*/
#include "tiffrw.h"

#if defined(__cplusplus)
extern "C" {
#endif

#pragma message ("Exporting BMGLibJPEG functions")

/* saves the contents of an HBITMAP to a file.  The extension of the file name
// determines the file type.  returns 1 if successfull, 0 otherwise */
extern
BMGError BMG_EXPORT SaveBitmapToJPEGFile( HBITMAP hBitmap,      /* bitmap to be saved */
                                    const char *filename, /* name of output file */
                                    int quality );

/* Creates an HBITMAP to an image file.  The extension of the file name
// determines the file type.  returns an HBITMAP if successfull, NULL
// otherwise */
extern
HBITMAP BMG_EXPORT CreateBitmapFromJPEGFile( const char *filename );

#if defined(__cplusplus)
 }
#endif

#endif
