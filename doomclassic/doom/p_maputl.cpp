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
#include <utility>


#include "m_bbox.h"

#include "doomdef.h"
#include "p_local.h"


// State.
#include "r_state.h"

//
// P_ApproxDistance
// Gives an estimation of distance (not exact)
//

fixed_t
P_ApproxDistance
(fixed_t	dx,
	fixed_t	dy)
{
	dx = abs(dx);
	dy = abs(dy);
	if (dx < dy)
	{
		return dx + dy - (dx >> 1);
	}
	return dx + dy - (dy >> 1);
}


//
// P_PointOnLineSide
// Returns 0 or 1
//
int8 P_PointOnLineSide(const fixed_t x, const fixed_t y, const line_t* line)
{
	if (line)
	{
		if (!line->dx)
		{
			if (x <= line->v1->x)
			{
				return static_cast<int8>(line->dy > 0);
			}

			return static_cast<int8>(line->dy < 0);
		}
		if (!line->dy)
		{
			if (y <= line->v1->y)
			{
				return static_cast<int8>(line->dx < 0);
			}

			return static_cast<int8>(line->dx > 0);
		}

		const fixed_t dx = (x - line->v1->x);
		const fixed_t dy = (y - line->v1->y);

		const fixed_t left = (line->dy * dx);
		const fixed_t right = (dy * line->dx);

		if (right < left)
		{
			return 0; // front side
		}

		return 1; // back side
	}

	return -1; // invalid
}



//
// P_BoxOnLineSide
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
int8 P_BoxOnLineSide (const fixed_t* tmbox, line_t* ld)
{
	int		p1 = 0;
	int		p2 = 0;

	switch (ld->slopetype)
	{
	case ST_HORIZONTAL:
		p1 = tmbox[BOXTOP] > ld->v1->y;
		p2 = tmbox[BOXBOTTOM] > ld->v1->y;
		if (ld->dx < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
		break;

	case ST_VERTICAL:
		p1 = tmbox[BOXRIGHT] < ld->v1->x;
		p2 = tmbox[BOXLEFT] < ld->v1->x;
		if (ld->dy < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
		break;

	case ST_POSITIVE:
		p1 = P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXTOP], ld);
		p2 = P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld);
		break;

	case ST_NEGATIVE:
		p1 = P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXTOP], ld);
		p2 = P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld);
		break;
	}

	if (p1 == p2)
	{
		return p1;
	}
	return -1;
}


//
// P_PointOnDivlineSide
// Returns 0 or 1.
//
int8 P_PointOnDivlineSide (const fixed_t x, const fixed_t y, const divline_t* line)
{
	if (!line->dx)
	{
		if (x <= line->x)
		{
			return line->dy > 0;
		}

		return line->dy < 0;
	}
	if (!line->dy)
	{
		if (y <= line->y)
		{
			return line->dx < 0;
		}

		return line->dx > 0;
	}

	const fixed_t dx = (x - line->x);
	const fixed_t dy = (y - line->y);

	// try to quickly decide by looking at sign bits
	if ((line->dy ^ line->dx ^ dx ^ dy) & 0x80000000)
	{
		if ((line->dy ^ dx) & 0x80000000)
		{
			return 1; // (left is negative)
		}
		return 0;
	}

	const fixed_t left = (line->dy >> 8 * dx >> 8);
	const fixed_t right = (dy >> 8 * line->dx >> 8);

	if (right < left)
	{
		return 0; // front side
	}
	return 1;			// back side
}



//
// P_MakeDivline
//
void
P_MakeDivline
(const line_t* li,
	divline_t* dl)
{
	dl->x = li->v1->x;
	dl->y = li->v1->y;
	dl->dx = li->dx;
	dl->dy = li->dy;
}



