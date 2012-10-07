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

//************************************************************************/
/* Sharpen filters                                                      */
/************************************************************************/
void SharpenFilter_32(uint32 *pdata, uint32 width, uint32 height, uint32 pitch, uint32 filter)
{
	uint32 len=height*pitch;
	uint32 *pcopy = new uint32[len];

	if( !pcopy )	return;

	memcpy(pcopy, pdata, len<<2);

	uint32 mul1, mul2, mul3, shift4;
	switch( filter )
	{
	case TEXTURE_SHARPEN_MORE_ENHANCEMENT:
		mul1=1;
		mul2=8;
		mul3=12;
		shift4=2;
		break;
	case TEXTURE_SHARPEN_ENHANCEMENT:
	default:
		mul1=1;
		mul2=8;
		mul3=16;
		shift4=3;
		break;
	}

	uint32 x,y,z;
	uint32 *src1, *src2, *src3, *dest;
	uint32 val[4];
	uint32 t1,t2,t3,t4,t5,t6,t7,t8,t9;

	for( y=1; y<height-1; y++)
	{
		dest = pdata+y*pitch;
		src1 = pcopy+(y-1)*pitch;
		src2 = src1 + pitch;
		src3 = src2 + pitch;
		for( x=1; x<width-1; x++)
		{
			for( z=0; z<4; z++ )
			{
				t1 = *((uint8*)(src1+x-1)+z);
				t2 = *((uint8*)(src1+x  )+z);
				t3 = *((uint8*)(src1+x+1)+z);
				t4 = *((uint8*)(src2+x-1)+z);
				t5 = *((uint8*)(src2+x  )+z);
				t6 = *((uint8*)(src2+x+1)+z);
				t7 = *((uint8*)(src3+x-1)+z);
				t8 = *((uint8*)(src3+x  )+z);
				t9 = *((uint8*)(src3+x+1)+z);
				val[z]=t5;
				if( (t5*mul2) > (t1+t3+t7+t9+t2+t4+t6+t8)*mul1 )
				{
					val[z]= min((((t5*mul3) - (t1+t3+t7+t9+t2+t4+t6+t8)*mul1)>>shift4),0xFF);
				}
			}
			dest[x] = val[0]|(val[1]<<8)|(val[2]<<16)|(val[3]<<24);
		}
	}
	delete [] pcopy;
}

void SharpenFilter_16(uint16 *pdata, uint32 width, uint32 height, uint32 pitch, uint32 filter)
{
	//return;	// Sharpen does not make sense for 16 bits

	uint32 len=height*pitch;
	uint16 *pcopy = new uint16[len];

	if( !pcopy )	return;

	memcpy(pcopy, pdata, len<<1);

	uint16 mul1, mul2, mul3, shift4;
	switch( filter )
	{
	case TEXTURE_SHARPEN_MORE_ENHANCEMENT:
		mul1=1;
		mul2=8;
		mul3=12;
		shift4=2;
		break;
	case TEXTURE_SHARPEN_ENHANCEMENT:
	default:
		mul1=1;
		mul2=8;
		mul3=16;
		shift4=3;
		break;
	}

	uint32 x,y,z;
	uint16 *src1, *src2, *src3, *dest;
	uint16 val[4];
	uint16 t1,t2,t3,t4,t5,t6,t7,t8,t9;

	for( y=1; y<height-1; y++)
	{
		dest = pdata+y*pitch;
		src1 = pcopy+(y-1)*pitch;
		src2 = src1 + pitch;
		src3 = src2 + pitch;
		for( x=1; x<width-1; x++)
		{
			for( z=0; z<4; z++ )
			{
				uint32 shift = (z%1)?4:0;
				t1 = (*((uint8*)(src1+x-1)+(z>>1)))>>shift;
				t2 = (*((uint8*)(src1+x  )+(z>>1)))>>shift;
				t3 = (*((uint8*)(src1+x+1)+(z>>1)))>>shift;
				t4 = (*((uint8*)(src2+x-1)+(z>>1)))>>shift;
				t5 = (*((uint8*)(src2+x  )+(z>>1)))>>shift;
				t6 = (*((uint8*)(src2+x+1)+(z>>1)))>>shift;
				t7 = (*((uint8*)(src3+x-1)+(z>>1)))>>shift;
				t8 = (*((uint8*)(src3+x  )+(z>>1)))>>shift;
				t9 = (*((uint8*)(src3+x+1)+(z>>1)))>>shift;
				val[z]=t5;
				if( (t5*mul2) > (t1+t3+t7+t9+t2+t4+t6+t8)*mul1 )
				{
					val[z] = (((t5*mul3) - (t1+t3+t7+t9+t2+t4+t6+t8)*mul1)>>shift4);
					val[z]= min(val[z],0xF);
				}
			}
			dest[x] = val[0]|(val[1]<<4)|(val[2]<<8)|(val[3]<<12);
		}
	}
	delete [] pcopy;
}

void SharpenFilter(uint32 *pdata, uint32 width, uint32 height, uint32 pitch, uint32 filter, bool bPixelSize4)
{
	if(bPixelSize4)
		SharpenFilter_32((uint32*)pdata, width, height, pitch, filter);
	else
		SharpenFilter_16((uint16*)pdata, width, height, pitch, filter);
}