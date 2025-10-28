/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __MATH_MATH_H__
#define __MATH_MATH_H__

#pragma once

#include <bit>
#include <limits>
#include <cmath>
#include <concepts>
#include <functional>
#include <safeint.h>

#include "idlib/sys/sys_helpers.h"

#ifdef MACOS_X
// for square root estimate instruction
#include <ppc_intrinsics.h>
// for FLT_MIN
#include <float.h>
#endif



/*
===============================================================================

  Math

===============================================================================
*/
#ifdef INFINITY
#undef INFINITY
#endif

#ifdef FLT_EPSILON
#undef FLT_EPSILON
#endif

#define DEG2RAD(a)				( (a) * idMath::M_DEG2RAD )
#define RAD2DEG(a)				( (a) * idMath::M_RAD2DEG )

#define SEC2MS(t)				( numeric_cast<ID_TIME_T>( (t) * idMath::M_SEC2MS ) )
#define MS2SEC(t)				( (t) * idMath::M_MS2SEC )

#define	ANGLE2SHORT(x)			( numeric_cast<int>( (x) * 65536.0f / 360.0f ) & 65535 )
#define	SHORT2ANGLE(x)			( (x) * ( 360.0f / 65536.0f ) )

#define	ANGLE2BYTE(x)			( numeric_cast<int>( (x) * 256.0f / 360.0f ) & 255 )
#define	BYTE2ANGLE(x)			( (x) * ( 360.0f / 256.0f ) )

//#define C_FLOAT_TO_INT( x )		(int)(x)

/*
================================================================================================

	Safe integral to floating point conversion

================================================================================================
*/

#if defined(_MSC_VER)
#define I2F_FORCEINLINE __forceinline
#define I2F_RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
#define I2F_FORCEINLINE __attribute__((always_inline)) inline
#define I2F_RESTRICT __restrict__
#else
#define I2F_FORCEINLINE inline
#define I2F_RESTRICT
#endif

namespace idMath_i2f {
	// Normalize first
	template <class T>
	using decay_t = std::remove_cv_t<std::remove_reference_t<T>>;

	// base_int<T>: maps enums -> underlying; else identity (done via partial specialization)
	template <class T, bool IsEnum = std::is_enum_v<decay_t<T>>>
	struct base_int { using type = decay_t<T>; };

	template <class T>
	struct base_int<T, /*IsEnum=*/true> { using type = std::underlying_type_t<decay_t<T>>; };

	template <class T>
	using base_int_t = typename base_int<T>::type;

	// Concept: integral or enum-with-integral-underlying (after decay)
	template <class T>
	concept integral_like = std::is_integral_v<base_int_t<T>>;

	// Convenience traits
	template <class T>
	inline constexpr bool is_signed_integral_like_v =
		std::is_signed_v<base_int_t<T>>;

	// ---------- compile-time traits for destination FP type ----------
	template <std::floating_point F>
	struct Traits {
		// Number of base-2 precision bits in F (incl. implicit 1)
		static constexpr int precision_bits = std::numeric_limits<F>::digits;

		// Largest integer magnitude guaranteed to be exactly representable by F.
		// (= 2^precision_bits; avoid shifting by full width of uintmax_t)
		static constexpr std::uintmax_t exact_limit = []() consteval {
			constexpr int P = precision_bits;
			constexpr int UM_BITS = std::numeric_limits<std::uintmax_t>::digits; // typically 64
			if constexpr (P >= UM_BITS) {
				return (std::numeric_limits<std::uintmax_t>::max)(); // any uintmax_t fits
			}
			else {
				return (std::uintmax_t{ 1 } << P); // 2^P
			}
			}();
	};

	// unsigned magnitude of any integral (handles negatives & INT_MIN safely)
	template <integral_like T>
	constexpr std::uintmax_t umagnitude(T v) noexcept {
		using B = base_int_t<T>;
		using UB = std::make_unsigned_t<std::remove_cv_t<B>>;
		if constexpr (std::is_signed_v<B>) {
			const UB uv = static_cast<UB>(static_cast<B>(v));
			return (static_cast<B>(v) < 0) ? std::uintmax_t(UB(0) - uv)
				: std::uintmax_t(uv);
		}
		else {
			return std::uintmax_t(static_cast<UB>(static_cast<B>(v)));
		}
	}

	// handy compile-time constants
	template <std::floating_point F>
	inline constexpr int kPrecisionBits = Traits<F>::precision_bits;

	template <std::floating_point F>
	inline constexpr std::uintmax_t kExactLimit = Traits<F>::exact_limit;

	enum class Eu64 : size_t { A = 0, B = 1 };
} // namespace idMath_i2f

static_assert(idMath_i2f::integral_like<size_t>);
static_assert(idMath_i2f::integral_like<uint64>);
static_assert(idMath_i2f::integral_like<int64>);
static_assert(idMath_i2f::integral_like<unsigned long long>);
static_assert(idMath_i2f::integral_like<idMath_i2f::Eu64>);
static_assert(idMath_i2f::integral_like<const volatile unsigned long long>);
static_assert(idMath_i2f::integral_like<bool>);

/*
================================================================================================

	Safe double to float conversion

================================================================================================
*/

#ifndef SAFE_D2F_SATURATE_OVERFLOW
#define SAFE_D2F_SATURATE_OVERFLOW 1
#endif

#ifndef SAFE_D2F_ASSERT_ON_NAN
#ifdef _DEBUG
#define SAFE_D2F_ASSERT_ON_NAN 1
#else
#define SAFE_D2F_ASSERT_ON_NAN 0
#endif // _DEBUG
#endif // SAFE_D2F_ASSERT_ON_NAN

namespace idMath_d2f {
	// IEEE-754 double layout (compile-time constants)
	inline constexpr uint64 SIGN_MASK  = 0x8000'0000'0000'0000ull;
	inline constexpr uint64 EXP_MASK   = 0x7FF0'0000'0000'0000ull;
	inline constexpr uint64 FRAC_MASK  = 0x000F'FFFF'FFFF'FFFFull;
	inline constexpr int    EXP_BIAS   = 1023;
	inline constexpr int    EXP_INF    = 0x7FF;         // 2047
	inline constexpr int    F32_MAX_EU = 127;           // float max unbiased exponent

	// Precache float scalars
	inline constexpr float  F32_MAX    = (std::numeric_limits<float>::max)();
	inline constexpr float  PINF       = std::numeric_limits<float>::infinity();
	inline constexpr float  NINF       = -std::numeric_limits<float>::infinity();
	inline constexpr float  QNAN       = std::numeric_limits<float>::quiet_NaN();
}

/*
================================================================================================

	two-complements integer bit layouts

================================================================================================
*/

constexpr auto INT8_SIGN_BIT = 7;
constexpr auto INT16_SIGN_BIT = 15;
constexpr auto INT32_SIGN_BIT = 31;
constexpr auto INT64_SIGN_BIT = 63;

#define INT8_SIGN_MASK		( 1 << INT8_SIGN_BIT )
#define INT16_SIGN_MASK		( 1 << INT16_SIGN_BIT )
#define INT32_SIGN_MASK		( 1UL << INT32_SIGN_BIT )
#define INT64_SIGN_MASK		( 1ULL << INT64_SIGN_BIT )

/*
================================================================================================

	integer sign bit tests

================================================================================================
*/

// If this was ever compiled on a system that had 64 bit unsigned ints,
// it would fail.
compile_time_assert(sizeof(unsigned int) == 4);// Why is this here?

// NOTE: These are really bool but are treated as int32 throughout the library, so we return int32 instead

[[nodiscard]] ID_INLINE_EXTERN constexpr int32 INTEGER_SIGN_BIT_IS_SET( const std::unsigned_integral auto v ) noexcept {
	using Type = decltype(v);
	constexpr int sign_bit = std::numeric_limits<Type>::digits - 1;
	return static_cast<int32>(((v >> sign_bit) & Type { 1 }) != Type{ 0 });
}

[[nodiscard]] ID_INLINE_EXTERN constexpr int32 INTEGER_SIGN_BIT_IS_NOT_SET( const std::unsigned_integral auto v ) noexcept {
	return !INTEGER_SIGN_BIT_IS_SET(v);
}

