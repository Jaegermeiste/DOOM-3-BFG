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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <cctype>
#include <utility>


#include "doomdef.h"
#include "dstrings.h"

#include "d_main.h"

#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "m_misc.h"
#include "r_local.h"


#include "hu_stuff.h"

#include "g_game.h"

#include "m_argv.h"
#include "m_swap.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "sounds.h"

#include "m_menu.h"


#include "Main.h"
//#include "../game/player/PlayerProfileDoom.h"
#include "sys/sys_session.h"
#include "sys/sys_signin.h"
#include "d3xp/Game_local.h"

extern idCVar in_useJoystick;

//
// defaulted values
//

// Show messages has default, 0 = off, 1 = on


// Blocky mode, has default, 0 = high, 1 = normal

// temp for ::g->screenblocks (0-9)

// -1 = no quicksave slot picked!

// 1 = message to be printed
// ...and here is the message string!

// message x & y

// timed message = no input from user



static constexpr auto gammamsg[5][26] =
{
	GAMMALVL0,
	GAMMALVL1,
	GAMMALVL2,
	GAMMALVL3,
	GAMMALVL4
};

// we are going to be entering a savegame string
// old save description before edit






//
// MENU TYPEDEFS
//





// graphic name of skulls
// warning: initializer-string for array of chars is too long
static constexpr const char skullName[2][/*8*/9] = 
{
	"M_SKULL1", "M_SKULL2"
};

// current menudef

//
// PROTOTYPES
//
static void M_NewGame(const index_t choice);
static void M_Episode(const index_t choice);
static void M_Expansion(const index_t choice);
static void M_ChooseSkill(const index_t choice);
static void M_LoadGame(const index_t choice);
static void M_LoadExpansion(const index_t choice);
static void M_SaveGame(const index_t choice);
static void M_Options(const index_t choice);
static void M_EndGame(const index_t choice);
static void M_ReadThis(const index_t choice);
static void M_ReadThis2(const index_t choice);
static void M_QuitDOOM(const index_t choice);
static void M_ExitGame(const index_t choice);
static void M_GameSelection(const index_t choice);
static void M_CancelExit(const index_t choice);
static void M_ChangeMessages(const index_t choice);
static void M_ChangeGPad(const index_t choice);
static void M_FullScreen(const index_t choice);
static void M_ChangeSensitivity(const index_t choice);
static void M_SfxVol(const index_t choice);
static void M_MusicVol(const index_t choice);
static void M_ChangeDetail(const index_t choice);
static void M_SizeDisplay(const index_t choice);
void M_StartGame(const index_t choice);
static void M_Sound(const index_t choice);

static void M_FinishReadThis(const index_t choice);
static void M_LoadSelect(const index_t choice);
static void M_SaveSelect(const index_t choice);
static void M_ReadSaveStrings();
static void M_QuickSave();
static void M_QuickLoad();

static void M_DrawMainMenu();
static void M_DrawQuit();
static void M_DrawReadThis1();
static void M_DrawReadThis2();
static void M_DrawNewGame();
static void M_DrawEpisode();
static void M_DrawOptions();
static void M_DrawSound();
static void M_DrawLoad();
static void M_DrawSave();

static void M_DrawSaveLoadBorder(const std::integral auto x, const std::integral auto y);
static void M_SetupNextMenu(menu_t *menudef);
static void M_DrawThermo(const std::integral auto x, const std::integral auto y, const std::integral auto thermWidth, const std::integral auto thermDot);
static void M_DrawEmptyCell(const menu_t *menu,int item);
static void M_DrawSelCell(const menu_t *menu,int item);
static void M_WriteText(const std::integral auto x, const std::integral auto y, const char *string);
static size_t  M_StringWidth(const char *string);
static size_t  M_StringHeight(const char *string);
void M_StartControlPanel();
static void M_StartMessage(const char *string, messageRoutine_t routine, bool input);
static void M_StopMessage();
static void M_ClearMenus ();




//
// DOOM MENU
//




//
// EPISODE SELECT
//



//
// NEW GAME
//





//
// OPTIONS MENU
//



//
// Read This! MENU 1 & 2
//






//
// SOUND VOLUME MENU
//



//
// LOAD GAME MENU
//



//
// SAVE GAME MENU
//

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
void M_ReadSaveStrings()
{
	char    name[256] = {};

	for (size_t i = 0; i < load_end; ++i)
	{
		if( common->GetCurrentGame() == DOOM_CLASSIC ) {
			idStr::snPrintf(name, sizeof(name),"DOOM\\%s%d.dsg",  SAVEGAMENAME, i );
		} else {
			if( DoomLib::idealExpansion == doom2 ) {
				idStr::snPrintf(name, sizeof(name), "DOOM2\\%s%d.dsg",  SAVEGAMENAME, i );
			} else {
				idStr::snPrintf(name, sizeof(name), "DOOM2_NRFTL\\%s%d.dsg",  SAVEGAMENAME, i );
			}
			
		}

		idFile* handle = fileSystem->OpenFileRead(name, false);

		if (handle == nullptr)
		{
			strncpy_s(&::g->savegamestrings[i][0], sizeof(::g->savegamestrings[i][0]),EMPTYSTRING, strlen(EMPTYSTRING));
			::g->LoadMenu[i].status = 0;
			continue;
		}

		std::ignore = handle->Read(&::g->savegamestrings[i], SAVESTRINGSIZE);

		fileSystem->CloseFile( handle );	

		strncpy_s( ::g->savegamepaths[i], name, strlen(name) );

		::g->LoadMenu[i].status = 1;
	}
}


