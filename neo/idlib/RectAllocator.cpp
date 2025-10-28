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


/*

This routine performs a tight packing of a list of rectangles, attempting to minimize the area
of the rectangle that encloses all of them.  Algorithm order is N^2, so it is not appropriate
for lists with many thousands of elements.

Contrast with idBitBlockAllocator, which is used incrementally with either fixed size or
size-doubling target areas.

Typical uses:
packing glyphs into a font image
packing model surfaces into a skin atlas
packing images into swf atlases

If you want a minimum alignment, ensure that all the sizes are multiples of that alignment,
or scale the input sizes down by that alignment and scale the outputPositions back up.

*/

static float	RectPackingFraction( const idList<idVec2i> &inputSizes, const idVec2i totalSize ) {
	const int	totalArea = totalSize.Area();
	if ( totalArea == 0 ) {
		return 0;
	}
	int	inputArea = 0;
	for ( size_t i = 0 ; i < inputSizes.Num() ; i++ ) {
		inputArea += inputSizes[i].Area();
	}
	return numeric_cast<float>(inputArea) / numeric_cast<float>(totalArea);
}


class idSortrects : public idSort_Quick< size_t, idSortrects > {
public:
	[[nodiscard]] static size_t SizeMetric( const idVec2i v )
	{
		// skinny rects will sort earlier than square ones, because
		// they are more likely to grow the entire region
		return (v.x * v.x) + (v.y * v.y);
	}

	[[nodiscard]] size_t Compare( const Ordinal auto & a, const Ordinal auto & b ) const {
		ORDINAL_CHECK( a, inputSizes->Num() );
		ORDINAL_CHECK( b, inputSizes->Num() );

		return SizeMetric( (*inputSizes)[b] ) - SizeMetric( (*inputSizes)[a] );
	}

	const idList<idVec2i> *inputSizes;
};

static void RectAllocator( const idList<idVec2i> &inputSizes, idList<idVec2i> &outputPositions, idVec2i &totalSize ) {
	outputPositions.SetNum( inputSizes.Num() );
	if ( inputSizes.Num() == 0 ) {
		totalSize.Set( 0, 0 );
		return;
	}
	idList<size_t> sizeRemap = {};
	sizeRemap.SetNum( inputSizes.Num() );
	for ( size_t i = 0; i < inputSizes.Num(); i++ ) {
		sizeRemap[i] = i;
	}

	// Sort the rects from largest to smallest (it makes allocating them in the image better)
	idSortrects sortRectsBySize = {};
	sortRectsBySize.inputSizes = &inputSizes;
	sizeRemap.SortWithTemplate( sortRectsBySize );

	// the largest rect goes to the top-left corner
	outputPositions[sizeRemap[0]].Set( 0, 0 );

	totalSize = inputSizes[sizeRemap[0]];

	// For each image try to fit it at a corner of one of the already fitted images while
	// minimizing the total area.
	// Somewhat better allocation could be had by checking all the combinations of x and y edges
	// in the allocated rectangles, rather than just the corners of each rectangle, but it
	// still does a pretty good job.
	static constexpr int START_MAX = 1<<14;
	for ( size_t i = 1; i < inputSizes.Num(); i++ ) {
		idVec2i	best( 0, 0 );
		idVec2i	bestMax( START_MAX, START_MAX );
		idVec2i	size = inputSizes[sizeRemap[i]];
		for ( size_t j = 0; j < i; j++ ) {
			for ( size_t k = 1;  k < 4; k++ ) {
				idVec2i	test = {};
				for ( size_t n = 0 ; n < 2 ; n++ ) {
					test[n] = numeric_cast<int32>(outputPositions[sizeRemap[j]][n] + ( ( k >> n ) & 1 ) * inputSizes[sizeRemap[j]][n]);
				}

				idVec2i	newMax = {};
				for ( size_t n = 0 ; n < 2 ; n++ ) {
					newMax[n] = Max( totalSize[n], test[n] + size[n] );
				}
				// widths must be multiples of 128 pixels / 32 DXT blocks to
				// allow it to be used directly as a GPU texture without re-packing
				// FIXME: make this a parameter
				newMax[0] = (newMax[0]+31) & ~31;

				// don't let an image get larger than 1024 DXT block, or PS3 crashes
				// FIXME: pass maxSize in as a parameter
				if ( newMax[0] > 1024 || newMax[1] > 1024 ) {
					continue;
				}

				// if we have already found a spot that keeps the image smaller, don't bother checking here
				// This calculation biases the rect towards more square shapes instead of
				// allowing it to extend in one dimension for a long time.
				const int32	newSize = newMax.x * newMax.x + newMax.y * newMax.y;
				const int32	bestSize = bestMax.x * bestMax.x + bestMax.y * bestMax.y;
				if ( newSize > bestSize ) {
					continue;
				}

				// if the image isn't required to grow, favor the location closest to the origin
				if ( newSize == bestSize && best.x + best.y < test.x + test.y ) {
					continue;
				}

				// see if this spot overlaps any already allocated rect
				size_t n = 0;
				for ( ; n < i; n++ ) {
					const idVec2i &check = outputPositions[sizeRemap[n]];
					const idVec2i &checkSize = inputSizes[sizeRemap[n]];
					if (	test.x + size.x > check.x &&
							test.y + size.y > check.y &&
							test.x < check.x + checkSize.x &&
							test.y < check.y + checkSize.y ) {
						break;
					}
				}
				if ( n < i ) {
					// overlapped, can't use
					continue;
				}
				best = test;
				bestMax = newMax;
			}
		}
		if ( bestMax[0] == START_MAX ) {	// FIXME: return an error code
			idLib::FatalError( "RectAllocator: couldn't fit everything" ); 
		}
		outputPositions[sizeRemap[i]] = best;
		totalSize = bestMax;
	}
}

