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

#ifndef __MATH_RANDOM_H__
#define __MATH_RANDOM_H__

#pragma once

//#define OLD_RANDOM

/*
===============================================================================

	Random number generator

===============================================================================
*/

class idRandom {
public:
	// --- lifecycle ---
	ID_INLINE                    idRandom() noexcept;
	ID_INLINE explicit           idRandom( const uint64 seed ) noexcept;
	virtual                      ~idRandom() = default;

	// --- canonical virtual (non-templated) API ---
	// seeding / querying
	virtual ID_INLINE void       SetSeed64( const uint64 seed ) noexcept;
	[[nodiscard]] virtual ID_INLINE int64 GetSeed() const noexcept; // legacy signature

	// unbounded integers (preserve MAX_RAND = 0x7fff semantics)
	[[nodiscard]] virtual ID_INLINE byte   RandomByte() noexcept;    // [0, 255]
	[[nodiscard]] virtual ID_INLINE int8   RandomInt8() noexcept;    // [0, 127]
	[[nodiscard]] virtual ID_INLINE uint8  RandomUInt8() noexcept;   // [0, 255]
	[[nodiscard]] virtual ID_INLINE int16  RandomInt16() noexcept;   // [0, 32767]
	[[nodiscard]] virtual ID_INLINE uint16 RandomUInt16() noexcept;  // [0, 32767]
	[[nodiscard]] virtual ID_INLINE int32  RandomInt32() noexcept;   // [0, 32767]
	[[nodiscard]] virtual ID_INLINE uint32 RandomUInt32() noexcept;  // [0, 32767]
	[[nodiscard]] virtual ID_INLINE int64  RandomInt64() noexcept;   // [0, 32767]
	[[nodiscard]] virtual ID_INLINE uint64 RandomUInt64() noexcept;  // [0, 32767]

	// bounded integers (multiply-scale; clamp caps)
	[[nodiscard]] virtual ID_INLINE byte   RandomByte( const uint32 max ) noexcept;   // [0, max[
	[[nodiscard]] virtual ID_INLINE int8   RandomInt8( const uint32 max ) noexcept;
	[[nodiscard]] virtual ID_INLINE uint8  RandomUInt8( const uint32 max ) noexcept;
	[[nodiscard]] virtual ID_INLINE int16  RandomInt16( const uint32 max ) noexcept;
	[[nodiscard]] virtual ID_INLINE uint16 RandomUInt16( const uint32 max ) noexcept;
	[[nodiscard]] virtual ID_INLINE int32  RandomInt32( const uint32 max ) noexcept;
	[[nodiscard]] virtual ID_INLINE uint32 RandomUInt32( const uint32 max ) noexcept;
	[[nodiscard]] virtual ID_INLINE int64  RandomInt64( const uint64 max ) noexcept;
	[[nodiscard]] virtual ID_INLINE uint64 RandomUInt64( const uint64 max ) noexcept;

	// reals
	[[nodiscard]] virtual ID_INLINE float       RandomFloat() noexcept;        // [0,1)
	[[nodiscard]] virtual ID_INLINE float       CRandomFloat() noexcept;       // [-1,1)
	[[nodiscard]] virtual ID_INLINE double      RandomDouble() noexcept;       // [0,1)
	[[nodiscard]] virtual ID_INLINE double      CRandomDouble() noexcept;      // [-1,1)
	[[nodiscard]] virtual ID_INLINE long double RandomLongDouble() noexcept;   // [0,1)
	[[nodiscard]] virtual ID_INLINE long double CRandomLongDouble() noexcept;  // [-1,1)

	// --- non-virtual templated convenience wrappers (const-correct + noexcept) ---
	ID_INLINE void SetSeed( const integral_or_enum auto& seed ) noexcept {
		SetSeed64(static_cast<uint64>(integral_or_enum_to_value(seed)));
	}