//
// M_LoadGame & Cie.
//
void M_DrawLoad()
{
	V_DrawPatchDirect (72,28,0,static_cast<patch_t*>(W_CacheLumpName("M_LOADG",PU_CACHE_SHARED)));

	for (size_t i = 0; i < load_end; ++i)
	{
		M_DrawSaveLoadBorder(::g->LoadDef.x, ::g->LoadDef.y + LINEHEIGHT * i);
		M_WriteText(::g->LoadDef.x, ::g->LoadDef.y + LINEHEIGHT * i,::g->savegamestrings[i]);
	}
}



//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(const std::integral auto x, const std::integral auto y)
{
	V_DrawPatchDirect (x - 8, y + 7,0,static_cast<patch_t*>(W_CacheLumpName("M_LSLEFT",PU_CACHE_SHARED)));

	auto xx = x;
	for (size_t i = 0; i < 28; ++i)
	{
		V_DrawPatchDirect (xx, y + 7, 0,static_cast<patch_t*>(W_CacheLumpName("M_LSCNTR",PU_CACHE_SHARED)));
		xx += 8;
	}

	V_DrawPatchDirect (xx, y + 7, 0,static_cast<patch_t*>(W_CacheLumpName("M_LSRGHT",PU_CACHE_SHARED)));
}



//
// User wants to load this game
//
void M_LoadSelect(const index_t choice)
{
	ORDINAL_CHECK(choice, MAX_SAVE_GAMES);

	if( ::g->gamemode != commercial ) {
		G_LoadGame ( ::g->savegamepaths[ choice ] );
	} else {
		strncpy_s( DoomLib::loadGamePath, ::g->savegamepaths[ choice ], strlen(::g->savegamepaths[choice]) );
		DoomLib::SetCurrentExpansion( DoomLib::idealExpansion );
		DoomLib::skipToLoad = true;
	}
	M_ClearMenus ();
}


void M_LoadExpansion(const index_t choice)
{
	::g->exp = choice;

	if( choice == 0 ) {
		DoomLib::SetIdealExpansion( doom2 );
	}else {
		DoomLib::SetIdealExpansion( pack_nerve );
	}

	M_SetupNextMenu(&::g->LoadDef);
	M_ReadSaveStrings();
}

//
// Selected from DOOM menu
//
void M_LoadGame (const index_t choice)
{
	if (::g->netgame)
	{
		M_StartMessage(LOADNET, nullptr,false);
		return;
	}

	if (::g->gamemode == commercial) {
		M_SetupNextMenu(&::g->LoadExpDef);
	} else {
		M_SetupNextMenu(&::g->LoadDef);
		M_ReadSaveStrings();
	}
	
}


//
//  M_SaveGame & Cie.
//
void M_DrawSave()
{
	size_t i = 0;

	V_DrawPatchDirect (72,28,0,static_cast<patch_t*>(W_CacheLumpName("M_SAVEG",PU_CACHE_SHARED)));

	for (i = 0; i < load_end; ++i)
	{
		M_DrawSaveLoadBorder(::g->LoadDef.x, ::g->LoadDef.y + LINEHEIGHT * i);
		M_WriteText(::g->LoadDef.x, ::g->LoadDef.y + LINEHEIGHT * i,::g->savegamestrings[i]);
	}

	if (::g->saveStringEnter)
	{
		i = M_StringWidth(::g->savegamestrings[::g->saveSlot]);

		M_WriteText(::g->LoadDef.x + i, ::g->LoadDef.y + LINEHEIGHT * ::g->saveSlot,"_");
	}
}

//
// M_Responder calls this when user is finished
//
static void M_DoSave(const index_t slot)
{
	G_SaveGame (slot,::g->savegamestrings[slot]);
	M_ClearMenus ();

	// PICK QUICKSAVE SLOT YET?
	if (::g->quickSaveSlot == -2)
	{
		::g->quickSaveSlot = slot;
	}
}

//
// User wants to save. Start string input for M_Responder
//
//
// Locally used constants, shortcuts.
//
extern const char* mapnames[];
extern const char* mapnames2[];
void M_SaveSelect(const index_t choice)
{
	ORDINAL_CHECK(choice, MAX_SAVE_GAMES);

	const char* s = nullptr;
	const ExpansionData* exp = DoomLib::GetCurrentExpansion();

	switch ( ::g->gamemode )
	{
	case shareware:
	case registered:
	case retail:
		s = (exp->mapNames[(::g->gameepisode-1) * 9 + ::g->gamemap - 1]);
		break;
	case commercial:
	case indetermined:
	default:
		s = (exp->mapNames[::g->gamemap-1]);
		break;
	}

	::g->saveSlot = choice;

	strncpy_s(::g->savegamestrings[::g->saveSlot], s, strlen(s));

	M_DoSave(::g->saveSlot);
}

//
// Selected from DOOM menu
//
void M_SaveGame (const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	if (!::g->usergame)
	{
		M_StartMessage(SAVEDEAD, nullptr,false);
		return;
	}
	else if( ::g->plyr && ::g->plyr->mo && ::g->plyr->mo->health <= 0 ) {
		M_StartMessage(SAVEDEAD2, nullptr,false);
		return;
	}

	if (::g->gamestate != GS_LEVEL)
	{
		return;
	}

	// Reset back to what expansion we are currently playing.
	DoomLib::SetIdealExpansion( DoomLib::expansionSelected );

	M_SetupNextMenu(&::g->SaveDef);
	M_ReadSaveStrings();
}



//
//      M_QuickSave
//

static void M_QuickSaveResponse(const int ch)
{
	if (ch == KEY_ENTER)
	{
		M_DoSave(::g->quickSaveSlot);
		S_StartSound(nullptr,sfx_swtchx);
	}
}

