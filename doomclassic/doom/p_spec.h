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

#ifndef __P_SPEC__
#define __P_SPEC__

#pragma once

//
// End-level timer (-TIMER option)
//
extern	bool levelTimer;
extern	ID_TIME_T	levelTimeCount;


//      Define values for map objects
constexpr auto MO_TELEPORTMAN = 14;


// at game start
void P_InitPicAnims ();

// at map load
void P_SpawnSpecials ();

// every tic
void P_UpdateSpecials ();

// when needed
bool P_UseSpecialLine ( mobj_t* thing, line_t* line, int8 side );

void P_ShootSpecialLine (const mobj_t* thing, line_t* line );

void P_CrossSpecialLine ( const index_t linenum, const int8 side, mobj_t* thing );

void    P_PlayerInSpecialSector (player_t* player);

static bool twoSided ( const index_t sectorIndex, const index_t lineIndex );

sector_t* getSector ( const index_t	currentSector, const index_t line, const index_t side );

side_t* getSide ( const index_t currentSector, const index_t line, const index_t side );

fixed_t P_FindLowestFloorSurrounding( const sector_t* sec);
fixed_t P_FindHighestFloorSurrounding( const sector_t* sec);

fixed_t P_FindNextHighestFloor (const sector_t*	sec, int		currentheight );

fixed_t P_FindLowestCeilingSurrounding( const sector_t* sec );
fixed_t P_FindHighestCeilingSurrounding( const sector_t* sec );

static index_t P_FindSectorFromLineTag ( const line_t* line, const index_t start );

int16 P_FindMinSurroundingLight ( sector_t*	sector, const int16	max );

sector_t* getNextSector ( const line_t* line, const sector_t*	sec );


//
// SPECIAL
//
int EV_DoDonut( const line_t* line );



//
// P_LIGHTS
//
typedef struct fireflicker_s
{
    thinker_t	thinker;
    sector_t*	sector;
    size_t		count;
    int16		maxlight;
	int16		minlight;
    
} fireflicker_t;



typedef struct lightflash_s
{
    thinker_t	thinker;
    sector_t*	sector;
    size_t		count;
	int16		maxlight;
	int16		minlight;
    ID_TIME_T	maxtime;
	ID_TIME_T	mintime;
    
} lightflash_t;



typedef struct strobe_s
{
    thinker_t	thinker;
    sector_t*	sector;
    size_t		count;
	int16		minlight;
	int16		maxlight;
	ID_TIME_T	darktime;
	ID_TIME_T	brighttime;
    
} strobe_t;




typedef struct glow_s
{
    thinker_t	thinker;
    sector_t*	sector;
	int16		minlight;
	int16		maxlight;
    int8	    direction;

} glow_t;


constexpr auto GLOWSPEED    = 8;
constexpr auto STROBEBRIGHT = 5;
constexpr auto FASTDARK     = 15;
constexpr auto SLOWDARK     = 35;

void	T_FireFlicker (fireflicker_t* flick);
void    P_SpawnFireFlicker (sector_t* sector);
void    T_LightFlash (lightflash_t* flash);
void    P_SpawnLightFlash (sector_t* sector);
void    T_StrobeFlash (strobe_t* flash);

void P_SpawnStrobeFlash ( sector_t*	sector, const int fastOrSlow, const int inSync );

void    EV_StartLightStrobing(const line_t* line);
void    EV_TurnTagLightsOff(const line_t* line);

void EV_LightTurnOn ( const line_t*	line, const int bright );

void    T_Glow( glow_t* g );
void    P_SpawnGlowingLight( sector_t* sector );




//
// P_SWITCH
//
typedef struct switchlist_s
{
    char	name1[9];
    char	name2[9];
    short	episode;
    
} switchlist_t;


typedef enum : uint8
{
    top,
    middle,
    bottom

} bwhere_e;


typedef struct button_s
{
    line_t*	line;
    bwhere_e	where;
    index_t		btexture;
    int		btimer;
	union {
		mobj_t *		soundorg;
		degenmobj_t *	degensoundorg;
	};
} button_t;




 // max # of wall switches in a level
