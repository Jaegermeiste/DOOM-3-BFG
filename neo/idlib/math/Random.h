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

/*
===============================================================================

	Random number generator

===============================================================================
*/

class idRandom {
public:
	// --- lifecycle ---
	ID_INLINE                    idRandom() noexcept;
	ID_INLINE explicit           idRandom( uint64 seed ) noexcept;
	virtual                      ~idRandom() = default;

	// --- canonical virtual (non-templated) API ---
	// seeding / querying
	virtual ID_INLINE void       SetSeed64(uint64 seed) noexcept;
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
	ID_INLINE void SetSeed(const integral_or_enum auto& seed) noexcept {
		SetSeed64(static_cast<uint64>(integral_or_enum_to_value(seed)));
	}

	// Bounded integers
	[[nodiscard]] ID_INLINE byte   RandomByte( const integral_or_enum auto& max ) noexcept { return RandomByte(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE int8   RandomInt8( const integral_or_enum auto& max ) noexcept { return RandomInt8(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE uint8  RandomUInt8( const integral_or_enum auto& max ) noexcept { return RandomUInt8(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE int16  RandomInt16( const integral_or_enum auto& max ) noexcept { return RandomInt16(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE uint16 RandomUInt16( const integral_or_enum auto& max ) noexcept { return RandomUInt16(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE int32  RandomInt32( const integral_or_enum auto& max ) noexcept { return RandomInt32(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE uint32 RandomUInt32( const integral_or_enum auto& max ) noexcept { return RandomUInt32(numeric_cast<uint32>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE int64  RandomInt64( const integral_or_enum auto& max ) noexcept { return RandomInt64(numeric_cast<uint64>(integral_or_enum_to_value(max))); }
	[[nodiscard]] ID_INLINE uint64 RandomUInt64( const integral_or_enum auto& max ) noexcept { return RandomUInt64(numeric_cast<uint64>(integral_or_enum_to_value(max))); }

	static constexpr int32 MAX_RAND = 0x7fff; // 15 bits

protected:
	// state (protected so derived classes can use it)
	uint64 state64 = 0;

	// single step (virtual, non-templated)
	virtual ID_INLINE void PermuteSeed() noexcept { state64 = 69069ull * state64 + 1ull; }

	// helpers
	static constexpr uint32 RAND_BITS = 15; // keep old semantics
	ID_INLINE uint32 Next15() noexcept { PermuteSeed(); return static_cast<uint32>(state64 >> (64 - RAND_BITS)); }

	template <class UInt>
	static ID_INLINE UInt ScaleTo(UInt m, uint32 r) noexcept {
		if (m == 0) return 0;
		// floor(r * m / 2^k), with 64-bit intermediate
		return static_cast<UInt>((static_cast<uint64>(r) * static_cast<uint64>(m)) >> RAND_BITS);
	}
};

/*** implementation ***/
ID_INLINE idRandom::idRandom() noexcept {
	SetSeed64(numeric_cast<uint64>(Sys_Milliseconds()));
}
ID_INLINE idRandom::idRandom( uint64 seed ) noexcept {
	SetSeed64(seed);
}

ID_INLINE void idRandom::SetSeed64( uint64 seed ) noexcept {
	state64 = seed;
}
ID_INLINE int64 idRandom::GetSeed() const noexcept {
	return numeric_cast<int64>(state64);
}

/*** unbounded integers ***/
ID_INLINE byte   idRandom::RandomByte() noexcept { return numeric_cast<byte>(ScaleTo<uint32>(static_cast<uint32>(UINT8_MAX), Next15())); }
ID_INLINE int8   idRandom::RandomInt8() noexcept { return numeric_cast<int8>(ScaleTo<uint32>(static_cast<uint32>(INT8_MAX), Next15())); }
ID_INLINE uint8  idRandom::RandomUInt8() noexcept { return numeric_cast<uint8>(ScaleTo<uint32>(static_cast<uint32>(UINT8_MAX), Next15())); }

ID_INLINE int16  idRandom::RandomInt16() noexcept { return numeric_cast<int16>(Next15()); }  // 0..32767
ID_INLINE uint16 idRandom::RandomUInt16() noexcept { return numeric_cast<uint16>(Next15()); }  // 0..32767

ID_INLINE int32  idRandom::RandomInt32() noexcept { return numeric_cast<int32>(Next15()); }  // 0..32767
ID_INLINE uint32 idRandom::RandomUInt32() noexcept { return numeric_cast<uint32>(Next15()); }  // 0..32767

ID_INLINE int64  idRandom::RandomInt64() noexcept { return numeric_cast<int64>(Next15()); }  // 0..32767
ID_INLINE uint64 idRandom::RandomUInt64() noexcept { return numeric_cast<uint64>(Next15()); }  // 0..32767

/*** bounded integers ***/
// Note: For 8-bit types, cap by the type limit; for 16/32/64, cap by MAX_RAND to preserve 15-bit semantics.
ID_INLINE byte   idRandom::RandomByte(uint32 max) noexcept { if (!max) return 0; const uint32 cap = Min(max, static_cast<uint32>(UINT8_MAX));  return numeric_cast<byte>(ScaleTo<uint32>(cap, Next15())); }
ID_INLINE int8   idRandom::RandomInt8(uint32 max) noexcept { if (!max) return 0; const uint32 cap = Min(max, static_cast<uint32>(INT8_MAX));   return numeric_cast<int8>(ScaleTo<uint32>(cap, Next15())); }
ID_INLINE uint8  idRandom::RandomUInt8(uint32 max) noexcept { if (!max) return 0; const uint32 cap = Min(max, static_cast<uint32>(UINT8_MAX));  return numeric_cast<uint8>(ScaleTo<uint32>(cap, Next15())); }

ID_INLINE int16  idRandom::RandomInt16(uint32 max) noexcept { if (!max) return 0; const uint32 cap = Min(max, static_cast<uint32>(MAX_RAND));   return numeric_cast<int16>(ScaleTo<uint32>(cap, Next15())); }
ID_INLINE uint16 idRandom::RandomUInt16(uint32 max) noexcept { if (!max) return 0; const uint32 cap = Min(max, static_cast<uint32>(MAX_RAND));   return numeric_cast<uint16>(ScaleTo<uint32>(cap, Next15())); }

ID_INLINE int32  idRandom::RandomInt32(uint32 max) noexcept { if (!max) return 0; const uint32 cap = Min(max, static_cast<uint32>(MAX_RAND));   return numeric_cast<int32>(ScaleTo<uint32>(cap, Next15())); }
ID_INLINE uint32 idRandom::RandomUInt32(uint32 max) noexcept { if (!max) return 0; const uint32 cap = Min(max, static_cast<uint32>(MAX_RAND));   return numeric_cast<uint32>(ScaleTo<uint32>(cap, Next15())); }

ID_INLINE int64  idRandom::RandomInt64(uint64 max) noexcept { if (!max) return 0; const uint64 cap = Min(max, static_cast<uint64>(MAX_RAND));   return numeric_cast<int64>(ScaleTo<uint64>(cap, Next15())); }
ID_INLINE uint64 idRandom::RandomUInt64(uint64 max) noexcept { if (!max) return 0; const uint64 cap = Min(max, static_cast<uint64>(MAX_RAND));   return numeric_cast<uint64>(ScaleTo<uint64>(cap, Next15())); }

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
	// Stronger 64-bit LCG step (unsigned wraparound)
	ID_INLINE void PermuteSeed() noexcept override { state64 = 1664525ull * state64 + 1013904223ull; }

private:
	// IEEE layouts
	static constexpr uint32 IEEE_ONE_F = 0x3f800000u;               // float  1.0
	static constexpr uint32 IEEE_MASK_F = 0x007fffffu;               // 23-bit mantissa
	static constexpr uint64 IEEE_ONE_D = 0x3ff0'0000'0000'0000ull;  // double 1.0
	static constexpr uint64 IEEE_MASK_D = 0x000f'ffff'ffff'ffffull;  // 52-bit mantissa

	// Helper to get 64 high-quality bits (advance once)
	ID_INLINE uint64 Next64() noexcept { PermuteSeed(); return state64; }
};

/*** float/double via IEEE mantissa splice ***/
ID_INLINE float idRandom2::RandomFloat() noexcept {
	PermuteSeed();
	const uint32 mant = static_cast<uint32>((state64 >> (64 - 23)) & IEEE_MASK_F);
	const uint32 bits = IEEE_ONE_F | mant;               // 1.mmmm…
	const float  f = std::bit_cast<float>(bits);      // [1,2)
	return f - 1.0f;                                     // [0,1)
}
ID_INLINE float idRandom2::CRandomFloat() noexcept {
	PermuteSeed();
	const uint32 mant = static_cast<uint32>((state64 >> (64 - 23)) & IEEE_MASK_F);
	const uint32 bits = IEEE_ONE_F | mant;
	const float  f = std::bit_cast<float>(bits);      // [1,2)
	return 2.0f * f - 3.0f;                              // [-1,1)
}

ID_INLINE double idRandom2::RandomDouble() noexcept {
	PermuteSeed();
	const uint64 mant = (state64 >> (64 - 52)) & IEEE_MASK_D;
	const uint64 bits = IEEE_ONE_D | mant;               // 1.mmmm…
	const double d = std::bit_cast<double>(bits);     // [1,2)
	return d - 1.0;                                      // [0,1)
}
ID_INLINE double idRandom2::CRandomDouble() noexcept {
	PermuteSeed();
	const uint64 mant = (state64 >> (64 - 52)) & IEEE_MASK_D;
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
