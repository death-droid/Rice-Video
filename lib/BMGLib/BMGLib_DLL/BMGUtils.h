#ifndef _BMG_UTILS_H_
#define _BMG_UTILS_H_
/*
    some handy utilities used in several units

    Copyright 2001
    M. Scott Heiman
    All Rights Reserved
*/

#include "BMGImage.h"

/* creates a 24 bpp image from a 16 bpp image */
BMGError Convert16to24( struct BMGImageStruct *img );

#endif
