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
#include <cmath>

#include <algorithm>
#include <utility>


#include "doomdef.h"
#include "d_net.h"

#include "m_bbox.h"

#include "r_local.h"
#include "r_sky.h"
#include "i_system.h"




// Fineangles in the SCREENWIDTH wide window.




// increment every time a check is made





// just for profiling purposes






// 0 = high, 1 = low

//
// precalculated math tables
//

// The ::g->viewangletox[::g->viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat ::g->projection plane.
// There will be many angles mapped to the same X. 

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest ::g->viewangle that maps back to x ranges
// from ::g->clipangle to -::g->clipangle.


// UNUSED.
// The finetangentgent[angle+FINEANGLES/4] table
// holds the fixed_t tangent values for view angles,
// ranging from MININT to 0 to MAXINT.
// fixed_t		finetangent[FINEANGLES/2];

// fixed_t		finesine[5*FINEANGLES/4];
const fixed_t*		finecosine = &finesine[FINEANGLES/4];



// bumped light from gun blasts



void (*colfunc) (lighttable_t * dc_colormap,
				 byte * dc_source);
void (*basecolfunc) (lighttable_t * dc_colormap,
						byte * dc_source);
void (*fuzzcolfunc) (lighttable_t * dc_colormap,
						byte * dc_source);
static void (*transcolfunc) (lighttable_t * dc_colormap,
                             byte * dc_source);
void (*spanfunc) (fixed_t xfrac,
	fixed_t yfrac,
	fixed_t ds_y,
	int ds_x1,
	int ds_x2,
	fixed_t ds_xstep,
	fixed_t ds_ystep,
	lighttable_t * ds_colormap,
	byte * ds_source);



//
// R_AddPointToBox
// Expand a given bbox
// so that it encloses a given point.
//
void R_AddPointToBox (const int x, const int y, fixed_t* box )
{
	box[BOXLEFT] = Min(x, box[BOXLEFT]);
	box[BOXRIGHT] = Max(x, box[BOXRIGHT]);
	box[BOXBOTTOM] = Min(y, box[BOXBOTTOM]);
	box[BOXTOP] = Max(y, box[BOXTOP]);
}


//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
int8 R_PointOnSide (const fixed_t x, const fixed_t y, const node_t* node )
{
	if (node)
	{
		if (!node->dx)
		{
			if (x <= node->x)
			{
				return node->dy > 0;
			}

			return node->dy < 0;
		}
		if (!node->dy)
		{
			if (y <= node->y)
			{
				return node->dx < 0;
			}

			return node->dx > 0;
		}

		const fixed_t dx = (x - node->x);
		const fixed_t dy = (y - node->y);

		// Try to quickly decide by looking at sign bits.
		if ((node->dy ^ node->dx ^ dx ^ dy) & 0x80000000)
		{
			if ((node->dy ^ dx) & 0x80000000)
			{
				// (left is negative)
				return 1;
			}
			return 0;
		}

		const fixed_t left = (node->dy * dx);
		const fixed_t right = (dy * node->dx);

		if (right < left)
		{
			// front side
			return 0;
		}
		// back side
		return 1;
	}

	return -1;
}


int R_PointOnSegSide (const fixed_t	x, const fixed_t y, seg_t* line )
{
	if (line)
	{
		const fixed_t lx = line->v1->x;
		const fixed_t ly = line->v1->y;

		const fixed_t ldx = line->v2->x - lx;
		const fixed_t ldy = line->v2->y - ly;

		if (!ldx)
		{
			if (x <= lx)
			{
				return ldy > 0;
			}

			return ldy < 0;
		}
		if (!ldy)
		{
			if (y <= ly)
			{
				return ldx < 0;
			}

			return ldx > 0;
		}

		const fixed_t dx = (x - lx);
		const fixed_t dy = (y - ly);

		// Try to quickly decide by looking at sign bits.
		if ((ldy ^ ldx ^ dx ^ dy) & 0x80000000)
		{
			if ((ldy ^ dx) & 0x80000000)
			{
				// (left is negative)
				return 1;
			}
			return 0;
		}

		const fixed_t left = (ldy * dx);
		const fixed_t right = (dy * ldx);

		if (right < left)
		{
			// front side
			return 0;
		}
		// back side
		return 1;
	}

	return - 1;
}


