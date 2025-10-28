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

#ifndef __SYS_NUMERIC_CAST_H__
#define __SYS_NUMERIC_CAST_H__

#pragma once

// hide annoyances
#pragma warning( disable: 26481 )	// warning C26481: Don't use pointer arithmetic. Use span instead (bounds.1).
#pragma warning( disable: 26490 )	// warning C26490: Don't use reinterpret_cast (type.1).

// numeric_cast_all.hpp — Safe + fast numeric conversions (C++20 / MSVC 2022)
//
// Features:
//   • numeric_cast<Dest>(Src)  — unified safe cast among integral/enum/bool/float/double/long double
//   • Batched span converters (Try… non-throwing / … throwing):
//       - FP  -> Int : TryNumericCastSpanFpToInt / NumericCastSpanFpToInt
//       - Int -> FP  : TryNumericCastSpanIntToFp / NumericCastSpanIntToFp
//       - FP  <-> FP : TryNumericCastSpanFpToFp  / NumericCastSpanFpToFp
//       - Int <-> Int: TryNumericCastSpanIntToInt / NumericCastSpanIntToInt
//   • SIMD hot paths:
//       - x86/x64: AVX-512 > AVX2 > AVX > SSE2 fallbacks
//       - ARM: NEON (AArch64: float<->double vectorized; ARMv7: float/int32 vectorized)
//   • Exact NaN policy for FP<->FP spans (propagate by default)
//   • No “magic numbers”: all bounds in numeric_cast_internal::constants
//   • SafeInt integration: uses bool-return SafeCast(T from, U& to)
//   • Throws use SafeIntError::SafeIntArithmeticOverflow
//
// Assumptions present in your codebase (as you stated):
//   template<class T> concept Numeric = std::is_arithmetic_v<T>;
//   typedefs: byte/word/dword/uint/ulong, int8/uint8/int16/uint16/int32/uint32/int64/uint64,
//             index_t, size_t (unsigned __int64), ptrdiff_t (__int64), intptr_t (__int64)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <cmath>

#include <safeint.h> // msl::utilities::SafeCast(T from, U& to) -> bool

#if defined(_MSC_VER)
#include <intrin.h>   // _cvt_*_fast (x86/x64)
#endif

#if defined(_M_ARM64)
#include <arm64_neon.h>
#elif defined(__aarch64__) || defined(_M_ARM) || defined(__arm__)
#include <arm_neon.h>
#endif

// x86 SIMD headers
#if defined(__AVX512F__) || defined(__AVX2__) || defined(__AVX__) || defined(__SSE2__) || (defined(_MSC_VER) && (defined(_M_X64)||defined(_M_IX86)))
#include <immintrin.h>
#include <emmintrin.h>
#include <xmmintrin.h>
#endif

// -------------------------------- Config knobs -------------------------------
// Require exact integer representability for Int -> Float casts?
#define NUMERIC_CAST_REQUIRE_EXACT_INT_TO_FLOAT

// Allow NaN to pass through casts
#define NUMERIC_CAST_NAN_PROPAGATE

// ---- SIMD feature detection (prefer x86 AVX-512 > AVX2 > AVX > SSE2; then NEON64; else scalar) ----
#if defined(__AVX512F__)
#define NC_HAS_AVX512F
#endif
#if defined(__AVX2__)
#define NC_HAS_AVX2
#endif
#if defined(__AVX__)
#define NC_HAS_AVX
#endif
#if defined(__SSE2__) || (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86)))
#define NC_HAS_SSE2
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
  // AArch64 always has NEON; supports fp64 NEON
#define NC_HAS_NEON64
#elif (defined(__ARM_NEON) || defined(__ARM_NEON__)) && (defined(__arm__) || defined(_M_ARM))
#define NC_HAS_NEON
#endif

namespace numeric_cast_internal {

	// =============================== constants ===============================
	namespace constants {
		// Powers of two and related bounds
		inline constexpr float  TWO31_F = 2147483648.0f;              // 2^31
		inline constexpr float  TWO32_F = 4294967296.0f;              // 2^32
		inline constexpr double TWO63_D = 9223372036854775808.0;      // 2^63
		inline constexpr double TWO64_D = 18446744073709551616.0;     // 2^64
		inline constexpr float  TWO64_F = 1.8446744e19f;              // 2^64 rounded to float

		// float max as double for precise compare in f64->f32
		inline constexpr double F32_MAX_D = static_cast<double>(std::numeric_limits<float>::max());
		inline constexpr float  F32_MAX_F = std::numeric_limits<float>::max();

		// Handy zeros
		inline constexpr float  F0 = 0.0f;
		inline constexpr double D0 = 0.0;
	} // namespace constants

	// =============================== Traits =================================
	template <class T>
	using decay_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

