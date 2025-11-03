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

#include "i_system.h"
#include "z_zone.h"
#include "m_random.h"

#include "doomdef.h"
#include "p_local.h"
#include "sounds.h"

#include "st_stuff.h"
#include "hu_stuff.h"

#include "s_sound.h"

#include "doomstat.h"

extern bool globalNetworking;

void G_PlayerReborn (int player);
static void P_SpawnMapThing (mapthing_t*	mthing);


//
// P_SetMobjState
// Returns true if the mobj is still present.
//

qboolean P_SetMobjState ( mobj_t* mobj, statenum_t state )
{
	do
	{
		if (state == S_NULL)
		{
			mobj->state = nullptr;
			P_RemoveMobj (mobj);
			return false;
		}

		const state_t* st = &::g->states[state];
		mobj->state = st;
		mobj->tics = st->tics;
		mobj->sprite = st->sprite;
		mobj->frame = st->frame;

		// Modified handling.
		// Call action functions when the state is set
		if (st->action)
		{
			st->action(mobj, nullptr);
		}

		state = st->nextstate;
	} while (!mobj->tics);

	return true;
}


//
// P_ExplodeMissile  
//
static void P_ExplodeMissile (mobj_t* mo)
{
	mo->momx = mo->momy = mo->momz = 0;

	P_SetMobjState (mo, static_cast<statenum_t>(mobjinfo[mo->type].deathstate));

	mo->tics -= P_Random()&3;

	mo->tics = Max(mo->tics, 1);

	mo->flags &= ~MF_MISSILE;

	if (mo->info->deathsound)
	{
		S_StartSound (mo, mo->info->deathsound);
	}
}


//
// P_XYMovement  
//

static void P_XYMovement (mobj_t* mo) 
{ 	
	fixed_t 	ptryx;
	fixed_t	ptryy;

	if (!mo->momx && !mo->momy)
	{
		if (mo->flags & MF_SKULLFLY)
		{
			// the skull slammed into something
			mo->flags &= ~MF_SKULLFLY;
			mo->momx = mo->momy = mo->momz = 0;

			P_SetMobjState (mo, mo->info->spawnstate);
		}
		return;
	}

	player_t* player = ::g->players[mo->player];

	if (mo->momx > MAXMOVE)
	{
		mo->momx = MAXMOVE;
	}
	else if (mo->momx < -MAXMOVE)
	{
		mo->momx = -MAXMOVE;
	}

	if (mo->momy > MAXMOVE)
	{
		mo->momy = MAXMOVE;
	}
	else if (mo->momy < -MAXMOVE)
	{
		mo->momy = -MAXMOVE;
	}

	fixed_t xmove = mo->momx;
	fixed_t ymove = mo->momy;

	do
	{
		if (xmove > MAXMOVE/2 || ymove > MAXMOVE/2)
		{
			ptryx = mo->x + xmove/2;
			ptryy = mo->y + ymove/2;
			xmove >>= 1;
			ymove >>= 1;
		}
		else
		{
			ptryx = mo->x + xmove;
			ptryy = mo->y + ymove;
			xmove = ymove = 0;
		}

		if (!P_TryMove (mo, ptryx, ptryy))
		{
			// blocked move
			if (mo->player)
			{	// try to slide along it
				P_SlideMove (mo);
			}
			else if (mo->flags & MF_MISSILE)
			{
				// explode a missile
				if (::g->ceilingline &&
					::g->ceilingline->backsector &&
					::g->ceilingline->backsector->ceilingpic == ::g->skyflatnum)
				{
					// Hack to prevent missiles exploding
					// against the sky.
					// Does not handle sky floors.
					P_RemoveMobj (mo);
					return;
				}
				P_ExplodeMissile (mo);
			}
			else
			{
				mo->momx = mo->momy = 0;
			}
		}
	} while (xmove || ymove);

	// slow down
	if (player && player->cheats & CF_NOMOMENTUM)
	{
		// debug option for no sliding at all
		mo->momx = mo->momy = 0;
		return;
	}

	if (mo->flags & (MF_MISSILE | MF_SKULLFLY) )
	{
		return; // no friction for missiles ever
	}

	if (mo->z > mo->floorz)
	{
		return; // no friction when airborne
	}

	if (mo->flags & MF_CORPSE)
	{
		// do not stop sliding
		//  if halfway off a step with some momentum
		if (mo->momx > FRACUNIT/4
			|| mo->momx < -FRACUNIT/4
			|| mo->momy > FRACUNIT/4
			|| mo->momy < -FRACUNIT/4)
		{
			if (mo->floorz != mo->subsector->sector->floorheight)
			{
				return;
			}
		}
	}

	if (mo->momx > -STOPSPEED
		&& mo->momx < STOPSPEED
		&& mo->momy > -STOPSPEED
		&& mo->momy < STOPSPEED
		&& (!player
		|| (player->cmd.forwardmove== 0
		&& player->cmd.sidemove == 0 ) ) )
	{
		// if in a walking frame, stop moving
		if ( player&&static_cast<unsigned>((player->mo->state - ::g->states) - S_PLAY_RUN1) < 4)
		{
			P_SetMobjState (player->mo, S_PLAY);
		}

		mo->momx = 0;
		mo->momy = 0;
	}
	else
	{
		mo->momx *= FRICTION;
		mo->momy *= FRICTION;
	}
}

