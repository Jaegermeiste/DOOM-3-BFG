#include <algorithm>
#include <utility>

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

#ifndef __HASHINDEX_H__
#define __HASHINDEX_H__

#pragma once

/*
===============================================================================

	Fast hash table for indexes and arrays.
	Does not allocate memory until the first key/index pair is added.

===============================================================================
*/

constexpr auto DEFAULT_HASH_SIZE = 1024;
constexpr auto DEFAULT_HASH_INDEX_SIZE = DEFAULT_HASH_SIZE;
constexpr auto DEFAULT_HASH_GRANULARITY = 1024;

class idHashIndex {
public:
	static constexpr int NULL_INDEX = -1;
					idHashIndex() noexcept;
					idHashIndex( const size_t initialHashSize, const size_t initialIndexSize );
					~idHashIndex();

					// returns total size of allocated memory
	[[nodiscard]] size_t			Allocated() const;
					// returns total size of allocated memory including size of hash index type
	[[nodiscard]] size_t			Size() const;

	idHashIndex &	operator=( const idHashIndex &other );
					// add an index to the hash, assumes the index has not yet been added to the hash
	
	void			Add( const int64 key, const Ordinal auto index );
					// remove an index from the hash
	
	void			Remove( const int64 key, const Ordinal auto index ) const;
					// get the first index from the hash, returns -1 if empty hash entry
	[[nodiscard]] int64			First( const int64 key ) const;
					// get the next index from the hash, returns -1 if at the end of the hash chain
	
	int64			Next( const Ordinal auto index ) const;

	// For porting purposes...
	[[nodiscard]] int64			GetFirst( const int key ) const { return First( key ); }
	
	int64			GetNext( const Ordinal auto index ) const { return Next( index ); }

					// insert an entry into the index and add it to the hash, increasing all indexes >= index
	
	void			InsertIndex( const int64 key, const Ordinal auto index );
					// remove an entry from the index and remove it from the hash, decreasing all indexes >= index
	
	void			RemoveIndex( const int64 key, const Ordinal auto index );
					// clear the hash
	void			Clear() const;
					// clear and resize
	void			Clear( const size_t newHashSize, const size_t newIndexSize );
					// free allocated memory
	void			Free();
					// get size of hash table
	[[nodiscard]] size_t			GetHashSize() const;
					// get size of the index
	[[nodiscard]] size_t			GetIndexSize() const;
					// set granularity
	void			SetGranularity( const size_t newGranularity );
					// force resizing the index, current hash table stays intact
	void			ResizeIndex( const size_t newIndexSize );
					// returns number in the range [0-100] representing the spread over the hash table
	[[nodiscard]] uint8			GetSpread() const;
					// returns a key for a string
	int64			GenerateKey( const char *string, bool caseSensitive = true ) const;
					// returns a key for a vector
	[[nodiscard]] int64			GenerateKey( const idVec3 &v ) const;
					// returns a key for two integers
	[[nodiscard]] int64			GenerateKey( const int64 n1, const int64 n2 ) const;
					// returns a key for a single integer
	[[nodiscard]] int64			GenerateKey( const int64 n ) const;

private:
	size_t			hashSize;
	int64 *			hash;
	size_t			indexSize;
	int64 * 		indexChain;
	size_t			granularity;
	int64			hashMask;
	int				lookupMask;

	static int64	INVALID_HASH[1];
	static int64	INVALID_INDEX[1];

	void			Init( const size_t initialHashSize, const size_t initialIndexSize );
	void			Allocate( const size_t newHashSize, const size_t newIndexSize );
};

/*
================
idHashIndex::idHashIndex
================
*/
ID_INLINE idHashIndex::idHashIndex() noexcept {
	Init( DEFAULT_HASH_SIZE, DEFAULT_HASH_INDEX_SIZE );
}

