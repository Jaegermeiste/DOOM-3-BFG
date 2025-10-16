#ifndef __SYS_TYPE_ORDINAL_HPP__
#define __SYS_TYPE_ORDINAL_HPP__

#pragma once

#ifndef POSITIVE_INTEGRAL_CONCEPT
#define POSITIVE_INTEGRAL_CONCEPT
#include <cstddef>
#include <type_traits>
#include <concepts>
#include <gsl/span>

#include "sys_assert.h"// for assert()

namespace idOrdinal
{
	// ============================================================================
	// INTERNAL IMPLEMENTATION
	// ============================================================================

	// -------------------- Ordinal and OrdinalPtr concepts -----------------------
	// ---- base rule: non-bool integrals (incl. MSVC forms) ---------------------
	template <class T>
	concept OrdinalBase_ =
		(!std::same_as<std::remove_cvref_t<T>, bool>) &&
		(std::integral<std::remove_cvref_t<T>>                  // This is buggy on MSVC 2022, C++20
			|| std::same_as<std::remove_cvref_t<T>, std::byte>
			|| std::same_as<std::remove_cvref_t<T>, char>
			|| std::same_as<std::remove_cvref_t<T>, signed char>
			|| std::same_as<std::remove_cvref_t<T>, unsigned char>
			|| std::same_as<std::remove_cvref_t<T>, short>
			|| std::same_as<std::remove_cvref_t<T>, signed short>
			|| std::same_as<std::remove_cvref_t<T>, unsigned short>
			|| std::same_as<std::remove_cvref_t<T>, int>
			|| std::same_as<std::remove_cvref_t<T>, signed int>
			|| std::same_as<std::remove_cvref_t<T>, unsigned int>
			|| std::same_as<std::remove_cvref_t<T>, long>
			|| std::same_as<std::remove_cvref_t<T>, signed long>
			|| std::same_as<std::remove_cvref_t<T>, unsigned long>
			|| std::same_as<std::remove_cvref_t<T>, long long>
			|| std::same_as<std::remove_cvref_t<T>, signed long long>
			|| std::same_as<std::remove_cvref_t<T>, unsigned long long>
			|| std::same_as<std::remove_cvref_t<T>, __int64>
			|| std::same_as<std::remove_cvref_t<T>, signed __int64>
			|| std::same_as<std::remove_cvref_t<T>, unsigned __int64>
			|| std::same_as<std::remove_cvref_t<T>, std::size_t>);

	// ---- public-facing rule: base integrals OR enums whose underlying is base ---
	template <class T>
	concept Ordinal_ = [] {
		using U = std::remove_cvref_t<T>;
		if constexpr (std::is_enum_v<U>) {
			using UT = std::underlying_type_t<U>;
			return OrdinalBase_<UT>;
		}
		else {
			return OrdinalBase_<U>;
		}
		}();

	template <class P>
	concept OrdinalPtr_ =
		std::is_pointer_v<std::remove_cvref_t<P>> &&
		Ordinal_< std::remove_cv_t<std::remove_pointer_t<std::remove_cvref_t<P>>>>;

	// -------------------- as_span_auto (type-deducing helper) -------------------
	// typed pointers → span<I>
	template <Ordinal_ I>
	constexpr gsl::span<I> as_span_auto(I* ptr, std::size_t n) noexcept {
		return ptr ? gsl::span<I>(ptr, n) : gsl::span<I>();
	}
	template <Ordinal_ I>
	constexpr gsl::span<const I> as_span_auto(const I* ptr, std::size_t n) noexcept {
		return ptr ? gsl::span<const I>(ptr, n) : gsl::span<const I>();
	}

	// arrays → span<I> (count ignored)
	template <Ordinal_ I, std::size_t N>
	constexpr gsl::span<I> as_span_auto(I(&arr)[N], std::size_t) noexcept {
		return gsl::span<I>(arr, N);
	}
	template <Ordinal_ I, std::size_t N>
	constexpr gsl::span<const I> as_span_auto(const I(&arr)[N], std::size_t) noexcept {
		return gsl::span<const I>(arr, N);
	}

	// void* → span<std::byte>
	constexpr gsl::span<std::byte> as_span_auto(void* p, std::size_t n) noexcept {
		return p ? gsl::span<std::byte>(static_cast<std::byte*>(p), n) : gsl::span<std::byte>();
	}
	constexpr gsl::span<const std::byte> as_span_auto(const void* p, std::size_t n) noexcept {
		return p ? gsl::span<const std::byte>(static_cast<const std::byte*>(p), n)
			: gsl::span<const std::byte>();
	}

	// nullptr literal → empty byte span
	constexpr gsl::span<std::byte> as_span_auto(std::nullptr_t, std::size_t) noexcept {
		return gsl::span<std::byte>();
	}

	// passthrough for spans
	template <class T, std::size_t Extent>
	constexpr gsl::span<T, Extent> as_span_auto(gsl::span<T, Extent> s) noexcept {
		return s;
	}

	// -------------------- pointer validity helper -------------------------------
	template <typename T>
	constexpr bool ptr_valid(const T& p) noexcept {
		if constexpr (std::is_pointer_v<std::remove_cvref_t<T>>)
		{
			return static_cast<const void*>(p) != nullptr;
		}
		else
		{
			return false;
		}
	}

	// -------------------- ordinal range checker ---------------------------------
	template <Ordinal_ V, Ordinal_ U>
	constexpr bool ordinal_check(V value, U upperBound) noexcept {
		using V0 = std::remove_cvref_t<V>;
		using U0 = std::remove_cvref_t<U>;

		// Negative indices are always invalid
		if constexpr (std::is_signed_v<V0>) {
			if (value < 0)
			{
				return false;
			}
		}
		// Non-positive upper bounds mean no valid indices
		if constexpr (std::is_signed_v<U0>) {
			if (upperBound <= 0)
			{
				return false;
			}
		}

		using UV = std::make_unsigned_t<V0>;
		using UU = std::make_unsigned_t<U0>;
		return static_cast<UV>(value) < static_cast<UU>(upperBound);
	}

} // namespace idOrdinal


// ============================================================================
// PUBLIC SURFACE (only these symbols are visible)
// ============================================================================

// Any integral number, expected to be 0 or positive, including enums
template <class T>
concept Ordinal = idOrdinal::Ordinal_<T>;

template <class P>
concept OrdinalPtr = idOrdinal::OrdinalPtr_<P>;

// auto-deducing span generator (handles nullptr, arrays, pointers, void*)
#define AS_SPAN(expr, count) (idOrdinal::as_span_auto((expr), (count)))

// quick pointer validity check → bool
#define SPAN_VALID(p) (idOrdinal::ptr_valid(p))

// simple numeric bounds check (exclusive upper bound)
#define ORDINAL_CHECK(val, upper) assert(idOrdinal::ordinal_check((val), (upper)) == true)

#endif // POSITIVE_INTEGRAL_CONCEPT

#endif // __SYS_TYPE_ORDINAL_HPP__