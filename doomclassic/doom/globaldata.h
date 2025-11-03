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

#ifndef _GLOBAL_DATA_H
#define _GLOBAL_DATA_H

#pragma once

#include "doomtype.h"
#include "d_net.h"
#include "m_fixed.h"
#include "info.h"
#include "sounds.h"
#include "r_defs.h" 
#include "z_zone.h"
#include "d_player.h"
#include "m_cheat.h"
#include "doomlib.h"
#include "d_main.h"
#include "hu_lib.h"
#include "hu_stuff.h"
#include "p_spec.h"
#include "p_local.h"
#include "r_bsp.h"
#include "st_stuff.h"
#include "st_lib.h"
#include "w_wad.h"
#include "dstrings.h"

#include "typedefs.h"
#include "defs.h"
#include "structs.h"

struct Globals {
	void InitGlobals();

	// vars moved here - #include<vars.h> was wreaking havoc with IntelliSense

	// the all-important zone //
	memzone_t* mainzone;

	idList<idFile, TAG_CLASSIC_DOOM> wadFileHandles;


	//  am_map.vars begin // 
	int32 	cheating;
	int32 	grid;
	bool 	leveljuststarted;
	bool    automapactive;
	int32 	finit_width;
	int32 	finit_height;
	int32 	f_x;
	int32	    f_y;
	int32 	f_w;
	int32	    f_h;
	int32 	lightlev; 		// used for funky strobing effect
	byte* fb; 			// pseudo-frame buffer
	ID_TIME_T 	amclock;
	mpoint_t    map_pan_increment; // how far the window pans each tic (map coords)
	fixed_t 	mtof_zoommul; // how far the window zooms in each tic (map coords)
	fixed_t 	ftom_zoommul; // how far the window zooms in each tic (fb coords)
	mpoint_t 	map_window_LL;   // LL x,y where the window is on the map (map coords)
	mpoint_t    old_map_window_LL;
	fixed_t 	map_window_w, map_window_h;
	fixed_t     old_map_window_w, old_map_window_h;
	fixed_t 	min_x;
	fixed_t	    min_y;
	fixed_t 	max_x;
	fixed_t     max_y;
	fixed_t 	max_w; // max_x-min_x,
	fixed_t     max_h; // max_y-min_y
	fixed_t 	min_w;
	fixed_t     min_h;
	fixed_t 	min_scale_mtof; // used to tell when to stop zooming out
	fixed_t 	max_scale_mtof; // used to tell when to stop zooming in
	mpoint_t    f_oldloc;
	fixed_t     scale_mtof;
	fixed_t     scale_ftom;
	player_t*   amap_plr; // the player represented by an arrow
	idArray<marknum_t, AM_NUMMARKPOINTS>    marknums; // numbers used for marking by the automap
	index_t     nextMarkPointIndex;
	index_t     followplayer;
	bool        stopped;
	index_t     lastlevel;
	index_t     lastepisode;
	int32         cheatstate;
	int32         bigstate;
	char        buffer[MAX_STRING_CHARS];
	ID_TIME_T   nexttic;
	size_t      litelevelscnt;
	// am_map.vars end // 
	//  doomlib.vars begin // 
	fixed_t     realoffset;
	fixed_t     viewxoffset;
	fixed_t     viewyoffset;
	// doomlib.vars end // 
	//  doomstat.vars begin // 
	GameMode_t  gamemode;
	index_t	    gamemission;
	Language_t   language;
	bool	     modifiedgame;
	// doomstat.vars end // 
	//  d_main.vars begin // 
	bool		 devparm;	// started game with -devparm
	bool         nomonsters;	// checkparm of -nomonsters
	bool         respawnparm;	// checkparm of -respawn
	bool         fastparm;	// checkparm of -fast
	bool         drone;
	bool		singletics;
	skill_t		startskill;
	index_t             startepisode;
	index_t		startmap;
	bool		autostart;
	FILE* debugfile;
	bool		advancedemo;
	char		wadfile[MAX_STRING_CHARS];		// primary wad file
	char		mapdir[MAX_STRING_CHARS];           // directory of development maps
	char		basedefault[MAX_STRING_CHARS];      // default file
	idQueue<event_t, &event_t::queueNode> events;
	gamestate_t     wipegamestate;
	bool		viewactivestate;
	bool		menuactivestate;
	bool		inhelpscreensstate;
	bool		fullscreen;
	size_t			borderdrawcount;
	bool			wipe;
	bool waitingForWipe;
	ID_TIME_T		wipestart;
	bool			wipedone;
	uint8            demosequence;
	ID_TIME_T             pagetic;
	const char* pagename;
	char            title[MAX_STRING_CHARS];
	// d_main.vars end // 
	//  d_net.vars begin // 
	doomcom_t	doomcom;
	doomdata_t* netbuffer;		// points inside doomcom
	ticcmd_t	localcmds[BACKUPTICS];
	ticcmd_t        netcmds[MAXPLAYERS][BACKUPTICS];
	idList<netNode_s, TAG_CLASSIC_DOOM> netNodes;
	ID_TIME_T             maketic;
	ID_TIME_T		lastnettic;
	ID_TIME_T		skiptics;
	int32		ticdup;
	size_t		maxsend;	// BACKUPTICS/(2*ticdup)-1
	bool		reboundpacket;
	doomdata_t	reboundstore;
	char    exitmsg[MAX_STRING_CHARS];
	ID_TIME_T      gametime;
	bool	gotinfo[MAXNETNODES];
	ID_TIME_T	frametics[4];
	int32	frameon;
	int32	frameskip[4];
	ID_TIME_T	oldnettics;
	ID_TIME_T	oldtrt_entertics;
	int32 trt_phase;
	ID_TIME_T		trt_lowtic;
	ID_TIME_T		trt_entertic;
	ID_TIME_T		trt_realtics;
	ID_TIME_T		trt_availabletics;
	size_t		trt_counts;
	size_t		trt_numplaying;
	// d_net.vars end // 
	//  f_finale.vars begin // 
	int32		finalestage;
	size_t		finalecount;
	size_t		castnum;
	ID_TIME_T		casttics;
	state_t* caststate;
	bool		castdeath;
	int32		castframes;
	int32		castonmelee;
	int32		caststartmenu;
	bool		castattacking;
	int32	laststage;
	// f_finale.vars end // 
	//  f_wipe.vars begin // 
	bool	go;
	byte* wipe_scr_start;
	byte* wipe_scr_end;
	byte* wipe_scr;
	void* g_tempPointer;
	int32* wipe_y;
	// f_wipe.vars end // 
	//  g_game.vars begin // 
	gameaction_t    gameaction;
	gamestate_t     gamestate;
	gamestate_t		oldgamestate;
	skill_t         gameskill;
	bool		respawnmonsters;
	index_t         gameepisode;
	index_t         gamemap;
	bool         paused;
	bool         sendpause;             	// send a pause event next tic 
	bool         sendsave;             	// send a save event next tic 
	bool         usergame;               // ok to save / end game 
	bool         timingdemo;             // if true, exit with report on completion 
	bool         nodrawers;              // for comparative timing purposes 
	bool         noblit;                 // for comparative timing purposes 
	ID_TIME_T        starttime;          	// for comparative timing purposes  	 
	bool         viewactive;
	uint16   deathmatch;           	// only if started as net death 
	bool         netgame;                // only true if packets are broadcast 
	idList<player_t, TAG_CLASSIC_DOOM> players;
	index_t             consoleplayer;          // player taking events and displaying 
	index_t             displayplayer;          // view being displayed 
	ID_TIME_T             gametic;
	ID_TIME_T             levelstarttic;          // gametic at level start 
	size_t             totalkills, totalitems, totalsecret;    // for intermission 
	char            demoname[MAX_STRING_CHARS];
	bool        demoplayback;
	bool        demorecording;
	bool		netdemo;
	byte* demobuffer;
	byte* demo_p;
	byte* demoend;
	bool         singledemo;            	// quit after playing a demo from cmdline 
	bool         precache;
	wbstartstruct_t wminfo;               	// parms for world map / intermission 
	int16		consistency[MAXPLAYERS][BACKUPTICS];
	byte* savebuffer;
	size_t			savebufferSize;
	int32             key_right;
	int32		key_left;
	int32		key_up;
	int32		key_down;
	int32             key_strafeleft;
	int32		key_straferight;
	int32             key_fire;
	int32		key_use;
	int32		key_strafe;
	int32		key_speed;
	int32             mousebfire;
	int32             mousebstrafe;
	int32             mousebforward;
	int32             joybfire;
	int32             joybstrafe;
	int32             joybuse;
	int32             joybspeed;
	fixed_t		forwardmove[2];
	fixed_t		sidemove[2];
	fixed_t		angleturn[3];
	bool         gamekeydown[NUMKEYS];
	int32             turnheld;				// for accelerative turning 
	bool		mousearray[MAX_MOUSEBUTTONS];
	bool* mousebuttons;
	int32             mousex;
	int32		mousey;
	int32             dclicktime;
	int32		dclickstate;
	int32		dclicks;
	int32             dclicktime2;
	int32		dclickstate2;
	int32		dclicks2;
	int32             joyxmove;
	int32		joyymove;
	bool         joyarray[MAX_JOYBUTTONS];
	bool* joybuttons;
	index_t		savegameslot;
	char		savedescription[MAX_STRING_CHARS];
	mobj_t* bodyqueue[BODYQUEUESIZE];
	index_t		bodyqueueslot;
	char turbomessage[MAX_STRING_CHARS];
	bool		secretexit;
	char	savename[MAX_STRING_CHARS];
	skill_t	d_skill;
	index_t     d_episode;
	index_t     d_map;
	index_t		d_mission;
	const char* defdemoname;
	// g_game.vars end // 
	//  hu_lib.vars begin // 
	bool	lastautomapactive;
	// hu_lib.vars end // 
	//  hu_stuff.vars begin // 
	char			chat_char; // remove later.
	player_t* plr;
	patch_t* hu_font[HU_FONTSIZE];
	hu_textline_t	w_title;
	bool			chat_on;
	hu_itext_t	w_chat;
	bool		always_off;
	char		chat_dest[MAXPLAYERS];
	hu_itext_t w_inputbuffer[MAXPLAYERS];
	bool		message_on;
	bool			message_dontfuckwithme;
	bool		message_nottobefuckedwith;
	hu_stext_t	w_message;
	size_t		message_counter;
	bool		headsupactive;
	byte	chatchars[QUEUESIZE];
	index_t	head;
	index_t	tail;
	char		lastmessage[HU_MAXLINELENGTH + 1];
	bool	shiftdown;
	bool	altdown;
	size_t		num_nobrainers;
	// hu_stuff.vars end // 
	//  i_input.vars begin // 
	InputEvent mouseEvents[2];
	InputEvent joyEvents[18];
	// i_input.vars end // 
	//  i_net_xbox.vars begin // 
	uint16			sendsocket;
	uint16			insocket;
	struct	sockaddr_in	sendaddress[MAXNETNODES];
	// i_net_xbox.vars end // 
	//  i_system.vars begin // 
	int32	mb_used;
	ticcmd_t	emptycmd;
	ID_TIME_T current_time;
	// i_system.vars end // 
	//  i_video_xbox.vars begin // 

