/***************************************************************************/
/* Basic types */

/***************************************************************************/
/* interpolation */

static unsigned interp_bits_per_pixel;
#define INTERP_32_MASK_1_3(v) ((v)&0x00FF00FF)
#define INTERP_32_MASK_SHIFT_2_4(v) (((v)&0xFF00FF00)>>8)
#define INTERP_32_MASK_SHIFTBACK_2_4(v) (((INTERP_32_MASK_1_3(v))<<8))

static inline uint32 hq2x_interp_32_521(uint32 p1, uint32 p2, uint32 p3)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*5 + INTERP_32_MASK_1_3(p2)*2 + INTERP_32_MASK_1_3(p3)*1) / 8)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*5 + INTERP_32_MASK_SHIFT_2_4(p2)*2 + INTERP_32_MASK_SHIFT_2_4(p3)*1) / 8);
}

static inline uint32 hq2x_interp_32_332(uint32 p1, uint32 p2, uint32 p3)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*3 + INTERP_32_MASK_1_3(p2)*3 + INTERP_32_MASK_1_3(p3)*2) / 8)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*3 + INTERP_32_MASK_SHIFT_2_4(p2)*3 + INTERP_32_MASK_SHIFT_2_4(p3)*2) / 8);
}

static inline uint32 hq2x_interp_32_211(uint32 p1, uint32 p2, uint32 p3)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*2 + INTERP_32_MASK_1_3(p2) + INTERP_32_MASK_1_3(p3)) / 4)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*2 + INTERP_32_MASK_SHIFT_2_4(p2) + INTERP_32_MASK_SHIFT_2_4(p3)) / 4);
}

static inline uint32 hq2x_interp_32_611(uint32 p1, uint32 p2, uint32 p3)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*6 + INTERP_32_MASK_1_3(p2) + INTERP_32_MASK_1_3(p3)) / 8)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*6 + INTERP_32_MASK_SHIFT_2_4(p2) + INTERP_32_MASK_SHIFT_2_4(p3)) / 8);
}

static inline uint32 hq2x_interp_32_71(uint32 p1, uint32 p2)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*7 + INTERP_32_MASK_1_3(p2)) / 8)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*7 + INTERP_32_MASK_SHIFT_2_4(p2)) / 8);
}

static inline uint32 hq2x_interp_32_772(uint32 p1, uint32 p2, uint32 p3)
{
	return INTERP_32_MASK_1_3(((INTERP_32_MASK_1_3(p1) + INTERP_32_MASK_1_3(p2))*7 + INTERP_32_MASK_1_3(p3)*2) / 16)
		| INTERP_32_MASK_SHIFTBACK_2_4(((INTERP_32_MASK_SHIFT_2_4(p1) + INTERP_32_MASK_SHIFT_2_4(p2))*7 + INTERP_32_MASK_SHIFT_2_4(p3)*2) / 16);
}

static inline uint32 hq2x_interp_32_11(uint32 p1, uint32 p2)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1) + INTERP_32_MASK_1_3(p2)) / 2)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1) + INTERP_32_MASK_SHIFT_2_4(p2)) / 2);
}

static inline uint32 hq2x_interp_32_31(uint32 p1, uint32 p2)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*3 + INTERP_32_MASK_1_3(p2)) / 4)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*3 + INTERP_32_MASK_SHIFT_2_4(p2)) / 4);
}

static inline uint32 hq2x_interp_32_1411(uint32 p1, uint32 p2, uint32 p3)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*14 + INTERP_32_MASK_1_3(p2) + INTERP_32_MASK_1_3(p3)) / 16)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*14 + INTERP_32_MASK_SHIFT_2_4(p2) + INTERP_32_MASK_SHIFT_2_4(p3)) / 16);
}

static inline uint32 hq2x_interp_32_431(uint32 p1, uint32 p2, uint32 p3)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*4 + INTERP_32_MASK_1_3(p2)*3 + INTERP_32_MASK_1_3(p3)) / 8)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*4 + INTERP_32_MASK_SHIFT_2_4(p2)*3 + INTERP_32_MASK_SHIFT_2_4(p3)) / 8);
}

static inline uint32 hq2x_interp_32_53(uint32 p1, uint32 p2)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*5 + INTERP_32_MASK_1_3(p2)*3) / 8)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*5 + INTERP_32_MASK_SHIFT_2_4(p2)*3) / 8);
}

static inline uint32 hq2x_interp_32_151(uint32 p1, uint32 p2)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*15 + INTERP_32_MASK_1_3(p2)) / 16)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*15 + INTERP_32_MASK_SHIFT_2_4(p2)) / 16);
}

static inline uint32 hq2x_interp_32_97(uint32 p1, uint32 p2)
{
	return INTERP_32_MASK_1_3((INTERP_32_MASK_1_3(p1)*9 + INTERP_32_MASK_1_3(p2)*7) / 16)
		| INTERP_32_MASK_SHIFTBACK_2_4((INTERP_32_MASK_SHIFT_2_4(p1)*9 + INTERP_32_MASK_SHIFT_2_4(p2)*7) / 16);
}

/***************************************************************************/
/* diff */

#define INTERP_Y_LIMIT (0x30*4)
#define INTERP_U_LIMIT (0x07*4)
#define INTERP_V_LIMIT (0x06*8)

static int hq2x_interp_32_diff(uint32 p1, uint32 p2)
{
	int r, g, b;
	int y, u, v;

	if ((p1 & 0xF8F8F8) == (p2 & 0xF8F8F8))
		return 0;

	b = (int)((p1 & 0xFF) - (p2 & 0xFF));
	g = (int)((p1 & 0xFF00) - (p2 & 0xFF00)) >> 8;
	r = (int)((p1 & 0xFF0000) - (p2 & 0xFF0000)) >> 16;

	y = r + g + b;
	u = r - b;
	v = -r + 2*g - b;

	if (y < -INTERP_Y_LIMIT || y > INTERP_Y_LIMIT)
		return 1;

	if (u < -INTERP_U_LIMIT || u > INTERP_U_LIMIT)
		return 1;

	if (v < -INTERP_V_LIMIT || v > INTERP_V_LIMIT)
		return 1;

	return 0;
}

static void interp_set(unsigned bits_per_pixel)
{
	interp_bits_per_pixel = bits_per_pixel;
}