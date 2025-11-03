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

#ifndef __M_FIXED__
#define __M_FIXED__

#pragma once

#ifdef __GNUG__
#pragma interface
#endif

#include <cstdint>
#include <limits>
#include <cmath>
#include <type_traits>
#include <concepts>
#include <format>
#include <functional>
#include <ostream>
#include <istream>

#if defined(ID_CPU_ARCH_X64) || defined(ID_CPU_ARCH_ARM64)
//
// Fixed point, 64bit as 32.32.
//
typedef int64 fixed_storage_t;    // Q32.32
#define FIXED_T_IS_64BIT 1
#elif defined(ID_CPU_ARCH_X86) || defined(ID_CPU_ARCH_ARM32)
//
// Fixed point, 32bit as 16.16.
//
typedef int32 fixed_storage_t;  // Q16.16
#define FIXED_T_IS_64BIT 0
#endif

// Clamp out-of-range integer conversions instead of UB/impl-defined
#ifndef FIXED_T_CLAMP_INTEGRAL_CONVERSIONS
#define FIXED_T_CLAMP_INTEGRAL_CONVERSIONS
#endif

class fixed_t {
public:
	using storage_type = fixed_storage_t;

	// Scale & limits
	static constexpr bool    FIXED_Q_32_32 = (sizeof(storage_type) == 8);
	static constexpr bool    FIXED_Q_16_16 = (sizeof(storage_type) == 4);
	static constexpr size_t  FRACTIONAL_BITS = FIXED_Q_32_32 ? 32 : 16;             // fractional bits
	static constexpr size_t  FRACTIONAL_UNIT = 1 << FRACTIONAL_BITS;                       // fractional unit
	static constexpr auto    ONE = static_cast<storage_type>(1) << FRACTIONAL_BITS;  // 1.0 in fixed
	static constexpr auto    HALF = static_cast<storage_type>(ONE >> 1);
	static constexpr auto    ZERO = static_cast<storage_type>(0);
	static constexpr auto    MIN = std::numeric_limits<storage_type>::min();
	static constexpr auto    MAX = std::numeric_limits<storage_type>::max();

	// Single-field storage for POD friendliness
	storage_type v{};

	// -------------------------------------------------------------------------
	// Special tag to construct from raw bits (no scaling)
	// -------------------------------------------------------------------------
	struct raw_tag { explicit raw_tag() = default; };
	static constexpr raw_tag raw{};

	// -------------------------------------------------------------------------
	// Big Three/Five — make sure templated operator= never shadows these
	// -------------------------------------------------------------------------
	constexpr fixed_t() noexcept = default;
	fixed_t(const fixed_t&) = default;
	fixed_t(fixed_t&&) noexcept = default;
	fixed_t& operator=(const fixed_t&) = default;
	fixed_t& operator=(fixed_t&&) noexcept = default;

	// -------------------------------------------------------------------------
	// Constructors
	// -------------------------------------------------------------------------
	// From raw fixed bits (no scaling)
	constexpr fixed_t(storage_type raw_value, raw_tag) noexcept : v(raw_value) {}

	// Bool -> fixed (explicit; opt-out accidental bool conversions)
	explicit constexpr fixed_t(bool b) noexcept : v(b ? ONE : storage_type{ 0 }) {}

	// Integral (EXCEPT bool) -> fixed (scaled by 2^FRACTIONAL_BITS)
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	constexpr fixed_t(I i) noexcept
		: v(static_cast<storage_type>(static_cast<int64>(i) << FRACTIONAL_BITS)) {
	}

	// Floating -> fixed (round-to-nearest, ties away from zero)
	template <std::floating_point Fp>
	fixed_t(Fp d) noexcept {
		const long double scaled = static_cast<long double>(d) * static_cast<long double>(ONE);
		const long double bias = (scaled >= 0) ? 0.5L : -0.5L;
		const long double vld = std::floor(scaled + bias);
		if (vld > static_cast<long double>(MAX))
		{
			v = MAX;
		}
		else if (vld < static_cast<long double>(MIN))
		{
			v = MIN;
		}
		else
		{
			v = static_cast<storage_type>(vld);
		}
	}