	uint32	XColorMap[256];
	uint32* ImageBuff;
	uint32* ImageBuffs[2];

	// i_video_xbox.vars end // 
	//  m_argv.vars begin // 
	size_t		myargc;
	const char** myargv;
	// m_argv.vars end // 
	//  m_cheat.vars begin // 
	ID_TIME_T		firsttime;
	byte	cheat_xlate_table[256];
	byte cheatbuffer[256];
	int32 usedcheatbuffer;
	// m_cheat.vars end // 
	//  m_menu.vars begin // 
	int32			mouseSensitivity;       // has default
	bool		showMessages;
	int32			detailLevel;
	size_t		screenblocks;		// has default
	size_t		screenSize;
	index_t		quickSaveSlot;
	index_t		messageToPrint;
	const char* messageString;
	int32			messx;
	int32			messy;
	ID_TIME_T	messageLastMenuActive;
	bool		messageNeedsInput;
	messageRoutine_t messageRoutine;
	int32			saveStringEnter;
	index_t    	saveSlot;	// which slot to save in
	index_t		saveCharIndex;	// which char we're editing
	char		saveOldString[SAVESTRINGSIZE]{};
	bool		inhelpscreens;
	bool		menuactive;
	char		savegamestrings[MAX_SAVE_GAMES][SAVESTRINGSIZE]{};
	char		savegamepaths[MAX_SAVE_GAMES][MAX_PATH + 1]{};
	char	    endstring[MAX_STRING_CHARS]{};
	index_t		itemOn;			// menu item skull is on
	int16		skullAnimCounter;	// skull animation counter
	index_t		whichSkull;		// which skull to draw
	menu_t* currentMenu;
	menuitem_t MainMenu[5];
	menu_t  QuitDef;
	menuitem_t QuitMenu[3];
	menu_t  MainDef;
	menuitem_t EpisodeMenu[4];
	menu_t  EpiDef;
	menuitem_t ExpansionMenu[2];
	menu_t  ExpDef;
	menuitem_t NewGameMenu[5];
	menu_t  NewDef;
	menuitem_t OptionsMenu[8];
	menu_t  OptionsDef;
	menuitem_t SoundMenu[4];
	menu_t  SoundDef;
	menuitem_t LoadMenu[6];
	menu_t  LoadDef;
	menuitem_t LoadExpMenu[2];
	menu_t  LoadExpDef;
	menuitem_t SaveMenu[6];
	menu_t  SaveDef;
	char    tempstring[MAX_STRING_CHARS]{};
	index_t epi;
	index_t exp;
	int32     quitsounds[8]{};
	int32     quitsounds2[8]{};
	ID_TIME_T     joywait;
	ID_TIME_T     mousewait;
	int32     mmenu_mousey;
	int32     lasty;
	int32     mmenu_mousex;
	int32     lastx;
	//int16	md_x;	// should not be global
	//int16	md_y;	// should not be global
	// m_menu.vars end // 
	//  m_misc.vars begin // 
	const char* g_pszSaveFile;
	const char* g_pszImagePath;
	const char* g_pszImageMeta;
	bool		usemouse;
	bool		usejoystick;
	char* mousetype;
	char* mousedev;
	idList<default_t, TAG_CLASSIC_DOOM> defaults;
	const char* defaultfile;
	// m_misc.vars end // 
	//  m_random.vars begin // 
	index_t	rndindex;
	index_t	prndindex;
	// m_random.vars end // 
	//  p_ceilng.vars begin // 
	idList<ceiling_t*> activeceilings;
	// p_ceilng.vars end // 
	//  p_enemy.vars begin // 
	mobj_t* soundtarget;
	int32	TRACEANGLE;
	mobj_t* corpsehit;
	mobj_t* vileobj;
	fixed_t		viletryx;
	fixed_t		viletryy;
	idList<mobj_t*, TAG_CLASSIC_DOOM> braintargets;
	index_t		braintargeton;
	int32	easy;
	// p_enemy.vars end // 
	//  p_map.vars begin // 
	fixed_t		tmbbox[4];
	mobj_t* tmthing;
	int32		tmflags;
	fixed_t		tmx;
	fixed_t		tmy;
	bool		floatok;
	fixed_t		tmfloorz;
	fixed_t		tmceilingz;
	fixed_t		tmdropoffz;
	line_t* ceilingline;
	idList<line_t, TAG_CLASSIC_DOOM> spechit;
	fixed_t		bestslidefrac;
	fixed_t		secondslidefrac;
	line_t* bestslideline;
	line_t* secondslideline;
	mobj_t* slidemo;
	fixed_t		tmxmove;
	fixed_t		tmymove;
	mobj_t* linetarget;	// who got hit (or NULL)
	mobj_t* shootthing;
	fixed_t		shootz;
	int32		la_damage;
	fixed_t		attackrange;
	fixed_t		aimslope;
	mobj_t* usething;
	mobj_t* bombsource;
	mobj_t* bombspot;
	int32		bombdamage;
	bool		crushchange;
	bool		nofit;
	// p_map.vars end // 
	//  p_maputl.vars begin // 
	fixed_t opentop;
	fixed_t openbottom;
	fixed_t openrange;
	fixed_t	lowfloor;
	idList<intercept_t, TAG_CLASSIC_DOOM> intercepts;
	//intercept_t* intercept_p;
	divline_t 	trace;
	bool 	earlyout;
	int32		ptflags;
	// p_maputl.vars end // 
	//  p_mobj.vars begin // 
	int32 test;
	itemRespawnQueue_t itemRespawnQueue;
	ID_TIME_T lastItemRemovalTime;
	// p_mobj.vars end // 
	//  p_plats.vars begin // 
	idList<plat_t, TAG_CLASSIC_DOOM> activeplats;
	// p_plats.vars end // 
	//  p_pspr.vars begin // 
	fixed_t		swingx;
	fixed_t		swingy;
	fixed_t		bulletslope;
	// p_pspr.vars end // 
	//  p_saveg.vars begin // 
	byte* save_p;
	// p_saveg.vars end // 
	//  p_setup.vars begin // 
	idList<vertex_t, TAG_CLASSIC_DOOM> vertexes;
	idList<seg_t, TAG_CLASSIC_DOOM> segs;
	idList<sector_t, TAG_CLASSIC_DOOM> sectors;
	idList<subsector_t, TAG_CLASSIC_DOOM> subsectors;
	idList<node_t, TAG_CLASSIC_DOOM> nodes;
	idList<line_t, TAG_CLASSIC_DOOM> lines;
	idList<side_t, TAG_CLASSIC_DOOM> sides;
	size_t		blockmap_width;
	size_t		blockmap_height;	// size in mapblocks
	index_t* blockmap;	// int32 for larger maps
	index_t* blockmaplump;
	mpoint_t blockmap_origin;
	mobj_t** blocklinks;
	idList<mapthing_t, TAG_CLASSIC_DOOM> deathmatchstarts;
	//mapthing_t* deathmatch_p;
	idList<mapthing_t, TAG_CLASSIC_DOOM> playerstarts;
	// p_setup.vars end // 
	//  p_sight.vars begin // 
	fixed_t		sightzstart;		// eye z of looker
	fixed_t		topslope;
	fixed_t		bottomslope;		// slopes to top and bottom of target
	divline_t	strace;			// from t1 to t2
	fixed_t		t2x;
	fixed_t		t2y;
	size_t		sightcounts[2];
	// p_sight.vars end // 
	//  p_spec.vars begin // 
	idList<anim_t2>	anims;
	//anim_t2* lastanim;
	bool		levelTimer;
	size_t		levelTimeCount;
	size_t		levelFragCount;
	idList<line_t, TAG_CLASSIC_DOOM> linespeciallist;
	// p_spec.vars end // 
	//  p_switch.vars begin // 
	idList<index_t, TAG_CLASSIC_DOOM> switchlist;
	idList<button_t, TAG_CLASSIC_DOOM> buttonlist;
	// p_switch.vars end // 
	//  p_tick.vars begin // 
	ID_TIME_T	leveltime;
	thinker_t	thinkercap;
	// p_tick.vars end // 
	//  p_user.vars begin // 
	bool		onground;
	// p_user.vars end // 
	//  r_bsp.vars begin // 
	seg_t* curline;
	side_t* sidedef;
	line_t* linedef;
	sector_t* frontsector;
	sector_t* backsector;
	idList<drawseg_t, TAG_CLASSIC_DOOM>	drawsegs;
	//drawseg_t* ds_p;
	cliprange_t* newend;
	cliprange_t	solidsegs[MAXSEGS];
	int32	checkcoord[12][4];
	// r_bsp.vars end // 
	//  r_data.vars begin // 
	index_t		    firstflat;
	index_t		    lastflat;
	size_t	        numflats;
	index_t		    firstpatch;
	index_t		    lastpatch;
	size_t	        numpatches;
	index_t		    firstspritelump;
	index_t		    lastspritelump;
	size_t	        numspritelumps;
	int32*            flattranslation;
	int32*            texturetranslation;
	fixed_t*        spritewidth;
	fixed_t*        spriteoffset;
	fixed_t*        spritetopoffset;
	lighttable_t*   colormaps;
	size_t		    flatmemory;
	size_t		    texturememory;
	size_t		    spritememory;
	// r_data.vars end // 
	//  r_draw.vars begin // 
	byte*           viewimage;
	size_t		    viewwidth;
	size_t		    scaledviewwidth;
	size_t		    viewheight;
	int32 		    viewwindowx;
	int32 		    viewwindowy;
	byte*           ylookup[MAXHEIGHT];
	size_t		    columnofs[MAXWIDTH];
	byte		    translations[3][256];
	lighttable_t*   dc_colormap;
	int32			    dc_x;
	int32			    dc_yl;
	int32			    dc_yh;
	fixed_t		    dc_iscale;
	fixed_t		    dc_texturemid;
	byte*           dc_source;
	size_t          dccount;
	index_t	        fuzzoffset[FUZZTABLE];
	index_t	        fuzzpos;
	byte*           dc_translation;
	byte*           translationtables;
	int32			    ds_y;
	int32			    ds_x1;
	int32			    ds_x2;
	lighttable_t*   ds_colormap;
	fixed_t		    ds_xfrac;
	fixed_t		    ds_yfrac;
	fixed_t		    ds_xstep;
	fixed_t		    ds_ystep;
	byte*           ds_source;
	size_t			dscount;
	// r_draw.vars end // 
	//  r_main.vars begin // 
	int32			viewangleoffset;
	size_t			validcount;
	lighttable_t*   fixedcolormap;
	int32    		centerx;
	int32 			centery;
	fixed_t			centerxfrac;
	fixed_t			centeryfrac;
	fixed_t			projection;
	size_t			framecount;
	size_t			sscount;
	size_t			linecount;
	size_t			loopcount;
	fixed_t			viewx;
	fixed_t			viewy;
	fixed_t			viewz;
	angle_t			viewangle;
	fixed_t			viewcos;
	fixed_t			viewsin;
	player_t*       viewplayer;
	int32			    detailshift;
	angle_t			clipangle;
	int32			viewangletox[FINEANGLES / 2];
	angle_t			xtoviewangle[SCREENWIDTH + 1];
	lighttable_t*   scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
	lighttable_t*   scalelightfixed[MAXLIGHTSCALE];
	lighttable_t*   zlight[LIGHTLEVELS][MAXLIGHTZ];
	int32			    extralight;
	bool		    setsizeneeded;
	size_t		    setblocks;
	int32		        setdetail;
	// r_main.vars end // 
	//  r_plane.vars begin // 
	planefunction_t	floorfunc;
	planefunction_t	ceilingfunc;
	int16			openings[MAXOPENINGS];
	int16*          lastopening;
	int16			floorclip[SCREENWIDTH];
	int16			ceilingclip[SCREENWIDTH];
	int32			    spanstart[SCREENHEIGHT];
	int32			    spanstop[SCREENHEIGHT];
	lighttable_t**  planezlight;
	fixed_t			planeheight;
	fixed_t			yslope[SCREENHEIGHT];
	fixed_t			distscale[SCREENWIDTH];
	fixed_t			basexscale;
	fixed_t			baseyscale;
	fixed_t			cachedheight[SCREENHEIGHT];
	fixed_t			cacheddistance[SCREENHEIGHT];
	fixed_t			cachedxstep[SCREENHEIGHT];
	fixed_t			cachedystep[SCREENHEIGHT];
	// r_plane.vars end // 
	//  r_segs.vars begin // 
	bool		segtextured;
	bool		markfloor;
	bool		markceiling;
	bool		maskedtexture;
	index_t		toptexture;
	index_t		bottomtexture;
	index_t		midtexture;
	angle_t		rw_normalangle;
	angle_t		rw_angle1;
	int32		rw_x;
	int32		rw_stopx;
	angle_t		rw_centerangle;
	fixed_t		rw_offset;
	fixed_t		rw_distance;
	fixed_t		rw_scale;
	fixed_t		rw_scalestep;
	fixed_t		rw_midtexturemid;
	fixed_t		rw_toptexturemid;
	fixed_t		rw_bottomtexturemid;
	int32		worldtop;
	int32		worldbottom;
	int32		worldhigh;
	int32		worldlow;
	fixed_t		pixhigh;
	fixed_t		pixlow;
	fixed_t		pixhighstep;
	fixed_t		pixlowstep;
	fixed_t		topfrac;
	fixed_t		topstep;
	fixed_t		bottomfrac;
	fixed_t		bottomstep;
	lighttable_t** walllights;
	int16* maskedtexturecol;
	// r_segs.vars end // 
	//  r_sky.vars begin // 
	index_t			skyflatnum;
	index_t			skytexture;
	index_t			skytexturemid;
	// r_sky.vars end // 
	//  r_things.vars begin // 
	fixed_t		pspritescale;
	fixed_t		pspriteiscale;
	lighttable_t** spritelights;
	int16		negonearray[SCREENWIDTH];
	int16		screenheightarray[SCREENWIDTH];
	idList<spritedef_t, TAG_CLASSIC_DOOM> sprites;
	spriteframe_t	sprtemp[29];
	size_t		maxframe;
	idList<vissprite_t, TAG_CLASSIC_DOOM>	vissprites;
	//vissprite_t* vissprite_p;
	//index_t		newvissprite;
	index_t	overflowsprite;
	int16* mfloorclip;
	int16* mceilingclip;
	fixed_t		spryscale;
	fixed_t		sprtopscreen;
	//vissprite_t	vsprsortedhead;
	// r_things.vars end // 
	//  sounds.vars begin // 
	idList<musicinfo_t, TAG_CLASSIC_DOOM> S_music;
	// sounds.vars end // 
	//  st_lib.vars begin // 
	patch_t* sttminus;
	// st_lib.vars end // 
	//  st_stuff.vars begin // 
	player_t* plyr;
	bool		st_firsttime;
	int32		veryfirsttime;
	int32		lu_palette;
	uint32	st_clock;
	size_t		st_msgcounter;
	st_chatstateenum_t	st_chatstate;
	st_stateenum_t	st_gamestate;
	bool		st_statusbaron;
	bool		st_chat;
	bool		st_oldchat;
	bool		st_cursoron;
	bool		st_notdeathmatch;
	bool		st_armson;
	bool		st_fragson;
	patch_t* sbar; // AffLANHACK IGNORE
	patch_t* tallnum[10]; // AffLANHACK IGNORE
	patch_t* tallpercent; // AffLANHACK IGNORE
	patch_t* shortnum[10]; // AffLANHACK IGNORE
	patch_t* keys[NUMCARDS];  // AffLANHACK IGNORE
	patch_t* faces[ST_NUMFACES]; // AffLANHACK IGNORE
	patch_t* faceback; // AffLANHACK IGNORE
	patch_t* armsbg; // AffLANHACK IGNORE
	patch_t* arms[6][2];  // AffLANHACK IGNORE
	st_number_t	w_ready;
	st_number_t	w_frags;
	st_percent_t	w_health;
	st_binicon_t	w_armsbg;
	st_multicon_t	w_arms[6];
	st_multicon_t	w_faces;
	st_multicon_t	w_keyboxes[3];
	st_percent_t	w_armor;
	st_number_t	w_ammo[4];
	st_number_t	w_maxammo[4];
	size_t	st_fragscount;
	int32	st_oldhealth;
	bool	oldweaponsowned[NUMWEAPONS];
	size_t	st_facecount;
	index_t	st_faceindex;
	int32	keyboxes[3];
	int32	st_randomnumber;
	cheatseq_t	cheat_powerup[7];
	ID_TIME_T	lastcalc;
	int32	oldhealth;
	int32	lastattackdown;
	int32	priority;
	int32	largeammo;
	int32 st_palette;
	bool	st_stopped;
	// st_stuff.vars end // 
	//  s_sound.vars begin // 
	channel_t* channels;
	bool		mus_paused;
	bool		mus_looping;
	musicinfo_t* mus_playing;
	size_t			numChannels;
	int32		nextcleanup;
	// s_sound.vars end // 
	//  v_video.vars begin // 
	byte* screens[5];
	int32				dirtybox[4];
	bool	usegamma;
	// v_video.vars end // 
	//  wi_stuff.vars begin // 
	anim_t epsd0animinfo[10];
	anim_t epsd1animinfo[9];
	anim_t epsd2animinfo[6];
	anim_t* wi_stuff_anims[NUMEPISODES];
	int32 NUMANIMS[NUMEPISODES];
	const char* chat_macros[10];
	int32 acceleratestage;
	int32 me;
	stateenum_t state;
	wbstartstruct_t* wbs;
	int32 cnt;
	int32 bcnt;
	int32 firstrefresh;
	size_t cnt_kills[MAXPLAYERS];
	size_t cnt_items[MAXPLAYERS];
	size_t cnt_secret[MAXPLAYERS];
	size_t cnt_time;
	size_t cnt_par;
	size_t cnt_pause;
	size_t NUMCMAPS;
	patch_t* colon;
	bool		snl_pointeron;
	int32		dm_state;
	int32		dm_frags[MAXPLAYERS][MAXPLAYERS];
	int32		dm_totals[MAXPLAYERS];
	size_t	cnt_frags[MAXPLAYERS];
	int32	dofrags;
	int32	ng_state;
	int32	sp_state;
	// wi_stuff.vars end // 
	//  w_wad.vars begin // 
	int32			reloadlump;
	// w_wad.vars end // 
	//  z_zone.vars begin // 
	size_t sizes[NUM_ZONES + 1];
	memzone_t* zones[NUM_ZONES];
	size_t NumAlloc;
	// z_zone.vars end // 
	// info vars begin //
	state_t	states[NUMSTATES];
	// info vars end //
	// p_local begin //
	byte* rejectmatrix;
	// p_local end //
	// r_data begin //
	size_t		s_numtextures;
	texture_t** s_textures;
	int32* s_texturewidthmask;
	// needed for texture pegging 
	fixed_t* s_textureheight;
	index_t** s_texturecolumnlump;
	size_t** s_texturecolumnofs;
	byte** s_texturecomposite;
	size_t* s_texturecompositesize;
	// r_data end //
	// r_plane begin //
	idList<visplane_t> visplanes;
	//visplane_t* lastvisplane;
	index_t floorplane;
	index_t ceilingplane;
	// r_plane end //