	// Treat MSVC aliases as integral-compatible as well
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
			|| std::is_same_v<U, size_t>
			|| std::is_same_v<U, ptrdiff_t>
#endif
			;
	};
	template <class T>
	inline constexpr bool is_integral_compat_v = is_integral_compat<T>::value;

	template <class T, bool Signed, size_t Bytes> struct fixed_width_of_size;
	template <class T> struct fixed_width_of_size<T, true,  8> { using type = int64;  };
	template <class T> struct fixed_width_of_size<T, true,  4> { using type = int32;  };
	template <class T> struct fixed_width_of_size<T, true,  2> { using type = int16;  };
	template <class T> struct fixed_width_of_size<T, true,  1> { using type = int8;   };
	template <class T> struct fixed_width_of_size<T, false, 8> { using type = uint64; };
	template <class T> struct fixed_width_of_size<T, false, 4> { using type = uint32; };
	template <class T> struct fixed_width_of_size<T, false, 2> { using type = uint16; };
	template <class T> struct fixed_width_of_size<T, false, 1> { using type = uint8;  };

	template <class T>
	using FixedLike_t = typename fixed_width_of_size<
		T, std::is_signed_v<decay_cvref_t<T>>, sizeof(decay_cvref_t<T>)
	>::type;

	// Normalize integral/enum SOURCE to SafeInt-friendly fixed width
	template <class S, bool IsEnum = std::is_enum_v<decay_cvref_t<S>>>
	struct normalize_src;
	template <class S>
	struct normalize_src<S, true> {
		using raw = decay_cvref_t<S>;
		using base = std::underlying_type_t<raw>;
		using type = FixedLike_t<base>;
	};
	template <class S>
	struct normalize_src<S, false> {
		using raw = decay_cvref_t<S>;
		using type = FixedLike_t<raw>;
	};

	// ======================= Guard helpers & checks =========================
	template <class DestFloat, class Float>
	inline bool check_fp_to_fp_ok(Float x) {
		static_assert(std::is_floating_point_v<DestFloat>);
		const long double xv = static_cast<long double>(x);
		if (!std::isfinite(xv)) {
#if defined(NUMERIC_CAST_NAN_PROPAGATE)
			if (std::isnan(static_cast<long double>(x)))
			{
				return true; // allow NaN to propagate
			}
#endif
			return false; // reject infinities
		}
		if constexpr (!std::is_same_v<DestFloat, long double>) {
			constexpr long double lo = static_cast<long double>(std::numeric_limits<DestFloat>::lowest());
			constexpr long double hi = static_cast<long double>(std::numeric_limits<DestFloat>::max());
			if (xv < lo || xv > hi)
			{
				return false;
			}
		}
		return true;
	}

	template <class DestFloat, class Float>
	inline void ensure_fp_to_fp_in_range(Float x) {
		if (!check_fp_to_fp_ok<DestFloat>(x)) {
			throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
		}
	}

	template <class DestFixed, class Float>
	inline void ensure_fp_to_int_in_range(Float x) {
		static_assert(std::is_integral_v<DestFixed>);
		const long double xv = static_cast<long double>(x);
		if (!std::isfinite(xv)) {
			throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
		}
		constexpr long double lo = static_cast<long double>(std::numeric_limits<DestFixed>::min());
		constexpr long double hi = static_cast<long double>(std::numeric_limits<DestFixed>::max());
		if (xv < lo || xv > hi) {
			throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
		}
	}

	template <class DestFloat, class Int>
	inline void ensure_int_to_fp_exact(Int x) {
#if defined(NUMERIC_CAST_REQUIRE_EXACT_INT_TO_FLOAT)
		constexpr int p = std::numeric_limits<DestFloat>::digits; // mantissa bits
		using U = std::make_unsigned_t<decay_cvref_t<Int>>;
		const U ax = (x >= 0) ? static_cast<U>(x)
			: static_cast<U>(-(static_cast<std::make_signed_t<U>>(x)));
		if constexpr (p < 64) {
			constexpr unsigned long long limit = 1ULL << p; // exact integers are |x| < 2^p
			if (ax >= limit) {
				throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
			}
		}
#else
		(void)x;
#endif
	}

	// =================== Fast scalar FP->Int (x86/x64) =====================
	template <class D, class F>
	__forceinline bool try_fp_to_int_fast_x86(F x, D& out) {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
		using namespace constants;
		static_assert(std::is_floating_point_v<F>);
		static_assert(std::is_integral_v<D>);
		if (std::isnan(x))
		{
			return false; // NaN
		}

		if constexpr (std::is_same_v<F, double>) {
			if constexpr (std::is_same_v<D, std::uint64_t>) {
				if (x < D0 || x >= TWO64_D)
				{
					return false;
				}
				out = static_cast<D>(_cvt_dtoull_fast(x)); return true;
			}
			else if constexpr (std::is_same_v<D, std::int64_t>) {
				constexpr double lo = static_cast<double>(std::numeric_limits<int64>::min());
				constexpr double hi = static_cast<double>(std::numeric_limits<int64>::max()) + 1.0;
				if (x <= lo || x >= hi)
				{
					return false;
				}
				out = static_cast<D>(_cvt_dtoll_fast(x)); return true;
			}
			else if constexpr (std::is_same_v<D, std::uint32_t>) {
				if (x < D0 || x >= static_cast<double>(TWO32_F))
				{
					return false;
				}
				out = static_cast<D>(_cvt_dtoui_fast(x)); return true;
			}
			else if constexpr (std::is_same_v<D, std::int32_t>) {
				constexpr double lo = static_cast<double>(std::numeric_limits<int32>::min());
				constexpr double hi = static_cast<double>(std::numeric_limits<int32>::max()) + 1.0;
				if (x <= lo || x >= hi)
				{
					return false;
				}
				out = static_cast<D>(_cvt_dtoi_fast(x)); return true;
			}
		}
		else { // float
			if constexpr (std::is_same_v<D, std::uint64_t>) {
				if (x < F0 || x >= TWO64_F)
				{
					return false;
				}
				out = static_cast<D>(_cvt_ftoull_fast(x)); return true;
			}
			else if constexpr (std::is_same_v<D, std::int64_t>) {
				constexpr float lo = static_cast<float>(std::numeric_limits<int64>::min());
				constexpr float hi = static_cast<float>(std::numeric_limits<int64>::max()) + 1.0f;
				if (x <= lo || x >= hi)
				{
					return false;
				}
				out = static_cast<D>(_cvt_ftoll_fast(x)); return true;
			}
			else if constexpr (std::is_same_v<D, std::uint32_t>) {
				if (x < F0 || x >= TWO32_F)
				{
					return false;
				}
				out = static_cast<D>(_cvt_ftoui_fast(x)); return true;
			}
			else if constexpr (std::is_same_v<D, std::int32_t>) {
				constexpr float lo = static_cast<float>(std::numeric_limits<int32>::min());
				constexpr float hi = static_cast<float>(std::numeric_limits<int32>::max()) + 1.0f;
				if (x <= lo || x >= hi)
				{
					return false;
				}
				out = static_cast<D>(_cvt_ftoi_fast(x)); return true;
			}
		}
#endif
		(void)x; (void)out; return false;
	}

	// =================== Fast scalar FP->Int (ARM NEON) ====================
