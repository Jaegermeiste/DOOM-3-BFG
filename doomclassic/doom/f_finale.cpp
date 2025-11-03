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

#include <cctype>

#include <algorithm>

// Functions.
#include "i_system.h"
#include "m_swap.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "doomstat.h"
#include "r_state.h"

#include "Main.h"
#include "d3xp/Game_local.h"

// ?
//#include "doomstat.h"
//#include "r_local.h"
//#include "f_finale.h"

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast


static const char* e1text = E1TEXT;
static const char* e2text = E2TEXT;
static const char* e3text = E3TEXT;
static const char* e4text = E4TEXT;

static const char* c1text = C1TEXT;
static const char* c2text = C2TEXT;
static const char* c3text = C3TEXT;
static const char* c4text = C4TEXT;
static const char* c5text = C5TEXT;
static const char* c6text = C6TEXT;
static const char* c7text = C7TEXT;
static const char* c8Text = C8TEXT;

static const char* p1text = P1TEXT;
static const char* p2text = P2TEXT;
static const char* p3text = P3TEXT;
static const char* p4text = P4TEXT;
static const char* p5text = P5TEXT;
static const char* p6text = P6TEXT;

static const char* t1text = T1TEXT;
static const char* t2text = T2TEXT;
static const char* t3text = T3TEXT;
static const char* t4text = T4TEXT;
static const char* t5text = T5TEXT;
static const char* t6text = T6TEXT;

static const char* finaletext;
static const char* finaleflat;

static void	F_StartCast();
static void	F_CastTicker();
static bool F_CastResponder(event_t* ev);
static void	F_CastDrawer();

//
// F_StartFinale
//
static void F_StartFinale()
{
	::g->gameaction = ga_nothing;
	::g->gamestate = GS_FINALE;
	::g->viewactive = false;
	::g->automapactive = false;

	// Check for end of episode/mission
	bool endOfMission = false;

	if (((::g->gamemission == doom || ::g->gamemission == doom2 || ::g->gamemission == pack_tnt || ::g->gamemission == pack_plut) && ::g->gamemap == 30)
		|| (::g->gamemission == pack_nerve && ::g->gamemap == 8)
		|| (::g->gamemission == pack_master && ::g->gamemap == 21))
	{
		endOfMission = true;
	}

	localCalculateAchievements(endOfMission);

	// Okay - IWAD dependent stuff.
	// This has been changed severely, and
	// some stuff might have changed in the process.
	switch (::g->gamemode)
	{

		// DOOM 1 - E1, E3 or E4, but each nine missions
	case shareware:
	case registered:
	case retail:
	{
		S_ChangeMusic(mus_victor, true);

		switch (::g->gameepisode)
		{
		case 1:
			finaleflat = "FLOOR4_8";
			finaletext = e1text;
			break;
		case 2:
			finaleflat = "SFLR6_1";
			finaletext = e2text;
			break;
		case 3:
			finaleflat = "MFLR8_4";
			finaletext = e3text;
			break;
		case 4:
			finaleflat = "MFLR8_3";
			finaletext = e4text;
			break;
		default:
			// Ouch.
			break;
		}
		break;
	}

	// DOOM II and missions packs with E1, M34
	case commercial:
	{
		S_ChangeMusic(mus_read_m, true);

		if (::g->gamemission == doom2 || ::g->gamemission == pack_tnt || ::g->gamemission == pack_plut) {
			switch (::g->gamemap)
			{
			case 6:
				finaleflat = "SLIME16";
				finaletext = c1text;
				break;
			case 11:
				finaleflat = "RROCK14";
				finaletext = c2text;
				break;
			case 20:
				finaleflat = "RROCK07";
				finaletext = c3text;
				break;
			case 30:
				finaleflat = "RROCK17";
				finaletext = c4text;
				break;
			case 15:
				finaleflat = "RROCK13";
				finaletext = c5text;
				break;
			case 31:
				finaleflat = "RROCK19";
				finaletext = c6text;
				break;
			default:
				// Ouch.
				break;
			}
		}
		else if (::g->gamemission == pack_master) {
			switch (::g->gamemap)
			{
			case 21:
				finaleflat = "SLIME16";
				finaletext = c8Text;
				break;
			}
		}
		else if (::g->gamemission == pack_nerve) {
			switch (::g->gamemap) {
			case 8:
				finaleflat = "SLIME16";
				finaletext = c7text;
				break;
			default:
				break;
			}
		}

		break;
	}

	// Indeterminate.
	case indetermined:
	default:
		S_ChangeMusic(mus_read_m, true);
		finaleflat = "F_SKY1"; // Not used anywhere else.
		finaletext = c1text;  // FIXME - other text, music?
		break;
	}

	::g->finalestage = 0;
	::g->finalecount = 0;
}


