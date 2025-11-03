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

#ifndef __SYS_HELPERS_H__
#define __SYS_HELPERS_H__

#pragma once
#include <utility>
#include <concepts>
#include <type_traits>
#include <memory>          // unique_ptr, shared_ptr
#include <wrl/client.h>  // Microsoft::WRL::ComPtr

#include "sys_numeric_cast.h"

template<class T> concept Numeric = std::is_arithmetic_v<T> || std::is_enum_v<T>;

template <class T>
constexpr bool is_numeric_v = std::is_arithmetic_v<T> || std::is_enum_v<T>;

// Implementation for Min
template<Numeric T, Numeric U>
[[nodiscard]] static inline constexpr auto Min(const T a, const U b) noexcept
-> std::common_type_t<T, U>
{
	using CommonType = std::common_type_t<T, U>;

	const auto _a = numeric_cast<CommonType>(a);
	const auto _b = numeric_cast<CommonType>(b);

	return ( _a < _b )
		? _a
		: _b;
}

// Implementation for Max
template<Numeric T, Numeric U>
[[nodiscard]] static inline constexpr auto Max(const T a, const U b) noexcept
-> std::common_type_t<T, U>
{
	using CommonType = std::common_type_t<T, U>;

	const auto _a = numeric_cast<CommonType>(a);
	const auto _b = numeric_cast<CommonType>(b);

	return (_a > _b)
		? _a
		: _b;
}


template<Numeric U, Numeric V, Numeric T >
[[nodiscard]] static inline constexpr auto Clamp(const U minVal, const V maxVal, const T value) noexcept {
	using CommonType = std::common_type_t<U, V, T>;
	const auto _minVal = numeric_cast<CommonType>(minVal);
	const auto _maxVal = numeric_cast<CommonType>(maxVal);

	assert(_minVal <= _maxVal && "Clamp() requires minVal <= maxVal");

	const CommonType v = numeric_cast<CommonType>(value);

	if (v < _minVal)
	{
		return _minVal;
	}
	if (v > _maxVal)
	{
		return _maxVal;
	}
	return v;
}

// Implementation for Floor()
template<Numeric T>
[[nodiscard]] static inline constexpr auto Floor(const T value) noexcept
-> std::common_type_t<T, double>
{
	using CommonType = std::common_type_t<T, double>;

	if constexpr (std::integral<T>)
	{
		return numeric_cast<CommonType>(value); // already integral
	}
	else
	{
		return numeric_cast<CommonType>(std::floor(numeric_cast<CommonType>(value)));
	}
}

// Implementation for Ceil()
template<Numeric T>
[[nodiscard]] static inline constexpr auto Ceil(const T value) noexcept
-> std::common_type_t<T, double>
{
	using CommonType = std::common_type_t<T, double>;

	if constexpr (std::integral<T>)
	{
		return numeric_cast<CommonType>(value); // already integral
	}
	else
	{
		return numeric_cast<CommonType>(std::ceil(numeric_cast<CommonType>(value)));
	}
}

template<Numeric T, Numeric U>
[[nodiscard]] constexpr int Compare(const T a, const U b) noexcept
{
	using CommonType = std::common_type_t<T, U>;
	const CommonType lhs = static_cast<CommonType>(a);
	const CommonType rhs = static_cast<CommonType>(b);

	// Using ternary ensures full constexpr compatibility
	return (lhs < rhs) ? -1 :
		   (lhs > rhs) ?  1 :
	                      0;
}

// ==============
// selective_cast
// ==============

// If T is numeric, use numeric_cast<T>(src)
template<class T, class Src>
	requires Numeric<T>
[[nodiscard]] constexpr T selective_cast(Src&& src)
noexcept(noexcept(numeric_cast<T>(std::forward<Src>(src))))
{
	// Bring possible user-defined numeric_cast into scope for ADL if needed
	return numeric_cast<T>(std::forward<Src>(src));
}

// Otherwise, use static_cast<T>(src)
template<class T, class Src>
	requires (!Numeric<T>)
[[nodiscard]] constexpr T selective_cast(Src&& src)
noexcept(noexcept(static_cast<T>(std::forward<Src>(src))))
{
	return static_cast<T>(std::forward<Src>(src));
}

// ==============
// BASE_TYPE
// ==============

namespace type_determination {

	// ---------------------------
	// cv-STRIPPED implementation
	// ---------------------------
	template<class T> struct base_element_impl {
		using type = std::remove_cv_t<std::remove_reference_t<T>>;
	};

	// arrays
	template<class T> struct base_element_impl<T[]> : base_element_impl<T> {};
	template<class T, std::size_t N>
	struct base_element_impl<T[N]> : base_element_impl<T> {};

	// pointers
	template<class T> struct base_element_impl<T*> : base_element_impl<std::remove_cv_t<T>> {};