[[nodiscard]] ID_INLINE_EXTERN constexpr int32 INTEGER_SIGN_BIT_IS_SET( const std::signed_integral auto v ) noexcept {
	using UnsignedType = std::make_unsigned_t<decltype(v)>;
	const UnsignedType u = numeric_cast<UnsignedType>(v);

	return INTEGER_SIGN_BIT_IS_SET(u);
}

[[nodiscard]] ID_INLINE_EXTERN constexpr int32 INTEGER_SIGN_BIT_IS_NOT_SET( const std::signed_integral auto v ) noexcept {
	// exact logical complement keeps the intent clear and still constant-folds
	return !INTEGER_SIGN_BIT_IS_SET(v);
}

/*
================================================================================================

	floating point bit layouts according to the IEEE 754-1985 and 754-2008 standard

================================================================================================
*/

constexpr auto IEEE_FLT16_MANTISSA_BITS = 10;
constexpr auto IEEE_FLT16_EXPONENT_BITS = 5;
constexpr auto IEEE_FLT16_EXPONENT_BIAS = 15;
constexpr auto IEEE_FLT16_SIGN_BIT = 15;
#define IEEE_FLT16_SIGN_MASK		( 1U << IEEE_FLT16_SIGN_BIT )

constexpr auto IEEE_FLT_MANTISSA_BITS = 23;
constexpr auto IEEE_FLT_EXPONENT_BITS = 8;
constexpr auto IEEE_FLT_EXPONENT_BIAS = 127;
constexpr auto IEEE_FLT_SIGN_BIT = 31;
#define IEEE_FLT_SIGN_MASK			( 1UL << IEEE_FLT_SIGN_BIT )

constexpr auto IEEE_DBL_MANTISSA_BITS = 52;
constexpr auto IEEE_DBL_EXPONENT_BITS = 11;
constexpr auto IEEE_DBL_EXPONENT_BIAS = 1023;
constexpr auto IEEE_DBL_SIGN_BIT = 63;
#define IEEE_DBL_SIGN_MASK			( 1ULL << IEEE_DBL_SIGN_BIT )

constexpr auto IEEE_DBLE_MANTISSA_BITS = 63;
constexpr auto IEEE_DBLE_EXPONENT_BITS = 15;
constexpr auto IEEE_DBLE_EXPONENT_BIAS = 0;
constexpr auto IEEE_DBLE_SIGN_BIT = 79;

constexpr int SMALLEST_NON_DENORMAL_FLT = 1 << IEEE_FLT_MANTISSA_BITS;
constexpr auto SMALLEST_NON_DENORMAL_DBL = 1ULL << IEEE_DBL_MANTISSA_BITS;
constexpr int NAN_VALUE = 0x7f800000;

/*
================================================================================================

	floating point sign bit tests

================================================================================================
*/

#define IEEE_FLT_SIGNBITSET( a )	(reinterpret_cast<const unsigned int &>(a) >> IEEE_FLT_SIGN_BIT)
#define IEEE_FLT_SIGNBITNOTSET( a )	((~reinterpret_cast<const unsigned int &>(a)) >> IEEE_FLT_SIGN_BIT)
#define IEEE_FLT_ISNOTZERO( a )		(reinterpret_cast<const unsigned int &>(a) & ~(1u<<IEEE_FLT_SIGN_BIT))

/*
================================================================================================

	floating point special value tests

================================================================================================
*/

/*
========================
IEEE_FLT_IS_NAN
========================
*/
ID_INLINE_EXTERN constexpr bool IEEE_FLT_IS_NAN(const std::floating_point auto x ) {
	//return x != x;
	return std::isnan(x);
}

/*
========================
IEEE_FLT_IS_INF
========================
*/
ID_INLINE_EXTERN constexpr bool IEEE_FLT_IS_INF(const std::floating_point auto x ) {
	//return x == x && x * 0 != x * 0;
	return std::isinf(x);
}

/*
========================
IEEE_FLT_IS_INF_NAN
========================
*/
ID_INLINE_EXTERN constexpr bool IEEE_FLT_IS_INF_NAN(const std::floating_point auto x ) {
	//return x * 0 != x * 0;
	return std::isinf(x) && std::isnan(x);
}

/*
========================
IEEE_FLT_IS_IND
========================
*/
ID_INLINE_EXTERN constexpr bool IEEE_FLT_IS_IND(const std::floating_point auto x ) {
	//return	(reinterpret_cast<const unsigned int &>(x) == 0xffc00000);
	// functionally identical to isnan(), but separated semantically
	return std::isnan(x);
}

/*
========================
IEEE_FLT_IS_DENORMAL
========================
*/
ID_INLINE_EXTERN constexpr bool IEEE_FLT_IS_DENORMAL(const std::floating_point auto x ) {
	/*return ((reinterpret_cast<const unsigned int &>(x) & 0x7f800000) == 0x00000000 &&
			(reinterpret_cast<const unsigned int &>(x) & 0x007fffff) != 0x00000000 ); */
	return std::fpclassify(x) == FP_SUBNORMAL;
}


/*
========================
IsNAN
========================
*/template<class type>
ID_INLINE_EXTERN bool IsNAN( const type &v ) {
	for ( size_t i = 0; i < v.GetDimension(); i++ ) {
		const float f = v.ToFloatPtr()[i];
		if ( IEEE_FLT_IS_NAN( f ) || IEEE_FLT_IS_INF( f ) || IEEE_FLT_IS_IND( f ) ) {
			return true;
		}
	}
	return false;
}

/*
========================
IsValid
========================
*/
template<class type>
ID_INLINE_EXTERN bool IsValid( const type &v ) {
	for ( size_t i = 0; i < v.GetDimension(); i++ ) {
		const float f = v.ToFloatPtr()[i];
		if ( IEEE_FLT_IS_NAN( f ) || IEEE_FLT_IS_INF( f ) || IEEE_FLT_IS_IND( f ) || IEEE_FLT_IS_DENORMAL( f ) ) {
			return false;
		}
	}
	return true;
}

/*
========================
IsValid
========================
*/
//template<>
ID_INLINE_EXTERN bool IsValid( const std::floating_point auto & f ) {	// these parameter must be a reference for the function to be considered a specialization
	return !( IEEE_FLT_IS_NAN( f ) || IEEE_FLT_IS_INF( f ) || IEEE_FLT_IS_IND( f ) || IEEE_FLT_IS_DENORMAL( f ) );
}

/*
========================
IsNAN
========================
*/
//template<>
ID_INLINE_EXTERN bool IsNAN( const std::floating_point auto & f ) {	// these parameter must be a reference for the function to be considered a specialization
	if ( IEEE_FLT_IS_NAN( f ) || IEEE_FLT_IS_INF( f ) || IEEE_FLT_IS_IND( f ) ) {
		return true;
	}
	return false;
}

/*
========================
IsInRange

Returns true if any scalar is greater than the range or less than the negative range.
========================
*/
template<class type>
ID_INLINE_EXTERN bool IsInRange( const type &v, const float range ) {
	for ( size_t i = 0; i < v.GetDimension(); i++ ) {
		const float f = v.ToFloatPtr()[i];
		if ( f > range || f < -range ) {
			return false;
		}
	}
	return true;
}


/*
================================================================================================

	MinIndex/MaxIndex

================================================================================================
*/
template<class T> ID_INLINE int	MaxIndex( T x, T y ) { return  ( x > y ) ? 0 : 1; }
template<class T> ID_INLINE int	MinIndex( T x, T y ) { return ( x < y ) ? 0 : 1; }

template<class T> ID_INLINE T	Max3( T x, T y, T z ) { return ( x > y ) ? ( ( x > z ) ? x : z ) : ( ( y > z ) ? y : z ); }
template<class T> ID_INLINE T	Min3( T x, T y, T z ) { return ( x < y ) ? ( ( x < z ) ? x : z ) : ( ( y < z ) ? y : z ); }
template<class T> ID_INLINE int	Max3Index( T x, T y, T z ) { return ( x > y ) ? ( ( x > z ) ? 0 : 2 ) : ( ( y > z ) ? 1 : 2 ); }
template<class T> ID_INLINE int	Min3Index( T x, T y, T z ) { return ( x < y ) ? ( ( x < z ) ? 0 : 2 ) : ( ( y < z ) ? 1 : 2 ); }

