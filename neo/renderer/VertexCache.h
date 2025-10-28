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
#ifndef __VERTEXCACHE2_H__
#define __VERTEXCACHE2_H__

#pragma once
#include <utility>

#include "BufferObject.h"

constexpr size_t VERTCACHE_INDEX_MEMORY_PER_FRAME  = 31ULL  * 1024ULL * 1024ULL;
constexpr size_t VERTCACHE_VERTEX_MEMORY_PER_FRAME = 31ULL  * 1024ULL * 1024ULL;
constexpr size_t VERTCACHE_JOINT_MEMORY_PER_FRAME  = 256ULL * 1024ULL;

constexpr size_t VERTCACHE_NUM_FRAMES = 2;

// there are a lot more static indexes than vertexes, because interactions are just new
// index lists that reference existing vertexes
constexpr size_t STATIC_INDEX_MEMORY  = 31ULL * 1024ULL * 1024ULL;
constexpr size_t STATIC_VERTEX_MEMORY = 31ULL * 1024ULL * 1024ULL;	// make sure it fits in VERTCACHE_OFFSET_MASK!

// vertCacheHandle_t packs size, offset, and frame number into 64 bits
typedef uint64 vertCacheHandle_t;
constexpr int VERTCACHE_STATIC = 1;					// in the static set, not the per-frame set
constexpr int VERTCACHE_SIZE_SHIFT = 1;
constexpr int VERTCACHE_SIZE_MASK = 0x7fffff;		// 8 megs 
constexpr int VERTCACHE_OFFSET_SHIFT = 24;
constexpr int VERTCACHE_OFFSET_MASK = 0x1ffffff;	// 32 megs 
constexpr int VERTCACHE_FRAME_SHIFT = 49;
constexpr int VERTCACHE_FRAME_MASK = 0x7fff;		// 15 bits = 32k frames to wrap around

constexpr int VERTEX_CACHE_ALIGN		= 32;
constexpr int INDEX_CACHE_ALIGN			= 16;
constexpr int JOINT_CACHE_ALIGN			= 16;

typedef enum cacheType_e : uint8 {
	CACHE_VERTEX,
	CACHE_INDEX,
	CACHE_JOINT
} cacheType_t;

struct geoBufferSet_t {
	idIndexBuffer			indexBuffer;
	idVertexBuffer			vertexBuffer;
	idJointBuffer			jointBuffer;
	byte *					mappedVertexBase;
	byte *					mappedIndexBase;
	byte *					mappedJointBase;
	idSysInterlockedInteger	indexMemUsed;
	idSysInterlockedInteger	vertexMemUsed;
	idSysInterlockedInteger	jointMemUsed;
	size_t					allocations;	// number of index and vertex allocations combined
};

class idVertexCache {
public:
	void			Init( bool restart = false );
	void			Shutdown();
	void			PurgeAll();

	// call on loading a new map
	void			FreeStaticData();

	// this data is only valid for one frame of rendering
	vertCacheHandle_t	AllocVertex( const void * data, const int bytes ) {
		return ActuallyAlloc( frameData[listNum], data, bytes, CACHE_VERTEX );
	}
	vertCacheHandle_t	AllocIndex( const void * data, const int bytes ) {
		return ActuallyAlloc( frameData[listNum], data, bytes, CACHE_INDEX );
	}
	vertCacheHandle_t	AllocJoint( const void * data, const int bytes ) {
		return ActuallyAlloc( frameData[listNum], data, bytes, CACHE_JOINT );
	}

	// this data is valid until the next map load
	vertCacheHandle_t	AllocStaticVertex( const void * data, const int bytes ) {
		if (std::cmp_greater(staticData.vertexMemUsed.GetValue() + bytes, STATIC_VERTEX_MEMORY)) {
			idLib::FatalError( "AllocStaticVertex failed, increase STATIC_VERTEX_MEMORY" );
		}
		return ActuallyAlloc( staticData, data, bytes, CACHE_VERTEX );
	}
	vertCacheHandle_t	AllocStaticIndex( const void * data, const int bytes ) {
		if (std::cmp_greater(staticData.indexMemUsed.GetValue() + bytes, STATIC_INDEX_MEMORY)) {
			idLib::FatalError( "AllocStaticIndex failed, increase STATIC_INDEX_MEMORY" );
		}
		return ActuallyAlloc( staticData, data, bytes, CACHE_INDEX );
	}

