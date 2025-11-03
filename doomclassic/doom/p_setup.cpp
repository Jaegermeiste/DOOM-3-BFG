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


#include <cmath>

#include <utility>

#include "z_zone.h"

#include "m_swap.h"
#include "m_bbox.h"

#include "g_game.h"

#include "i_system.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "doomstat.h"


void	P_SpawnMapThing (mapthing_t*	mthing);


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//








// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
// offsets in ::g->blockmap are from here
// origin of block map
// for thing chains


// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//


// Maintain single and multi player starting spots.






//
// P_LoadVertexes
//
static void P_LoadVertexes (const index_t lump)
{
	// Determine number of lumps:
	//  total lump length / vertex record length.
	::g->numvertexes = W_LumpLength (lump) / sizeof(mapvertex_t);

	// Allocate zone memory for buffer.
//	::g->vertexes = (vertex_t*)Z_Malloc (::g->numvertexes*sizeof(vertex_t),PU_LEVEL,0);	
	if (MallocForLump( lump, ::g->numvertexes*sizeof(vertex_t ), ::g->vertexes, PU_LEVEL_SHARED ))
	{
		// Load data into cache.
		byte* data = static_cast<byte*>(W_CacheLumpNum(lump, PU_CACHE_SHARED)); // ALAN: LOADTIME

		mapvertex_t* ml = reinterpret_cast<mapvertex_t*>(data);
		vertex_t* li = ::g->vertexes;

		// Copy and convert vertex coordinates,
		// internal representation as fixed.
		for (size_t i = 0 ; i < ::g->numvertexes ; i++, li++, ml++)
		{
			li->x = ml->x;
			li->y = ml->y;
		}

		// Free buffer memory.
		Z_Free(data);
	}
}



//
// P_LoadSegs
//
static void P_LoadSegs (const index_t lump)
{
	::g->numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
//	::g->segs = (seg_t*)Z_Malloc (::g->numsegs*sizeof(seg_t),PU_LEVEL,0);	

	if (MallocForLump( lump, ::g->numsegs*sizeof(seg_t), ::g->segs, PU_LEVEL_SHARED ))
	{
		memset (::g->segs, 0, ::g->numsegs*sizeof(seg_t));
		byte* data = static_cast<byte*>(W_CacheLumpNum(lump, PU_CACHE_SHARED)); // ALAN: LOADTIME

		mapseg_t* ml = reinterpret_cast<mapseg_t*>(data);
		seg_t* li = ::g->segs;
		for (size_t i = 0 ; i < ::g->numsegs ; i++, li++, ml++)
		{
			li->v1 = &::g->vertexes[SHORT(ml->v1)];
			li->v2 = &::g->vertexes[SHORT(ml->v2)];

			li->angle = (SHORT(ml->angle))<<16;
			li->offset = (SHORT(ml->offset))<<16;
			int16 psetup_linedef = SHORT(ml->linedef);
			line_t* ldef = &::g->lines[psetup_linedef];
			li->linedef = ldef;
			int16 side = SHORT(ml->side);
			li->sidedef = &::g->sides[ldef->sidenum[side]];
			li->frontsector = ::g->sides[ldef->sidenum[side]].sector;
			if (ldef-> flags & ML_TWOSIDED)
			{
				li->backsector = ::g->sides[ldef->sidenum[side^1]].sector;
			}
			else
			{
				li->backsector = nullptr;
			}
		}

		Z_Free(data);
	}
}


//
// P_LoadSubsectors
//
static void P_LoadSubsectors (const index_t lump)
{
	::g->numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);

	if (MallocForLump( lump, ::g->numsubsectors*sizeof(subsector_t), ::g->subsectors, PU_LEVEL_SHARED ))
	{
		byte* data = static_cast<byte*>(W_CacheLumpNum(lump, PU_CACHE_SHARED)); // ALAN: LOADTIME

		mapsubsector_t* ms = reinterpret_cast<mapsubsector_t*>(data);
		memset (::g->subsectors,0, ::g->numsubsectors*sizeof(subsector_t));
		subsector_t* ss = ::g->subsectors;

		for (size_t i = 0 ; i < ::g->numsubsectors ; i++, ss++, ms++)
		{
			ss->numlines = ms->numsegs;
			ss->firstline = ms->firstseg;
		}

		Z_Free(data);
	}
}