//
// P_ZMovement
//
static void P_ZMovement (mobj_t* mo)
{
	if (mo)
	{
		player_t* player = nullptr;

		if (mo->player >= 0 && std::cmp_less(mo->player, ::g->players.Num()))
		{
			player = &::g->players[mo->player];
		}

		// check for smooth step up
		if (player && mo->z < mo->floorz)
		{
			player->viewheight -= mo->floorz - mo->z;

			player->deltaviewheight = (VIEWHEIGHT - player->viewheight) >> 3;
		}

		// adjust height
		mo->z += mo->momz;

		if (mo->flags & MF_FLOAT
			&& mo->target)
		{
			// float down towards target if too close
			if (!(mo->flags & MF_SKULLFLY)
				&& !(mo->flags & MF_INFLOAT))
			{
				fixed_t dist = P_ApproxDistance(mo->x - mo->target->x,
					mo->y - mo->target->y);

				fixed_t delta = (mo->target->z + (mo->height >> 1)) - mo->z;

				if (delta < 0 && dist < -(delta * 3))
				{
					mo->z -= FLOATSPEED;
				}
				else if (delta > 0 && dist < (delta * 3))
				{
					mo->z += FLOATSPEED;
				}
			}

		}

		// clip movement
		if (mo->z <= mo->floorz)
		{
			// hit the floor

			// Note (id):
			//  somebody left this after the setting momz to 0,
			//  kinda useless there.
			if (mo->flags & MF_SKULLFLY)
			{
				// the skull slammed into something
				mo->momz = -mo->momz;
			}

			if (mo->momz < 0)
			{
				if (player && mo->momz < -GRAVITY * 8)
				{
					// Squat down.
					// Decrease ::g->viewheight for a moment
					// after hitting the ground (hard),
					// and utter appropriate sound.
					player->deltaviewheight = mo->momz >> 3;
					if (globalNetworking || (player == &::g->players[::g->consoleplayer]))
					{
						S_StartSound(mo, sfx_oof);
					}
				}
				mo->momz = 0;
			}
			mo->z = mo->floorz;

			if ((mo->flags & MF_MISSILE)
				&& !(mo->flags & MF_NOCLIP))
			{
				P_ExplodeMissile(mo);
				return;
			}
		}
		else if (!(mo->flags & MF_NOGRAVITY))
		{
			if (mo->momz == 0)
			{
				mo->momz = -GRAVITY * 2;
			}
			else
			{
				mo->momz -= GRAVITY;
			}
		}

		if (mo->z + mo->height > mo->ceilingz)
		{
			// hit the ceiling
			mo->momz = Min(mo->momz, 0);
			{
				mo->z = mo->ceilingz - mo->height;
			}

			if (mo->flags & MF_SKULLFLY)
			{	// the skull slammed into something
				mo->momz = -mo->momz;
			}

			if ((mo->flags & MF_MISSILE)
				&& !(mo->flags & MF_NOCLIP))
			{
				P_ExplodeMissile(mo);
				return;
			}
		}
	}
} 



