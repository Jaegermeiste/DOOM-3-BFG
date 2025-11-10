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
#ifndef __UUID7_H__
#define __UUID7_H__

#pragma once

#ifndef ID_MICROSEC_T
#define ID_MICROSEC_T uint64
#endif
#ifndef ID_TIME_T
#define ID_TIME_T int64
#endif

// External clock
extern ID_MICROSEC_T Sys_Microseconds() noexcept;

// ============================================================================
// UUID Type
// ============================================================================
struct Uuid {
	static constexpr size_t UUID_BYTES = 16;
	static constexpr size_t UUID_STR_LEN = 36;   // "8-4-4-4-12"
	static constexpr size_t UUID_STR_LEN_MAX = UUID_STR_LEN + 1;   // +NULL
	static constexpr size_t UUID_THREAD_LOCAL_STR_POOL_SIZE = 10;   // rotating buffers for ToCString()
	static constexpr index_t UUID_DASH_POSITIONS[] = { 8, 13, 18, 23 };
	static constexpr size_t UUID_DASH_POSITIONS_COUNT = std::size(UUID_DASH_POSITIONS);

	idArray<uint8, UUID_BYTES> bytes{};

	// ---- Predefined sentinels ----
	[[nodiscard]] static constexpr Uuid Nil() noexcept;
	[[nodiscard]] static constexpr Uuid Max()  noexcept;
	[[nodiscard]] static constexpr Uuid Fail() noexcept;

	// ---- Element access ----
	[[nodiscard]] uint8& at( const Ordinal auto index) noexcept;
	[[nodiscard]] const uint8& at( const Ordinal auto index) const noexcept;
	[[nodiscard]] uint8& operator[]( const Ordinal auto index) noexcept;
	[[nodiscard]] const uint8& operator[]( const Ordinal auto index) const noexcept;

	[[nodiscard]] static constexpr size_t size() noexcept { return UUID_BYTES; }
	[[nodiscard]] uint8* data()       noexcept { return bytes.Ptr(); }
	[[nodiscard]] const uint8* data() const noexcept { return bytes.Ptr(); }

	// ---- Metadata ----
	[[nodiscard]] bool IsNil() const noexcept;
	[[nodiscard]] bool IsMax() const noexcept;
	[[nodiscard]] bool IsRfc4122Variant() const noexcept;
	[[nodiscard]] int32 Version() const noexcept;

	// ---- Formatting ----
	[[nodiscard]] const char* ToCString() const noexcept;              // thread_local ring buffer
	[[nodiscard]] const char* ToCString(char* out, size_t len) const noexcept;

	// ---- Parsing ----
	[[nodiscard]] static bool ParseCanonical(const char* str, Uuid& out) noexcept;

	// ---- Comparisons ----
	friend bool operator==(const Uuid& a, const Uuid& b) noexcept;
	friend std::strong_ordering operator<=>(const Uuid& a, const Uuid& b) noexcept;
};

// Hash functor for unordered_map/set
struct UuidHash {
	[[nodiscard]] size_t operator()(const Uuid& u) const noexcept;
};

// Generate monotonic UUIDv7 (microsecond-aware)
[[nodiscard]] Uuid GenerateUuidV7Monotonic() noexcept;

#endif // __UUID7_H__