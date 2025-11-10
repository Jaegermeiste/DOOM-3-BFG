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

#pragma hdrstop
#include "precompiled.h"

// ============================================================================
// Internal helpers (namespace-local)
// ============================================================================
namespace uuidv7_mono_detail {
	// ---- 74-bit counter ----
	struct Counter74 {
		uint16 a12{ 0 };
		uint16 b14{ 0 };
		uint64 c48{ 0 };

		static constexpr uint16 A_MASK = 0x0FFFu;
		static constexpr uint16 B_MASK = 0x3FFFu;
		static constexpr uint64 C_MASK = 0x0000FFFFFFFFFFFFull;

		void RefreshSeed() noexcept {
			static idRandom2 rng;
			a12 = static_cast<uint16>(rng.RandomUInt16(UINT16_MAX) & A_MASK);
			b14 = static_cast<uint16>(rng.RandomUInt16(UINT16_MAX) & B_MASK);
			c48 = rng.RandomUInt64(UINT64_MAX) & C_MASK;
		}

		[[nodiscard]] bool Increment() {

			c48 = (c48 + 1) & C_MASK;
			if (c48 != 0)
			{
				return false;
			}
			b14 = static_cast<uint16>((b14 + 1) & B_MASK);
			if (b14 != 0)
			{
				return false;
			}
			a12 = static_cast<uint16>((a12 + 1) & A_MASK);
			if (a12 != 0)
			{
				return false;
			}

			return true; // overflowed all 74 bits
		}
	};

} // namespace uuidv7_mono_detail

// ============================================================================
// Uuid methods
// ============================================================================
constexpr Uuid Uuid::Nil() noexcept { return Uuid{ { { 0 } } }; }
constexpr Uuid Uuid::Max()  noexcept { return Uuid{ { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } } }; }
constexpr Uuid Uuid::Fail() noexcept { return Max(); }

uint8& Uuid::at( const Ordinal auto index) noexcept {
	ORDINAL_CHECK(index, UUID_BYTES);
	return bytes[index];
}
const uint8& Uuid::at( const Ordinal auto index) const noexcept {
	ORDINAL_CHECK(index, UUID_BYTES);
	return bytes[index];
}
uint8& Uuid::operator[]( const Ordinal auto index) noexcept { return at(index); }
const uint8& Uuid::operator[]( const Ordinal auto index) const noexcept { return at(index); }

bool Uuid::IsNil() const noexcept {
	for (index_t i = 0; std::cmp_less(i, UUID_BYTES); ++i) {
		if (bytes[i] != 0) { return false; }
	}
	return true;
}
bool Uuid::IsMax() const noexcept {
	for (index_t i = 0; std::cmp_less(i, UUID_BYTES); ++i) {
		if (bytes[i] != 0xFF) { return false; }
	}
	return true;
}
bool Uuid::IsRfc4122Variant() const noexcept { return (bytes[8] & 0xC0) == 0x80; }
int32 Uuid::Version() const noexcept { return static_cast<int32>((bytes[6] >> 4) & 0x0F); }

const char* Uuid::ToCString() const noexcept {
	static thread_local char pool[UUID_THREAD_LOCAL_STR_POOL_SIZE][UUID_STR_LEN_MAX];
	static thread_local size_t pool_index = 0;

	char* out = pool[pool_index];
	pool_index = (pool_index + 1) % UUID_THREAD_LOCAL_STR_POOL_SIZE;

	return ToCString(out, UUID_STR_LEN_MAX);
}

const char* Uuid::ToCString(char* out, size_t len) const noexcept {
	if (out == nullptr || len < UUID_STR_LEN_MAX)
	{
		return nullptr;
	}

	memset(out, 0, len);

	static constexpr char HEX[] = "0123456789abcdef";
	
	index_t next_dash = 0;
	index_t out_index = 0;

	for ( index_t byte_index = 0; std::cmp_less(byte_index, UUID_BYTES); ++byte_index ) {
		if (std::cmp_less(next_dash, UUID_DASH_POSITIONS_COUNT) && out_index == UUID_DASH_POSITIONS[next_dash]) {
			out[out_index++] = '-';
			++next_dash;
		}
		const uint8 v = bytes[byte_index];
		out[out_index++] = HEX[(v >> 4) & 0xF];
		out[out_index++] = HEX[v & 0xF];
	}
	out[UUID_STR_LEN] = '\0';
	return out;
}

bool Uuid::ParseCanonical(const char* s, Uuid& out) noexcept {
	if (!s)
	{
		return false;
	}

	for (index_t i = 0; std::cmp_less(i, UUID_STR_LEN); ++i) {
		if (s[i] == '\0')
		{
			return false;
		}
	}

	if (s[UUID_STR_LEN] != '\0')
	{
		return false;
	}

	for (index_t i = 0; std::cmp_less(i, UUID_DASH_POSITIONS_COUNT); ++i) {
		const index_t dash_pos = UUID_DASH_POSITIONS[i];
		if (s[dash_pos] != '-')
		{
			return false;
		}
	}

	auto hex = [](char c) -> int32 {
		if (c >= '0' && c <= '9') { return c - '0'; }
		if (c >= 'a' && c <= 'f') { return 10 + (c - 'a'); }
		if (c >= 'A' && c <= 'F') { return 10 + (c - 'A'); }
		return -1;
		};

	index_t next_dash = 0;
	index_t i = 0;
	index_t out_byte = 0;

	while (out_byte < 16) {
		if (std::cmp_less(next_dash, UUID_DASH_POSITIONS_COUNT) && i == UUID_DASH_POSITIONS[next_dash]) {
			++i;
			++next_dash;
		}

		const int32 hi = hex(s[i++]);
		if (hi < 0)
		{
			return false;
		}

		const int32 lo = hex(s[i++]);
		if (lo < 0)
		{
			return false;
		}

		out.bytes[out_byte++] =
			static_cast<uint8>((hi << 4) | lo);
	}
	return true;
}

