// Pixel 1
pDst1[xSrc*2] = pSrc[xSrc];

// Pixel 2
if( xSrc<nWidth-1 )
{
	pDst1[xSrc*2+1] = DWORD_MAKE((r1+r2)/2, (g1+g2)/2, (b1+b2)/2, (a1+a2)/2);
}
else
pDst1[xSrc*2+1] = pSrc[xSrc];


// Pixel 3
if( ySrc<nHeight-1 )
{
	pDst2[xSrc*2] = DWORD_MAKE((r1+r3)/2, (g1+g3)/2, (b1+b3)/2, (a1+a3)/2);
}
else
pDst2[xSrc*2] = pSrc[xSrc];

// Pixel 4
if( xSrc<nWidth-1 )
{
	if( ySrc<nHeight-1 )
	{
	  pDst2[xSrc*2+1] = DWORD_MAKE((r1+r2+r3+r4)/4, (g1+g2+g3+g4)/4, (b1+b2+b3+b4)/4, (a1+a2+a3+a4)/4);
	}
	else
	{
	  pDst2[xSrc*2+1] = DWORD_MAKE((r1+r2)/2, (g1+g2)/2, (b1+b2)/2, (a1+a2)/2);
	}
}
else
{
	if( ySrc<nHeight-1 )
	{
	  pDst2[xSrc*2+1] = DWORD_MAKE((r1+r3)/2, (g1+g3)/2, (b1+b3)/2, (a1+a3)/2);
	}
	else
	pDst2[xSrc*2+1] = pSrc[xSrc];
}