/*
================================================================================================

	Sign/Square/Cube

================================================================================================
*/
template<class T> ID_INLINE T	Sign( T f ) { return ( f > 0 ) ? 1 : ( ( f < 0 ) ? -1 : 0 ); }
template<class T> ID_INLINE T	Square( T x ) { return x * x; }
template<class T> ID_INLINE T	Cube( T x ) { return x * x * x; }

namespace idMathInternal
{
	template <class T>
	concept Floatish = std::is_floating_point_v<T>;

	template <Floatish T>
	consteval T pow2i(int n) {
		T x = T{ 1 };
		if (n >= 0) { while (n--)
			{
				x *= T{ 2 };
			}
		}
		else { while (n++)
			{
				x /= T{ 2 };
			}
		}
		return x;
	}

	// For IEEE-754, the smallest *normalized* value is radix^(min_exponent-1).
	// This stays fully constant-evaluated in C++20.
	template <Floatish T>
	consteval T min_normal_ieee() {
		static_assert(std::numeric_limits<T>::radix == 2, "Non-binary radix not supported here");
		static_assert(std::numeric_limits<T>::is_iec559, "Assumes IEEE-754 (IEC 559)");
		return pow2i<T>(std::numeric_limits<T>::min_exponent - 1);
	}
}

class idMath {
public:

	static void					Init();

	static float				InvSqrt( float x );			// inverse square root with 32 bits precision, returns huge number when x == 0.0
	static double				InvSqrt( double x );		// inverse square root with 64 bits precision, returns huge number when x == 0.0
	static float				InvSqrt16( float x );		// inverse square root with 16 bits precision, returns huge number when x == 0.0

	static float				Sqrt( float x );			// square root with 32 bits precision
	static double				Sqrt( double x );			// square root with 64 bits precision
	static float				Sqrt16( float x );			// square root with 16 bits precision

	static float				Sin( float a );				// sine with 32 bits precision
	static float				Sin16( float a );			// sine with 16 bits precision, maximum absolute error is 2.3082e-09

	static float				Cos( float a );				// cosine with 32 bits precision
	static float				Cos16( float a );			// cosine with 16 bits precision, maximum absolute error is 2.3082e-09

	static void					SinCos( float a, float &s, float &c );		// sine and cosine with 32 bits precision
	static void					SinCos16( float a, float &s, float &c );	// sine and cosine with 16 bits precision

	static float				Tan( float a );				// tangent with 32 bits precision
	static float				Tan16( float a );			// tangent with 16 bits precision, maximum absolute error is 1.8897e-08

	static float				ASin( float a );			// arc sine with 32 bits precision, input is clamped to [-1, 1] to avoid a silent NaN
	static float				ASin16( float a );			// arc sine with 16 bits precision, maximum absolute error is 6.7626e-05

	static float				ACos( float a );			// arc cosine with 32 bits precision, input is clamped to [-1, 1] to avoid a silent NaN
	static float				ACos16( float a );			// arc cosine with 16 bits precision, maximum absolute error is 6.7626e-05

	static float				ATan( float a );			// arc tangent with 32 bits precision
	static float				ATan16( float a );			// arc tangent with 16 bits precision, maximum absolute error is 1.3593e-08

	static float				ATan( float y, float x );	// arc tangent with 32 bits precision
	static float				ATan16( float y, float x );	// arc tangent with 16 bits precision, maximum absolute error is 1.3593e-08

	static float				Pow( float x, float y );	// x raised to the power y with 32 bits precision
	static float				Pow16( float x, float y );	// x raised to the power y with 16 bits precision

	static float				Exp( float f );				// e raised to the power f with 32 bits precision
	static float				Exp16( float f );			// e raised to the power f with 16 bits precision

	static float				Log( float f );				// natural logarithm with 32 bits precision
	static float				Log16( float f );			// natural logarithm with 16 bits precision

	static auto					IPow( const std::integral auto x, const std::integral auto power );		// integral x raised to the power y
	static int					ILog2( float f );			// integral base-2 logarithm of the floating point value
	static int					ILog2( int i );				// integral base-2 logarithm of the integer value

	static size_t				BitsForFloat( const std::floating_point auto f );	// minimum number of bits required to represent ceil( f )
	static size_t				BitsForInteger( const Ordinal auto i );	// minimum number of bits required to represent i
	static int					MaskForFloatSign( float f );// returns 0x00000000 if x >= 0.0f and returns 0xFFFFFFFF if x <= -0.0f
	static int					MaskForIntegerSign( int i );// returns 0x00000000 if x >= 0 and returns 0xFFFFFFFF if x < 0
	static auto					FloorPowerOfTwo(const Numeric auto v) noexcept;	// round v down to the nearest power of 2
	static auto					CeilPowerOfTwo(const Numeric auto v) noexcept;	// round v up to the nearest power of 2
	static constexpr bool       IsPowerOfTwo( const Numeric auto v ) noexcept; // returns true if v is a power of 2
	static int					BitCount( int x );			// returns the number of 1 bits in x
	static int					BitReverse( int x );		// returns the bit reverse of x

	static int					Abs( int x );				// returns the absolute value of the integer value (for reference only)
	static float				Fabs( float f );			// returns the absolute value of the floating point value
	static double				Fabs( double f );			// returns the absolute value of the floating point value
	static auto			     	Floor( const std::floating_point auto f );			// returns the largest integer that is less than or equal to the given value
	static auto 				Ceil( const std::floating_point auto f );			// returns the smallest integer that is greater than or equal to the given value
	static float				Rint( float f );			// returns the nearest integer

	static auto 				Frac( const std::floating_point auto f );			// f - Floor( f )

	static int					Ftoi( float f );			// float to int conversion
	static int					Ftoi( double d );			// double to int conversion
	static char					Ftoi8( float f );			// float to char conversion
	static short				Ftoi16( float f );			// float to short conversion
	static int64				Ftoi64(const float f);		// float to int64 conversion
	static int64				Ftoi64(const double d);		// double to int64 conversion
	static unsigned short		Ftoui16( float f );			// float to unsigned short conversion
	static uint64				Ftoui64(float f);			// float to uint64 conversion
	static uint64				Ftoui64(double d);			// double to uint64 conversion
	static byte					Ftob( float f );			// float to byte conversion, the result is clamped to the range [0-255]

	static float				Dtof(double d) noexcept;    // double to float conversion, the result is clamped to the range [FLT_MIN-FLT_MAX]
	static void 				Dtofv(_In_ const double* __restrict dv_In, _Out_ float* __restrict fv_Out, _In_ const size_t count) noexcept;    // double to float conversion for bulk vectors, the results are clamped to the range [FLT_MIN-FLT_MAX]

	template <std::floating_point DestFloat, idMath_i2f::integral_like Src>
	static void Itofv(DestFloat* I2F_RESTRICT dst,
		const Src* I2F_RESTRICT src,
		size_t count) noexcept;

	template <std::floating_point DestFloat, idMath_i2f::integral_like Src>
	[[nodiscard]] static DestFloat Itof(Src v) noexcept;

	static signed char			ClampChar( int i );
	static signed short			ClampShort( int i );
	static int					ClampInt( int min, int max, int value );
	static int64				ClampInt64( int64 min, int64 max, int64 value );
	static uint64				ClampUInt64( uint64 min, uint64 max, uint64 value);
	static float				ClampFloat( float min, float max, float value );
	static double				ClampDouble( double min, double max, double value );

	static float				AngleNormalize360( float angle );
	static float				AngleNormalize180( float angle );
	static float				AngleDelta( float angle1, float angle2 );

	static int					FloatToBits( float f, int exponentBits, int mantissaBits );
	static float				BitsToFloat( int i, int exponentBits, int mantissaBits );

	static int					FloatHash( const float *array, const size_t numFloats );

	static float				LerpToWithScale( const float cur, const float dest, const float scale );