//
// P_InterceptVector
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//
fixed_t
P_InterceptVector
(divline_t* v2,
	divline_t* v1)
{
#if 1

	fixed_t den = ((v1->dy >> 8) * v2->dx) - ((v1->dx >> 8), v2->dy);

	if (den == 0)
	{
		return 0;
	}
	//	I_Error ("P_InterceptVector: parallel");

	fixed_t num = (((v1->x - v2->x) >> 8) * v1->dy) + (((v2->y - v1->y) >> 8) * v1->dx);

	const fixed_t frac = (num / den);

	return frac;
#else	// UNUSED, float debug.
	float	frac;
	float	num;
	float	den;
	float	v1x;
	float	v1y;
	float	v1dx;
	float	v1dy;
	float	v2x;
	float	v2y;
	float	v2dx;
	float	v2dy;

	v1x = (float)v1->x / FRACUNIT;
	v1y = (float)v1->y / FRACUNIT;
	v1dx = (float)v1->dx / FRACUNIT;
	v1dy = (float)v1->dy / FRACUNIT;
	v2x = (float)v2->x / FRACUNIT;
	v2y = (float)v2->y / FRACUNIT;
	v2dx = (float)v2->dx / FRACUNIT;
	v2dy = (float)v2->dy / FRACUNIT;

	den = v1dy * v2dx - v1dx * v2dy;

	if (den == 0)
	{
		return 0; // parallel
	}

	num = (v1x - v2x) * v1dy + (v2y - v1y) * v1dx;
	frac = num / den;

	return frac * FRACUNIT;
#endif
}


//
// P_LineOpening
// Sets ::g->opentop and ::g->openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated
//


void P_LineOpening(const line_t* maputil_linedef)
{
	if (maputil_linedef->sidenum[1] == -1)
	{
		// single sided line
		::g->openrange = 0;
		return;
	}

	const sector_t* front = maputil_linedef->frontsector;
	const sector_t* back = maputil_linedef->backsector;

	if (front->ceilingheight < back->ceilingheight)
	{
		::g->opentop = front->ceilingheight;
	}
	else
	{
		::g->opentop = back->ceilingheight;
	}

	if (front->floorheight > back->floorheight)
	{
		::g->openbottom = front->floorheight;
		::g->lowfloor = back->floorheight;
	}
	else
	{
		::g->openbottom = back->floorheight;
		::g->lowfloor = front->floorheight;
	}

	::g->openrange = ::g->opentop - ::g->openbottom;
}


//
// THING POSITION SETTING
//


//
// P_UnsetThingPosition
// Unlinks a thing from block map and ::g->sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.
//
void P_UnsetThingPosition(const mobj_t* thing)
{
	if (thing)
	{
		if (!(thing->flags & MF_NOSECTOR))
		{
			// inert things don't need to be in blockmap?
			// unlink from subsector
			if (thing->snext)
			{
				thing->snext->sprev = thing->sprev;
			}

			if (thing->sprev)
			{
				thing->sprev->snext = thing->snext;
			}
			else
			{
				thing->subsector->sector->thinglist = thing->snext;
			}
		}

		if (!(thing->flags & MF_NOBLOCKMAP))
		{
			// inert things don't need to be in ::g->blockmap
			// unlink from block map
			if (thing->bnext)
			{
				thing->bnext->bprev = thing->bprev;
			}

			if (thing->bprev)
			{
				thing->bprev->bnext = thing->bnext;
			}
			else
			{
				const auto blockx = (thing->x - ::g->blockmap_origin.x);
				const auto blocky = (thing->y - ::g->blockmap_origin.y);

				if (blockx >= 0 && blockx < ::g->blockmap_width
					&& blocky >= 0 && blocky < ::g->blockmap_height)
				{
					::g->blocklinks[blocky * ::g->blockmap_width + blockx] = thing->bnext;
				}
			}
		}
	}
}


