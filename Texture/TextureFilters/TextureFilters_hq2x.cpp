/*
Copyright (C) 2003-2009 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "stdafx.h"
#include "interp.h"


/*
* This effect is derived from the hq2x effect made by Maxim Stepin
*/
static void hq2x_32_def(uint32* dst0, uint32* dst1, const uint32* src0, const uint32* src1, const uint32* src2, unsigned count)
{
	unsigned i;

	for(i=0;i<count;++i) {
		unsigned char mask;

		uint32 c[9];

		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i>0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = src0[0];
			c[3] = src1[0];
			c[6] = src2[0];
		}

		if (i<count-1) {
			c[2] = src0[1];
			c[5] = src1[1];
			c[8] = src2[1];
		} else {
			c[2] = src0[0];
			c[5] = src1[0];
			c[8] = src2[0];
		}

		mask = 0;

		if (hq2x_interp_32_diff(c[0], c[4]))
			mask |= 1 << 0;
		if (hq2x_interp_32_diff(c[1], c[4]))
			mask |= 1 << 1;
		if (hq2x_interp_32_diff(c[2], c[4]))
			mask |= 1 << 2;
		if (hq2x_interp_32_diff(c[3], c[4]))
			mask |= 1 << 3;
		if (hq2x_interp_32_diff(c[5], c[4]))
			mask |= 1 << 4;
		if (hq2x_interp_32_diff(c[6], c[4]))
			mask |= 1 << 5;
		if (hq2x_interp_32_diff(c[7], c[4]))
			mask |= 1 << 6;
		if (hq2x_interp_32_diff(c[8], c[4]))
			mask |= 1 << 7;

#define P0 dst0[0]
#define P1 dst0[1]
#define P2 dst1[0]
#define P3 dst1[1]
#define HQ2X_MUR hq2x_interp_32_diff(c[1], c[5])
#define HQ2X_MDR hq2x_interp_32_diff(c[5], c[7])
#define HQ2X_MDL hq2x_interp_32_diff(c[7], c[3])
#define HQ2X_MUL hq2x_interp_32_diff(c[3], c[1])
#define IC(p0) c[p0]
#define I11(p0,p1) hq2x_interp_32_11(c[p0], c[p1])
#define I211(p0,p1,p2) hq2x_interp_32_211(c[p0], c[p1], c[p2])
#define I31(p0,p1) hq2x_interp_32_31(c[p0], c[p1])
#define I332(p0,p1,p2) hq2x_interp_32_332(c[p0], c[p1], c[p2])
#define I431(p0,p1,p2) hq2x_interp_32_431(c[p0], c[p1], c[p2])
#define I521(p0,p1,p2) hq2x_interp_32_521(c[p0], c[p1], c[p2])
#define I53(p0,p1) hq2x_interp_32_53(c[p0], c[p1])
#define I611(p0,p1,p2) hq2x_interp_32_611(c[p0], c[p1], c[p2])
#define I71(p0,p1) hq2x_interp_32_71(c[p0], c[p1])
#define I772(p0,p1,p2) hq2x_interp_32_772(c[p0], c[p1], c[p2])
#define I97(p0,p1) hq2x_interp_32_97(c[p0], c[p1])
#define I1411(p0,p1,p2) hq2x_interp_32_1411(c[p0], c[p1], c[p2])
#define I151(p0,p1) hq2x_interp_32_151(c[p0], c[p1])

		switch (mask) {
#include "TextureFilters_hq2x.h"
		}

#undef P0
#undef P1
#undef P2
#undef P3
#undef HQ2X_MUR
#undef HQ2X_MDR
#undef HQ2X_MDL
#undef HQ2X_MUL
#undef IC
#undef I11
#undef I211
#undef I31
#undef I332
#undef I431
#undef I521
#undef I53
#undef I611
#undef I71
#undef I772
#undef I97
#undef I1411
#undef I151

		src0 += 1;
		src1 += 1;
		src2 += 1;
		dst0 += 2;
		dst1 += 2;
	}
}