bool operator==(const Uuid& a, const Uuid& b) noexcept {
	for (index_t i = 0; std::cmp_less(i, Uuid::UUID_BYTES); ++i) {
		if (a.bytes[i] != b.bytes[i]) {
			return false;
		}
	}
	return true;
}
std::strong_ordering operator<=>(const Uuid& a, const Uuid& b) noexcept {
	for (index_t i = 0; std::cmp_less(i, Uuid::UUID_BYTES); ++i) {
		const uint8 va = a.bytes[i];
		const uint8 vb = b.bytes[i];
		if (va < vb) { return std::strong_ordering::less; }
		if (va > vb) { return std::strong_ordering::greater; }
	}
	return std::strong_ordering::equal;
}

size_t UuidHash::operator()(const Uuid& u) const noexcept {
	uint64 h = 0xcbf29ce484222325ULL;

	for (index_t i = 0; std::cmp_less(i, Uuid::UUID_BYTES); ++i) {
		h ^= u.bytes[i];
		h *= 0x100000001b3ULL;
	}

	if constexpr (sizeof(size_t) == 4)
	{
		h ^= (h >> 32);
	}

	return static_cast<size_t>(h);
}

// ============================================================================
// UUIDv7 Monotonic Generator (microsecond-aware)
// ============================================================================
[[nodiscard]] Uuid GenerateUuidV7Monotonic() noexcept {
	using namespace uuidv7_mono_detail;

	static constexpr uint16 subMsBits = 10;
	static constexpr uint16 subMsMask = static_cast<uint16>((1u << subMsBits) - 1u);

	static std::mutex  state_mutex;
	static ID_TIME_T logical_ms = 0;
	static uint16    last_sub_ms_us = 0;
	static Counter74 counter{};
	static bool      initialized = false;

	const ID_MICROSEC_T unix_us = Sys_Microseconds();
	const ID_TIME_T unix_ms = static_cast<ID_TIME_T>(unix_us / 1000u);
	const uint16    sub_ms_us = static_cast<uint16>(unix_us % 1000u);

	uint64 ts48 = 0;
	uint16 rand_a12 = 0;
	uint16 rand_b14 = 0;
	uint64 rand_c48 = 0;

	{
		std::lock_guard<std::mutex> guard(state_mutex);

		if (!initialized) {
			logical_ms = unix_ms;
			last_sub_ms_us = sub_ms_us;
			counter.RefreshSeed();
			initialized = true;
		}

		if (unix_ms > logical_ms) {
			logical_ms = unix_ms;
			last_sub_ms_us = sub_ms_us;
			counter.RefreshSeed();
		}
		else if (unix_ms < logical_ms) {
			// rollback ignored
		}
		else {
			if (sub_ms_us > last_sub_ms_us) {
				last_sub_ms_us = sub_ms_us;
				counter.RefreshSeed();
			}
			else if (sub_ms_us < last_sub_ms_us) {
				// hold steady
			}
			else {
				if (counter.Increment()) {
					++logical_ms;
					last_sub_ms_us = 0;
					counter.RefreshSeed();
				}
			}
		}

		ts48 = static_cast<uint64>(logical_ms) & Counter74::C_MASK;
		rand_a12 = static_cast<uint16>(((last_sub_ms_us & subMsMask) << (12 - subMsBits)) | (counter.a12 & ((1u << (12 - subMsBits)) - 1u)));
		rand_b14 = static_cast<uint16>(counter.b14 & Counter74::B_MASK);
		rand_c48 = counter.c48 & Counter74::C_MASK;
	}

	// ---- assemble UUID ----
	Uuid out{};

	// RFC 9562 fields
	const uint32 time_low = static_cast<uint32>(ts48 >> 16);
	const uint16 time_mid = static_cast<uint16>(ts48 & 0xFFFFu);
	const uint16 time_hi_and_version = static_cast<uint16>((0x7u << 12) | rand_a12);   // version=7
	const uint16 clock_seq = static_cast<uint16>(0x8000u | (rand_b14 & 0x3FFFu)); // variant '10' + 14 bits
	const uint64 node48 = rand_c48;  // lower 48 bits used

	// Write time_low (32, big-endian)
	const uint32 be_time_low = BigULong(time_low);
	std::memcpy(&out.bytes[0], &be_time_low, 4);

	// Write time_mid (16, big-endian)
	const uint16 be_time_mid = BigUShort(time_mid);
	std::memcpy(&out.bytes[4], &be_time_mid, 2);

	// Write time_hi_and_version (16, big-endian; version already set)
	const uint16 be_thv = BigUShort(time_hi_and_version);
	std::memcpy(&out.bytes[6], &be_thv, 2);

	// Write clock_seq (16, big-endian; includes RFC4122 variant in top 2 bits)
	const uint16 be_clock_seq = BigUShort(clock_seq);
	std::memcpy(&out.bytes[8], &be_clock_seq, 2);

	// Write node (48, big-endian)
	// We turn the 48-bit value into big-endian bytes by using a 64-bit BE convert,
	// then copying the lower 6 bytes from the BE representation (skip the top 2).
	const uint64 be_node64 = BigULongLong(node48);
	uint8 be_node_bytes[8] = {};
	std::memcpy(be_node_bytes, &be_node64, 8);
	for (index_t i = 0; i < 6; ++i) {
		out.bytes[10 + i] = be_node_bytes[2 + i];
	}

	return out;
}