#if (defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM))
	__forceinline bool try_fp_to_int_fast_arm(const float x, int32& out) {
		using namespace constants;
		if (!(x == x))
		{
			return false;
		}
		const float lo = static_cast<float>(std::numeric_limits<int32>::min());
		const float hi = static_cast<float>(std::numeric_limits<int32>::max()) + 1.0f;
		if (x <= lo || x >= hi)
		{
			return false;
		}
		const float32x4_t fx = vdupq_n_f32(x);
		const int32x4_t   vi = vcvtq_s32_f32(fx);
		out = vgetq_lane_s32(vi, 0);
		return true;
	}
	__forceinline bool try_fp_to_int_fast_arm(const float x, uint32& out) {
		using namespace constants;
		if (!(x == x))
		{
			return false;
		}
		if (x < F0 || x >= TWO32_F)
		{
			return false;
		}
		const float32x4_t fx = vdupq_n_f32(x);
		const uint32x4_t  vu = vcvtq_u32_f32(fx);
		out = vgetq_lane_u32(vu, 0);
		return true;
	}
#if defined(__aarch64__) || defined(_M_ARM64)
	__forceinline bool try_fp_to_int_fast_arm(const double x, int64& out) {
		if (!(x == x))
		{
			return false;
		}
		const double lo = static_cast<double>(std::numeric_limits<int64>::min());
		const double hi = static_cast<double>(std::numeric_limits<int64>::max()) + 1.0;
		if (x <= lo || x >= hi)
		{
			return false;
		}
		const float64x2_t dx = vdupq_n_f64(x);
		const int64x2_t   vi = vcvtq_s64_f64(dx);
		out = vgetq_lane_s64(vi, 0);
		return true;
	}
	__forceinline bool try_fp_to_int_fast_arm(const double x, uint64& out) {
		using namespace constants;
		if (!(x == x))
		{
			return false;
		}
		if (x < D0 || x >= TWO64_D)
		{
			return false;
		}
		const float64x2_t dx = vdupq_n_f64(x);
		const uint64x2_t  vu = vcvtq_u64_f64(dx);
		out = vgetq_lane_u64(vu, 0);
		return true;
	}
#endif
#endif

	// ===================== Vector kernels (float -> int32/uint32) =====================
	__forceinline bool kernel_f32x4_to_s32x4(const float* src, int32* dst) noexcept {
		using namespace constants;
#if   defined(NC_HAS_AVX2) || defined(NC_HAS_SSE2)
		const __m128 x = _mm_loadu_ps(src);
		const __m128 nan = _mm_cmpunord_ps(x, x); if (_mm_movemask_ps(nan))
		{
			return false;
		}
		const __m128 loOk = _mm_cmpgt_ps(x, _mm_set1_ps(static_cast<float>(std::numeric_limits<int32>::min())));
		const __m128 hiOk = _mm_cmplt_ps(x, _mm_set1_ps(static_cast<float>(std::numeric_limits<int32>::max()) + 1.0f));
		if (_mm_movemask_ps(_mm_and_ps(loOk, hiOk)) != 0xF)
		{
			return false;
		}
		const __m128i r = _mm_cvttps_epi32(x);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(dst), r);
		return true;
#elif defined(NC_HAS_NEON) || defined(NC_HAS_NEON64)
		const float s0 = src[0], s1 = src[1], s2 = src[2], s3 = src[3];
		const float lo = static_cast<float>(std::numeric_limits<int32>::min());
		const float hi = static_cast<float>(std::numeric_limits<int32>::max()) + 1.0f;
		if (!(s0 == s0 && s0 > lo && s0<hi && s1 == s1 && s1>lo && s1<hi && s2 == s2 && s2>lo && s2 < hi && s3 == s3 && s3 < hi))
		{
			return false;
		}
		const float32x4_t fx = vld1q_f32(src);
		const int32x4_t   vi = vcvtq_s32_f32(fx);
		vst1q_s32(dst, vi);
		return true;
#else
		(void)src; (void)dst; return false;