	static constexpr float			PI = 3.14159265358979323846f;							// pi
	static constexpr float			TWO_PI = 2.0f * PI;						// pi * 2
	static constexpr float			HALF_PI = 0.5f * PI;					// pi / 2
	static constexpr float			ONEFOURTH_PI = 0.25f * PI;				// pi / 4
	static constexpr float			ONEOVER_PI = 1.0f / PI;					// 1 / pi
	static constexpr float			ONEOVER_TWOPI = 1.0f / TWO_PI;				// 1 / pi * 2
	static constexpr float			E = 2.71828182845904523536f;							// e
	static constexpr float			SQRT_TWO = 1.41421356237309504880f;					// sqrt( 2 )
	static constexpr float			SQRT_THREE = 1.73205080756887729352f;					// sqrt( 3 )
	static constexpr float			SQRT_1OVER2 = 0.70710678118654752440f;				// sqrt( 1 / 2 )
	static constexpr float			SQRT_1OVER3 = 0.57735026918962576450f;				// sqrt( 1 / 3 )
	static constexpr float			M_DEG2RAD = PI / 180.0f;					// degrees to radians multiplier
	static constexpr float			M_RAD2DEG = 180.0f / PI;					// radians to degrees multiplier
	static constexpr float			M_SEC2MS = 1000.0f;					// seconds to milliseconds multiplier
	static constexpr float			M_MS2SEC = 0.001f;					// milliseconds to seconds multiplier
	static constexpr float			INFINITY = 1e30f;					// huge number which should be larger than any valid number used
	static constexpr float			FLT_EPSILON = 1.192092896e-07f;				// smallest positive number such that 1.0+FLT_EPSILON != 1.0
	static constexpr float			FLT_SMALLEST_NON_DENORMAL = idMathInternal::min_normal_ieee<float>();	// 1.1754944e-038f	// smallest non-denormal 32-bit floating point value
	static constexpr double			DBL_SMALLEST_NON_DENORMAL = idMathInternal::min_normal_ieee<double>();	// smallest non-denormal 64-bit floating point value
	static constexpr long double	DBLL_SMALLEST_NON_DENORMAL = idMathInternal::min_normal_ieee<long double>();	// smallest non-denormal 32-bit floating point value

#if defined( ID_WIN_X86_SSE_INTRIN )
	static const __m128				SIMD_SP_zero;
	static const __m128				SIMD_SP_255;
	static const __m128				SIMD_SP_min_char;
	static const __m128				SIMD_SP_max_char;
	static const __m128				SIMD_SP_min_short;
	static const __m128				SIMD_SP_max_short;
	static const __m128				SIMD_SP_smallestNonDenorm;
	static const __m128				SIMD_SP_tiny;
	static const __m128				SIMD_SP_rsqrt_c0;
	static const __m128				SIMD_SP_rsqrt_c1;
#endif

private:
	enum idMath_e : uint16 {
		LOOKUP_BITS				= 8,							
		EXP_POS					= 23,							
		EXP_BIAS				= 127,							
		LOOKUP_POS				= (EXP_POS-LOOKUP_BITS),
		SEED_POS				= (EXP_POS-8),
		SQRT_TABLE_SIZE			= (2<<LOOKUP_BITS),
		LOOKUP_MASK				= (SQRT_TABLE_SIZE-1)
	};

	union _flint {
		dword					i;
		float					f;
	};

	static dword				iSqrt[SQRT_TABLE_SIZE];
	static bool					initialized;
};

ID_INLINE byte CLAMP_BYTE(const int x )	{ 
	return ( (x) < 0 ? (0) : ( (x) > 255 ? 255 : static_cast<byte>(x) ) ); 
}

/*
========================
idMath::InvSqrt
========================
*/
ID_INLINE float idMath::InvSqrt(const float x ) {
#ifdef ID_WIN_X86_SSE_INTRIN

	return ( x > FLT_SMALLEST_NON_DENORMAL ) ? sqrtf( 1.0f / x ) : INFINITY;

#else

	return ( x > FLT_SMALLEST_NON_DENORMAL ) ? sqrtf( 1.0f / x ) : INFINITY;

#endif
}

ID_INLINE double idMath::InvSqrt(const double x) {
#ifdef ID_WIN_X86_SSE_INTRIN

	return (x > DBL_SMALLEST_NON_DENORMAL) ? sqrt(1.0f / x) : INFINITY;

#else

	return (x > DBL_SMALLEST_NON_DENORMAL) ? sqrt(1.0 / x) : INFINITY;

#endif
}

/*
========================
idMath::InvSqrt16
========================
*/
ID_INLINE float idMath::InvSqrt16(const float x ) {
#ifdef ID_WIN_X86_SSE_INTRIN

	return ( x > FLT_SMALLEST_NON_DENORMAL ) ? sqrtf( 1.0f / x ) : INFINITY;

#else

	return ( x > FLT_SMALLEST_NON_DENORMAL ) ? sqrtf( 1.0f / x ) : INFINITY;

#endif
}

/*
========================
idMath::Sqrt
========================
*/
ID_INLINE float idMath::Sqrt(const float x ) {
#ifdef ID_WIN_X86_SSE_INTRIN
	return ( x >= 0.0f ) ?  x * InvSqrt( x ) : 0.0f;
#else
	return ( x >= 0.0f ) ? sqrtf( x ) : 0.0f;
#endif
}

ID_INLINE double idMath::Sqrt(const double x) {
#ifdef ID_WIN_X86_SSE_INTRIN
	return (x >= 0.0f) ? x * InvSqrt(x) : 0.0f;
#else
	return (x >= 0.0f) ? sqrt(x) : 0.0f;
#endif
}

/*
========================
idMath::Sqrt16
========================
*/
ID_INLINE float idMath::Sqrt16(const float x ) {
#ifdef ID_WIN_X86_SSE_INTRIN
	return ( x >= 0.0f ) ?  x * InvSqrt16( x ) : 0.0f;
#else
	return ( x >= 0.0f ) ? sqrtf( x ) : 0.0f;
#endif
}

/*
========================
idMath::Frac
========================
*/
ID_INLINE auto idMath::Frac(const std::floating_point auto f ) {
	return f - ::Floor( f );
}

/*
========================
idMath::Sin
========================
*/
ID_INLINE float idMath::Sin(const float a ) {
	return sinf( a );
}

/*
========================
idMath::Sin16
========================
*/
ID_INLINE float idMath::Sin16( float a ) {
	float s;

	if ( ( a < 0.0f ) || ( a >= TWO_PI ) ) {
		a -= floorf( a * ONEOVER_TWOPI ) * TWO_PI;
	}
#if 1
	if ( a < PI ) {
		if ( a > HALF_PI ) {
			a = PI - a;
		}
	} else {
		if ( a > PI + HALF_PI ) {
			a = a - TWO_PI;
		} else {
			a = PI - a;
		}
	}
#else
	a = PI - a;
	if ( fabsf( a ) >= HALF_PI ) {
		a = ( ( a < 0.0f ) ? -PI : PI ) - a;
	}
#endif
	s = a * a;
	return a * ( ( ( ( ( -2.39e-08f * s + 2.7526e-06f ) * s - 1.98409e-04f ) * s + 8.3333315e-03f ) * s - 1.666666664e-01f ) * s + 1.0f );
}

/*
========================
idMath::Cos
========================
*/
ID_INLINE float idMath::Cos(const float a ) {
	return cosf( a );
}

/*
========================
idMath::Cos16
========================
*/
ID_INLINE float idMath::Cos16( float a ) {
	float s, d;

	if ( ( a < 0.0f ) || ( a >= TWO_PI ) ) {
		a -= floorf( a * ONEOVER_TWOPI ) * TWO_PI;
	}
#if 1
	if ( a < PI ) {
		if ( a > HALF_PI ) {
			a = PI - a;
			d = -1.0f;
		} else {
			d = 1.0f;
		}
	} else {
		if ( a > PI + HALF_PI ) {
			a = a - TWO_PI;
			d = 1.0f;
		} else {
			a = PI - a;
			d = -1.0f;
		}
	}
#else
	a = PI - a;
	if ( fabsf( a ) >= HALF_PI ) {
		a = ( ( a < 0.0f ) ? -PI : PI ) - a;
		d = 1.0f;
	} else {
		d = -1.0f;
	}
#endif
	s = a * a;
	return d * ( ( ( ( ( -2.605e-07f * s + 2.47609e-05f ) * s - 1.3888397e-03f ) * s + 4.16666418e-02f ) * s - 4.999999963e-01f ) * s + 1.0f );
}