//
// P_LoadSectors
//
static void P_LoadSectors (const index_t lump)
{
	::g->numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
	
	::g->sectors = static_cast<sector_t*>(Z_Malloc(::g->numsectors * sizeof(sector_t), PU_LEVEL, nullptr));
	memset (::g->sectors, 0, ::g->numsectors*sizeof(sector_t));
	byte* data = static_cast<byte*>(W_CacheLumpNum(lump, PU_CACHE_SHARED)); // ALAN: LOADTIME

	mapsector_t* ms = reinterpret_cast<mapsector_t*>(data);
	sector_t* ss = ::g->sectors;
	for (size_t i = 0; i < ::g->numsectors; i++, ss++, ms++)
	{
		ss->floorheight = SHORT(ms->floorheight);
		ss->ceilingheight = SHORT(ms->ceilingheight);
		ss->floorpic = R_FlatNumForName(ms->floorpic);
		ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		ss->tag = SHORT(ms->tag);
		ss->thinglist = nullptr;
	}

	Z_Free(data);

/*
	if (MallocForLump( lump, ::g->numsectors*sizeof(sector_t), (void**)&::g->sectors, PU_LEVEL_SHARED ))
	{
		memset (::g->sectors, 0, ::g->numsectors*sizeof(sector_t));
		data = (byte*)W_CacheLumpNum (lump,PU_CACHE_SHARED); // ALAN: LOADTIME

		ms = (mapsector_t *)data;
		ss = ::g->sectors;
		for (i=0 ; i < ::g->numsectors ; i++, ss++, ms++)
		{
			ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
			ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
			ss->floorpic = R_FlatNumForName(ms->floorpic);
			ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
			ss->lightlevel = SHORT(ms->lightlevel);
			ss->special = SHORT(ms->special);
			ss->tag = SHORT(ms->tag);
			ss->thinglist = NULL;
		}

		DoomLib::Z_Free(data);
	}
*/	
}


//
// P_LoadNodes
//
static void P_LoadNodes (const index_t lump)
{
	::g->numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
	if (MallocForLump( lump, ::g->numnodes*sizeof(node_t), ::g->nodes, PU_LEVEL_SHARED ))
	{
		byte* data = static_cast<byte*>(W_CacheLumpNum(lump, PU_CACHE_SHARED)); // ALAN: LOADTIME

		mapnode_t* mn = reinterpret_cast<mapnode_t*>(data);
		node_t* no = ::g->nodes;

		for (size_t i = 0; i < ::g->numnodes; ++i, ++no, ++mn)
		{
			no->x = SHORT(mn->x);
			no->y = SHORT(mn->y);
			no->dx = SHORT(mn->dx);
			no->dy = SHORT(mn->dy);
			for (size_t j = 0 ; j < 2; ++j)
			{
				no->children[j] = SHORT(mn->children[j]);
				for (size_t k = 0 ; k<4 ; k++)
				{
					no->bbox[j][k] = SHORT(mn->bbox[j][k]);
				}
			}
		}

		Z_Free(data);
	}
}


//
// P_LoadThings
//
static void P_LoadThings (const index_t lump)
{
	byte* data = static_cast<byte*>(W_CacheLumpNum(lump, PU_CACHE_SHARED)); // ALAN: LOADTIME
	size_t numthings = (W_LumpLength(lump) / sizeof(mapthing_t));

	mapthing_t* mt = reinterpret_cast<mapthing_t*>(data);
	for (size_t i = 0; i < numthings; ++i, ++mt)
	{
		bool spawn = true;

		// Do not spawn cool, new monsters if !commercial
		if ( ::g->gamemode != commercial)
		{
			switch(mt->type)
			{
			case 68:	// Arachnotron
			case 64:	// Archvile
			case 88:	// Boss Brain
			case 89:	// Boss Shooter
			case 69:	// Hell Knight
			case 67:	// Mancubus
			case 71:	// Pain Elemental
			case 65:	// Former Human Commando
			case 66:	// Revenant
			case 84:	// Wolf SS
				spawn = false;
				continue;                     // FIX: https://doomwiki.org/wiki/Doom_II_monster_exclusion_bug
			default:
				break;
			}
		}
		if (spawn == false)
		{
			break;
		}

		// Do spawn all other stuff. 
		mt->x = SHORT(mt->x);
		mt->y = SHORT(mt->y);
		mt->angle = SHORT(mt->angle);
		mt->type = SHORT(mt->type);
		mt->options = SHORT(mt->options);

		P_SpawnMapThing (mt);
	}

	Z_Free(data);
}