#endif
	}

	__forceinline bool kernel_f32x4_to_u32x4(const float* src, uint32* dst) noexcept {
		using namespace constants;
#if   defined(NC_HAS_AVX2) || defined(NC_HAS_SSE2)
		const __m128 x = _mm_loadu_ps(src);
		const __m128 nan = _mm_cmpunord_ps(x, x); if (_mm_movemask_ps(nan))
		{
			return false;
		}
		const __m128 ge0 = _mm_cmpge_ps(x, _mm_set1_ps(F0));
		const __m128 lt2 = _mm_cmplt_ps(x, _mm_set1_ps(TWO32_F));
		if (_mm_movemask_ps(_mm_and_ps(ge0, lt2)) != 0xF)
		{
			return false;
		}

		const __m128  two31f = _mm_set1_ps(TWO31_F);
		const __m128  ge2 = _mm_cmpge_ps(x, two31f);
		const __m128  xAdj = _mm_sub_ps(x, _mm_and_ps(ge2, two31f));
		const __m128i lo = _mm_cvttps_epi32(xAdj);
		const __m128i add = _mm_and_si128(_mm_castps_si128(ge2), _mm_set1_epi32(0x80000000));
		const __m128i res = _mm_add_epi32(lo, add);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(dst), res);
		return true;
#elif defined(NC_HAS_NEON) || defined(NC_HAS_NEON64)
		const float s0 = src[0], s1 = src[1], s2 = src[2], s3 = src[3];
		if (!(s0 == s0 && s0 >= F0 && s0 < TWO32_F &&
			s1 == s1 && s1 >= F0 && s1 < TWO32_F &&
			s2 == s2 && s2 >= F0 && s2 < TWO32_F &&
			s3 == s3 && s3 >= F0 && s3 < TWO32_F))
		{
			return false;
		}
		const float32x4_t fx = vld1q_f32(src);
		const uint32x4_t  vu = vcvtq_u32_f32(fx);
		vst1q_u32(dst, vu);
		return true;
#else
		(void)src; (void)dst; return false;
