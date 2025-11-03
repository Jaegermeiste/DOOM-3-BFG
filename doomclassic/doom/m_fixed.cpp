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

#include "Precompiled.h"
#include "globaldata.h"

#include <cstdlib>

#include "doomtype.h"
#include "i_system.h"

#ifdef __GNUG__
#pragma implementation "m_fixed.h"
#endif
#include "m_fixed.h"

#if defined(FIXED_T_IS_64_BIT)
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#endif

#include <cstdlib>   // llabs

// ======================= 32-bit kernels (Q16.16) =======================
fixed_t fixed_t::mul32(storage_type a, storage_type b) noexcept {
	using wide = std::int64_t;
	const wide t = static_cast<wide>(a) * static_cast<wide>(b); // Q32.32
	const wide bias = (t >= 0)
		? (static_cast<wide>(1) << (FRACTIONAL_BITS - 1))
		: -(static_cast<wide>(1) << (FRACTIONAL_BITS - 1));
	return fixed_t(static_cast<storage_type>((t + bias) >> FRACTIONAL_BITS), raw);
}

fixed_t fixed_t::div32(storage_type a, storage_type b) noexcept {
	using wide = std::int64_t;
	if (b == 0) return fixed_t((a >= 0) ? MAX : MIN, raw);

	const bool neg = ((a ^ b) < 0);
	const wide n = static_cast<wide>(a) << FRACTIONAL_BITS; // promote numerator
	const wide hb = static_cast<wide>(std::llabs(static_cast<long long>(b))) >> 1; // |b|/2
	const wide nb = neg ? -hb : +hb; // ties-away-from-zero bias
	return fixed_t(static_cast<storage_type>((n + nb) / static_cast<wide>(b)), raw);
}

fixed_t fixed_t::mad32(storage_type a, storage_type b, storage_type c) noexcept {
	using wide = std::int64_t;
	wide t = static_cast<wide>(a) * static_cast<wide>(b);  // Q32.32
	t += (static_cast<wide>(c) << FRACTIONAL_BITS);        // + c in Q32.32
	const wide bias = (t >= 0)
		? (static_cast<wide>(1) << (FRACTIONAL_BITS - 1))
		: -(static_cast<wide>(1) << (FRACTIONAL_BITS - 1));
	return fixed_t(static_cast<storage_type>((t + bias) >> FRACTIONAL_BITS), raw);
}

// ======================= 64-bit kernels (Q32.32) =======================
#if FIXED_T_IS_64BIT

// add unsigned 64 into signed 128 (hi:lo)
static inline void fx_add128_u64(std::uint64_t add, std::int64_t& hi, std::uint64_t& lo) noexcept {
	const std::uint64_t prev = lo;
	lo += add;
	if (lo < prev) { ++hi; }
}

// arithmetic right shift of signed 128 (hi:lo)
static inline std::int64_t fx_arshift128(std::int64_t hi, std::uint64_t lo, unsigned s) noexcept {
#if defined(_MSC_VER)
	std::uint64_t x = __shiftright128(lo, static_cast<std::uint64_t>(hi), static_cast<unsigned char>(s));
#else
	// Portable fallback (combine and shift with sign extension)
	std::uint64_t x = (lo >> s) | (static_cast<std::uint64_t>(hi) << (64u - s));
#endif
	if (hi < 0) {
		if (s < 64) x |= (~std::uint64_t{ 0 }) << (64u - s);
		else        x = ~std::uint64_t{ 0 };
	}
	return static_cast<std::int64_t>(x);
}

fixed_t fixed_t::mul64(storage_type a, storage_type b) noexcept {
#if defined(_MSC_VER)
	std::int64_t  hi{};
	std::uint64_t lo = static_cast<std::uint64_t>(_mul128(a, b, &hi)); // signed 128 in (hi:lo)
#else
	__int128 prod = static_cast<__int128>(a) * static_cast<__int128>(b); // Q64.64
	std::int64_t  hi = static_cast<std::int64_t>(prod >> 64);
	std::uint64_t lo = static_cast<std::uint64_t>(prod);
#endif

	// + 2^(FRACTIONAL_BITS-1) for round-to-nearest (ties away from zero)
	fx_add128_u64(static_cast<std::uint64_t>(1) << (FRACTIONAL_BITS - 1), hi, lo);
	return fixed_t(static_cast<storage_type>(fx_arshift128(hi, lo, FRACTIONAL_BITS)), raw);
}

