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

#ifndef __DEFS_H__
#define __DEFS_H__

#pragma once


//  am_map.defs begin // 
constexpr auto  REDS = (256 - 5 * 16);
constexpr auto  REDRANGE = 16;
constexpr auto  BLUES = (256 - 4 * 16 + 8);
constexpr auto  BLUERANGE = 8;
constexpr auto  GREENS = (7 * 16);
constexpr auto  GREENRANGE = 16;
constexpr auto  GRAYS = (6 * 16);
constexpr auto  GRAYSRANGE = 16;
constexpr auto  BROWNS = (4 * 16);
constexpr auto  BROWNRANGE = 16;
constexpr auto  YELLOWS = (256 - 32 + 7);
constexpr auto  YELLOWRANGE = 1;
constexpr auto  BLACK = 0;
constexpr auto  WHITE = (256 - 47);
constexpr auto  BACKGROUND = BLACK;
constexpr auto  YOURCOLORS = WHITE;
constexpr auto  YOURRANGE = 0;
constexpr auto  WALLCOLORS = REDS;
constexpr auto  WALLRANGE = REDRANGE;
constexpr auto  TSWALLCOLORS = GRAYS;
constexpr auto  TSWALLRANGE = GRAYSRANGE;
constexpr auto  FDWALLCOLORS = BROWNS;
constexpr auto  FDWALLRANGE = BROWNRANGE;
constexpr auto  CDWALLCOLORS = YELLOWS;
constexpr auto  CDWALLRANGE = YELLOWRANGE;
constexpr auto  THINGCOLORS = GREENS;
constexpr auto  THINGRANGE = GREENRANGE;
#if defined (_DEBUG) || defined(DEBUG)
constexpr auto  SECRETWALLCOLORS = GREENS;
#else
constexpr auto  SECRETWALLCOLORS = WALLCOLORS;
#endif
constexpr auto  SECRETWALLRANGE = WALLRANGE;
constexpr auto  GRIDCOLORS = (GRAYS + GRAYSRANGE / 2);
constexpr auto  GRIDRANGE = 0;
constexpr auto  XHAIRCOLORS = GRAYS;
constexpr index_t	FB = 0;

enum autoMapKeys_e : uint8
{
	AM_PANDOWNKEY    = KEY_DOWNARROW,
	AM_PANUPKEY      = KEY_UPARROW,
	AM_PANRIGHTKEY   = KEY_RIGHTARROW,
	AM_PANLEFTKEY    = KEY_LEFTARROW,
	AM_ZOOMINKEY     = K_EQUALS,
	AM_ZOOMOUTKEY    = K_MINUS,
	AM_STARTKEY      = KEY_TAB,
	AM_ENDKEY        = KEY_TAB,
	AM_GOBIGKEY      = K_0,
	AM_FOLLOWKEY     = K_F,
	AM_GRIDKEY       = K_G,
	AM_MARKKEY       = K_M,
	AM_CLEARMARKKEY  = K_C
};

constexpr size_t AM_NUMMARKPOINTS = 10;

constexpr auto INITSCALEMTOF = (0.2 * FRACUNIT);
constexpr size_t F_PANINC = 4;
constexpr auto  M_ZOOMIN = (1.02 * FRACUNIT);
constexpr auto  M_ZOOMOUT = (FRACUNIT / 1.02);
#define FTOM(_v) (((_v)<<16) *::g->scale_ftom)
#define MTOF(_v) (((_v) * ::g->scale_mtof)>>16)
#define CXMTOF(_x)  (::g->f_x + MTOF(((_x) - ::g->map_window_LL.x)))
#define CYMTOF(_y)  (::g->f_y + (::g->f_h - MTOF((_y) - ::g->map_window_LL.y)))
constexpr auto LINE_NEVERSEE = ML_DONTDRAW;
#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))
#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(mline_t))
#define NUMTRIANGLEGUYLINES (sizeof(triangle_guy)/sizeof(mline_t))
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(mline_t))
#define DOOUTCODE(oc, mx, my) \
	do { \
		(oc) = 0; \
		if ((my) < 0) (oc) |= TOP; \
		else if ((my) >= ::g->f_h) (oc) |= BOTTOM; \
		if ((mx) < 0) (oc) |= LEFT; \
		else if ((mx) >= ::g->f_w) (oc) |= RIGHT; \
	} while (0)
