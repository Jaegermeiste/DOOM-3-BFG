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

#include <algorithm>

#include "Precompiled.h"
#include "globaldata.h"


#include "doomdef.h"

#include "i_system.h"
#include "p_local.h"

// State.
#include "r_state.h"

//
// P_CheckSight
//




//
// P_DivlineSide
// Returns side 0 (front), 1 (back), or 2 (on).
//
static int8 P_DivlineSide ( const fixed_t x, const fixed_t y, const divline_t* node )
{
	if (node)
	{
		if (!node->dx)
		{
			if (x == node->x)
			{
				return 2;
			}

			if (x <= node->x)
			{
				return static_cast<int8>(node->dy > 0);
			}

			return static_cast<int8>(node->dy < 0);
		}

		if (!node->dy)
		{
			if (x == node->y)
			{
				return 2;
			}

			if (y <= node->y)
			{
				return static_cast<int8>(node->dx < 0);
			}

			return static_cast<int8>(node->dx > 0);
		}

		const fixed_t dx = (x - node->x);
		const fixed_t dy = (y - node->y);

		const fixed_t left = (node->dy) * (dx);
		const fixed_t right = (dy) * (node->dx);

		if (right < left)
		{
			return 0; // front side
		}

		if (left == right)
		{
			return 2;
		}
		return 1;		// back side
	}

	return -1;
}


//
// P_InterceptVector2
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings and addlines traversers.
//
static fixed_t
P_InterceptVector2
( divline_t*	v2,
  divline_t*	v1 )
{
    fixed_t	frac;
    fixed_t	num;
    fixed_t	den;
	
    den = ((v1->dy>>8) * v2->dx) - ((v1->dx>>8) * v2->dy);

    if (den == 0)
    {
	    return 0;
    }
    //	I_Error ("P_InterceptVector: parallel");
    
    num = (((v1->x - v2->x)>>8) * v1->dy) + 
			(((v2->y - v1->y)>>8) * v1->dx);
    frac = (num / den);

    return frac;
}

//
// P_CrossSubsector
// Returns true
//  if ::g->strace crosses the given subsector successfully.
//
static bool P_CrossSubsector (size_t num)
{
    seg_t*		seg;
    line_t*		line;
    int			s1;
    int			s2;
    int			count;
    subsector_t*	sub;
    sector_t*		front;
    sector_t*		back;
    fixed_t		psight_opentop;
    fixed_t		psight_openbottom;
    divline_t		divl;
    vertex_t*		v1;
    vertex_t*		v2;
    fixed_t		frac;
    fixed_t		slope;
	
#ifdef RANGECHECK
    if (num >=::g->subsectors.Num())
    {
	    I_Error ("P_CrossSubsector: ss %i with numss = %i",
	             num,
	             ::g->subsectors.Num());
    }
#endif

    sub = &::g->subsectors[num];
    
    // check ::g->lines
    count = sub->numlines;
    seg = &::g->segs[sub->firstline];

    for ( ; count ; seg++, count--)
    {
	line = seg->linedef;

	// allready checked other side?
	if (line->validcount == ::g->validcount)
	{
		continue;
	}

	line->validcount = ::g->validcount;
		
	v1 = line->v1;
	v2 = line->v2;
	s1 = P_DivlineSide (v1->x,v1->y, &::g->strace);
	s2 = P_DivlineSide (v2->x, v2->y, &::g->strace);

	// line isn't crossed?
	if (s1 == s2)
	{
		continue;
	}

	divl.x = v1->x;
	divl.y = v1->y;
	divl.dx = v2->x - v1->x;
	divl.dy = v2->y - v1->y;
	s1 = P_DivlineSide (::g->strace.x, ::g->strace.y, &divl);
	s2 = P_DivlineSide (::g->t2x, ::g->t2y, &divl);

	// line isn't crossed?
	if (s1 == s2)
	{
		continue;
	}

	// stop because it is not two sided anyway
	// might do this after updating validcount?
	if ( !(line->flags & ML_TWOSIDED) )
	{
		return false;
	}

	// crosses a two sided line
	front = seg->frontsector;
	back = seg->backsector;

	// no wall to block sight with?
	if (front->floorheight == back->floorheight
	    && front->ceilingheight == back->ceilingheight)
	{
		continue;
	}

	// possible occluder
	// because of ceiling height differences
	if (front->ceilingheight < back->ceilingheight)
	{
		psight_opentop = front->ceilingheight;
	}
	else
	{
		psight_opentop = back->ceilingheight;
	}

	// because of ceiling height differences
	if (front->floorheight > back->floorheight)
	{
		psight_openbottom = front->floorheight;
	}
	else
	{
		psight_openbottom = back->floorheight;
	}

	// quick test for totally closed doors
	if (psight_openbottom >= psight_opentop)
	{
		return false; // stop
	}

	frac = P_InterceptVector2 (&::g->strace, &divl);
		
	if (front->floorheight != back->floorheight)
	{
	    slope = ((psight_openbottom - ::g->sightzstart) / frac);
	    ::g->bottomslope = Max(slope, ::g->bottomslope);
	}
		
	if (front->ceilingheight != back->ceilingheight)
	{
	    slope = ((psight_opentop - ::g->sightzstart) / frac);
	    ::g->topslope = Min(slope, ::g->topslope);
	}
		
	if (::g->topslope <= ::g->bottomslope)
	{
		return false; // stop				
	}
    }
    // passed the subsector ok
    return true;		
}



