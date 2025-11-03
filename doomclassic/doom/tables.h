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

#ifndef __TABLES__
#define __TABLES__

#pragma once

#include <array>
#include <cstdint>
#include <numbers>
#include <type_traits>
#include <limits>
#include <cmath>
#include <concepts>

#ifdef LINUX
#include <math.h>
#else
//constexpr float PI = 				3.141592657f;
static constexpr long double PI = std::numbers::pi_v<long double>;
static constexpr long double TWO_PI = 2.0L * PI;
static constexpr long double HALF_PI = 0.5L * PI;
static constexpr long double QUARTER_PI = 0.25L * PI;
#endif

#include "m_fixed.h"

#pragma once
// angles.hpp — constexpr LUT trig for generic “BAM-like” angles
// C++20, MSVC/Clang/GCC. Array-style [] access for forward & inverse trig.
// To use your fixed_t (turns), define ANGLES_ADAPT_FIXED_T=1 before including.

#include <array>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <cmath>
#include <concepts>

namespace angles {

	// -------------------- concepts/traits --------------------
	template<class T>
	concept UnsignedInt = std::is_integral_v<T> && std::is_unsigned_v<T>;
	template<class T>
	concept EnumType = std::is_enum_v<T>;
	template<class T>
	concept HasToUnit = requires(T t) { { t.to_unit() } -> std::convertible_to<long double>; };

	template<class T>
	constexpr int bit_width_v = static_cast<int>(sizeof(T) * 8);

	// -------------------- angle_adapter (core hook) --------------------
	// Map any “angle” to unit fraction u ∈ [0,1) where 1.0 == full circle.
	// Default: unsigned integrals, enums (via unsigned underlying), or types with .to_unit().
	template<class Angle, class Enable = void>
	struct angle_adapter;

	// Unsigned integral: u = a / 2^bits
	template<UnsignedInt Angle>
	struct angle_adapter<Angle, void> {
		static constexpr long double to_unit(Angle a) noexcept {
			return static_cast<long double>(a) / std::ldexpl(1.0L, bit_width_v<Angle>);
		}
	};

	// Enums: recurse on unsigned underlying
	template<EnumType Angle>
	struct angle_adapter<Angle, void> {
		using U = std::underlying_type_t<Angle>;
		using UU = std::make_unsigned_t<U>;
		static constexpr long double to_unit(Angle a) noexcept {
			return angle_adapter<UU>::to_unit(static_cast<UU>(static_cast<U>(a)));
		}
	};

	// Has .to_unit()
	template<class Angle>
	struct angle_adapter<Angle, std::enable_if_t<HasToUnit<Angle>, void>> {
		static constexpr long double to_unit(const Angle& a) noexcept {
			return static_cast<long double>(a.to_unit());
		}
	};

	// -------------------- fixed_t adapter (turns) --------------------
#if defined(ANGLES_ADAPT_FIXED_T) && ANGLES_ADAPT_FIXED_T
	class fixed_t; // fwd decl; must be defined before translation unit ends
	template<> struct angle_adapter<fixed_t, void> {
		static constexpr long double to_unit(const fixed_t& a) noexcept;
	};
#endif

	// -------------------- BAM <-> radians helpers --------------------
	template<UnsignedInt AngleT>
	constexpr AngleT rad_to_bam(long double rad) noexcept {
		long double t = std::fmod(rad, TWO_PI);
		if (t < 0) t += TWO_PI;
		const long double scale = std::ldexpl(1.0L, bit_width_v<AngleT>) / TWO_PI;
		const long double u = t * scale;
		if constexpr (sizeof(AngleT) >= 8)
			return static_cast<AngleT>(static_cast<unsigned long long>(u));
		else
			return static_cast<AngleT>(static_cast<std::uint64_t>(u));
	}

	template<class AngleT>
	constexpr long double bam_to_rad(AngleT a) noexcept {
		if constexpr (UnsignedInt<AngleT>) {
			const long double inv = TWO_PI / std::ldexpl(1.0L, bit_width_v<AngleT>);
			return static_cast<long double>(a) * inv;
		}
		else if constexpr (EnumType<AngleT>) {
			using U = std::make_unsigned_t<std::underlying_type_t<AngleT>>;
			return bam_to_rad(static_cast<U>(static_cast<std::underlying_type_t<AngleT>>(a)));
		}
		else {
			// arbitrary type: interpret as unit fraction
			return angle_adapter<AngleT>::to_unit(a) * TWO_PI;
		}
	}

