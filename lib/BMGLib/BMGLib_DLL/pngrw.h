#ifndef _PNG_RW_H_
#define _PNG_RW_H_
/*
//  header file for the BMGLib PNG functions
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
#include "BMGImage.h"

#ifdef BUILD_BMG_DLL
#	define BMG_EXPORT __declspec( dllexport )
#else
#	define BMG_EXPORT
#endif

#if defined(__cplusplus)
extern "C" {
#endif

extern
BMGError BMG_EXPORT ReadPNG( const char *filename,
                        struct BMGImageStruct *img );

extern
BMGError BMG_EXPORT WritePNG( const char *filename,
                         struct BMGImageStruct img );

#if defined(__cplusplus)
 }
#endif

#endif