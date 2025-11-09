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

#ifndef __SYS_CONCEPT_STRINGLIKEORENUM_H__
#define __SYS_CONCEPT_STRINGLIKEORENUM_H__

#pragma once

#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <concepts>
#include <cstring>
#include <optional>
#include "magic_enum/magic_enum.hpp"

class idStr;

namespace stringlike {

#define TBL_STRINGLIKE_METHOD_LIST \
    X(ToString)                \
    X(toString)                \
    X(AsString)                \
    X(String)                  \
    X(str)                     \
    X(DebugString)

	template<typename T>
	struct DetectStringMethod {
	private:
		template<typename U>
		static auto test(int)
			-> decltype(std::declval<const U>().ToString());
		template<typename> static void test(...);

	public:
		using type = decltype(test<T>(0));
	};

#undef X

	// utility: test a specific method name and return type
#define X(NAME)                                                                                 \
    template<typename T>                                                                        \
    concept Has_##NAME = requires(const T& t) { { t.NAME() }; };                                \
    template<typename T>                                                                        \
    struct NAME##_Return {                                                                      \
        using type = decltype(std::declval<const T>().NAME());                                  \
    };                                                                                          \
    template<typename T>                                                                        \
    using NAME##_Return_t = typename NAME##_Return<T>::type;
	TBL_STRINGLIKE_METHOD_LIST
#undef X

	template<typename T>
	concept HasCStr = requires(const T & t) {
		{ t.c_str() } -> std::convertible_to<const char*>;
	};

	template<typename T>
	concept HasCharPtrCast = requires(const T & t) {
		{ static_cast<const char*>(t) } -> std::convertible_to<const char*>;
	};

	template<typename T>
	concept HasAnyStringMethod =
		Has_ToString<T> || Has_toString<T> || Has_AsString<T> ||
		Has_String<T> || Has_str<T> || Has_DebugString<T>;

	template<typename R>
	constexpr bool returns_cstring_v =
		std::is_convertible_v<R, const char*>;

	template<typename R>
	constexpr bool returns_stdstring_v =
		std::is_same_v<std::remove_cvref_t<R>, std::string>;

} // namespace stringlike


template <class T>
concept StringLike =
	std::same_as<std::remove_cvref_t<T>, idStr>
	|| std::convertible_to<T, const char*>                 // covers const char*, char*, string literals
	|| std::same_as<std::remove_cvref_t<T>, std::string>
	|| std::same_as<std::remove_cvref_t<T>, std::string_view>
	|| stringlike::HasAnyStringMethod< std::remove_cvref_t<T>>;

template <class T>
concept StringLikeOrEnum =
	std::is_enum_v<std::remove_reference_t<T>>
	|| StringLike<T>;

// Options to control output for enums
struct EnumNameOptions {
	bool qualified = true;   // "Type::Value" (or "Type::A|B" for flags)
	bool allow_empty = true; // if false, returns "<unknown>" on unmapped values
};

template <typename E>
constexpr E enum_from_cstr(const char* s, const E fallback = E{}) noexcept {
	static_assert(std::is_enum_v<E>, "E must be an enum");
	if (!s || !*s)
	{
		return fallback;
	}

	std::string_view sv{ s };
	if (const size_t pos = sv.find("::"); pos != std::string_view::npos)
	{
		sv.remove_prefix(pos + 2); // strip "Type::" if present
	}

	if (auto e = magic_enum::enum_cast<E>(sv))
	{
		return *e;
	}

	return fallback; // <- 0 by default, or pass your own (e.g., E::Unknown)
}

#endif // __SYS_CONCEPT_STRINGLIKEORENUM_H__