#define PUTDOT(xx,yy,cc) \
    do { \
        uint32 _c_ = static_cast<uint32>(cc); \
        memcpy(&::g->fb[(yy) * ::g->f_w + (xx)], &_c_, sizeof(_c_)); \
    } while(0)
// am_map.defs end // 
//  d_main.defs begin // 
//#define	BGCOLOR		7
//#define	FGCOLOR		8
constexpr auto DOOMWADDIR = "wads/";
// d_main.defs end // 
//  d_net.defs begin // 
enum NCMD_e : uint32
{
	NCMD_EXIT       = 0x80000000,
	NCMD_RETRANSMIT = 0x40000000,
	NCMD_SETUP      = 0x20000000,
	NCMD_KILL       = 0x10000000, // kill game
	NCMD_CHECKSUM   = 0x0fffffff
};

constexpr size_t RESENDCOUNT = 10;
#define	PL_DRONE	0x80	// bit flag in doomdata->player
// d_net.defs end // 
//  f_finale.defs begin // 
constexpr ID_SECONDS_T	TEXTSPEED = 3;
constexpr ID_TIME_T TEXTWAIT = 250;
// f_finale.defs end // 
//  g_game.defs begin //
constexpr size_t MAX_SAVE_GAMES = 10;
constexpr size_t SAVESTRINGSIZE = 64;
#define MAXPLMOVE		(::g->forwardmove[1]) 
constexpr auto TURBOTHRESHOLD = 0x32;
constexpr ID_TIME_T SLOWTURNTICS = 6;
constexpr size_t NUMKEYS = 256;
constexpr size_t MAX_MOUSEBUTTONS = 4;
constexpr size_t MAX_JOYBUTTONS = 5;
constexpr size_t BODYQUEUESIZE = 32;
constexpr size_t VERSIONSIZE = 16;
constexpr auto DEMOMARKER = 0x80;
// g_game.defs end // 
//  hu_lib.defs begin // 
#define noterased ::g->viewwindowx
// hu_lib.defs end // 
//  hu_stuff.defs begin // 
#define HU_TITLE	(mapnames[(::g->gameepisode-1)*9+::g->gamemap-1])
#define HU_TITLE2	(mapnames2[::g->gamemap-1])
#define HU_TITLEP	(mapnamesp[::g->gamemap-1])
#define HU_TITLET	(mapnamest[::g->gamemap-1])
constexpr size_t  HU_TITLEHEIGHT = 1;
constexpr int HU_TITLEX = 0;
#define HU_TITLEY	(167 - SHORT(::g->hu_font[0]->height))
constexpr auto HU_INPUTTOGGLE = K_T;
#define HU_INPUTX	HU_MSGX
#define HU_INPUTY	(HU_MSGY + HU_MSGHEIGHT*(SHORT(::g->hu_font[0]->height) +1))
constexpr size_t HU_INPUTWIDTH = 64;
constexpr size_t HU_INPUTHEIGHT = 1;
constexpr size_t QUEUESIZE = 128;
// hu_stuff.defs end // 
//  i_net.defs begin // 
// SMF
/*
#define ntohl(x) \
        ((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
                             (((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
                             (((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
                             (((unsigned long int)(x) & 0xff000000U) >> 24)))
#define ntohs(x) \
        ((unsigned short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
                              (((unsigned short int)(x) & 0xff00) >> 8))) \
#define htonl(x) ntohl(x)
#define htons(x) ntohs(x)
// i_net.defs end // 
//  i_net_xbox.defs begin // 
#define ntohl(x) \
	((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
	(((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
	(((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
	(((unsigned long int)(x) & 0xff000000U) >> 24)))
#define ntohs(x) \
	((unsigned short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
	(((unsigned short int)(x) & 0xff00) >> 8))) \

#define htonl(x) ntohl(x)
#define htons(x) ntohs(x)
*/	  

