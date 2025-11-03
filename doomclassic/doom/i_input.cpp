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
#include <cstdarg>
#include <sys/types.h>

#include "i_video.h"
#include "i_system.h"

#include "doomstat.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#include "sys/sys_public.h"

#include "g_game.h"

#define ALLOW_CHEATS	1



extern size_t PLAYERCOUNT;

constexpr size_t NUM_BUTTONS = 4;

static bool Cheat_God() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}
	::g->plyr->cheats ^= CF_GODMODE;
	if (::g->plyr->cheats & CF_GODMODE)
	{
		if (::g->plyr->mo)
		{
			::g->plyr->mo->health = 100;
		}

		::g->plyr->health = 100;
		::g->plyr->message = STSTR_DQDON;
	}
	else
	{
		::g->plyr->message = STSTR_DQDOFF;
	}
	return true;
}


static bool Cheat_NextLevel() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	G_ExitLevel();

	return true;
}

static bool Cheat_GiveAll() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	::g->plyr->armorpoints = ARMOR_VALUES[ARMOR_MEGA];
	::g->plyr->armortype = ARMOR_MEGA;

	size_t i = 0;
	for (i = 0; i < NUMWEAPONS; ++i)
	{
		::g->plyr->weaponowned[i] = true;
	}

	for (i = 0; i < NUMAMMO; ++i)
	{
		::g->plyr->ammo[i] = ::g->plyr->maxammo[i];
	}

	for (i = 0; i < NUMCARDS; ++i)
	{
		::g->plyr->cards[i] = true;
	}

	::g->plyr->message = STSTR_KFAADDED;

	return true;
}

static bool Cheat_GiveAmmo() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	::g->plyr->armorpoints = ARMOR_VALUES[ARMOR_MEGA];
	::g->plyr->armortype = ARMOR_MEGA;

	size_t i = 0;
	for (i = 0; i < NUMWEAPONS; ++i)
	{
		::g->plyr->weaponowned[i] = true;
	}

	for (i = 0; i < NUMAMMO; ++i)
	{
		::g->plyr->ammo[i] = ::g->plyr->maxammo[i];
	}

	::g->plyr->message = STSTR_KFAADDED;

	return true;
}

static bool Cheat_Choppers() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	::g->plyr->weaponowned[wp_chainsaw] = true;
	::g->plyr->message = "Chainsaw!";

	return true;
}

extern bool P_GivePower ( player_t*	player, const powertype_t power );

static void TogglePowerUp( const powertype_t power ) {
	ORDINAL_CHECK(power, NUMPOWERS);

	if (!::g->plyr->powers[power])
	{
		P_GivePower( ::g->plyr, power);
	}
	else if (power != pw_strength)
	{
		::g->plyr->powers[power] = 1;
	}
	else
	{
		::g->plyr->powers[power] = 0;
	}

	::g->plyr->message = STSTR_BEHOLDX;
}

static bool Cheat_GiveInvul() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( pw_invulnerability );

	return true;
}

static bool Cheat_GiveBerserk() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( pw_strength );
	return true;
}

static bool Cheat_GiveBlur() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( pw_invisibility );
	return true;
}

static bool Cheat_GiveRad() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( pw_ironfeet );
	return true;
}

static bool Cheat_GiveMap() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( pw_allmap );
	return true;
}

static bool Cheat_GiveLight() {
	if( PLAYERCOUNT != 1 || ::g->netgame ) {
		return false;
	}

	TogglePowerUp( pw_infrared );
	return true;
}



#ifndef __PS3__

static bool			tracking		= false;
static index_t		currentCode[NUM_BUTTONS]{};
static size_t		currentCheatLength = 0;

#endif

typedef bool(*cheat_command)();
struct cheatcode_t
{
	int			code[NUM_BUTTONS]{};
	cheat_command	function;
};

static cheatcode_t codes[] = {
	{.code = {0, 1, 1, 0}, .function = Cheat_God }, // a b b a
	{.code = {0, 0, 1, 1}, .function = Cheat_NextLevel }, // a a b b
	{.code = {1, 0, 1, 0}, .function = Cheat_GiveAmmo }, // b a b a
	{.code = {1, 1, 0, 0}, .function = Cheat_Choppers}, // b b a a
	{.code = {0, 1, 0, 1}, .function = Cheat_GiveAll },  // a b a b
	{.code = {2, 3, 3, 2}, .function = Cheat_GiveInvul }, // x y y x
	{.code = {2, 2, 2, 3}, .function = Cheat_GiveBerserk }, // x x x y
	{.code = {2, 2, 3, 3}, .function = Cheat_GiveBlur }, // x x y y
	{.code = {2, 3, 3, 3}, .function = Cheat_GiveRad }, // x y y y
	{.code = {3, 2, 3, 2}, .function = Cheat_GiveMap }, // y x y x
	{.code = {3, 3, 3, 2}, .function = Cheat_GiveLight}, // y y y x
};

static constexpr size_t numberOfCodes = std::size(codes);


static void BeginTrackingCheat() {
#if ALLOW_CHEATS
	tracking = true;
	currentCheatLength = 0;
	memset( currentCode, 0, sizeof( currentCode ) );
#endif
}

static void EndTrackingCheat() {
#if ALLOW_CHEATS
	tracking = false;
#endif
}

extern void S_StartSound ( void* origin, const sfxenum_e sfx_id );

