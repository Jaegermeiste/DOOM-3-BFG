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

#ifndef __P_LOCAL__
#define __P_LOCAL__

#pragma once

#ifndef __R_LOCAL__
#include "r_local.h"
#endif

constexpr fixed_t FLOATSPEED = (FRACUNIT * 4);


constexpr size_t MAXHEALTH = 200;
constexpr size_t DEFAULTHEALTH = 100;
constexpr size_t MAXARMOR = 200;
constexpr fixed_t VIEWHEIGHT = (41 * FRACUNIT);

// mapblocks are used to check movement
// against lines and things

constexpr size_t MAPBLOCKSHIFT = 7; // 2^7 = 128
constexpr size_t MAPBLOCKUNITS = 1 << MAPBLOCKSHIFT; // 128
constexpr size_t MAPBLOCKSIZE = (MAPBLOCKUNITS * FRACUNIT);
constexpr size_t MAPBMASK = (MAPBLOCKSIZE - 1);
constexpr size_t MAPBTOFRAC = fixed_t::FRACTIONAL_BITS + MAPBLOCKSHIFT; //(MAPBLOCKSHIFT - FRACBITS);

// player radius for movement checking
constexpr fixed_t PLAYERRADIUS = 16 * FRACUNIT;

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
constexpr size_t MAXRADIUS = 32ULL * FRACUNIT;

constexpr fixed_t GRAVITY = FRACUNIT;
constexpr fixed_t MAXMOVE = (30 * FRACUNIT);

constexpr fixed_t USERANGE = (64 * FRACUNIT);
constexpr fixed_t MELEERANGE = (64 * FRACUNIT);
constexpr fixed_t MISSILERANGE = (32 * 64 * FRACUNIT);

// follow a player exclusively for 3 seconds
constexpr ID_TIME_T BASETHRESHOLD = 100;



//
// P_TICK
//

// both the head and tail of the thinker list
extern	thinker_t	thinkercap;	


void P_InitThinkers ();
void P_AddThinker (thinker_t* thinker);
void P_RemoveThinker (thinker_t* thinker);


//
// P_PSPR
//
void P_SetupPsprites (player_t* curplayer);
void P_MovePsprites (player_t* curplayer);
void P_DropWeapon (player_t* player);


//
// P_USER
//
void	P_PlayerThink (player_t* player);


//
// P_MOBJ
//
constexpr fixed_t ONFLOORZ = std::numeric_limits<int32>::min();
constexpr fixed_t ONCEILINGZ = std::numeric_limits<int32>::max();

// Time interval for item respawning.
constexpr ID_TIME_T ITEM_RESPAWN_DELAY = (30 * TICRATE); // 30 seconds


void P_RespawnSpecials ();

mobj_t* P_SpawnMobj ( const fixed_t x, const fixed_t y, const fixed_t z, const mobjtype_t type );

void 	P_RemoveMobj (mobj_t* th);
bool	P_SetMobjState (mobj_t* mobj, const statenum_t state);
void 	P_MobjThinker (mobj_t* mobj);

void	P_SpawnPuff (const fixed_t x, const fixed_t y, const fixed_t z);
void 	P_SpawnBlood (const fixed_t x, const fixed_t y, const fixed_t z, const int damage);
mobj_t* P_SpawnMissile (mobj_t* source, mobj_t* dest, const mobjtype_t type);
void	P_SpawnPlayerMissile (mobj_t* source, const mobjtype_t type);


//
// P_ENEMY
//
void P_NoiseAlert ( const mobj_t* target, mobj_t* emitter );


//
// P_MAPUTL
//
typedef struct divline_s
{
    fixed_t	x;
    fixed_t	y;
    fixed_t	dx;
    fixed_t	dy;
    
} divline_t;

typedef struct intercept_s
{
    fixed_t	frac;		// along trace line
    bool	isaline;
    union {
	mobj_t*	thing;
	line_t*	line;
    }			d;
} intercept_t;

constexpr size_t MAXINTERCEPTS = 128;

extern intercept_t	intercepts[MAXINTERCEPTS];
extern intercept_t*	intercept_p;

typedef bool (*traverser_t) (intercept_t *in);

fixed_t P_ApproxDistance ( const fixed_t dx, const fixed_t dy );
int8 	P_PointOnLineSide ( const fixed_t x, const fixed_t y, const line_t* line );
int8 	P_PointOnDivlineSide ( const fixed_t x, const fixed_t y, const divline_t* line );
void 	P_MakeDivline (const line_t* li, divline_t* dl );
fixed_t P_InterceptVector ( divline_t* v2, divline_t* v1 );
int8 	P_BoxOnLineSide (const fixed_t* tmbox, line_t* ld );

extern fixed_t		opentop;
extern fixed_t 		openbottom;
extern fixed_t		openrange;
extern fixed_t		lowfloor;

void 	P_LineOpening (const line_t* linedef);

template <class T>
	requires std::same_as<std::remove_const_t<T>, line_t>
static bool P_BlockLinesIterator ( const int32 x, const int32 y, bool(*func)(T*) );

template <class T>
	requires std::same_as<std::remove_const_t<T>, mobj_t>
static bool P_BlockThingsIterator ( const int32 x, const int32 y, bool(*func)(T*) );

enum PT_e : uint8
{
	PT_ADDLINES = 1,
	PT_ADDTHINGS = 2,
	PT_EARLYOUT = 4
};

extern divline_t	trace;

bool P_PathTraverse ( const fixed_t x1, const fixed_t y1, const fixed_t x2, const fixed_t y2, const int flags, bool (*trav) (intercept_t *) );

void P_UnsetThingPosition (const mobj_t* thing );
void P_SetThingPosition ( mobj_t* thing );


//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern bool		floatok;
extern fixed_t		tmfloorz;
extern fixed_t		tmceilingz;


extern	line_t*		ceilingline;

bool P_CheckPosition ( const mobj_t *thing, const fixed_t x, const fixed_t y );
bool P_TryMove ( mobj_t* thing, const fixed_t x, const fixed_t y );
bool P_TeleportMove ( mobj_t* thing, const fixed_t x, const fixed_t y );
void P_SlideMove ( mobj_t* mo );
bool P_CheckSight ( const mobj_t* t1, const mobj_t* t2 );
void P_UseLines ( player_t* player );

bool P_ChangeSector ( sector_t* sector, const bool crunch );

extern mobj_t*	linetarget;	// who got hit (or NULL)

fixed_t P_AimLineAttack ( mobj_t* t1, const angle_t angle, const fixed_t distance );

void P_LineAttack ( mobj_t* t1, const angle_t angle, const fixed_t distance, const fixed_t slope, const int damage );

void P_RadiusAttack ( mobj_t* spot, const mobj_t* source, const int damage );



//
// P_SETUP
//


//
// P_INTER
//
void P_TouchSpecialThing ( mobj_t* special, const mobj_t* toucher );

void P_DamageMobj ( mobj_t* target, const mobj_t* inflictor, mobj_t* source, const int damage );


//
// P_SPEC
//
#include "p_spec.h"


#endif	// __P_LOCAL__