static bool finaleButtonPressed = false;
static bool startButtonPressed = false;

static bool F_Responder(event_t* event)
{
	if (!common->IsMultiplayer() && event->type == ev_keydown && event->data1 == KEY_ESCAPE) {
		startButtonPressed = true;
		return true;
	}

	if (::g->finalestage == 2)
	{
		return F_CastResponder(event);
	}

	return false;
}


//
// F_Ticker
//
static void F_Ticker()
{
	// check for skipping
	if ((::g->gamemode == commercial) && (::g->finalecount > 50))
	{
		size_t i = 0;
		// go on to the next level
		for (i = 0; i < MAXPLAYERS; ++i)
		{
			if (::g->players[i].cmd.buttons)
			{
				break;
			}
		}

		if (finaleButtonPressed || i < MAXPLAYERS)
		{
			bool castStarted = false;
			if (::g->gamemission == doom2 || ::g->gamemission == pack_plut || ::g->gamemission == pack_tnt) {
				if (::g->gamemap == 30) {
					F_StartCast();
					castStarted = true;
				}

			}
			else if (::g->gamemission == pack_master) {
				if (::g->gamemap == 21) {
					F_StartCast();
					castStarted = true;
				}

			}
			else if (::g->gamemission == pack_nerve) {
				if (::g->gamemap == 8) {
					F_StartCast();
					castStarted = true;
				}

			}

			if (castStarted == false) {
				::g->gameaction = ga_worlddone;
			}
		}
	}

	const bool SkipTheText = finaleButtonPressed;

	// advance animation
	::g->finalecount++;
	finaleButtonPressed = false;

	if (::g->finalestage == 2)
	{
		F_CastTicker();
		return;
	}

	if (::g->gamemode == commercial) {
		startButtonPressed = false;
		return;
	}

	if (SkipTheText && (::g->finalecount > 50)) {
		::g->finalecount = strlen(finaletext) * TEXTSPEED + TEXTWAIT;
	}

	if (!::g->finalestage && ::g->finalecount > strlen(finaletext) * TEXTSPEED + TEXTWAIT)
	{
		::g->finalecount = 0;
		::g->finalestage = 1;
		::g->wipegamestate = GS_INVALID;		// force a wipe
		if (::g->gameepisode == 3)
		{
			S_StartMusic(mus_bunny);
		}
	}

	startButtonPressed = false;

}



//
// F_TextWrite
//

#include "hu_stuff.h"


static void F_TextWrite()
{
	if (::g->finalecount == 60) {
		DoomLib::ShowXToContinue(true);
	}

	// erase the entire screen to a tiled background
	byte* src = static_cast<byte*>(W_CacheLumpName(finaleflat, PU_CACHE_SHARED));
	byte* dest = ::g->screens[0];

	for (size_t y = 0; y < SCREENHEIGHT; y++)
	{
		for (size_t x = 0; x < SCREENWIDTH / 64; x++)
		{
			memcpy(dest, src + ((y & 63) << 6), 64);
			dest += 64;
		}
		if (false)
		{
			memcpy(dest, src + ((y & 63) << 6), SCREENWIDTH & 63);
			dest += (SCREENWIDTH & 63);
		}
	}

	V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);

	// draw some of the text onto the screen
	int cx = 10;
	int cy = 10;
	const char* ch = finaletext;

	size_t count = (::g->finalecount - 10) / TEXTSPEED;
	count = Max(count, 0);
	for (; count; count--)
	{

		auto c = *ch;
		++c;

		if (!c)
		{
			break;
		}
		if (c == '\n')
		{
			cx = 10;
			cy += 11;
			continue;
		}

		c = idStr::ToUpper(c) - HU_FONTSTART;
		if (c < 0 || c> HU_FONTSIZE)
		{
			cx += 4;
			continue;
		}

		size_t w = SHORT(::g->hu_font[c]->width);
		if (cx + w > SCREENWIDTH)
		{
			break;
		}
		V_DrawPatch(cx, cy, 0, ::g->hu_font[c]);
		cx += w;
	}

}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//

