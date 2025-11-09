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

#include "i_system.h"
#include "z_zone.h"
#include "p_local.h"

// State.
#include "doomstat.h"
#include "r_state.h"



// Pads ::g->save_p to a 4-byte boundary
//  so that the load/save works on SGI&Gecko.



//
// P_ArchivePlayers
//
static void P_ArchivePlayers ()
{
    int		i;

    for (auto& player : ::g->players)
    {
	if (!player.playerInGame)
	{
		continue;
	}

	PADSAVEP();

	player_t* dest = (player_t*)::g->save_p;
	memcpy (dest,&::g->players[i],sizeof(player_t));
	::g->save_p += sizeof(player_t);
	for (int j = 0 ; j<NUMPSPRITES ; j++)
	{
	    if (dest->psprites[j].state)
	    {
		dest->psprites[j].state 
			= (state_t *)(dest->psprites[j].state-::g->states);
	    }
	}
    }
}



//
// P_UnArchivePlayers
//
static void P_UnArchivePlayers ()
{
    int		i;

    for (auto& player : ::g->players)
    {
	if (!player.playerInGame)
	{
		continue;
	}

	PADSAVEP();

	memcpy (&::g->players[i],::g->save_p, sizeof(player_t));
	::g->save_p += sizeof(player_t);
	
	// will be set when unarc thinker
	::g->players[i].mo = nullptr;	
	::g->players[i].message = nullptr;
	::g->players[i].attacker = nullptr;

	for (int j = 0 ; j<NUMPSPRITES ; j++)
	{
	    if (::g->players[i]. psprites[j].state)
	    {
		::g->players[i]. psprites[j].state 
		    = &::g->states[ (int)::g->players[i].psprites[j].state ];
	    }
	}
    }
}


//
// P_ArchiveWorld
//
static void P_ArchiveWorld ()
{
    int			i;
    sector_t*		sec;
    line_t*		li;

    short* put = (short*)::g->save_p;
    
    // do ::g->sectors
    for (i=0, sec = ::g->sectors ; i < ::g->numsectors ; i++,sec++)
    {
	*put++ = sec->floorheight;
	*put++ = sec->ceilingheight;
	*put++ = sec->floorpic;
	*put++ = sec->ceilingpic;
	*put++ = sec->lightlevel;
	*put++ = sec->special;		// needed?
	*put++ = sec->tag;		// needed?
    }

    
    // do ::g->lines
    for (i=0, li = ::g->lines ; i < ::g->numlines ; i++,li++)
    {
	*put++ = li->flags;
	*put++ = li->special;
	*put++ = li->tag;
	for (int j = 0 ; j<2 ; j++)
	{
	    if (li->sidenum[j] == -1)
	    {
		    continue;
	    }

	    side_t* si = &::g->sides[li->sidenum[j]];

	    *put++ = si->textureoffset;
	    *put++ = si->rowoffset;
	    *put++ = si->toptexture;
	    *put++ = si->bottomtexture;
	    *put++ = si->midtexture;	
	}
    }

	// Doom 2 level 30 requires some global pointers, wheee!
	*put++ = ::g->braintargeton;
	*put++ = ::g->easy;

    ::g->save_p = reinterpret_cast<byte*>(put);
}



//
// P_UnArchiveWorld
//
static void P_UnArchiveWorld ()
{
    int			i;
    sector_t*	sec;
    line_t*		li;

    short* get = reinterpret_cast<short*>(::g->save_p);
    
    // do ::g->sectors
    for (i=0, sec = ::g->sectors ; i < ::g->numsectors ; i++,sec++)
    {
	sec->floorheight = *get++;
	sec->ceilingheight = *get++;
	sec->floorpic = *get++;
	sec->ceilingpic = *get++;
	sec->lightlevel = *get++;
	sec->special = *get++;		// needed?
	sec->tag = *get++;		// needed?
	sec->specialdata = nullptr;
	sec->soundtarget = nullptr;
    }
    
    // do ::g->lines
    for (i=0, li = ::g->lines ; i < ::g->numlines ; i++,li++)
    {
	li->flags = *get++;
	li->special = *get++;
	li->tag = *get++;
	for (int j = 0 ; j<2 ; j++)
	{
	    if (li->sidenum[j] == -1)
	    {
		    continue;
	    }
	    side_t* si = &::g->sides[li->sidenum[j]];
	    si->textureoffset = *get++;
	    si->rowoffset = *get++;
	    si->toptexture = *get++;
	    si->bottomtexture = *get++;
	    si->midtexture = *get++;
	}
    }

	// Doom 2 level 30 requires some global pointers, wheee!
	::g->braintargeton = *get++;
	::g->easy = *get++;

    ::g->save_p = (byte *)get;	
}