constexpr size_t MAXSWITCHES = 50;

 // 4 players, 4 buttons each at once, max.
constexpr size_t MAXBUTTONS = 16;

 // 1 second, in ticks. 
constexpr ID_TIME_T BUTTONTIME = TICRATE;

extern button_t	buttonlist[MAXBUTTONS]; 

void
P_ChangeSwitchTexture
( line_t*	line,
  int		useAgain );

void P_InitSwitchList();


//
// P_PLATS
//
typedef enum : uint8
{
    up,
    down,
    waiting,
    in_stasis

} plat_e;



typedef enum : uint8
{
    perpetualRaise,
    downWaitUpStay,
    raiseAndChange,
    raiseToNearestAndChange,
    blazeDWUS

} plattype_e;



typedef struct plat_s
{
    thinker_t	thinker;
    sector_t*	sector;
    fixed_t	speed;
    fixed_t	low;
    fixed_t	high;
    ID_TIME_T		wait;
    size_t		count;
    plat_e	status;
    plat_e	oldstatus;
    bool	crush;
    int		tag;
    plattype_e	type;
    
} plat_t;



constexpr auto PLATWAIT = 3;
constexpr auto PLATSPEED = FRACUNIT;
//constexpr size_t MAXPLATS = 30;


//extern plat_t*	activeplats[MAXPLATS];

void    T_PlatRaise(plat_t*	plat);

int
EV_DoPlat
( line_t*	line,
  plattype_e	type,
  int		amount );

void    P_AddActivePlat(plat_t* plat);
void    P_RemoveActivePlat(plat_t* plat);
void    EV_StopPlat(line_t* line);
void    P_ActivateInStasis(int tag);


//
// P_DOORS
//
typedef enum : uint8
{
    normal,
    close30ThenOpen,
    closed,
    opened,
    raiseIn5Mins,
    blazeRaise,
    blazeOpen,
    blazeClose,

	MAX_VL_DOOR_TYPES

} vldoor_e;



typedef struct vldoor_s
{
    thinker_t	    thinker;
    vldoor_e	    type;
    sector_t*	    sector;
    fixed_t	        topheight;
    fixed_t	        speed;

    // 1 = up, 0 = waiting at top, -1 = down
    int8            direction;
    
    // tics to wait at the top
    ID_TIME_T       topwait;
    // (keep in case a door going down is reset)
    // when it reaches 0, start going down
	ID_TIME_T       topcountdown;
} vldoor_t;



constexpr auto VDOORSPEED = FRACUNIT * 2;
constexpr ID_TIME_T VDOORWAIT = 150;

void EV_VerticalDoor ( line_t* line, mobj_t* thing );

bool EV_DoDoor ( const line_t* line, const vldoor_e type );

bool EV_DoLockedDoor ( const line_t* line, const vldoor_e type, mobj_t* thing );

void T_VerticalDoor (vldoor_t* door);

void P_SpawnDoorCloseIn30 ( sector_t* sec );

void P_SpawnDoorRaiseIn5Mins ( sector_t* sec );



#if 0 // UNUSED
//
//      Sliding doors...
//
typedef enum : uint8
{
    sd_opening,
    sd_waiting,
    sd_closing

} sd_e;



typedef enum : uint8
{
    sdt_openOnly,
    sdt_closeOnly,
    sdt_openAndClose

} sdt_e;




typedef struct slidedoor_s
{
    thinker_t	thinker;
    sdt_e	type;
    line_t*	line;
    index_t		frame;
    index_t		whichDoorIndex;
    ID_TIME_T		timer;
    sector_t*	frontsector;
    sector_t*	backsector;
    sd_e	 status;

} slidedoor_t;



typedef struct slidename_s
{
    char	frontFrame1[9];
    char	frontFrame2[9];
    char	frontFrame3[9];
    char	frontFrame4[9];
    char	backFrame1[9];
    char	backFrame2[9];
    char	backFrame3[9];
    char	backFrame4[9];
    
} slidename_t;