static castinfo_t	castorder[] =
{
	{CC_ZOMBIE, MT_POSSESSED},
	{CC_SHOTGUN, MT_SHOTGUY},
	{CC_HEAVY, MT_CHAINGUY},
	{CC_IMP, MT_TROOP},
	{CC_DEMON, MT_SERGEANT},
	{CC_LOST, MT_SKULL},
	{CC_CACO, MT_HEAD},
	{CC_HELL, MT_KNIGHT},
	{CC_BARON, MT_BRUISER},
	{CC_ARACH, MT_BABY},
	{CC_PAIN, MT_PAIN},
	{CC_REVEN, MT_UNDEAD},
	{CC_MANCU, MT_FATSO},
	{CC_ARCH, MT_VILE},
	{CC_SPIDER, MT_SPIDER},
	{CC_CYBER, MT_CYBORG},
	{CC_HERO, MT_PLAYER},

	{nullptr,static_cast<mobjtype_t>(0)}
};



//
// F_StartCast
//


void F_StartCast()
{
	if (::g->finalestage != 2) {
		::g->wipegamestate = static_cast<gamestate_t>(-1);		// force a screen wipe
		::g->castnum = 0;
		::g->caststate = &::g->states[mobjinfo[castorder[::g->castnum].type].seestate];
		::g->casttics = ::g->caststate->tics;
		::g->castdeath = false;
		::g->finalestage = 2;
		::g->castframes = 0;
		::g->castonmelee = 0;
		::g->castattacking = false;
		S_ChangeMusic(mus_evil, true);

		::g->caststartmenu = ::g->finalecount + 50;
	}
}


//
// F_CastTicker
//
void F_CastTicker()
{
	statenum_e	st = S_NULL;
	sfxenum_e	sfx = sfx_None;

	if (::g->finalecount == ::g->caststartmenu) {
		DoomLib::ShowXToContinue(true);
	}

	if (--::g->casttics > 0)
	{
		return; // not time to change state yet
	}

	if (::g->caststate->tics == -1 || ::g->caststate->nextstate == S_NULL)
	{
		// switch from deathstate to next monster
		::g->castnum++;
		::g->castdeath = false;
		if (castorder[::g->castnum].name == nullptr)
		{
			::g->castnum = 0;
		}
		if (mobjinfo[castorder[::g->castnum].type].seesound)
		{
			S_StartSound(nullptr, mobjinfo[castorder[::g->castnum].type].seesound);
		}
		::g->caststate = &::g->states[mobjinfo[castorder[::g->castnum].type].seestate];
		::g->castframes = 0;
	}
	else
	{
		// just advance to next state in animation
		if (::g->caststate == &::g->states[S_PLAY_ATK1])
		{
			goto stopattack; // HACK: Oh, gross hack!
		}
		st = ::g->caststate->nextstate;
		::g->caststate = &::g->states[st];
		::g->castframes++;

		// sound hacks....
		switch (st)
		{
		case S_PLAY_ATK1:	sfx = sfx_dshtgn; break;
		case S_POSS_ATK2:	sfx = sfx_pistol; break;
		case S_SPOS_ATK2:	sfx = sfx_shotgn; break;
		case S_VILE_ATK2:	sfx = sfx_vilatk; break;
		case S_SKEL_FIST2:	sfx = sfx_skeswg; break;
		case S_SKEL_FIST4:	sfx = sfx_skepch; break;
		case S_SKEL_MISS2:	sfx = sfx_skeatk; break;
		case S_FATT_ATK8:
		case S_FATT_ATK5:
		case S_FATT_ATK2:	sfx = sfx_firsht; break;
		case S_CPOS_ATK2:
		case S_CPOS_ATK3:
		case S_CPOS_ATK4:	sfx = sfx_shotgn; break;
		case S_TROO_ATK3:	sfx = sfx_claw; break;
		case S_SARG_ATK2:	sfx = sfx_sgtatk; break;
		case S_BOSS_ATK2:
		case S_BOS2_ATK2:
		case S_HEAD_ATK2:	sfx = sfx_firsht; break;
		case S_SKULL_ATK2:	sfx = sfx_sklatk; break;
		case S_SPID_ATK2:
		case S_SPID_ATK3:	sfx = sfx_shotgn; break;
		case S_BSPI_ATK2:	sfx = sfx_plasma; break;
		case S_CYBER_ATK2:
		case S_CYBER_ATK4:
		case S_CYBER_ATK6:	sfx = sfx_rlaunc; break;
		case S_PAIN_ATK3:	sfx = sfx_sklatk; break;
		default: sfx = sfx_None; break;
		}

		if (sfx)
		{
			S_StartSound(nullptr, sfx);
		}
	}

	if (::g->castframes == 12)
	{
		// go into attack frame
		::g->castattacking = true;
		if (::g->castonmelee)
		{
			::g->caststate = &::g->states[mobjinfo[castorder[::g->castnum].type].meleestate];
		}
		else
		{
			::g->caststate = &::g->states[mobjinfo[castorder[::g->castnum].type].missilestate];
		}
		::g->castonmelee ^= 1;
		if (::g->caststate == &::g->states[S_NULL])
		{
			if (::g->castonmelee)
			{
				::g->caststate =
					&::g->states[mobjinfo[castorder[::g->castnum].type].meleestate];
			}
			else
			{
				::g->caststate =
					&::g->states[mobjinfo[castorder[::g->castnum].type].missilestate];
			}
		}
	}

	if (::g->castattacking)
	{
		if (::g->castframes == 24
			|| ::g->caststate == &::g->states[mobjinfo[castorder[::g->castnum].type].seestate])
		{
		stopattack:
			::g->castattacking = false;
			::g->castframes = 0;
			::g->caststate = &::g->states[mobjinfo[castorder[::g->castnum].type].seestate];
		}
	}

	::g->casttics = ::g->caststate->tics;
	if (::g->casttics == -1)
	{
		::g->casttics = 15;
	}
}


