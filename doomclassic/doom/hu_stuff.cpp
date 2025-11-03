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

#include <utility>

#include "doomdef.h"

#include "z_zone.h"

#include "m_swap.h"

#include "hu_stuff.h"
#include "hu_lib.h"
#include "w_wad.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "Main.h"

//
// Locally used constants, shortcuts.
//


static constexpr const char* temp_chat_macros[] =
{
	HUSTR_CHATMACRO0,
	HUSTR_CHATMACRO1,
	HUSTR_CHATMACRO2,
	HUSTR_CHATMACRO3,
	HUSTR_CHATMACRO4,
	HUSTR_CHATMACRO5,
	HUSTR_CHATMACRO6,
	HUSTR_CHATMACRO7,
	HUSTR_CHATMACRO8,
	HUSTR_CHATMACRO9
};

static constexpr const char* player_names[] =
{
	HUSTR_PLRGREEN,
	HUSTR_PLRINDIGO,
	HUSTR_PLRBROWN,
	HUSTR_PLRRED
};



//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

static constexpr const char*	mapnames[] =
{
	    HUSTR_E1M1,
		HUSTR_E1M2,
		HUSTR_E1M3,
		HUSTR_E1M4,
		HUSTR_E1M5,
		HUSTR_E1M6,
		HUSTR_E1M7,
		HUSTR_E1M8,
		HUSTR_E1M9,

		HUSTR_E2M1,
		HUSTR_E2M2,
		HUSTR_E2M3,
		HUSTR_E2M4,
		HUSTR_E2M5,
		HUSTR_E2M6,
		HUSTR_E2M7,
		HUSTR_E2M8,
		HUSTR_E2M9,

		HUSTR_E3M1,
		HUSTR_E3M2,
		HUSTR_E3M3,
		HUSTR_E3M4,
		HUSTR_E3M5,
		HUSTR_E3M6,
		HUSTR_E3M7,
		HUSTR_E3M8,
		HUSTR_E3M9,

		HUSTR_E4M1,
		HUSTR_E4M2,
		HUSTR_E4M3,
		HUSTR_E4M4,
		HUSTR_E4M5,
		HUSTR_E4M6,
		HUSTR_E4M7,
		HUSTR_E4M8,
		HUSTR_E4M9,

		"NEWLEVEL",
		"NEWLEVEL",
		"NEWLEVEL",
		"NEWLEVEL",
		"NEWLEVEL",
		"NEWLEVEL",
		"NEWLEVEL",
		"NEWLEVEL",
		"NEWLEVEL"
};

static constexpr const char*	mapnames2[] =
{
	    HUSTR_1,
		HUSTR_2,
		HUSTR_3,
		HUSTR_4,
		HUSTR_5,
		HUSTR_6,
		HUSTR_7,
		HUSTR_8,
		HUSTR_9,
		HUSTR_10,
		HUSTR_11,

		HUSTR_12,
		HUSTR_13,
		HUSTR_14,
		HUSTR_15,
		HUSTR_16,
		HUSTR_17,
		HUSTR_18,
		HUSTR_19,
		HUSTR_20,

		HUSTR_21,
		HUSTR_22,
		HUSTR_23,
		HUSTR_24,
		HUSTR_25,
		HUSTR_26,
		HUSTR_27,
		HUSTR_28,
		HUSTR_29,
		HUSTR_30,
		HUSTR_31,
		HUSTR_32,
		HUSTR_33

};


static constexpr const char*	mapnamesp[] =
{
	    PHUSTR_1,
		PHUSTR_2,
		PHUSTR_3,
		PHUSTR_4,
		PHUSTR_5,
		PHUSTR_6,
		PHUSTR_7,
		PHUSTR_8,
		PHUSTR_9,
		PHUSTR_10,
		PHUSTR_11,

		PHUSTR_12,
		PHUSTR_13,
		PHUSTR_14,
		PHUSTR_15,
		PHUSTR_16,
		PHUSTR_17,
		PHUSTR_18,
		PHUSTR_19,
		PHUSTR_20,

		PHUSTR_21,
		PHUSTR_22,
		PHUSTR_23,
		PHUSTR_24,
		PHUSTR_25,
		PHUSTR_26,
		PHUSTR_27,
		PHUSTR_28,
		PHUSTR_29,
		PHUSTR_30,
		PHUSTR_31,
		PHUSTR_32
};

	// TNT WAD map names.
static constexpr const char *mapnamest[] =
{
	    THUSTR_1,
		THUSTR_2,
		THUSTR_3,
		THUSTR_4,
		THUSTR_5,
		THUSTR_6,
		THUSTR_7,
		THUSTR_8,
		THUSTR_9,
		THUSTR_10,
		THUSTR_11,

		THUSTR_12,
		THUSTR_13,
		THUSTR_14,
		THUSTR_15,
		THUSTR_16,
		THUSTR_17,
		THUSTR_18,
		THUSTR_19,
		THUSTR_20,

		THUSTR_21,
		THUSTR_22,
		THUSTR_23,
		THUSTR_24,
		THUSTR_25,
		THUSTR_26,
		THUSTR_27,
		THUSTR_28,
		THUSTR_29,
		THUSTR_30,
		THUSTR_31,
		THUSTR_32
};