#endif
	}

	// ======================= FP<->FP vectorized helpers =====================

	// float -> double (prefer AVX-512 > AVX > SSE2 > NEON64)
	__forceinline std::size_t f32_to_f64_vector(const float* src, double* dst, size_t n) noexcept {
		size_t i = 0;
#if   defined(NC_HAS_AVX512F)
		for (; i + 16 <= n; i += 16) {
			const __m256  lo = _mm256_loadu_ps(src + i + 0);
			const __m256  hi = _mm256_loadu_ps(src + i + 8);
			const __m512d d0 = _mm512_cvtps_pd(lo);
			const __m512d d1 = _mm512_cvtps_pd(hi);
			_mm512_storeu_pd(dst + i + 0, d0);
			_mm512_storeu_pd(dst + i + 8, d1);
		}
#elif defined(NC_HAS_AVX)
		for (; i + 8 <= n; i += 8) {
			const __m128  lo = _mm_loadu_ps(src + i + 0);
			const __m128  hi = _mm_loadu_ps(src + i + 4);
			const __m256d d0 = _mm256_cvtps_pd(lo);
			const __m256d d1 = _mm256_cvtps_pd(hi);
			_mm256_storeu_pd(dst + i + 0, d0);
			_mm256_storeu_pd(dst + i + 4, d1);
		}
#elif defined(NC_HAS_SSE2)
		for (; i + 4 <= n; i += 4) {
			const __m128  f = _mm_loadu_ps(src + i);
			const __m128d d0 = _mm_cvtps_pd(f);
			const __m128  f2 = _mm_movehl_ps(f, f);
			const __m128d d1 = _mm_cvtps_pd(f2);
			_mm_storeu_pd(dst + i + 0, d0);
			_mm_storeu_pd(dst + i + 2, d1);
		}
#elif defined(NC_HAS_NEON64)
		// AArch64 NEON float[4]→double[4]
		for (; i + 4 <= n; i += 4) {
			const float32x4_t vf = vld1q_f32(src + i);
			const float32x2_t lo2 = vget_low_f32(vf);
			const float32x2_t hi2 = vget_high_f32(vf);
			const float64x2_t d0 = vcvt_f64_f32(lo2);
			const float64x2_t d1 = vcvt_f64_f32(hi2);
			vst1q_f64(dst + i + 0, d0);
			vst1q_f64(dst + i + 2, d1);
		}
#endif
		return i;
	}

	// double -> float (prefer AVX-512 > AVX > SSE2 > NEON64); NaN policy honored
	__forceinline std::size_t f64_to_f32_vector(const double* src, float* dst, size_t n, bool& ok_all) noexcept {
		using namespace constants;
		size_t i = 0; ok_all = true;
#if   defined(NC_HAS_AVX512F)
		const __m512d maxf = _mm512_set1_pd(F32_MAX_D);
		for (; i + 16 <= n; i += 16) {
			const __m512d a0 = _mm512_loadu_pd(src + i + 0);
			const __m512d a1 = _mm512_loadu_pd(src + i + 8);
			const __m512d abs0 = _mm512_abs_pd(a0);
			const __m512d abs1 = _mm512_abs_pd(a1);
			const __mmask8 ok0 = _mm512_cmp_pd_mask(abs0, maxf, _CMP_LE_OQ); // false for NaN or |x|>max
			const __mmask8 ok1 = _mm512_cmp_pd_mask(abs1, maxf, _CMP_LE_OQ);
#if defined(NUMERIC_CAST_NAN_PROPAGATE)
			const __mmask8 nan0 = _mm512_cmp_pd_mask(a0, a0, _CMP_UNORD_Q);
			const __mmask8 nan1 = _mm512_cmp_pd_mask(a1, a1, _CMP_UNORD_Q);
			const __mmask8 bad0 = (~ok0) & (~nan0);
			const __mmask8 bad1 = (~ok1) & (~nan1);
#else
			const __mmask8 bad0 = (~ok0);
			const __mmask8 bad1 = (~ok1);
#endif
			if (bad0 || bad1) { ok_all = false; return i; }
			const __m256 f0 = _mm512_cvtpd_ps(a0);
			const __m256 f1 = _mm512_cvtpd_ps(a1);
			_mm256_storeu_ps(dst + i + 0, f0);
			_mm256_storeu_ps(dst + i + 8, f1);
		}
#elif defined(NC_HAS_AVX)
		const __m256d maxf = _mm256_set1_pd(F32_MAX_D);
		const __m256d signmask = _mm256_set1_pd(-0.0);
		for (; i + 8 <= n; i += 8) {
			const __m256d d0 = _mm256_loadu_pd(src + i + 0);
			const __m256d d1 = _mm256_loadu_pd(src + i + 4);
			const __m256d a0 = _mm256_andnot_pd(signmask, d0);
			const __m256d a1 = _mm256_andnot_pd(signmask, d1);
			const __m256d c0 = _mm256_cmp_pd(a0, maxf, _CMP_LE_OQ);
			const __m256d c1 = _mm256_cmp_pd(a1, maxf, _CMP_LE_OQ);
#if defined(NUMERIC_CAST_NAN_PROPAGATE)
			const __m256d nn0 = _mm256_cmp_pd(d0, d0, _CMP_UNORD_Q);
			const __m256d nn1 = _mm256_cmp_pd(d1, d1, _CMP_UNORD_Q);
			const int bad0 = _mm256_movemask_pd(_mm256_andnot_pd(_mm256_or_pd(c0, nn0), _mm256_set1_pd(-1)));
			const int bad1 = _mm256_movemask_pd(_mm256_andnot_pd(_mm256_or_pd(c1, nn1), _mm256_set1_pd(-1)));
#else
			const int bad0 = _mm256_movemask_pd(_mm256_xor_pd(c0, _mm256_set1_pd(-1)));
			const int bad1 = _mm256_movemask_pd(_mm256_xor_pd(c1, _mm256_set1_pd(-1)));
#endif
			if (bad0 || bad1) { ok_all = false; return i; }
			const __m128 f0 = _mm256_cvtpd_ps(d0);
			const __m128 f1 = _mm256_cvtpd_ps(d1);
			_mm_storeu_ps(dst + i + 0, f0);
			_mm_storeu_ps(dst + i + 4, f1);
		}
#elif defined(NC_HAS_SSE2)
		const __m128d maxf = _mm_set1_pd(F32_MAX_D);
		const __m128d signmask = _mm_set1_pd(-0.0);
		for (; i + 4 <= n; i += 4) {
			const __m128d d0 = _mm_loadu_pd(src + i + 0);
			const __m128d d1 = _mm_loadu_pd(src + i + 2);
			const __m128d a0 = _mm_andnot_pd(signmask, d0);
			const __m128d a1 = _mm_andnot_pd(signmask, d1);
			const __m128d c0 = _mm_cmple_pd(a0, maxf);
			const __m128d c1 = _mm_cmple_pd(a1, maxf);
#if defined(NUMERIC_CAST_NAN_PROPAGATE)
			const __m128d nn0 = _mm_cmpunord_pd(d0, d0);
			const __m128d nn1 = _mm_cmpunord_pd(d1, d1);
			const int bad0 = _mm_movemask_pd(_mm_andnot_pd(_mm_or_pd(c0, nn0), _mm_set1_pd(-1)));
			const int bad1 = _mm_movemask_pd(_mm_andnot_pd(_mm_or_pd(c1, nn1), _mm_set1_pd(-1)));
#else
			const int bad0 = _mm_movemask_pd(_mm_xor_pd(c0, _mm_set1_pd(-1)));
			const int bad1 = _mm_movemask_pd(_mm_xor_pd(c1, _mm_set1_pd(-1)));
#endif
			if (bad0 || bad1) { ok_all = false; return i; }
			const __m128 f0 = _mm_cvtpd_ps(d0);
			const __m128 f1 = _mm_cvtpd_ps(d1);
			_mm_storeu_ps(dst + i + 0, f0);
			_mm_storeu_ps(dst + i + 2, f1);
		}
#elif defined(NC_HAS_NEON64)
		// AArch64 NEON double[4]→float[4]
		for (; i + 4 <= n; i += 4) {
			const double s0 = src[i + 0], s1 = src[i + 1], s2 = src[i + 2], s3 = src[i + 3];
			auto lane_ok = [](const double x)->bool {
				using namespace constants;
				const double ax = std::fabs(x);
#if defined(NUMERIC_CAST_NAN_PROPAGATE)
				if (std::isnan(x))
				{
					return true;
				}
#endif
				if (!std::isfinite(x))
				{
					return false;
				}
				return ax <= F32_MAX_D;
				};
			if (!(lane_ok(s0) && lane_ok(s1) && lane_ok(s2) && lane_ok(s3))) { ok_all = false; return i; }

			const float64x2_t d0 = vld1q_f64(src + i + 0);
			const float64x2_t d1 = vld1q_f64(src + i + 2);
			const float32x2_t f0 = vcvt_f32_f64(d0);
			const float32x2_t f1 = vcvt_f32_f64(d1);
			const float32x4_t f = vcombine_f32(f0, f1);
			vst1q_f32(dst + i, f);
		}
#endif
		return i;
	}

	// ================== Scalar helpers for spans (const-correct) =================
	template <class D>
	inline bool scalar_checked_float_to_int(const float x, D& out) {
		using DF = FixedLike_t<D>;
		if (!(x == x))
		{
			return false;
		}
		const long double xv = static_cast<long double>(x);
		const long double lo = static_cast<long double>(std::numeric_limits<DF>::min());
		const long double hi = static_cast<long double>(std::numeric_limits<DF>::max());
		if (xv < lo || xv > hi)
		{
			return false;
		}
		out = static_cast<D>(static_cast<DF>(x));
		return true;
	}
	template <class D>
	inline bool scalar_checked_double_to_int(const double x, D& out) {
		using DF = FixedLike_t<D>;
		if (!(x == x))
		{
			return false;
		}
		const long double xv = static_cast<long double>(x);
		const long double lo = static_cast<long double>(std::numeric_limits<DF>::lowest());
		const long double hi = static_cast<long double>(std::numeric_limits<DF>::max());
		if (xv < lo || xv > hi)
		{
			return false;
		}
		out = static_cast<D>(static_cast<DF>(x));
		return true;
	}
	template <class DF, class SF>
	inline bool scalar_checked_fp_to_fp(const SF x, DF& out) {
		if (!check_fp_to_fp_ok<DF>(x))
		{
			return false;
		}
		out = static_cast<DF>(x);
		return true;
	}

	template <class D, class S>
	inline bool scalar_safe_int_to_int(const S v, D& out) {
		using Df = FixedLike_t<D>;
		using Sf = typename normalize_src<S>::type;
		Df dd{};
		if (!msl::utilities::SafeCast(static_cast<Sf>(v), dd)) {
			return false;
		}
		out = static_cast<D>(dd);
		return true;
	}

} // namespace numeric_cast_internal

