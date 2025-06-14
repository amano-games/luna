#pragma once

#include <math.h>

#include "sys-types.h"
#include "sys-utils.h"

#define PI_FLOAT    3.1415927f
#define PI2_FLOAT   6.2831853f
#define TURN_HALF   0.5f
#define RAD_TO_TURN ((f32)(TURN_HALF / PI_FLOAT))
#define TURN_TO_RAD ((f32)(PI_FLOAT / TURN_HALF))
#define DEG_TO_TURN ((f32)(1 / 360.0f))
#define DEG_TO_RAD  ((f32)(PI_FLOAT / 180))
#define EPSILON     0.000001f

static const i32 COS_TABLE[256];

static inline u64
min_u64(u64 a, u64 b)
{
	return (a < b ? a : b);
}

static inline u64
max_u64(u64 a, u64 b)
{
	return (a > b ? a : b);
}

static inline i32
clamp_i32(i32 x, i32 lo, i32 hi)
{
	if(x < lo) return lo;
	if(x > hi) return hi;
	return x;
}

static inline i32
max_i32(i32 a, i32 b)
{
	return (a > b ? a : b);
}

static inline i32
min_i32(i32 a, i32 b)
{
	return (a < b ? a : b);
}

static inline i32
sqrt_i32(i32 x)
{
	return (i32)sqrtf((f32)x);
}

static inline i32
abs_i32(i32 a)
{
	return (a < 0 ? -a : a);
}

static inline i32
sgn_i32(i32 a)
{
	if(a < 0) return -1;
	if(a > 0) return +1;
	return 0;
}

static inline f32
max_f32(f32 a, f32 b)
{
	return (a > b ? a : b);
}

static inline f32
min_f32(f32 a, f32 b)
{
	return (a < b ? a : b);
}

static inline f32
clamp_f32(f32 x, f32 lo, f32 hi)
{
	if(x < lo) return lo;
	if(x > hi) return hi;
	return x;
}

static inline f32
abs_f32(f32 a)
{
	return (a < 0 ? -a : a);
}

static inline f32
floor_f32(f32 a)
{
	return floorf(a);
}

static inline f32
ceil_f32(f32 a)
{
	return ceilf(a);
}

static inline f32
sqrt_f32(f32 x)
{
	f32 r = sqrtf(x);
	return r;
}

static inline f32
exp_f32(f32 x)
{
	f32 r = expf(x);
	return r;
}

static inline u32
sqrt_u32(u32 x)
{
	return (u32)sqrtf((f32)x);
}

static inline i32
sgn_f32(f32 a)
{
	return signbit(a) ? -1 : 1;
}

static inline f32
lerp(f32 a, f32 b, f32 t)
{
	return (1.0f - t) * a + b * t;
}

static inline f32
lerp_inv(f32 a, f32 b, f32 v)
{
	return (v - a) / (b - a);
}

static inline f32
remap(f32 i_min, f32 i_max, f32 o_min, f32 o_max, f32 v)
{
	float t = lerp_inv(i_min, i_max, v);
	return lerp(o_min, o_max, t);
}

static inline bool
f32_equal(f32 a, f32 b)
{
	return abs_f32(a - b) <= EPSILON;
}

// ANGLES

#define Q16_ANGLE_TURN 0x40000

static i32
turn_q18_calc(i32 num, i32 den)
{
	return (0x40000 * num) / den;
}

// p: angle/turn, where 2 PI or 1 turn = 262144 (0x40000)
// output: [-65536, 65536] = [-1; +1]
static i32
cos_q16(i32 turn_q18)
{
	int i   = (uint)(turn_q18 >> 8) & 0x3FF;
	int neg = 0;
	switch(i & 0x300) {                        // [0, 256)
	case 0x100: i = 0x200 - i, neg = 1; break; // [256, 512)
	case 0x200: i = i - 0x200, neg = 1; break; // [512, 768)
	case 0x300: i = 0x400 - i; break;          // [768, 1024)
	}
	if(i == 0x100) return 0;
	i32 r = COS_TABLE[i];
	return neg ? -r : r;
}

// p: angle, where 262144 = 0x40000 = 2 PI
// output: [-65536, 65536] = [-1; +1]
static inline i32
sin_q16(i32 turn_q18)
{
	return cos_q16(turn_q18 - 0x10000);
}

// p: angle, where 1024 = 0x400 = 2 PI
// output: [-64, 64] = [-1; +1]
static inline i32
cos_q6(i32 turn_q10)
{
	return (cos_q16(turn_q10 << 8) >> 10);
}