	// -------------------- reduction & seed polynomials (constexpr) --------------------
	struct Reduced { long double x; int q; }; // x ∈ [-π/2, π/2], q∈{0,1,2,3}
	constexpr Reduced reduce_full(long double r) noexcept {
		long double t = std::fmod(r, TWO_PI);
		if (t > PI)  t -= TWO_PI;
		if (t <= -PI)  t += TWO_PI;
		int q = 0;
		if (t > HALF_PI) { t = PI - t;  q = 1; }
		else if (t < -HALF_PI) { t = -PI - t; q = 3; }
		else { q = (t >= 0) ? 0 : 2; }
		return { t,q };
	}
	constexpr long double sin_poly(long double x) noexcept {
		const long double x2 = x * x, x3 = x * x2, x5 = x3 * x2, x7 = x5 * x2, x9 = x7 * x2;
		return x - x3 / 6.0L + x5 / 120.0L - x7 / 5040.0L + x9 / 362880.0L;
	}
	constexpr long double cos_poly(long double x) noexcept {
		const long double x2 = x * x, x4 = x2 * x2, x6 = x4 * x2, x8 = x6 * x2, x10 = x8 * x2;
		return 1.0L - x2 / 2.0L + x4 / 24.0L - x6 / 720.0L + x8 / 40320.0L - x10 / 3628800.0L;
	}
	constexpr long double sin_full(long double r) noexcept {
		const auto re = reduce_full(r);
		const long double s = sin_poly(re.x);
		return (re.q < 2) ? s : -s;
	}
	constexpr long double cos_full(long double r) noexcept {
		const auto re = reduce_full(r);
		const long double c = cos_poly(re.x);
		return (re.q == 0 || re.q == 2) ? c : -c;
	}

	// -------------------- table builders (constexpr) --------------------
	template<std::size_t N>
	constexpr std::array<float, N> make_sin_table() {
		static_assert(N >= 2);
		std::array<float, N> tab{};
		for (std::size_t i = 0; i < N; ++i) {
			const long double rad = (TWO_PI * static_cast<long double>(i)) / static_cast<long double>(N);
			tab[i] = static_cast<float>(sin_full(rad));
		}
		return tab;
	}
	template<std::size_t N>
	constexpr std::array<float, N> make_cos_table() {
		static_assert(N >= 2);
		std::array<float, N> tab{};
		for (std::size_t i = 0; i < N; ++i) {
			const long double rad = (TWO_PI * static_cast<long double>(i)) / static_cast<long double>(N);
			tab[i] = static_cast<float>(cos_full(rad));
		}
		return tab;
	}
	template<std::size_t N>
	constexpr std::array<float, N> make_tan_table() {
		static_assert(N >= 2);
		std::array<float, N> tab{};
		for (std::size_t i = 0; i < N; ++i) {
			const long double rad = (TWO_PI * static_cast<long double>(i)) / static_cast<long double>(N);
			const long double c = cos_full(rad);
			const long double s = sin_full(rad);
			tab[i] = (c == 0.0L) ? (s >= 0.0L ? std::numeric_limits<float>::infinity()
				: -std::numeric_limits<float>::infinity())
				: static_cast<float>(s / c);
		}
		return tab;
	}

	// -------------------- u∈[0,1) → index & frac --------------------
	template<std::size_t N>
	inline void unit_to_index(long double u, std::size_t& i, float& t) noexcept {
		u = std::fmod(u, 1.0L); if (u < 0) u += 1.0L;
		const long double f = u * static_cast<long double>(N);
		const long double fi = std::floor(f);
		i = static_cast<std::size_t>(fi);
		long double frac = f - fi; if (frac < 0.0L) frac = 0.0L;
		t = static_cast<float>(frac);
	}

