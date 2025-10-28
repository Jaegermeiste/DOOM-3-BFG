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

#include "sys_numeric_cast.h"

template<class T> concept Numeric = std::is_arithmetic_v<T> || std::is_enum_v<T>;

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


#endif // __SYS_HELPERS_H__