// ======================================================================
//                          BATCHED CONVERSIONS
// ======================================================================

// ---------- FP -> Int (float) ----------
template <class Dest>
inline bool TryNumericCastSpanFpToInt(const float* src, std::size_t n, Dest* dst, std::size_t* first_bad = nullptr)
	requires (numeric_cast_internal::is_integral_compat_v<Dest>)
{
	using namespace numeric_cast_internal;
	using D0 = FixedLike_t<Dest>;

	std::size_t i = 0;

	if constexpr (sizeof(D0) == 4) {
		for (; i + 4 <= n; i += 4) {
			alignas(16) int32 ts[4];
			alignas(16) uint32 tu[4];
			bool ok = false;
			if constexpr (std::is_signed_v<D0>) {
				ok = kernel_f32x4_to_s32x4(src + i, ts);
				if (ok) { dst[i + 0] = static_cast<Dest>(ts[0]); dst[i + 1] = static_cast<Dest>(ts[1]); dst[i + 2] = static_cast<Dest>(ts[2]); dst[i + 3] = static_cast<Dest>(ts[3]); continue; }
			}
			else {
				ok = kernel_f32x4_to_u32x4(src + i, tu);
				if (ok) { dst[i + 0] = static_cast<Dest>(tu[0]); dst[i + 1] = static_cast<Dest>(tu[1]); dst[i + 2] = static_cast<Dest>(tu[2]); dst[i + 3] = static_cast<Dest>(tu[3]); continue; }
			}
			for (size_t k = 0; k < 4; ++k) {
				D0 t{};
#if defined(_MSC_VER)
				if (!try_fp_to_int_fast_x86<D0>(src[i + k], t))
#elif (defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM))
				if constexpr (sizeof(D0) == 4) {
					if (!try_fp_to_int_fast_arm(src[i + k],
						*reinterpret_cast<std::conditional_t<std::is_signed_v<D0>, int32*, uint32*>>(&t))) { /* fallthrough to scalar */
					}
				}
#endif
				{
					if (!scalar_checked_float_to_int<D0>(src[i + k], t)) { if (first_bad)
						{
							*first_bad = i + k;
						}
						return false; }
				}
				dst[i + k] = static_cast<Dest>(t);
			}
		}
	}

	for (; i < n; ++i) {
		D0 t{};
#if defined(_MSC_VER)
		if (!try_fp_to_int_fast_x86<D0>(src[i], t))
#elif (defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM))
		if constexpr (sizeof(D0) == 4) {
			if (!try_fp_to_int_fast_arm(src[i],
				*reinterpret_cast<std::conditional_t<std::is_signed_v<D0>, std::int32_t*, std::uint32_t*>>(&t))) { /* fallthrough */
			}
		}
#endif
		{
			if (!scalar_checked_float_to_int<D0>(src[i], t)) { if (first_bad)
				{
					*first_bad = i;
				}
				return false; }
		}
		dst[i] = static_cast<Dest>(t);
	}
	return true;
}
template <class Dest>
inline void NumericCastSpanFpToInt(const float* src, size_t n, Dest* dst)
	requires (numeric_cast_internal::is_integral_compat_v<Dest>)
{
	std::size_t bad{};
	if (!TryNumericCastSpanFpToInt(src, n, dst, &bad)) {
		throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
	}
}

