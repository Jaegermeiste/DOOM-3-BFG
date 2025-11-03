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
#include <utility>

#include "m_random.h"
#include "i_system.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "g_game.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "sounds.h"


extern bool globalNetworking;



//
// P_NewChaseDir related LUT.
//
const dirtype_t opposite[] =
{
  DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
  DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};

const dirtype_t diags[] =
{
	DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};


void A_Fall( mobj_t* actor, void* );


//
// ENEMY THINKING
// Enemies are always spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all ::g->players,
// but some can be made pre-aware
//


//
// Called by P_NoiseAlert.
// Recursively traverse adjacent ::g->sectors,
// sound blocking ::g->lines cut off traversal.
//


static void P_RecursiveSound ( sector_t* sec, const int soundblocks)
{
	if (sec)
	{
		sector_t* other = nullptr;

		// wake up all monsters in this sector
		if (sec->validcount == ::g->validcount
			&& sec->soundtraversed <= soundblocks + 1)
		{
			return;		// already flooded
		}

		sec->validcount = ::g->validcount;
		sec->soundtraversed = soundblocks + 1;
		sec->soundtarget = ::g->soundtarget;

		for (size_t i = 0; i < sec->linecount; i++)
		{
			const line_t* check = sec->lines[i];

			if (check)
			{
				if (!(check->flags & ML_TWOSIDED))
				{
					continue;
				}

				P_LineOpening(check);

				if (::g->openrange <= 0)
				{
					continue; // closed door
				}

				if (::g->sides[check->sidenum[0]].sector == sec)
				{
					other = ::g->sides[check->sidenum[1]].sector;
				}
				else
				{
					other = ::g->sides[check->sidenum[0]].sector;
				}

				if (check->flags & ML_SOUNDBLOCK)
				{
					if (!soundblocks)
					{
						P_RecursiveSound(other, 1);
					}
				}
				else
				{
					P_RecursiveSound(other, soundblocks);
				}
			}
		}
	}
}



//
// P_NoiseAlert
// If a monster yells at a player,
// it will alert other monsters to the player.
//
void P_NoiseAlert ( const mobj_t* target, mobj_t* emitter )
{
	if (target && emitter)
	{
		::g->soundtarget = const_cast<mobj_t*>(target);
		::g->validcount++;

		P_RecursiveSound(emitter->subsector->sector, 0);
	}
}




//
// P_CheckMeleeRange
//
static bool P_CheckMeleeRange( const mobj_t* actor )
{
	if (actor)
	{
		if (!actor->target)
		{
			return false;
		}

		const mobj_t* pl = actor->target;
		const fixed_t dist = P_ApproxDistance(pl->x - actor->x, pl->y - actor->y);

		if (dist >= MELEERANGE - 20 * FRACUNIT + pl->info->radius)
		{
			return false;
		}

		if (!P_CheckSight(actor, actor->target))
		{
			return false;
		}

		return true;
	}

	return false;
}

//
// P_CheckMissileRange
//
static bool P_CheckMissileRange( mobj_t* actor )
{
	if (actor)
	{
		if (!P_CheckSight(actor, actor->target))
		{
			return false;
		}

		if (actor->flags & MF_JUSTHIT)
		{
			// the target just hit the enemy,
			// so fight back!
			actor->flags &= ~MF_JUSTHIT;
			return true;
		}

		if (actor->reactiontime)
		{
			return false; // do not attack yet
		}

		// OPTIMIZE: get this from a global checksight
		fixed_t dist = P_ApproxDistance(actor->x - actor->target->x, actor->y - actor->target->y) - 64 * FRACUNIT;

		if (!actor->info->meleestate)
		{
			dist -= 128 * FRACUNIT; // no melee attack, so fire more
		}

		dist >>= 16;

		if (actor->type == MT_VILE)
		{
			if (dist > 14 * 64)
			{
				return false; // too far away
			}
		}


		if (actor->type == MT_UNDEAD)
		{
			if (dist < 196)
			{
				return false; // close for fist attack
			}
			dist >>= 1;
		}


		if (actor->type == MT_CYBORG
			|| actor->type == MT_SPIDER
			|| actor->type == MT_SKULL)
		{
			dist >>= 1;
		}

		dist = Min(dist, 200);

		if (actor->type == MT_CYBORG && dist > 160)
		{
			dist = 160;
		}

		if (P_Random() < dist)
		{
			return false;
		}

		return true;
	}

	return false;
}


//
// P_Move
// Move in the current direction,
// returns false if the move is blocked.
//
//const fixed_t xspeed[8] = { FRACUNIT,47000,0,-47000,-FRACUNIT,-47000,0,47000 };
//const fixed_t yspeed[8] = { 0,47000,FRACUNIT,47000,0,-47000,-FRACUNIT,-47000 };

constexpr fixed_t SPEED_SCALED = 47000 / OLDFRACUNIT;  // 0.7171630859375f

// dirtype_t:                      DI_EAST,  DI_NORTHEAST,  DI_NORTH,  DI_NORTHWEST,    DI_WEST,    DI_SOUTHWEST,   DI_SOUTH,  DI_SOUTHEAST
constexpr fixed_t xspeed[8] = { FRACUNIT, SPEED_SCALED,0,        -SPEED_SCALED,-FRACUNIT,  -SPEED_SCALED, 0,         SPEED_SCALED };
constexpr fixed_t yspeed[8] = { 0,        SPEED_SCALED,FRACUNIT,  SPEED_SCALED, 0,         -SPEED_SCALED,-FRACUNIT, -SPEED_SCALED };