//
// P_NightmareRespawn
//
static void
P_NightmareRespawn (mobj_t* mobj)
{
	fixed_t		z;

	fixed_t x = mobj->spawnpoint.x; 
	fixed_t y = mobj->spawnpoint.y; 

	// somthing is occupying it's position?
	if (!P_CheckPosition (mobj, x, y) )
	{
		return; // no respwan
	}

	// spawn a teleport fog at old spot
	// because of removal of the body?
	mobj_t* mo = P_SpawnMobj(mobj->x,
	                         mobj->y,
	                         mobj->subsector->sector->floorheight, MT_TFOG); 
	// initiate teleport sound
	S_StartSound (mo, sfx_telept);

	// spawn a teleport fog at the new spot
	subsector_t* ss = R_PointInSubsector(x, y); 

	mo = P_SpawnMobj (x, y, ss->sector->floorheight , MT_TFOG); 

	S_StartSound (mo, sfx_telept);

	// spawn the new monster
	mapthing_t* mthing = &mobj->spawnpoint;

	// spawn it
	if (mobj->info->flags & MF_SPAWNCEILING)
	{
		z = ONCEILINGZ;
	}
	else
	{
		z = ONFLOORZ;
	}

	// inherit attributes from deceased one
	mo = P_SpawnMobj (x,y,z, mobj->type);
	mo->spawnpoint = mobj->spawnpoint;	
	mo->angle = ANG45 * (mthing->angle/45);

	if (mthing->options & MTF_AMBUSH)
	{
		mo->flags |= MF_AMBUSH;
	}

	mo->reactiontime = 18;

	// remove the old monster,
	P_RemoveMobj (mobj);
}


//
// P_MobjThinker
//
void P_MobjThinker (mobj_t* mobj)
{
	// momentum movement
	if (mobj->momx
		|| mobj->momy
		|| (mobj->flags&MF_SKULLFLY) )
	{
		P_XYMovement (mobj);

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if (mobj->thinker.function == nullptr)
		{
			return; // mobj was removed
		}
	}
	if ( (mobj->z != mobj->floorz)
		|| mobj->momz )
	{
		P_ZMovement (mobj);

		// FIXME: decent NOP/NULL/Nil function pointer please.
		if (mobj->thinker.function == nullptr)
		{
			return; // mobj was removed
		}
	}


	// cycle through states,
	// calling action functions at transitions
	if (mobj->tics != -1)
	{
		mobj->tics--;

		// you can cycle through multiple states in a tic
		if (!mobj->tics)
		{
			if (!P_SetMobjState (mobj, mobj->state->nextstate) )
			{
				return; // freed itself
			}
		}
	}
	else
	{
		// check for nightmare respawn
		if (! (mobj->flags & MF_COUNTKILL) )
		{
			return;
		}

		if (!::g->respawnmonsters)
		{
			return;
		}

		mobj->movecount++;

		if (mobj->movecount < 12 * TICRATE)
		{
			return;
		}

		if ( ::g->leveltime&31 )
		{
			return;
		}

		if (P_Random () > 4)
		{
			return;
		}

		P_NightmareRespawn (mobj);
	}

}


//
// P_SpawnMobj
//
mobj_t*
P_SpawnMobj
(const fixed_t	x,
 const fixed_t	y,
 const fixed_t	z,
 const mobjtype_t	type )
{
	mobj_t* mobj = static_cast<mobj_t*>(DoomLib::Z_Malloc(sizeof(*mobj), PU_LEVEL, nullptr));
	memset (mobj, 0, sizeof (*mobj));
	const mobjinfo_t* info = &mobjinfo[type];

	mobj->type = type;
	mobj->info = info;
	mobj->x = x;
	mobj->y = y;
	mobj->radius = info->radius;
	mobj->height = info->height;
	mobj->flags = info->flags;
	mobj->health = info->spawnhealth;

	if (::g->gameskill != sk_nightmare)
	{
		mobj->reactiontime = info->reactiontime;
	}

	mobj->lastlook = P_Random () % MAXPLAYERS;
	// do not set the state with P_SetMobjState,
	// because action routines can not be called yet
	const state_t* st = &::g->states[info->spawnstate];

	mobj->state = st;
	mobj->tics = st->tics;
	mobj->sprite = st->sprite;
	mobj->frame = st->frame;

	// set subsector and/or block links
	P_SetThingPosition (mobj);

	mobj->floorz = mobj->subsector->sector->floorheight;
	mobj->ceilingz = mobj->subsector->sector->ceilingheight;

	if (z == ONFLOORZ)
	{
		mobj->z = mobj->floorz;
	}
	else if (z == ONCEILINGZ)
	{
		mobj->z = mobj->ceilingz - mobj->info->height;
	}
	else
	{
		mobj->z = z;
	}

	mobj->thinker.function = ACTIONF_T(P_MobjThinker);

	P_AddThinker (&mobj->thinker);

	return mobj;
}