// ---------- FP -> Int (double) ----------
template <class Dest>
inline bool TryNumericCastSpanFpToInt(const double* src, size_t n, Dest* dst, size_t* first_bad = nullptr)
	requires (numeric_cast_internal::is_integral_compat_v<Dest>)
{
	using namespace numeric_cast_internal;
	using D0 = FixedLike_t<Dest>;
	for (size_t i = 0; i < n; ++i) {
		D0 t{};
#if defined(_MSC_VER)
		if (!try_fp_to_int_fast_x86<D0>(src[i], t))
#elif (defined(__aarch64__) || defined(_M_ARM64))
		if (!try_fp_to_int_fast_arm(src[i], t))
#endif
		{
			if (!scalar_checked_double_to_int<D0>(src[i], t)) { if (first_bad)
				{
					*first_bad = i;
				}
				return false; }
		}
		dst[i] = static_cast<Dest>(t);
	}
	return true;
}
template <class Dest>
inline void NumericCastSpanFpToInt(const double* src, size_t n, Dest* dst)
	requires (numeric_cast_internal::is_integral_compat_v<Dest>)
{
	size_t bad{};
	if (!TryNumericCastSpanFpToInt(src, n, dst, &bad)) {
		throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
	}
}

// ---------- Int -> FP (float/double/long double) ----------
template <class DestFloat, class SrcInt>
inline bool TryNumericCastSpanIntToFp(const SrcInt* src, size_t n, DestFloat* dst, size_t* first_bad = nullptr)
	requires (std::is_floating_point_v<numeric_cast_internal::decay_cvref_t<DestFloat>>&&
numeric_cast_internal::is_integral_compat_v<SrcInt>)
{
	using namespace numeric_cast_internal;
	using DF = decay_cvref_t<DestFloat>;
	using S0 = FixedLike_t<SrcInt>;

	size_t i = 0;
	if constexpr (std::is_same_v<DF, float> && sizeof(S0) == 4) {
		for (; i + 4 <= n; i += 4) {
			if constexpr (std::is_signed_v<S0>) {
				kernel_s32x4_to_f32x4(reinterpret_cast<const int32*>(src + i), dst + i);
			}
			else {
				kernel_u32x4_to_f32x4(reinterpret_cast<const uint32*>(src + i), dst + i);
			}
		}
	}
	for (; i < n; ++i) {
		const long double xv = static_cast<long double>(static_cast<S0>(src[i]));
		const long double lo = static_cast<long double>(std::numeric_limits<DF>::lowest());
		const long double hi = static_cast<long double>(std::numeric_limits<DF>::max());
		if (xv < lo || xv > hi) { if (first_bad)
			{
				*first_bad = i;
			}
			return false; }
#if defined(NUMERIC_CAST_REQUIRE_EXACT_INT_TO_FLOAT)
		ensure_int_to_fp_exact<DF>(static_cast<S0>(src[i]));
#endif
		dst[i] = static_cast<DF>(static_cast<S0>(src[i]));
	}
	return true;
}
template <class DestFloat, class SrcInt>
inline void NumericCastSpanIntToFp(const SrcInt* src, size_t n, DestFloat* dst)
	requires (std::is_floating_point_v<numeric_cast_internal::decay_cvref_t<DestFloat>>&&
numeric_cast_internal::is_integral_compat_v<SrcInt>)
{
	size_t bad{};
	if (!TryNumericCastSpanIntToFp(src, n, dst, &bad)) {
		throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
	}
}

// ---------- FP <-> FP (float/double/long double) ----------
template <class DestFloat, class SrcFloat>
inline bool TryNumericCastSpanFpToFp(const SrcFloat* src, size_t n, DestFloat* dst, size_t* first_bad = nullptr)
	requires (std::is_floating_point_v<numeric_cast_internal::decay_cvref_t<DestFloat>>&&
std::is_floating_point_v<numeric_cast_internal::decay_cvref_t<SrcFloat>>)
{
	using namespace numeric_cast_internal;
	using DF = decay_cvref_t<DestFloat>;
	using SF = decay_cvref_t<SrcFloat>;

	size_t i = 0;

	if constexpr (std::is_same_v<SF, float> && std::is_same_v<DF, double>) {
		i += f32_to_f64_vector(src, dst, n);
	}
	else if constexpr (std::is_same_v<SF, double> && std::is_same_v<DF, float>) {
		bool ok = true;
		i += f64_to_f32_vector(src, dst, n, ok);
		if (!ok) { if (first_bad)
			{
				*first_bad = i;
			}
			return false; }
	}

	for (; i < n; ++i) {
		DF out{};
		if (!scalar_checked_fp_to_fp<DF>(src[i], out)) { if (first_bad)
			{
				*first_bad = i;
			}
			return false; }
		dst[i] = out;
	}
	return true;
}
template <class DestFloat, class SrcFloat>
inline void NumericCastSpanFpToFp(const SrcFloat* src, size_t n, DestFloat* dst)
	requires (std::is_floating_point_v<numeric_cast_internal::decay_cvref_t<DestFloat>>&&
std::is_floating_point_v<numeric_cast_internal::decay_cvref_t<SrcFloat>>)
{
	size_t bad{};
	if (!TryNumericCastSpanFpToFp(src, n, dst, &bad)) {
		throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
	}
}