	[[nodiscard]] byte *			MappedVertexBuffer(const vertCacheHandle_t handle ) const
	{
		release_assert( !CacheIsStatic( handle ) );
		const uint64 offset = static_cast<int>(handle >> VERTCACHE_OFFSET_SHIFT) & VERTCACHE_OFFSET_MASK;
		const uint64 frameNum = static_cast<int>(handle >> VERTCACHE_FRAME_SHIFT) & VERTCACHE_FRAME_MASK;
		release_assert( frameNum == ( currentFrame & VERTCACHE_FRAME_MASK ) );
		return frameData[ listNum ].mappedVertexBase + offset;
	}

	[[nodiscard]] byte *			MappedIndexBuffer(const vertCacheHandle_t handle ) const
	{
		release_assert( !CacheIsStatic( handle ) );
		const uint64 offset = static_cast<int>(handle >> VERTCACHE_OFFSET_SHIFT) & VERTCACHE_OFFSET_MASK;
		const uint64 frameNum = static_cast<int>(handle >> VERTCACHE_FRAME_SHIFT) & VERTCACHE_FRAME_MASK;
		release_assert( frameNum == ( currentFrame & VERTCACHE_FRAME_MASK ) );
		return frameData[ listNum ].mappedIndexBase + offset;
	}

	// Returns false if it's been purged
	// This can only be called by the front end, the back end should only be looking at
	// vertCacheHandle_t that are already validated.
	[[nodiscard]] bool			CacheIsCurrent( const vertCacheHandle_t handle ) const
	{
		const bool isStatic = handle & VERTCACHE_STATIC;
		if ( isStatic ) {
			return true;
		}
		const uint64 frameNum = static_cast<uint64>(handle >> VERTCACHE_FRAME_SHIFT) & VERTCACHE_FRAME_MASK;
		if ( frameNum != ( currentFrame & VERTCACHE_FRAME_MASK ) ) {
			return false;
		}
		return true;
	}

	static bool		CacheIsStatic( const vertCacheHandle_t handle ) {
		return ( handle & VERTCACHE_STATIC ) != 0;
	}

	// vb/ib is a temporary reference -- don't store it
	bool			GetVertexBuffer( vertCacheHandle_t handle, idVertexBuffer * vb ) const;
	bool			GetIndexBuffer( vertCacheHandle_t handle, idIndexBuffer * ib ) const;
	bool			GetJointBuffer( vertCacheHandle_t handle, idJointBuffer * jb ) const;

	void			BeginBackEnd();

public:
	uint64			currentFrame;	// for determining the active buffers
	uint64			listNum;		// currentFrame % VERTCACHE_NUM_FRAMES
	uint64			drawListNum;	// (currentFrame-1) % VERTCACHE_NUM_FRAMES

	geoBufferSet_t	staticData;
	geoBufferSet_t	frameData[VERTCACHE_NUM_FRAMES];

	// High water marks for the per-frame buffers
	index_t			mostUsedVertex;
	index_t			mostUsedIndex;
	index_t			mostUsedJoint;

	// Try to make room for <bytes> bytes
	vertCacheHandle_t	ActuallyAlloc( geoBufferSet_t & vcs, const void * data, size_t bytes, cacheType_t type ) const;
};

// platform specific code to memcpy into vertex buffers efficiently
// 16 byte alignment is guaranteed
void CopyBuffer( byte * dst, const byte * src, size_t numBytes );

extern	idVertexCache	vertexCache;

#endif // __VERTEXCACHE2_H__