static const char*	shiftxform;

constexpr const char english_shiftxform[] =
{

	    0,
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
		11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
		21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		31,
		' ', '!', '"', '#', '$', '%', '&',
		'"', // shift-'
		'(', ')', '*', '+',
		'<', // shift-,
		'_', // shift--
		'>', // shift-.
		'?', // shift-/
		')', // shift-0
		'!', // shift-1
		'@', // shift-2
		'#', // shift-3
		'$', // shift-4
		'%', // shift-5
		'^', // shift-6
		'&', // shift-7
		'*', // shift-8
		'(', // shift-9
		':',
		':', // shift-;
		'<',
		'+', // shift-=
		'>', '?', '@',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'[', // shift-[
		'!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
		']', // shift-]
		'"', '_',
		'\'', // shift-`
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
		'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'{', '|', '}', '~', 127
};

static unsigned char ForeignTranslation(const unsigned char ch)
{
	return ch;
}

void HU_Init()
{
	char	buffer[128] = {};

	shiftxform = english_shiftxform;

	// load the heads-up font
	index_t j = HU_FONTSTART;
	for (auto& i : ::g->hu_font)
	{
		idStr::snPrintf(buffer, sizeof(buffer), "STCFN%.3lld", j++);

		i = static_cast<patch_t*>(W_CacheLumpName(buffer, PU_STATIC_SHARED));
	}
}

static void HU_Stop()
{
	::g->headsupactive = false;
}

void HU_Start()
{
	const char*	s = nullptr;

	if (::g->headsupactive)
	{
		HU_Stop();
	}

	::g->plr = &::g->players[::g->consoleplayer];
	::g->message_on = false;
	::g->message_dontfuckwithme = false;
	::g->message_nottobefuckedwith = false;
	::g->chat_on = false;

	// create the message widget
	HUlib_initSText(&::g->w_message,
		HU_MSGX, HU_MSGY, HU_MSGHEIGHT,
		::g->hu_font,
		HU_FONTSTART, &::g->message_on);

	// create the map title widget
	HUlib_initTextLine(&::g->w_title,
		HU_TITLEX, HU_TITLEY,
		::g->hu_font,
		HU_FONTSTART);

	switch ( ::g->gamemode )
	{
	case shareware:
	case registered:
	case retail:
		s = HU_TITLE;
		break;
	case indetermined:
		s = nullptr;
		break;
	case commercial:
	default:
		if( DoomLib::expansionSelected == 5 ) {
			index_t map = ::g->gamemap;
			if( ::g->gamemap > 9 ) {
				map = 0;
			} 

			s = DoomLib::GetCurrentExpansion()->mapNames[ map - 1 ];
		} else {
			s = DoomLib::GetCurrentExpansion()->mapNames[ ::g->gamemap - 1 ];
		}

		
		break;
	}

	while (*s)
	{
		HUlib_addCharToTextLine(&::g->w_title, *(s++));
	}

	// create the chat widget
	HUlib_initIText(&::g->w_chat,
		HU_INPUTX, HU_INPUTY,
		::g->hu_font,
		HU_FONTSTART, &::g->chat_on);

	// create the inputbuffer widgets
	for (auto& i : ::g->w_inputbuffer)
	{
		HUlib_initIText(&i, 0, 0, nullptr, 0, &::g->always_off);
	}

	::g->headsupactive = true;

}

void HU_Drawer()
{
	HUlib_drawSText(&::g->w_message);
	HUlib_drawIText(&::g->w_chat);

	if (::g->automapactive)
	{
		HUlib_drawTextLine(&::g->w_title, false);
	}
}

void HU_Erase()
{
	HUlib_eraseSText(&::g->w_message);
	HUlib_eraseIText(&::g->w_chat);
	HUlib_eraseTextLine(&::g->w_title);

}

void HU_Ticker()
{
	// tick down message counter if message is up
	if (::g->message_counter && !(::g->message_counter -= 1))
	{
		::g->message_on = false;
		::g->message_nottobefuckedwith = false;
	}

	if ( ( m_inDemoMode.GetBool() == false && m_show_messages.GetBool() ) || ::g->message_dontfuckwithme)
	{

		// display message if necessary
		if ((::g->plr->message && !::g->message_nottobefuckedwith)
			|| (::g->plr->message && ::g->message_dontfuckwithme))
		{
			HUlib_addMessageToSText(&::g->w_message, nullptr, ::g->plr->message);
			::g->plr->message = nullptr;
			::g->message_on = true;
			::g->message_counter = HU_MSGTIMEOUT;
			::g->message_nottobefuckedwith = ::g->message_dontfuckwithme;
			::g->message_dontfuckwithme = false;
		}

	} // else ::g->message_on = false;
}