//
// P_LoadLineDefs
// Also counts secret ::g->lines for intermissions.
//
static void P_LoadLineDefs (const index_t lump)
{
	::g->numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
	if (MallocForLump( lump, ::g->numlines*sizeof(line_t), ::g->lines, PU_LEVEL_SHARED ))
	{
		memset (::g->lines, 0, ::g->numlines*sizeof(line_t));
		byte* data = static_cast<byte*>(W_CacheLumpNum(lump, PU_CACHE_SHARED)); // ALAN: LOADTIME

		maplinedef_t* mld = reinterpret_cast<maplinedef_t*>(data);
		line_t* ld = ::g->lines;
		for (size_t i = 0; i < ::g->numlines; ++i, ++mld, ++ld)
		{
			ld->flags = SHORT(mld->flags);
			ld->special = SHORT(mld->special);
			ld->tag = SHORT(mld->tag);
			vertex_t* v1 = ld->v1 = &::g->vertexes[SHORT(mld->v1)];
			vertex_t* v2 = ld->v2 = &::g->vertexes[SHORT(mld->v2)];
			ld->dx = v2->x - v1->x;
			ld->dy = v2->y - v1->y;

			if (!ld->dx)
			{
				ld->slopetype = ST_VERTICAL;
			}
			else if (!ld->dy)
			{
				ld->slopetype = ST_HORIZONTAL;
			}
			else
			{
				if ((ld->dy / ld->dx) > 0)
				{
					ld->slopetype = ST_POSITIVE;
				}
				else
				{
					ld->slopetype = ST_NEGATIVE;
				}
			}

			if (v1->x < v2->x)
			{
				ld->bbox[BOXLEFT] = v1->x;
				ld->bbox[BOXRIGHT] = v2->x;
			}
			else
			{
				ld->bbox[BOXLEFT] = v2->x;
				ld->bbox[BOXRIGHT] = v1->x;
			}

			if (v1->y < v2->y)
			{
				ld->bbox[BOXBOTTOM] = v1->y;
				ld->bbox[BOXTOP] = v2->y;
			}
			else
			{
				ld->bbox[BOXBOTTOM] = v2->y;
				ld->bbox[BOXTOP] = v1->y;
			}

			ld->sidenum[0] = SHORT(mld->sidenum[0]);
			ld->sidenum[1] = SHORT(mld->sidenum[1]);

			if (ld->sidenum[0] != -1)
			{
				ld->frontsector = ::g->sides[ld->sidenum[0]].sector;
			}
			else
			{
				ld->frontsector = nullptr;
			}

			if (ld->sidenum[1] != -1)
			{
				ld->backsector = ::g->sides[ld->sidenum[1]].sector;
			}
			else
			{
				ld->backsector = nullptr;
			}
		}

		Z_Free(data);
	}
}


//
// P_LoadSideDefs
//
static void P_LoadSideDefs (const index_t lump)
{
	::g->numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
	if (MallocForLump( lump, ::g->numsides*sizeof(side_t), ::g->sides, PU_LEVEL_SHARED))
	{
		memset (::g->sides, 0, ::g->numsides*sizeof(side_t));
		byte* data = static_cast<byte*>(W_CacheLumpNum(lump,PU_CACHE_SHARED)); // ALAN: LOADTIME

		mapsidedef_t* msd = reinterpret_cast<mapsidedef_t*>(data);
		side_t* sd = ::g->sides;
		for (size_t i = 0 ; i < ::g->numsides ; i++, msd++, sd++)
		{
			sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
			sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;
			sd->toptexture = R_TextureNumForName(msd->toptexture);
			sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
			sd->midtexture = R_TextureNumForName(msd->midtexture);
			sd->sector = &::g->sectors[SHORT(msd->sector)];
		}

		Z_Free(data);
	}
}