//
// P_RemoveMobj
//


void P_RemoveMobj (mobj_t* mobj)
{
	// only respawn items in ::g->deathmatch
	if (::g->deathmatch != 2)
	{
		if ((mobj->flags & MF_SPECIAL)
			&& !(mobj->flags & MF_DROPPED)
			&& (mobj->type != MT_INV)
			&& (mobj->type != MT_INS))
		{
			auto* item_respawn = new itemRespawn_s{};
			item_respawn->removalTime = ::g->leveltime;
			item_respawn->thing = mobj->spawnpoint;
			::g->itemRespawnQueue.Add(item_respawn);
			::g->lastItemRemovalTime = I_GetTime();
		}
	}

	// unlink from sector and block lists
	P_UnsetThingPosition (mobj);

	// stop any playing sound
	//S_StopSound (mobj);

	// free block
	P_RemoveThinker (reinterpret_cast<thinker_t*>(mobj));
}




//
// P_RespawnSpecials
//
void P_RespawnSpecials ()
{
	// only respawn items in ::g->deathmatch
	if (::g->deathmatch != 2)
	{
		return; // 
	}

	// nothing left to respawn?
	if (::g->itemRespawnQueue.IsEmpty())
	{
		return;
	}
		
	auto respawn_item = ::g->itemRespawnQueue.Peek();

	if (respawn_item)
	{
		// wait at least 30 seconds
		if ((::g->leveltime - respawn_item->removalTime) < ITEM_RESPAWN_DELAY)
		{
			return;
		}

		// pull it from the queue
		respawn_item = ::g->itemRespawnQueue.RemoveFirst();

		if (respawn_item)
		{
			mapthing_t* mthing = &respawn_item->thing;

			if (mthing)
			{
				fixed_t x = mthing->x;
				fixed_t y = mthing->y;
				fixed_t z = 0;

				// spawn a teleport fog at the new spot
				subsector_t* ss = R_PointInSubsector(x, y);

				if (ss)
				{
					mobj_t* mo = P_SpawnMobj(x, y, ss->sector->floorheight, MT_IFOG);

					if (mo)
					{
						S_StartSound(mo, sfx_itmbk);

						// find which type to spawn
						index_t i = 0;
						for (i = 0; i < NUMMOBJTYPES; ++i)
						{
							if (mthing->type == mobjinfo[i].doomednum)
							{
								break;
							}
						}

						// spawn it
						if (mobjinfo[i].flags & MF_SPAWNCEILING)
						{
							z = ONCEILINGZ;
						}
						else
						{
							z = ONFLOORZ;
						}

						mo = P_SpawnMobj(x, y, z, static_cast<mobjtype_t>(i));

						if (mo)
						{
							mo->spawnpoint = *mthing;
							mo->angle = ANG45 * (mthing->angle / 45);
						}
					}
				}
			}

			delete respawn_item;
		}
	}
}