	// -------------------------------------------------------------------------
	// Conversions
	// -------------------------------------------------------------------------
	[[nodiscard]] constexpr storage_type raw_value() const noexcept { return v; }

	[[nodiscard]] inline int64 to_int_trunc() const noexcept {
		return static_cast<int64>(static_cast<uint64>(v) >> FRACTIONAL_BITS);
	}
	[[nodiscard]] inline int64 to_int_round() const noexcept {
		const storage_type add = (v >= 0) ? (ONE >> 1) : -static_cast<storage_type>(ONE >> 1);
		return static_cast<int64>((v + add) >> FRACTIONAL_BITS);
	}
	[[nodiscard]] inline double to_double() const noexcept {
		return static_cast<double>(v) / static_cast<double>(ONE);
	}

	// Explicit scalar conversions (handy for assigning to scalars)
	//[[nodiscard]] explicit operator double()       const noexcept { return to_double(); }
	//[[nodiscard]] explicit operator int64() const noexcept { return to_int_trunc(); }

	// -------------------------------------------------------------------------
	// Bool interop (opt-out implicit conversions)
	// -------------------------------------------------------------------------
	[[nodiscard]] constexpr bool   is_zero() const noexcept { return v == 0; }
	//[[nodiscard]] constexpr explicit operator bool() const noexcept { return v != 0; }
	[[nodiscard]] constexpr bool   operator!() const noexcept { return v == 0; }

	// Assignment from arithmetic EXCEPT bool (prevents implicit bool conversions)
	template <class T>
		requires (std::is_arithmetic_v<std::remove_cv_t<T>> &&
	!std::same_as<std::remove_cv_t<T>, bool>)
		fixed_t& operator=(T rhs) noexcept {
		v = fixed_t(rhs).v;
		return *this;
	}
	// Explicit bool assignment (keep or remove as desired)
	fixed_t& operator=(bool rhs) noexcept {
		v = rhs ? ONE : storage_type{ 0 };
		return *this;
	}

	// -------------------------------------------------------------------------
// Implicit conversions from fixed_t to scalar types
//  - bool
//  - any signed/unsigned integral (with safe clamping)
//  - float, double, long double
// -------------------------------------------------------------------------

// BOOL (implicit)
	[[nodiscard]] explicit constexpr operator bool() const noexcept {
		return v != 0;
	}

	// FLOATING (implicit)
	[[nodiscard]] constexpr operator float() const noexcept {
		return static_cast<float>(to_double());
	}
	[[nodiscard]] constexpr operator double() const noexcept {
		return to_double();
	}
	[[nodiscard]] constexpr operator long double() const noexcept {
		return static_cast<long double>(to_double());
	}

	// SIGNED INTEGRALS (implicit, trunc toward zero, with optional clamping)
	template <std::signed_integral I>
	[[nodiscard]] constexpr operator I() const noexcept {
		// Truncate toward zero in world units
		long long t = static_cast<long long>(to_int_trunc());

#if defined(FIXED_T_CLAMP_INTEGRAL_CONVERSIONS)
		if (t < static_cast<long long>(std::numeric_limits<I>::min())) {
			return std::numeric_limits<I>::min();
		}
		if (t > static_cast<long long>(std::numeric_limits<I>::max())) {
			return std::numeric_limits<I>::max();
		}
#endif
		return static_cast<I>(t);
	}

	// UNSIGNED INTEGRALS (implicit, trunc toward zero, negatives clamp to 0)
	template <std::unsigned_integral U>
	[[nodiscard]] constexpr operator U() const noexcept {
		long long t = static_cast<long long>(to_int_trunc());

#if defined(FIXED_T_CLAMP_INTEGRAL_CONVERSIONS)
		if (t <= 0) return U{ 0 };
		using W = unsigned long long;
		if (static_cast<W>(t) > static_cast<W>(std::numeric_limits<U>::max())) {
			return std::numeric_limits<U>::max();
		}
#endif
		return static_cast<U>(t);
	}


	// -------------------------------------------------------------------------
	// Unary and ++/--
	// -------------------------------------------------------------------------
	[[nodiscard]] constexpr fixed_t operator+() const noexcept { return *this; }
	[[nodiscard]] constexpr fixed_t operator-() const noexcept {
		if (v == MIN)
		{
			return fixed_t(MAX, raw); // saturating abs(-MIN)
		}
		return fixed_t(static_cast<storage_type>(-v), raw);
	}