	// -------------------- forward LUTs with [] access --------------------
	template<std::size_t N>
	struct Sin {
		static constexpr auto table = make_sin_table<N>();
		template<class Angle>
		inline float operator[](const Angle& a) const noexcept {
			std::size_t i; float t;
			unit_to_index<N>(angle_adapter<Angle>::to_unit(a), i, t);
			const float a0 = table[i], a1 = table[(i + 1) % N];
			return a0 + (a1 - a0) * t;
		}
	};
	template<std::size_t N>
	struct Cos {
		static constexpr auto table = make_cos_table<N>();
		template<class Angle>
		inline float operator[](const Angle& a) const noexcept {
			std::size_t i; float t;
			unit_to_index<N>(angle_adapter<Angle>::to_unit(a), i, t);
			const float a0 = table[i], a1 = table[(i + 1) % N];
			return a0 + (a1 - a0) * t;
		}
	};
	template<std::size_t N>
	struct Tan {
		static constexpr auto table = make_tan_table<N>();
		static constexpr float CLAMP = 1.0e6f;
		template<class Angle>
		inline float operator[](const Angle& a) const noexcept {
			std::size_t i; float t;
			unit_to_index<N>(angle_adapter<Angle>::to_unit(a), i, t);
			float v0 = table[i], v1 = table[(i + 1) % N];
			if ((v0 > 0 && v1 < 0) || (v0 < 0 && v1>0)) {
				if (std::isinf(v0) || std::isinf(v1) ||
					(std::fabs(v0) > 1e4f && std::fabs(v1) > 1e4f)) {
					return (t < 0.5f) ? v0 : v1;
				}
			}
			float v = v0 + (v1 - v0) * t;
			if (v > CLAMP) v = CLAMP;
			if (v < -CLAMP) v = -CLAMP;
			return v;
		}
		// Safe tan via sin/cos tables of your chosen sizes:
		template<std::size_t NS, std::size_t NC, class Angle>
		static inline float safe(const Angle& a) noexcept {
			const float s = Sin<NS>{} [a] ;
			const float c = Cos<NC>{} [a] ;
			if (std::fabs(c) < 1e-6f)
				return (s >= 0.0f) ? std::numeric_limits<float>::infinity()
				: -std::numeric_limits<float>::infinity();
			return s / c;
		}
	};

	// -------------------- inverse Q1 tables & helpers --------------------
	template<std::size_t N>
	constexpr std::array<float, N> make_sin_q1_table() {
		static_assert(N >= 2);
		std::array<float, N> tab{};
		const long double step = HALF_PI / static_cast<long double>(N - 1);
		for (std::size_t i = 0; i < N; ++i) {
			const long double th = static_cast<long double>(i) * step; // 0..π/2
			tab[i] = static_cast<float>(sin_full(th));
		}
		return tab;
	}
	template<std::size_t N>
	constexpr std::array<float, N> make_tan_q1_table() {
		static_assert(N >= 2);
		std::array<float, N> tab{};
		const long double step = QUARTER_PI / static_cast<long double>(N - 1);
		for (std::size_t i = 0; i < N; ++i) {
			const long double th = static_cast<long double>(i) * step; // 0..π/4
			const long double s = sin_full(th), c = cos_full(th);
			tab[i] = static_cast<float>(s / c);
		}
		return tab;
	}
	template<std::size_t N>
	inline float lower_lerp_index(const std::array<float, N>& arr, float y) noexcept {
		float lo = arr.front(), hi = arr.back();
		if (y <= lo) return 0.0f;
		if (y >= hi) return static_cast<float>(N - 1);
		std::size_t L = 0, H = N - 1;
		while (H - L > 1) {
			std::size_t M = (L + H) >> 1;
			(arr[M] <= y) ? L = M : H = M;
		}
		const float a = arr[L], b = arr[L + 1];
		const float t = (y - a) / (b - a);
		return static_cast<float>(L) + t;
	}