constexpr auto IPPORT_USERRESERVED = 5000;
// i_net_xbox.defs end // 
//  i_sound_xbox.defs begin // 
constexpr size_t SAMPLECOUNT = 512;
constexpr size_t NUM_SOUNDBUFFERS = 64;
constexpr size_t BUFMUL = 4;
constexpr size_t MIXBUFFERSIZE = (SAMPLECOUNT * BUFMUL);
// i_sound_xbox.defs end // 
//  i_video_xbox.defs begin // 
//#define TEXTUREWIDTH	512
//#define TEXTUREHEIGHT	256
// i_video_xbox.defs end // 
//  mus2midi.defs begin //
enum musEvent_e : uint8
{
	MUSEVENT_KEYOFF = 0,
	MUSEVENT_KEYON = 1,
	MUSEVENT_PITCHWHEEL = 2,
	MUSEVENT_CHANNELMODE = 3,
	MUSEVENT_CONTROLLERCHANGE = 4,
	MUSEVENT_END = 6
};
constexpr size_t MIDI_MAXCHANNELS = 16;
constexpr size_t MIDIHEADERSIZE = 14;
// mus2midi.defs end // 
//  m_menu.defs begin // 
//constexpr size_t SAVESTRINGSIZE = 64;
constexpr int SKULLXOFF = -32;
constexpr size_t LINEHEIGHT = 16;
// m_menu.defs end // 
//  p_enemy.defs begin // 
constexpr size_t MAXSPECIALCROSS = 8;
constexpr auto	FATSPREAD = ANG10 + ANG1 + ANG0_25; // (ANG90/8) = 11.25
constexpr auto	SKULLSPEED = (20 * FRACUNIT);
// p_enemy.defs end // 
//  p_inter.defs begin // 
constexpr auto	 BONUSADD = 6;
// p_inter.defs end // 
//  p_map.defs begin // 
//constexpr size_t MAXSPECIALCROSS = 8;
// p_map.defs end // 
//  p_mobj.defs begin // 
constexpr auto	 STOPSPEED = 0x1000;
constexpr auto	 FRICTION = 0xe800;
// p_mobj.defs end // 
//  p_pspr.defs begin // 
constexpr auto	 LOWERSPEED = FRACUNIT * 6;
constexpr auto	 RAISESPEED = FRACUNIT * 6;
constexpr auto	 WEAPONBOTTOM = 128 * FRACUNIT;
constexpr auto	 WEAPONTOP = 32 * FRACUNIT;
constexpr auto	 BFGCELLS = 40;
// p_pspr.defs end // 
//  p_saveg.defs begin // 
#define PADSAVEP()	(::g->save_p += (4 - ((int) ::g->save_p & 3)) & 3)
// p_saveg.defs end // 
//  p_setup.defs begin // 
constexpr size_t MAX_DEATHMATCH_STARTS = 10;
// p_setup.defs end // 
//  p_spec.defs begin // 
constexpr size_t MAXANIMS = 40;     // https://doomwiki.org/wiki/Static_limits
constexpr size_t MAXLINEANIMS = 96;    // https://doomwiki.org/wiki/Static_limits
constexpr size_t MAX_ADJOINING_SECTORS = 20;
// p_spec.defs end // 
//  p_user.defs begin // 
constexpr auto	 INVERSECOLORMAP = 32;

// DHM - NERVE :: MAXBOB reduced 25%
//#define MAXBOB	0x100000
constexpr auto MAXBOB = 0xC0000;