	// prefix ++ / --
	constexpr fixed_t& operator++() noexcept { v = static_cast<storage_type>(v + ONE); return *this; }
	constexpr fixed_t& operator--() noexcept { v = static_cast<storage_type>(v - ONE); return *this; }

	// postfix ++ / --
	constexpr fixed_t operator++(int) noexcept { fixed_t old{ v, raw }; v = static_cast<storage_type>(v + ONE); return old; }
	constexpr fixed_t operator--(int) noexcept { fixed_t old{ v, raw }; v = static_cast<storage_type>(v - ONE); return old; }

	// Optional: saturating ±1.0 steps
	constexpr fixed_t& inc_sat() noexcept {
		if (v > static_cast<storage_type>(MAX - ONE))
		{
			v = MAX;
		}
		else
		{
			v = static_cast<storage_type>(v + ONE);
		}
		return *this;
	}
	constexpr fixed_t& dec_sat() noexcept {
		if (v < static_cast<storage_type>(MIN + ONE))
		{
			v = MIN;
		}
		else
		{
			v = static_cast<storage_type>(v - ONE);
		}
		return *this;
	}

	// -------------------------------------------------------------------------
	// Add/Sub
	// -------------------------------------------------------------------------
	[[nodiscard]] friend constexpr fixed_t operator+(fixed_t a, fixed_t b) noexcept {
		return fixed_t(static_cast<storage_type>(a.v + b.v), raw);
	}
	[[nodiscard]] friend constexpr fixed_t operator-(fixed_t a, fixed_t b) noexcept {
		return fixed_t(static_cast<storage_type>(a.v - b.v), raw);
	}
	constexpr fixed_t& operator+=(fixed_t rhs) noexcept { v = static_cast<storage_type>(v + rhs.v); return *this; }
	constexpr fixed_t& operator-=(fixed_t rhs) noexcept { v = static_cast<storage_type>(v - rhs.v); return *this; }

	// Mixed add/sub with arithmetic EXCEPT bool
	template <class T>
		requires (std::is_arithmetic_v<std::remove_cv_t<T>> &&
	!std::same_as<std::remove_cv_t<T>, bool>)
		friend inline fixed_t operator+(fixed_t a, T b) noexcept { return a + fixed_t(b); }
	template <class T>
		requires (std::is_arithmetic_v<std::remove_cv_t<T>> &&
	!std::same_as<std::remove_cv_t<T>, bool>)
		friend inline fixed_t operator-(fixed_t a, T b) noexcept { return a - fixed_t(b); }
	template <class T>
		requires (std::is_arithmetic_v<std::remove_cv_t<T>> &&
	!std::same_as<std::remove_cv_t<T>, bool>)
		friend inline fixed_t operator+(T a, fixed_t b) noexcept { return fixed_t(a) + b; }
	template <class T>
		requires (std::is_arithmetic_v<std::remove_cv_t<T>> &&
	!std::same_as<std::remove_cv_t<T>, bool>)
		friend inline fixed_t operator-(T a, fixed_t b) noexcept { return fixed_t(a) - b; }

	template <class T>
		requires (std::is_arithmetic_v<std::remove_cv_t<T>> &&
	!std::same_as<std::remove_cv_t<T>, bool>)
		inline fixed_t& operator+=(T rhs) noexcept { *this = *this + rhs; return *this; }
	template <class T>
		requires (std::is_arithmetic_v<std::remove_cv_t<T>> &&
	!std::same_as<std::remove_cv_t<T>, bool>)
		inline fixed_t& operator-=(T rhs) noexcept { *this = *this - rhs; return *this; }