//
// P_CrossBSPNode
// Returns true
//  if ::g->strace crosses the given node successfully.
//
static bool P_CrossBSPNode( const index_t bspnum )
{
	if (bspnum & NF_SUBSECTOR)
	{
		if (bspnum == -1)
		{
			return P_CrossSubsector(0);
		}
		else
		{
			return P_CrossSubsector(bspnum & (~NF_SUBSECTOR));
		}
	}

	node_t* bsp = &::g->nodes[bspnum];

	// decide which side the start point is on
	int8 side = P_DivlineSide(::g->strace.x, ::g->strace.y, reinterpret_cast<divline_t*>(bsp));
	if (side == 2)
	{
		side = 0; // an "on" should cross both ::g->sides
	}

	// cross the starting side
	if (!P_CrossBSPNode(bsp->children[side]))
	{
		return false;
	}

	// the partition plane is crossed here
	if (side == P_DivlineSide(::g->t2x, ::g->t2y, reinterpret_cast<divline_t*>(bsp)))
	{
		// the line doesn't touch the other side
		return true;
	}

	// cross the ending side		
	return P_CrossBSPNode(bsp->children[side ^ 1]);
}


//
// P_CheckSight
// Returns true
//  if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
//
bool P_CheckSight(const mobj_t* t1, const mobj_t* t2)
{
	// First check for trivial rejection.
	if (t1 && t2)
	{
		// Determine subsector entries in REJECT table.
		const size_t s1 = (t1->subsector->sector - ::g->sectors);
		const size_t s2 = (t2->subsector->sector - ::g->sectors);
		const size_t pnum = s1 * ::g->sectors.Num() + s2;
		const index_t bytenum = pnum >> 3;
		const size_t bitnum = 1 << (pnum & 7);

		// Check in REJECT table.
		if (::g->rejectmatrix[bytenum] & bitnum)
		{
			::g->sightcounts[0]++;

			// can't possibly be connected
			return false;
		}

		// An unobstructed LOS is possible.
		// Now look from eyes of t1 to any part of t2.
		::g->sightcounts[1]++;

		::g->validcount++;

		::g->sightzstart = t1->z + t1->height - (t1->height >> 2);
		::g->topslope = (t2->z + t2->height) - ::g->sightzstart;
		::g->bottomslope = (t2->z) - ::g->sightzstart;

		::g->strace.x = t1->x;
		::g->strace.y = t1->y;
		::g->t2x = t2->x;
		::g->t2y = t2->y;
		::g->strace.dx = t2->x - t1->x;
		::g->strace.dy = t2->y - t1->y;

		// the head node is the last node output
		return P_CrossBSPNode(numeric_cast<index_t>(::g->netNodes.Num()) - 1);
	}

	return false;
}



