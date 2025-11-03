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
#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

// State.
#include "doomstat.h"
#include "r_state.h"
// Data.
#include "sounds.h"


//
// FLOORS
//

//
// Move a plane (floor or ceiling) and check for crushing
//
result_e T_MovePlane(sector_t* sector, const fixed_t speed, const fixed_t dest, const bool crush, const int floorOrCeiling, const int direction)
{
	if (sector)
	{
		bool	flag = false;
		fixed_t	lastpos = 0;

		switch (floorOrCeiling)
		{
		case 0:
			// FLOOR
			switch (direction)
			{
			case -1:
				// DOWN
				if (sector->floorheight - speed < dest)
				{
					lastpos = sector->floorheight;
					sector->floorheight = dest;
					flag = P_ChangeSector(sector, crush);
					if (flag == true)
					{
						sector->floorheight = lastpos;
						P_ChangeSector(sector, crush);
						//return crushed;
					}
					return pastdest;
				}
				else
				{
					lastpos = sector->floorheight;
					sector->floorheight -= speed;
					flag = P_ChangeSector(sector, crush);
					if (flag == true)
					{
						sector->floorheight = lastpos;
						P_ChangeSector(sector, crush);
						return crushed;
					}
				}
				break;

			case 1:
				// UP
				if (sector->floorheight + speed > dest)
				{
					lastpos = sector->floorheight;
					sector->floorheight = dest;
					flag = P_ChangeSector(sector, crush);
					if (flag == true)
					{
						sector->floorheight = lastpos;
						P_ChangeSector(sector, crush);
						//return crushed;
					}
					return pastdest;
				}
				else
				{
					// COULD GET CRUSHED
					lastpos = sector->floorheight;
					sector->floorheight += speed;
					flag = P_ChangeSector(sector, crush);
					if (flag == true)
					{
						if (crush == true)
						{
							return crushed;
						}
						sector->floorheight = lastpos;
						P_ChangeSector(sector, crush);
						return crushed;
					}
				}
				break;
			default:
				break;
			}
			break;

		case 1:
			// CEILING
			switch (direction)
			{
			case -1:
				// DOWN
				if (sector->ceilingheight - speed < dest)
				{
					lastpos = sector->ceilingheight;
					sector->ceilingheight = dest;
					flag = P_ChangeSector(sector, crush);

					if (flag == true)
					{
						sector->ceilingheight = lastpos;
						P_ChangeSector(sector, crush);
						//return crushed;
					}
					return pastdest;
				}
				else
				{
					// COULD GET CRUSHED
					lastpos = sector->ceilingheight;
					sector->ceilingheight -= speed;
					flag = P_ChangeSector(sector, crush);

					if (flag == true)
					{
						if (crush == true)
						{
							return crushed;
						}
						sector->ceilingheight = lastpos;
						P_ChangeSector(sector, crush);
						return crushed;
					}
				}
				break;

			case 1:
				// UP
				if (sector->ceilingheight + speed > dest)
				{
					lastpos = sector->ceilingheight;
					sector->ceilingheight = dest;
					flag = P_ChangeSector(sector, crush);
					if (flag == true)
					{
						sector->ceilingheight = lastpos;
						P_ChangeSector(sector, crush);
						//return crushed;
					}
					return pastdest;
				}
				else
				{
					lastpos = sector->ceilingheight;
					sector->ceilingheight += speed;
					flag = P_ChangeSector(sector, crush);
					// UNUSED
#if 0
					if (flag == true)
					{
						sector->ceilingheight = lastpos;
						P_ChangeSector(sector, crush);
						return crushed;
					}
#endif
				}
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		return ok;
	}

	return error;
}


//
// MOVE A FLOOR TO ITS DESTINATION (UP OR DOWN)
//
void T_MoveFloor(floormove_t* floor)
{
	if (floor)
	{
		result_e res = T_MovePlane(floor->sector,
		                           floor->speed,
		                           floor->floordestheight,
		                           floor->crush, 0, floor->direction);

		if (!(::g->leveltime & 7))
		{
			S_StartSound(&floor->sector->soundorg, sfx_stnmov);
		}

		if (res == pastdest)
		{
			floor->sector->specialdata = nullptr;

			if (floor->direction == 1)
			{
				switch (floor->type)
				{
				case donutRaise:
					floor->sector->special = floor->newspecial;
					floor->sector->floorpic = floor->texture;
				default:
					break;
				}
			}
			else if (floor->direction == -1)
			{
				switch (floor->type)
				{
				case lowerAndChange:
					floor->sector->special = floor->newspecial;
					floor->sector->floorpic = floor->texture;
				default:
					break;
				}
			}

			P_RemoveThinker(&floor->thinker);

			S_StartSound(&floor->sector->soundorg, sfx_pstop);
		}
	}
}

//
// HANDLE FLOOR TYPES
//
bool EV_DoFloor ( const line_t* line, const floor_e floortype )
{
	bool rtn = false;

	if (line)
	{
		index_t secnum = -1;

		while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
		{
			sector_t* sec = &::g->sectors[secnum];

			if (sec)
			{
				// ALREADY MOVING?  IF SO, KEEP GOING...
				if (sec->specialdata)
				{
					continue;
				}

				// new floor thinker
				rtn = true;
				floormove_t* floor = static_cast<floormove_t*>(DoomLib::Z_Malloc(sizeof(*floor), PU_LEVEL, nullptr));

				if (floor)
				{
					index_t i = 0;

					P_AddThinker(&floor->thinker);
					sec->specialdata = floor;
					floor->thinker.function = ACTIONF_T(T_MoveFloor);
					floor->type = floortype;
					floor->crush = false;

					switch (floortype)
					{
					case lowerFloor:
						floor->direction = -1;
						floor->sector = sec;
						floor->speed = FLOORSPEED;
						floor->floordestheight = P_FindHighestFloorSurrounding(sec);
						break;

					case lowerFloorToLowest:
						floor->direction = -1;
						floor->sector = sec;
						floor->speed = FLOORSPEED;
						floor->floordestheight = P_FindLowestFloorSurrounding(sec);
						break;

					case turboLower:
						floor->direction = -1;
						floor->sector = sec;
						floor->speed = FLOORSPEED * 4;
						floor->floordestheight = P_FindHighestFloorSurrounding(sec);
						if (floor->floordestheight != sec->floorheight)
						{
							floor->floordestheight += 8 * FRACUNIT;
						}
						break;

					case raiseFloorCrush:
						floor->crush = true;
					case raiseFloor:
						floor->direction = 1;
						floor->sector = sec;
						floor->speed = FLOORSPEED;
						floor->floordestheight = P_FindLowestCeilingSurrounding(sec);
						floor->floordestheight = Min(floor->floordestheight, sec->ceilingheight);
						floor->floordestheight -= (8 * FRACUNIT) * (floortype == raiseFloorCrush);
						break;

					case raiseFloorTurbo:
						floor->direction = 1;
						floor->sector = sec;
						floor->speed = FLOORSPEED * 4;
						floor->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
						break;

					case raiseFloorToNearest:
						floor->direction = 1;
						floor->sector = sec;
						floor->speed = FLOORSPEED;
						floor->floordestheight = P_FindNextHighestFloor(sec, sec->floorheight);
						break;

					case raiseFloor24:
						floor->direction = 1;
						floor->sector = sec;
						floor->speed = FLOORSPEED;
						floor->floordestheight = floor->sector->floorheight + 24 * FRACUNIT;
						break;
					case raiseFloor512:
						floor->direction = 1;
						floor->sector = sec;
						floor->speed = FLOORSPEED;
						floor->floordestheight = floor->sector->floorheight + 512 * FRACUNIT;
						break;

					case raiseFloor24AndChange:
						floor->direction = 1;
						floor->sector = sec;
						floor->speed = FLOORSPEED;
						floor->floordestheight = floor->sector->floorheight + 24 * FRACUNIT;
						sec->floorpic = line->frontsector->floorpic;
						sec->special = line->frontsector->special;
						break;

					case raiseToTexture:
					{
						fixed_t minsize = fixed_t::MAX;

						floor->direction = 1;
						floor->sector = sec;
						floor->speed = FLOORSPEED;

						for (i = 0; std::cmp_less(i, sec->linecount); ++i)
						{
							if (twoSided(secnum, i))
							{
								side_t* side = getSide(secnum, i, 0);

								if (side)
								{
									if (side->bottomtexture >= 0)
									{
										minsize = Min(::g->s_textureheight[side->bottomtexture], minsize);
									}
								}

								side = getSide(secnum, i, 1);

								if (side)
								{
									if (side->bottomtexture >= 0)
									{
										minsize = Min(::g->s_textureheight[side->bottomtexture], minsize);
									}
								}
							}
						}
						floor->floordestheight =
							floor->sector->floorheight + minsize;
					}
					break;

					case lowerAndChange:
						floor->direction = -1;
						floor->sector = sec;
						floor->speed = FLOORSPEED;
						floor->floordestheight =
							P_FindLowestFloorSurrounding(sec);
						floor->texture = sec->floorpic;

						for (i = 0; std::cmp_less(i, sec->linecount); ++i)
						{
							if (twoSided(secnum, i))
							{
								if (getSide(secnum, i, 0)->sector - ::g->sectors == secnum)
								{
									sec = getSector(secnum, i, 1);

									if (sec->floorheight == floor->floordestheight)
									{
										floor->texture = sec->floorpic;
										floor->newspecial = sec->special;
										break;
									}
								}
								else
								{
									sec = getSector(secnum, i, 0);

									if (sec->floorheight == floor->floordestheight)
									{
										floor->texture = sec->floorpic;
										floor->newspecial = sec->special;
										break;
									}
								}
							}
						}
					default:
						break;
					}
				}
			}
		}
	}
    return rtn;
}




//
// BUILD A STAIRCASE!
//
bool EV_BuildStairs ( const line_t* line, const stair_e type )
{
	bool rtn = false;

	if (line)
	{
		index_t secnum = -1;

		while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
		{
			sector_t* sec = &::g->sectors[secnum];

			if (sec)
			{
				// ALREADY MOVING?  IF SO, KEEP GOING...
				if (sec->specialdata)
				{
					continue;
				}

				// new floor thinker
				rtn = true;
				floormove_t* floor = static_cast<floormove_t*>(DoomLib::Z_Malloc(sizeof(*floor), PU_LEVEL, nullptr));

				if (floor)
				{
					fixed_t stairsize = 0;
					fixed_t speed = 0;

					P_AddThinker(&floor->thinker);
					sec->specialdata = floor;
					floor->thinker.function = ACTIONF_T(T_MoveFloor);
					floor->direction = 1;
					floor->sector = sec;

					switch (type)
					{
					case build8:
						speed = FLOORSPEED / 4;
						stairsize = 8 * FRACUNIT;
						break;
					case turbo16:
						speed = FLOORSPEED * 4;
						stairsize = 16 * FRACUNIT;
						break;
					}

					floor->speed = speed;
					fixed_t height = sec->floorheight + stairsize;
					floor->floordestheight = height;

					index_t texture = sec->floorpic;

					// Find next sector to raise
					// 1.	Find 2-sided line with same sector side[0]
					// 2.	Other side is the next sector to raise
					bool ok = false;
					do
					{
						ok = false;

						for (index_t i = 0; std::cmp_less(i, sec->linecount); ++i)
						{
							if (!((sec->lines[i])->flags & ML_TWOSIDED))
							{
								continue;
							}

							sector_t* tsec = (sec->lines[i])->frontsector;

							if (tsec)
							{
								auto newsecnum = tsec - ::g->sectors;

								if (secnum != newsecnum)
								{
									continue;
								}

								tsec = (sec->lines[i])->backsector;

								if (tsec)
								{
									newsecnum = tsec - ::g->sectors;

									if (tsec->floorpic != texture)
									{
										continue;
									}

									height += stairsize;

									if (tsec->specialdata)
									{
										continue;
									}

									sec = tsec;
									secnum = newsecnum;
									floor = static_cast<floormove_t*>(DoomLib::Z_Malloc(sizeof(*floor), PU_LEVEL, nullptr));

									if (floor)
									{
										P_AddThinker(&floor->thinker);

										sec->specialdata = floor;
										floor->thinker.function = ACTIONF_T(T_MoveFloor);
										floor->direction = 1;
										floor->sector = sec;
										floor->speed = speed;
										floor->floordestheight = height;

										ok = true;
									}
								}
							}
							break;
						}
					} while (ok);
				}
			}
		}
	}

    return rtn;
}