// p_user.defs end // 
//  r_bsp.defs begin // 
constexpr size_t MAXSEGS = 32;
// r_bsp.defs end // 
//  r_draw.defs begin // 
//#define MAXWIDTH			1120
//#define MAXHEIGHT			832
constexpr size_t SBARHEIGHT = 32 * GLOBAL_IMAGE_SCALER;
constexpr size_t FUZZTABLE = 50;
constexpr auto FUZZOFF = numeric_cast<index_t>(SCREENWIDTH);
// r_draw.defs end // 
//  r_main.defs begin // 
constexpr int32 FIELDOFVIEW = FINEANGLES / 4; // =2048, 90 degrees
constexpr auto DISTMAP = 2;
// r_main.defs end // 
//  r_plane.defs begin // 
//#define MAXVISPLANES	128
constexpr size_t MAXVISPLANES = 384;
constexpr size_t MAXOPENINGS = SCREENWIDTH * SCREENHEIGHT; //SCREENWIDTH * 64;    // https://doomwiki.org/wiki/Static_limits
// r_plane.defs end // 
//  r_segs.defs begin // 
constexpr size_t HEIGHTBITS = 12;
constexpr size_t HEIGHTUNIT = (1 << HEIGHTBITS);
// r_segs.defs end // 
//  r_things.defs begin // 
constexpr auto MINZ = (FRACUNIT * 4);
constexpr int BASEYCENTER = 100;
// r_things.defs end // 
//  st_stuff.defs begin // 
constexpr index_t STARTREDPALS = 1;
constexpr index_t STARTBONUSPALS = 9;
constexpr size_t NUMREDPALS = 8;
constexpr size_t NUMBONUSPALS = 4;
constexpr index_t RADIATIONPAL = 13;

