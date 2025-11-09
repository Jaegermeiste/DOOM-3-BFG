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
#ifndef __ARRAY_H__
#define __ARRAY_H__

#pragma once

/*
================================================
idArray is a replacement for a normal C array.

int		myArray[ARRAY_SIZE];

becomes:

idArray<int,ARRAY_SIZE>	myArray;

Has no performance overhead in release builds, but
does index range checking in debug builds.

Unlike idTempArray, the memory is allocated inline with the
object, rather than on the heap.

Unlike idStaticList, there are no fields other than the
actual raw data, and the size is fixed.
================================================
*/
template<class T_, size_t numElements > class idArray {
public:
	// Default constructor
	constexpr idArray() noexcept = default;

	// Initializer list constructor
	constexpr idArray(std::initializer_list<T_> init) noexcept {
		assert(init.size() <= numElements && "Initializer list too large for idArray");
		std::copy(init.begin(), init.end(), ptr);
	}

	// Copy constructor / assignment
	constexpr idArray(const idArray&) noexcept = default;
	constexpr idArray& operator=(const idArray&) noexcept = default;

	// Assignment from initializer list
	constexpr idArray& operator=(std::initializer_list<T_> init) noexcept {
		assert(init.size() <= numElements && "Initializer list too large for idArray");
		std::copy(init.begin(), init.end(), ptr);
		return *this;
	}

	// returns number of elements in list
	[[nodiscard]] static size_t			Num() { return numElements; }

	// returns the number of bytes the array takes up
	[[nodiscard]] size_t			ByteSize() const { return sizeof( ptr ); }

	// memset the entire array to zero
	void			Zero() { memset( ptr, 0, sizeof( ptr ) ); }

	// memset the entire array to a specific value
	void			Memset( const char fill ) { memset( ptr, fill, numElements * sizeof( *ptr ) ); }

	// array operators
	[[nodiscard]] const T_ &		operator[]( const Ordinal auto index ) const noexcept { ORDINAL_CHECK(index, numElements); return ptr[index]; }
	[[nodiscard]] T_ &			operator[]( const Ordinal auto index ) noexcept { ORDINAL_CHECK(index, numElements); return ptr[index]; }

	// returns a pointer to the list
	[[nodiscard]] const T_ *		Ptr() const { return ptr; }
	[[nodiscard]] T_ *  			Ptr() { return ptr; }

	[[nodiscard]] constexpr       T_* begin()  noexcept { return ptr; }
	[[nodiscard]] constexpr       T_* end()    noexcept { return ptr + numElements; }
	[[nodiscard]] constexpr const T_* begin()  const noexcept { return ptr; }
	[[nodiscard]] constexpr const T_* end()    const noexcept { return ptr + numElements; }
	[[nodiscard]] constexpr const T_* cbegin() const noexcept { return begin(); }
	[[nodiscard]] constexpr const T_* cend()   const noexcept { return end(); }

private:
	T_				ptr[numElements];
};

#define ARRAY_COUNT( arrayName ) ( sizeof( arrayName )/sizeof( arrayName[0] ) )
#define ARRAY_DEF( arrayName ) arrayName, ARRAY_COUNT( arrayName )


/*
================================================
id2DArray is essentially a typedef (as close as we can
get for templates before C++11 anyway) to make
declaring two-dimensional idArrays easier.

Usage:
	id2DArray< int, 5, 10 >::type someArray;

================================================
*/
template<class _type_, size_t _dim1_, size_t _dim2_ >
struct id2DArray {
	typedef idArray< idArray< _type_, _dim2_ >, _dim1_ > type;
};


/*
================================================
idTupleSize
Generic way to get the size of a tuple-like type.
Add specializations as needed.
This is modeled after std::tuple_size from C++11,
which works for std::arrays also.
================================================
*/
template< class _type_ >
struct idTupleSize;

template< class _type_, size_t _num_ >
struct idTupleSize< idArray< _type_, _num_ > > {
	enum idTupleSizeValue_e : size_t { value = _num_ };
};

#endif // !__ARRAY_H__