	// -------------------- array-style inverse: ASIN[x], ACOS[x], ATAN[t], ATAN(y,x) --------------------
	template<std::size_t NSinQ1, UnsignedInt AngleT = std::uint32_t>
	struct ASinArray {
		static constexpr auto TAB = make_sin_q1_table<NSinQ1>();
		static constexpr long double STEP = HALF_PI / static_cast<long double>(NSinQ1 - 1);
		inline AngleT operator[](float x) const noexcept {
			if (std::isnan(x)) x = 0.0f;
			if (x > 1.0f) x = 1.0f; if (x < -1.0f) x = -1.0f;
			const bool neg = (x < 0.0f);
			const float y = std::fabs(x);
			const float idx = lower_lerp_index(TAB, y);
			const long double th = static_cast<long double>(idx) * STEP; // [0,π/2]
			const long double rad = neg ? -th : th; // [-π/2, π/2]
			return rad_to_bam<AngleT>(rad);
		}
	};
	template<std::size_t NSinQ1, UnsignedInt AngleT = std::uint32_t>
	struct ACosArray {
		static constexpr auto TAB = make_sin_q1_table<NSinQ1>();
		static constexpr long double STEP = HALF_PI / static_cast<long double>(NSinQ1 - 1);
		inline AngleT operator[](float x) const noexcept {
			if (std::isnan(x)) x = 0.0f;
			if (x > 1.0f) x = 1.0f; if (x < -1.0f) x = -1.0f;
			const bool neg = (x < 0.0f);
			const float y = std::fabs(x);
			const float idx = lower_lerp_index(TAB, y);
			const long double th_asin = static_cast<long double>(idx) * STEP; // [0,π/2]
			const long double asin_r = neg ? -th_asin : th_asin;             // [-π/2,π/2]
			long double acos_r = HALF_PI - asin_r;                           // [0,π]
			if (acos_r < 0)  acos_r = 0;
			if (acos_r > PI) acos_r = PI;
			return rad_to_bam<AngleT>(acos_r);
		}
	};
	template<std::size_t NTanQ1, UnsignedInt AngleT = std::uint32_t>
	struct ATanArray {
		static constexpr auto TAB = make_tan_q1_table<NTanQ1>();
		static constexpr long double STEP = QUARTER_PI / static_cast<long double>(NTanQ1 - 1);
		inline AngleT operator[](float t) const noexcept {
			if (std::isnan(t)) t = 0.0f;
			if (std::isinf(t)) return rad_to_bam<AngleT>((t > 0) ? HALF_PI : -HALF_PI);
			const bool neg = (t < 0.0f); float a = std::fabs(t);
			long double theta;
			if (a <= TAB.back()) {
				const float idx = lower_lerp_index(TAB, a);
				theta = static_cast<long double>(idx) * STEP;
			}
			else {
				const float inv = 1.0f / a;
				const float idx = lower_lerp_index(TAB, inv);
				const long double phi = static_cast<long double>(idx) * STEP;
				theta = HALF_PI - phi;
			}
			if (neg) theta = -theta;
			return rad_to_bam<AngleT>(theta);
		}
		inline AngleT operator()(float y, float x) const noexcept {
			if (std::isnan(y) || std::isnan(x)) return AngleT{ 0 };
			if (x == 0.0f) {
				if (y > 0.0f) return rad_to_bam<AngleT>(HALF_PI);
				if (y < 0.0f) return rad_to_bam<AngleT>(-HALF_PI);
				return AngleT{ 0 };
			}
			if (y == 0.0f) {
				return (x < 0.0f) ? rad_to_bam<AngleT>(PI) : AngleT{ 0 };
			}
			const float t = y / x;
			long double ang = bam_to_rad((*this)[t]); // principal
			if (x < 0.0f) ang += (y >= 0.0f) ? PI : -PI;
			if (ang <= -PI) ang += TWO_PI;
			if (ang > PI) ang -= TWO_PI;
			return rad_to_bam<AngleT>(ang);
		}
	};