void M_QuickSave()
{
	if (!::g->usergame)
	{
		S_StartSound(nullptr,sfx_oof);
		return;
	}

	if (::g->gamestate != GS_LEVEL)
	{
		return;
	}

	if (::g->quickSaveSlot < 0)
	{
		M_StartControlPanel();
		M_ReadSaveStrings();
		M_SetupNextMenu(&::g->SaveDef);
		::g->quickSaveSlot = -2;	// means to pick a slot now
		return;
	}

	idStr::snPrintf(::g->tempstring, sizeof(::g->tempstring), QSPROMPT,::g->savegamestrings[::g->quickSaveSlot]);
	M_StartMessage(::g->tempstring,M_QuickSaveResponse,true);
}



//
// M_QuickLoad
//
static void M_QuickLoadResponse(const int ch)
{
	if (ch == KEY_ENTER)
	{
		M_LoadSelect(::g->quickSaveSlot);
		S_StartSound(nullptr,sfx_swtchx);
	}
}


void M_QuickLoad()
{
	if (::g->netgame)
	{
		M_StartMessage(QLOADNET, nullptr,false);
		return;
	}

	if (::g->quickSaveSlot < 0)
	{
		M_StartMessage(QSAVESPOT, nullptr,false);
		return;
	}

	idStr::snPrintf(::g->tempstring, sizeof(::g->tempstring),QLPROMPT,::g->savegamestrings[::g->quickSaveSlot]);
	M_StartMessage(::g->tempstring,M_QuickLoadResponse,true);
}


//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1()
{
	::g->inhelpscreens = true;

	switch ( ::g->gamemode )
	{
	case commercial:
		V_DrawPatchDirect (0, 0, 0, static_cast<patch_t*>(W_CacheLumpName("HELP",PU_CACHE_SHARED)));
		break;
	case shareware:
	case registered:
	case retail:
		V_DrawPatchDirect (0, 0, 0, static_cast<patch_t*>(W_CacheLumpName("HELP1",PU_CACHE_SHARED)));
		break;
	case indetermined:
	default:
		break;
	}
	return;
}



//
// Read This Menus - optional second page.
//
void M_DrawReadThis2()
{
	::g->inhelpscreens = true;

	switch ( ::g->gamemode )
	{
	case retail:
	case commercial:
		// This hack keeps us from having to change menus.
		V_DrawPatchDirect (0, 0, 0, static_cast<patch_t*>(W_CacheLumpName("CREDIT",PU_CACHE_SHARED)));
		break;
	case shareware:
	case registered:
		V_DrawPatchDirect (0, 0, 0, static_cast<patch_t*>(W_CacheLumpName("HELP2",PU_CACHE_SHARED)));
		break;
	case indetermined:
	default:
		break;
	}
	return;
}


//
// Change Sfx & Music volumes
//
void M_DrawSound()
{
	V_DrawPatchDirect (60, 38, 0, static_cast<patch_t*>(W_CacheLumpName("M_SVOL",PU_CACHE_SHARED)));

	M_DrawThermo(::g->SoundDef.x, ::g->SoundDef.y + LINEHEIGHT * (sfx_vol + 1), 16, s_volume_sound.GetInteger() );
	M_DrawThermo(::g->SoundDef.x, ::g->SoundDef.y + LINEHEIGHT * (music_vol + 1), 16, s_volume_midi.GetInteger() );
}

void M_Sound(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	M_SetupNextMenu(&::g->SoundDef);
}

void M_SfxVol(const index_t choice)
{
	ORDINAL_CHECK(choice, 2);

	switch(choice)
	{
	case 0:
		s_volume_sound.SetInteger( s_volume_sound.GetInteger() - 1 );
		break;
	case 1:
		s_volume_sound.SetInteger( s_volume_sound.GetInteger() + 1 );
		break;
	default:
		break;
	}

	S_SetSfxVolume( s_volume_sound.GetInteger() );
}

void M_MusicVol(const index_t choice)
{
	ORDINAL_CHECK(choice, 2);

	switch(choice)
	{
	case 0:
		s_volume_midi.SetInteger( s_volume_midi.GetInteger() - 1 );
		break;
	case 1:
		s_volume_midi.SetInteger( s_volume_midi.GetInteger() + 1 );
		break;
	default:
		break;
	}

	S_SetMusicVolume( s_volume_midi.GetInteger() );
}




//
// M_DrawMainMenu
//
void M_DrawMainMenu()
{
	V_DrawPatchDirect (94, 2, 0, static_cast<patch_t*>(W_CacheLumpName("M_DOOM",PU_CACHE_SHARED)));
}

//
// M_DrawQuit
//
void M_DrawQuit()
{
	V_DrawPatchDirect (54, 38, 0, static_cast<patch_t*>(W_CacheLumpName("M_EXITO",PU_CACHE_SHARED)));
}



//
// M_NewGame
//
void M_DrawNewGame()
{
	V_DrawPatchDirect (96, 14, 0, static_cast<patch_t*>(W_CacheLumpName("M_NEWG",PU_CACHE_SHARED)));
	V_DrawPatchDirect (54, 38, 0, static_cast<patch_t*>(W_CacheLumpName("M_SKILL",PU_CACHE_SHARED)));
}

void M_NewGame(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	if (::g->netgame && !::g->demoplayback)
	{
		M_StartMessage(NEWGAME, nullptr,false);
		return;
	}

	if ( ::g->gamemode == commercial )
	{
		M_SetupNextMenu(&::g->ExpDef);
	}
	else
	{
		M_SetupNextMenu(&::g->EpiDef);
	}
}


//
//      M_Episode
//

void M_DrawEpisode()
{
	V_DrawPatchDirect (54,38,0,static_cast<patch_t*>(W_CacheLumpName("M_EPISOD",PU_CACHE_SHARED)));
}