	// wi_stuff
	// background (map of levels).
	patch_t* bg;

	// You Are Here graphic
	patch_t* yah[2];

	// splat
	patch_t* splat;

	// %, : graphics
	patch_t* percent;

	// 0-9 graphic
	patch_t* num[10];

	// minus sign
	patch_t* wiminus;

	// "Finished!" graphics
	patch_t* finished;

	// "Entering" graphic
	patch_t* entering;

	// "secret"
	patch_t* sp_secret;

	// "Kills", "Scrt", "Items", "Frags"
	patch_t* kills;
	patch_t* secret;
	patch_t* items;
	patch_t* wistuff_frags;

	// Time sucks.
	patch_t* time;
	patch_t* par;
	patch_t* sucks;

	// "killers", "victims"
	patch_t* killers;
	patch_t* victims;

	// "Total", your face, your dead face
	patch_t* total;
	patch_t* star;
	patch_t* bstar;

	// "red P[1..MAXPLAYERS]"
	patch_t* wistuff_p[MAXPLAYERS];

	// "gray P[1..MAXPLAYERS]"
	patch_t* wistuff_bp[MAXPLAYERS];

	// Name graphics of each level (centered)
	patch_t** lnames;

	const char* spritename;

};

extern Globals *g;

#define GLOBAL( type, name ) type name
#define GLOBAL_ARRAY( type, name, count ) type name[count]

extern void localCalculateAchievements(bool epComplete);


#endif