	// -------------------- optional fixed_t helpers (turns) --------------------
#if defined(ANGLES_ADAPT_FIXED_T) && ANGLES_ADAPT_FIXED_T
#include <cstddef>
	class fixed_t {
	public:
		using storage_type =
#if defined(_M_X64) || defined(__x86_64__) || defined(_M_ARM64) || defined(__aarch64__)
			std::int64_t;
#else
			std::int32_t;
#endif
		static constexpr int F =
#if defined(_M_X64) || defined(__x86_64__) || defined(_M_ARM64) || defined(__aarch64__)
			32;
#else
			16;
#endif
		static constexpr storage_type ONE = static_cast<storage_type>(1) << F;
		// ---- NOTE ----
		// This embedded stub is ONLY to make the adapter compile if the real fixed_t
		// isn’t visible yet. If you already have fixed_t defined, remove this block.
		fixed_t(storage_type r, struct raw_tag {}) : v_(r) {}
		explicit fixed_t(double d) : v_(static_cast<storage_type>(d* static_cast<long double>(ONE))) {}
		fixed_t() = default;
		storage_type raw_value() const noexcept { return v_; }
	private: storage_type v_{};
	};
	inline constexpr long double fixed_raw_to_unit(long double raw_over_ONE) noexcept {
		long double u = std::fmod(raw_over_ONE, 1.0L); if (u < 0) u += 1.0L; return u;
	}
	template<> inline constexpr long double angle_adapter<fixed_t, void>::to_unit(const fixed_t& a) noexcept {
		return fixed_raw_to_unit(static_cast<long double>(a.raw_value()) /
			static_cast<long double>(fixed_t::ONE));
	}
	inline constexpr fixed_t fixed_from_bam32(std::uint32_t bam32) noexcept {
		const long double u = static_cast<long double>(bam32) / 4294967296.0L;
		return fixed_t(static_cast<fixed_t::storage_type>(u * static_cast<long double>(fixed_t::ONE)),
			struct raw_tag {});
	}
#endif // ANGLES_ADAPT_FIXED_T

	// -------------------- default singletons (tune sizes here) --------------------
	// Forward
	inline constexpr Sin<10240>  SIN{};   // 10,240-entry sin
	inline constexpr Cos<8192>   COS{};   // 8,192-entry cos
	inline constexpr Tan<4096>   TAN{};   // 4,096-entry tan

	// Inverse (return BAM32 by default)
	inline constexpr ASinArray<10240, std::uint32_t> ASIN{}; // ASIN[x]
	inline constexpr ACosArray<10240, std::uint32_t> ACOS{}; // ACOS[x]
	inline constexpr ATanArray<4096, std::uint32_t> ATAN{}; // ATAN[t], ATAN(y,x)

} // namespace angles

constexpr size_t FINEANGLES = 8192;
constexpr auto   FINEMASK = (FINEANGLES - 1);

constexpr size_t SINE_TABLE_SIZE = 5 * FINEANGLES / 4; // 10240
constexpr size_t COSINE_TABLE_SIZE = SINE_TABLE_SIZE; // 10240
constexpr size_t TANGENT_TABLE_SIZE = FINEANGLES / 2; // 4096

// 0x100000000 to 0x2000
constexpr size_t ANGLETOFINESHIFT = 19;

/*
// Effective size is 10240.
const extern fixed_t	finesine[5*FINEANGLES/4];

// Re-use data, is just PI/2 phase shift.
const extern fixed_t*	finecosine;


// Effective size is 4096.
const extern fixed_t	finetangent[FINEANGLES/2];
*/

static inline constexpr angles::Sin<SINE_TABLE_SIZE>      SIN {};
static inline constexpr angles::Cos<COSINE_TABLE_SIZE>    COS {};
static inline constexpr angles::Tan<TANGENT_TABLE_SIZE>   TAN {};
#define finesine    SIN
#define finecosine  COS
#define finetangent TAN