	// pointers-to-arrays
	template<class T> struct base_element_impl<T(*)[]> : base_element_impl<T> {};
	template<class T, std::size_t N>
	struct base_element_impl<T(*)[N]> : base_element_impl<T> {};

	// entry point (normalize top-level cv/ref before the impl)
	template<class T>
	constexpr std::type_identity<typename base_element_impl<std::remove_cvref_t<T>>::type>
		base_element(T&&) noexcept { return {}; }


	// ---------------------------
	// cv-PRESERVED implementation
	// ---------------------------
	template<class T> struct base_element_cv_impl {
		using type = std::remove_reference_t<T>;
	};

	// arrays
	template<class T> struct base_element_cv_impl<T[]> : base_element_cv_impl<T> {};
	template<class T, std::size_t N>
	struct base_element_cv_impl<T[N]> : base_element_cv_impl<T> {};

	// pointers (preserve cv on the *ultimate* element, so don't strip here)
	template<class T> struct base_element_cv_impl<T*> : base_element_cv_impl<T> {};

	// pointers-to-arrays
	template<class T> struct base_element_cv_impl<T(*)[]> : base_element_cv_impl<T> {};
	template<class T, std::size_t N>
	struct base_element_cv_impl<T(*)[N]> : base_element_cv_impl<T> {};

	template<class T>
	constexpr std::type_identity<typename base_element_cv_impl<std::remove_cvref_t<T>>::type>
		base_element_cv(T&&) noexcept { return {}; }


	// ---------------------------
	// DECAY variant (scalar, cv off)
	// ---------------------------
	template<class T>
	constexpr std::type_identity<
		std::remove_cv_t<typename base_element_impl<std::remove_cvref_t<T>>::type>
	>
		base_element_decay(T&&) noexcept { return {}; }

} // namespace type_determination


// ===========================================
// Public macros
// ===========================================

// Expression-based (cv stripped)
#define BASE_TYPE(expr_) \
    typename decltype(::type_determination::base_element((expr_)))::type

// Expression-based (cv preserved)
#define BASE_TYPE_CV(expr_) \
    typename decltype(::type_determination::base_element_cv((expr_)))::type

// Expression-based (decayed ultimate element; cv stripped)
#define BASE_TYPE_DECAY(expr_) \
    typename decltype(::type_determination::base_element_decay((expr_)))::type


// ----------------------------------------------------
// Type-based forms
// (These macros fabricate an expression from a TYPE.)
// Use only if you want to pass a type instead of expr.
// ----------------------------------------------------
#define BASE_TYPE_FROM_TYPE(T_) \
    BASE_TYPE( *static_cast<std::add_pointer_t<(T_)>>(nullptr) )

#define BASE_TYPE_CV_FROM_TYPE(T_) \
    BASE_TYPE_CV( *static_cast<std::add_pointer_t<(T_)>>(nullptr) )

#define BASE_TYPE_DECAY_FROM_TYPE(T_) \
    BASE_TYPE_DECAY( *static_cast<std::add_pointer_t<(T_)>>(nullptr) )


namespace concat_string_literals {
	template <size_t N>
	constexpr std::array<char, N> to_array(const char(&s)[N]) {
		std::array<char, N> out{};
		for (size_t i = 0; i < N; ++i)
		{
			out[i] = s[i];
		}
		return out;
	}

	// Base concat of two arrays
	template <size_t N1, size_t N2>
	constexpr auto concat_array(const std::array<char, N1>& a, const std::array<char, N2>& b) {
		std::array<char, N1 + N2 - 1> out{};
		for (size_t i = 0; i < N1 - 1; ++i)
		{
			out[i] = a[i];
		}
		for (size_t i = 0; i < N2; ++i)
		{
			out[(N1 - 1) + i] = b[i];
		}
		return out;
	}

	// Variadic concat for any number of strings
	template <typename... Parts>
	constexpr auto concat_impl(const Parts&... parts);

	template <typename First>
	constexpr auto concat_impl(const First& first) { return to_array(first); }

	template <typename First, typename Second, typename... Rest>
	constexpr auto concat_impl(const First& a, const Second& b, const Rest&... rest) {
		if constexpr (sizeof...(Rest) == 0)
		{
			return concat_array(to_array(a), to_array(b));
		}
		else
		{
			return concat_impl(concat_array(to_array(a), to_array(b)), rest...);
		}
	}
}

// Public user-facing helper: returns constexpr const char*
template <typename... Parts>
constexpr auto concat(const Parts&... parts) {
	constexpr auto arr = concat_string_literals::concat_impl(parts...);
	return arr.data();
}