static bool P_Move( mobj_t* actor )
{
	// warning: 'catch', 'throw', and 'try'
	// are all C++ reserved words

	if (actor)
	{
		if (actor->movedir == DI_NODIR)
		{
			return false;
		}

		if (actor->movedir >= NUMDIRS)
		{
			I_Error("Weird actor->movedir!");
		}

		const fixed_t tryx = actor->x + actor->info->speed * xspeed[actor->movedir];
		const fixed_t tryy = actor->y + actor->info->speed * yspeed[actor->movedir];

		const bool try_ok = P_TryMove(actor, tryx, tryy);

		if (!try_ok)
		{
			// open any specials
			if (actor->flags & MF_FLOAT && ::g->floatok)
			{
				// must adjust height
				if (actor->z < ::g->tmfloorz)
				{
					actor->z += FLOATSPEED;
				}
				else
				{
					actor->z -= FLOATSPEED;
				}

				actor->flags |= MF_INFLOAT;
				return true;
			}

			if (!::g->numspechit)
			{
				return false;
			}

			actor->movedir = DI_NODIR;
			bool good = false;
			while (::g->numspechit--)
			{
				line_t* ld = ::g->spechit[::g->numspechit];

				if (ld)
				{
					// if the special is not a door
					// that can be opened,
					// return false
					if (P_UseSpecialLine(actor, ld, 0))
					{
						good = true;
					}
				}
			}
			return good;
		}
		else
		{
			actor->flags &= ~MF_INFLOAT;
		}


		if (!(actor->flags & MF_FLOAT))
		{
			actor->z = actor->floorz;
		}

		return true;
	}

	return false;
}


//
// TryWalk
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
//
static bool P_TryWalk( mobj_t* actor )
{
	if (actor)
	{
		if (!P_Move(actor))
		{
			return false;
		}

		actor->movecount = P_Random() & 15;

		return true;
	}

	return false;
}


static void P_NewChaseDir( mobj_t* actor )
{
	if (actor)
	{
		dirtype_t d[3] = {};

		dirtype_t tdir = DI_NODIR;

		if (!actor->target)
		{
			I_Error("P_NewChaseDir: called with no target");
		}

		const dirtype_t olddir = actor->movedir;
		const dirtype_t turnaround = opposite[olddir];

		const fixed_t deltax = actor->target->x - actor->x;
		const fixed_t deltay = actor->target->y - actor->y;

		if (deltax > 10 * FRACUNIT)
		{
			d[1] = DI_EAST;
		}
		else if (deltax < -10 * FRACUNIT)
		{
			d[1] = DI_WEST;
		}
		else
		{
			d[1] = DI_NODIR;
		}

		if (deltay < -10 * FRACUNIT)
		{
			d[2] = DI_SOUTH;
		}
		else if (deltay > 10 * FRACUNIT)
		{
			d[2] = DI_NORTH;
		}
		else
		{
			d[2] = DI_NODIR;
		}

		// try direct route
		if (d[1] != DI_NODIR
			&& d[2] != DI_NODIR)
		{
			actor->movedir = diags[((deltay < 0) << 1) + (deltax > 0)];
			if (actor->movedir != turnaround && P_TryWalk(actor))
			{
				return;
			}
		}

		// try other directions
		if (P_Random() > 200
			|| _abs64(deltay) > _abs64(deltax))
		{
			tdir = d[1];
			d[1] = d[2];
			d[2] = tdir;
		}

		if (d[1] == turnaround)
		{
			d[1] = DI_NODIR;
		}
		if (d[2] == turnaround)
		{
			d[2] = DI_NODIR;
		}

		if (d[1] != DI_NODIR)
		{
			actor->movedir = d[1];

			if (P_TryWalk(actor))
			{
				// either moved forward or attacked
				return;
			}
		}

		if (d[2] != DI_NODIR)
		{
			actor->movedir = d[2];

			if (P_TryWalk(actor))
			{
				return;
			}
		}

		// there is no direct path to the player,
		// so pick another direction.
		if (olddir != DI_NODIR)
		{
			actor->movedir = olddir;

			if (P_TryWalk(actor))
			{
				return;
			}
		}

		// randomly determine direction of search
		if (P_Random() & 1)
		{
			for (index_t i = DI_EAST; i <= DI_SOUTHEAST; i++)
			{
				tdir = static_cast<dirtype_e>(i);

				if (tdir != turnaround)
				{
					actor->movedir = tdir;

					if (P_TryWalk(actor))
					{
						return;
					}
				}
			}
		}
		else
		{
			for (index_t i = DI_SOUTHEAST; i != (DI_EAST - 1); i--)
			{
				tdir = static_cast<dirtype_e>(i);

				if (tdir != turnaround)
				{
					actor->movedir = tdir;

					if (P_TryWalk(actor))
					{
						return;
					}
				}
			}
		}

		if (turnaround != DI_NODIR)
		{
			actor->movedir = turnaround;
			if (P_TryWalk(actor))
			{
				return;
			}
		}

		actor->movedir = DI_NODIR;	// can not move
	}
}



//
// P_LookForPlayers
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//
static bool P_LookForPlayers (mobj_t* actor, const bool	allaround)
{
	if (actor)
	{
		size_t c = 0;
		const index_t stop = (actor->lastlook - 1) & 3;

		for (; ; actor->lastlook = (actor->lastlook + 1) & 3)
		{
			if (!::g->players[actor->lastlook].playerInGame)
			{
				continue;
			}

			if (++c == 2 || actor->lastlook == stop)
			{
				// done looking
				return false;
			}

			const player_t* player = &::g->players[actor->lastlook];

			if (player)
			{
				if (player->health <= 0)
				{
					continue; // dead
				}

				if (!P_CheckSight(actor, player->mo))
				{
					continue; // out of sight
				}

				if (!allaround)
				{
					const angle_t angle = R_PointToAngle(actor->x, actor->y, player->mo->x, player->mo->y) - actor->angle;

					if (angle > ANG90 && angle < ANG270)
					{
						const fixed_t dist = P_ApproxDistance(player->mo->x - actor->x, player->mo->y - actor->y);

						// if real close, react anyway
						if (dist > MELEERANGE)
						{
							continue; // behind back
						}
					}
				}

				actor->target = player->mo;

				return true;
			}
		}
	}

	return false;
}