	// Bounded integers
	[[nodiscard]] ID_INLINE byte   RandomByte(const integral_or_enum auto& max) noexcept { return RandomByte(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE int8   RandomInt8(const integral_or_enum auto& max) noexcept { return RandomInt8(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE uint8  RandomUInt8(const integral_or_enum auto& max) noexcept { return RandomUInt8(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE int16  RandomInt16(const integral_or_enum auto& max) noexcept { return RandomInt16(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE uint16 RandomUInt16(const integral_or_enum auto& max) noexcept { return RandomUInt16(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE int32  RandomInt32(const integral_or_enum auto& max) noexcept { return RandomInt32(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE uint32 RandomUInt32(const integral_or_enum auto& max) noexcept { return RandomUInt32(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE int64  RandomInt64(const integral_or_enum auto& max) noexcept { return RandomInt64(numeric_cast<uint64>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE uint64 RandomUInt64(const integral_or_enum auto& max) noexcept { return RandomUInt64(numeric_cast<uint64>(integral_or_enum_to_value(max))); }

	static constexpr int32 MAX_RAND = 0x7fff; // 15 bits

protected:
#if defined (OLD_RANDOM)
	// state (protected so derived classes can use it)
	uint64 state64 = 0;
#else
	// WELL512a internal state
	uint32 state[16] = {};
	uint32 index = 0;
	uint32 out = 0;
#endif // OLD_RANDOM

	virtual ID_INLINE void PermuteSeed() noexcept {
#if defined (OLD_RANDOM)
		// single step (virtual, non-templated)
		state64 = 69069ull * state64 + 1ull;
#else
		// Advance one step. We implement WELL512a’s transition.

			// Produce one 32-bit output into out_ and advance idx_
		out = Next32();
#endif // OLD_RANDOM
	}

	// helpers
	static constexpr uint32 RAND_BITS = 15;
	ID_INLINE uint32 Next15() noexcept
	{
		PermuteSeed();
#if defined(OLD_RANDOM)
		return static_cast<uint32>(state64 >> (64 - RAND_BITS));
#else
		return out >> (32 - RAND_BITS);
#endif // OLD_RANDOM
	}

	// Core WELL512a step (returns next 32 bits)
	ID_INLINE uint32 Next32() noexcept {
#if defined(OLD_RANDOM)
		PermuteSeed();
		return static_cast<uint32>(state64 >> 32);
#else
		// Reference: Panneton et al., WELL512a parameters
		uint32 a = state[index];
		uint32 c = state[(index + 13) & 15];
		uint32 b = a ^ c ^ (a << 16) ^ (c << 15);
		c = state[(index + 9) & 15];
		c ^= (c >> 11);
		a = state[index] = b ^ c;
		uint32 d = a ^ ((a << 5) & 0xDA442D24u);
		index = (index + 15) & 15;
		uint32 e = state[index];
		state[index] = e ^ b ^ d ^ (e << 2) ^ (b << 18) ^ (c << 28);
		return state[index];
#endif // OLD_RANDOM
	}

	// Compose 64 bits from two 32-bit outputs (advances twice)
	ID_INLINE uint64 Next64() noexcept {
#if defined(OLD_RANDOM)
		PermuteSeed();
		return state64;  // 64-bit LCG state after one step
#else
		const uint64 hi = static_cast<uint64>(Next32()) << 32;
		const uint64 lo = static_cast<uint64>(Next32());
		return hi | lo;
#endif // OLD_RANDOM
	}

	template <class UInt>
	static ID_INLINE UInt ScaleTo(UInt m, uint32 r) noexcept {
		if (m == 0)
		{
			return 0;
		}
		// floor(r * m / 2^k), with 64-bit intermediate
		return static_cast<UInt>((static_cast<uint64>(r) * static_cast<uint64>(m)) >> RAND_BITS);
	}

	template<class UInt>
	ID_INLINE UInt ScaleFrom32(UInt m, uint32 r) noexcept {
		if (!m)
		{
			return 0;
		}
		return static_cast<UInt>((static_cast<uint64>(r) * static_cast<uint64>(m)) >> 32);
	}

	template<class UInt>
	ID_INLINE UInt ScaleFrom64(UInt m, uint64 r) noexcept {
		if (!m)
		{
			return 0;
		}
#if ID_HAS_INT128
		return static_cast<UInt>(
			(static_cast<ID_UINT128>(r) * static_cast<ID_UINT128>(m)) >> 64
			);
#else
		// fallback unbiased method
		const uint64 t = (~uint64{ 0 } - m + 1) % m;
		uint64 x = 0;
		do
		{
			x = Next64();
		} while (x < t);
		return static_cast<UInt>(x % m);
#endif
	}
};

/*** implementation ***/
ID_INLINE idRandom::idRandom() noexcept {
	SetSeed64(numeric_cast<uint64>(Sys_Milliseconds()));
}
ID_INLINE idRandom::idRandom( uint64 seed ) noexcept {
	SetSeed64(seed);
}

ID_INLINE void idRandom::SetSeed64( const uint64 seed ) noexcept {
#if defined (OLD_RANDOM)
	state64 = seed;
#else
	// SplitMix64 to fill state_
	auto next = [s = seed]() mutable noexcept -> uint64 {
		s += 0x9E3779B97F4A7C15ull;
		uint64 z = s;
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
		return z ^ (z >> 31);
		};

	for (uint32& i : state)
	{
		const uint64 z = next();
		i = static_cast<uint32>(z);            // lower 32 ok
	}
	index = 0;

	// Avoid all-zero state (period break) — if degenerate, force a nonzero word
	uint32 accum = 0;
	for (uint32 v : state)
	{
		accum |= v;
	}
	if (accum == 0)
	{
		state[0] = 0xA5A5A5A5u;
	}
#endif // OLD_RANDOM
}

ID_INLINE int64 idRandom::GetSeed() const noexcept {
#if defined(OLD_RANDOM)
	return numeric_cast<int64>(state64);
#else
	// Return a hash of state_ to provide a stable-ish observable
	static constexpr auto HASH_MAGIC = 0x9E3779B97F4A7C15ull;
	uint64 hash = HASH_MAGIC;
	for (uint32 v : state) {
		hash ^= v + HASH_MAGIC + (hash << 6) + (hash >> 2);
	}
	return static_cast<int64>(hash);
#endif // OLD_RANDOM
}

/*** unbounded integers ***/
ID_INLINE byte   idRandom::RandomByte() noexcept { return numeric_cast<byte>(ScaleTo<uint32>(static_cast<uint32>(UINT8_MAX), Next15())); }
ID_INLINE int8   idRandom::RandomInt8() noexcept { return numeric_cast<int8>(ScaleTo<uint32>(static_cast<uint32>(INT8_MAX), Next15())); }
ID_INLINE uint8  idRandom::RandomUInt8() noexcept { return numeric_cast<uint8>(ScaleTo<uint32>(static_cast<uint32>(UINT8_MAX), Next15())); }

ID_INLINE int16  idRandom::RandomInt16() noexcept { return numeric_cast<int16>(Next15()); }  // 0..32767
ID_INLINE uint16 idRandom::RandomUInt16() noexcept { return numeric_cast<uint16>(Next15()); }  // 0..32767

ID_INLINE int32  idRandom::RandomInt32() noexcept
{
#if defined(OLD_RANDOM)
	return numeric_cast<int32>(Next15()); // 0..32767
#else
	return static_cast<int32>(Next32());
#endif
}

ID_INLINE uint32 idRandom::RandomUInt32() noexcept
{
#if defined(OLD_RANDOM)
	return numeric_cast<uint32>(Next15()); // 0..32767
#else
	return Next32();
#endif
}

ID_INLINE int64  idRandom::RandomInt64() noexcept
{
#if defined(OLD_RANDOM)
	return numeric_cast<int64>(Next15()); // 0..32767
#else
	return static_cast<int64>(Next64());
#endif
}

ID_INLINE uint64 idRandom::RandomUInt64() noexcept
{
#if defined(OLD_RANDOM)
	return numeric_cast<uint64>(Next15()); // 0..32767
#else
	return Next64();
#endif
}

/*** bounded integers ***/
// Note: For 8-bit types, cap by the type limit; for 16/32/64, cap by MAX_RAND to preserve 15-bit semantics.
ID_INLINE byte   idRandom::RandomByte( const uint32 max ) noexcept {
	if (!max)
	{
		return 0;
	}
	const uint32 cap = Min(max, static_cast<uint32>(UINT8_MAX));
	return numeric_cast<byte>(ScaleTo<uint32>(cap, Next15()));
}
ID_INLINE int8   idRandom::RandomInt8( const uint32 max ) noexcept {
	if (!max)
	{
		return 0;
	}
	const uint32 cap = Min(max, static_cast<uint32>(INT8_MAX));
	return numeric_cast<int8>(ScaleTo<uint32>(cap, Next15()));
}
ID_INLINE uint8  idRandom::RandomUInt8( const uint32 max ) noexcept {
	if (!max)
	{
		return 0;
	}
	const uint32 cap = Min(max, static_cast<uint32>(UINT8_MAX));
	return numeric_cast<uint8>(ScaleTo<uint32>(cap, Next15()));
}

ID_INLINE int16  idRandom::RandomInt16( const uint32 max ) noexcept {
	if (!max)
	{
		return 0;
	}
	const uint32 cap = Min(max, static_cast<uint32>(MAX_RAND));
	return numeric_cast<int16>(ScaleTo<uint32>(cap, Next15()));
}
ID_INLINE uint16 idRandom::RandomUInt16( const uint32 max ) noexcept {
	if (!max)
	{
		return 0;
	}
	const uint32 cap = Min(max, static_cast<uint32>(MAX_RAND));
	return numeric_cast<uint16>(ScaleTo<uint32>(cap, Next15()));
}

ID_INLINE int32  idRandom::RandomInt32( const uint32 max ) noexcept {
	if (!max)
	{
		return 0;
	}
#if defined(OLD_RANDOM)
	const uint32 cap = Min(max, static_cast<uint32>(MAX_RAND));
	return numeric_cast<int32>(ScaleTo<uint32>(cap, Next15()));
#else
	const int32 cap = numeric_cast<int32>(Min(max, INT32_MAX));
	return ScaleFrom32<int32>(cap, Next32());
#endif
}
ID_INLINE uint32 idRandom::RandomUInt32( const uint32 max ) noexcept {
	if (!max)
	{
		return 0;
	}
#if defined(OLD_RANDOM)
	const uint32 cap = Min(max, static_cast<uint32>(MAX_RAND));
	return numeric_cast<uint32>(ScaleTo<uint32>(cap, Next15()));
#else
	return ScaleFrom32<uint32>(max, Next32());
#endif
}

ID_INLINE int64  idRandom::RandomInt64( const uint64 max ) noexcept {
	if (!max)
	{
		return 0;
	}
#if defined(OLD_RANDOM)
	const uint64 cap = Min(max, static_cast<uint64>(MAX_RAND));
	return numeric_cast<int64>(ScaleTo<uint64>(cap, Next15()));
#else
	const int64 cap = numeric_cast<int64>(Min(max, INT64_MAX));
	return ScaleFrom64<int64>(cap, Next64());
#endif
}
ID_INLINE uint64 idRandom::RandomUInt64( const uint64 max ) noexcept {
	if (!max)
	{
		return 0;
	}
#if defined (OLD_RANDOM)
	const uint64 cap = Min(max, static_cast<uint64>(MAX_RAND));
	return numeric_cast<uint64>(ScaleTo<uint64>(cap, Next15()));
#else
	return ScaleFrom64<uint64>(max, Next64());
#endif
}

/*** reals ***/
// Base uses integer scaling (portable); derived classes can override with IEEE-mantissa splice.
ID_INLINE float       idRandom::RandomFloat()       noexcept { return static_cast<float>(RandomInt32()) / static_cast<float>(MAX_RAND + 1); }
ID_INLINE float       idRandom::CRandomFloat()      noexcept { return 2.0f * (RandomFloat() - 0.5f); }
ID_INLINE double      idRandom::RandomDouble()      noexcept { return static_cast<double>(RandomInt64()) / static_cast<double>(MAX_RAND + 1); }
ID_INLINE double      idRandom::CRandomDouble()     noexcept { return 2.0 * (RandomDouble() - 0.5); }
ID_INLINE long double idRandom::RandomLongDouble()  noexcept { return static_cast<long double>(RandomInt64()) / static_cast<long double>(MAX_RAND + 1); }
ID_INLINE long double idRandom::CRandomLongDouble() noexcept { return 2.0L * (RandomLongDouble() - 0.5L); }


/*
===============================================================================

	Random number generator

===============================================================================
*/

class idRandom2 : public idRandom {
public:
	ID_INLINE                  idRandom2() noexcept : idRandom(static_cast<uint64>(Sys_Milliseconds())) {}
	ID_INLINE explicit         idRandom2( uint64 seed ) noexcept : idRandom(seed) {}

	// Higher-quality reals using IEEE mantissa splice
	[[nodiscard]] ID_INLINE float        RandomFloat()       noexcept override;
	[[nodiscard]] ID_INLINE float        CRandomFloat()      noexcept override;
	[[nodiscard]] ID_INLINE double       RandomDouble()      noexcept override;
	[[nodiscard]] ID_INLINE double       CRandomDouble()     noexcept override;

	// Guarded long double overrides
	[[nodiscard]] ID_INLINE long double  RandomLongDouble()  noexcept override;
	[[nodiscard]] ID_INLINE long double  CRandomLongDouble() noexcept override;

protected:
#if defined(OLD_RANDOM)
	// Stronger 64-bit LCG step (unsigned wraparound)
	ID_INLINE void PermuteSeed() noexcept override { state64 = 1664525ull * state64 + 1013904223ull; }
#endif

private:
	// IEEE layouts
	static constexpr uint32 IEEE_ONE_F = 0x3f800000u;               // float  1.0
	static constexpr uint32 IEEE_MASK_F = 0x007fffffu;               // 23-bit mantissa
	static constexpr uint64 IEEE_ONE_D = 0x3ff0'0000'0000'0000ull;  // double 1.0
	static constexpr uint64 IEEE_MASK_D = 0x000f'ffff'ffff'ffffull;  // 52-bit mantissa
};

/*** float/double via IEEE mantissa splice ***/
ID_INLINE float idRandom2::RandomFloat() noexcept {
#if defined(OLD_RANDOM)
	PermuteSeed();
	const uint32 mant = static_cast<uint32>((state64 >> (64 - 23)) & IEEE_MASK_F);
#else
	const uint32 mant = (Next32() >> (32 - 23)) & IEEE_MASK_F;
#endif // OLD_RANDOM
	const uint32 bits = IEEE_ONE_F | mant;               // 1.mmmm…
	const float  f = std::bit_cast<float>(bits);      // [1,2)
	return f - 1.0f;                                     // [0,1)
}
ID_INLINE float idRandom2::CRandomFloat() noexcept {
#if defined(OLD_RANDOM)
	PermuteSeed();
	const uint32 mant = static_cast<uint32>((state64 >> (64 - 23)) & IEEE_MASK_F);
#else
	const uint32 mant = (Next32() >> (32 - 23)) & IEEE_MASK_F;
#endif // OLD_RANDOM
	const uint32 bits = IEEE_ONE_F | mant;
	const float  f = std::bit_cast<float>(bits);      // [1,2)
	return 2.0f * f - 3.0f;                              // [-1,1)
}

ID_INLINE double idRandom2::RandomDouble() noexcept {
#if defined(OLD_RANDOM)
	PermuteSeed();
	const uint64 mant = (state64 >> (64 - 52)) & IEEE_MASK_D;
#else
	// Need 52 bits: combine two 32-bit pulls
	const uint64 hi = static_cast<uint64>(Next32()) << 20; // take top 32-> use top 20
	const uint64 lo = static_cast<uint64>(Next32()) >> 12; // take top 12
	const uint64 mant = (hi | lo) & IEEE_MASK_D;
#endif // OLD_RANDOM
	const uint64 bits = IEEE_ONE_D | mant;               // 1.mmmm…
	const double d = std::bit_cast<double>(bits);     // [1,2)
	return d - 1.0;                                      // [0,1)
}
ID_INLINE double idRandom2::CRandomDouble() noexcept {
#if defined(OLD_RANDOM)
	PermuteSeed();
	const uint64 mant = (state64 >> (64 - 52)) & IEEE_MASK_D;
#else
	// Need 52 bits: combine two 32-bit pulls
	const uint64 hi = static_cast<uint64>(Next32()) << 20; // take top 32-> use top 20
	const uint64 lo = static_cast<uint64>(Next32()) >> 12; // take top 12
	const uint64 mant = (hi | lo) & IEEE_MASK_D;
#endif // OLD_RANDOM
	const uint64 bits = IEEE_ONE_D | mant;
	const double d = std::bit_cast<double>(bits);     // [1,2)
	return 2.0 * d - 3.0;                                // [-1,1)
}

/*** long double — guarded ***/
#if defined(ID_HAS_INT128)   // only compile this block if 128-bit ints exist
ID_INLINE long double idRandom2::RandomLongDouble() noexcept {
	if constexpr (ID_LDBL_IS_DOUBLE) {
		return static_cast<long double>(RandomDouble());
	}
	else if constexpr (ID_LDBL_IS_QUAD) {
		using U128 = ID_UINT128;
		const uint64 a = Next64();
		const uint64 b = Next64();
		const U128   mant = (static_cast<U128>(a) << 48) | (static_cast<U128>(b) >> 16);
		const U128   ONE = static_cast<U128>(0x3fff) << 112;   // bias 16383
		const U128   bits = ONE | mant;
		const long double v = std::bit_cast<long double>(bits);
		return v - 1.0L;
	}
	else {
		return idRandom::RandomLongDouble();
	}
}

ID_INLINE long double idRandom2::CRandomLongDouble() noexcept {
	if constexpr (ID_LDBL_IS_DOUBLE) {
		return static_cast<long double>(CRandomDouble());
	}
	else if constexpr (ID_LDBL_IS_QUAD) {
		using U128 = ID_UINT128;
		const uint64 a = Next64();
		const uint64 b = Next64();
		const U128   mant = (static_cast<U128>(a) << 48) | (static_cast<U128>(b) >> 16);
		const U128   ONE = static_cast<U128>(0x3fff) << 112;
		const U128   bits = ONE | mant;
		const long double v = std::bit_cast<long double>(bits);
		return 2.0L * v - 3.0L;
	}
	else {
		return idRandom::CRandomLongDouble();
	}
}
#else  // !ID_HAS_INT128
// MSVC and others without 128-bit integers: just forward to base
ID_INLINE long double idRandom2::RandomLongDouble()  noexcept {
	return idRandom::RandomLongDouble();
}
ID_INLINE long double idRandom2::CRandomLongDouble() noexcept {
	return idRandom::CRandomLongDouble();
}
#endif



#endif /* !__MATH_RANDOM_H__ */