// p: angle, where 1024 = 0x400 = 2 PI
// output: [-64, 64] = [-1; +1]
static inline i32
sin_q6(i32 turn_q10)
{
	return (cos_q16((turn_q10 - 0x100) << 8) >> 10);
}

// input: [0,65536] = [0,1]
// output: [0, 32768] = [0, PI/4], 65536 = PI/2
static i32
atan_q16(i32 x)
{
	if(x == 0) return 0;

	// taylor expansion works for x [0, 1]
	// for bigger x: atan(x) = PI/2 - atan(1/x)
	i32 add = 0, mul = +1;
	u32 i = (u32)abs_i32(x);
	if(i > 0x10000) {
		i   = 0xFFFFFFFFU / i;
		add = 0x10000;
		mul = -1;
	}

	if(i == 0x10000) return (x > 0 ? +0x8000 : -0x8000); // atan(1) = PI/4
	u32 i2 = (i * i + 0x8000) >> 16;
	u32 r;                          // magic constants roughly:
	r = 0x00A97;                    // 1/13 + E
	r = 0x017AA - ((r * i2) >> 16); // 1/11
	r = 0x01CE0 - ((r * i2) >> 16); // 1/9
	r = 0x024F6 - ((r * i2) >> 16); // 1/7
	r = 0x0336F - ((r * i2) >> 16); // 1/5
	r = 0x05587 - ((r * i2) >> 16); // 1/3
	r = 0x10050 - ((r * i2) >> 16); // 1
	r = (r * i) >> 16;
	r = (r * 0x0A2FB) >> 16; // divide by PI/2 - 0x0A2FB ~ 0xFFFFFFFFu / (PI/2 << 16)

	i32 res = add + mul * (i32)r;
	return (x > 0 ? +res : -res);
}

// INPUT:  [-0x10000,0x10000] = [-1;1]
// OUTPUT: [-0x10000, 0x10000] = [-PI/2;PI/2]
static i32
asin_q16(i32 x)
{
	// ASSERT(-0x10000 <= x && x <= 0x10000);
	if(x == 0) return 0;
	if(x == +0x10000) return +0x10000;
	if(x == -0x10000) return -0x10000;
	u32 i = (u32)abs_i32(x);
	u32 r;
	r = 0x030D;
	r = 0x0C1A - ((r * i) >> 16);
	r = 0x2292 - ((r * i) >> 16);
	r = 0xFFFC - ((r * i) >> 16);
	r = sqrt_u32(((0x10000 - i) << 16) + 0x800) * r;
	r = ((0xFFFF8000U - r)) >> 16;
	return (x > 0 ? +(i32)r : -(i32)r);
}

// INPUT:  [-0x10000,0x10000] = [-1;1]
// OUTPUT: [0, 0x20000] = [0;PI]
static i32
acos_q16(i32 x)
{
	return 0x10000 - asin_q16(x);
}

static f32
pow_f32(f32 v, f32 power)
{
	f32 r = powf(v, power);
	return r;
}

static f32
sin_f32(f32 x)
{
	f32 i = fmodf(x, PI2_FLOAT) / PI2_FLOAT;
	return ((f32)sin_q16((i32)(i * (f32)Q16_ANGLE_TURN)) * .000015258789f);
}

static f32
cos_f32(f32 x)
{
	f32 i = fmodf(x, PI2_FLOAT) / PI2_FLOAT;
	return ((f32)cos_q16((i32)(i * (f32)Q16_ANGLE_TURN)) * .000015258789f);
}

static f32
atan2_f32(f32 y, f32 x)
{
	return atan2f(y, x);
}

static f32
acos_f32(f32 x)
{
	return acosf(x);
}

static i32
mod_euc_i32(i32 a, i32 b)
{
	assert(b > 0);
	i32 r = a % b;
	return r < 0 ? r + b : r;
}

static f32
rem_f32(f32 a, f32 b)
{
	assert(b > 0);
	f32 r = fmodf(a, b);
	return r;
}

static f32
mod_euc_f32(f32 a, f32 b)
{
	assert(b > 0);
	f32 r = fmodf(a, b);
	r     = r < 0 ? r + b : r;
	return r;
}

static inline union rng_u64
rng_u64(u64 min, u64 max)
{
	union rng_u64 r = {{min, max}};
	if(r.min > r.max) { SWAP(u64, r.min, r.max); }
	return r;
}

