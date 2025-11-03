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


#include "z_zone.h"
#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "sounds.h"

//
// CEILINGS
//




//
// T_MoveCeiling
//

void T_MoveCeiling (ceiling_t* ceiling)
{
	if (ceiling)
	{
		result_e	res = {};

		switch (ceiling->direction)
		{
		case 0:
			// IN STASIS
			break;
		case 1:
			// UP
			res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->topheight, false, 1, ceiling->direction);

			if (!(::g->leveltime & 7))
			{
				switch (ceiling->type)
				{
				case silentCrushAndRaise:
					break;
				default:
					S_StartSound(&ceiling->sector->soundorg, sfx_stnmov);
					// ?
					break;
				}
			}

			if (res == pastdest)
			{
				switch (ceiling->type)
				{
				case raiseToHighest:
					P_RemoveActiveCeiling(ceiling);
					break;

				case silentCrushAndRaise:
					S_StartSound(&ceiling->sector->soundorg, sfx_pstop);
				case fastCrushAndRaise:
				case crushAndRaise:
					ceiling->direction = -1;
					break;

				default:
					break;
				}

			}
			break;

		case -1:
			// DOWN
			res = T_MovePlane(ceiling->sector, ceiling->speed, ceiling->bottomheight, ceiling->crush, 1, ceiling->direction);

			if (!(::g->leveltime & 7))
			{
				switch (ceiling->type)
				{
				case silentCrushAndRaise: break;
				default:
					S_StartSound(&ceiling->sector->soundorg, sfx_stnmov);
				}
			}

			if (res == pastdest)
			{
				switch (ceiling->type)
				{
				case silentCrushAndRaise:
					S_StartSound(&ceiling->sector->soundorg, sfx_pstop);
				case crushAndRaise:
					ceiling->speed = CEILSPEED;
				case fastCrushAndRaise:
					ceiling->direction = 1;
					break;

				case lowerAndCrush:
				case lowerToFloor:
					P_RemoveActiveCeiling(ceiling);
					break;

				default:
					break;
				}
			}
			else // ( res != pastdest )
			{
				if (res == crushed)
				{
					switch (ceiling->type)
					{
					case silentCrushAndRaise:
					case crushAndRaise:
					case lowerAndCrush:
						ceiling->speed = CEILSPEED / 8;
						break;

					default:
						break;
					}
				}
			}
			break;
		}
	}
}


//
// EV_DoCeiling
// Move a ceiling up/down and all around!
//
bool EV_DoCeiling ( const line_t* line, const ceiling_e type )
{
	bool rtn = false;

	if (line)
	{
		index_t secnum = -1;

		//	Reactivate in-stasis ceilings...for certain types.
		switch (type)
		{
		case fastCrushAndRaise:
		case silentCrushAndRaise:
		case crushAndRaise:
			P_ActivateInStasisCeiling(line);
		default:
			break;
		}

		while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
		{
			sector_t* sec = &::g->sectors[secnum];
			if (sec->specialdata)
			{
				continue;
			}

			// new door thinker
			rtn = true;
			ceiling_t* ceiling = static_cast<ceiling_t*>(DoomLib::Z_Malloc(sizeof(*ceiling), PU_LEVEL, nullptr));
			P_AddThinker(&ceiling->thinker);
			sec->specialdata = ceiling;
			ceiling->thinker.function = ACTIONF_T(T_MoveCeiling);
			ceiling->sector = sec;
			ceiling->crush = false;

			switch (type)
			{
			case fastCrushAndRaise:
				ceiling->crush = true;
				ceiling->topheight = sec->ceilingheight;
				ceiling->bottomheight = sec->floorheight + (8 * FRACUNIT);
				ceiling->direction = -1;
				ceiling->speed = CEILSPEED * 2;
				break;

			case silentCrushAndRaise:
			case crushAndRaise:
				ceiling->crush = true;
				ceiling->topheight = sec->ceilingheight;
			case lowerAndCrush:
			case lowerToFloor:
				ceiling->bottomheight = sec->floorheight;
				if (type != lowerToFloor)
				{
					ceiling->bottomheight += 8 * FRACUNIT;
				}
				ceiling->direction = -1;
				ceiling->speed = CEILSPEED;
				break;

			case raiseToHighest:
				ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
				ceiling->direction = 1;
				ceiling->speed = CEILSPEED;
				break;
			}

			ceiling->tag = sec->tag;
			ceiling->type = type;
			P_AddActiveCeiling(ceiling);
		}
	}

    return rtn;
}


//
// Add an active ceiling
//
void P_AddActiveCeiling( const ceiling_t* c )
{
	if (c)
	{
		for (auto& active_ceiling : ::g->activeceilings)
		{
			if (active_ceiling == nullptr)
			{
				active_ceiling = const_cast<ceiling_t*>(c);
				return;
			}
		}
	}
}



//
// Remove a ceiling's thinker
//
void P_RemoveActiveCeiling( const ceiling_t* c )
{
	for (auto& active_ceiling : ::g->activeceilings)
	{
	if (active_ceiling == c)
	{
		active_ceiling->sector->specialdata = nullptr;
	    P_RemoveThinker (&active_ceiling->thinker);
		active_ceiling = nullptr;
	    break;
	}
    }
}



//
// Restart a ceiling that's in-stasis
//
void P_ActivateInStasisCeiling( const line_t* line )
{
	for (auto& active_ceiling : ::g->activeceilings)
	{
		if (active_ceiling
			&& (active_ceiling->tag == line->tag)
			&& (active_ceiling->direction == 0))
		{
			active_ceiling->direction = active_ceiling->olddirection;
			active_ceiling->thinker.function = ACTIONF_T(T_MoveCeiling);
		}
	}
}



//
// EV_CeilingCrushStop
// Stop a ceiling from crushing!
//
bool EV_CeilingCrushStop( const line_t* line )
{
	bool rtn = false;

	for (auto& active_ceiling : ::g->activeceilings)
	{
		if (active_ceiling
			&& (active_ceiling->tag == line->tag)
			&& (active_ceiling->direction != 0))
		{
			active_ceiling->olddirection = active_ceiling->direction;
			active_ceiling->thinker.function = nullptr;
			active_ceiling->direction = 0;		// in-stasis
			rtn = true;
		}
	}


	return rtn;
}