/*
================
idHashIndex::idHashIndex
================
*/
ID_INLINE idHashIndex::idHashIndex( const size_t initialHashSize, const size_t initialIndexSize ) {
	Init( initialHashSize, initialIndexSize );
}

/*
================
idHashIndex::~idHashIndex
================
*/
ID_INLINE idHashIndex::~idHashIndex() {
	Free();
}

/*
================
idHashIndex::Allocated
================
*/
ID_INLINE size_t idHashIndex::Allocated() const {
	return hashSize * sizeof( int64 ) + indexSize * sizeof( size_t );
}

/*
================
idHashIndex::Size
================
*/
ID_INLINE size_t idHashIndex::Size() const {
	return sizeof( *this ) + Allocated();
}

/*
================
idHashIndex::operator=
================
*/
ID_INLINE idHashIndex &idHashIndex::operator=( const idHashIndex &other ) {
	granularity = other.granularity;
	hashMask = other.hashMask;
	lookupMask = other.lookupMask;

	if ( other.lookupMask == 0 ) {
		hashSize = other.hashSize;
		indexSize = other.indexSize;
		Free();
	}
	else {
		if ( other.hashSize != hashSize || hash == INVALID_HASH ) {
			if ( hash != INVALID_HASH ) {
				delete[] hash;
				hash = nullptr;
			}
			hashSize = other.hashSize;
			hash = new (TAG_IDLIB_HASH) int64[hashSize];
		}
		if ( other.indexSize != indexSize || indexChain == INVALID_INDEX ) {
			if ( indexChain != INVALID_INDEX ) {
				delete[] indexChain;
				indexChain = nullptr;
			}
			indexSize = other.indexSize;
			indexChain = new (TAG_IDLIB_HASH) int64[indexSize];
		}
		memcpy( hash, other.hash, hashSize * sizeof( hash[0] ) );
		memcpy( indexChain, other.indexChain, indexSize * sizeof( indexChain[0] ) );
	}

	return *this;
}

/*
================
idHashIndex::Add
================
*/

ID_INLINE void idHashIndex::Add( const int64 key, const Ordinal auto index ) {
	assert( index >= 0 );
	if ( hash == INVALID_HASH ) {
		Allocate( hashSize, std::cmp_greater_equal(index, indexSize) ? index + 1 : indexSize );
	}
	else if (std::cmp_greater_equal(index, indexSize)) {
		ResizeIndex( index + 1 );
	}
	const int64 h = key & hashMask;
	indexChain[index] = hash[h];
	hash[h] = idMath::integer_cast<int64>(index);
}

/*
================
idHashIndex::Remove
================
*/

ID_INLINE void idHashIndex::Remove( const int64 key, const Ordinal auto index ) const
{
	ORDINAL_CHECK(index, indexSize);
	const int64 k = key & hashMask;

	if ( hash == INVALID_HASH ) {
		return;
	}
	if (std::cmp_equal(hash[k], index)) {
		hash[k] = indexChain[index];
	}
	else {
		for ( int64 i = hash[k]; i != -1; i = indexChain[i] ) {
			if (std::cmp_equal(indexChain[i], index)) {
				indexChain[i] = indexChain[index];
				break;
			}
		}
	}
	indexChain[index] = -1;
}

/*
================
idHashIndex::First
================
*/
ID_INLINE int64 idHashIndex::First( const int64 key ) const {
	return hash[key & hashMask & lookupMask];
}

/*
================
idHashIndex::Next
================
*/
ID_INLINE int64 idHashIndex::Next( const Ordinal auto index ) const {
	assert( index >= 0 && std::cmp_less(index, indexSize) );
	return indexChain[index & lookupMask];
}

/*
================
idHashIndex::InsertIndex
================
*/