static const i32 COS_TABLE[256] = {
	0x10000, 0x0FFFF, 0x0FFFB, 0x0FFF5, 0x0FFEC, 0x0FFE1, 0x0FFD4, 0x0FFC4, 0x0FFB1, 0x0FF9C, 0x0FF85, 0x0FF6B, 0x0FF4E, 0x0FF30, 0x0FF0E, 0x0FEEB, 0x0FEC4, 0x0FE9C, 0x0FE71, 0x0FE43, 0x0FE13, 0x0FDE1, 0x0FDAC, 0x0FD74, 0x0FD3B, 0x0FCFE, 0x0FCC0, 0x0FC7F, 0x0FC3B, 0x0FBF5, 0x0FBAD, 0x0FB62, 0x0FB15, 0x0FAC5, 0x0FA73, 0x0FA1F, 0x0F9C8, 0x0F96E, 0x0F913, 0x0F8B4, 0x0F854, 0x0F7F1, 0x0F78C, 0x0F724, 0x0F6BA, 0x0F64E, 0x0F5DF, 0x0F56E, 0x0F4FA, 0x0F484, 0x0F40C, 0x0F391, 0x0F314, 0x0F295, 0x0F213, 0x0F18F, 0x0F109, 0x0F080, 0x0EFF5, 0x0EF68, 0x0EED9, 0x0EE47, 0x0EDB3, 0x0ED1C, 0x0EC83, 0x0EBE8, 0x0EB4B, 0x0EAAB, 0x0EA0A, 0x0E966, 0x0E8BF, 0x0E817, 0x0E76C, 0x0E6BF, 0x0E60F, 0x0E55E, 0x0E4AA, 0x0E3F4, 0x0E33C, 0x0E282, 0x0E1C6, 0x0E107, 0x0E046, 0x0DF83, 0x0DEBE, 0x0DDF7, 0x0DD2D, 0x0DC62, 0x0DB94, 0x0DAC4, 0x0D9F2, 0x0D91E, 0x0D848, 0x0D770, 0x0D696, 0x0D5B9, 0x0D4DB, 0x0D3FB, 0x0D318, 0x0D234, 0x0D14D, 0x0D065, 0x0CF7A, 0x0CE8D, 0x0CD9F, 0x0CCAE, 0x0CBBC, 0x0CAC7, 0x0C9D1, 0x0C8D9, 0x0C7DE, 0x0C6E2, 0x0C5E4, 0x0C4E4, 0x0C3E2, 0x0C2DE, 0x0C1D8, 0x0C0D1, 0x0BFC7, 0x0BEBC, 0x0BDAF, 0x0BCA0, 0x0BB8F, 0x0BA7D, 0x0B968, 0x0B852, 0x0B73A, 0x0B620, 0x0B505, 0x0B3E8, 0x0B2C9, 0x0B1A8, 0x0B086, 0x0AF61, 0x0AE3C, 0x0AD14, 0x0ABEB, 0x0AAC0, 0x0A994, 0x0A866, 0x0A736, 0x0A605, 0x0A4D2, 0x0A39D, 0x0A267, 0x0A130, 0x09FF7, 0x09EBC, 0x09D80, 0x09C42, 0x09B03, 0x099C2, 0x09880, 0x0973C, 0x095F7, 0x094B0, 0x09368, 0x0921E, 0x090D4, 0x08F87, 0x08E3A, 0x08CEB, 0x08B9A, 0x08A48, 0x088F5, 0x087A1, 0x0864B, 0x084F4, 0x0839C, 0x08242, 0x080E8, 0x07F8C, 0x07E2E, 0x07CD0, 0x07B70, 0x07A0F, 0x078AD, 0x0774A, 0x075E6, 0x07480, 0x07319, 0x071B2, 0x07049, 0x06EDF, 0x06D74, 0x06C08, 0x06A9B, 0x0692D, 0x067BE, 0x0664D, 0x064DC, 0x0636A, 0x061F7, 0x06083, 0x05F0E, 0x05D98, 0x05C22, 0x05AAA, 0x05932, 0x057B8, 0x0563E, 0x054C3, 0x05347, 0x051CB, 0x0504D, 0x04ECF, 0x04D50, 0x04BD0, 0x04A50, 0x048CF, 0x0474D, 0x045CA, 0x04447, 0x042C3, 0x0413F, 0x03FB9, 0x03E34, 0x03CAD, 0x03B26, 0x0399F, 0x03817, 0x0368E, 0x03505, 0x0337B, 0x031F1, 0x03066, 0x02EDB, 0x02D50, 0x02BC4, 0x02A37, 0x028AB, 0x0271D, 0x02590, 0x02402, 0x02273, 0x020E5, 0x01F56, 0x01DC7, 0x01C37, 0x01AA7, 0x01917, 0x01787, 0x015F6, 0x01466, 0x012D5, 0x01144, 0x00FB2, 0x00E21, 0x00C8F, 0x00AFE, 0x0096C, 0x007DA, 0x00648, 0x004B6, 0x00324, 0x00192};