static void CheckCheat(const index_t button ) {
#if ALLOW_CHEATS
	if( tracking && !::g->netgame ) {

		currentCode[ currentCheatLength++ ] = button;

		if( currentCheatLength == NUM_BUTTONS ) {
			for (auto& code : codes)
			{
				if( memcmp( &code.code[0], &currentCode[0], sizeof(currentCode) ) == 0 ) {
					if(code.function()) {
						S_StartSound(nullptr, sfx_cybsit);
					}
				}
			}
			// reset the code
			memset( currentCode, 0, sizeof( currentCode ) );
			currentCheatLength = 0;
		}
	}
#endif
}


static constexpr float xbox_deadzone = 0.28f;

// input event storage
//PRIVATE TO THE INPUT THREAD!



void I_InitInput()
{
}

void I_ShutdownInput() 
{
}


static float _joyAxisConvert(const short x, const float xbxScale, const float dScale, const float deadZone)
{
	//const float signConverted = x - 127;
	float y = x - 127;
	y		= y / xbxScale;
	return (fabs(y) < deadZone) ? 0.f : (y * dScale);
}


size_t I_PollMouseInputEvents() 
{
	constexpr size_t numEvents = 0;

	return numEvents;
}

bool I_ReturnMouseInputEvent(const index_t n, event_t* e) {
	if (e)
	{
		e->type = ev_mouse;
		e->data1 = e->data2 = e->data3 = 0;

		switch (::g->mouseEvents[n].type) {
		case IETAxis:
			switch (::g->mouseEvents[n].action)
			{
			case M_DELTAX:
				e->data2 = ::g->mouseEvents[n].data;
				break;
			case M_DELTAY:
				e->data3 = ::g->mouseEvents[n].data;
				break;
			default:
				break;
			}
			return true;

		default:
			break;
		}
	}

	return false;
}

size_t I_PollJoystickInputEvents() {
	constexpr size_t numEvents	= 0;

	return numEvents;
}

//
//  Translates the key currently in X_event
//
static keys_e xlatekey(const int key)
{
	keys_e rc = KEY_F1;
	
	switch (key)
	{
	case 0:	// A
		//rc = KEY_ENTER;
		//rc = ' ';
		rc = KEY_SPACE;
		break;
	case 1:	// B
		if( ::g->menuactive ) {
			rc = KEY_BACKSPACE;
		}
		else {
			//rc = '2';
			rc = KEY_2;
		}
		break;
	case 2: // X
		//rc = ' ';
		rc = KEY_TAB;
		break;
	case 3: // Y
		//rc = '1';
		rc = KEY_1;
		break;
	case 4:	// White
		rc = KEY_MINUS;
		break;
	case 5: // Black	
		rc = KEY_EQUALS;
		break;
	case 6: // Left triggers
		rc = KEY_RSHIFT;
		break;
	case 7: // Right
		rc = KEY_RCTRL;
		break;
	case 8:	// Up
		if( ::g->menuactive ) {
			rc = KEY_UPARROW;
		}
		else {
			//rc = KEY_ENTER;
			//rc = '3';
			rc = KEY_3;
		}
		break;
	case 9:
		if( ::g->menuactive ) {
			rc = KEY_DOWNARROW;
		}
		else {
			//rc = KEY_TAB;
			//rc = '5';
			rc = KEY_5;
		}
		break;
	case 10:
		if( ::g->menuactive ) {
			rc = KEY_UPARROW;
		}
		else {
			//rc = '1';
			//rc = '6';
			rc = KEY_6;
		}
		break;
	case 11:
		if( ::g->menuactive ) {
			rc = KEY_DOWNARROW;
		}
		else {
			//rc = '2';
			//rc = '4';
			rc = KEY_4;
		}
		break;
	case 12:	// start
		rc = KEY_ESCAPE;
		break;
	case 13:	//select
		//rc = KEY_ESCAPE;
		break;
	case 14:	// lclick
	case 15:	// rclick
		//rc = ' ';
		break;
	default:
		break;
	}
    return rc;
}

bool I_ReturnJoystickInputEvent( const index_t n, event_t* e) {
	if (e)
	{
		e->data1 = e->data2 = e->data3 = 0;

		switch (::g->joyEvents[n].type)
		{
		case IETAxis:
			e->type = ev_joystick;//ev_mouse;
			switch (::g->joyEvents[n].action)
			{
			case J_DELTAX:
				/*
							if (::g->joyEvents[n].data < 0)
								e->data2 = -1;
							else if (::g->joyEvents[n].data > 0)
								e->data2 = 1;
				*/
				e->data2 = ::g->joyEvents[n].data;
				break;
			case J_DELTAY:
				e->type = ev_mouse;
				e->data3 = ::g->joyEvents[n].data;
				break;
			default:
				break;
			}
			return true;
		case IETButtonAnalog:
		case IETButtonDigital:
			if (::g->joyEvents[n].data)
			{
				e->type = ev_keydown;
			}
			else
			{
				e->type = ev_keyup;
			}
			e->data1 = xlatekey(::g->joyEvents[n].action);
			return true;

		case IETNone:
			break;
		}
	}

	return false;
}

void I_EndJoystickInputEvents() {
	for (auto& joyEvent : ::g->joyEvents)
	{
		joyEvent.type = IETNone;
	}

}