//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table.

//

extern fixed_t GetViewX();
extern fixed_t GetViewY();

angle_t R_PointToAngle( fixed_t x, fixed_t y )
{
	return R_PointToAngle(GetViewX(), GetViewY(),x, y);
}


angle_t R_PointToAngle( const fixed_t x1, const fixed_t y1, const fixed_t x2, const fixed_t y2 )
{	
	fixed_t x = x2 - x1;
	fixed_t y = y2 - y1;

	if ( (!x) && (!y) )
	{
		return 0;
	}

	if (x >= 0)
	{
		// x >=0
		if (y >= 0)
		{
			// y>= 0

			if (x > y)
			{
				// octant 0
				return tantoangle[ SlopeDiv(y,x) ];
			}
			else
			{
				// octant 1
				return ANG90 - 1 - tantoangle[ SlopeDiv(x,y) ];
			}
		}
		else
		{
			// y<0
			y = -y;

			if (x>y)
			{
				// octant 8
				return -tantoangle[ SlopeDiv(y,x) ]; // // ALANHACK UNSIGNED
			}
			else
			{
				// octant 7
				return ANG270 + tantoangle[ SlopeDiv(x,y) ];
			}
		}
	}
	else
	{
		// x<0
		x = -x;

		if (y>= 0)
		{
			// y>= 0
			if (x > y)
			{
				// octant 3
				return ANG180 - 1 - tantoangle[ SlopeDiv(y,x) ];
			}
			else
			{
				// octant 2
				return ANG90 + tantoangle[ SlopeDiv(x,y) ];
			}
		}
		else
		{
			// y<0
			y = -y;

			if (x > y)
			{
				// octant 4
				return ANG180 + tantoangle[ SlopeDiv(y,x) ];
			}
			else
			{
				// octant 5
				return ANG270 - 1 - tantoangle[ SlopeDiv(x,y) ];
			}
		}
	}
	return 0;
}

fixed_t R_PointToDist (const fixed_t x, const fixed_t y )
{
	fixed_t dx = fixed_t::abs(x - GetViewX());
	fixed_t dy = fixed_t::abs(y - GetViewY());

	if (dy > dx)
	{
		const fixed_t temp = dx;
		dx = dy;
		dy = temp;
	}

	const index_t angle = (tantoangle[(dy / dx)] + ANG90);

	// use as cosine
	const fixed_t dist = (dx / finesine[angle]);	

	return dist;
}




//
// R_InitPointToAngle
//
static void R_InitPointToAngle ()
{
	// UNUSED - now getting from tables.c
#if 0
	int	i;
	long	t;
	float	f;
	//
	// slope (tangent) to angle lookup
	//
	for (i=0 ; i<=SLOPERANGE ; i++)
	{
		f = atan( (float)i/SLOPERANGE )/(3.141592657*2);
		t = 0xffffffff*f;
		tantoangle[i] = t;
	}
#endif
}


//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// ::g->rw_distance must be calculated first.
//
extern angle_t GetViewAngle();
fixed_t R_ScaleFromGlobalAngle (const angle_t visangle)
{
	fixed_t		scale = 0;
	angle_t		anglea = 0;
	angle_t		angleb = 0;
	int			sinea = 0;
	int			sineb = 0;
	fixed_t		num = 0;
	int			den = 1;

	// UNUSED
#if 0
	{
		fixed_t		dist;
		fixed_t		z;
		fixed_t		sinv;
		fixed_t		cosv;

		sinv = finesine[(visangle-::g->rw_normalangle)>>ANGLETOFINESHIFT];	
		dist = FixedDiv (::g->rw_distance, sinv);
		cosv = finecosine[(::g->viewangle-visangle)>>ANGLETOFINESHIFT];
		z = abs(FixedMul (dist, cosv));
		scale = FixedDiv(::g->projection, z);
		return scale;
	}
#endif

	anglea = ANG90 + (visangle - GetViewAngle());
	angleb = ANG90 + (visangle - ::g->rw_normalangle);

	// both sines are always positive
	sinea = finesine[anglea];	
	sineb = finesine[angleb];
	num = (::g->projection * sineb) << ::g->detailshift;
	den = (::g->rw_distance * sinea);

	// DHM - Nerve :: If the den is pretty much 0, don't try the divide
	if (den>>8 > 0 && den > num>>16)
	{
		scale = (num / den);

		if (scale > 64*FRACUNIT)
		{
			scale = 64*FRACUNIT;
		}
		else if (scale < 256)
		{
			scale = 256;
		}
	}
	else
	{
		scale = 64*FRACUNIT;
	}

	return scale;
}