/*
========================
idMath::SinCos
========================
*/
ID_INLINE void idMath::SinCos(const float a, float &s, float &c ) {
#if defined( ID_WIN_X86_ASM )
	_asm {
		fld		a
		fsincos
		mov		ecx, c
		mov		edx, s
		fstp	dword ptr [ecx]
		fstp	dword ptr [edx]
	}
#else
	s = sinf( a );
	c = cosf( a );
#endif
}

/*
========================
idMath::SinCos16
========================
*/
ID_INLINE void idMath::SinCos16( float a, float &s, float &c ) {
	float t, d;

	if ( ( a < 0.0f ) || ( a >= TWO_PI ) ) {
		a -= floorf( a * ONEOVER_TWOPI ) * TWO_PI;
	}
#if 1
	if ( a < PI ) {
		if ( a > HALF_PI ) {
			a = PI - a;
			d = -1.0f;
		} else {
			d = 1.0f;
		}
	} else {
		if ( a > PI + HALF_PI ) {
			a = a - TWO_PI;
			d = 1.0f;
		} else {
			a = PI - a;
			d = -1.0f;
		}
	}
#else
	a = PI - a;
	if ( fabsf( a ) >= HALF_PI ) {
		a = ( ( a < 0.0f ) ? -PI : PI ) - a;
		d = 1.0f;
	} else {
		d = -1.0f;
	}
#endif
	t = a * a;
	s = a * ( ( ( ( ( -2.39e-08f * t + 2.7526e-06f ) * t - 1.98409e-04f ) * t + 8.3333315e-03f ) * t - 1.666666664e-01f ) * t + 1.0f );
	c = d * ( ( ( ( ( -2.605e-07f * t + 2.47609e-05f ) * t - 1.3888397e-03f ) * t + 4.16666418e-02f ) * t - 4.999999963e-01f ) * t + 1.0f );
}

/*
========================
idMath::Tan
========================
*/
ID_INLINE float idMath::Tan(const float a ) {
	return tanf( a );
}

/*
========================
idMath::Tan16
========================
*/
ID_INLINE float idMath::Tan16( float a ) {
	float s;
	bool reciprocal;

	if ( ( a < 0.0f ) || ( a >= PI ) ) {
		a -= floorf( a * ONEOVER_PI ) * PI;
	}
#if 1
	if ( a < HALF_PI ) {
		if ( a > ONEFOURTH_PI ) {
			a = HALF_PI - a;
			reciprocal = true;
		} else {
			reciprocal = false;
		}
	} else {
		if ( a > HALF_PI + ONEFOURTH_PI ) {
			a = a - PI;
			reciprocal = false;
		} else {
			a = HALF_PI - a;
			reciprocal = true;
		}
	}
#else
	a = HALF_PI - a;
	if ( fabsf( a ) >= ONEFOURTH_PI ) {
		a = ( ( a < 0.0f ) ? -HALF_PI : HALF_PI ) - a;
		reciprocal = false;
	} else {
		reciprocal = true;
	}
#endif
	s = a * a;
	s = a * ( ( ( ( ( ( 9.5168091e-03f * s + 2.900525e-03f ) * s + 2.45650893e-02f ) * s + 5.33740603e-02f ) * s + 1.333923995e-01f ) * s + 3.333314036e-01f ) * s + 1.0f );
	if ( reciprocal ) {
		return 1.0f / s;
	} else {
		return s;
	}
}

/*
========================
idMath::ASin
========================
*/
ID_INLINE float idMath::ASin(const float a ) {
	if ( a <= -1.0f ) {
		return -HALF_PI;
	}
	if ( a >= 1.0f ) {
		return HALF_PI;
	}
	return asinf( a );
}

/*
========================
idMath::ASin16
========================
*/
ID_INLINE float idMath::ASin16( float a ) {
	if ( a < 0.0f ) {
		if ( a <= -1.0f ) {
			return -HALF_PI;
		}
		a = fabsf( a );
		return ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * idMath::Sqrt( 1.0f - a ) - HALF_PI;
	} else {
		if ( a >= 1.0f ) {
			return HALF_PI;
		}
		return HALF_PI - ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * idMath::Sqrt( 1.0f - a );
	}
}

/*
========================
idMath::ACos
========================
*/
ID_INLINE float idMath::ACos(const float a ) {
	if ( a <= -1.0f ) {
		return PI;
	}
	if ( a >= 1.0f ) {
		return 0.0f;
	}
	return acosf( a );
}

/*
========================
idMath::ACos16
========================
*/
ID_INLINE float idMath::ACos16( float a ) {
	if ( a < 0.0f ) {
		if ( a <= -1.0f ) {
			return PI;
		}
		a = fabsf( a );
		return PI - ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * idMath::Sqrt( 1.0f - a );
	} else {
		if ( a >= 1.0f ) {
			return 0.0f;
		}
		return ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * idMath::Sqrt( 1.0f - a );
	}
}

/*
========================
idMath::ATan
========================
*/
ID_INLINE float idMath::ATan(const float a ) {
	return atanf( a );
}

/*
========================
idMath::ATan16
========================
*/
ID_INLINE float idMath::ATan16( float a ) {
	float s;
	if ( fabsf( a ) > 1.0f ) {
		a = 1.0f / a;
		s = a * a;
		s = - ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
				* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
		if ( a < 0.0f ) {
			return s - HALF_PI;
		} else {
			return s + HALF_PI;
		}
	} else {
		s = a * a;
		return ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
			* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
	}
}

/*
========================
idMath::ATan
========================
*/
ID_INLINE float idMath::ATan(const float y, const float x ) {
	assert( fabs( y ) > idMath::FLT_SMALLEST_NON_DENORMAL || fabs( x ) > idMath::FLT_SMALLEST_NON_DENORMAL );
	return atan2f( y, x );
}

/*
========================
idMath::ATan16
========================
*/
ID_INLINE float idMath::ATan16(const float y, const float x ) {
	assert( fabs( y ) > idMath::FLT_SMALLEST_NON_DENORMAL || fabs( x ) > idMath::FLT_SMALLEST_NON_DENORMAL );

	float a, s;
	if ( fabsf( y ) > fabsf( x ) ) {
		a = x / y;
		s = a * a;
		s = - ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
				* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
		if ( a < 0.0f ) {
			return s - HALF_PI;
		} else {
			return s + HALF_PI;
		}
	} else {
		a = y / x;
		s = a * a;
		return ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
			* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
	}
}

/*
========================
idMath::Pow
========================
*/
ID_INLINE float idMath::Pow(const float x, const float y ) {
	return powf( x, y );
}

/*
========================
idMath::Pow16
========================
*/
ID_INLINE float idMath::Pow16(const float x, const float y ) {
	return Exp16( y * Log16( x ) );
}

/*
========================
idMath::Exp
========================
*/
ID_INLINE float idMath::Exp(const float f ) {
	return expf( f );
}

/*
========================
idMath::Exp16
========================
*/
ID_INLINE float idMath::Exp16(const float f ) {
	float x = f * 1.44269504088896340f;		// multiply with ( 1 / log( 2 ) )
#if 1
	int i = *reinterpret_cast<int *>(&x);
	const int s = ( i >> IEEE_FLT_SIGN_BIT );
	const int e = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	const int m = ( i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 ) ) | ( 1 << IEEE_FLT_MANTISSA_BITS );
	i = ( ( m >> ( IEEE_FLT_MANTISSA_BITS - e ) ) & ~( e >> INT32_SIGN_BIT ) ) ^ s;
#else
	int i = (int) x;
	if ( x < 0.0f ) {
		i--;
	}
#endif
	const int exponent = ( i + IEEE_FLT_EXPONENT_BIAS ) << IEEE_FLT_MANTISSA_BITS;
	float y = *reinterpret_cast<float *>(const_cast<int*>(&exponent));
	x -= static_cast<float>(i);
	if ( x >= 0.5f ) {
		x -= 0.5f;
		y *= 1.4142135623730950488f;	// multiply with sqrt( 2 )
	}
	const float x2 = x * x;
	const float p = x * ( 7.2152891511493f + x2 * 0.0576900723731f );
	const float q = 20.8189237930062f + x2;
	x = y * ( q + p ) / ( q - p );
	return x;
}

/*
========================
idMath::Log
========================
*/
ID_INLINE float idMath::Log(const float f ) {
	return logf( f );
}