static void HU_queueChatChar(const unsigned char c)
{
	if (((::g->head + 1) & (QUEUESIZE - 1)) == ::g->tail)
	{
		::g->plr->message = HUSTR_MSGU;
	}
	else
	{
		::g->chatchars[::g->head] = c;
		::g->head = numeric_cast<index_t>((::g->head + 1) & (QUEUESIZE - 1));
	}
}

unsigned char HU_dequeueChatChar()
{
	unsigned char c = 0;

	if (::g->head != ::g->tail)
	{
		c = ::g->chatchars[::g->tail];
		::g->tail = numeric_cast<index_t>((::g->tail + 1) & (QUEUESIZE - 1));
	}
	else
	{
		c = 0;
	}

	return c;
}

bool HU_Responder(event_t *ev)
{
	bool		eatkey = false;

	if (ev)
	{
		const char* macromessage = nullptr;
		size_t			i = 0;

		static constexpr char destination_keys[MAXPLAYERS] =
		{
			HUSTR_KEYGREEN,
			HUSTR_KEYINDIGO,
			HUSTR_KEYBROWN,
			HUSTR_KEYRED
		};


		size_t numplayers = 0;
		for (const auto& player : ::g->players)
		{
			numplayers += player.playerInGame;
		}

		if (ev->data1 == KEY_RSHIFT)
		{
			::g->shiftdown = ev->type == ev_keydown;
			return false;
		}
		else if (ev->data1 == KEY_RALT || ev->data1 == KEY_LALT)
		{
			::g->altdown = ev->type == ev_keydown;
			return false;
		}

		if (ev->type != ev_keydown)
		{
			return false;
		}

		if (!::g->chat_on)
		{
			if (ev->data1 == HU_MSGREFRESH)
			{
				::g->message_on = true;
				::g->message_counter = HU_MSGTIMEOUT;
				eatkey = true;
			}
			else if (::g->netgame && ev->data1 == HU_INPUTTOGGLE)
			{
				eatkey = ::g->chat_on = true;
				HUlib_resetIText(&::g->w_chat);
				HU_queueChatChar(HU_BROADCAST);
			}
			else if (::g->netgame && numplayers > 2)
			{
				for (const auto& player : ::g->players)
				{
					if (ev->data1 == destination_keys[i])
					{
						if (player.playerInGame && std::not_equal_to<>()(i, ::g->consoleplayer))
						{
							eatkey = ::g->chat_on = true;
							HUlib_resetIText(&::g->w_chat);
							HU_queueChatChar(numeric_cast<char>(i + 1));
							break;
						}
						else if (std::cmp_equal(i, ::g->consoleplayer))
						{
							::g->num_nobrainers++;
							if (::g->num_nobrainers < 3)
							{
								::g->plr->message = HUSTR_TALKTOSELF1;
							}
							else if (::g->num_nobrainers < 6)
							{
								::g->plr->message = HUSTR_TALKTOSELF2;
							}
							else if (::g->num_nobrainers < 9)
							{
								::g->plr->message = HUSTR_TALKTOSELF3;
							}
							else if (::g->num_nobrainers < 32)
							{
								::g->plr->message = HUSTR_TALKTOSELF4;
							}
							else
							{
								::g->plr->message = HUSTR_TALKTOSELF5;
							}
						}
					}
				}
			}
		}
		else
		{
			unsigned char c = numeric_cast<BASE_TYPE(c)>(ev->data1);
			// send a macro
			if (::g->altdown)
			{
				c = numeric_cast<BASE_TYPE(c)>(c - '0');

				if (c > 9)
				{
					return false;
				}

				// I_PrintfE( "got here\n");
				macromessage = temp_chat_macros[c];

				// kill last message with a '\n'
				HU_queueChatChar(KEY_ENTER); // DEBUG!!!

				// send the macro message
				while (*macromessage)
				{
					HU_queueChatChar(*macromessage++);
				}

				HU_queueChatChar(KEY_ENTER);

				// leave chat mode and notify that it was sent
				::g->chat_on = false;
				strncpy_s(::g->lastmessage, temp_chat_macros[c], strlen(temp_chat_macros[c]));
				::g->plr->message = ::g->lastmessage;
				eatkey = true;
			}
			else
			{
				if (::g->shiftdown || (c >= 'a' && c <= 'z'))
				{
					c = shiftxform[c];
				}
				eatkey = HUlib_keyInIText(&::g->w_chat, c);
				if (eatkey)
				{
					// static unsigned char buf[20]; // DEBUG
					HU_queueChatChar(c);

					// idStr::snPrintf(buf, "KEY: %d => %d", ev->data1, c);
					//      ::g->plr->message = buf;
				}
				if (c == KEY_ENTER)
				{
					::g->chat_on = false;
					if (::g->w_chat.l.len)
					{
						strncpy_s(::g->lastmessage, ::g->w_chat.l.l, strlen(::g->w_chat.l.l));
						::g->plr->message = ::g->lastmessage;
					}
				}
				else if (c == KEY_ESCAPE)
				{
					::g->chat_on = false;
				}
			}
		}
	}
	return eatkey;

}

