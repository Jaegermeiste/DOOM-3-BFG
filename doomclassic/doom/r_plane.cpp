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

#include <algorithm>

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"




//
// opening
//

// Here comes the obnoxious "visplane".



//
// Clip values are the solid pixel bounding the range.
//  ::g->floorclip starts out SCREENHEIGHT
//  ::g->ceilingclip starts out -1
//

//
// ::g->spanstart holds the start of a plane span
// initialized to 0 at start
//

//
// texture mapping
//





//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes ()
{
  // Doh!
}


//
// R_MapPlane
//
// Uses global vars:
//  ::g->planeheight
//  ::g->ds_source
//  ::g->basexscale
//  ::g->baseyscale
//  ::g->viewx
//  ::g->viewy
//
// BASIC PRIMITIVE
//
void
R_MapPlane
(const int		y,
  const int		x1,
  const int		x2 )
{
	fixed_t	distance;

	//#ifdef RANGECHECK
    if ( x2 < x1 || x1<0 || x2>=::g->viewwidth || y>::g->viewheight )
    {
		//I_Error ("R_MapPlane: %i, %i at %i",x1,x2,y);
		return;
    }
//#endif

    if (::g->planeheight != ::g->cachedheight[y])
    {
	::g->cachedheight[y] = ::g->planeheight;
	distance = ::g->cacheddistance[y] = (::g->planeheight * ::g->yslope[y]);
	::g->ds_xstep = ::g->cachedxstep[y] = (distance *::g->basexscale);
	::g->ds_ystep = ::g->cachedystep[y] = (distance *::g->baseyscale);
    }
    else
    {
	distance = ::g->cacheddistance[y];
	::g->ds_xstep = ::g->cachedxstep[y];
	::g->ds_ystep = ::g->cachedystep[y];
    }
	
	extern angle_t GetViewAngle();
    fixed_t length = (distance * ::g->distscale[x1]);
    angle_t angle = (GetViewAngle() + ::g->xtoviewangle[x1]) >> ANGLETOFINESHIFT;
	extern fixed_t GetViewX(); extern fixed_t GetViewY();
    ::g->ds_xfrac = GetViewX() + (finecosine[angle] * length);
    ::g->ds_yfrac = -GetViewY() - (finesine[angle] * length);

    if (::g->fixedcolormap)
    {
	    ::g->ds_colormap = ::g->fixedcolormap;
    }
    else
    {
	unsigned index = distance >> LIGHTZSHIFT;
	
	if (index >= MAXLIGHTZ )
	{
		index = MAXLIGHTZ-1;
	}

	::g->ds_colormap = ::g->planezlight[index];
    }
	
    ::g->ds_y = y;
    ::g->ds_x1 = x1;
    ::g->ds_x2 = x2;

    // high or low detail
    spanfunc (
		::g->ds_xfrac,
		::g->ds_yfrac,
		::g->ds_y,
		::g->ds_x1,
		::g->ds_x2,
		::g->ds_xstep,
		::g->ds_ystep,
		::g->ds_colormap,
		::g->ds_source );	
}


//
// R_ClearPlanes
// At beginning of frame.
//
void R_ClearPlanes ()
{
	// opening / clipping determination
    for (index_t i = 0 ; std::cmp_less(i, ::g->viewwidth); ++i)
    {
	::g->floorclip[i] = numeric_cast<BASE_TYPE(::g->floorclip)>(::g->viewheight);
	::g->ceilingclip[i] = -1;
    }

	::g->visplanes.Clear();
    ::g->lastopening = ::g->openings;

    // texture calculation
    memset (::g->cachedheight, 0, sizeof(::g->cachedheight));

    // left to right mapping
	extern angle_t GetViewAngle();
    angle_t angle = (GetViewAngle() - ANG90) >> ANGLETOFINESHIFT;
	
    // scale will be unit scale at SCREENWIDTH/2 distance
    ::g->basexscale = (finecosine[angle] *::g->centerxfrac);
    ::g->baseyscale = -(finesine[angle] *::g->centerxfrac);
}




//
// R_FindPlane
//
visplane_t* R_FindPlane( fixed_t height, const index_t picnum, int lightlevel ) {

    if (picnum == ::g->skyflatnum) {
		height = 0;			// all skys map together
		lightlevel = 0;
	}

	index_t visplane_index = 0;
	visplane_t* check = nullptr;

	for (visplane_index = 0; std::cmp_less(visplane_index, ::g->visplanes.Num()); ++visplane_index)
	{
		check = &::g->visplanes[visplane_index];

		if (check)
		{
			if (height == check->height && picnum == check->picnum && lightlevel == check->lightlevel) 
			{
				break;
			}
		}
	}

	if (std::cmp_less(visplane_index, ::g->visplanes.Num()))
	{
		return check;
	}

    //if (::g->lastvisplane - ::g->visplanes == MAXVISPLANES)
		//I_Error ("R_FindPlane: no more visplanes");
	/*if ( ::g->lastvisplane - ::g->visplanes == MAXVISPLANES ) {
		check = ::g->visplanes;
		return check;
	}
		
    ::g->lastvisplane++;*/

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = SCREENWIDTH;
    check->maxx = -1;

    memset(check->top,0xff,sizeof(check->top));

    return check;
}