static void hq2xS_32_def(uint32* dst0, uint32* dst1, const uint32* src0, const uint32* src1, const uint32* src2, unsigned count)
{
   unsigned i;

   for(i=0;i<count;++i) {
      unsigned char mask;

      uint32 c[9];

      c[1] = src0[0];
      c[4] = src1[0];
      c[7] = src2[0];

      if (i>0) {
         c[0] = src0[-1];
         c[3] = src1[-1];
         c[6] = src2[-1];
      } else {
         c[0] = src0[0];
         c[3] = src1[0];
         c[6] = src2[0];
      }

      if (i<count-1) {
         c[2] = src0[1];
         c[5] = src1[1];
         c[8] = src2[1];
      } else {
         c[2] = src0[0];
         c[5] = src1[0];
         c[8] = src2[0];
      }
    
	mask = 0;
    // hq2xS dynamic edge detection:
	// simply comparing the center color against its surroundings will give bad results in many cases,
	// so, instead, compare the center color relative to the max difference in brightness of this 3x3 block
	int brightArray[9];
	int maxBright = 0, minBright = 999999;
	for(int j = 0 ; j < 9 ; j++)
	{
		const int b = (int)((c[j] & 0xF8));
		const int g = (int)((c[j] & 0xF800)) >> 8;
		const int r = (int)((c[j] & 0xF80000)) >> 16;
		const int bright = r+r+r + g+g+g + b+b;
		if(bright > maxBright) maxBright = bright;
		if(bright < minBright) minBright = bright;

		brightArray[j] = bright;
	}
	int diffBright = ((maxBright - minBright) * 7) >> 4;
	if(diffBright > 7)
	{
		#define ABS(x) ((x) < 0 ? -(x) : (x))

		const int centerBright = brightArray[4];
		if(ABS(brightArray[0] - centerBright) > diffBright)
			mask |= 1 << 0;
		if(ABS(brightArray[1] - centerBright) > diffBright)
			mask |= 1 << 1;
		if(ABS(brightArray[2] - centerBright) > diffBright)
			mask |= 1 << 2;
		if(ABS(brightArray[3] - centerBright) > diffBright)
			mask |= 1 << 3;
		if(ABS(brightArray[5] - centerBright) > diffBright)
			mask |= 1 << 4;
		if(ABS(brightArray[6] - centerBright) > diffBright)
			mask |= 1 << 5;
		if(ABS(brightArray[7] - centerBright) > diffBright)
			mask |= 1 << 6;
		if(ABS(brightArray[8] - centerBright) > diffBright)
			mask |= 1 << 7;
	}
#define P0 dst0[0]
#define P1 dst0[1]
#define P2 dst1[0]
#define P3 dst1[1]
#define HQ2X_MUR false
#define HQ2X_MDR false
#define HQ2X_MDL false
#define HQ2X_MUL false
#define IC(p0) c[p0]
#define I11(p0,p1) hq2x_interp_32_11(c[p0], c[p1])
#define I211(p0,p1,p2) hq2x_interp_32_211(c[p0], c[p1], c[p2])
#define I31(p0,p1) hq2x_interp_32_31(c[p0], c[p1])
#define I332(p0,p1,p2) hq2x_interp_32_332(c[p0], c[p1], c[p2])
#define I431(p0,p1,p2) hq2x_interp_32_431(c[p0], c[p1], c[p2])
#define I521(p0,p1,p2) hq2x_interp_32_521(c[p0], c[p1], c[p2])
#define I53(p0,p1) hq2x_interp_32_53(c[p0], c[p1])
#define I611(p0,p1,p2) hq2x_interp_32_611(c[p0], c[p1], c[p2])
#define I71(p0,p1) hq2x_interp_32_71(c[p0], c[p1])
#define I772(p0,p1,p2) hq2x_interp_32_772(c[p0], c[p1], c[p2])
#define I97(p0,p1) hq2x_interp_32_97(c[p0], c[p1])
#define I1411(p0,p1,p2) hq2x_interp_32_1411(c[p0], c[p1], c[p2])
#define I151(p0,p1) hq2x_interp_32_151(c[p0], c[p1])

      switch (mask) {
#include "TextureFilters_hq2x.h"
      }

#undef P0
#undef P1
#undef P2
#undef P3
#undef HQ2X_MUR
#undef HQ2X_MDR
#undef HQ2X_MDL
#undef HQ2X_MUL
#undef IC
#undef I11
#undef I211
#undef I31
#undef I332
#undef I431
#undef I521
#undef I53
#undef I611
#undef I71
#undef I772
#undef I97
#undef I1411
#undef I151

      src0 += 1;
      src1 += 1;
      src2 += 1;
      dst0 += 2;
      dst1 += 2;
   }
}

void hq2x(uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height)
{
	interp_set(32);
	uint32 *dst0 = (uint32 *)dstPtr;
	uint32 *dst1 = dst0 + (dstPitch >> 2);

	uint32 *src0 = (uint32 *)srcPtr;
	uint32 *src1 = src0 + (srcPitch >> 2);
	uint32 *src2 = src1 + (srcPitch >> 2);
	hq2x_32_def(dst0, dst1, src0, src0, src1, width);
	if( height == 1 ) return;

	int count = height;

	count -= 2;
	while(count>0) {
		dst0 += dstPitch >> 1;
		dst1 += dstPitch >> 1;
		hq2x_32_def(dst0, dst1, src0, src1, src2, width);
		src0 = src1;
		src1 = src2;
		src2 += srcPitch >> 2;
		--count;
	}
	dst0 += dstPitch >> 1;
	dst1 += dstPitch >> 1;
	hq2x_32_def(dst0, dst1, src0, src1, src1, width);
}

void hq2xS(uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height)
{
   interp_set(32);
   uint32 *dst0 = (uint32 *)dstPtr;
   uint32 *dst1 = dst0 + (dstPitch >> 2);

   uint32 *src0 = (uint32 *)srcPtr;
   uint32 *src1 = src0 + (srcPitch >> 2);
   uint32 *src2 = src1 + (srcPitch >> 2);
  hq2xS_32_def(dst0, dst1, src0, src0, src1, width);
  
  int count = height;
  
  count -= 2;
  while(count) {
    dst0 += dstPitch >> 1;
    dst1 += dstPitch >> 1;
    hq2xS_32_def(dst0, dst1, src0, src1, src2, width);
    src0 = src1;
    src1 = src2;
    src2 += srcPitch >> 2;
    --count;
  }
  dst0 += dstPitch >> 1;
  dst1 += dstPitch >> 1;
  hq2xS_32_def(dst0, dst1, src0, src1, src1, width);
}