//
// Thinkers
//

static int GetMOIndex( mobj_t* findme ) {
	int			index = 0;

	for (thinker_t* th = ::g->thinkercap.next ; th != &::g->thinkercap ; th=th->next)
	{
		if (th->function.acp1 == reinterpret_cast<actionf_p1>(P_MobjThinker)) {
			index++;
			mobj_t* mobj = (mobj_t*)th;

			if ( mobj == findme ) {
				return index;
			}
		}
	}

	return 0;
}

static mobj_t* GetMO(const index_t index ) {
	int			testindex = 0;

	if ( !index ) {
		return nullptr;
	}

	for (thinker_t* th = ::g->thinkercap.next ; th != &::g->thinkercap ; th=th->next)
	{
		if (th->function.acp1 == reinterpret_cast<actionf_p1>(P_MobjThinker)) {
			testindex++;

			if ( testindex == index ) {
				return (mobj_t*)th;
			}
		}
	}

	return nullptr;
}

//
// P_ArchiveThinkers
//
static void P_ArchiveThinkers ()
{
	ceiling_t*		ceiling;

	int i;
	
	// save off the current thinkers
	//I_Printf( "Savegame on leveltime %d\n====================\n", ::g->leveltime );

	for (thinker_t* th = ::g->thinkercap.next ; th != &::g->thinkercap ; th=th->next)
	{
		//mobj_t*	test = (mobj_t*)th;
		//I_Printf( "%3d: %x == function\n", index++, th->function.acp1 );

		if (th->function.acp1 == reinterpret_cast<actionf_p1>(P_MobjThinker))
		{
			*::g->save_p++ = tc_mobj;
			PADSAVEP();

			mobj_t* mobj = (mobj_t*)::g->save_p;
			memcpy (mobj, th, sizeof(*mobj));
			::g->save_p += sizeof(*mobj);
			mobj->state = (state_t *)(mobj->state - ::g->states);

			if (mobj->player)
			{
				mobj->player = static_cast<player_t*>((mobj->player - ::g->players) + 1);
			}

			// Save out 'target'
			int moIndex = GetMOIndex( mobj->target );
			*::g->save_p++ = moIndex >> 8;
			*::g->save_p++ = moIndex;

			// Save out 'tracer'
			moIndex = GetMOIndex( mobj->tracer );
			*::g->save_p++ = moIndex >> 8;
			*::g->save_p++ = moIndex;

			moIndex = GetMOIndex( mobj->snext );
			*::g->save_p++ = moIndex >> 8;
			*::g->save_p++ = moIndex;

			moIndex = GetMOIndex( mobj->sprev );
			*::g->save_p++ = moIndex >> 8;
			*::g->save_p++ = moIndex;

			// Is this the head of a sector list?
			if ( mobj->subsector->sector->thinglist == reinterpret_cast<mobj_t*>(th) ) {
				*::g->save_p++ = 1;
			}
			else {
				*::g->save_p++ = 0;
			}

			moIndex = GetMOIndex( mobj->bnext );
			*::g->save_p++ = moIndex >> 8;
			*::g->save_p++ = moIndex;

			moIndex = GetMOIndex( mobj->bprev );
			*::g->save_p++ = moIndex >> 8;
			*::g->save_p++ = moIndex;

			// Is this the head of a block list?
			const auto blockx = (mobj->x - ::g->blockmap_origin.x);
			const auto blocky = (mobj->y - ::g->blockmap_origin.y);
			if ( blockx >= 0 && blockx < ::g->blockmap_width && blocky >= 0 && blocky < ::g->blockmap_height 
				&& reinterpret_cast<mobj_t*>(th) == ::g->blocklinks[blocky*::g->blockmap_width+blockx] ) {

					*::g->save_p++ = 1;
			}
			else {
				*::g->save_p++ = 0;
			}
			continue;
		}

		if (th->function.acv == static_cast<actionf_v>(nullptr))
		{
			/*for (i = 0; i < MAXCEILINGS;i++)
			{
				if (::g->activeceilings[i] == reinterpret_cast<ceiling_t*>(th))
				{
					break;
				}
			}*/

			const index_t idx = ::g->activeceilings.FindIndex(reinterpret_cast<ceiling_t*>(th));

			if (idx >= 0)
			{
				*::g->save_p++ = tc_ceiling;
				PADSAVEP();
				ceiling = reinterpret_cast<ceiling_t*>(::g->save_p);
				memcpy (ceiling, th, sizeof(*ceiling));
				::g->save_p += sizeof(*ceiling);
				ceiling->sector = reinterpret_cast<sector_t*>(ceiling->sector - ::g->sectors);
			}
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_MoveCeiling)
		{
			*::g->save_p++ = tc_ceiling;
			PADSAVEP();
			ceiling = (ceiling_t *)::g->save_p;
			memcpy (ceiling, th, sizeof(*ceiling));
			::g->save_p += sizeof(*ceiling);
			ceiling->sector = (sector_t *)(ceiling->sector - ::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_VerticalDoor)
		{
			*::g->save_p++ = tc_door;
			PADSAVEP();
			vldoor_t* door = (vldoor_t*)::g->save_p;
			memcpy (door, th, sizeof(*door));
			::g->save_p += sizeof(*door);
			door->sector = (sector_t *)(door->sector - ::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_MoveFloor)
		{
			*::g->save_p++ = tc_floor;
			PADSAVEP();
			floormove_t* floor = (floormove_t*)::g->save_p;
			memcpy (floor, th, sizeof(*floor));
			::g->save_p += sizeof(*floor);
			floor->sector = (sector_t *)(floor->sector - ::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_PlatRaise)
		{
			*::g->save_p++ = tc_plat;
			PADSAVEP();
			plat_t* plat = (plat_t*)::g->save_p;
			memcpy (plat, th, sizeof(*plat));
			::g->save_p += sizeof(*plat);
			plat->sector = (sector_t *)(plat->sector - ::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_FireFlicker)
		{
			*::g->save_p++ = tc_fire;
			PADSAVEP();
			fireflicker_t* fire = (fireflicker_t*)::g->save_p;
			memcpy (fire, th, sizeof(*fire));
			::g->save_p += sizeof(*fire);
			fire->sector = (sector_t *)(fire->sector - ::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_LightFlash)
		{
			*::g->save_p++ = tc_flash;
			PADSAVEP();
			lightflash_t* flash = (lightflash_t*)::g->save_p;
			memcpy (flash, th, sizeof(*flash));
			::g->save_p += sizeof(*flash);
			flash->sector = (sector_t *)(flash->sector - ::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
		{
			*::g->save_p++ = tc_strobe;
			PADSAVEP();
			strobe_t* strobe = (strobe_t*)::g->save_p;
			memcpy (strobe, th, sizeof(*strobe));
			::g->save_p += sizeof(*strobe);
			strobe->sector = (sector_t *)(strobe->sector - ::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_Glow)
		{
			*::g->save_p++ = tc_glow;
			PADSAVEP();
			glow_t* glow = (glow_t*)::g->save_p;
			memcpy (glow, th, sizeof(*glow));
			::g->save_p += sizeof(*glow);
			glow->sector = (sector_t *)(glow->sector - ::g->sectors);
			continue;
		}
	}

	// add a terminating marker
	*::g->save_p++ = tc_end;

	sector_t* sec;
    short* put = (short *)::g->save_p;
	for (i=0, sec = ::g->sectors ; i < ::g->numsectors ; i++,sec++) {
		*put++ = static_cast<short>(GetMOIndex(sec->soundtarget));
	}

	::g->save_p = (byte *)put;

	// add a terminating marker
	*::g->save_p++ = tc_end;
}



//
// P_UnArchiveThinkers
//
static void P_UnArchiveThinkers ()
{
	mobj_t*			mobj;
	ceiling_t*		ceiling;
	vldoor_t*		door;
	floormove_t*	floor;
	plat_t*			plat;
	fireflicker_t*	fire;
	lightflash_t*	flash;
	strobe_t*		strobe;
	glow_t*			glow;

	thinker_t*	th;

	int count = 0;
	sector_t* ss = nullptr;

	int			mo_index = 0;
	int			mo_targets[1024];
	int			mo_tracers[1024];
	int			mo_snext[1024];
	int			mo_sprev[1024];
	bool		mo_shead[1024];
	int			mo_bnext[1024];
	int			mo_bprev[1024];
	bool		mo_bhead[1024];

	// remove all the current thinkers
	thinker_t* currentthinker = ::g->thinkercap.next;
	while (currentthinker != &::g->thinkercap)
	{
		thinker_t* next = currentthinker->next;

		if (currentthinker->function.acp1 == reinterpret_cast<actionf_p1>(P_MobjThinker))
		{
			P_RemoveMobj ((mobj_t *)currentthinker);
		}
		else
		{
			Z_Free(currentthinker);
		}

		currentthinker = next;
	}

	P_InitThinkers ();

	// read in saved thinkers
	while (true)
	{
		byte tclass = *::g->save_p++;
		switch (tclass)
		{
		case tc_end:

			// clear sector thing lists
			ss = ::g->sectors;
			for (int i=0 ; i < ::g->numsectors ; i++, ss++) {
				ss->thinglist = nullptr;
			}

			// clear blockmap thing lists
			count = sizeof(*::g->blocklinks) * ::g->blockmap_width * ::g->blockmap_height;
			memset (::g->blocklinks, 0, count);

			// Doom 2 level 30 requires some global pointers, wheee!
			::g->numbraintargets = 0;

			// fixup mobj_t pointers now that all thinkers have been restored
			mo_index = 0;
			for (th = ::g->thinkercap.next ; th != &::g->thinkercap ; th=th->next) {
				if (th->function.acp1 == reinterpret_cast<actionf_p1>(P_MobjThinker)) {
					mobj = (mobj_t*)th;

					mobj->target = GetMO( mo_targets[mo_index] );
					mobj->tracer = GetMO( mo_tracers[mo_index] );

					mobj->snext = GetMO( mo_snext[mo_index] );
					mobj->sprev = GetMO( mo_sprev[mo_index] );

					if ( mo_shead[mo_index] ) {
						mobj->subsector->sector->thinglist = mobj;
					}

					mobj->bnext = GetMO( mo_bnext[mo_index] );
					mobj->bprev = GetMO( mo_bprev[mo_index] );

					if ( mo_bhead[mo_index] ) {
						// Is this the head of a block list?
						const int	blockx = (mobj->x - ::g->blockmap_origin);
						const int	blocky = (mobj->y - ::g->bmaporgy);
						if ( blockx >= 0 && blockx < ::g->blockmap_width && blocky >= 0 && blocky < ::g->blockmap_height ) {
							::g->blocklinks[blocky*::g->blockmap_width+blockx] = mobj;
						}
					}

					// Doom 2 level 30 requires some global pointers, wheee!
					if ( mobj->type == MT_BOSSTARGET ) {
						::g->braintargets[::g->numbraintargets] = mobj;
						::g->numbraintargets++;
					}

					mo_index++;
				}
			}

			int i;
			sector_t*	sec;
		    short*	get;

			get = (short *)::g->save_p;
			for (i=0, sec = ::g->sectors ; i < ::g->numsectors ; i++,sec++)
			{
				sec->soundtarget = GetMO( *get++ );
			}
			::g->save_p = (byte *)get;

			tclass = *::g->save_p++;
			if ( tclass != tc_end ) {
				I_Error( "Savegame error after loading sector soundtargets." );
			}

			// print the current thinkers
			//I_Printf( "Loadgame on leveltime %d\n====================\n", ::g->leveltime );
			for (th = ::g->thinkercap.next ; th != &::g->thinkercap ; th=th->next)
			{
				//mobj_t*	test = (mobj_t*)th;
				//I_Printf( "%3d: %x == function\n", index++, th->function.acp1 );
			}

			return; 	// end of list

		case tc_mobj:
			PADSAVEP();
			mobj = static_cast<mobj_t*>(DoomLib::Z_Malloc(sizeof(*mobj), PU_LEVEL, nullptr));
			memcpy (mobj, ::g->save_p, sizeof(*mobj));
			::g->save_p += sizeof(*mobj);
			mobj->state = &::g->states[(int)mobj->state];

			mobj->target = nullptr;
			mobj->tracer = nullptr;

			if (mobj->player)
			{
				mobj->player = &::g->players[(int)mobj->player-1];
				mobj->player->mo = mobj;
			}

			P_SetThingPosition (mobj);

			mobj->info = &mobjinfo[mobj->type];
			mobj->floorz = mobj->subsector->sector->floorheight;
			mobj->ceilingz = mobj->subsector->sector->ceilingheight;
			mobj->thinker.function.acp1 = reinterpret_cast<actionf_p1>(P_MobjThinker);

			// Read in 'target' and store for fixup
			int a, b, foundIndex;
			a = *::g->save_p++;
			b = *::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_targets[mo_index] = foundIndex;

			// Read in 'tracer' and store for fixup
			a = *::g->save_p++;
			b = *::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_tracers[mo_index] = foundIndex;

			// Read in 'snext' and store for fixup
			a = *::g->save_p++;
			b = *::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_snext[mo_index] = foundIndex;

			// Read in 'sprev' and store for fixup
			a = *::g->save_p++;
			b = *::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_sprev[mo_index] = foundIndex;

			foundIndex = *::g->save_p++;
			mo_shead[mo_index] = foundIndex == 1;

			// Read in 'bnext' and store for fixup
			a = *::g->save_p++;
			b = *::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_bnext[mo_index] = foundIndex;

			// Read in 'bprev' and store for fixup
			a = *::g->save_p++;
			b = *::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_bprev[mo_index] = foundIndex;

			foundIndex = *::g->save_p++;
			mo_bhead[mo_index] = foundIndex == 1;

			mo_index++;

			P_AddThinker (&mobj->thinker);
			break;

		case tc_ceiling:
			PADSAVEP();
			ceiling = static_cast<ceiling_t*>(DoomLib::Z_Malloc(sizeof(*ceiling), PU_LEVEL, nullptr));
			memcpy (ceiling, ::g->save_p, sizeof(*ceiling));
			::g->save_p += sizeof(*ceiling);
			ceiling->sector = &::g->sectors[(int)ceiling->sector];
			ceiling->sector->specialdata = ceiling;

			if (ceiling->thinker.function.acp1)
			{
				ceiling->thinker.function.acp1 = reinterpret_cast<actionf_p1>(T_MoveCeiling);
			}

			P_AddThinker (&ceiling->thinker);
			P_AddActiveCeiling(ceiling);
			break;

		case tc_door:
			PADSAVEP();
			door = static_cast<vldoor_t*>(DoomLib::Z_Malloc(sizeof(*door), PU_LEVEL, nullptr));
			memcpy (door, ::g->save_p, sizeof(*door));
			::g->save_p += sizeof(*door);
			door->sector = &::g->sectors[(int)door->sector];
			door->sector->specialdata = door;
			door->thinker.function.acp1 = reinterpret_cast<actionf_p1>(T_VerticalDoor);
			P_AddThinker (&door->thinker);
			break;

		case tc_floor:
			PADSAVEP();
			floor = static_cast<floormove_t*>(DoomLib::Z_Malloc(sizeof(*floor), PU_LEVEL, nullptr));
			memcpy (floor, ::g->save_p, sizeof(*floor));
			::g->save_p += sizeof(*floor);
			floor->sector = &::g->sectors[(int)floor->sector];
			floor->sector->specialdata = floor;
			floor->thinker.function.acp1 = reinterpret_cast<actionf_p1>(T_MoveFloor);
			P_AddThinker (&floor->thinker);
			break;

		case tc_plat:
			PADSAVEP();
			plat = static_cast<plat_t*>(DoomLib::Z_Malloc(sizeof(*plat), PU_LEVEL, nullptr));
			memcpy (plat, ::g->save_p, sizeof(*plat));
			::g->save_p += sizeof(*plat);
			plat->sector = &::g->sectors[(int)plat->sector];
			plat->sector->specialdata = plat;

			if (plat->thinker.function.acp1)
			{
				plat->thinker.function.acp1 = reinterpret_cast<actionf_p1>(T_PlatRaise);
			}

			P_AddThinker (&plat->thinker);
			P_AddActivePlat(plat);
			break;

		case tc_fire:
			PADSAVEP();
			fire = static_cast<fireflicker_t*>(DoomLib::Z_Malloc(sizeof(*fire), PU_LEVEL, nullptr));
			memcpy (fire, ::g->save_p, sizeof(*fire));
			::g->save_p += sizeof(*fire);
			fire->sector = &::g->sectors[(int)fire->sector];
			fire->thinker.function.acp1 = reinterpret_cast<actionf_p1>(T_FireFlicker);
			P_AddThinker (&fire->thinker);
			break;

		case tc_flash:
			PADSAVEP();
			flash = static_cast<lightflash_t*>(DoomLib::Z_Malloc(sizeof(*flash), PU_LEVEL, nullptr));
			memcpy (flash, ::g->save_p, sizeof(*flash));
			::g->save_p += sizeof(*flash);
			flash->sector = &::g->sectors[(int)flash->sector];
			flash->thinker.function.acp1 = reinterpret_cast<actionf_p1>(T_LightFlash);
			P_AddThinker (&flash->thinker);
			break;

		case tc_strobe:
			PADSAVEP();
			strobe = static_cast<strobe_t*>(DoomLib::Z_Malloc(sizeof(*strobe), PU_LEVEL, nullptr));
			memcpy (strobe, ::g->save_p, sizeof(*strobe));
			::g->save_p += sizeof(*strobe);
			strobe->sector = &::g->sectors[(int)strobe->sector];
			strobe->thinker.function.acp1 = reinterpret_cast<actionf_p1>(T_StrobeFlash);
			P_AddThinker (&strobe->thinker);
			break;

		case tc_glow:
			PADSAVEP();
			glow = static_cast<glow_t*>(DoomLib::Z_Malloc(sizeof(*glow), PU_LEVEL, nullptr));
			memcpy (glow, ::g->save_p, sizeof(*glow));
			::g->save_p += sizeof(*glow);
			glow->sector = &::g->sectors[(int)glow->sector];
			glow->thinker.function.acp1 = reinterpret_cast<actionf_p1>(T_Glow);
			P_AddThinker (&glow->thinker);
			break;

		default:
			I_Error ("Unknown tclass %i in savegame",tclass);
		}
	}
}


//
// P_ArchiveSpecials
//



//
// Things to handle:
//
// T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list
// T_VerticalDoor, (vldoor_t: sector_t * swizzle),
// T_MoveFloor, (floormove_t: sector_t * swizzle),
// T_LightFlash, (lightflash_t: sector_t * swizzle),
// T_StrobeFlash, (strobe_t: sector_t *),
// T_Glow, (glow_t: sector_t *),
// T_PlatRaise, (plat_t: sector_t *), - active list
//
static void P_ArchiveSpecials ()
{
	ceiling_t*		ceiling;
	int			i;
	
    // save off the current thinkers
    for (thinker_t* th = ::g->thinkercap.next ; th != &::g->thinkercap ; th=th->next)
    {
	if (th->function.acv == static_cast<actionf_v>(nullptr))
	{
	    /*for (i = 0; i < MAXCEILINGS;i++)
	    {
		    if (::g->activeceilings[i] == reinterpret_cast<ceiling_t*>(th))
		    {
			    break;
		    }
	    }*/

		const index_t idx = ::g->activeceilings.FindIndex(reinterpret_cast<ceiling_t*>(th));

		if (idx >= 0)
		{
			*::g->save_p++ = tc_ceiling;
			PADSAVEP();
			ceiling = reinterpret_cast<ceiling_t*>(::g->save_p);
			memcpy(ceiling, th, sizeof(*ceiling));
			::g->save_p += sizeof(*ceiling);
			ceiling->sector = reinterpret_cast<sector_t*>(ceiling->sector - ::g->sectors);
		}
		continue;
	}
			
	if (th->function.acp1 == reinterpret_cast<actionf_p1>(T_MoveCeiling))
	{
	    *::g->save_p++ = tc_ceiling;
	    PADSAVEP();
	    ceiling = (ceiling_t *)::g->save_p;
	    memcpy (ceiling, th, sizeof(*ceiling));
	    ::g->save_p += sizeof(*ceiling);
	    ceiling->sector = (sector_t *)(ceiling->sector - ::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_VerticalDoor)
	{
	    *::g->save_p++ = tc_door;
	    PADSAVEP();
	    vldoor_t* door = (vldoor_t*)::g->save_p;
	    memcpy (door, th, sizeof(*door));
	    ::g->save_p += sizeof(*door);
	    door->sector = (sector_t *)(door->sector - ::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_MoveFloor)
	{
	    *::g->save_p++ = tc_floor;
	    PADSAVEP();
	    floormove_t* floor = (floormove_t*)::g->save_p;
	    memcpy (floor, th, sizeof(*floor));
	    ::g->save_p += sizeof(*floor);
	    floor->sector = (sector_t *)(floor->sector - ::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_PlatRaise)
	{
	    *::g->save_p++ = tc_plat;
	    PADSAVEP();
	    plat_t* plat = (plat_t*)::g->save_p;
	    memcpy (plat, th, sizeof(*plat));
	    ::g->save_p += sizeof(*plat);
	    plat->sector = (sector_t *)(plat->sector - ::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_LightFlash)
	{
	    *::g->save_p++ = tc_flash;
	    PADSAVEP();
	    lightflash_t* flash = (lightflash_t*)::g->save_p;
	    memcpy (flash, th, sizeof(*flash));
	    ::g->save_p += sizeof(*flash);
	    flash->sector = (sector_t *)(flash->sector - ::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
	{
	    *::g->save_p++ = tc_strobe;
	    PADSAVEP();
	    strobe_t* strobe = (strobe_t*)::g->save_p;
	    memcpy (strobe, th, sizeof(*strobe));
	    ::g->save_p += sizeof(*strobe);
	    strobe->sector = (sector_t *)(strobe->sector - ::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_Glow)
	{
	    *::g->save_p++ = tc_glow;
	    PADSAVEP();
	    glow_t* glow = (glow_t*)::g->save_p;
	    memcpy (glow, th, sizeof(*glow));
	    ::g->save_p += sizeof(*glow);
	    glow->sector = (sector_t *)(glow->sector - ::g->sectors);
	    continue;
	}
    }
	
    // add a terminating marker
    *::g->save_p++ = tc_endspecials;	

}


//
// P_UnArchiveSpecials
//
static void P_UnArchiveSpecials ()
{
	ceiling_t*		ceiling;
    vldoor_t*		door;
    floormove_t*	floor;
    plat_t*		plat;
    lightflash_t*	flash;
    strobe_t*		strobe;
    glow_t*		glow;

    // read in saved thinkers
    while (true)
    {
	byte tclass = *::g->save_p++;
	switch (tclass)
	{
	  case tc_endspecials:
	    return;	// end of list
			
	  case tc_ceiling:
	    PADSAVEP();
	    ceiling = static_cast<ceiling_t*>(DoomLib::Z_Malloc(sizeof(*ceiling), PU_LEVEL, nullptr));
	    memcpy (ceiling, ::g->save_p, sizeof(*ceiling));
	    ::g->save_p += sizeof(*ceiling);
	    ceiling->sector = &::g->sectors[(int)ceiling->sector];
	    ceiling->sector->specialdata = ceiling;

	    if (ceiling->thinker.function.acp1)
	    {
		    ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;
	    }

	    P_AddThinker (&ceiling->thinker);
	    P_AddActiveCeiling(ceiling);
	    break;
				
	  case tc_door:
	    PADSAVEP();
	    door = static_cast<vldoor_t*>(DoomLib::Z_Malloc(sizeof(*door), PU_LEVEL, nullptr));
	    memcpy (door, ::g->save_p, sizeof(*door));
	    ::g->save_p += sizeof(*door);
	    door->sector = &::g->sectors[(int)door->sector];
	    door->sector->specialdata = door;
	    door->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
	    P_AddThinker (&door->thinker);
	    break;
				
	  case tc_floor:
	    PADSAVEP();
	    floor = static_cast<floormove_t*>(DoomLib::Z_Malloc(sizeof(*floor), PU_LEVEL, nullptr));
	    memcpy (floor, ::g->save_p, sizeof(*floor));
	    ::g->save_p += sizeof(*floor);
	    floor->sector = &::g->sectors[(int)floor->sector];
	    floor->sector->specialdata = floor;
	    floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
	    P_AddThinker (&floor->thinker);
	    break;
				
	  case tc_plat:
	    PADSAVEP();
	    plat = static_cast<plat_t*>(DoomLib::Z_Malloc(sizeof(*plat), PU_LEVEL, nullptr));
	    memcpy (plat, ::g->save_p, sizeof(*plat));
	    ::g->save_p += sizeof(*plat);
	    plat->sector = &::g->sectors[(int)plat->sector];
	    plat->sector->specialdata = plat;

	    if (plat->thinker.function.acp1)
	    {
		    plat->thinker.function.acp1 = (actionf_p1)T_PlatRaise;
	    }

	    P_AddThinker (&plat->thinker);
	    P_AddActivePlat(plat);
	    break;
				
	  case tc_flash:
	    PADSAVEP();
	    flash = static_cast<lightflash_t*>(DoomLib::Z_Malloc(sizeof(*flash), PU_LEVEL, nullptr));
	    memcpy (flash, ::g->save_p, sizeof(*flash));
	    ::g->save_p += sizeof(*flash);
	    flash->sector = &::g->sectors[(int)flash->sector];
	    flash->thinker.function.acp1 = (actionf_p1)T_LightFlash;
	    P_AddThinker (&flash->thinker);
	    break;
				
	  case tc_strobe:
	    PADSAVEP();
	    strobe = static_cast<strobe_t*>(DoomLib::Z_Malloc(sizeof(*strobe), PU_LEVEL, nullptr));
	    memcpy (strobe, ::g->save_p, sizeof(*strobe));
	    ::g->save_p += sizeof(*strobe);
	    strobe->sector = &::g->sectors[(int)strobe->sector];
	    strobe->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
	    P_AddThinker (&strobe->thinker);
	    break;
				
	  case tc_glow:
	    PADSAVEP();
	    glow = static_cast<glow_t*>(DoomLib::Z_Malloc(sizeof(*glow), PU_LEVEL, nullptr));
	    memcpy (glow, ::g->save_p, sizeof(*glow));
	    ::g->save_p += sizeof(*glow);
	    glow->sector = &::g->sectors[(int)glow->sector];
	    glow->thinker.function.acp1 = (actionf_p1)T_Glow;
	    P_AddThinker (&glow->thinker);
	    break;
				
	  default:
	    I_Error ("P_UnarchiveSpecials:Unknown tclass %i "
		     "in savegame",tclass);
	}
	
    }

}