//
// R_CheckPlane
//
index_t R_CheckPlane ( const index_t pl, const int start, const int stop )
{
    int		intrl = 0;
    int		intrh = 0;
    int		unionl = 0;
    int		unionh = 0;
    int		x = 0;

	ORDINAL_CHECK(pl, ::g->visplanes.Num());

	visplane_t* visplane = &::g->visplanes[pl];
	
	if (start < visplane->minx)
	{
		intrl = visplane->minx;
		unionl = start;
	}
	else
	{
		unionl = visplane->minx;
		intrl = start;
	}

	if (stop > visplane->maxx)
	{
		intrh = visplane->maxx;
		unionh = stop;
	}
	else
	{
		unionh = visplane->maxx;
		intrh = stop;
	}

	for (x=intrl ; x<= intrh ; x++)
	{
		if (visplane->top[x] != 0xffff)
		{
			break;
		}
	}

    if (x > intrh)
	{
		visplane->minx = unionl;
		visplane->maxx = unionh;

		// use the same one
		return pl;		
	}
	
	/*if ( ::g->lastvisplane - ::g->visplanes == MAXVISPLANES ) {
		return pl;
	}*/

    // make a new visplane
	visplane_t new_visplane = {};
	new_visplane.height = visplane->height;
	new_visplane.picnum = visplane->picnum;
	new_visplane.lightlevel = visplane->lightlevel;
	new_visplane.minx = start;
	new_visplane.maxx = stop;

    memset(new_visplane.top,0xff,sizeof(new_visplane.top));

	return ::g->visplanes.AddUnique(new_visplane);
}


//
// R_MakeSpans
//
void
R_MakeSpans
(const int		x,
  int		t1,
  int		b1,
  int		t2,
  int		b2 )
{
    while (t1 < t2 && t1<=b1)
    {
	R_MapPlane (t1,::g->spanstart[t1],x-1);
	t1++;
    }
    while (b1 > b2 && b1>=t1)
    {
	R_MapPlane (b1,::g->spanstart[b1],x-1);
	b1--;
    }
	
    while (t2 < t1 && t2<=b2)
    {
	::g->spanstart[t2] = x;
	t2++;
    }
    while (b2 > b1 && b2>=t2)
    {
	::g->spanstart[b2] = x;
	b2--;
    }
}



//
// R_DrawPlanes
// At the end of each frame.
//
void R_DrawPlanes ()
{
	int			light;
    int			x;
    int			stop;
    angle_t		angle;
				
#ifdef RANGECHECK
    /*if (::g->ds_p - ::g->drawsegs > MAXDRAWSEGS)
    {
	    I_Error ("R_DrawPlanes: ::g->drawsegs overflow (%i)",
	             ::g->ds_p - ::g->drawsegs);
    }*/

    /*if (::g->lastvisplane - ::g->visplanes > MAXVISPLANES)
    {
	    I_Error ("R_DrawPlanes: visplane overflow (%i)",
	             ::g->lastvisplane - ::g->visplanes);
    }*/

    if (::g->lastopening - ::g->openings > MAXOPENINGS)
    {
	    I_Error ("R_DrawPlanes: opening overflow (%i)",
	             ::g->lastopening - ::g->openings);
    }
#endif

    for (visplane_t* pl = ::g->visplanes ; pl < ::g->lastvisplane ; pl++)
    {
	if (pl->minx > pl->maxx)
	{
		continue;
	}


	// sky flat
	if (pl->picnum == ::g->skyflatnum)
	{
	    ::g->dc_iscale = ::g->pspriteiscale>>::g->detailshift;
	    
	    // Sky is allways drawn full bright,
	    //  i.e. ::g->colormaps[0] is used.
	    // Because of this hack, sky is not affected
	    //  by INVUL inverse mapping.
	    ::g->dc_colormap = ::g->colormaps;
	    ::g->dc_texturemid = ::g->skytexturemid;
	    for (x=pl->minx ; x <= pl->maxx ; x++)
	    {
		::g->dc_yl = pl->top[x];
		::g->dc_yh = pl->bottom[x];

		if (::g->dc_yl <= ::g->dc_yh)
		{
			extern angle_t GetViewAngle();
		    angle = (GetViewAngle() + ::g->xtoviewangle[x])>>ANGLETOSKYSHIFT;
		    ::g->dc_x = x;
		    ::g->dc_source = R_GetColumn(::g->skytexture, angle);
		    colfunc ( ::g->dc_colormap, ::g->dc_source );
		}
	    }
	    continue;
	}
	
	// regular flat
	::g->ds_source = static_cast<byte*>(W_CacheLumpNum(::g->firstflat +
	                                                   ::g->flattranslation[pl->picnum],
	                                                   PU_CACHE_SHARED));
	
	::g->planeheight = abs(pl->height-::g->viewz);
	light = (pl->lightlevel >> LIGHTSEGSHIFT)+::g->extralight;

	if (light >= LIGHTLEVELS)
	{
		light = LIGHTLEVELS-1;
	}

	light = Max(light, 0);

	::g->planezlight = ::g->zlight[light];

	pl->top[pl->maxx+1] = 0xffff;
	pl->top[pl->minx-1] = 0xffff;
		
	stop = pl->maxx + 1;

	for (x=pl->minx ; x<= stop ; x++)
	{
	    R_MakeSpans(x,pl->top[x-1],
			pl->bottom[x-1],
			pl->top[x],
			pl->bottom[x]);
	}
    }
}