//
// R_InitTables
//
static void R_InitTables ()
{
	// UNUSED: now getting from tables.c
#if 0
	int		i;
	float	a;
	float	fv;
	int		t;

	// ::g->viewangle tangent table
	for (i=0 ; i<FINEANGLES/2 ; i++)
	{
		a = (i-FINEANGLES/4+0.5)*PI*2/FINEANGLES;
		fv = FRACUNIT*tan (a);
		t = fv;
		finetangent[i] = t;
	}

	// finesine table
	for (i=0 ; i<5*FINEANGLES/4 ; i++)
	{
		// OPTIMIZE: mirror...
		a = (i+0.5)*PI*2/FINEANGLES;
		t = FRACUNIT*sin (a);
		finesine[i] = t;
	}
#endif

}



//
// R_InitTextureMapping
//
static void R_InitTextureMapping ()
{
	size_t		i = 0;
	int32		t = 0;

	// Use tangent table to generate viewangletox:
	//  ::g->viewangletox will give the next greatest x
	//  after the view angle.
	//
	// Calc focallength
	//  so FIELDOFVIEW angles covers SCREENWIDTH.
	fixed_t focallength = ::g->centerxfrac / finetangent[FINEANGLES / 4 + FIELDOFVIEW / 2];

	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (finetangent[i] > FRACUNIT*2)
		{
			t = -1;
		}
		else if (finetangent[i] < -FRACUNIT*2)
		{
			t = ::g->viewwidth+1;
		}
		else
		{
			t = (finetangent[i] * focallength);
			t = (::g->centerxfrac - t + FRACUNIT - 1 );

			if (t < -1)
			{
				t = -1;
			}
			else if (std::cmp_greater(t, ::g->viewwidth + 1))
			{
				t = numeric_cast<BASE_TYPE(t)>(::g->viewwidth) + 1;
			}
		}
		::g->viewangletox[i] = t;
	}

	// Scan ::g->viewangletox[] to generate ::g->xtoviewangle[]:
	//  ::g->xtoviewangle will give the smallest view angle
	//  that maps to x.	
	for (size_t x = 0; x <= ::g->viewwidth; ++x)
	{
		i = 0;
		while (std::cmp_greater(::g->viewangletox[i], x))
		{
			++i;
		}
		::g->xtoviewangle[x] = numeric_cast<angle_t>((i << ANGLETOFINESHIFT) - ANG90);
	}

	// Take out the fencepost cases from ::g->viewangletox.
	for (i = 0; i < FINEANGLES/2 ; ++i)
	{
		t = (finetangent[i] * focallength);
		t = ::g->centerx - t;

		if (::g->viewangletox[i] == -1)
		{
			::g->viewangletox[i] = 0;
		}
		else if (::g->viewangletox[i] == ::g->viewwidth+1)
		{
			::g->viewangletox[i]  = ::g->viewwidth;
		}
	}

	::g->clipangle = ::g->xtoviewangle[0];
}



//
// R_InitLightTables
// Only inits the ::g->zlight table,
//  because the ::g->scalelight table changes with view size.
//

