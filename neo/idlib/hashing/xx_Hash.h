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

/*
===============================================================================

	xxHasher

===============================================================================
*/

#ifndef __xx_HASH_H__
#define __xx_HASH_H__

#pragma once

#include "xxhash.h"  // assumes you have xxHash source/library in your project

struct XXH32Hasher {
	uint32 operator()(void const* data, size_t len, uint32 seed = 0) const noexcept {
		return static_cast<uint32>(XXH32(data, len, seed));
	}
	template <typename T>
	uint32 operator()(T const& obj, uint32 seed = 0) const noexcept {
		static_assert(std::is_trivially_copyable_v<T>);
		return operator()(&obj, sizeof(obj), seed);
	}
};

struct XXH3_64Hasher {
	uint64 operator()(void const* data, size_t len, uint64 seed = 0) const noexcept {
		return static_cast<uint64>(XXH3_64bits_withSeed(data, len, seed));
	}
	template <typename T>
	uint64 operator()(T const& obj, uint64 seed = 0) const noexcept {
		static_assert(std::is_trivially_copyable_v<T>);
		return operator()(&obj, sizeof(obj), seed);
	}
};


#endif // __xx_HASH_H__ 