//
// P_LoadBlockMap
//
static void P_LoadBlockMap (const index_t lump)
{
	bool firstTime = false;
	if (!lumpcache[lump]) {			// SMF - solution for double endian conversion issue
		firstTime = true;
	}

	::g->blockmaplump = static_cast<BASE_TYPE(::g->blockmaplump)*>(W_CacheLumpNum(lump,PU_LEVEL_SHARED)); // ALAN: This is initialized somewhere else as shared...
	::g->blockmap = ::g->blockmaplump+4;
	size_t count = W_LumpLength(lump) / 2;

	if ( firstTime ) {				// SMF
		for (size_t i = 0 ; i<count ; i++)
		{
			::g->blockmaplump[i] = SHORT(::g->blockmaplump[i]);
		}
	}

	::g->blockmap_origin = ( ::g->blockmaplump[0] )<<FRACBITS;
	::g->bmaporgy = ( ::g->blockmaplump[1] )<<FRACBITS;
	::g->blockmap_width = ( ::g->blockmaplump[2] );
	::g->blockmap_height = ( ::g->blockmaplump[3] );

	// clear out mobj chains
	count = sizeof(*::g->blocklinks)* ::g->blockmap_width*::g->blockmap_height;
	::g->blocklinks = static_cast<mobj_t**>(Z_Malloc(count,PU_LEVEL, nullptr));
	memset (static_cast<void*>(::g->blocklinks), 0, count);
}



//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for ::g->sectors.
//
static void P_GroupLines ()
{
	size_t			i = 0;
	fixed_t		bbox[4] = {};


	// look up sector number for each subsector
	subsector_t* ss = ::g->subsectors;
	for (i=0 ; i < ::g->numsubsectors ; i++, ss++)
	{
		seg_t* seg = &::g->segs[ss->firstline];
		ss->sector = seg->sidedef->sector;
	}

	// count number of ::g->lines in each sector
	line_t* li = ::g->lines;
	size_t total = 0;
	for (i = 0; i < ::g->numlines; ++i, ++li)
	{
		++total;
		++li->frontsector->linecount;

		if (li->backsector && li->backsector != li->frontsector)
		{
			++li->backsector->linecount;
			++total;
		}
	}

	// build line tables for each sector	
	line_t** linebuffer = static_cast<line_t**>(Z_Malloc(total * 4, PU_LEVEL, nullptr));
	sector_t* sector = ::g->sectors;
	for (i = 0; i < ::g->numsectors; ++i, ++sector)
	{
		M_ClearBox (bbox);
		sector->lines = linebuffer;
		li = ::g->lines;
		for (int j = 0 ; std::cmp_less(j, ::g->numlines); j++, li++)
		{
			if (li->frontsector == sector || li->backsector == sector)
			{
				*linebuffer++ = li;
				M_AddToBox (bbox, li->v1->x, li->v1->y);
				M_AddToBox (bbox, li->v2->x, li->v2->y);
			}
		}
		if (std::not_equal_to<>()(linebuffer - sector->lines, sector->linecount))
		{
			I_Error ("P_GroupLines: miscounted");
		}

		// set the degenmobj_t to the middle of the bounding box
		sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
		sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;

		// adjust bounding box to map blocks
		int block = (bbox[BOXTOP] - ::g->bmaporgy + MAXRADIUS) ;
		block = std::cmp_greater_equal(block, ::g->blockmap_height) ? numeric_cast<int>(::g->blockmap_height)-1 : block;
		sector->blockbox[BOXTOP]=block;

		block = (bbox[BOXBOTTOM]-::g->bmaporgy-MAXRADIUS);
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXBOTTOM]=block;

		block = (bbox[BOXRIGHT]-::g->blockmap_origin+MAXRADIUS);
		block = std::cmp_greater_equal(block, ::g->blockmap_width) ? numeric_cast<int>(::g->blockmap_width)-1 : block;
		sector->blockbox[BOXRIGHT]=block;

		block = (bbox[BOXLEFT]-::g->blockmap_origin-MAXRADIUS);
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXLEFT]=block;
	}

}