//#define ST_FACEPROBABILITY		96
constexpr auto ST_TOGGLECHAT = KEY_ENTER;
constexpr int ST_X = 0;
constexpr int  ST_X2 = 104;
constexpr int  ST_FX = 143;
constexpr int  ST_FY = 169;
#define        ST_TALLNUMWIDTH		(::g->tallnum[0]->width)
constexpr size_t  ST_NUMPAINFACES = 5;
constexpr size_t  ST_NUMSTRAIGHTFACES = 3;
constexpr size_t  ST_NUMTURNFACES = 2;
constexpr size_t  ST_NUMSPECIALFACES = 3;
constexpr size_t  ST_FACESTRIDE = (ST_NUMSTRAIGHTFACES + ST_NUMTURNFACES + ST_NUMSPECIALFACES);
constexpr size_t  ST_NUMEXTRAFACES = 2;
constexpr size_t  ST_NUMFACES = (ST_FACESTRIDE* ST_NUMPAINFACES + ST_NUMEXTRAFACES);
constexpr size_t  ST_TURNOFFSET = (ST_NUMSTRAIGHTFACES);
constexpr size_t  ST_OUCHOFFSET = (ST_TURNOFFSET + ST_NUMTURNFACES);
constexpr size_t  ST_EVILGRINOFFSET = (ST_OUCHOFFSET + 1);
constexpr size_t  ST_RAMPAGEOFFSET = (ST_EVILGRINOFFSET + 1);
constexpr index_t ST_GODFACE = (ST_NUMPAINFACES * ST_FACESTRIDE);
constexpr index_t ST_DEADFACE = (ST_GODFACE + 1);
constexpr int  ST_FACESX = 143;
constexpr int  ST_FACESY = 168;
constexpr ID_TIME_T  ST_EVILGRINCOUNT = (2 * TICRATE);
constexpr ID_TIME_T   ST_STRAIGHTFACECOUNT = (TICRATE / 2);
constexpr ID_TIME_T   ST_TURNCOUNT = (1 * TICRATE);
constexpr ID_TIME_T   ST_OUCHCOUNT = (1 * TICRATE);
constexpr ID_TIME_T   ST_RAMPAGEDELAY = (2 * TICRATE);
constexpr size_t  ST_MUCHPAIN = 20;
constexpr size_t  ST_AMMOWIDTH = 3;
constexpr int32  ST_AMMOX = 44;
constexpr int32  ST_AMMOY = 171;
constexpr size_t  ST_HEALTHWIDTH = 3;
constexpr int32 ST_HEALTHX = 90;
constexpr int32  ST_HEALTHY = 171;
constexpr int32  ST_ARMSX = 111;
constexpr int32  ST_ARMSY = 172;
constexpr int32  ST_ARMSBGX = 104;
constexpr int32  ST_ARMSBGY = 168;
constexpr size_t  ST_ARMSXSPACE = 12;
constexpr size_t  ST_ARMSYSPACE = 10;
constexpr int32  ST_FRAGSX = 138;
constexpr int32  ST_FRAGSY = 171;
constexpr size_t  ST_FRAGSWIDTH = 2;
constexpr size_t  ST_ARMORWIDTH = 3;
constexpr int32  ST_ARMORX = 221;
constexpr int32  ST_ARMORY = 171;
constexpr size_t  ST_KEY0WIDTH = 8;
constexpr size_t  ST_KEY0HEIGHT = 5;
constexpr int32  ST_KEY0X = 239;
constexpr int32  ST_KEY0Y = 171;
constexpr size_t  ST_KEY1WIDTH = ST_KEY0WIDTH;
constexpr int32  ST_KEY1X = 239;
constexpr int32  ST_KEY1Y = 181;
constexpr size_t  ST_KEY2WIDTH = ST_KEY0WIDTH;
constexpr int32  ST_KEY2X = 239;
constexpr int32  ST_KEY2Y = 191;
constexpr size_t  ST_AMMO0WIDTH = 3;
constexpr size_t  ST_AMMO0HEIGHT = 6;
constexpr int32  ST_AMMO0X = 288;
constexpr int32  ST_AMMO0Y = 173;
constexpr size_t  ST_AMMO1WIDTH = ST_AMMO0WIDTH;
constexpr int32  ST_AMMO1X = 288;
constexpr int32  ST_AMMO1Y = 179;
constexpr size_t  ST_AMMO2WIDTH = ST_AMMO0WIDTH;
constexpr int32  ST_AMMO2X = 288;
constexpr int32  ST_AMMO2Y = 191;
constexpr size_t  ST_AMMO3WIDTH = ST_AMMO0WIDTH;
constexpr int32  ST_AMMO3X = 288;
constexpr int32  ST_AMMO3Y = 185;
constexpr size_t  ST_MAXAMMO0WIDTH = 3;
constexpr size_t  ST_MAXAMMO0HEIGHT = 5;
constexpr int32  ST_MAXAMMO0X = 314;
constexpr int32  ST_MAXAMMO0Y = 173;
constexpr size_t  ST_MAXAMMO1WIDTH = ST_MAXAMMO0WIDTH;
constexpr int32  ST_MAXAMMO1X = 314;
constexpr int32  ST_MAXAMMO1Y = 179;
constexpr size_t  ST_MAXAMMO2WIDTH = ST_MAXAMMO0WIDTH;
constexpr int32  ST_MAXAMMO2X = 314;
constexpr int32  ST_MAXAMMO2Y = 191;
constexpr size_t  ST_MAXAMMO3WIDTH = ST_MAXAMMO0WIDTH;
constexpr int32  ST_MAXAMMO3X = 314;
constexpr int32  ST_MAXAMMO3Y = 185;
constexpr int32  ST_WEAPON0X = 110;
constexpr int32  ST_WEAPON0Y = 172;
constexpr int32  ST_WEAPON1X = 122;
constexpr int32  ST_WEAPON1Y = 172;
constexpr int32  ST_WEAPON2X = 134;
constexpr int32  ST_WEAPON2Y = 172;
constexpr int32  ST_WEAPON3X = 110;
constexpr int32  ST_WEAPON3Y = 181;
constexpr int32  ST_WEAPON4X = 122;
constexpr int32  ST_WEAPON4Y = 181;
constexpr int32  ST_WEAPON5X = 134;
constexpr int32  ST_WEAPON5Y = 181;
constexpr int32  ST_WPNSX = 109;
constexpr int32  ST_WPNSY = 191;
constexpr int32  ST_DETHX = 109;
constexpr int32  ST_DETHY = 191;
constexpr int32  ST_MSGTEXTX = 0;
constexpr int32  ST_MSGTEXTY = 0;
constexpr size_t  ST_MSGWIDTH = 52;
constexpr size_t  ST_MSGHEIGHT = 1;
constexpr int32  ST_OUTTEXTX = 0;
constexpr int32  ST_OUTTEXTY = 6;
constexpr size_t  ST_OUTWIDTH = 52;
constexpr size_t  ST_OUTHEIGHT = 1;
//#define ST_MAPWIDTH	(strlen(mapnames[(::g->gameepisode-1)*9+(::g->gamemap-1)]))
//#define ST_MAPTITLEX  (SCREENWIDTH - ST_MAPWIDTH * ST_CHATFONTWIDTH);
constexpr int32  ST_MAPTITLEY = 0;
constexpr size_t  ST_MAPHEIGHT = 1;

