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

#define SEC2MS(t)				( idMath::Ftoi( (t) * idMath::M_SEC2MS ) )
#define MS2SEC(t)				( (t) * idMath::M_MS2SEC )

#define	ANGLE2SHORT(x)			( idMath::Ftoi( (x) * 65536.0f / 360.0f ) & 65535 )
#define	SHORT2ANGLE(x)			( (x) * ( 360.0f / 65536.0f ) )

#define	ANGLE2BYTE(x)			( idMath::Ftoi( (x) * 256.0f / 360.0f ) & 255 )
#define	BYTE2ANGLE(x)			( (x) * ( 360.0f / 256.0f ) )

#define C_FLOAT_TO_INT( x )		(int)(x)

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

	enum class Eu64 : std::size_t { A = 0, B = 1 };
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
compile_time_assert( sizeof( unsigned int ) == 4 );

#define OLD_INT32_SIGNBITSET(i)		(static_cast<const unsigned int>(i) >> INT32_SIGN_BIT)
#define OLD_INT32_SIGNBITNOTSET(i)	((~static_cast<const unsigned int>(i)) >> INT32_SIGN_BIT)

// Unfortunately, /analyze can't figure out that these always return
// either 0 or 1, so this extra wrapper is needed to avoid the static
// analysis warning.

ID_INLINE_EXTERN int INT32_SIGNBITSET(const int i ) {
	const int	r = OLD_INT32_SIGNBITSET( i );
	assert( r == 0 || r == 1 );
	return r;
}

ID_INLINE_EXTERN int INT32_SIGNBITNOTSET(const int i ) {
	const int	r = OLD_INT32_SIGNBITNOTSET( i );
	assert( r == 0 || r == 1 );
	return r;
}

ID_INLINE_EXTERN int64 INT64_SIGNBITSET(const int64 i) {
	const int64	r = static_cast<const uint64>(i) >> INT64_SIGN_BIT;
	assert(r == 0 || r == 1);
	return r;
}

ID_INLINE_EXTERN int64 INT64_SIGNBITNOTSET(const int64 i) {
	const int64	r = (~static_cast<const uint64>(i)) >> INT64_SIGN_BIT;
	assert(r == 0 || r == 1);
	return r;
}

namespace idMath_integral_signs {

	// Remove cv/ref once.
	template <class T>
	using decay_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

	// MSVC-friendly integral detection (treat common aliases as integral)
	template <class T>
	struct is_integral_compat {
		using U = decay_cvref_t<T>;
		static constexpr bool value =
			std::is_integral_v<U>
#if defined(_MSC_VER)
			|| std::is_same_v<U, __int64>
			|| std::is_same_v<U, unsigned __int64>
			|| std::is_same_v<U, long long>
			|| std::is_same_v<U, unsigned long long>
			|| std::is_same_v<U, std::size_t>
			|| std::is_same_v<U, std::ptrdiff_t>
#endif
			;
	};
	template <class T>
	inline constexpr bool is_integral_compat_v = is_integral_compat<T>::value;

	// Map any integral T to a fixed-width type of the same size & signedness.
	template <class T, bool Signed, size_t Bytes>
	struct fixed_width_of_size;

	template <class T> struct fixed_width_of_size<T, true, 8> { using type = std::int64_t; };
	template <class T> struct fixed_width_of_size<T, true, 4> { using type = std::int32_t; };
	template <class T> struct fixed_width_of_size<T, true, 2> { using type = std::int16_t; };
	template <class T> struct fixed_width_of_size<T, true, 1> { using type = std::int8_t; };
	template <class T> struct fixed_width_of_size<T, false, 8> { using type = std::uint64_t; };
	template <class T> struct fixed_width_of_size<T, false, 4> { using type = std::uint32_t; };
	template <class T> struct fixed_width_of_size<T, false, 2> { using type = std::uint16_t; };
	template <class T> struct fixed_width_of_size<T, false, 1> { using type = std::uint8_t; };

	template <class T>
	using SafeIntCompat_t =
		typename fixed_width_of_size<
		T,
		std::is_signed_v<decay_cvref_t<T>>,
		sizeof(decay_cvref_t<T>)
		>::type;