//
// F_CastResponder
//

bool F_CastResponder(event_t* ev)
{
	if (ev->type != ev_keydown)
	{
		return false;
	}

	if (::g->castdeath)
	{
		return true; // already in dying frames
	}

	// go into death frame
	::g->castdeath = true;
	::g->caststate = &::g->states[mobjinfo[castorder[::g->castnum].type].deathstate];
	::g->casttics = ::g->caststate->tics;
	::g->castframes = 0;
	::g->castattacking = false;
	if (mobjinfo[castorder[::g->castnum].type].deathsound)
	{
		S_StartSound(nullptr, mobjinfo[castorder[::g->castnum].type].deathsound);
	}

	return true;
}


static void F_CastPrint(const char* text)
{
	int		c;
	int		w;

	// find width
	const char* ch = text;
	int width = 0;

	while (ch)
	{
		c = *ch++;
		if (!c)
		{
			break;
		}
		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c> HU_FONTSIZE)
		{
			width += 4;
			continue;
		}

		w = SHORT(::g->hu_font[c]->width);
		width += w;
	}

	// draw it
	int cx = 160 - width / 2;
	ch = text;
	while (ch)
	{
		c = *ch++;
		if (!c)
		{
			break;
		}
		c = toupper(c) - HU_FONTSTART;
		if (c < 0 || c> HU_FONTSIZE)
		{
			cx += 4;
			continue;
		}

		w = SHORT(::g->hu_font[c]->width);
		V_DrawPatch(cx, 180, 0, ::g->hu_font[c]);
		cx += w;
	}

}


//
// F_CastDrawer
//
void V_DrawPatchFlipped(int x, int y, int scrn, patch_t* patch);

void F_CastDrawer()
{
	// erase the entire screen to a background
	V_DrawPatch(0, 0, 0, static_cast<patch_t*>(W_CacheLumpName("BOSSBACK", PU_CACHE_SHARED)));

	F_CastPrint(castorder[::g->castnum].name);

	// draw the current frame in the middle of the screen
	spritedef_t* sprdef = &::g->sprites[::g->caststate->sprite];
	spriteframe_t* sprframe = &sprdef->spriteframes[::g->caststate->frame & FF_FRAMEMASK];
	int lump = sprframe->lump[0];
	bool flip = static_cast<bool>(sprframe->flip[0]);

	patch_t* patch = static_cast<patch_t*>(W_CacheLumpNum(lump + ::g->firstspritelump, PU_CACHE_SHARED));
	if (flip)
	{
		V_DrawPatchFlipped(160, 170, 0, patch);
	}
	else
	{
		V_DrawPatch(160, 170, 0, patch);
	}
}


