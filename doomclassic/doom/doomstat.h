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

#ifndef __D_STATE__
#define __D_STATE__

#pragma once

// We need globally shared data structures,
//  for defining the global state variables.
#include "doomdata.h"
#include "d_net.h"

// We need the player data structure as well.
#include "d_player.h"


#ifdef __GNUG__
#pragma interface
#endif



// ------------------------
// Command line parameters.
//
extern  bool	nomonsters;	// checkparm of -nomonsters
extern  bool	respawnparm;	// checkparm of -respawn
extern  bool	fastparm;	// checkparm of -fast

extern  bool	devparm;	// DEBUG: launched with -devparm



// -----------------------------------------------------
// Game Mode - identify IWAD as shareware, retail etc.
//
extern GameMode_t	gamemode;
extern index_t	gamemission;

// Set if homebrew PWAD stuff has been added.
extern  bool	modifiedgame;


// -------------------------------------------
// Language.
extern  Language_t   language;


// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
extern  skill_t		startskill;
extern  index_t     startepisode;
extern	index_t		startmap;

extern  bool		autostart;

// Selected by user. 
extern  skill_t     gameskill;
extern  index_t		gameepisode;
extern  index_t		gamemap;

// Nightmare mode flag, single player.
extern  bool    respawnmonsters;

// Netgame? Only true if >1 player.
extern  bool	netgame;

// Flag: true only if started as net deathmatch.
// An enum might handle altdeath/cooperative better.
extern  bool	deathmatch;	
	
// -------------------------
// Internal parameters for sound rendering.
// These have been taken from the DOS version,
//  but are not (yet) supported with Linux
//  (e.g. no sound volume adjustment with menu.


// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
// Ideally, this would use indices found
//  in: /usr/include/linux/soundcard.h
extern index_t snd_MusicDevice;
extern index_t snd_SfxDevice;
// Config file? Same disclaimer as above.
extern index_t snd_DesiredMusicDevice;
extern index_t snd_DesiredSfxDevice;


// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern  bool statusbaractive;

extern  bool automapactive;	// In AutoMap mode?
extern  bool	menuactive;	// Menu overlayed?
extern  bool	paused;		// Game Pause?


extern  bool		viewactive;

extern  bool		nodrawers;
extern  bool		noblit;

extern	int		viewwindowx;
extern	int		viewwindowy;
extern	size_t		viewheight;
extern	size_t		viewwidth;
extern	size_t		scaledviewwidth;






// This one is related to the 3-screen display mode.
// ANG90 = left side, ANG270 = right
extern  int	viewangleoffset;

// Player taking events, and displaying.
extern  index_t	consoleplayer;	
extern  index_t	displayplayer;


// -------------------------------------
// Scores, rating.
// Statistics on a given map, for intermission.
//
extern  size_t	totalkills;
extern	size_t	totalitems;
extern	size_t	totalsecret;

// Timer, for scores.
extern  ID_TIME_T	levelstarttic;	// gametic at level start
extern  ID_TIME_T	leveltime;	// tics in game play for par



// --------------------------------------
// DEMO playback/recording related stuff.
// No demo, there is a human player in charge?
// Disable save/end game?
extern  bool	usergame;

//?
extern  bool	demoplayback;

// Quit after playing a demo from cmdline.
extern  bool		singledemo;	




//?
extern  gamestate_t     gamestate;






//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.



extern	ID_TIME_T		gametic;


// Bookkeeping on players - state.
extern	player_t	players[MAXPLAYERS];

// Alive? Disconnected?
extern  bool		playeringame[MAXPLAYERS];


// Player spawn spots for deathmatch.
//constexpr size_t MAX_DM_STARTS = 16; // https://doomwiki.org/wiki/Static_limits
//extern  mapthing_t      deathmatchstarts[MAX_DM_STARTS];
//extern  mapthing_t*	deathmatch_p;

// Player spawn spots.
extern  mapthing_t      playerstarts[MAXPLAYERS];

// Intermission stats.
// Parameters for world map / intermission.
extern  wbstartstruct_t		wminfo;	


// LUT of ammunition limits for each kind.
// This doubles with BackPack powerup item.
const extern  size_t  maxammo[NUMAMMO];





//-----------------------------------------
// Internal parameters, used for engine.
//

// File handling stuff.
extern	char		basedefault[1024];
extern  FILE*		debugfile;

// if true, load all graphics at level load
extern  bool         precache;


// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern  gamestate_t     wipegamestate;

extern  int             mouseSensitivity;
//?
// debug flag to cancel adaptiveness
extern  bool         singletics;	

extern  int             bodyqueslot;



// Needed to store the number of the dummy sky flat.
// Used for rendering,
//  as well as tracking projectiles etc.
extern int		skyflatnum;



// Netgame stuff (buffers and pointers, i.e. indices).

// This is ???
extern  doomcom_t	doomcom;

// This points inside doomcom.
extern  doomdata_t*	netbuffer;	


extern  ticcmd_t	localcmds[BACKUPTICS];
extern	int		rndindex;

extern	int		maketic;
extern  ID_TIME_T             nettics[MAXNETNODES];

extern  ticcmd_t        netcmds[MAXPLAYERS][BACKUPTICS];
extern	int		ticdup;



#endif