extern "C" {
	//
	// A_KeenDie
	// DOOM II special, map 32.
	// Uses special tag 666.
	//
	void A_KeenDie( mobj_t* mo, void* )
	{
		if (mo)
		{
			line_t	junk = {};

			A_Fall(mo, nullptr);

			// scan the remaining thinkers
			// to see if all Keens are dead
			for (thinker_t* th = ::g->thinkercap.next; th != &::g->thinkercap; th = th->next)
			{
				if (th->function != ACTIONF_T(P_MobjThinker))
				{
					continue;
				}

				const mobj_t* mo2 = reinterpret_cast<mobj_t*>(th);
				if (mo2 != mo
					&& mo2->type == mo->type
					&& mo2->health > 0)
				{
					// other Keen not dead
					return;
				}
			}

			junk.tag = 666;
			EV_DoDoor(&junk, opened);
		}
	}


	//
	// ACTION ROUTINES
	//

	//
	// A_Look
	// Stay in state until a player is sighted.
	//
	void A_Look(mobj_t* actor, void*)
	{
		if (actor)
		{
			bool lookForPlayers = true;

			actor->threshold = 0;	// any shot will wake up
			mobj_t* targ = actor->subsector->sector->soundtarget;

			if (targ && (targ->flags & MF_SHOOTABLE))
			{
				actor->target = targ;

				if (actor->flags & MF_AMBUSH)
				{
					if (P_CheckSight(actor, actor->target))
					{
						lookForPlayers = false;
					}
				}
				else
				{
					lookForPlayers = false;
				}
			}


			if (lookForPlayers && !P_LookForPlayers(actor, false))
			{
				return;
			}

			// go into chase state
			if (actor->info->seesound)
			{
				sfxenum_e sound = sfx_None;

				switch (actor->info->seesound)
				{
				case sfx_posit1:
				case sfx_posit2:
				case sfx_posit3:
					sound = static_cast<sfxenum_e>(sfx_posit1 + P_Random() % 3);
					break;

				case sfx_bgsit1:
				case sfx_bgsit2:
					sound = static_cast<sfxenum_e>(sfx_bgsit1 + P_Random() % 2);
					break;

				default:
					sound = actor->info->seesound;
					break;
				}

				if (actor->type == MT_SPIDER || actor->type == MT_CYBORG)
				{
					// full volume
					S_StartSound(nullptr, sound);
				}
				else
				{
					S_StartSound(actor, sound);
				}
			}

			P_SetMobjState(actor, static_cast<statenum_t>(actor->info->seestate));
		}
	}


	//
	// A_Chase
	// Actor has a melee attack,
	// so it tries to close as fast as possible
	//
	void A_Chase(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (actor->reactiontime)
			{
				actor->reactiontime--;
			}


			// modify target threshold
			if (actor->threshold)
			{
				if (!actor->target
					|| actor->target->health <= 0)
				{
					actor->threshold = 0;
				}
				else
				{
					actor->threshold--;
				}
			}

			// turn towards movement direction if not there yet
			if (actor->movedir < NUMDIRS)
			{
				actor->angle &= (7 << 29);
				const int64 angle_delta = actor->angle - (actor->movedir << 29);

				if (angle_delta > 0)
				{
					actor->angle -= ANG45;
				}
				else if (angle_delta < 0)
				{
					actor->angle += ANG45;
				}
			}

			if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
			{
				// look for a new target
				if (P_LookForPlayers(actor, true))
				{
					return; // got a new target
				}

				P_SetMobjState(actor, actor->info->spawnstate);
				return;
			}

			// do not attack twice in a row
			if (actor->flags & MF_JUSTATTACKED)
			{
				actor->flags &= ~MF_JUSTATTACKED;
				if (::g->gameskill != sk_nightmare && !::g->fastparm)
				{
					P_NewChaseDir(actor);
				}
				return;
			}

			// check for melee attack
			if (actor->info->meleestate && P_CheckMeleeRange(actor))
			{
				if (actor->info->attacksound)
				{
					S_StartSound(actor, actor->info->attacksound);
				}

				P_SetMobjState(actor, actor->info->meleestate);
				return;
			}

			// check for missile attack
			if (actor->info->missilestate)
			{
				bool processMissile = true;

				if (::g->gameskill < sk_nightmare && !::g->fastparm && actor->movecount)
				{
					processMissile = false;
				}

				if (processMissile && !P_CheckMissileRange(actor))
				{
					processMissile = false;
				}

				if (processMissile)
				{
					P_SetMobjState(actor, actor->info->missilestate);
					actor->flags |= MF_JUSTATTACKED;
					return;
				}
			}

			// possibly choose another target
			if (::g->netgame && !actor->threshold && !P_CheckSight(actor, actor->target))
			{
				if (P_LookForPlayers(actor, true))
				{
					return; // got a new target
				}
			}

			// chase towards player
			if (--actor->movecount < 0 || !P_Move(actor))
			{
				P_NewChaseDir(actor);
			}

			// make active sound
			if (actor->info->activesound && P_Random() < 3)
			{
				S_StartSound(actor, actor->info->activesound);
			}
		}
	}


	//
	// A_FaceTarget
	//
	void A_FaceTarget(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			actor->flags &= ~MF_AMBUSH;

			actor->angle = R_PointToAngle(actor->x, actor->y, actor->target->x, actor->target->y);

			if (actor->target->flags & MF_SHADOW)
			{
				actor->angle += (P_Random() - P_Random()) << 21;
			}
		}
	}


	//
	// A_PosAttack
	//
	void A_PosAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);

			angle_t angle = actor->angle;
			const fixed_t slope = P_AimLineAttack(actor, angle, MISSILERANGE);

			S_StartSound(actor, sfx_pistol);

			angle += (P_Random() - P_Random()) << 20;
			const int damage = ((P_Random() % 5) + 1) * 3;

			P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
		}
	}

	void A_SPosAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			S_StartSound(actor, sfx_shotgn);
			A_FaceTarget(actor, nullptr);

			const angle_t bangle = actor->angle;
			const fixed_t slope = P_AimLineAttack(actor, bangle, MISSILERANGE);

			for (size_t i = 0; i < 3; i++)
			{
				const angle_t angle = bangle + ((P_Random() - P_Random()) << 20);
				const int32 damage = ((P_Random() % 5) + 1) * 3;

				P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
			}
		}
	}

	void A_CPosAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			S_StartSound(actor, sfx_shotgn);
			A_FaceTarget(actor, nullptr);

			const angle_t bangle = actor->angle;
			const fixed_t slope = P_AimLineAttack(actor, bangle, MISSILERANGE);

			const angle_t angle = bangle + ((P_Random() - P_Random()) << 20);
			const int damage = ((P_Random() % 5) + 1) * 3;

			P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
		}
	}

	void A_CPosRefire(mobj_t* actor, void*)
	{
		if (actor)
		{
			// keep firing unless target got out of sight
			A_FaceTarget(actor, nullptr);

			if (P_Random() < 40)
			{
				return;
			}

			if (!actor->target
				|| actor->target->health <= 0
				|| !P_CheckSight(actor, actor->target))
			{
				P_SetMobjState(actor, actor->info->seestate);
			}
		}
	}


	void A_SpidRefire(mobj_t* actor, void*)
	{
		if (actor)
		{
			// keep firing unless target got out of sight
			A_FaceTarget(actor, nullptr);

			if (P_Random() < 10)
			{
				return;
			}

			if (!actor->target
				|| actor->target->health <= 0
				|| !P_CheckSight(actor, actor->target))
			{
				P_SetMobjState(actor, actor->info->seestate);
			}
		}
	}

	void A_BspiAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);

			// launch a missile
			P_SpawnMissile(actor, actor->target, MT_ARACHPLAZ);
		}
	}


	//
	// A_TroopAttack
	//
	void A_TroopAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);

			if (P_CheckMeleeRange(actor))
			{
				S_StartSound(actor, sfx_claw);
				const int damage = (P_Random() % 8 + 1) * 3;
				P_DamageMobj(actor->target, actor, actor, damage);
				return;
			}
			// launch a missile
			P_SpawnMissile(actor, actor->target, MT_TROOPSHOT);
		}
	}


	void A_SargAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);

			if (P_CheckMeleeRange(actor))
			{
				const int damage = ((P_Random() % 10) + 1) * 4;
				P_DamageMobj(actor->target, actor, actor, damage);
			}
		}
	}

	void A_HeadAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);

			if (P_CheckMeleeRange(actor))
			{
				const int damage = (P_Random() % 6 + 1) * 10;
				P_DamageMobj(actor->target, actor, actor, damage);
				return;
			}

			// launch a missile
			P_SpawnMissile(actor, actor->target, MT_HEADSHOT);
		}
	}

	void A_CyberAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);

			P_SpawnMissile(actor, actor->target, MT_ROCKET);
		}
	}


	void A_BruisAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			if (P_CheckMeleeRange(actor))
			{
				S_StartSound(actor, sfx_claw);
				const int damage = (P_Random() % 8 + 1) * 10;
				P_DamageMobj(actor->target, actor, actor, damage);
				return;
			}

			// launch a missile
			P_SpawnMissile(actor, actor->target, MT_BRUISERSHOT);
		}
	}


	//
	// A_SkelMissile
	//
	void A_SkelMissile(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);
			actor->z += 16 * FRACUNIT;	// so missile spawns higher
			mobj_t* mo = P_SpawnMissile(actor, actor->target, MT_TRACER);
			actor->z -= 16 * FRACUNIT;	// back to normal

			if (mo)
			{
				mo->x += mo->momx;
				mo->y += mo->momy;
				mo->tracer = actor->target;
			}
		}
	}


	void A_Tracer(mobj_t* actor, void*)
	{
		//if (::g->gametic & 3)
			//return;

		// DHM - Nerve :: Demo fix - Keep the game state deterministic!!!
		if (::g->leveltime & 3) {
			return;
		}

		if (actor)
		{

			// spawn a puff of smoke behind the rocket		
			P_SpawnPuff(actor->x, actor->y, actor->z);

			mobj_t* th = P_SpawnMobj(actor->x - actor->momx, actor->y - actor->momy, actor->z, MT_SMOKE);

			th->momz = FRACUNIT;
			th->tics -= P_Random() & 3;
			th->tics = Max(th->tics, 1);

			// adjust direction
			const mobj_t* dest = actor->tracer;

			if (!dest || dest->health <= 0)
			{
				return;
			}

			// change angle	
			angle_t exact = R_PointToAngle(actor->x, actor->y, dest->x, dest->y);

			if (exact != actor->angle)
			{
				if (exact - actor->angle > ANG180)
				{
					actor->angle -= ::g->TRACEANGLE;
					if (exact - actor->angle < ANG180)
					{
						actor->angle = exact;
					}
				}
				else
				{
					actor->angle += ::g->TRACEANGLE;
					if (exact - actor->angle > ANG180)
					{
						actor->angle = exact;
					}
				}
			}

			actor->momx = (actor->info->speed * finecosine[actor->angle]);
			actor->momy = (actor->info->speed * finesine[actor->angle]);

			// change slope
			fixed_t dist = P_ApproxDistance(dest->x - actor->x, dest->y - actor->y);

			dist = dist / actor->info->speed;

			dist = Max(dist, 1);
			const fixed_t slope = (dest->z + 40 * FRACUNIT - actor->z) / dist;

			if (slope < actor->momz)
			{
				actor->momz -= FRACUNIT / 8;
			}
			else
			{
				actor->momz += FRACUNIT / 8;
			}
		}
	}


	void A_SkelWhoosh(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);

			S_StartSound(actor, sfx_skeswg);
		}
	}

	void A_SkelFist(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);

			if (P_CheckMeleeRange(actor))
			{
				S_StartSound(actor, sfx_skepch);

				const int damage = ((P_Random() % 10) + 1) * 6;
				P_DamageMobj(actor->target, actor, actor, damage);
			}
		}
	}



	//
	// PIT_VileCheck
	// Detect a corpse that could be raised.
	//

	bool PIT_VileCheck( const mobj_t* thing )
	{
		if (thing)
		{
			if (!(thing->flags & MF_CORPSE))
			{
				return true; // not a monster
			}

			if (thing->tics != -1)
			{
				return true; // not lying still yet
			}

			if (thing->info->raisestate == S_NULL)
			{
				return true; // monster doesn't have a raise state
			}

			const auto maxdist = thing->info->radius + mobjinfo[MT_VILE].radius;

			if (std::cmp_greater(_abs64(thing->x - ::g->viletryx), maxdist)
				|| std::cmp_greater(_abs64(thing->y - ::g->viletryy), maxdist))
			{
				return true; // not actually touching
			}

			::g->corpsehit = const_cast<mobj_t*>(thing);
			::g->corpsehit->momx = ::g->corpsehit->momy = 0;
			::g->corpsehit->height <<= 2;
			const bool check = P_CheckPosition(::g->corpsehit, ::g->corpsehit->x, ::g->corpsehit->y);
			::g->corpsehit->height >>= 2;

			if (!check)
			{
				return true; // doesn't fit here
			}
		}

		return false;		// got one, so stop checking
	}



	//
	// A_VileChase
	// Check for resurrecting a body
	//
	void A_VileChase(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (actor->movedir != DI_NODIR)
			{
				// check for corpses to raise
				::g->viletryx = actor->x + actor->info->speed * xspeed[actor->movedir];
				::g->viletryy = actor->y + actor->info->speed * yspeed[actor->movedir];

				const fixed_t xl = (::g->viletryx - ::g->blockmap_origin.x - MAXRADIUS * 2) ;
				fixed_t xh = (::g->viletryx - ::g->blockmap_origin.x + MAXRADIUS * 2) ;
				const fixed_t yl = (::g->viletryy - ::g->blockmap_origin.y - MAXRADIUS * 2) ;
				fixed_t yh = (::g->viletryy - ::g->blockmap_origin.y + MAXRADIUS * 2) ;

				::g->vileobj = actor;
				for (fixed_t bx = xl; bx <= xh; ++bx)
				{
					for (fixed_t by = yl; by <= yh; ++by)
					{
						// Call PIT_VileCheck to check
						// whether object is a corpse
						// that can be raised.
						if (!P_BlockThingsIterator(bx, by, PIT_VileCheck))
						{
							// got one!
							mobj_t* temp = actor->target;
							actor->target = ::g->corpsehit;
							A_FaceTarget(actor, nullptr);
							actor->target = temp;

							P_SetMobjState(actor, S_VILE_HEAL1);
							S_StartSound(::g->corpsehit, sfx_slop);
							const mobjinfo_t* info = ::g->corpsehit->info;

							P_SetMobjState(::g->corpsehit, info->raisestate);
							::g->corpsehit->height <<= 2;
							::g->corpsehit->flags = info->flags;
							::g->corpsehit->health = info->spawnhealth;
							::g->corpsehit->target = nullptr;

							return;
						}
					}
				}
			}

			// Return to normal attack.
			A_Chase(actor, nullptr);
		}
	}


	//
	// A_VileStart
	//
	void A_VileStart(mobj_t* actor, void*)
	{
		if (actor)
		{
			S_StartSound(actor, sfx_vilatk);
		}
	}


	//
	// A_Fire
	// Keep fire in front of player unless out of sight
	//
	void A_Fire(mobj_t* actor, void*)
	{
		if (actor)
		{
			const mobj_t* dest = actor->tracer;
			if (!dest)
			{
				return;
			}

			// don't move it if the vile lost sight
			if (!P_CheckSight(actor->target, dest))
			{
				return;
			}

			P_UnsetThingPosition(actor);
			actor->x = dest->x + (24 * FRACUNIT * finecosine[dest->angle]);
			actor->y = dest->y + (24 * FRACUNIT * finesine[dest->angle]);
			actor->z = dest->z;
			P_SetThingPosition(actor);
		}
	}

	void A_StartFire(mobj_t* actor, void*)
	{
		if (actor)
		{
			S_StartSound(actor, sfx_flamst);
			A_Fire(actor, nullptr);
		}
	}

	void A_FireCrackle(mobj_t* actor, void*)
	{
		if (actor)
		{
			S_StartSound(actor, sfx_flame);
			A_Fire(actor, nullptr);
		}
	}

	//
	// A_VileTarget
	// Spawn the hellfire
	//
	void A_VileTarget(mobj_t* actor, void*)
	{
		if (!actor || !actor->target)
		{
			return;
		}

		A_FaceTarget(actor, nullptr);

		mobj_t* fog = P_SpawnMobj(actor->target->x,
			actor->target->y,                              // FIX: https://doomwiki.org/wiki/Arch-vile_fire_spawned_at_the_wrong_location
			actor->target->z, MT_FIRE);

		if (fog)
		{
			actor->tracer = fog;
			fog->target = actor;
			fog->tracer = actor->target;
			A_Fire(fog, nullptr);
		}
	}




	//
	// A_VileAttack
	//
	void A_VileAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);

			if (!P_CheckSight(actor, actor->target))
			{
				return;
			}

			S_StartSound(actor, sfx_barexp);
			P_DamageMobj(actor->target, actor, actor, 20);
			actor->target->momz = 1000 * FRACUNIT / actor->target->info->mass;

			mobj_t* fire = actor->tracer;

			if (!fire)
			{
				return;
			}

			// move the fire between the vile and the player
			fire->x = actor->target->x - (24 * FRACUNIT * finecosine[actor->angle]);
			fire->y = actor->target->y - (24 * FRACUNIT * finesine[actor->angle]);
			P_RadiusAttack(fire, actor, 70);
		}
	}


	//
	// Mancubus attack,
	// firing three missiles (bruisers)
	// in three different directions?
	// Doesn't look like it. 
	//

	void A_FatRaise(mobj_t* actor, void*)
	{
		if (actor)
		{
			A_FaceTarget(actor, nullptr);
			S_StartSound(actor, sfx_manatk);
		}
	}


	void A_FatAttack1(mobj_t* actor, void*)
	{
		if (actor)
		{
			A_FaceTarget(actor, nullptr);

			// Change direction  to ...
			actor->angle += FATSPREAD;
			std::ignore = P_SpawnMissile(actor, actor->target, MT_FATSHOT);

			mobj_t* mo = P_SpawnMissile(actor, actor->target, MT_FATSHOT);

			if (mo)
			{
				mo->angle += FATSPREAD;
				mo->momx = (actor->info->speed * finecosine[mo->angle]);
				mo->momy = (actor->info->speed * finesine[mo->angle]);
			}
		}
	}

	void A_FatAttack2(mobj_t* actor, void*)
	{
		if (actor)
		{
			A_FaceTarget(actor, nullptr);
			// Now here choose opposite deviation.
			actor->angle -= FATSPREAD;
			std::ignore = P_SpawnMissile(actor, actor->target, MT_FATSHOT);

			mobj_t* mo = P_SpawnMissile(actor, actor->target, MT_FATSHOT);

			if (mo)
			{
				mo->angle -= FATSPREAD * 2;
				mo->momx = (actor->info->speed * finecosine[mo->angle]);
				mo->momy = (actor->info->speed * finesine[mo->angle]);
			}
		}
	}

	void A_FatAttack3(mobj_t* actor, void*)
	{
		if (actor)
		{
			A_FaceTarget(actor, nullptr);

			mobj_t* mo = P_SpawnMissile(actor, actor->target, MT_FATSHOT);

			if (mo)
			{
				mo->angle -= FATSPREAD / 2;
				mo->momx = (actor->info->speed * finecosine[mo->angle]);
				mo->momy = (actor->info->speed * finesine[mo->angle]);
			}

			mo = P_SpawnMissile(actor, actor->target, MT_FATSHOT);

			if (mo)
			{
				mo->angle += FATSPREAD / 2;
				mo->momx = (actor->info->speed * finecosine[mo->angle]);
				mo->momy = (actor->info->speed * finesine[mo->angle]);
			}
		}
	}


	//
	// SkullAttack
	// Fly at the player like a missile.
	//

	void A_SkullAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			const mobj_t* dest = actor->target;
			actor->flags |= MF_SKULLFLY;

			S_StartSound(actor, actor->info->attacksound);
			A_FaceTarget(actor, nullptr);

			actor->momx = (SKULLSPEED * finecosine[actor->angle]);
			actor->momy = (SKULLSPEED * finesine[actor->angle]);
			fixed_t dist = P_ApproxDistance(dest->x - actor->x, dest->y - actor->y);
			dist = dist / SKULLSPEED;

			dist = Max(dist, 1);
			actor->momz = (dest->z + (dest->height >> 1) - actor->z) / dist;
		}
	}


	//
	// A_PainShootSkull
	// Spawn a lost soul and launch it at the target
	//
	void A_PainShootSkull ( mobj_t* actor, const angle_t angle )
	{
		// count total number of skulls currently on the level
		size_t count = 0;

		thinker_t* currentthinker = ::g->thinkercap.next;
		while (currentthinker != &::g->thinkercap)
		{
			if ((currentthinker->function = ACTIONF_T(P_MobjThinker))
				&& reinterpret_cast<mobj_t*>(currentthinker)->type == MT_SKULL)
			{
				count++;
			}
			currentthinker = currentthinker->next;
		}

		// if there are already 20 skulls on the level,
		// don't spit another one
		if (count > 20)
		{
			return;
		}

		if (actor)
		{
			// okay, there's player for another one

			fixed_t prestep = 4 * FRACUNIT + 3 * (actor->info->radius + mobjinfo[MT_SKULL].radius) / 2;

			const fixed_t x = actor->x + (prestep * finecosine[angle]);
			const fixed_t y = actor->y + (prestep * finesine[angle]);
			const fixed_t z = actor->z + 8 * FRACUNIT;

			mobj_t* newmobj = P_SpawnMobj(x, y, z, MT_SKULL);

			// Check for movements.
			if (!P_TryMove(newmobj, newmobj->x, newmobj->y))
			{
				// kill it immediately
				P_DamageMobj(newmobj, actor, actor, 10000);
				return;
			}

			newmobj->target = actor->target;
			A_SkullAttack(newmobj, nullptr);
		}
	}


	//
	// A_PainAttack
	// Spawn a lost soul and launch it at the target
	// 
	void A_PainAttack(mobj_t* actor, void*)
	{
		if (actor)
		{
			if (!actor->target)
			{
				return;
			}

			A_FaceTarget(actor, nullptr);
			A_PainShootSkull(actor, actor->angle);
		}
	}


	void A_PainDie(mobj_t* actor, void*)
	{
		if (actor)
		{
			A_Fall(actor, nullptr);
			A_PainShootSkull(actor, actor->angle + ANG90);
			A_PainShootSkull(actor, actor->angle + ANG180);
			A_PainShootSkull(actor, actor->angle + ANG270);
		}
	}






	void A_Scream(mobj_t* actor, void*)
	{
		if (actor && actor->info)
		{
			sfxenum_e sound = sfx_None;

			switch (actor->info->deathsound)
			{
			case 0:
				return;

			case sfx_podth1:
			case sfx_podth2:
			case sfx_podth3:
				sound = static_cast<sfxenum_e>(sfx_podth1 + P_Random() % 3);
				break;

			case sfx_bgdth1:
			case sfx_bgdth2:
				sound = static_cast<sfxenum_e>(sfx_bgdth1 + P_Random() % 2);
				break;

			default:
				sound = actor->info->deathsound;
				break;
			}

			// Check for bosses.
			if (actor->type == MT_SPIDER || actor->type == MT_CYBORG)
			{
				// full volume
				S_StartSound(nullptr, sound);
			}
			else
			{
				S_StartSound(actor, sound);
			}
		}
	}


	void A_XScream(mobj_t* actor, void*)
	{
		if (actor)
		{
			S_StartSound(actor, sfx_slop);
		}
	}

	void A_Pain(mobj_t* actor, void*)
	{
		if (actor && actor->info)
		{
			if (actor->info->painsound)
			{
				S_StartSound(actor, actor->info->painsound);
			}
		}
	}



	void A_Fall( mobj_t* actor, void* )
	{
		if (actor)
		{
			// actor is on ground, it can be walked over
			actor->flags &= ~MF_SOLID;

			// So change this if corpse objects
			// are meant to be obstacles.
		}
	}


	//
	// A_Explode
	//
	void A_Explode(mobj_t* thingy, void*)
	{
		if (thingy)
		{
			P_RadiusAttack(thingy, thingy->target, 128);
		}
	}


	//
	// A_BossDeath
	// Possibly trigger special effects
	// if on first boss level
	//
	void A_BossDeath(mobj_t* mo, void*)
	{
		if (mo)
		{
			line_t	junk = {};
			size_t	i = 0;

			if (::g->gamemode == commercial)
			{
				if (::g->gamemap != 7)
				{
					return;
				}

				if ((mo->type != MT_FATSO) && (mo->type != MT_BABY))
				{
					return;
				}
			}
			else
			{
				switch (::g->gameepisode)
				{
				case 1:
					if (::g->gamemap != 8)
					{
						return;
					}

					if (mo->type != MT_BRUISER)
					{
						return;
					}
					break;

				case 2:
					if (::g->gamemap != 8)
					{
						return;
					}

					if (mo->type != MT_CYBORG)
					{
						return;
					}
					break;

				case 3:
					if (::g->gamemap != 8)
					{
						return;
					}

					if (mo->type != MT_SPIDER)
					{
						return;
					}

					break;

				case 4:
					switch (::g->gamemap)
					{
					case 6:
						if (mo->type != MT_CYBORG)
						{
							return;
						}
						break;

					case 8:
						if (mo->type != MT_SPIDER)
						{
							return;
						}
						break;

					default:
						return;
						break;
					}
					break;

				default:
					if (::g->gamemap != 8)
					{
						return;
					}
					break;
				}

			}


			// make sure there is a player alive for victory
			for (i = 0; i < ::g->players.Num(); i++)
			{
				if (::g->players[i].playerInGame && ::g->players[i].health > 0)
				{
					break;
				}
			}

			if (i == MAXPLAYERS)
			{
				return; // no one left alive, so do not end game
			}

			// scan the remaining thinkers to see
			// if all bosses are dead
			for (thinker_t* th = ::g->thinkercap.next; th != &::g->thinkercap; th = th->next)
			{
				if (th->function != ::g->braintargets(P_MobjThinker))
				{
					continue;
				}

				const mobj_t* mo2 = reinterpret_cast<mobj_t*>(th);
				if (mo2 && mo2 != mo
					&& mo2->type == mo->type
					&& mo2->health > 0)
				{
					// other boss not dead
					return;
				}
			}

			// victory!
			if (::g->gamemode == commercial)
			{
				if (::g->gamemap == 7)
				{
					if (mo->type == MT_FATSO)
					{
						junk.tag = 666;
						EV_DoFloor(&junk, lowerFloorToLowest);
						return;
					}

					if (mo->type == MT_BABY)
					{
						junk.tag = 667;
						EV_DoFloor(&junk, raiseToTexture);
						return;
					}
				}
			}
			else
			{
				switch (::g->gameepisode)
				{
				case 1:
					junk.tag = 666;
					EV_DoFloor(&junk, lowerFloorToLowest);
					return;
					break;

				case 4:
					switch (::g->gamemap)
					{
					case 6:
						junk.tag = 666;
						EV_DoDoor(&junk, blazeOpen);
						return;
						break;

					case 8:
						junk.tag = 666;
						EV_DoFloor(&junk, lowerFloorToLowest);
						return;
						break;
					default:
						break;
					}
				default:
					break;
				}
			}

			G_ExitLevel();
		}
	}


	void A_Hoof(mobj_t* mo, void*)
	{
		if (mo)
		{
			S_StartSound(mo, sfx_hoof);
			A_Chase(mo, nullptr);
		}
	}

	void A_Metal(mobj_t* mo, void*)
	{
		if (mo)
		{
			S_StartSound(mo, sfx_metal);
			A_Chase(mo, nullptr);
		}
	}

	void A_BabyMetal(mobj_t* mo, void*)
	{
		if (mo)
		{
			S_StartSound(mo, sfx_bspwlk);
			A_Chase(mo, nullptr);
		}
	}

	void A_OpenShotgun2 (player_t* player, pspdef_t* psp)
	{
		if (player)
		{
			if (globalNetworking || (player == &::g->players[::g->consoleplayer]))
			{
				S_StartSound(player->mo, sfx_dbopn);
			}
		}
	}

	void A_LoadShotgun2 (player_t* player, pspdef_t* psp)
	{
		if (player)
		{
			if (globalNetworking || (player == &::g->players[::g->consoleplayer]))
			{
				S_StartSound(player->mo, sfx_dbload);
			}
		}
	}

	void A_ReFire (player_t* player, pspdef_t* psp);

	void A_CloseShotgun2 (player_t* player, pspdef_t* psp)
	{
		if (player)
		{
			if (globalNetworking || (player == &::g->players[::g->consoleplayer]))
			{
				S_StartSound(player->mo, sfx_dbcls);
			}

			if (psp)
			{
				A_ReFire(player, psp);
			}
		}
	}

	void A_BrainAwake(mobj_t* mo, void*)
	{
		// find all the target spots
		::g->easy = 0;
		::g->braintargets.Clear();
		::g->braintargeton = 0;

		thinker_t* thinker = ::g->thinkercap.next;
		for (thinker = ::g->thinkercap.next;
			thinker != &::g->thinkercap;
			thinker = thinker->next)
		{
			if (thinker && thinker->function != ACTIONF_T(P_MobjThinker))
			{
				continue; // not a mobj
			}

			mobj_t* m = reinterpret_cast<mobj_t*>(thinker);

			if (m && m->type == MT_BOSSTARGET)
			{
				::g->braintargets.Append(m);
			}
		}

		S_StartSound(nullptr, sfx_bossit);
	}


	void A_BrainPain(mobj_t* mo, void*)
	{
		S_StartSound(nullptr, sfx_bospn);
	}


	void A_BrainScream(mobj_t* mo, void*)
	{
		if (mo)
		{
			for (fixed_t x = mo->x - 196 * FRACUNIT; x < mo->x + 320 * FRACUNIT; x += FRACUNIT * 8)
			{
				const fixed_t y = mo->y - 320 * FRACUNIT;
				const fixed_t z = 128 + P_Random() * 2 * FRACUNIT;

				mobj_t* th = P_SpawnMobj(x, y, z, MT_ROCKET);

				if (th)
				{
					th->momz = P_Random() * 512;

					P_SetMobjState(th, S_BRAINEXPLODE1);

					th->tics -= P_Random() & 7;
					th->tics = Max(th->tics, 1);
				}
			}

			S_StartSound(nullptr, sfx_bosdth);
		}
	}



	void A_BrainExplode(mobj_t* mo, void*)
	{
		if (mo)
		{
			const fixed_t x = mo->x + (P_Random() - P_Random()) * 2048;
			const fixed_t y = mo->y;
			const fixed_t z = 128 + P_Random() * 2 * FRACUNIT;

			mobj_t* th = P_SpawnMobj(x, y, z, MT_ROCKET);

			if (th)
			{
				th->momz = P_Random() * 512;

				P_SetMobjState(th, S_BRAINEXPLODE1);

				th->tics -= P_Random() & 7;
				th->tics = Max(th->tics, 1);
			}
		}
	}


	void A_BrainDie(mobj_t* mo, void*)
	{
		G_ExitLevel();
	}

	void A_BrainSpit(mobj_t* mo, void*)
	{
		::g->easy ^= 1;
		if (::g->gameskill <= sk_easy && (!::g->easy))
		{
			return;
		}

		if (true) {
			// count number of thinkers
			size_t numCorpse = 0;
			size_t numEnemies = 0;

			for (thinker_t* th = ::g->thinkercap.next; th != &::g->thinkercap; th = th->next) {
				if (th && th->function == ACTIONF_T(P_MobjThinker)) {
					const mobj_t* obj = reinterpret_cast<mobj_t*>(th);

					if (obj)
					{
						if (obj->flags & MF_CORPSE) {
							numCorpse++;
						}
						else if (obj->type > MT_PLAYER && obj->type < MT_KEEN) {
							numEnemies++;
						}
					}
				}
			}

			if (numCorpse > 48) {
				for (size_t i = 0; i < 12; i++) {
					for (thinker_t* th = ::g->thinkercap.next; th != &::g->thinkercap; th = th->next) {
						if (th && th->function == ACTIONF_T(P_MobjThinker)) {
							mobj_t* obj = reinterpret_cast<mobj_t*>(th);

							if (obj)
							{
								if (obj->flags & MF_CORPSE) {
									P_RemoveMobj(obj);
									break;
								}
							}
						}
					}
				}
			}

			if (numEnemies > 32) {
				return;
			}
		}

		// shoot a cube at current target
		mobj_t* targ = ::g->braintargets[::g->braintargeton];
		::g->braintargeton = numeric_cast<BASE_TYPE(::g->braintargeton)>((::g->braintargeton + 1) % ::g->braintargets.Num());

		if (mo && targ)
		{
			// spawn brain missile
			mobj_t* newmobj = P_SpawnMissile(mo, targ, MT_SPAWNSHOT);

			if (newmobj)
			{
				newmobj->target = targ;
				newmobj->reactiontime = ((targ->y - mo->y) / newmobj->momy) / newmobj->state->tics;
			}
		}

		S_StartSound(nullptr, sfx_bospit);
	}



	void A_SpawnFly(mobj_t* mo, void*);

	// travelling cube sound
	void A_SpawnSound(mobj_t* mo, void*)
	{
		if (mo)
		{
			S_StartSound(mo, sfx_boscub);
			A_SpawnFly(mo, nullptr);
		}
	}

	void A_SpawnFly(mobj_t* mo, void*)
	{
		if (mo)
		{
			mobjtype_t	type = {};

			if (--mo->reactiontime)
			{
				return; // still flying
			}

			const mobj_t* targ = mo->target;

			if (targ)
			{
				// First spawn teleport fog.
				mobj_t* fog = P_SpawnMobj(targ->x, targ->y, targ->z, MT_SPAWNFIRE);

				if (fog)
				{
					S_StartSound(fog, sfx_telept);
				}

				// Randomly select monster to spawn.
				const int r = P_Random();

				// Probability distribution (kind of :),
				// decreasing likelihood.
				if (r < 50)
				{
					type = MT_TROOP;
				}
				else if (r < 90)
				{
					type = MT_SERGEANT;
				}
				else if (r < 120)
				{
					type = MT_SHADOWS;
				}
				else if (r < 130)
				{
					type = MT_PAIN;
				}
				else if (r < 160)
				{
					type = MT_HEAD;
				}
				else if (r < 162)
				{
					type = MT_VILE;
				}
				else if (r < 172)
				{
					type = MT_UNDEAD;
				}
				else if (r < 192)
				{
					type = MT_BABY;
				}
				else if (r < 222)
				{
					type = MT_FATSO;
				}
				else if (r < 246)
				{
					type = MT_KNIGHT;
				}
				else
				{
					type = MT_BRUISER;
				}

				mobj_t* newmobj = P_SpawnMobj(targ->x, targ->y, targ->z, type);

				if (newmobj)
				{
					if (P_LookForPlayers(newmobj, true))
					{
						P_SetMobjState(newmobj, static_cast<statenum_t>(newmobj->info->seestate));
					}

					// telefrag anything in this spot
					P_TeleportMove(newmobj, newmobj->x, newmobj->y);
				}
			}

			// remove self (i.e., cube).
			P_RemoveMobj(mo);
		}
	}



	void A_PlayerScream(mobj_t* mo, void*)
	{
		if (mo)
		{
			// Default death sound.
			sfxenum_e sound = sfx_pldeth;

			if ((::g->gamemode == commercial) && (mo->health < -50))
			{
				// IF THE PLAYER DIES
				// LESS THAN -50% WITHOUT GIBBING
				sound = sfx_pdiehi;
			}

			if (::g->demoplayback || globalNetworking || (mo == ::g->players[::g->consoleplayer].mo))
			{
				S_StartSound(mo, sound);
			}
		}
	}

}; // extern "C"