static void M_VerifyNightmare(const int ch)
{
	if (ch != KEY_ENTER)
	{
		return;
	}

	G_DeferredInitNew(static_cast<skill_t>(nightmare),::g->epi + 1, 1);
	M_ClearMenus ();
}

void M_ChooseSkill(const index_t choice)
{
	ORDINAL_CHECK(choice, NUM_SKILLS);
	/*
	if (choice == nightmare)
	{
		M_StartMessage(NIGHTMARE,M_VerifyNightmare,true);
		return;
	}
	*/
	if ( ::g->gamemode != commercial ) {
		static int startLevel = 1;
		G_DeferredInitNew(static_cast<skill_t>(choice),::g->epi + 1, startLevel);
		M_ClearMenus ();
	} else {
		DoomLib::SetCurrentExpansion( DoomLib::idealExpansion );
		DoomLib::skipToNew = true;
		DoomLib::chosenSkill = choice;
		DoomLib::chosenEpisode = ::g->epi+1;
	}
}

void M_Episode(const index_t choice)
{
	auto _choice = choice;

	// Yet another hack...
	if ( (::g->gamemode == registered) && (_choice > 2))
	{
		I_PrintfE("M_Episode: 4th episode requires UltimateDOOM\n");
		_choice = 0;
	}

	::g->epi = numeric_cast<BASE_TYPE(::g->epi)>(_choice);

	M_SetupNextMenu(&::g->NewDef);
}

void M_Expansion(const index_t choice)
{
	::g->exp = choice;

	if( choice == 0 ) {
		DoomLib::SetIdealExpansion( doom2 );
	}else {
		DoomLib::SetIdealExpansion( pack_nerve );
	}

	M_SetupNextMenu(&::g->NewDef);
}

//
// M_Options
//
static constexpr auto detailNames[2][9]	= 
{
"M_GDHIGH", "M_GDLOW"
};

static constexpr auto msgNames[2][9] = 
{
"M_MSGOFF", "M_MSGON"
};

extern idCVar in_mouseSpeed;

static int M_GetMouseSpeedForMenu( const float cvarValue ) {
	const float shiftedMouseSpeed = cvarValue - 0.25f;
	const float normalizedMouseSpeed = shiftedMouseSpeed / ( 4.0f - 0.25f );
	const float scaledMouseSpeed = normalizedMouseSpeed * 15.0f;
	const int roundedMouseSpeed = numeric_cast<int>( scaledMouseSpeed + 0.5f );
	
	return roundedMouseSpeed;
}

void M_DrawOptions()
{
	V_DrawPatchDirect (108,15,0,static_cast<patch_t*>(W_CacheLumpName("M_OPTTTL",PU_CACHE_SHARED)));

	//V_DrawPatchDirect (::g->OptionsDef.x + 175,::g->OptionsDef.y+LINEHEIGHT*detail,0,
	//	(patch_t*)W_CacheLumpName(detailNames[::g->detailLevel],PU_CACHE_SHARED));

	const int fullscreenOnOff = r_fullscreen.GetInteger() >= 1 ? 1 : 0;

	V_DrawPatchDirect (::g->OptionsDef.x + 150,::g->OptionsDef.y+LINEHEIGHT*endgame,0,
		static_cast<patch_t*>(W_CacheLumpName(msgNames[fullscreenOnOff],PU_CACHE_SHARED)));

	V_DrawPatchDirect (::g->OptionsDef.x + 120,::g->OptionsDef.y+LINEHEIGHT*scrnsize,0,
		static_cast<patch_t*>(W_CacheLumpName(msgNames[in_useJoystick.GetInteger()],PU_CACHE_SHARED)));

	V_DrawPatchDirect (::g->OptionsDef.x + 120,::g->OptionsDef.y+LINEHEIGHT*messages,0,
		static_cast<patch_t*>(W_CacheLumpName(msgNames[m_show_messages.GetInteger()],PU_CACHE_SHARED)));

	const int roundedMouseSpeed = M_GetMouseSpeedForMenu( in_mouseSpeed.GetFloat() );

	M_DrawThermo( ::g->OptionsDef.x, ::g->OptionsDef.y + LINEHEIGHT * ( mousesens + 1 ), 16, roundedMouseSpeed );

	//M_DrawThermo(::g->OptionsDef.x,::g->OptionsDef.y+LINEHEIGHT*(scrnsize+1),
	//	9,::g->screenSize);
}

void M_Options(const index_t choice)
{
	M_SetupNextMenu(&::g->OptionsDef);
}



//
//      Toggle messages on/off
//
void M_ChangeMessages(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	m_show_messages.SetBool( !m_show_messages.GetBool() );

	if (!m_show_messages.GetBool())
	{
		::g->players[::g->consoleplayer].message = MSGOFF;
	}
	else
	{
		::g->players[::g->consoleplayer].message = MSGON ;
	}

	::g->message_dontfuckwithme = true;
}

//
//      Toggle messages on/off
//
void M_ChangeGPad(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	in_useJoystick.SetBool( !in_useJoystick.GetBool() );

	::g->message_dontfuckwithme = true;
}

//
//      Toggle Fullscreen
//
void M_FullScreen( const index_t choice ) {
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	r_fullscreen.SetInteger( r_fullscreen.GetInteger() ? 0: 1 );
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
}

//
// M_EndGame
//
static void M_EndGameResponse(const int ch)
{
	if (ch != KEY_ENTER)
	{
		return;
	}

	::g->currentMenu->lastOn = ::g->itemOn;

	M_ClearMenus ();
	D_StartTitle ();
}

