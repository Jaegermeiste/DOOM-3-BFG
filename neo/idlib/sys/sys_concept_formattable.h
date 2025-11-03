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

#ifndef __SYS_CONCEPT_FORMATTABLE_H__
#define __SYS_CONCEPT_FORMATTABLE_H__

#pragma once

#include <concepts>
#include <type_traits>
#include <cstddef>
#include <string>
#include <string_view>
#include <charconv>
#include <format>
#include "magic_enum/magic_enum.hpp"

template<class T>
concept FormattableNoStrings =
	// arithmetic (includes bool implicitly)
	std::is_arithmetic_v<std::remove_cvref_t<T>> ||
	// explicit size_t
	std::is_same_v<std::remove_cvref_t<T>, size_t> ||
	// enums
	std::is_enum_v<std::remove_cvref_t<T>>;


template<class T>
concept Formattable =
#if CPP_STD_VER >= 202302L
	std::formattable<std::remove_cvref_t<T>, char> ||      // primary path if >=C++23
#endif
	FormattableNoStrings<T> ||
	// character scalars
	std::is_same_v<std::remove_cvref_t<T>, char> ||
	std::is_same_v<std::remove_cvref_t<T>, signed char> ||
	std::is_same_v<std::remove_cvref_t<T>, unsigned char> ||
	// C-string pointers
	std::is_same_v<std::remove_cvref_t<T>, const char*> ||
	std::is_same_v<std::remove_cvref_t<T>, char*> ||
	// fixed-size char arrays (string literals)
	(std::is_array_v<std::remove_reference_t<T>> &&
		std::is_same_v<
		std::remove_cv_t<std::remove_extent_t<std::remove_reference_t<T>>>,
		char
		>
		) ||
		// std::string and std::string_view
		std::is_same_v<std::remove_cvref_t<T>, std::string> ||
		std::is_same_v<std::remove_cvref_t<T>, std::string_view> ||
		// idStr or any custom type with operator const char *()
		std::convertible_to<T, const char*>;

#endif // __SYS_CONCEPT_FORMATTABLE_H__