//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
//
static void P_SpawnPlayer (mapthing_t* mthing)
{
	// not playing?
	if (!::g->players[mthing->type-1].playerInGame)
	{
		return;
	}

	player_t* p = &::g->players[mthing->type - 1];

	if (p->playerstate == PST_REBORN)
	{
		G_PlayerReborn (mthing->type-1);
	}

	fixed_t x = mthing->x;
	fixed_t y = mthing->y;
	fixed_t z = ONFLOORZ;
	mobj_t* mobj = P_SpawnMobj(x, y, z, MT_PLAYER);

	// set color translations for player ::g->sprites
	if (mthing->type > 1)
	{
		mobj->flags |= (mthing->type-1)<<MF_TRANSSHIFT;
	}

	mobj->angle	= ANG45 * (mthing->angle/45);
	mobj->player = p;
	mobj->health = p->health;

	p->mo = mobj;
	p->playerstate = PST_LIVE;	
	p->refire = 0;
	p->message = nullptr;
	p->damagecount = 0;
	p->bonuscount = 0;
	p->extralight = 0;
	p->fixedcolormap = 0;
	p->viewheight = VIEWHEIGHT;

	// setup gun psprite
	P_SetupPsprites (p);

	// give all cards in death match mode
	if (::g->deathmatch)
	{
		for (int i = 0 ; i<NUMCARDS ; ++i)
		{
			p->cards[i] = true;
		}
	}

	if (mthing->type-1 == ::g->consoleplayer)
	{
		// wake up the status bar
		ST_Start ();
		// wake up the heads up text
		HU_Start ();		
	}

	// Give him everything is Give All is on.
	if( p->cheats & CF_GIVEALL ) {
		 p->armorpoints = 200;
		 p->armortype = 2;

		int i;
		for (i=0;i<NUMWEAPONS;++i)
		{
			p->weaponowned[i] = true;
		}

		 for (i=0;i<NUMAMMO;++i)
		 {
			 p->ammo[i] =  p->maxammo[i];
		 }

		 for (i=0;i<NUMCARDS;++i)
		 {
			 p->cards[i] = true;
		 }
	}

}


//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
void P_SpawnMapThing (mapthing_t* mthing)
{
	int			i;
	int			bit;
	fixed_t		z;

	// count ::g->deathmatch start positions
	if (mthing->type == 11)
	{
		if (::g->deathmatch_p < &::g->deathmatchstarts)
		{
			memcpy (::g->deathmatch_p, mthing, sizeof(*mthing));
			::g->deathmatch_p++;
		}
		return;
	}

	// check for ::g->players specially
	if (mthing->type <= 4)
	{
		// save spots for respawning in network games
		::g->playerstarts[mthing->type-1] = *mthing;
		if (!::g->deathmatch)
		{
			P_SpawnPlayer (mthing);
		}

		return;
	}

	// check for appropriate skill level
	if (!::g->netgame && (mthing->options & 16) )
	{
		return;
	}

	if (::g->gameskill == sk_baby)
	{
		bit = 1;
	}
	else if (::g->gameskill == sk_nightmare)
	{
		bit = 4;
	}
	else
	{
		bit = 1<<(::g->gameskill-1);
	}

	if (!(mthing->options & bit) )
	{
		return;
	}

	// find which type to spawn
	for (i=0 ; i< NUMMOBJTYPES ; ++i)
	{
		if (mthing->type == mobjinfo[i].doomednum)
		{
			break;
		}
	}

	if ( i==NUMMOBJTYPES ) {
		//printf( "P_SpawnMapThing: Unknown type %i at (%i, %i)", mthing->type, mthing->x, mthing->y);
		return;
		//I_Error ("P_SpawnMapThing: Unknown type %i at (%i, %i)",
		//mthing->type,
		//mthing->x, mthing->y);
	}

	// don't spawn keycards and ::g->players in ::g->deathmatch
	if (::g->deathmatch && mobjinfo[i].flags & MF_NOTDMATCH)
	{
		return;
	}

	// don't spawn any monsters if -::g->nomonsters
	if (::g->nomonsters
		&& ( i == MT_SKULL
		|| (mobjinfo[i].flags & MF_COUNTKILL)) )
	{
		return;
	}

	// spawn it
	fixed_t x = mthing->x;
	fixed_t y = mthing->y;

	if (mobjinfo[i].flags & MF_SPAWNCEILING)
	{
		z = ONCEILINGZ;
	}
	else
	{
		z = ONFLOORZ;
	}

	mobj_t* mobj = P_SpawnMobj(x, y, z, static_cast<mobjtype_t>(i));
	mobj->spawnpoint = *mthing;

	if (mobj->tics > 0)
	{
		mobj->tics = 1 + (P_Random () % mobj->tics);
	}
	if (mobj->flags & MF_COUNTKILL)
	{
		::g->totalkills++;
	}
	if (mobj->flags & MF_COUNTITEM)
	{
		::g->totalitems++;
	}

	mobj->angle = ANG45 * (mthing->angle/45);
	if (mthing->options & MTF_AMBUSH)
	{
		mobj->flags |= MF_AMBUSH;
	}
}



//
// GAME SPAWN FUNCTIONS
//