//
// P_SetThingPosition
// Links a thing into both a block and a subsector
// based on it's x y.
// Sets thing->subsector properly
//
void P_SetThingPosition(mobj_t* thing)
{
	if (thing)
	{
		// link into subsector
		subsector_t* ss = R_PointInSubsector(thing->x, thing->y);
		thing->subsector = ss;

		if (!(thing->flags & MF_NOSECTOR))
		{
			// invisible things don't go into the sector links
			sector_t* sec = ss->sector;

			thing->sprev = nullptr;
			thing->snext = sec->thinglist;

			if (sec->thinglist)
			{
				sec->thinglist->sprev = thing;
			}

			sec->thinglist = thing;
		}


		// link into ::g->blockmap
		if (!(thing->flags & MF_NOBLOCKMAP))
		{
			// inert things don't need to be in ::g->blockmap		
			const fixed_t blockx = (thing->x - ::g->blockmap_origin.x);
			const fixed_t blocky = (thing->y - ::g->blockmap_origin.y);

			if (blockx >= 0
				&& std::cmp_less(blockx, ::g->blockmap_width)
				&& blocky >= 0
				&& std::cmp_less(blocky, ::g->blockmap_height))
			{
				mobj_t** link = &::g->blocklinks[blocky * ::g->blockmap_width + blockx];

				if (link)
				{
					thing->bprev = nullptr;
					thing->bnext = *link;
					if (*link)
					{
						(*link)->bprev = thing;
					}

					*link = thing;
				}
			}
			else
			{
				// thing is off the map
				thing->bnext = thing->bprev = nullptr;
			}
		}
	}
}



//
// BLOCK MAP ITERATORS
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//


//
// P_BlockLinesIterator
// The ::g->validcount flags are used to avoid checking ::g->lines
// that are marked in multiple mapblocks,
// so increment ::g->validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
template <class T>
	requires std::same_as<std::remove_const_t<T>, line_t>
static bool P_BlockLinesIterator(const int x, const int y, bool(*func)(T*))
{
	if (x < 0 || y < 0 || std::cmp_greater_equal(x, ::g->blockmap_width) || std::cmp_greater_equal(y, ::g->blockmap_height))
	{
		return true;
	}

	if (func)
	{
		const size_t offset = *(::g->blockmap + (y * ::g->blockmap_width + x));

		for (auto list = ::g->blockmaplump + offset; ((list != nullptr) && (*list != -1)); list++)
		{
			line_t* ld = &::g->lines[*list];

			if (ld)
			{
				if (ld->validcount == ::g->validcount)
				{
					continue; // line has already been checked
				}

				ld->validcount = ::g->validcount;

				if (!func(ld))
				{
					return false;
				}
			}
		}

		return true;	// everything was checked
	}

	return false;
}


//
// P_BlockThingsIterator
//
template <class T>
	requires std::same_as<std::remove_const_t<T>, mobj_t>