	// -------------------------------------------------------------------------
	// Shifts (value-domain ×/÷ 2^k). Exclude bool as shift count.
	// -------------------------------------------------------------------------
	[[nodiscard]] friend constexpr fixed_t operator<<(fixed_t a, unsigned k) noexcept {
		using U = std::make_unsigned_t<storage_type>;
		return fixed_t(static_cast<storage_type>(static_cast<U>(a.v) << k), raw);
	}
	[[nodiscard]] friend constexpr fixed_t operator>>(fixed_t a, unsigned k) noexcept {
		return fixed_t(static_cast<storage_type>(a.v >> k), raw);
	}
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	[[nodiscard]] friend constexpr fixed_t operator<<(fixed_t a, I k) noexcept {
		return a << static_cast<unsigned>(k);
	}
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	[[nodiscard]] friend constexpr fixed_t operator>>(fixed_t a, I k) noexcept {
		return a >> static_cast<unsigned>(k);
	}
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	fixed_t& operator<<=(I k) noexcept {
		using U = std::make_unsigned_t<storage_type>;
		v = static_cast<storage_type>(static_cast<U>(v) << static_cast<unsigned>(k));
		return *this;
	}
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	fixed_t& operator>>=(I k) noexcept {
		v = static_cast<storage_type>(v >> static_cast<unsigned>(k));
		return *this;
	}

	// -------------------------------------------------------------------------
	// Bitwise ops (raw-bit semantics) + integer-mask variants (bool excluded)
	// -------------------------------------------------------------------------
	// ~
	[[nodiscard]] friend constexpr fixed_t operator~(fixed_t a) noexcept {
		return fixed_t(static_cast<storage_type>(~a.v), raw);
	}
	// ^ & |
	[[nodiscard]] friend constexpr fixed_t operator^(fixed_t a, fixed_t b) noexcept {
		return fixed_t(static_cast<storage_type>(a.v ^ b.v), raw);
	}
	[[nodiscard]] friend constexpr fixed_t operator&(fixed_t a, fixed_t b) noexcept {
		return fixed_t(static_cast<storage_type>(a.v & b.v), raw);
	}
	[[nodiscard]] friend constexpr fixed_t operator|(fixed_t a, fixed_t b) noexcept {
		return fixed_t(static_cast<storage_type>(a.v | b.v), raw);
	}
	constexpr fixed_t& operator^=(fixed_t rhs) noexcept { v = static_cast<storage_type>(v ^ rhs.v); return *this; }
	constexpr fixed_t& operator&=(fixed_t rhs) noexcept { v = static_cast<storage_type>(v & rhs.v); return *this; }
	constexpr fixed_t& operator|=(fixed_t rhs) noexcept { v = static_cast<storage_type>(v | rhs.v); return *this; }

	// masks with integrals (EXCEPT bool)
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	[[nodiscard]] friend constexpr fixed_t operator^(fixed_t a, I m) noexcept {
		using S = storage_type;
		return fixed_t(static_cast<S>(a.v ^ static_cast<S>(m)), raw);
	}
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	[[nodiscard]] friend constexpr fixed_t operator&(fixed_t a, I m) noexcept {
		using S = storage_type;
		return fixed_t(static_cast<S>(a.v & static_cast<S>(m)), raw);
	}
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	[[nodiscard]] friend constexpr fixed_t operator|(fixed_t a, I m) noexcept {
		using S = storage_type;
		return fixed_t(static_cast<S>(a.v | static_cast<S>(m)), raw);
	}
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	constexpr fixed_t& operator^=(I m) noexcept {
		v = static_cast<storage_type>(v ^ static_cast<storage_type>(m)); return *this;
	}
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	constexpr fixed_t& operator&=(I m) noexcept {
		v = static_cast<storage_type>(v & static_cast<storage_type>(m)); return *this;
	}
	template <std::integral I>
		requires (!std::same_as<std::remove_cv_t<I>, bool>)
	constexpr fixed_t& operator|=(I m) noexcept {
		v = static_cast<storage_type>(v | static_cast<storage_type>(m)); return *this;
	}

	// -------------------------------------------------------------------------
	// Comparisons
	// -------------------------------------------------------------------------
	[[nodiscard]] friend constexpr bool operator==(fixed_t a, fixed_t b) noexcept { return a.v == b.v; }
	[[nodiscard]] friend constexpr bool operator!=(fixed_t a, fixed_t b) noexcept { return a.v != b.v; }
	[[nodiscard]] friend constexpr bool operator< (fixed_t a, fixed_t b) noexcept { return a.v < b.v; }
	[[nodiscard]] friend constexpr bool operator> (fixed_t a, fixed_t b) noexcept { return a.v > b.v; }
	[[nodiscard]] friend constexpr bool operator<=(fixed_t a, fixed_t b) noexcept { return a.v <= b.v; }
	[[nodiscard]] friend constexpr bool operator>=(fixed_t a, fixed_t b) noexcept { return a.v >= b.v; }