/*
========================
idMath::Log16
========================
*/
ID_INLINE float idMath::Log16( float f ) {
	int i = *reinterpret_cast<int *>(&f);
	const int exponent = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	i -= ( exponent + 1 ) << IEEE_FLT_MANTISSA_BITS;	// get value in the range [.5, 1>
	float y = *reinterpret_cast<float *>(&i);
	y *= 1.4142135623730950488f;						// multiply with sqrt( 2 )
	y = ( y - 1.0f ) / ( y + 1.0f );
	const float y2 = y * y;
	y = y * ( 2.000000000046727f + y2 * ( 0.666666635059382f + y2 * ( 0.4000059794795f + y2 * ( 0.28525381498f + y2 * 0.2376245609f ) ) ) );
	y += 0.693147180559945f * ( static_cast<float>(exponent) + 0.5f );
	return y;
}

/*
========================
idMath::IPow
========================
*/
ID_INLINE auto idMath::IPow(const std::integral auto x, const std::integral auto power ) {
	using CommonType = std::common_type_t<decltype(x), decltype(power)>;

	if (power < 0)
	{
		return CommonType{ 0 }; // No fractional results for integral base/exponent
	}

	CommonType base = numeric_cast<CommonType>(x);
	CommonType exp = numeric_cast<CommonType>(power);
	CommonType result = numeric_cast<CommonType>(1);

	while (exp > 0) {
		if (exp & 1)
		{
			result *= base;
		}
		base *= base;
		exp >>= 1;
	}

	return result;
}