	// Normalize an integral or enum source to a SafeInt-compatible fixed-width type.
	template <class S, bool IsEnum = std::is_enum_v<decay_cvref_t<S>>>
	struct normalize_src;

	// Enum source → use underlying_type_t first, then normalize
	template <class S>
	struct normalize_src<S, /*IsEnum=*/true> {
		using raw = decay_cvref_t<S>;
		using base = std::underlying_type_t<raw>;
		using type = SafeIntCompat_t<base>;
	};

	// Non-enum source → normalize directly
	template <class S>
	struct normalize_src<S, /*IsEnum=*/false> {
		using raw = decay_cvref_t<S>;
		using type = SafeIntCompat_t<raw>;
	};

	// Range-check floating point to integral Dest (using long double for headroom)
	template <class DestFixed, class Float>
	inline void ensure_fp_in_range(Float x) {
		using Lim = std::numeric_limits<DestFixed>;
		// reject NaN / Inf
		if (!std::isfinite(static_cast<long double>(x))) {
			throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
		}
		const long double lo = static_cast<long double>((Lim::min)());
		const long double hi = static_cast<long double>((Lim::max)());
		const long double xv = static_cast<long double>(x);
		if (xv < lo || xv > hi) {
			throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
		}
	}
} // namespace idMath_integral_signs

#ifdef _MSC_VER
static_assert(idMath_integral_signs::is_integral_compat_v<__int64>);
static_assert(idMath_integral_signs::is_integral_compat_v<unsigned __int64>);
static_assert(idMath_integral_signs::is_integral_compat_v<size_t>);
static_assert(idMath_integral_signs::is_integral_compat_v<int64>);
static_assert(idMath_integral_signs::is_integral_compat_v<uint64>);
#endif

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
ID_INLINE_EXTERN bool IEEE_FLT_IS_NAN(const float x ) {
	return x != x;
}

/*
========================
IEEE_FLT_IS_INF
========================
*/
ID_INLINE_EXTERN bool IEEE_FLT_IS_INF(const float x ) {
	return x == x && x * 0 != x * 0;
}

/*
========================
IEEE_FLT_IS_INF_NAN
========================
*/
ID_INLINE_EXTERN bool IEEE_FLT_IS_INF_NAN(const float x ) {
	return x * 0 != x * 0;
}

/*
========================
IEEE_FLT_IS_IND
========================
*/
ID_INLINE_EXTERN bool IEEE_FLT_IS_IND(const float x ) {
	return	(reinterpret_cast<const unsigned int &>(x) == 0xffc00000); 
}

/*
========================
IEEE_FLT_IS_DENORMAL
========================
*/
ID_INLINE_EXTERN bool IEEE_FLT_IS_DENORMAL(const float x ) {
	return ((reinterpret_cast<const unsigned int &>(x) & 0x7f800000) == 0x00000000 &&
			(reinterpret_cast<const unsigned int &>(x) & 0x007fffff) != 0x00000000 ); 
}