	// Mixed comparisons
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator==(fixed_t a, T b) noexcept { return a == fixed_t(b); }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator!=(fixed_t a, T b) noexcept { return !(a == b); }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator< (fixed_t a, T b) noexcept { return a < fixed_t(b); }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator> (fixed_t a, T b) noexcept { return fixed_t(b) < a; }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator<=(fixed_t a, T b) noexcept { return !(a > b); }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator>=(fixed_t a, T b) noexcept { return !(a < b); }

	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator==(T a, fixed_t b) noexcept { return fixed_t(a) == b; }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator!=(T a, fixed_t b) noexcept { return !(a == b); }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator< (T a, fixed_t b) noexcept { return fixed_t(a) < b; }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator> (T a, fixed_t b) noexcept { return b < fixed_t(a); }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator<=(T a, fixed_t b) noexcept { return !(a > b); }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline bool operator>=(T a, fixed_t b) noexcept { return !(a < b); }

	// -------------------------------------------------------------------------
	// Multiply / Divide (round-to-nearest; ties away from zero)
	// Non-trivial implementations are in fixed_t.cpp
	// -------------------------------------------------------------------------
	[[nodiscard]] friend inline fixed_t operator*(fixed_t a, fixed_t b) noexcept {
		if constexpr (FIXED_T_IS_64BIT)
		{
			return mul64(a.v, b.v);
		}
		else
		{
			return mul32(a.v, b.v);
		}
	}
	[[nodiscard]] friend inline fixed_t operator/(fixed_t a, fixed_t b) noexcept {
		if (b.v == 0)
		{
			return fixed_t((a.v >= 0) ? MAX : MIN, raw);
		}
		if constexpr (FIXED_T_IS_64BIT)
		{
			return div64(a.v, b.v);
		}
		else
		{
			return div32(a.v, b.v);
		}
	}
	inline fixed_t& operator*=(fixed_t rhs) noexcept { *this = *this * rhs; return *this; }
	inline fixed_t& operator/=(fixed_t rhs) noexcept { *this = *this / rhs; return *this; }

	// Mixed mul/div with arithmetic
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		friend inline fixed_t operator*(fixed_t a, T b) noexcept { return a * fixed_t(b); }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		friend inline fixed_t operator/(fixed_t a, T b) noexcept {
		if constexpr (std::is_floating_point_v<std::remove_cv_t<T>>) {
			if (std::equal_to<>()(b, static_cast<T>(0)))
			{
				return fixed_t((a.v >= 0) ? MAX : MIN, raw);
			}
		}
		else {
			if (b == T{ 0 })
			{
				return fixed_t((a.v >= 0) ? MAX : MIN, raw);
			}
		}
		return a / fixed_t(b);
	}
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		friend inline fixed_t operator*(T a, fixed_t b) noexcept { return fixed_t(a) * b; }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		friend inline fixed_t operator/(T a, fixed_t b) noexcept { return fixed_t(a) / b; }

	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		inline fixed_t& operator*=(T rhs) noexcept { *this = *this * rhs; return *this; }
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		inline fixed_t& operator/=(T rhs) noexcept { *this = *this / rhs; return *this; }

	// -------------------------------------------------------------------------
	// Modulo (value-domain % remainder)
	// -------------------------------------------------------------------------
	[[nodiscard]] friend inline fixed_t operator%(fixed_t a, fixed_t b) noexcept {
		if (b.v == 0) return fixed_t(0, raw);  // define as 0 for /0; could also saturate
		return fixed_t(static_cast<storage_type>(a.v % b.v), raw);
	}
	inline fixed_t& operator%=(fixed_t rhs) noexcept {
		if (rhs.v == 0) { v = 0; return *this; }
		v = static_cast<storage_type>(v % rhs.v);
		return *this;
	}

