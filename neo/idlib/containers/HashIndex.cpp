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
#include "../precompiled.h"

int64 idHashIndex::INVALID_HASH[1] = { -1 };
int64 idHashIndex::INVALID_INDEX[1] = { -1 };

/*
================
idHashIndex::Init
================
*/
void idHashIndex::Init( const size_t initialHashSize, const size_t initialIndexSize ) {
	assert( idMath::IsPowerOfTwo( initialHashSize ) );

	hashSize = initialHashSize;
	hash = INVALID_HASH;
	indexSize = initialIndexSize;
	indexChain = INVALID_INDEX;
	granularity = DEFAULT_HASH_GRANULARITY;
	hashMask = idMath::integer_cast<int64>(hashSize) - 1;
	lookupMask = 0;
}

/*
================
idHashIndex::Allocate
================
*/
void idHashIndex::Allocate( const size_t newHashSize, const size_t newIndexSize ) {
	assert( idMath::IsPowerOfTwo( newHashSize ) );

	Free();
	hashSize = newHashSize;
	hash = new (TAG_IDLIB_HASH) int64[hashSize];
	memset( hash, 0xff, hashSize * sizeof( hash[0] ) );
	indexSize = newIndexSize;
	indexChain = new (TAG_IDLIB_HASH) int64[indexSize];
	memset( indexChain, 0xff, indexSize * sizeof( indexChain[0] ) );
	hashMask = idMath::integer_cast<int64>(hashSize) - 1;
	lookupMask = -1;
}

/*
================
idHashIndex::Free
================
*/
void idHashIndex::Free() {
	if ( hash != INVALID_HASH ) {
		delete[] hash;
		hash = INVALID_HASH;
	}
	if ( indexChain != INVALID_INDEX ) {
		delete[] indexChain;
		indexChain = INVALID_INDEX;
	}
	lookupMask = 0;
}

/*
================
idHashIndex::ResizeIndex
================
*/
void idHashIndex::ResizeIndex( const size_t newIndexSize ) {
	size_t newSize = 0;

	if ( newIndexSize <= indexSize ) {
		return;
	}

	const size_t mod = newIndexSize % granularity;
	if ( !mod ) {
		newSize = newIndexSize;
	} else {
		newSize = newIndexSize + granularity - mod;
	}

	if ( indexChain == INVALID_INDEX ) {
		indexSize = newSize;
		return;
	}

	const int64* oldIndexChain = indexChain;
	indexChain = new (TAG_IDLIB_HASH) int64[newSize];
	memcpy( indexChain, oldIndexChain, indexSize * sizeof(int64) );
	memset( indexChain + indexSize, 0xff, (newSize - indexSize) * sizeof(int64) );
	delete[] oldIndexChain;
	indexSize = newSize;
}

/*
================
idHashIndex::GetSpread
================
*/
uint8 idHashIndex::GetSpread() const {
	size_t i = 0;

	if ( hash == INVALID_HASH ) {
		return 100;
	}

	size_t totalItems = 0;
	int64* numHashItems = new(TAG_IDLIB_HASH) int64[hashSize];
	for ( i = 0; i < hashSize; i++ ) {
		numHashItems[i] = 0;
		for ( int64 index = hash[i]; index >= 0; index = indexChain[index] ) {
			numHashItems[i]++;
		}
		totalItems += numHashItems[i];
	}
	// if no items in hash
	if ( totalItems <= 1 ) {
		delete[] numHashItems;
		return 100;
	}
	const int64 average = totalItems / hashSize;
	int64 error = 0;
	for ( i = 0; i < hashSize; i++ ) {
		const int64 e = abs(numHashItems[i] - average);
		if ( e > 1 ) {
			error += e - 1;
		}
	}
	delete[] numHashItems;
	return idMath::integer_cast<uint8>(100 - (error * 100 / idMath::integer_cast<int64>(totalItems)));
}