static void R_InitLightTables ()
{
	size_t	i = 0;
	size_t	j = 0;
	index_t	level = 0;
	size_t	nocollide_startmap = 0;
	fixed_t		scale = 0;

	// Calculate the light levels to use
	//  for each level / distance combination.
	for (i = 0; i < LIGHTLEVELS; i++)
	{
		nocollide_startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTZ; j++)
		{
			scale = ((SCREENWIDTH/2*FRACUNIT) / ((j + 1) << LIGHTZSHIFT));
			scale >>= LIGHTSCALESHIFT;
			level = (nocollide_startmap - scale/DISTMAP);

			level = Max(level, 0);

			if (std::cmp_greater_equal(level, NUMCOLORMAPS))
			{
				level = NUMCOLORMAPS-1;
			}

			::g->zlight[i][j] = ::g->colormaps + level*256;
		}
	}
}



//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//


void
R_SetViewSize
(const size_t	blocks,
 const int		detail )
{
	::g->setsizeneeded = true;
	::g->setblocks = blocks;
	::g->setdetail = detail;
}


//
// R_ExecuteSetViewSize
//
static void R_ExecuteSetViewSize ()
{
	fixed_t	cosadj = 0;
	fixed_t	dy = 0;
	size_t	i = 0;
	size_t	j = 0;

	::g->setsizeneeded = false;

	if (::g->setblocks == 11)
	{
		::g->scaledviewwidth = ORIGINAL_WIDTH;
		::g->viewheight = ORIGINAL_HEIGHT;
	}
	else
	{
		::g->scaledviewwidth = ::g->setblocks*32;
		::g->viewheight = (::g->setblocks*168/10)&~7;
	}

	// SMF - temp
	::g->scaledviewwidth *= GLOBAL_IMAGE_SCALER;
	::g->viewheight *= GLOBAL_IMAGE_SCALER;

	::g->detailshift = ::g->setdetail;
	::g->viewwidth = ::g->scaledviewwidth>>::g->detailshift;

	::g->centery = ::g->viewheight/2;
	::g->centerx = ::g->viewwidth/2;
	::g->centerxfrac = ::g->centerx<<FRACBITS;
	::g->centeryfrac = ::g->centery<<FRACBITS;
	::g->projection = ::g->centerxfrac;

	if (!::g->detailshift)
	{
		colfunc = basecolfunc = R_DrawColumn;
		fuzzcolfunc = R_DrawFuzzColumn;
		transcolfunc = R_DrawTranslatedColumn;
		spanfunc = R_DrawSpan;
	}
	else
	{
		colfunc = basecolfunc = R_DrawColumnLow;
		fuzzcolfunc = R_DrawFuzzColumn;
		transcolfunc = R_DrawTranslatedColumn;
		spanfunc = R_DrawSpanLow;
	}

	R_InitBuffer (::g->scaledviewwidth, ::g->viewheight);

	R_InitTextureMapping ();

	// psprite scales
	::g->pspritescale = FRACUNIT*::g->viewwidth/ORIGINAL_WIDTH;
	::g->pspriteiscale = FRACUNIT*ORIGINAL_WIDTH/::g->viewwidth;

	// thing clipping
	for (i = 0 ; std::cmp_less(i, ::g->viewwidth); i++)
	{
		::g->screenheightarray[i] = ::g->viewheight;
	}

	// planes
	for (i=0 ; i < ::g->viewheight ; i++)
	{
		dy = ((i-::g->viewheight/2)<<FRACBITS)+FRACUNIT/2;
		dy = abs(dy);
		::g->yslope[i] = ( (::g->viewwidth << ::g->detailshift)/2*FRACUNIT / dy);
	}

	for (i=0 ; i < ::g->viewwidth ; i++)
	{
		cosadj = abs(finecosine[::g->xtoviewangle[i]>>ANGLETOFINESHIFT]);
		::g->distscale[i] = (FRACUNIT / cosadj);
	}

	// Calculate the light levels to use
	//  for each level / scale combination.
	for (i=0 ; i< LIGHTLEVELS ; i++)
	{
		const size_t nocollide_startmap = ((LIGHTLEVELS - 1 - i) * 2) * NUMCOLORMAPS / LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTSCALE; j++)
		{
			index_t level = nocollide_startmap - j * SCREENWIDTH / (::g->viewwidth << ::g->detailshift) / DISTMAP;

			level = Max(level, 0);

			if (std::cmp_greater_equal(level, NUMCOLORMAPS))
			{
				level = NUMCOLORMAPS - 1;
			}

			::g->scalelight[i][j] = ::g->colormaps + level*256;
		}
	}
}