void M_EndGame(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	if (!::g->usergame)
	{
		S_StartSound(nullptr,sfx_oof);
		return;
	}

	if (::g->netgame)
	{
		M_StartMessage(NETEND, nullptr,false);
		return;
	}

	M_StartMessage(ENDGAME,M_EndGameResponse,true);
}




//
// M_ReadThis
//
void M_ReadThis(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;
}

void M_ReadThis2(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;
}

void M_FinishReadThis(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	M_SetupNextMenu(&::g->MainDef);
}




//
// M_QuitDOOM
//
static void M_QuitResponse(int ch)
{
	// Exceptions disabled by default on PS3
	//throw "";
}

void M_QuitDOOM(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	M_SetupNextMenu(&::g->QuitDef);
	//M_StartMessage("are you sure?\npress A to quit, or B to cancel",M_QuitResponse,true);
	//common->SwitchToGame( DOOM3_BFG );
}

void M_ExitGame(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	common->Quit();
}

void M_CancelExit(const index_t choice) {
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	M_SetupNextMenu(&::g->MainDef);
}

void M_GameSelection(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	common->SwitchToGame( DOOM3_BFG );
}

extern idCVar in_mouseSpeed;

void M_ChangeSensitivity(const index_t choice)
{
	int roundedMouseSpeed = M_GetMouseSpeedForMenu( in_mouseSpeed.GetFloat() );

	switch(choice)
	{
	case 0:
		if ( roundedMouseSpeed > 0 ) {
			roundedMouseSpeed--;
		}
		break;
	case 1:
		if ( roundedMouseSpeed < 15 ) {
			roundedMouseSpeed++;
		}
		break;
	default:
		break;
	}

	const float normalizedNewMouseSpeed = numeric_cast<float>(roundedMouseSpeed) / 15.0f;
	const float rescaledNewMouseSpeed = 0.25f + ( ( 4.0f - 0.25f ) * normalizedNewMouseSpeed );

	in_mouseSpeed.SetFloat( rescaledNewMouseSpeed );
}




void M_ChangeDetail(const index_t choice)
{
	// warning: unused parameter 'const index_t choice'
	std::ignore = choice;

	::g->detailLevel = 1 - ::g->detailLevel;

	// FIXME - does not work. Remove anyway?
	I_PrintfE("M_ChangeDetail: low detail mode n.a.\n");

	return;

	/*R_SetViewSize (::g->screenblocks, ::g->detailLevel);

	if (!::g->detailLevel)
	::g->players[::g->consoleplayer].message = DETAILHI;
	else
	::g->players[::g->consoleplayer].message = DETAILLO;*/
}




void M_SizeDisplay(const index_t choice)
{
	switch(choice)
	{
	case 0:
		if (::g->screenSize > 7)
		{
			::g->screenblocks--;
			::g->screenSize--;
		}
		break;
	case 1:
		if (::g->screenSize < 8)
		{
			::g->screenblocks++;
			::g->screenSize++;
		}
		break;
	default:
		break;
	}

	R_SetViewSize (::g->screenblocks, ::g->detailLevel);
}




//
//      Menu Functions
//
void M_DrawThermo
(const std::integral auto x,
 const std::integral auto y,
 const std::integral auto thermWidth,
 const std::integral auto thermDot )
{
	auto xx = x;
	V_DrawPatchDirect (xx, y,0,static_cast<patch_t*>(W_CacheLumpName("M_THERML",PU_CACHE_SHARED)));
	xx += 8;

	for (size_t i = 0; std::cmp_less(i, thermWidth); ++i)
	{
		V_DrawPatchDirect (xx, y,0,static_cast<patch_t*>(W_CacheLumpName("M_THERMM",PU_CACHE_SHARED)));
		xx += 8;
	}

	V_DrawPatchDirect (xx, y,0,static_cast<patch_t*>(W_CacheLumpName("M_THERMR",PU_CACHE_SHARED)));

	V_DrawPatchDirect ((x + 8) + thermDot * 8, y,
		0, static_cast<patch_t*>(W_CacheLumpName("M_THERMO",PU_CACHE_SHARED)));
}



void M_DrawEmptyCell (const menu_t* menu, const int item )
{
	if (menu)
	{
		V_DrawPatchDirect(menu->x - 10, menu->y + item * LINEHEIGHT - 1, 0,
			static_cast<patch_t*>(W_CacheLumpName("M_CELL1", PU_CACHE_SHARED)));
	}
}

void M_DrawSelCell (const menu_t* menu, const int item )
{
	V_DrawPatchDirect (menu->x - 10, menu->y + item * LINEHEIGHT - 1, 0,
		static_cast<patch_t*>(W_CacheLumpName("M_CELL2",PU_CACHE_SHARED)));
}


void M_StartMessage ( const char* string, const messageRoutine_t &routine, const bool input )
{
	if (string)
	{
		::g->messageLastMenuActive = ::g->menuactive;
		::g->messageToPrint = 1;
		::g->messageString = string;
		::g->messageRoutine = routine;
		::g->messageNeedsInput = input;
		::g->menuactive = true;
	}
}


void M_StopMessage()
{
	::g->menuactive = ::g->messageLastMenuActive;
	::g->messageToPrint = 0;
}


//
// Find string width from ::g->hu_font chars
//
size_t M_StringWidth( const char* string )
{
	size_t w = 0;

	for (size_t i = 0; i < strlen(string); ++i)
	{
		const index_t c = idStr::ToUpper(string[i]) - HU_FONTSTART;
		if (c < 0 || c >= HU_FONTSIZE)
		{
			w += 4;
		}
		else
		{
			w += SHORT (::g->hu_font[c]->width);
		}
	}

	return w;
}