ID_INLINE void idHashIndex::InsertIndex( const int64 key, const Ordinal auto index ) {
	ORDINAL_CHECK(index, hashSize);

	if ( hash != INVALID_HASH ) {
		size_t i = 0;
		size_t max = index;
		for ( i = 0; i < hashSize; i++ ) {
			if (std::cmp_greater_equal(hash[i], index)) {
				hash[i]++;
				max = std::max<size_t>(hash[i], max);
			}
		}
		for ( i = 0; i < indexSize; i++ ) {
			if (std::cmp_greater_equal(indexChain[i], index)) {
				indexChain[i]++;
				max = std::max<size_t>(indexChain[i], max);
			}
		}
		if ( max >= indexSize ) {
			ResizeIndex( max + 1 );
		}
		for ( i = max; i > index; i-- ) {
			indexChain[i] = indexChain[i-1];
		}
		indexChain[index] = -1;
	}
	Add( key, index );
}

/*
================
idHashIndex::RemoveIndex
================
*/

ID_INLINE void idHashIndex::RemoveIndex( const int64 key, const Ordinal auto index ) {
	ORDINAL_CHECK(index, hashSize);
	Remove( key, index );
	if ( hash != INVALID_HASH ) {
		size_t i = 0;
		size_t max = idMath::integer_cast<size_t>(index);
		for ( i = 0; i < hashSize; i++ ) {
			if (std::cmp_greater_equal(hash[i], index)) {
				max = std::max<size_t>(hash[i], max);
				hash[i]--;
			}
		}
		for ( i = 0; i < indexSize; i++ ) {
			if ( std::cmp_greater_equal(indexChain[i], index) ) {
				max = std::max<size_t>(indexChain[i], max);
				indexChain[i]--;
			}
		}
		for ( i = index; i < max; i++ ) {
			indexChain[i] = indexChain[i+1];
		}
		indexChain[max] = -1;
	}
}

/*
================
idHashIndex::Clear
================
*/
ID_INLINE void idHashIndex::Clear() const
{
	// only clear the hash table because clearing the indexChain is not really needed
	if ( hash != INVALID_HASH ) {
		memset( hash, 0xff, hashSize * sizeof( hash[0] ) );
	}
}

/*
================
idHashIndex::Clear
================
*/
ID_INLINE void idHashIndex::Clear( const size_t newHashSize, const size_t newIndexSize ) {
	Free();
	hashSize = newHashSize;
	indexSize = newIndexSize;
}

/*
================
idHashIndex::GetHashSize
================
*/
ID_INLINE size_t idHashIndex::GetHashSize() const {
	return hashSize;
}

/*
================
idHashIndex::GetIndexSize
================
*/
ID_INLINE size_t idHashIndex::GetIndexSize() const {
	return indexSize;
}

/*
================
idHashIndex::SetGranularity
================
*/
ID_INLINE void idHashIndex::SetGranularity( const size_t newGranularity ) {
	assert( newGranularity > 0 );
	granularity = newGranularity;
}

/*
================
idHashIndex::GenerateKey
================
*/
ID_INLINE int64 idHashIndex::GenerateKey( const char *string, const bool caseSensitive ) const {
	if ( caseSensitive ) {
		return ( idStr::Hash( string ) & hashMask );
	} else {
		return ( idStr::IHash( string ) & hashMask );
	}
}

/*
================
idHashIndex::GenerateKey
================
*/
ID_INLINE int64 idHashIndex::GenerateKey(const idVec3& v) const {
	return ((idMath::Ftoi64(v[0]) + idMath::Ftoi64(v[1]) + idMath::Ftoi64(v[2])) & hashMask);
}

/*
================
idHashIndex::GenerateKey
================
*/
ID_INLINE int64 idHashIndex::GenerateKey( const int64 n1, const int64 n2 ) const {
	return ( ( n1 + n2 ) & hashMask );
}

/*
================
idHashIndex::GenerateKey
================
*/
ID_INLINE int64 idHashIndex::GenerateKey( const int64 n ) const {
	return n & hashMask;
}

#endif /* !__HASHINDEX_H__ */