//
// P_SpawnPuff
//

void
P_SpawnPuff
(const fixed_t	x,
 const fixed_t	y,
 fixed_t	z )
{
	z += ((P_Random()-P_Random())<<10);

	mobj_t* th = P_SpawnMobj(x, y, z, MT_PUFF);
	th->momz = FRACUNIT;
	th->tics -= P_Random()&3;

	th->tics = Max(th->tics, 1);

	// don't make punches spark on the wall
	if (::g->attackrange == MELEERANGE) {

		P_SetMobjState (th, S_PUFF3);
		
	}
}



//
// P_SpawnBlood
// 
void
P_SpawnBlood
(const fixed_t	x,
 const fixed_t	y,
 fixed_t	z,
 const int		damage )
{
	z += ((P_Random()-P_Random())<<10);
	mobj_t* th = P_SpawnMobj(x, y, z, MT_BLOOD);
	th->momz = FRACUNIT*2;
	th->tics -= P_Random()&3;

	th->tics = Max(th->tics, 1);

	if (damage <= 12 && damage >= 9)
	{
		P_SetMobjState (th,S_BLOOD2);
	}
	else if (damage < 9)
	{
		P_SetMobjState (th,S_BLOOD3);
	}
}



//
// P_CheckMissileSpawn
// Moves the missile forward a bit
//  and possibly explodes it right there.
//
static void P_CheckMissileSpawn (mobj_t* th)
{
	th->tics -= P_Random()&3;
	th->tics = Max(th->tics, 1);

	// move a little forward so an angle can
	// be computed if it immediately explodes
	th->x += (th->momx>>1);
	th->y += (th->momy>>1);
	th->z += (th->momz>>1);

	if (!P_TryMove (th, th->x, th->y))
	{
		P_ExplodeMissile (th);
	}
}


//
// P_SpawnMissile
//
mobj_t* P_SpawnMissile ( mobj_t* source, mobj_t* dest, const mobjtype_t type )
{
	mobj_t* th = P_SpawnMobj(source->x,
	                         source->y,
	                         source->z + 4 * 8 * FRACUNIT, type);

	if (th->info->seesound)
	{
		S_StartSound (th, th->info->seesound);
	}

	th->target = source;	// where it came from
	angle_t an = R_PointToAngle(source->x, source->y, dest->x, dest->y);	

	// fuzzy player
	if (dest->flags & MF_SHADOW)
	{
		an += (P_Random()-P_Random())<<20;
	}

	th->angle = an;
	an >>= ANGLETOFINESHIFT;
	th->momx = (th->info->speed * finecosine[an]);
	th->momy = (th->info->speed * finesine[an]);

	int dist = P_ApproxDistance(dest->x - source->x, dest->y - source->y);
	dist = dist / th->info->speed;

	dist = Max(dist, 1);

	th->momz = (dest->z - source->z) / dist;
	P_CheckMissileSpawn (th);

	return th;
}


//
// P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//
void
P_SpawnPlayerMissile
( mobj_t*	source,
 const mobjtype_t	type )
{
	// see which target is to be aimed at
	angle_t an = source->angle;
	fixed_t slope = P_AimLineAttack(source, an, 16 * 64 * FRACUNIT);

	if (!::g->linetarget)
	{
		an += 1<<26;
		slope = P_AimLineAttack (source, an, 16*64*FRACUNIT);

		if (!::g->linetarget)
		{
			an -= 2<<26;
			slope = P_AimLineAttack (source, an, 16*64*FRACUNIT);
		}

		if (!::g->linetarget)
		{
			an = source->angle;
			slope = 0;
		}
	}

	fixed_t x = source->x;
	fixed_t y = source->y;
	fixed_t z = source->z + 4 * 8 * FRACUNIT;

	mobj_t* th = P_SpawnMobj(x, y, z, type);

	if (th->info->seesound && (source->player == &::g->players[::g->consoleplayer]) ) {
		S_StartSound (th, th->info->seesound);
	}

	th->target = source;
	th->angle = an;
	th->momx = ( th->info->speed * finecosine[an>>ANGLETOFINESHIFT] );
	th->momy = ( th->info->speed * finesine[an>>ANGLETOFINESHIFT] );
	th->momz = ( th->info->speed * slope );

	P_CheckMissileSpawn (th);
}