// st_stuff.defs end // 
//  s_sound.defs begin // 
constexpr size_t  S_MAX_VOLUME = 127;
constexpr size_t  S_CLIPPING_DIST = (1200ULL * 0x10000);
constexpr size_t  S_CLOSE_DIST = (160ULL * 0x10000);
constexpr auto  S_ATTENUATOR((S_CLIPPING_DIST - S_CLOSE_DIST));
#define NORM_VOLUME    		snd_MaxVolume
constexpr auto  NORM_PITCH = 128;
constexpr auto  NORM_PRIORITY = 64;
constexpr auto  NORM_SEP = 128;
constexpr auto  S_PITCH_PERTURB = 1;
constexpr auto  S_STEREO_SWING = (96 * 0x10000);
constexpr auto  S_IFRACVOL = 30;
constexpr auto  NA = 0;
constexpr size_t  S_NUMCHANNELS = 256;
// s_sound.defs end // 
//  wi_stuff.defs begin // 
constexpr size_t NUMEPISODES = 4;
constexpr size_t NUMMAPS = 9;
constexpr int32  WI_TITLEY = 2;
constexpr size_t  WI_SPACINGY = 33;
constexpr int32  SP_STATSX = 50;
constexpr int32  SP_STATSY = 50;
constexpr int32  SP_TIMEX = 16;
constexpr int32  SP_TIMEY(ORIGINAL_HEIGHT - 32);
constexpr int32  NG_STATSY = 50;
#define          NG_STATSX (32 + SHORT(::g->star->width) / 2 + 32 * !::g->dofrags)
constexpr size_t  NG_SPACINGX = 64;
constexpr int32  DM_MATRIXX = 42;
constexpr int32  DM_MATRIXY = 68;
constexpr size_t  DM_SPACINGX = 40;
constexpr int32  DM_TOTALSX = 269;
constexpr int32  DM_KILLERSX = 10;
constexpr int32  DM_KILLERSY = 100;
constexpr int32  DM_VICTIMSX = 5;
constexpr int32  DM_VICTIMSY = 50;
//#define SP_KILLS		0
//#define SP_ITEMS		2
//#define SP_SECRET		4
//#define SP_FRAGS		6 
//#define SP_TIME			8 
//#define SP_PAR			ST_TIME
//#define SP_PAUSE		1
constexpr ID_SECONDS_T SHOWNEXTLOCDELAY = 4;
// wi_stuff.defs end // 
//  w_wad.defs begin // 

// w_wad.defs end // 
//  z_zone.defs begin // 
constexpr auto   ZONEID = 0x1d4a11;
constexpr size_t NUM_ZONES = 11;
constexpr size_t MINFRAGMENT = 64;
//#define NO_SHARE_LUMPS
// z_zone.defs end // 
#endif // __DEFS_H__