// Binary Angle Measurement, BAM.
typedef enum BAM_e : uint32 {
	// --- Degrees ---
	ANG0       = 0x00000000u,  //   0°
	ANG0_1     = 0x00123D70u,  //  0.1°
	ANG0_25    = 0x002D82D8u,  //  0.25°
	ANG0_5     = 0x005B05B0u,  //  0.5°
	ANG1       = 0x02E8BA2Eu,  //   1°
	ANG2_5     = 0x0771C71Cu,  //   2.5°
	ANG5       = 0x038E38E3u,  //   5°
	ANG10      = 0x071C71C7u,  //  10°
	ANG15      = 0x0CCCCCCCu,  //  15°
	ANG30      = 0x19999999u,  //  30°
	ANG45      = 0x20000000u,  //  45°
	ANG60      = 0x26666666u,  //  60°
	ANG90      = 0x40000000u,  //  90°
	ANG120     = 0x55555555u,  // 120°
	ANG135     = 0x60000000u,  // 135°
	ANG150     = 0x66666666u,  // 150°
	ANG180     = 0x80000000u,  // 180°
	ANG210     = 0x99999999u,  // 210°
	ANG225     = 0xA0000000u,  // 225°
	ANG240     = 0xAAAAAAAau,  // 240°
	ANG270     = 0xC0000000u,  // 270°
	ANG300     = 0xD5555555u,  // 300°
	ANG315     = 0xE0000000u,  // 315°
	ANG330     = 0xE6666666u,  // 330°
	ANG345     = 0xF3333333u,  // 345°
	ANG360     = 0x00000000u,  // 360° (wraps to 0)
	FULL       = 0xFFFFFFFFu,  // conceptual max (one full turn)

	// --- Radians ---
	RAD0       = 0x00000000u,  //   0 rad
	RAD_PI_8   = 0x0C90FDBBu,  // π/8  ≈ 0.3927 rad
	RAD_PI_6   = 0x10C15238u,  // π/6  ≈ 0.5236 rad
	RAD_PI_4   = 0x20000000u,  // π/4  ≈ 0.7854 rad
	RAD_PI_3   = 0x2AAAAAAAu,  // π/3  ≈ 1.0472 rad
	RAD_PI_2   = 0x40000000u,  // π/2  ≈ 1.5708 rad
	RAD_2PI_3  = 0x55555555u,  // 2π/3 ≈ 2.0944 rad
	RAD_3PI_4  = 0x60000000u,  // 3π/4 ≈ 2.3562 rad
	RAD_5PI_6  = 0x6E147AE1u,  // 5π/6 ≈ 2.6180 rad
	RAD_PI     = 0x80000000u,  // π    ≈ 3.1416 rad
	RAD_7PI_6  = 0x931EDC83u,  // 7π/6 ≈ 3.6652 rad
	RAD_5PI_4  = 0xA0000000u,  // 5π/4 ≈ 3.9270 rad
	RAD_4PI_3  = 0xAAAAAAAau,  // 4π/3 ≈ 4.1888 rad
	RAD_3PI_2  = 0xC0000000u,  // 3π/2 ≈ 4.7124 rad
	RAD_5PI_3  = 0xD5555555u,  // 5π/3 ≈ 5.2360 rad
	RAD_7PI_4  = 0xE0000000u,  // 7π/4 ≈ 5.4978 rad
	RAD_11PI_6 = 0xEC3851ECu,  // 11π/6 ≈ 5.7596 rad
	RAD_2PI    = 0x00000000u   // 2π   (wraps to 0)
} BAM_t;



constexpr size_t SLOPERANGE = 2048;
constexpr size_t SLOPEBITS = 11;
//constexpr auto   DBITS = (FRACBITS - SLOPEBITS);

typedef uint32 angle_t;


// Effective size is 2049;
// The +1 size is to handle the case when x==y
//  without additional checking.
//const extern angle_t		tantoangle[SLOPERANGE+1];
static inline constexpr angles::ASinArray<SINE_TABLE_SIZE,   angle_t>  ASIN {};   // ASIN[x]
static inline constexpr angles::ACosArray<COSINE_TABLE_SIZE, angle_t>  ACOS {};   // ACOS[x]
static inline constexpr angles::ATanArray<SLOPERANGE + 1,    angle_t>  ATAN {};   // ATAN[t], ATAN(y,x)
#define tantoangle ATAN

// Utility function,
//  called by R_PointToAngle.
template<class A, class B>
	requires ((IsFixed<A> || IsFixed<B>) &&
				(IsFixed<A> || IsArithmeticOrEnumButNotFixed<A>) &&
				(IsFixed<B> || IsArithmeticOrEnumButNotFixed<B>))
static inline size_t SlopeDiv(const A num, const B den);

static inline size_t SlopeDiv( const Numeric auto num, const Numeric auto den);


#endif