//------------------------------------------------------------------------------
// Helper: unify "pointer-like" access into a raw pointer via addr(p).
// Supports raw pointer, smart pointers with .get(), WRL::ComPtr with .Get(),
// and (as a last resort) things that only expose operator->().
//------------------------------------------------------------------------------
template <typename P>
constexpr auto addr(const P& p) noexcept {
	if constexpr (std::is_pointer_v<P>) {
		return p;                    // raw pointer
	}
	else if constexpr (requires { p.get(); }) {
		return p.get();              // std::unique_ptr / std::shared_ptr / etc.
	}
	else if constexpr (requires { p.Get(); }) {
		return p.Get();              // Microsoft::WRL::ComPtr
	}
	else if constexpr (requires { p.operator->(); }) {
		return p.operator->();       // fallback: has operator->()
	}
	else {
		// Not pointer-like; this return will only be used if this branch is taken.
		return static_cast<void*>(nullptr);
	}
}

// Compares addresses of object pointers (or nullptr) safely.
// Accepts P1/P2 being either an object pointer OR std::nullptr_t.
template <typename P1, typename P2>
constexpr bool same_address(P1 p1, P2 p2) noexcept {
	// Fast path: any type that can become a 'void const *' (object pointers, nullptr)
	if constexpr (std::is_convertible_v<P1, void const*> &&
		std::is_convertible_v<P2, void const*>) {
		return static_cast<void const*>(p1) == static_cast<void const*>(p2);
	}
	else {
		// If either side is nullptr, rely on built-in comparisons against nullptr.
		if constexpr (std::is_null_pointer_v<P1>) {
			return p2 == nullptr;
		}
		else if constexpr (std::is_null_pointer_v<P2>) {
			return p1 == nullptr;
		}
		else {
			// Fallback: function pointers or exotic cases not convertible to void*
			// MSVC handles reinterpret_cast to uintptr_t for function pointers.
			// This is implementation-defined but widely supported in practice.
#if defined(_MSC_VER)
			return reinterpret_cast<std::uintptr_t>(p1) ==
				reinterpret_cast<std::uintptr_t>(p2);
#else
			// On strictly conforming compilers, only allow same-type comparison.
			if constexpr (std::is_same_v<P1, P2>) {
				return p1 == p2;
			}
			else {
				// Different function-pointer types: best we can do portably is "not equal".
				return false;
			}
#endif
		}
	}
}

// Is pointer-like if addr(x) is a raw pointer type.
template <typename T>
concept PointerLike = requires (const T & x) {
	{ addr(x) } -> std::same_as<decltype(addr(x))>;
}&& std::is_pointer_v<decltype(addr(std::declval<const T&>()))>;

// Pointee type helper: only valid when PointerLike<T> is true.
template <PointerLike P>
using pointee_t = std::remove_pointer_t<decltype(addr(std::declval<const P&>()))>;

//------------------------------------------------------------------------------
// Equality that handles:
//   * value vs value -> a == b
//   * pointer-like vs value -> *p == v (if p non-null), else false
//   * pointer-like vs pointer-like:
//         if pointee types are equality-comparable and both non-null -> *pa == *pb
//         else compare addresses (pa == pb)
//------------------------------------------------------------------------------
template <typename A, typename B>
constexpr bool safe_equal(const A& a, const B& b) noexcept {
	if constexpr (PointerLike<A> && PointerLike<B>) {
		auto pa = addr(a);
		auto pb = addr(b);

		// If both null or one null:
		if (!pa || !pb)
		{
			return same_address(pa, pb);
		}

		// If pointee types are the same and equality-comparable, compare values.
		using AX = pointee_t<A>;
		using BX = pointee_t<B>;
		if constexpr (std::is_same_v<std::remove_cv_t<AX>, std::remove_cv_t<BX>>
			&& requires (const AX & x, const BX & y) { { x == y } -> std::convertible_to<bool>; }) {
			return *pa == *pb;
		}
		else {
			// Different (or non-comparable) pointee types → compare addresses.
			return same_address(pa, pb);
		}
	}
	else if constexpr (PointerLike<A> && !PointerLike<B>) {
		auto pa = addr(a);
		if (!pa)
		{
			return false;
		}

		using AX = pointee_t<A>;
		if constexpr (requires (const AX & x, const B & y) { { x == y } -> std::convertible_to<bool>; }) {
			return *pa == b;
		}
		else {
			// No valid value comparison.
			return false;
		}
	}
	else if constexpr (!PointerLike<A> && PointerLike<B>) {
		auto pb = addr(b);
		if (!pb)
		{
			return false;
		}

		using BX = pointee_t<B>;
		if constexpr (requires (const A & x, const BX & y) { { x == y } -> std::convertible_to<bool>; }) {
			return a == *pb;
		}
		else {
			// No valid value comparison.
			return false;
		}
	}
	else {
		// Neither side is pointer-like.
		return a == b;
	}
}

template <typename A, typename B>
constexpr bool safe_not_equal(const A& a, const B& b) noexcept {
	return !safe_equal(a, b);
}

#endif // __SYS_HELPERS_H__