typedef struct slideframe_s
{
    index_t             frontFrames[4];
    index_t             backFrames[4];

} slideframe_t;



// how many frames of animation
constexpr auto SNUMFRAMES = 4;
constexpr ID_TIME_T SDOORWAIT = TICRATE * 3;
constexpr ID_TIME_T SWAITTICS = 4;

// how many diff. types of anims
constexpr size_t MAXSLIDEDOORS = 5;

void P_InitSlidingDoorFrames();

void
EV_SlidingDoor
( line_t*	line,
  mobj_t*	thing );
#endif



//
// P_CEILING
//
typedef enum : uint8
{
    lowerToFloor,
    raiseToHighest,
    lowerAndCrush,
    crushAndRaise,
    fastCrushAndRaise,
    silentCrushAndRaise,

	MAX_CEILING_TYPES

} ceiling_e;



typedef struct ceiling_s
{
    thinker_t	thinker;
    ceiling_e	type;
    sector_t*	sector;
    fixed_t	    bottomheight;
    fixed_t	    topheight;
    fixed_t	    speed;
    bool    	crush;

    // 1 = up, 0 = waiting, -1 = down
    int8	    direction;

    // ID
    int		    tag;                   
    int8	    olddirection;
    
} ceiling_t;

constexpr auto CEILSPEED = FRACUNIT;
constexpr ID_TIME_T CEILWAIT = 150;
//constexpr size_t MAXCEILINGS = 30;   // https://doomwiki.org/wiki/Static_limits

//extern ceiling_t*	activeceilings[MAXCEILINGS];

bool EV_DoCeiling ( const line_t* line, const ceiling_e type );

void    T_MoveCeiling (ceiling_t* ceiling);
void    P_AddActiveCeiling( const ceiling_t* c );
void    P_RemoveActiveCeiling( const ceiling_t* c );
bool 	EV_CeilingCrushStop( const line_t* line );
void    P_ActivateInStasisCeiling( const line_t* line );


//
// P_FLOOR
//
typedef enum : uint8
{
    // lower floor to highest surrounding floor
    lowerFloor,
    
    // lower floor to lowest surrounding floor
    lowerFloorToLowest,
    
    // lower floor to highest surrounding floor VERY FAST
    turboLower,
    
    // raise floor to lowest surrounding CEILING
    raiseFloor,
    
    // raise floor to next highest surrounding floor
    raiseFloorToNearest,

    // raise floor to shortest height texture around it
    raiseToTexture,
    
    // lower floor to lowest surrounding floor
    //  and change floorpic
    lowerAndChange,
  
    raiseFloor24,
    raiseFloor24AndChange,
    raiseFloorCrush,

     // raise to next highest floor, turbo-speed
    raiseFloorTurbo,       
    donutRaise,
    raiseFloor512,

	MAX_FLOOR_TYPES
    
} floor_e;




typedef enum : uint8
{
    build8,	// slowly build by 8
    turbo16	// quickly build by 16
    
} stair_e;



typedef struct floormove_s
{
    thinker_t	thinker;
    floor_e	    type;
    bool	    crush;
    sector_t*	sector;
    int8		direction;
    int16	    newspecial;
    index_t	    texture;
    fixed_t	    floordestheight;
    fixed_t	    speed;

} floormove_t;



constexpr auto FLOORSPEED = FRACUNIT;

typedef enum : int8
{
	error = -1,
    ok = 0,
    crushed,
    pastdest,
	
    
} result_e;

result_e T_MovePlane ( sector_t* sector, const fixed_t speed, const fixed_t	dest, const bool crush, const int floorOrCeiling, const int direction );

bool EV_BuildStairs (const line_t* line, const stair_e type );

bool EV_DoFloor ( const line_t*	line, const floor_e floortype );

void T_MoveFloor( floormove_t* floor);

//
// P_TELEPORT
//
bool EV_Teleport ( line_t* line, int side, mobj_t* thing );

#endif