/*
========================
IsNAN
========================
*/template<class type>
ID_INLINE_EXTERN bool IsNAN( const type &v ) {
	for ( int i = 0; i < v.GetDimension(); i++ ) {
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
	for ( int i = 0; i < v.GetDimension(); i++ ) {
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
ID_INLINE_EXTERN bool IsValid( const float & f ) {	// these parameter must be a reference for the function to be considered a specialization
	return !( IEEE_FLT_IS_NAN( f ) || IEEE_FLT_IS_INF( f ) || IEEE_FLT_IS_IND( f ) || IEEE_FLT_IS_DENORMAL( f ) );
}

/*
========================
IsNAN
========================
*/
//template<>
ID_INLINE_EXTERN bool IsNAN( const float & f ) {	// these parameter must be a reference for the function to be considered a specialization
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
	for ( int i = 0; i < v.GetDimension(); i++ ) {
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

	static int					IPow( int x, int y );		// integral x raised to the power y
	static int					ILog2( float f );			// integral base-2 logarithm of the floating point value
	static int					ILog2( int i );				// integral base-2 logarithm of the integer value

	static int					BitsForFloat( float f );	// minimum number of bits required to represent ceil( f )
	static int					BitsForInteger( int i );	// minimum number of bits required to represent i
	static int					MaskForFloatSign( float f );// returns 0x00000000 if x >= 0.0f and returns 0xFFFFFFFF if x <= -0.0f
	static int					MaskForIntegerSign( int i );// returns 0x00000000 if x >= 0 and returns 0xFFFFFFFF if x < 0
	static int					FloorPowerOfTwo( int x );	// round x down to the nearest power of 2
	static int					CeilPowerOfTwo( int x );	// round x up to the nearest power of 2
	template <class T>
	static constexpr bool       IsPowerOfTwo(T v) noexcept; // returns true if x is a power of 2
	static int					BitCount( int x );			// returns the number of 1 bits in x
	static int					BitReverse( int x );		// returns the bit reverse of x

	static int					Abs( int x );				// returns the absolute value of the integer value (for reference only)
	static float				Fabs( float f );			// returns the absolute value of the floating point value
	static double				Fabs( double f );			// returns the absolute value of the floating point value
	static float				Floor( float f );			// returns the largest integer that is less than or equal to the given value
	static float				Ceil( float f );			// returns the smallest integer that is greater than or equal to the given value
	static float				Rint( float f );			// returns the nearest integer

	static float				Frac( float f );			// f - Floor( f )

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

	template <typename Dest, typename Src>
	[[nodiscard]] static Dest integer_cast(Src v)
		requires (
	// Dest must be integral (including bool)
	idMath_integral_signs::is_integral_compat_v<Dest> &&
		// Src can be arithmetic or enum (including bool, floats, etc.)
		(std::is_arithmetic_v<idMath_integral_signs::decay_cvref_t<Src>> ||
			std::is_enum_v<idMath_integral_signs::decay_cvref_t<Src>>)
		);

	static signed char			ClampChar( int i );
	static signed short			ClampShort( int i );
	static int					ClampInt( int min, int max, int value );
	static float				ClampFloat( float min, float max, float value );

	static float				AngleNormalize360( float angle );
	static float				AngleNormalize180( float angle );
	static float				AngleDelta( float angle1, float angle2 );

	static int					FloatToBits( float f, int exponentBits, int mantissaBits );
	static float				BitsToFloat( int i, int exponentBits, int mantissaBits );

	static int					FloatHash( const float *array, const int numFloats );

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
	enum {
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
ID_INLINE float idMath::Frac(const float f ) {
	return f - floorf( f );
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
	int exponent = ( i + IEEE_FLT_EXPONENT_BIAS ) << IEEE_FLT_MANTISSA_BITS;
	float y = *reinterpret_cast<float *>(&exponent);
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
ID_INLINE int idMath::IPow(const int x, int y ) {
	int r; for( r = x; y > 1; y-- ) { r *= x; } return r;
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
ID_INLINE int idMath::BitsForFloat(const float f ) {
	return ILog2( f ) + 1;
}

/*
========================
idMath::BitsForInteger
========================
*/
ID_INLINE int idMath::BitsForInteger(const int i ) {
	return ILog2( static_cast<float>(i) ) + 1;
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
ID_INLINE int idMath::FloorPowerOfTwo( int x ) {
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	return x >> 1;
}

/*
========================
idMath::CeilPowerOfTwo
========================
*/
ID_INLINE int idMath::CeilPowerOfTwo( int x ) {
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	return x;
}

/*
========================
idMath::IsPowerOfTwo
========================
*/
template <class T>
ID_INLINE constexpr bool idMath::IsPowerOfTwo(T v) noexcept {
	using D = std::remove_cv_t<T>;

	// --- classify types (robust to buggy stdlib trait behavior) ---
	constexpr bool is_integral_like =
		std::is_integral_v<D> ||
		std::is_same_v<D, int64> ||
		std::is_same_v<D, uint64> ||
		std::is_same_v<D, size_t>;

	constexpr bool is_float_like =
		std::is_floating_point_v<D> ||
		std::is_same_v<D, long double>;

	static_assert(is_integral_like || is_float_like,
		"IsPowerOfTwo<T>: T must be an integer or floating-point type");

	// ========================= INTEGRAL PATH =========================
	if constexpr (is_integral_like && !is_float_like) {
		// Handle bool explicitly: true == 1 is a power of two
		if constexpr (std::is_same_v<D, bool>) {
			return v;
		}
		else if constexpr (std::is_signed_v<D>) {
			if (v <= 0)
			{
				return false;
			}
			using U = std::make_unsigned_t<D>;
			U u = static_cast<U>(v);
			return (u & (u - U{ 1 })) == U{ 0 };
		}
		else {
			// Unsigned (covers size_t, uint64_t, etc.)
			if (v == 0)
			{
				return false;
			}
			using U = D;
			return (v & (v - U{ 1 })) == U{ 0 };
		}
	}

	// ======================= FLOATING-POINT PATH =====================
	// Common fast rejects first
	if (!(v > D(0)) || !std::isfinite(static_cast<long double>(v)))
	{
		return false;
	}

	// IEEE-754 fast paths for float and double using bit patterns
	if constexpr (std::is_same_v<D, float>) {
		const uint32 u = std::bit_cast<uint32>(v);
		constexpr uint32 EXP = 0x7F80'0000u;
		constexpr uint32 MAN = 0x007F'FFFFu;
		const uint32 exp = u & EXP;
		const uint32 man = u & MAN;
		// normalized: mantissa==0; subnormal: exactly one bit in mantissa
		return (exp ? (man == 0u) : (man != 0u && (man & (man - 1u)) == 0u));
	}
	else if constexpr (std::is_same_v<D, double>) {
		const uint64 u = std::bit_cast<uint64>(v);
		constexpr uint64 EXP = 0x7FF0'0000'0000'0000ull;
		constexpr uint64 MAN = 0x000F'FFFF'FFFF'FFFFull;
		const uint64 exp = u & EXP;
		const uint64 man = u & MAN;
		return (exp ? (man == 0ull) : (man != 0ull && (man & (man - 1ull)) == 0ull));
	}
	else {
		// Portable fallback (covers long double and others):
		// x is a power of two iff frexp(x,&e) returns mantissa exactly 0.5
		int e = 0;
		const auto m = std::frexp(v, &e);       // m is float/double/long double as appropriate, v == m * 2^e, with 0.5 <= m < 1
		using MantissaT = decltype(m);
		return std::equal_to<MantissaT>()(m, MantissaT{ 0.5 });
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
ID_INLINE int idMath::Abs( int x ) {
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
ID_INLINE float idMath::Floor(const float f ) {
	return floorf( f );
}

/*
========================
idMath::Ceil
========================
*/
ID_INLINE float idMath::Ceil(const float f ) {
	return ceilf( f );
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
idMath::Ftoi
========================
*/
ID_INLINE int idMath::Ftoi( float f ) {
#ifdef ID_WIN_X86_SSE_INTRIN
	// If a converted result is larger than the maximum signed doubleword integer,
	// the floating-point invalid exception is raised, and if this exception is masked,
	// the indefinite integer value (80000000H) is returned.
	__m128 x = _mm_load_ss( &f );
	return _mm_cvttss_si32( x );
#elif 0 // round chop (C/C++ standard)
	int i, s, e, m, shift;
	i = *reinterpret_cast<int *>(&f);
	s = i >> IEEE_FLT_SIGN_BIT;
	e = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	m = ( i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 ) ) | ( 1 << IEEE_FLT_MANTISSA_BITS );
	shift = e - IEEE_FLT_MANTISSA_BITS;
	return ( ( ( ( m >> -shift ) | ( m << shift ) ) & ~( e >> INT32_SIGN_BIT ) ) ^ s ) - s;
#else
	// If a converted result is larger than the maximum signed doubleword integer the result is undefined.
	//return C_FLOAT_TO_INT( f );
	return _cvt_ftoi_fast(f);
#endif
}

ID_INLINE int idMath::Ftoi(double d) {
	return _cvt_dtoi_fast(d);
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
ID_INLINE float idMath::Dtof(double d) noexcept {
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
I2F_FORCEINLINE void idMath::Itofv(DestFloat* I2F_RESTRICT dst, const Src* I2F_RESTRICT src, size_t count) noexcept
{
	using namespace idMath_i2f;
	constexpr std::uintmax_t LIMIT = kExactLimit<DestFloat>;

	for (size_t i = 0; i < count; ++i) {
#if _DEBUG
		const std::uintmax_t mag = umagnitude(src[i]);
		// If DestFloat has >= uintmax_t precision bits, LIMIT==max and this trivially holds.
		assert(mag <= LIMIT && "Itof: integer exceeds exact precision of destination floating type");
#endif
		dst[i] = static_cast<DestFloat>(src[i]); // exact when the assert holds
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
idMath::integer_cast
========================
*/
template <typename Dest, typename Src>
[[nodiscard]] inline Dest idMath::integer_cast(Src v)
	requires (
idMath_integral_signs::is_integral_compat_v<Dest> &&
(std::is_arithmetic_v<idMath_integral_signs::decay_cvref_t<Src>> ||
	std::is_enum_v<idMath_integral_signs::decay_cvref_t<Src>>)
	)
{
	using D_raw = idMath_integral_signs::decay_cvref_t<Dest>;
	using S_raw = idMath_integral_signs::decay_cvref_t<Src>;
	using D_fixed = idMath_integral_signs::SafeIntCompat_t<D_raw>;

	// Fast path: destination is bool
	if constexpr (std::is_same_v<D_raw, bool>) {
		if constexpr (std::is_floating_point_v<S_raw>) {
			if (!std::isfinite(static_cast<long double>(v))) {
				throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
			}
			return static_cast<bool>(v != static_cast<S_raw>(0));
		}
		else if constexpr (std::is_enum_v<S_raw>) {
			using U = std::underlying_type_t<S_raw>;
			return static_cast<bool>(static_cast<U>(v) != static_cast<U>(0));
		}
		else {
			// integral (incl. bool)
			return static_cast<bool>(v != 0);
		}
	}

	// Non-bool destination:
	if constexpr (idMath_integral_signs::is_integral_compat_v<S_raw> || std::is_enum_v<S_raw>) {
		// Ordinal/enum source → SafeCast (normalize both sides)
		using S_fixed = typename idMath_integral_signs::normalize_src<S_raw>::type;

		if constexpr (std::is_same_v<D_raw, S_raw>) {
			return v; // exact match → zero-cost
		}
		else {
			const S_fixed ssrc = static_cast<S_fixed>(v);
			D_fixed ddst{};
			if (!msl::utilities::SafeCast(ssrc, ddst)) {
				throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
			}
			return static_cast<D_raw>(ddst);
		}
	}
	else if constexpr (std::is_floating_point_v<S_raw>) {
		// Floating → integral: validate finite & in-range, then truncate toward zero
		idMath_integral_signs::ensure_fp_in_range<D_fixed>(v);
		D_fixed ddst = static_cast<D_fixed>(v); // truncation toward zero per C++
		return static_cast<D_raw>(ddst);
	}
	else {
		// Shouldn't reach (constraints restrict to arithmetic or enum)
		static_assert(std::is_arithmetic_v<S_raw> || std::is_enum_v<S_raw>,
			"integer_cast<Dest>(Src): Src must be arithmetic or enum.");
		return D_raw{}; // placate MSVC warnings in unreachable paths
	}
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
idMath::ClampFloat
========================
*/
ID_INLINE float idMath::ClampFloat(const float min, const float max, const float value ) {
	return Max( min, Min( max, value ) );
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
ID_INLINE int idMath::FloatHash( const float *array, const int numFloats ) {
	int hash = 0;

	const int* ptr = reinterpret_cast<const int*>(array);
	for ( int i = 0; i < numFloats; i++ ) {
		hash ^= ptr[i];
	}
	return hash;
}

template< typename T >
ID_INLINE_EXTERN T Lerp( const T from, const T to, float f ) { 
	return from + ( ( to - from ) * f );
}

//template<>
ID_INLINE_EXTERN int Lerp( const int from, const int to, const float f ) { 
	return idMath::Ftoi( static_cast<float>(from) + ( ( static_cast<float>(to) - static_cast<float>(from) ) * f ) );
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