//
//      Find string height from ::g->hu_font chars
//
size_t M_StringHeight( const char* string )
{
	const size_t height = SHORT(::g->hu_font[0]->height);

	size_t h = height;
	const size_t strLen = strlen(string);

	for (size_t i = 0; i < strLen; ++i)
	{
		if (string[i] == '\n')
		{
			h += height;
		}
	}

	return h;
}


//
//      Write a string using the ::g->hu_font
//
void M_WriteText
(const std::integral auto x,
 const std::integral auto y,
 const char*		string)
{
	const char* ch = string;
	auto cx = x;
	auto cy = y;

	while(true)
	{
		index_t c = numeric_cast<index_t>(*ch++);

		if (!c)
		{
			break;
		}
		if (c == '\n')
		{
			cx = x;
			cy += 12;
			continue;
		}

		c = numeric_cast<BASE_TYPE(c)>(idStr::ToUpper(c) - HU_FONTSTART);

		if (c < 0 || c>= HU_FONTSIZE)
		{
			cx += 4;
			continue;
		}

		const auto w = SHORT(::g->hu_font[c]->width);

		if (std::cmp_greater(cx + w, SCREENWIDTH))
		{
			break;
		}

		V_DrawPatchDirect(cx, cy, 0, ::g->hu_font[c]);

		cx += w;
	}
}



//
// CONTROL PANEL
//