//
// F_DrawPatchCol
//
static void
F_DrawPatchCol(const int x, patch_t* patch, const int col) {
	postColumn_t* column = (postColumn_t*)((byte*)patch + LONG(patch->columnofs[col]));

	const int destx = x;

	// step through the posts in a column
	while (column->topdelta != 0xff)
	{
		byte* source = (byte*)column + 3;
		int desty = column->topdelta;
		int count = column->length;

		while (count--)
		{
			int scaledx = destx * GLOBAL_IMAGE_SCALER;
			int scaledy = desty * GLOBAL_IMAGE_SCALER;
			const byte src = *source++;

			for (size_t i = 0; i < GLOBAL_IMAGE_SCALER; i++) {
				for (size_t j = 0; j < GLOBAL_IMAGE_SCALER; j++) {
					::g->screens[0][(scaledx + j) + (scaledy + i) * SCREENWIDTH] = src;
				}
			}

			desty++;
		}
		column = (postColumn_t*)((byte*)column + column->length + 4);
	}
}


//
// F_BunnyScroll
//
static void F_BunnyScroll()
{
	char	name[64] = {};

	patch_t* p1 = static_cast<patch_t*>(W_CacheLumpName("PFUB2", PU_LEVEL_SHARED));
	patch_t* p2 = static_cast<patch_t*>(W_CacheLumpName("PFUB1", PU_LEVEL_SHARED));

	V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);

	int scrolled = 320 - (::g->finalecount - 230) / 2;
	scrolled = Min(scrolled, 320);
	scrolled = Max(scrolled, 0);

	for (int x = 0; x < ORIGINAL_WIDTH; x++)
	{
		if (x + scrolled < 320)
		{
			F_DrawPatchCol(x, p1, x + scrolled);
		}
		else
		{
			F_DrawPatchCol(x, p2, x + scrolled - 320);
		}
	}

	if (::g->finalecount < 1130)
	{
		return;
	}
	if (::g->finalecount < 1180)
	{
		V_DrawPatch((ORIGINAL_WIDTH - 13 * 8) / 2,
			(ORIGINAL_HEIGHT - 8 * 8) / 2, 0, static_cast<patch_t*>(W_CacheLumpName("END0", PU_CACHE_SHARED)));
		::g->laststage = 0;
		return;
	}

	int stage = (::g->finalecount - 1180) / 5;
	stage = Min(stage, 6);
	if (stage > ::g->laststage)
	{
		S_StartSound(nullptr, sfx_pistol);
		::g->laststage = stage;
	}

	idStr::snPrintf(name, sizeof(name),  "END%i", stage);
	V_DrawPatch((ORIGINAL_WIDTH - 13LL * 8) / 2, (ORIGINAL_HEIGHT - 8LL * 8) / 2, 0, static_cast<patch_t*>(W_CacheLumpName(name, PU_CACHE_SHARED)));
}


//
// F_Drawer
//
static void F_Drawer()
{
	if (::g->finalestage == 2)
	{
		F_CastDrawer();
		return;
	}

	if (!::g->finalestage)
	{
		F_TextWrite();
	}
	else
	{
		switch (::g->gameepisode)
		{
		case 1:
			if (::g->gamemode == retail)
			{
				V_DrawPatch(0, 0, 0,
					static_cast<patch_t*>(W_CacheLumpName("CREDIT", PU_CACHE_SHARED)));
			}
			else
			{
				V_DrawPatch(0, 0, 0,
					static_cast<patch_t*>(W_CacheLumpName("HELP2", PU_CACHE_SHARED)));
			}
			break;
		case 2:
			V_DrawPatch(0, 0, 0,
				static_cast<patch_t*>(W_CacheLumpName("VICTORY2", PU_CACHE_SHARED)));
			break;
		case 3:
			F_BunnyScroll();
			break;
		case 4:
			V_DrawPatch(0, 0, 0,
				static_cast<patch_t*>(W_CacheLumpName("ENDPIC", PU_CACHE_SHARED)));
			break;
		}
	}

}