	// Mixed arithmetic variants
	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline fixed_t operator%(fixed_t a, T b) noexcept {
		return a % fixed_t(b);
	}

	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		[[nodiscard]] friend inline fixed_t operator%(T a, fixed_t b) noexcept {
		return fixed_t(a) % b;
	}

	template <class T>
		requires std::is_arithmetic_v<std::remove_cv_t<T>>
		inline fixed_t& operator%=(T rhs) noexcept {
		*this = *this % rhs;
		return *this;
	}


	// -------------------------------------------------------------------------
	// Utility
	// -------------------------------------------------------------------------
	[[nodiscard]] static inline fixed_t abs(fixed_t x) noexcept {
		if (x.v >= 0)
		{
			return x;
		}
		return (x.v == MIN) ? fixed_t(MAX, raw)
			       : fixed_t(static_cast<storage_type>(-x.v), raw);
	}
	[[nodiscard]] static constexpr fixed_t clamp(fixed_t x, fixed_t lo, fixed_t hi) noexcept {
		return (x.v < lo.v) ? lo : (x.v > hi.v ? hi : x);
	}

private:
	// Non-trivial kernels (defined in fixed_t.cpp)
	static fixed_t mul32(storage_type a, storage_type b) noexcept;
	static fixed_t div32(storage_type a, storage_type b) noexcept;
	static fixed_t mad32(storage_type a, storage_type b, storage_type c) noexcept;

	static fixed_t mul64(storage_type a, storage_type b) noexcept;
	static fixed_t div64(storage_type a, storage_type b) noexcept;
	static fixed_t mad64(storage_type a, storage_type b, storage_type c) noexcept;
};

// ---- POD guarantees ---------------------------------------------------------
static_assert(std::is_trivially_copyable_v<fixed_t>, "fixed_t must be trivially copyable");
static_assert(std::is_standard_layout_v<fixed_t>, "fixed_t must be standard-layout");
static_assert(sizeof(fixed_t::storage_type) == sizeof(fixed_t),	"fixed_t must be exactly the size of its storage");