//
// M_Responder
//
bool M_Responder (const event_t* ev)
{
	if (ev)
	{
		int ch = -1;

		if (ev->type == ev_joystick && ::g->joywait < I_GetTime())
		{
			if (ev->data3 == -1)
			{
				ch = KEY_UPARROW;
				::g->joywait = I_GetTime() + 5;
			}
			else if (ev->data3 == 1)
			{
				ch = KEY_DOWNARROW;
				::g->joywait = I_GetTime() + 5;
			}

			if (ev->data2 == -1)
			{
				ch = KEY_LEFTARROW;
				::g->joywait = I_GetTime() + 2;
			}
			else if (ev->data2 == 1)
			{
				ch = KEY_RIGHTARROW;
				::g->joywait = I_GetTime() + 2;
			}

			if (ev->data1 & 1)
			{
				ch = KEY_ENTER;
				::g->joywait = I_GetTime() + 5;
			}
			if (ev->data1 & 2)
			{
				ch = KEY_BACKSPACE;
				::g->joywait = I_GetTime() + 5;
			}
		}
		else
		{
			if (ev->type == ev_mouse && ::g->mousewait < I_GetTime())
			{
				::g->mmenu_mousey += ev->data3;
				if (::g->mmenu_mousey < ::g->lasty - 30)
				{
					ch = KEY_DOWNARROW;
					::g->mousewait = I_GetTime() + 5;
					::g->mmenu_mousey = ::g->lasty -= 30;
				}
				else if (::g->mmenu_mousey > ::g->lasty + 30)
				{
					ch = KEY_UPARROW;
					::g->mousewait = I_GetTime() + 5;
					::g->mmenu_mousey = ::g->lasty += 30;
				}

				::g->mmenu_mousex += ev->data2;
				if (::g->mmenu_mousex < ::g->lastx - 30)
				{
					ch = KEY_LEFTARROW;
					::g->mousewait = I_GetTime() + 5;
					::g->mmenu_mousex = ::g->lastx -= 30;
				}
				else if (::g->mmenu_mousex > ::g->lastx + 30)
				{
					ch = KEY_RIGHTARROW;
					::g->mousewait = I_GetTime() + 5;
					::g->mmenu_mousex = ::g->lastx += 30;
				}

				if (ev->data1 & 1)
				{
					ch = KEY_ENTER;
					::g->mousewait = I_GetTime() + 15;
				}

				if (ev->data1 & 2)
				{
					ch = KEY_BACKSPACE;
					::g->mousewait = I_GetTime() + 15;
				}
			}
			else
				if (ev->type == ev_keydown)
				{
					ch = ev->data1;
				}
		}

		if (ch == -1)
		{
			return false;
		}


		// Save Game string input
		if (::g->saveStringEnter)
		{
			switch (ch)
			{
			case KEY_BACKSPACE:
				if (::g->saveCharIndex > 0)
				{
					::g->saveCharIndex--;
					::g->savegamestrings[::g->saveSlot][::g->saveCharIndex] = 0;
				}
				break;

			case KEY_ESCAPE:
				::g->saveStringEnter = 0;
				strncpy_s(&::g->savegamestrings[::g->saveSlot][0], sizeof(&::g->savegamestrings[::g->saveSlot][0]), ::g->saveOldString, strlen(::g->saveOldString));
				break;

			case KEY_ENTER:
				::g->saveStringEnter = 0;
				if (::g->savegamestrings[::g->saveSlot][0])
				{
					M_DoSave(::g->saveSlot);
				}
				break;

			default:
				ch = toupper(ch);
				if (ch != 32)
				{
					if (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE)
					{
						break;
					}
				}
				if (ch >= 32 && ch <= 127 &&
					std::cmp_less(::g->saveCharIndex, SAVESTRINGSIZE - 1) &&
					M_StringWidth(::g->savegamestrings[::g->saveSlot]) <
					(SAVESTRINGSIZE - 2) * 8)
				{
					::g->savegamestrings[::g->saveSlot][::g->saveCharIndex++] = ch;
					::g->savegamestrings[::g->saveSlot][::g->saveCharIndex] = 0;
				}
				break;
			}
			return true;
		}

		// Take care of any messages that need input
		if (::g->messageToPrint)
		{
			if (::g->messageNeedsInput == true &&
				!(ch == KEY_ENTER || ch == KEY_BACKSPACE || ch == KEY_ESCAPE))
			{
				return false;
			}

			::g->menuactive = ::g->messageLastMenuActive;
			::g->messageToPrint = 0;
			if (::g->messageRoutine)
			{
				::g->messageRoutine(ch);
			}

			S_StartSound(nullptr, sfx_swtchx);
			return true;
		}
		/*
			if (::g->devparm && ch == KEY_F1)
			{
				G_ScreenShot ();
				return true;
			}

			// F-Keys
			if (!::g->menuactive)
				switch(ch)
			{
				case KEY_MINUS:         // Screen size down
					if (::g->automapactive || ::g->chat_on)
						return false;
					//M_SizeDisplay(0);
					S_StartSound(NULL,sfx_stnmov);
					return true;

				case KEY_EQUALS:        // Screen size up
					if (::g->automapactive || ::g->chat_on)
						return false;
					//M_SizeDisplay(1);
					S_StartSound(NULL,sfx_stnmov);
					return true;

				case KEY_F1:            // Help key
					M_StartControlPanel ();

					if ( ::g->gamemode == retail )
						::g->currentMenu = &::g->ReadDef2;
					else
						::g->currentMenu = &::g->ReadDef1;

					::g->itemOn = 0;
					S_StartSound(NULL,sfx_swtchn);
					return true;

				case KEY_F2:            // Save
					M_StartControlPanel();
					S_StartSound(NULL,sfx_swtchn);
					M_SaveGame(0);
					return true;

				case KEY_F3:            // Load
					M_StartControlPanel();
					S_StartSound(NULL,sfx_swtchn);
					M_LoadGame(0);
					return true;

				case KEY_F4:            // Sound Volume
					M_StartControlPanel ();
					::g->currentMenu = &::g->SoundDef;
					::g->itemOn = sfx_vol;
					S_StartSound(NULL,sfx_swtchn);
					return true;

				case KEY_F5:            // Detail toggle
					M_ChangeDetail(0);
					S_StartSound(NULL,sfx_swtchn);
					return true;

				case KEY_F6:            // Quicksave
					S_StartSound(NULL,sfx_swtchn);
					M_QuickSave();
					return true;

				case KEY_F7:            // End game
					S_StartSound(NULL,sfx_swtchn);
					M_EndGame(0);
					return true;

				case KEY_F8:            // Toggle messages
					M_ChangeMessages(0);
					S_StartSound(NULL,sfx_swtchn);
					return true;

				case KEY_F9:            // Quickload
					S_StartSound(NULL,sfx_swtchn);
					M_QuickLoad();
					return true;

				case KEY_F10:           // Quit DOOM
					S_StartSound(NULL,sfx_swtchn);
					M_QuitDOOM(0);
					return true;

				case KEY_F11:           // gamma toggle
					::g->usegamma++;
					if (::g->usegamma > 4)
						::g->usegamma = 0;
					::g->players[::g->consoleplayer].message = gammamsg[::g->usegamma];
					I_SetPalette ((byte*)W_CacheLumpName ("PLAYPAL",PU_CACHE_SHARED));
					return true;

			}
		*/

		// Pop-up menu?
		if (!::g->menuactive)
		{
			if (ch == KEY_ESCAPE && (::g->gamestate == GS_LEVEL || ::g->gamestate == GS_INTERMISSION || ::g->gamestate == GS_FINALE))
			{
				M_StartControlPanel();

				S_StartSound(nullptr, sfx_swtchn);
				return true;
			}

			return false;
		}

		// Keys usable within menu
		switch (ch)
		{
		case KEY_DOWNARROW:
			do
			{
				if (std::cmp_greater(::g->itemOn + 1, ::g->currentMenu->numitems - 1))
				{
					::g->itemOn = 0;
				}
				else
				{
					::g->itemOn++;
				}
				S_StartSound(nullptr, sfx_pstop);
			} while (::g->currentMenu->menuitems[::g->itemOn].status == -1);
			return true;

		case KEY_UPARROW:
			do
			{
				if (!::g->itemOn)
				{
					::g->itemOn = numeric_cast<BASE_TYPE(::g->itemOn)>(::g->currentMenu->numitems) - 1;
				}
				else
				{
					::g->itemOn--;
				}
				S_StartSound(nullptr, sfx_pstop);
			} while (::g->currentMenu->menuitems[::g->itemOn].status == -1);
			return true;

		case KEY_LEFTARROW:
			if (::g->currentMenu->menuitems[::g->itemOn].routine &&
				::g->currentMenu->menuitems[::g->itemOn].status == 2)
			{
				S_StartSound(nullptr, sfx_stnmov);
				::g->currentMenu->menuitems[::g->itemOn].routine(0);
			}
			return true;

		case KEY_RIGHTARROW:
			if (::g->currentMenu->menuitems[::g->itemOn].routine &&
				::g->currentMenu->menuitems[::g->itemOn].status == 2)
			{
				S_StartSound(nullptr, sfx_stnmov);
				::g->currentMenu->menuitems[::g->itemOn].routine(1);
			}
			return true;

		case KEY_ENTER:
			if (::g->currentMenu->menuitems[::g->itemOn].routine &&
				::g->currentMenu->menuitems[::g->itemOn].status)
			{
				::g->currentMenu->lastOn = ::g->itemOn;
				if (::g->currentMenu->menuitems[::g->itemOn].status == 2)
				{
					::g->currentMenu->menuitems[::g->itemOn].routine(1);      // right arrow
					S_StartSound(nullptr, sfx_stnmov);
				}
				else
				{
					::g->currentMenu->menuitems[::g->itemOn].routine(::g->itemOn);
					S_StartSound(nullptr, sfx_pistol);
				}
			}
			return true;

		case KEY_ESCAPE:
		case KEY_BACKSPACE:
			::g->currentMenu->lastOn = ::g->itemOn;
			if (::g->currentMenu->prevMenu)
			{
				::g->currentMenu = ::g->currentMenu->prevMenu;
				::g->itemOn = ::g->currentMenu->lastOn;
				S_StartSound(nullptr, sfx_swtchn);
			}
			else if (::g->currentMenu == &::g->MainDef && (!::g->demoplayback && ::g->gamestate != GS_DEMOSCREEN)) {
				M_ClearMenus();
				::g->paused = false;
			}
			return true;

		default:
			size_t i = 0;

			for (i = ::g->itemOn + 1; std::cmp_less(i, ::g->currentMenu->numitems); ++i)
			{
				if (::g->currentMenu->menuitems[i].alphaKey == ch)
				{
					::g->itemOn = numeric_cast<BASE_TYPE(::g->itemOn)>(i);
					S_StartSound(nullptr, sfx_pstop);
					return true;
				}
			}

			for (i = 0; std::cmp_less_equal(i, ::g->itemOn); ++i)
			{
				if (::g->currentMenu->menuitems[i].alphaKey == ch)
				{
					::g->itemOn = numeric_cast<BASE_TYPE(::g->itemOn)>(i);
					S_StartSound(nullptr, sfx_pstop);
					return true;
				}
			}

			break;
		}
	}

	return false;
}