/*
========================
idMath::ILog2
========================
*/
ID_INLINE int idMath::ILog2( float f ) {
	return ( ( (*reinterpret_cast<int *>(&f)) >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
}

/*
========================
idMath::ILog2
========================
*/
ID_INLINE int idMath::ILog2(const int i ) {
	return ILog2( static_cast<float>(i) );
}

/*
========================
idMath::BitsForFloat
========================
*/

ID_INLINE size_t idMath::BitsForFloat( const std::floating_point auto f ) {
	//return ILog2( f ) + 1;

	// Minimal number of bits of a *signed* two's-complement integer type
	// required to represent `f` after rounding it away from zero.
	// Returns 1 for 0, 0 for NaN/inf (you can choose a different policy).

	if (!std::isfinite(f))
	{
		return 0;
	}

	const auto absVal  = std::fabs(f);

	if (absVal == 0)
	{
		return 1;
	}

	// Round magnitude away from zero
	const auto rounded = std::ceil(absVal);

	long double log2v = 0.0;

	// compute base-2 logarithm and derive bit width directly in log domain
	if (f < 0) {
		// negative values: need ceil(log2(|x|)) + 1 bits (sign included)
		log2v = std::log2(rounded);
	}
	else {
		// positive values: need ceil(log2(|x| + 1)) + 1 bits
		// (+1 ensures we can represent +x even at exact powers of two)
		log2v = std::log2(rounded + 1.0L);
	}

	return static_cast<size_t>(std::ceil(log2v)) + 1u;
}

/*
========================
idMath::BitsForInteger
========================
*/
ID_INLINE size_t idMath::BitsForInteger( const Ordinal auto i ) {
	//return ILog2( Itof<float>(i) ) + 1;

	using T = decltype(i);
	using U = std::make_unsigned_t<T>;

	if constexpr (std::is_signed_v<T>) {
		if (i < 0) {
			return std::bit_width(static_cast<U>(~i)) + 1u; // two's complement
		}
		else {
			return i == 0 ? 1u : std::bit_width(static_cast<U>(i));
		}
	}
	else {
		return i == 0 ? 1u : std::bit_width(static_cast<U>(i));
	}
}

/*
========================
idMath::MaskForFloatSign
========================
*/
ID_INLINE int idMath::MaskForFloatSign( float f ) {
	return ( (*reinterpret_cast<int *>(&f)) >> IEEE_FLT_SIGN_BIT );
}

/*
========================
idMath::MaskForIntegerSign
========================
*/
ID_INLINE int idMath::MaskForIntegerSign(const int i ) {
	return ( i >> INT32_SIGN_BIT );
}

/*
========================
idMath::FloorPowerOfTwo
========================
*/
ID_INLINE auto idMath::FloorPowerOfTwo(const Numeric auto v) noexcept {
	/*
	 	x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x++;
		return x >> 1;
	*/

	// Returns largest power of two <= v.
	// Integral inputs -> unsigned counterpart type; floating -> int64.
	// On overflow/invalid -> 0.

	using D = std::remove_cv_t<decltype(v)>;
	constexpr bool is_int = std::is_integral_v<D>;
	constexpr bool is_fp = std::is_floating_point_v<D>;

	// -------- Integral path --------
	if constexpr (is_int) {
		using U = std::conditional_t<std::is_signed_v<D>, std::make_unsigned_t<D>, D>;
		if constexpr (std::is_same_v<D, bool>) {
			return static_cast<U>(v ? 1 : 0);
		}
		else {
			if (v <= 0)
			{
				return static_cast<U>(0);
			}
			const U u = static_cast<U>(v);
			// bit_width(u) == floor(log2(u)) + 1 for u>0
			const int bw = std::bit_width(u);
			return U(1) << (bw - 1);
		}
	}

	// -------- Floating path --------
	if constexpr (is_fp) {
		// Reject non-positive or non-finite
		if (!(v > D(0)) || !std::isfinite(static_cast<long double>(v)))
		{
			return uint64{ 0 };
		}

		// Quick small/large cutoffs
		if (v < D(1))
		{
			return uint64{ 0 }; // floor power-of-two < 1 is 0 for integer return
		}

		// Use frexp: v = m * 2^e with 0.5 <= m < 1
		int e = 0;
		const auto m = std::frexp(v, &e);

		// For v >= 1, floor power is 2^(e-1) (and equals v if v is exact power-of-two)
		const int shift = e - 1;

		if (shift < 0 || shift >= 64)
		{
			return uint64{ 0 }; // overflow or underflow
		}

		return uint64{ 1 } << shift;
	}

	// Not reachable, but keeps compilers happy on exotic types
	return 0u;
}

/*
========================
idMath::CeilPowerOfTwo
========================
*/
ID_INLINE auto idMath::CeilPowerOfTwo( const Numeric auto v ) noexcept {
/*	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	return x;*/

	// Returns smallest power of two >= v.
	// Integral inputs -> unsigned counterpart type; floating -> uint64.
	// On overflow/invalid -> 0.

	using D = std::remove_cv_t<decltype(v)>;
	constexpr bool is_int = std::is_integral_v<D>;
	constexpr bool is_fp = std::is_floating_point_v<D>;

	// -------- Integral path --------
	if constexpr (is_int) {
		using U = std::conditional_t<std::is_signed_v<D>, std::make_unsigned_t<D>, D>;
		if constexpr (std::is_same_v<D, bool>) {
			// ceil_pow2(false)=0, ceil_pow2(true)=1
			return static_cast<U>(v ? 1 : 0);
		}
		else {
			if (v <= 0)
			{
				return static_cast<U>(0);
			}
			const U u = static_cast<U>(v);
			if (u <= U(1))
			{
				return u;
			}

			// Next power of two: 1 << bit_width(u-1)
			const int bw = std::bit_width(static_cast<U>(u - U{ 1 }));

			// Overflow if target shift >= number of value bits
			if (bw >= std::numeric_limits<U>::digits)
			{
				return static_cast<U>(0);
			}

			return U(1) << bw;
		}
	}

	// -------- Floating path --------
	if constexpr (is_fp) {
		if (!(v > D(0)) || !std::isfinite(static_cast<long double>(v)))
		{
			return uint64{ 0 };
		}
		if (v <= D(1))
		{
			return uint64{ 1 };
		}

		int e = 0;
		const auto m = std::frexp(v, &e);  // v = m * 2^e, 0.5 <= m < 1

		// If v is exactly a power of two, frexp gives m==0.5 and e==k+1.
		const bool is_exact_pow2 = std::equal_to<>{}(m, decltype(m){0.5});
		const int target_shift = is_exact_pow2 ? (e - 1) : e;

		if (target_shift < 0 || target_shift >= 64)
		{
			return uint64_t{ 0 }; // overflow/underflow
		}

		return uint64_t{ 1 } << target_shift;
	}

	return 0u;
}

/*
========================
idMath::IsPowerOfTwo
========================
*/
// Monolithic power-of-two check (integral + floating)
ID_INLINE constexpr bool idMath::IsPowerOfTwo( const Numeric auto v ) noexcept {
	using D = std::remove_cv_t<decltype(v)>;

	// Be robust to quirky trait behavior; explicitly include these common types
	constexpr bool is_integral_like =
		std::is_integral_v<D> ||
		std::is_same_v<D, std::int64_t> ||
		std::is_same_v<D, std::uint64_t> ||
		std::is_same_v<D, std::size_t>;

	constexpr bool is_float_like =
		std::is_floating_point_v<D> ||
		std::is_same_v<D, long double>;

	static_assert(is_integral_like || is_float_like,
		"IsPowerOfTwo: Numeric must be an integer or floating type");

	// ========================= INTEGRAL PATH =========================
	if constexpr (is_integral_like && !is_float_like) {
		if constexpr (std::is_same_v<D, bool>) {
			return v; // only true (1) is a power of two
		}
		else if constexpr (std::is_signed_v<D>) {
			if (v <= 0)
			{
				return false;
			}
			using U = std::make_unsigned_t<D>;
			const U u = static_cast<U>(v);
			return (u & (u - U{ 1 })) == U{ 0 };
		}
		else {
			if (v == 0)
			{
				return false;
			}
			return (v & (v - D{ 1 })) == D{ 0 };
		}
	}

	// ======================= FLOATING-POINT PATH =====================
	// Fast rejects
	if (!(v > D(0)) || !std::isfinite(static_cast<long double>(v)))
	{
		return false;
	}

	if constexpr (std::is_same_v<D, float>) {
		const std::uint32_t u = std::bit_cast<std::uint32_t>(v);
		constexpr std::uint32_t EXP = 0x7F80'0000u;
		constexpr std::uint32_t MAN = 0x007F'FFFFu;
		const std::uint32_t exp = u & EXP;
		const std::uint32_t man = u & MAN;
		// normalized: mantissa==0; subnormal: mantissa has exactly one bit
		return exp ? (man == 0u) : (man != 0u && (man & (man - 1u)) == 0u);
	}
	else if constexpr (std::is_same_v<D, double>) {
		const std::uint64_t u = std::bit_cast<std::uint64_t>(v);
		constexpr std::uint64_t EXP = 0x7FF0'0000'0000'0000ull;
		constexpr std::uint64_t MAN = 0x000F'FFFF'FFFF'FFFFull;
		const std::uint64_t exp = u & EXP;
		const std::uint64_t man = u & MAN;
		return exp ? (man == 0ull) : (man != 0ull && (man & (man - 1ull)) == 0ull);
	}
	else {
		// Portable fallback: power of two iff frexp mantissa is exactly 0.5
		int e = 0;
		const auto m = std::frexp(v, &e);                    // v == m * 2^e, 0.5 <= m < 1
		return std::equal_to<>{}(m, decltype(m){0.5});       // avoids float-== warning
	}
}

/*
========================
idMath::BitCount
========================
*/
ID_INLINE int idMath::BitCount( int x ) {
	x -= ( ( x >> 1 ) & 0x55555555 );
	x = ( ( ( x >> 2 ) & 0x33333333 ) + ( x & 0x33333333 ) );
	x = ( ( ( x >> 4 ) + x ) & 0x0f0f0f0f );
	x += ( x >> 8 );
	return ( ( x + ( x >> 16 ) ) & 0x0000003f );
}

/*
========================
idMath::BitReverse
========================
*/
ID_INLINE int idMath::BitReverse( int x ) {
	x = ( ( ( x >> 1 ) & 0x55555555 ) | ( ( x & 0x55555555 ) << 1 ) );
	x = ( ( ( x >> 2 ) & 0x33333333 ) | ( ( x & 0x33333333 ) << 2 ) );
	x = ( ( ( x >> 4 ) & 0x0f0f0f0f ) | ( ( x & 0x0f0f0f0f ) << 4 ) );
	x = ( ( ( x >> 8 ) & 0x00ff00ff ) | ( ( x & 0x00ff00ff ) << 8 ) );
	return ( ( x >> 16 ) | ( x << 16 ) );
}

/*
========================
idMath::Abs
========================
*/
ID_INLINE int idMath::Abs(const int x ) {
#if 1
	return abs( x );
#else
   int y = x >> INT32_SIGN_BIT;
   return ( ( x ^ y ) - y );
#endif
}

/*
========================
idMath::Fabs
========================
*/
ID_INLINE float idMath::Fabs( float f ) {
#if 1
	return fabsf( f );
#else
	int tmp = *reinterpret_cast<int *>( &f );
	tmp &= 0x7FFFFFFF;
	return *reinterpret_cast<float *>( &tmp );
#endif
}

ID_INLINE double idMath::Fabs(double f) {
#if 1
	return fabs(f);
#else
	int tmp = *reinterpret_cast<int*>(&f);
	tmp &= 0x7FFFFFFF;
	return *reinterpret_cast<float*>(&tmp);
#endif
}

/*
========================
idMath::Floor
========================
*/
ID_INLINE auto idMath::Floor(const std::floating_point auto f) {
	return ::Floor( f );
}

/*
========================
idMath::Ceil
========================
*/
ID_INLINE auto idMath::Ceil(const std::floating_point auto f) {
	return ::Ceil( f );
}

/*
========================
idMath::Rint
========================
*/
ID_INLINE float idMath::Rint(const float f ) {
	return floorf( f + 0.5f );
}


/*
========================
idMath::Ftoi8
========================
*/
ID_INLINE char idMath::Ftoi8(const float f ) {
#ifdef ID_WIN_X86_SSE_INTRIN
	__m128 x = _mm_load_ss( &f );
	x = _mm_max_ss( x, SIMD_SP_min_char );
	x = _mm_min_ss( x, SIMD_SP_max_char );
	return static_cast<char>( _mm_cvttss_si32( x ) );
#else
	// The converted result is clamped to the range [-128,127].
	//const int i = C_FLOAT_TO_INT( f );
	const int i = _cvt_ftoi_fast(f);
	if ( i < -128 ) {
		return -128;
	} else if ( i > 127 ) {
		return 127;
	}
	return static_cast<char>( i );
#endif
}

/*
========================
idMath::Ftoi16
========================
*/
ID_INLINE short idMath::Ftoi16(const float f ) {
#ifdef ID_WIN_X86_SSE_INTRIN
	__m128 x = _mm_load_ss( &f );
	x = _mm_max_ss( x, SIMD_SP_min_short );
	x = _mm_min_ss( x, SIMD_SP_max_short );
	return static_cast<short>( _mm_cvttss_si32( x ) );
#else
	// The converted result is clamped to the range [-32768,32767].
	//const int i = C_FLOAT_TO_INT( f );
	const int i = _cvt_ftoi_fast(f);
	if ( i < -32768 ) {
		return -32768;
	} else if ( i > 32767 ) {
		return 32767;
	}
	return static_cast<short>( i );
#endif
}

/*
========================
idMath::Ftoi64
========================
*/
ID_INLINE int64 idMath::Ftoi64(const float f) {
	return _cvt_ftoll_fast(f);
}

ID_INLINE int64 idMath::Ftoi64(const double d) {
	return _cvt_dtoll_fast(d);
}

/*
========================
idMath::Ftoui16
========================
*/
ID_INLINE unsigned short idMath::Ftoui16(const float f ) {
	// TO DO - SSE ??

	// The converted result is clamped to the range [-32768,32767].
	//const int i = C_FLOAT_TO_INT( f );
	const int i = _cvt_ftoi_fast(f);
	if ( i < 0 ) {
		return 0;
	} else if ( i > 65535 ) {
		return 65535;
	}
	return static_cast<unsigned short>( i );
}

/*
========================
idMath::Ftoui64
========================
*/
ID_INLINE uint64 idMath::Ftoui64(const float f) {
	return _cvt_ftoull_fast(f);
}

ID_INLINE uint64 idMath::Ftoui64(const double d) {
	return _cvt_dtoull_fast(d);
}

/*
========================
idMath::Ftob
========================
*/
ID_INLINE byte idMath::Ftob(const float f ) {
#ifdef ID_WIN_X86_SSE_INTRIN
	// If a converted result is negative the value (0) is returned and if the
	// converted result is larger than the maximum byte the value (255) is returned.
	__m128 x = _mm_load_ss( &f );
	x = _mm_max_ss( x, SIMD_SP_zero );
	x = _mm_min_ss( x, SIMD_SP_255 );
	return static_cast<byte>( _mm_cvttss_si32( x ) );
#else
	// The converted result is clamped to the range [0,255].
	//const int i = C_FLOAT_TO_INT( f );
	const int i = _cvt_ftoi_fast(f);
	if ( i < 0 ) {
		return 0;
	} else if ( i > 255 ) {
		return 255;
	}
	return static_cast<byte>( i );
#endif
}

/*
========================
idMath::Dtofv
========================
*/
ID_INLINE void idMath::Dtofv(
	_In_ const double* __restrict dv_In,
	_Out_ float* __restrict fv_Out,
	_In_ const size_t count) noexcept
{
	using namespace idMath_d2f;

	for (size_t i = 0; i < count; i++) {
		const double  x = dv_In[i];
		const auto    bits = std::bit_cast<uint64>(x);
		const auto    exp = static_cast<int>((bits & EXP_MASK) >> 52);
		const auto    frac = (bits & FRAC_MASK);
		const bool    neg = (bits & SIGN_MASK) != 0;

		float out = 0.0f;

		if (exp == EXP_INF) {
			// Inf or NaN
			if (frac == 0) {
				// Preserve infinities exactly
				out = neg ? NINF : PINF;
			}
			else {
#if SAFE_D2F_ASSERT_ON_NAN
				assert(false && "safe_double_to_float: NaN encountered");
#endif
				out = QNAN; // pass through a quiet NaN
			}
		}
		else if (exp != 0) {
			// Normal (non-subnormal) double
			const int e_unbiased = exp - EXP_BIAS;

			// If certainly too large for float, handle according to policy
			if (e_unbiased > F32_MAX_EU) {
#if SAFE_D2F_SATURATE_OVERFLOW
				out = neg ? -F32_MAX : F32_MAX;
#else
				out = neg ? NINF : PINF;
#endif
			}
			else {
				// Safe/near-safe range: let hardware do the rounding
				out = static_cast<float>(x);
			}
		}
		else {
			// Zero or subnormal double: cast is fine/fast
			out = static_cast<float>(x);
		}

		fv_Out[i] = out;
	}
}

/*
========================
idMath::Dtof
========================
*/
ID_INLINE float idMath::Dtof(const double d) noexcept {
	float out = 0.0f;
	Dtofv(&d, &out, 1);
	return out;
}

/*
========================
idMath::Itofv
========================
*/
// ---------- Vector version: integral[] -> float[] with debug asserts ----------
template <std::floating_point DestFloat, idMath_i2f::integral_like Src>
I2F_FORCEINLINE void idMath::Itofv(DestFloat* I2F_RESTRICT dst, const Src* I2F_RESTRICT src, const size_t count) noexcept
{
	using namespace idMath_i2f;
	constexpr std::uintmax_t LIMIT = kExactLimit<DestFloat>;

	for (size_t i = 0; i < count; ++i) {
#if _DEBUG
		const std::uintmax_t mag = umagnitude(src[i]);
		// If DestFloat has >= uintmax_t precision bits, LIMIT==max and this trivially holds.
		assert(mag <= LIMIT && "Itof: integer exceeds exact precision of destination floating type");
#endif
		dst[i] = numeric_cast<DestFloat>(src[i]); // exact when the assert holds
	}
}

/*
========================
idMath::Itof
========================
*/
// ---------- Scalar wrapper: calls vector path with count=1 ----------
template <std::floating_point DestFloat, idMath_i2f::integral_like Src>
[[nodiscard]] I2F_FORCEINLINE DestFloat idMath::Itof(Src v) noexcept {
	DestFloat out{};
	Itofv<DestFloat, Src>(&out, &v, 1);
	return out;
}

/*
========================
idMath::ClampChar
========================
*/
ID_INLINE signed char idMath::ClampChar(const int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return static_cast<signed char>( i );
}

/*
========================
idMath::ClampShort
========================
*/
ID_INLINE signed short idMath::ClampShort(const int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 32767 ) {
		return 32767;
	}
	return static_cast<signed short>( i );
}

/*
========================
idMath::ClampInt
========================
*/
ID_INLINE int idMath::ClampInt(const int min, const int max, const int value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

/*
========================
idMath::ClampInt64
========================
*/
ID_INLINE int64 idMath::ClampInt64(const int64 min, const int64 max, const int64 value) {
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

/*
========================
idMath::ClampUInt64
========================
*/
ID_INLINE uint64 idMath::ClampUInt64(const uint64 min, const uint64 max, const uint64 value) {
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

/*
========================
idMath::ClampFloat
========================
*/
ID_INLINE float idMath::ClampFloat(const float min, const float max, const float value ) {
	return Max( min, Min( max, value ) );
}

/*
========================
idMath::ClampDouble
========================
*/
ID_INLINE double idMath::ClampDouble(const double min, const double max, const double value) {
	return Max(min, Min(max, value));
}

/*
========================
idMath::AngleNormalize360
========================
*/
ID_INLINE float idMath::AngleNormalize360( float angle ) {
	if ( ( angle >= 360.0f ) || ( angle < 0.0f ) ) {
		angle -= floorf( angle * ( 1.0f / 360.0f ) ) * 360.0f;
	}
	return angle;
}

/*
========================
idMath::AngleNormalize180
========================
*/
ID_INLINE float idMath::AngleNormalize180( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0f ) {
		angle -= 360.0f;
	}
	return angle;
}

/*
========================
idMath::AngleDelta
========================
*/
ID_INLINE float idMath::AngleDelta(const float angle1, const float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}

/*
========================
idMath::FloatHash
========================
*/
ID_INLINE int idMath::FloatHash( const float *array, const size_t numFloats ) {
	int hash = 0;

	const int* ptr = reinterpret_cast<const int*>(array);
	for ( size_t i = 0; i < numFloats; i++ ) {
		hash ^= ptr[i];
	}
	return hash;
}

template< Numeric T, std::floating_point F >
[[nodiscard]] ID_INLINE_EXTERN auto Lerp( const T from, const T to, const F f ) noexcept
-> std::common_type_t<T, F> {
	using CommonType = std::common_type_t<T, F>;

	const CommonType a = numeric_cast<CommonType>(from);
	const CommonType b = numeric_cast<CommonType>(to);
	const CommonType c = numeric_cast<CommonType>(f);
	return numeric_cast<T>(a + ((b - a) * c));
}

template< typename T, std::floating_point F >
[[nodiscard]] ID_INLINE_EXTERN T Lerp(const T from, const T to, const F f) noexcept {

	return (from + ((to - from) * f));
}

template< typename T >
[[nodiscard]] ID_INLINE_EXTERN void LerpArray(const T* from, const T* to, T* out, size_t arraySize, double f) {
	for (size_t i = 0; i < arraySize; ++i)
	{
		out[i] = Lerp(from[i], to[i], f);
	}
}

template<Numeric T, std::floating_point F, size_t N>
[[nodiscard]] ID_INLINE_EXTERN constexpr void LerpArray(const T(&from)[N], const T(&to)[N], T(&out)[N], const F f) noexcept
{
	static_assert(N > 0, "Array size must be greater than zero");
	using CommonType = std::common_type_t<T, F>;

	for (size_t i = 0; i < N; ++i)
	{
		const CommonType a = numeric_cast<CommonType>(from[i]);
		const CommonType b = numeric_cast<CommonType>(to[i]);
		out[i] = numeric_cast<T>(Lerp(a, b, f));
	}
}


/*
========================
LerpToWithScale

Lerps from "cur" to "dest", scaling the delta to change by "scale"
If the delta between "cur" and "dest" is very small, dest is returned to prevent denormals.
========================
*/
inline float idMath::LerpToWithScale( const float cur, const float dest, const float scale ) {
	const float delta = dest - cur;
	if ( delta > -1.0e-6f && delta < 1.0e-6f ) {
		return dest;
	}
	return cur + ( dest - cur ) * scale;
}


#endif /* !__MATH_MATH_H__ */