//#define FRACBITS fixed_t::FRACTIONAL_BITS
	static constexpr uint16  OLDFRACUNIT = 65536; // 1<<16
	static constexpr fixed_t FRACUNIT    = 1;     // Default map unit.

	// -----------------------------------------------------------------------------
	// Concepts
	// -----------------------------------------------------------------------------
	template<class T>
	using decay_cvref_t = std::remove_cvref_t<T>;

	template<class T>
	concept IsFixed = std::same_as<decay_cvref_t<T>, fixed_t>;

	template<class T>
	concept IsArithmeticOrEnumButNotFixed =
		(!IsFixed<T>) && (std::is_arithmetic_v<decay_cvref_t<T>> || std::is_enum_v<decay_cvref_t<T>>);

	// Convert acceptable types to fixed_t
	template<IsFixed T>
	[[nodiscard]] inline fixed_t fx_to_fixed(T&& x) noexcept {
		// preserve lvalue/rvalue-ness of fixed_t
		return std::forward<T>(x);
	}

	template<IsArithmeticOrEnumButNotFixed T>
	[[nodiscard]] inline fixed_t fx_to_fixed(T&& x) noexcept {
		using U = decay_cvref_t<T>;
		if constexpr (std::is_enum_v<U>) {
			using UT = std::underlying_type_t<U>;
			return fixed_t(static_cast<UT>(std::forward<T>(x)));
		}
		else {
			// arithmetic (incl. bool, integral, floating)
			return fixed_t(std::forward<T>(x));
		}
	}

	// -----------------------------------------------------------------------------
	// Max / Min  (enabled only if at least one param is fixed_t)
	// -----------------------------------------------------------------------------
	template<class A, class B>
		requires ((IsFixed<A> || IsFixed<B>) &&
	(IsFixed<A> || IsArithmeticOrEnumButNotFixed<A>) &&
		(IsFixed<B> || IsArithmeticOrEnumButNotFixed<B>))
		[[nodiscard]] inline fixed_t Max(A&& a, B&& b) noexcept {
		fixed_t fa = fx_to_fixed(std::forward<A>(a));
		fixed_t fb = fx_to_fixed(std::forward<B>(b));
		return (fa >= fb) ? fa : fb;
	}

	template<class A, class B>
		requires ((IsFixed<A> || IsFixed<B>) &&
	(IsFixed<A> || IsArithmeticOrEnumButNotFixed<A>) &&
		(IsFixed<B> || IsArithmeticOrEnumButNotFixed<B>))
		[[nodiscard]] inline fixed_t Min(A&& a, B&& b) noexcept {
		fixed_t fa = fx_to_fixed(std::forward<A>(a));
		fixed_t fb = fx_to_fixed(std::forward<B>(b));
		return (fa <= fb) ? fa : fb;
	}

	// -----------------------------------------------------------------------------
	// Clamp  (enabled only if at least one of the three is fixed_t)
	// -----------------------------------------------------------------------------
	template<class V, class L, class H>
		requires ((IsFixed<V> || IsFixed<L> || IsFixed<H>) &&
	(IsFixed<V> || IsArithmeticOrEnumButNotFixed<V>) &&
		(IsFixed<L> || IsArithmeticOrEnumButNotFixed<L>) &&
		(IsFixed<H> || IsArithmeticOrEnumButNotFixed<H>))
		[[nodiscard]] inline fixed_t Clamp(V&& v, L&& lo, H&& hi) noexcept {
		fixed_t fv = fx_to_fixed(std::forward<V>(v));
		fixed_t flo = fx_to_fixed(std::forward<L>(lo));
		fixed_t fhi = fx_to_fixed(std::forward<H>(hi));
		return fixed_t::clamp(fv, flo, fhi);
	}

	// -----------------------------------------------------------------------------
	// Compare (returns -1, 0, +1) — enabled only if at least one param is fixed_t
	// -----------------------------------------------------------------------------
	template<class A, class B>
		requires ((IsFixed<A> || IsFixed<B>) &&
	(IsFixed<A> || IsArithmeticOrEnumButNotFixed<A>) &&
		(IsFixed<B> || IsArithmeticOrEnumButNotFixed<B>))
		[[nodiscard]] inline int Compare(A&& a, B&& b) noexcept {
		fixed_t fa = fx_to_fixed(std::forward<A>(a));
		fixed_t fb = fx_to_fixed(std::forward<B>(b));
		if (fa < fb)
		{
			return -1;
		}
		if (fa > fb)
		{
			return +1;
		}
		return 0;
	}

	// -----------------------------------------------------------------------------
	// Floor / Ceil — unary; require the argument to be fixed_t
	// -----------------------------------------------------------------------------
	template<IsFixed T>
	[[nodiscard]] inline fixed_t Floor(T&& x) noexcept {
		const fixed_t fx = std::forward<T>(x);
		using S = fixed_t::storage_type;
		const S  ONE = fixed_t::ONE;
		const S  v = fx.v;

		const S q = static_cast<S>(v / ONE);
		const S r = static_cast<S>(v % ONE);
		// v >= 0: floor = q ; v < 0: if r==0 -> q else q-1
		const S qi = (v >= 0 || r == 0) ? q : (q - 1);
		return fixed_t(static_cast<S>(qi << fixed_t::FRACTIONAL_BITS), fixed_t::raw);
	}

	template<IsFixed T>
	[[nodiscard]] inline fixed_t Ceil(T&& x) noexcept {
		const fixed_t fx = std::forward<T>(x);
		using S = fixed_t::storage_type;
		const S  ONE = fixed_t::ONE;
		const S  v = fx.v;

		const S q = static_cast<S>(v / ONE);
		const S r = static_cast<S>(v % ONE);
		// v >= 0: if r==0 -> q else q+1 ; v < 0: ceil = q
		const S qi = (v >= 0) ? ((r == 0) ? q : (q + 1)) : q;
		return fixed_t(static_cast<S>(qi << fixed_t::FRACTIONAL_BITS), fixed_t::raw);
	}

	[[nodiscard]] inline fixed_t abs(fixed_t x) noexcept { return fixed_t::abs(x); }

	[[nodiscard]] inline fixed_t floor(fixed_t x) noexcept {
		using S = fixed_t::storage_type; S q = x.raw_value() / fixed_t::ONE; S r = x.raw_value() % fixed_t::ONE;
		if (x.raw_value() >= 0 || r == 0) return fixed_t(q << fixed_t::FRACTIONAL_BITS, fixed_t::raw);
		return fixed_t((q - 1) << fixed_t::FRACTIONAL_BITS, fixed_t::raw);
	}
	[[nodiscard]] inline fixed_t ceil(fixed_t x) noexcept {
		using S = fixed_t::storage_type; S q = x.raw_value() / fixed_t::ONE; S r = x.raw_value() % fixed_t::ONE;
		if (x.raw_value() < 0) return fixed_t(q << fixed_t::FRACTIONAL_BITS, fixed_t::raw);
		return fixed_t(((r == 0) ? q : (q + 1)) << fixed_t::FRACTIONAL_BITS, fixed_t::raw);
	}
	[[nodiscard]] inline fixed_t trunc(fixed_t x) noexcept {
		using S = fixed_t::storage_type; S q = x.raw_value() / fixed_t::ONE;
		return fixed_t(q << fixed_t::FRACTIONAL_BITS, fixed_t::raw);
	}
	[[nodiscard]] inline fixed_t round(fixed_t x) noexcept {
		auto r = x.raw_value();
		auto add = (r >= 0) ? (fixed_t::ONE >> 1) : -(fixed_t::ONE >> 1);
		return fixed_t(static_cast<fixed_t::storage_type>((r + add) & ~static_cast<fixed_t::storage_type>(fixed_t::ONE - 1)), fixed_t::raw);
	}


	// STL injection