//
// M_StartControlPanel
//
void M_StartControlPanel ()
{
	// intro might call this repeatedly
	if (::g->menuactive)
	{
		return;
	}

	::g->menuactive = true;
	::g->currentMenu = &::g->MainDef;
	::g->itemOn = ::g->currentMenu->lastOn;
}


//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer ()
{
	size_t i = 0;
	char string[40] = {};
	size_t md_x = 0, md_y = 0;

	::g->inhelpscreens = false;

	// Horiz. & Vertically center string and print it.
	if (::g->messageToPrint)
	{
		size_t start = 0;
		md_y = 100 - M_StringHeight(::g->messageString) / 2;
		while (*(::g->messageString + start))
		{
			for (i = 0; i < strlen(::g->messageString + start); ++i)
			{
				if (*(::g->messageString + start + i) == '\n')
				{
					memset(string, 0, sizeof(string));
					strncpy_s(string, ::g->messageString + start, i);
					start += i + 1;
					break;
				}
			}

			if (i == strlen(::g->messageString + start))
			{
				strncpy_s(string, ::g->messageString + start, strlen(::g->messageString + start));
				start += i;
			}

			md_x = 160 - M_StringWidth(string) / 2;
			M_WriteText(md_x, md_y, string);
			md_y += SHORT(::g->hu_font[0]->height);
		}
		return;
	}

	if (!::g->menuactive)
	{
		return;
	}

	if (::g->currentMenu->routine)
	{
		::g->currentMenu->routine(); // call Draw routine
	}

	// DRAW MENU
	md_x = ::g->currentMenu->x;
	md_y = ::g->currentMenu->y;
	const size_t max = ::g->currentMenu->numitems;

	for (i=0; std::cmp_less(i, max);++i)
	{
		if (::g->currentMenu->menuitems[i].name[0])
		{
			V_DrawPatchDirect (md_x, md_y, 0, 
			                   static_cast<patch_t*>(W_CacheLumpName(::g->currentMenu->menuitems[i].name,PU_CACHE_SHARED)));
		}
		md_y += LINEHEIGHT;
	}


	// DRAW SKULL
	V_DrawPatchDirect( md_x + SKULLXOFF,::g->currentMenu->y - 5 + ::g->itemOn * LINEHEIGHT, 0,
		static_cast<patch_t*>(W_CacheLumpName(skullName[::g->whichSkull],PU_CACHE_SHARED)));
}


//
// M_ClearMenus
//
void M_ClearMenus ()
{
	::g->menuactive = false;
	// if (!::g->netgame && ::g->usergame && ::g->paused)
	//       ::g->sendpause = true;
}




//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
	if (menudef)
	{
		::g->currentMenu = menudef;
		::g->itemOn = ::g->currentMenu->lastOn;
	}
}


//
// M_Ticker
//
void M_Ticker ()
{
	if (--::g->skullAnimCounter <= 0)
	{
		::g->whichSkull ^= 1;
		::g->skullAnimCounter = 8;
	}
}


//
// M_Init
//
void M_Init ()
{	
	::g->currentMenu = &::g->MainDef;
	::g->menuactive = true;
	::g->itemOn = ::g->currentMenu->lastOn;
	::g->whichSkull = 0;
	::g->skullAnimCounter = 10;
	::g->screenSize = ::g->screenblocks - 3;
	::g->messageToPrint = 0;
	::g->messageString = nullptr;
	::g->messageLastMenuActive = ::g->menuactive;
	::g->quickSaveSlot = -1;

	// Here we could catch other version dependencies,
	//  like HELP1/2, and four episodes.


	switch ( ::g->gamemode )
	{
	case commercial:
		// This is used because DOOM 2 had only one HELP
		//  page. I use CREDIT as second page now, but
		//  kept this hack for educational purposes.
		//::g->MainMenu[readthis] = ::g->MainMenu[quitdoom];
		//::g->MainDef.numitems--;
		::g->MainDef.y += 8;
		::g->NewDef.prevMenu = &::g->MainDef;
		//::g->ReadDef1.routine = M_DrawReadThis1;
		//::g->ReadDef1.x = 330;
		//::g->ReadDef1.y = 165;
		//::g->ReadMenu1[0].routine = M_FinishReadThis;
		break;
	case shareware:
		// Episode 2 and 3 are handled,
		//  branching to an ad screen.
	case registered:
		// We need to remove the fourth episode.
		::g->EpiDef.numitems--;
		break;
	case retail:
		// We are fine.
	case indetermined:
	default:
		break;
	}
}