// ---------- Int <-> Int (SafeCast per element) ----------
template <class DestInt, class SrcInt>
inline bool TryNumericCastSpanIntToInt(const SrcInt* src, size_t n, DestInt* dst, size_t* first_bad = nullptr)
	requires (numeric_cast_internal::is_integral_compat_v<DestInt>&&
numeric_cast_internal::is_integral_compat_v<SrcInt>)
{
	using namespace numeric_cast_internal;
	for (size_t i = 0; i < n; ++i) {
		DestInt out{};
		if (!scalar_safe_int_to_int<DestInt, SrcInt>(src[i], out)) {
			if (first_bad)
			{
				*first_bad = i;
			}
			return false;
		}
		dst[i] = out;
	}
	return true;
}
template <class DestInt, class SrcInt>
inline void NumericCastSpanIntToInt(const SrcInt* src, size_t n, DestInt* dst)
	requires (numeric_cast_internal::is_integral_compat_v<DestInt>&&
numeric_cast_internal::is_integral_compat_v<SrcInt>)
{
	size_t bad{};
	if (!TryNumericCastSpanIntToInt(src, n, dst, &bad)) {
		throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
	}
}

// ======================================================================
//                           numeric_cast<Dest>(Src)
// ======================================================================
template <typename Dest, typename Src>
[[nodiscard]] static inline Dest numeric_cast(Src v)
	requires (
std::is_arithmetic_v<numeric_cast_internal::decay_cvref_t<Dest>> &&
(std::is_arithmetic_v<numeric_cast_internal::decay_cvref_t<Src>> ||
	std::is_enum_v<numeric_cast_internal::decay_cvref_t<Src>>)
	)
{
	using namespace numeric_cast_internal;
	using D_raw = decay_cvref_t<Dest>;
	using S_raw = decay_cvref_t<Src>;

	// ---- Dest is bool ----
	if constexpr (std::is_same_v<D_raw, bool>) {
		if constexpr (std::is_floating_point_v<S_raw>) {
			const long double xv = static_cast<long double>(v);
#if defined(NUMERIC_CAST_NAN_PROPAGATE)
			if (std::isnan(xv))
			{
				return false; // treat NaN as false; change if you prefer true
			}
#endif
			if (!std::isfinite(xv)) {
				throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
			}
			return static_cast<bool>(xv != 0.0L);
		}
		else if constexpr (std::is_enum_v<S_raw>) {
			using U = std::underlying_type_t<S_raw>;
			return static_cast<bool>(static_cast<U>(v) != static_cast<U>(0));
		}
		else {
			return static_cast<bool>(v != 0);
		}
	}

	// ---- Dest is integral (non-bool) ----
	if constexpr (is_integral_compat_v<D_raw> && !std::is_same_v<D_raw, bool>) {
		using D_fixed = FixedLike_t<D_raw>;

		if constexpr (is_integral_compat_v<S_raw> || std::is_enum_v<S_raw>) {
			using S_fixed = typename normalize_src<S_raw>::type;
			if constexpr (std::is_same_v<D_raw, S_raw>) {
				return v;
			}
			else {
				D_fixed out{};
				if (!msl::utilities::SafeCast(static_cast<S_fixed>(v), out)) {
					throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
				}
				return static_cast<D_raw>(out);
			}
		}
		else if constexpr (std::is_floating_point_v<S_raw>) {
			using D0 = FixedLike_t<D_raw>;
			D0 tmp{};
#if defined(_MSC_VER)
			if (try_fp_to_int_fast_x86<D0>(static_cast<S_raw>(v), tmp))
			{
				return static_cast<D_raw>(tmp);
			}
#endif
#if (defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM))
			if constexpr (std::is_same_v<S_raw, float>) {
				if constexpr (sizeof(D0) == 4) {
					if (try_fp_to_int_fast_arm(static_cast<float>(v),
						*reinterpret_cast<std::conditional_t<std::is_signed_v<D0>, int32*, uint32*>>(&tmp)))
					{
						return static_cast<D_raw>(tmp);
					}
				}
			}
#if defined(__aarch64__) || defined(_M_ARM64)
			if constexpr (std::is_same_v<S_raw, double>) {
				if (try_fp_to_int_fast_arm(static_cast<double>(v), tmp))
				{
					return static_cast<D_raw>(tmp);
				}
			}
#endif
#endif
			ensure_fp_to_int_in_range<D_fixed>(v);
			return static_cast<D_raw>(static_cast<D_fixed>(v));
		}
	}

	// ---- Dest is floating (float/double/long double) ----
	if constexpr (std::is_floating_point_v<D_raw>) {
		if constexpr (std::is_floating_point_v<S_raw>) {
			ensure_fp_to_fp_in_range<D_raw>(v);
			return static_cast<D_raw>(v);
		}
		else if constexpr (is_integral_compat_v<S_raw> || std::is_enum_v<S_raw>) {
			using S_fixed = typename normalize_src<S_raw>::type;  // no eager instantiation
			const long double xv = static_cast<long double>(static_cast<S_fixed>(v));
			constexpr long double lo = static_cast<long double>(std::numeric_limits<D_raw>::lowest());
			constexpr long double hi = static_cast<long double>(std::numeric_limits<D_raw>::max());
			if (xv < lo || xv > hi) {
				throw msl::utilities::SafeIntException(msl::utilities::SafeIntError::SafeIntArithmeticOverflow);
			}
			ensure_int_to_fp_exact<D_raw>(static_cast<S_fixed>(v));
			return static_cast<D_raw>(static_cast<S_fixed>(v));
		}
	}

	static_assert(std::is_arithmetic_v<D_raw>, "numeric_cast<Dest>: Dest must be arithmetic.");
	return D_raw{}; // unreachable
}

#endif // __SYS_NUMERIC_CAST_H__