// stream operators
	inline std::ostream& operator<<(std::ostream& os, const fixed_t& x) { return os << x.to_double(); }
	inline std::istream& operator>>(std::istream& is, fixed_t& x) { double d{}; is >> d; x = fixed_t(d); return is; }

namespace std
	{
	template<> class std::numeric_limits<fixed_t> {
		static constexpr bool is_specialized = true;
		static constexpr fixed_t min() noexcept { return fixed_t(fixed_t::MIN, fixed_t::raw); }
		static constexpr fixed_t max() noexcept { return fixed_t(fixed_t::MAX, fixed_t::raw); }
		static constexpr fixed_t lowest() noexcept { return min(); }
		static constexpr int  digits = (int)(sizeof(fixed_t::storage_type) * 8 - 1);
		static constexpr int  digits10 = 0; // not decimal; leave 0 or compute approx
		static constexpr int  radix = 2;
		static constexpr fixed_t epsilon() noexcept { return fixed_t(1, fixed_t::raw); } // 2^-F
		static constexpr fixed_t round_error() noexcept { return fixed_t(fixed_t::HALF, fixed_t::raw); }
		static constexpr int  min_exponent = 0, min_exponent10 = 0, max_exponent = 0, max_exponent10 = 0;
		static constexpr bool is_signed = true, is_integer = false, is_exact = true;
		static constexpr bool has_infinity = false, has_quiet_NaN = false, has_signaling_NaN = false, has_denorm = false;
		static constexpr bool traps = false, tinyness_before = false;
		static constexpr std::float_round_style round_style = std::round_toward_zero;
	};


	template<> struct std::hash<fixed_t> {
		size_t operator()(const fixed_t& x) const noexcept {
			using U = std::make_unsigned_t<fixed_t::storage_type>;
			return std::hash<U>{}(static_cast<U>(x.raw_value()));
		}
	};


	template<> struct std::formatter<fixed_t> : std::formatter<double> {
		template<class Ctx>
		auto format(const fixed_t& x, Ctx& ctx) const {
			return std::formatter<double>::format(x.to_double(), ctx);
		}
	};


	// with fixed + arithmetic -> fixed
	template<class T> struct std::common_type<fixed_t, T>
	{
		using type = std::conditional_t<std::is_arithmetic_v<T>, fixed_t, void>;
	};
	template<class T> struct std::common_type<T, fixed_t>
	{
		using type = std::conditional_t<std::is_arithmetic_v<T>, fixed_t, void>;
	};
	template<> struct std::common_type<fixed_t, fixed_t> { using type = fixed_t; };


	inline void swap(fixed_t& a, fixed_t& b) noexcept { auto t = a; a = b; b = t; }

	}
#endif