//
// R_Init
//



void R_Init ()
{
	R_InitData ();
	I_Printf ("\nR_InitData");
	R_InitPointToAngle ();
	I_Printf ("\nR_InitPointToAngle");
	R_InitTables ();
	// ::g->viewwidth / ::g->viewheight / ::g->detailLevel are set by the defaults
	I_Printf ("\nR_InitTables");

	R_SetViewSize (::g->screenblocks, ::g->detailLevel);
	R_InitPlanes ();
	I_Printf ("\nR_InitPlanes");
	R_InitLightTables ();
	I_Printf ("\nR_InitLightTables");
	R_InitSkyMap ();
	I_Printf ("\nR_InitSkyMap");
	R_InitTranslationTables ();
	I_Printf ("\nR_InitTranslationsTables");

	::g->framecount = 0;
}


//
// R_PointInSubsector
//
subsector_t* R_PointInSubsector ( const fixed_t x, const fixed_t y )
{
	// single subsector is a special case
	if (!::g->numnodes)
	{
		return ::g->subsectors;
	}

	index_t nodenum = numeric_cast<index_t>(::g->numnodes) - 1;

	while (! (nodenum & NF_SUBSECTOR) )
	{
		const node_t* node = &::g->nodes[nodenum];

		if (node)
		{
			const int8 side = R_PointOnSide(x, y, node);

			if (side >= 0)
			{
				nodenum = node->children[side];
			}
		}
	}

	if (nodenum >= 0)
	{
		return &::g->subsectors[nodenum & ~NF_SUBSECTOR];
	}

	return nullptr;
}

//
// R_SetupFrame
//

extern void SetViewX(fixed_t);
extern void SetViewY(fixed_t);
extern void SetViewAngle(angle_t);
extern angle_t GetViewAngle();

static void R_SetupFrame (player_t* player)
{
	::g->viewplayer = player;
	
	SetViewX( player->mo->x );
	SetViewY( player->mo->y );
	SetViewAngle( player->mo->angle + ::g->viewangleoffset );
	::g->extralight = player->extralight;

	::g->viewz = player->viewz;

	::g->viewsin = finesine[GetViewAngle()>>ANGLETOFINESHIFT];
	::g->viewcos = finecosine[GetViewAngle()>>ANGLETOFINESHIFT];

	::g->sscount = 0;

	if (player->fixedcolormap)
	{
		::g->fixedcolormap = ::g->colormaps	+ (player->fixedcolormap * 256 * sizeof(lighttable_t));

		::g->walllights = ::g->scalelightfixed;

		for (auto& i : ::g->scalelightfixed)
		{
			i = ::g->fixedcolormap;
		}
	}
	else
	{
		::g->fixedcolormap = nullptr;
	}

	::g->framecount++;
	::g->validcount++;
}



//
// R_RenderView
//
void R_RenderPlayerView (player_t* player)
{
	if ( player->mo == nullptr) {
		return;
	}

	R_SetupFrame (player);

	// Clear buffers.
	R_ClearClipSegs ();
	R_ClearDrawSegs ();
	R_ClearPlanes ();
	R_ClearSprites ();

	// check for new console commands.
	NetUpdate (nullptr);

	// The head node is the last node output.
	R_RenderBSPNode (::g->numnodes-1);

	// Check for new console commands.
	NetUpdate (nullptr);

	R_DrawPlanes ();

	// Check for new console commands.
	NetUpdate (nullptr);

	R_DrawMasked ();

	// Check for new console commands.
	NetUpdate (nullptr);				
}