static bool P_BlockThingsIterator(const int x, const int y, bool(*func)(T*))
{
	if (x < 0 || y < 0 || std::cmp_greater_equal(x, ::g->blockmap_width) || std::cmp_greater_equal(y, ::g->blockmap_height))
	{
		return true;
	}

	if (func)
	{
		for (mobj_t* mobj = ::g->blocklinks[y * ::g->blockmap_width + x]; mobj != nullptr; mobj = mobj->bnext)
		{
			if (!func(mobj))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}



//
// INTERCEPT ROUTINES
//


//
// PIT_AddLineIntercepts.
// Looks for ::g->lines in the given block
// that intercept the given ::g->trace
// to add to the ::g->intercepts list.
//
// A line is crossed if its endpoints
// are on opposite ::g->sides of the ::g->trace.
// Returns true if ::g->earlyout and a solid line hit.
//
static bool PIT_AddLineIntercepts(line_t* ld)
{
	int8			s1 = 0;
	int8			s2 = 0;
	divline_t		dl = {};

	// avoid precision problems with two routines
	if (::g->trace.dx > FRACUNIT * 16
		|| ::g->trace.dy > FRACUNIT * 16
		|| ::g->trace.dx < -FRACUNIT * 16
		|| ::g->trace.dy < -FRACUNIT * 16)
	{
		s1 = P_PointOnDivlineSide(ld->v1->x, ld->v1->y, &::g->trace);
		s2 = P_PointOnDivlineSide(ld->v2->x, ld->v2->y, &::g->trace);
	}
	else
	{
		s1 = P_PointOnLineSide(::g->trace.x, ::g->trace.y, ld);
		s2 = P_PointOnLineSide(::g->trace.x + ::g->trace.dx, ::g->trace.y + ::g->trace.dy, ld);
	}

	if (s1 == s2)
	{
		return true; // line isn't crossed
	}

	// hit the line
	P_MakeDivline(ld, &dl);
	const fixed_t frac = P_InterceptVector(&::g->trace, &dl);

	if (frac < 0)
	{
		return true; // behind source
	}

	// try to early out the check
	if (::g->earlyout
		&& frac < FRACUNIT
		&& !ld->backsector)
	{
		return false;	// stop checking
	}


	::g->intercepts->frac = frac;
	::g->intercept_p->isaline = true;
	::g->intercept_p->d.line = ld;
	::g->intercept_p++;

	return true;	// continue
}



//
// PIT_AddThingIntercepts
//
static bool PIT_AddThingIntercepts( const mobj_t* thing )
{
	fixed_t		x1 = 0;
	fixed_t		y1 = 0;
	fixed_t		x2 = 0;
	fixed_t		y2 = 0;

	divline_t		dl = {};

	const bool tracepositive = (::g->trace.dx ^ ::g->trace.dy) > 0;

	// check a corner to corner cross-section for hit
	if (tracepositive)
	{
		x1 = thing->x - thing->radius;
		y1 = thing->y + thing->radius;

		x2 = thing->x + thing->radius;
		y2 = thing->y - thing->radius;
	}
	else
	{
		x1 = thing->x - thing->radius;
		y1 = thing->y - thing->radius;

		x2 = thing->x + thing->radius;
		y2 = thing->y + thing->radius;
	}

	const int s1 = P_PointOnDivlineSide(x1, y1, &::g->trace);
	const int s2 = P_PointOnDivlineSide(x2, y2, &::g->trace);

	if (s1 == s2)
	{
		return true; // line isn't crossed
	}

	dl.x = x1;
	dl.y = y1;
	dl.dx = x2 - x1;
	dl.dy = y2 - y1;

	const fixed_t frac = P_InterceptVector(&::g->trace, &dl);

	if (frac < 0)
	{
		return true; // behind source
	}

	::g->intercept_p->frac = frac;
	::g->intercept_p->isaline = false;
	::g->intercept_p->d.thing = thing;
	::g->intercept_p++;

	return true;		// keep going
}


//
// P_TraverseIntercepts
// Returns true if the traverser function returns true
// for all ::g->lines.
// 
static bool
P_TraverseIntercepts
(const traverser_t	func,
	const fixed_t	maxfrac)
{
	intercept_t* scan;

	int count = ::g->intercept_p - ::g->intercepts;

	intercept_t* in = nullptr;			// shut up compiler warning

	while (count--)
	{
		fixed_t dist = fixed_t::MAX;
		for (scan = ::g->intercepts; scan < ::g->intercept_p; scan++)
		{
			if (scan->frac < dist)
			{
				dist = scan->frac;
				in = scan;
			}
		}

		if (dist > maxfrac)
		{
			return true; // checked everything in range		
		}

#if 0  // UNUSED
		{
			// don't check these yet, there may be others inserted
			in = scan = ::g->intercepts;
			for (scan = ::g->intercepts; scan < ::g->intercept_p; scan++)
			{
				if (scan->frac > maxfrac)
				{
					*in++ = *scan;
				}
			}
			::g->intercept_p = in;
			return false;
		}
#endif

		if (!func(in))
		{
			return false; // don't bother going farther
		}

		in->frac = fixed_t::MAX;
	}

	return true;		// everything was traversed
}




//
// P_PathTraverse
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all ::g->lines.
//
bool P_PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, const int flags, bool (*trav) (intercept_t*))
{
	fixed_t	xstep = 0;
	fixed_t	ystep = 0;

	fixed_t	partial = 0;

	int64	mapxstep = 0;
	int64	mapystep = 0;

	::g->earlyout = flags & PT_EARLYOUT;

	::g->validcount++;
	::g->intercept_p = ::g->intercepts;

	if (((x1 - ::g->blockmap_origin.x) & (MAPBLOCKSIZE - 1)) == 0)
	{
		x1 += FRACUNIT; // don't side exactly on a line
	}

	if (((y1 - ::g->blockmap_origin.y) & (MAPBLOCKSIZE - 1)) == 0)
	{
		y1 += FRACUNIT; // don't side exactly on a line
	}

	::g->trace.x = x1;
	::g->trace.y = y1;
	::g->trace.dx = x2 - x1;
	::g->trace.dy = y2 - y1;

	x1 -= ::g->blockmap_origin.x;
	y1 -= ::g->blockmap_origin.y;
	const fixed_t xt1 = x1;
	const fixed_t yt1 = y1;

	x2 -= ::g->blockmap_origin.x;
	y2 -= ::g->blockmap_origin.y;
	const fixed_t xt2 = x2;
	const fixed_t yt2 = y2;

	if (xt2 > xt1)
	{
		mapxstep = 1;
		partial = FRACUNIT - ((x1 >> MAPBTOFRAC) & (FRACUNIT - 1));
		ystep = (y2 - y1 / fixed_t::abs(x2 - x1));
	}
	else if (xt2 < xt1)
	{
		mapxstep = -1;
		partial = (x1 >> MAPBTOFRAC) & (FRACUNIT - 1);
		ystep = (y2 - y1 / fixed_t::abs(x2 - x1));
	}
	else
	{
		mapxstep = 0;
		partial = FRACUNIT;
		ystep = 256 * FRACUNIT;
	}

	fixed_t yintercept = (y1 >> MAPBTOFRAC) + (partial * ystep);


	if (yt2 > yt1)
	{
		mapystep = 1;
		partial = FRACUNIT - ((y1 >> MAPBTOFRAC) & (FRACUNIT - 1));
		xstep = (x2 - x1 / fixed_t::abs(y2 - y1));
	}
	else if (yt2 < yt1)
	{
		mapystep = -1;
		partial = (y1 >> MAPBTOFRAC) & (FRACUNIT - 1);
		xstep = (x2 - x1 / fixed_t::abs(y2 - y1));
	}
	else
	{
		mapystep = 0;
		partial = FRACUNIT;
		xstep = 256 * FRACUNIT;
	}
	fixed_t xintercept = (x1 >> MAPBTOFRAC) + (partial * xstep);

	// Step through map blocks.
	// Count is present to prevent a round off error
	// from skipping the break.
	int mapx = xt1;
	int mapy = yt1;

	for (size_t count = 0; count < 64; ++count)
	{
		if (flags & PT_ADDLINES)
		{
			if (!P_BlockLinesIterator(mapx, mapy, PIT_AddLineIntercepts))
			{
				return false; // early out
			}
		}

		if (flags & PT_ADDTHINGS)
		{
			if (!P_BlockThingsIterator(mapx, mapy, PIT_AddThingIntercepts))
			{
				return false; // early out
			}
		}

		if (mapx == xt2
			&& mapy == yt2)
		{
			break;
		}

		if ((yintercept) == mapy)
		{
			yintercept += ystep;
			mapx += mapxstep;
		}
		else if ((xintercept) == mapx)
		{
			xintercept += xstep;
			mapy += mapystep;
		}

	}
	// go through the sorted list
	return P_TraverseIntercepts(trav, FRACUNIT);
}