fixed_t fixed_t::div64(storage_type a, storage_type b) noexcept {
	if (b == 0) return fixed_t((a >= 0) ? MAX : MIN, raw);

	const bool neg = ((a ^ b) < 0);

	// absolute values as unsigned
	const std::uint64_t ua = (a < 0)
		? static_cast<std::uint64_t>(~static_cast<std::uint64_t>(a) + 1)
		: static_cast<std::uint64_t>(a);
	const std::uint64_t ub = (b < 0)
		? static_cast<std::uint64_t>(~static_cast<std::uint64_t>(b) + 1)
		: static_cast<std::uint64_t>(b);

	// shift numerator by FRACTIONAL_BITS into 128-bit (n_hi:n_lo)
	std::uint64_t n_hi = (FRACTIONAL_BITS == 0) ? 0ull : (ua >> (64 - FRACTIONAL_BITS));
	std::uint64_t n_lo = (ua << FRACTIONAL_BITS);

	// + ub/2 for round-to-nearest (ties away from zero)
	const std::uint64_t half = (ub >> 1);
	const std::uint64_t prev = n_lo;
	n_lo += half;
	if (n_lo < prev) { ++n_hi; }

#if defined(_MSC_VER)
	std::uint64_t rem{};
	const std::uint64_t uq = _udiv128(n_hi, n_lo, ub, &rem);
#else
	__int128 numer = (static_cast<__int128>(n_hi) << 64) | static_cast<__int128>(n_lo);
	const std::uint64_t uq = static_cast<std::uint64_t>(numer / ub);
#endif

	const std::int64_t q = static_cast<std::int64_t>(uq);
	return fixed_t(neg ? -q : q, raw);
}

fixed_t fixed_t::mad64(storage_type a, storage_type b, storage_type c) noexcept {
#if defined(_MSC_VER)
	std::int64_t  hi{};
	std::uint64_t lo = static_cast<std::uint64_t>(_mul128(a, b, &hi)); // Q64.64 in (hi:lo)
#else
	__int128 prod = static_cast<__int128>(a) * static_cast<__int128>(b); // Q64.64
	std::int64_t  hi = static_cast<std::int64_t>(prod >> 64);
	std::uint64_t lo = static_cast<std::uint64_t>(prod);
#endif

	// add (c << FRACTIONAL_BITS) into 128-bit accumulator
	const std::uint64_t add_lo = static_cast<std::uint64_t>(static_cast<std::uint64_t>(c) << FRACTIONAL_BITS);
	const std::int64_t  add_hi = (c < 0)
		? -static_cast<std::int64_t>(static_cast<std::uint64_t>(-c) >> (64 - FRACTIONAL_BITS))
		: static_cast<std::int64_t>(static_cast<std::uint64_t>(c) >> (64 - FRACTIONAL_BITS));

	const std::uint64_t p = lo;
	lo += add_lo;
	hi += add_hi + ((lo < p) ? 1 : 0);

	// rounding
	fx_add128_u64(static_cast<std::uint64_t>(1) << (FRACTIONAL_BITS - 1), hi, lo);
	return fixed_t(static_cast<storage_type>(fx_arshift128(hi, lo, FRACTIONAL_BITS)), raw);
}

#else  // !FIXED_T_IS_64BIT

// Stubs to guard against mismatched TU compiles (should never be used on 32-bit)
fixed_t fixed_t::mul64(storage_type, storage_type) noexcept { return fixed_t(0, raw); }
fixed_t fixed_t::div64(storage_type, storage_type) noexcept { return fixed_t(0, raw); }
fixed_t fixed_t::mad64(storage_type, storage_type, storage_type) noexcept { return fixed_t(0, raw); }

#endif // FIXED_T_IS_64BIT