//
// P_SetupLevel
//
void ST_loadData();
void HU_Init();

static void P_SetupLevel (const index_t episode, const index_t map, int playermask, const skill_t skill)
{
	size_t		i = 0;
	char	lumpname[32] = {};

	::g->totalkills = ::g->totalitems = ::g->totalsecret = ::g->wminfo.maxfrags = 0;
	::g->wminfo.partime = 180;
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		::g->players[i].killcount = ::g->players[i].secretcount 
			= ::g->players[i].itemcount = 0;

		::g->players[i].chainsawKills = 0;
		::g->players[i].berserkKills = 0;
	}

	// Initial height of PointOfView
	// will be set by player think.
	::g->players[::g->consoleplayer].viewz = 1; 

	// Make sure all sounds are stopped before Z_FreeTags.
	S_Start ();			

	Z_FreeTags( PU_LEVEL, PU_PURGELEVEL-1 );

	// UNUSED W_Profile ();
	P_InitThinkers ();

	// if working with a development map, reload it
	// W_Reload ();

	// DHM - NERVE :: Update the cached asset pointers in case the wad files were reloaded
	{
		ST_loadData();

		HU_Init();
	}

	// find map name
	if ( ::g->gamemode == commercial)
	{
		if (map < 10)
		{
			idStr::snPrintf(lumpname, sizeof(lumpname),"map0%i", map);
		}
		else
		{
			idStr::snPrintf(lumpname, sizeof(lumpname), "map%i", map);
		}
	}
	else
	{
		lumpname[0] = 'E';
		lumpname[1] = numeric_cast<char>('0' + episode);
		lumpname[2] = 'M';
		lumpname[3] = numeric_cast<char>('0' + map);
		lumpname[4] = 0;
	}

	index_t lumpnum = W_GetNumForName(lumpname);

	::g->leveltime = 0;

	// note: most of this ordering is important	
	P_LoadBlockMap (lumpnum + ML_BLOCKMAP);
	P_LoadVertexes (lumpnum + ML_VERTEXES);
	P_LoadSectors (lumpnum + ML_SECTORS);
	P_LoadSideDefs (lumpnum + ML_SIDEDEFS);

	P_LoadLineDefs (lumpnum + ML_LINEDEFS);
	P_LoadSubsectors (lumpnum + ML_SSECTORS);
	P_LoadNodes (lumpnum + ML_NODES);
	P_LoadSegs (lumpnum + ML_SEGS);

	::g->rejectmatrix = static_cast<byte*>(W_CacheLumpNum(lumpnum + ML_REJECT,PU_LEVEL));

	P_GroupLines ();

	::g->bodyqueueslot = 0;
	::g->deathmatch_p = ::g->deathmatchstarts;
	P_LoadThings (lumpnum + ML_THINGS);

	// if ::g->deathmatch, randomly spawn the active ::g->players
	if (::g->deathmatch)
	{
		for (i = 0; i < MAXPLAYERS; ++i)
		{
			if (::g->playeringame[i])
			{
				// DHM - Nerve :: In deathmatch, reset every player at match start
				::g->players[i].playerstate = PST_REBORN;

				::g->players[i].mo = nullptr;
				G_DeathMatchSpawnPlayer (numeric_cast<index_t>(i));
			}
		}
	}

	// clear special respawning queue
	::g->iqueuehead = ::g->iqueuetail = 0;		

	// set up world state
	P_SpawnSpecials ();

	// build subsector connect matrix
	//	UNUSED P_ConnectSubsectors ();

	// preload graphics
	if (::g->precache)
	{
		R_PrecacheLevel ();
	}
}



//
// P_Init
//
static void P_Init ()
{
	P_InitSwitchList ();
	P_InitPicAnims ();
	R_InitSprites (sprnames);
}




