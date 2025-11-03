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
#include <utility>

#include "Precompiled.h"
#include "globaldata.h"


#include "z_zone.h"
#include "m_random.h"

#include "doomdef.h"
#include "p_local.h"


// State.
#include "r_state.h"

//
// FIRELIGHT FLICKER
//

//
// T_FireFlicker
//
void T_FireFlicker(fireflicker_t* flick)
{
	if (--flick->count)
	{
		return;
	}

	const auto amount = (P_Random() & 3) * 16;

	if (flick->sector->lightlevel - amount < flick->minlight)
	{
		flick->sector->lightlevel = flick->minlight;
	}
	else
	{
		flick->sector->lightlevel = numeric_cast<BASE_TYPE(flick->sector->lightlevel)>(flick->maxlight - amount);
	}

	flick->count = 4;
}



//
// P_SpawnFireFlicker
//
void P_SpawnFireFlicker(sector_t* sector)
{
	// Note that we are resetting sector attributes.
	// Nothing special about it during gameplay.
	sector->special = 0;

	fireflicker_t* flick = static_cast<fireflicker_t*>(DoomLib::Z_Malloc(sizeof(*flick), PU_LEVEL, nullptr));

	P_AddThinker(&flick->thinker);

	flick->thinker.function = ACTIONF_T(T_FireFlicker);
	flick->sector = sector;
	flick->maxlight = sector->lightlevel;
	flick->minlight = numeric_cast<BASE_TYPE(flick->minlight)>(P_FindMinSurroundingLight(sector, sector->lightlevel) + 16);
	flick->count = 4;
}



//
// BROKEN LIGHT FLASHING
//


//
// T_LightFlash
// Do flashing lights.
//
void T_LightFlash(lightflash_t* flash)
{
	if (--flash->count)
	{
		return;
	}

	if (flash->sector->lightlevel == flash->maxlight)
	{
		flash->sector->lightlevel = flash->minlight;
		flash->count = (P_Random() & flash->mintime) + 1;
	}
	else
	{
		flash->sector->lightlevel = flash->maxlight;
		flash->count = (P_Random() & flash->maxtime) + 1;
	}

}




//
// P_SpawnLightFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void P_SpawnLightFlash(sector_t* sector)
{
	// nothing special about it during gameplay
	sector->special = 0;

	lightflash_t* flash = static_cast<lightflash_t*>(DoomLib::Z_Malloc(sizeof(*flash), PU_LEVEL, nullptr));

	P_AddThinker(&flash->thinker);

	flash->thinker.function = ACTIONF_T(T_LightFlash);
	flash->sector = sector;
	flash->maxlight = sector->lightlevel;

	flash->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);
	flash->maxtime = 64;
	flash->mintime = 7;
	flash->count = (P_Random() & flash->maxtime) + 1;
}



//
// STROBE LIGHT FLASHING
//


//
// T_StrobeFlash
//
void T_StrobeFlash(strobe_t* flash)
{
	if (--flash->count)
	{
		return;
	}

	if (flash->sector->lightlevel == flash->minlight)
	{
		flash->sector->lightlevel = flash->maxlight;
		flash->count = flash->brighttime;
	}
	else
	{
		flash->sector->lightlevel = flash->minlight;
		flash->count = flash->darktime;
	}

}



//
// P_SpawnStrobeFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void P_SpawnStrobeFlash ( sector_t* sector, const int fastOrSlow, const int inSync)
{
	strobe_t* flash = static_cast<strobe_t*>(DoomLib::Z_Malloc(sizeof(*flash), PU_LEVEL, nullptr));

	P_AddThinker(&flash->thinker);

	flash->sector = sector;
	flash->darktime = fastOrSlow;
	flash->brighttime = STROBEBRIGHT;
	flash->thinker.function = ACTIONF_T(T_StrobeFlash);
	flash->maxlight = sector->lightlevel;
	flash->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);

	if (flash->minlight == flash->maxlight)
	{
		flash->minlight = 0;
	}

	// nothing special about it during gameplay
	sector->special = 0;

	if (!inSync)
	{
		flash->count = (P_Random() & 7) + 1;
	}
	else
	{
		flash->count = 1;
	}
}


//
// Start strobing lights (usually from a trigger)
//
void EV_StartLightStrobing(const line_t* line)
{
	index_t secnum = -1;

	while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
	{
		sector_t* sec = &::g->sectors[secnum];

		if (sec->specialdata)
		{
			continue;
		}

		P_SpawnStrobeFlash(sec, SLOWDARK, 0);
	}
}



//
// TURN LINE'S TAG LIGHTS OFF
//
void EV_TurnTagLightsOff(const line_t* line)
{
	if (line)
	{
		for (index_t j = 0; std::cmp_less(j, ::g->sectors.Num()); j++)
		{
			if (::g->sectors[j]->tag == line->tag)
			{
				int16 min = ::g->sectors[j]->lightlevel;

				for (index_t i = 0; std::cmp_less(i, ::g->sectors[j]->linecount); i++)
				{
					sector_t* tsec = getNextSector(::g->sectors[j]->lines[i], ::g->sectors[j]);

					if (!tsec)
					{
						continue;
					}

					min = Min(tsec->lightlevel, min);
				}

				::g->sectors[j]->lightlevel = min;
			}
		}
	}
}


//
// TURN LINE'S TAG LIGHTS ON
//
void EV_LightTurnOn (line_t* line, int16 bright)
{
	for (index_t i = 0; std::cmp_less(i, ::g->sectors.Num()); i++)
	{
		if (::g->sectors[i]->tag == line->tag)
		{
			// bright = 0 means to search
			// for highest light level
			// surrounding sector
			if (!bright)
			{
				for (index_t j = 0; std::cmp_less(j, ::g->sectors[i]->linecount); j++)
				{
					sector_t* temp = getNextSector(::g->sectors[i]->lines[j], ::g->sectors[i]);

					if (!temp)
					{
						continue;
					}

					bright = Max(temp->lightlevel, bright);
				}
			}

			::g->sectors[i]->lightlevel = bright;
		}
	}
}


//
// Spawn glowing light
//

void T_Glow(glow_t* g)
{
	switch (g->direction)
	{
	case -1:
		// DOWN
		g->sector->lightlevel -= GLOWSPEED;
		if (g->sector->lightlevel <= g->minlight)
		{
			g->sector->lightlevel += GLOWSPEED;
			g->direction = 1;
		}
		break;

	case 1:
		// UP
		g->sector->lightlevel += GLOWSPEED;
		if (g->sector->lightlevel >= g->maxlight)
		{
			g->sector->lightlevel -= GLOWSPEED;
			g->direction = -1;
		}
		break;
	default:
		break;
	}
}


void P_SpawnGlowingLight(sector_t* sector)
{
	glow_t* g = static_cast<glow_t*>(DoomLib::Z_Malloc(sizeof(*g), PU_LEVEL, nullptr));

	P_AddThinker(&g->thinker);

	g->sector = sector;
	g->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);
	g->maxlight = sector->lightlevel;
	g->thinker.function = ACTIONF_T(T_Glow);
	g->direction = -1;

	sector->special = 0;
}


