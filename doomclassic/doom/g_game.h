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

#ifndef __G_GAME__
#define __G_GAME__

#pragma once

#include "doomdef.h"
#include "d_event.h"



//
// GAME
//
void G_DeathMatchSpawnPlayer (const index_t playernum);

void G_InitNew ( skill_t skill, index_t episode, index_t map );

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferredInitNew (skill_t skill, index_t episode, index_t map);

void G_DeferredPlayDemo (const char* demo);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_LoadGame (const char* name);

bool G_DoLoadGame ();

// Called by M_Responder.
void G_SaveGame (const index_t slot, const char* description);

// Only called by startup code.
void G_RecordDemo (const char* name);

void G_BeginRecording ();

void G_PlayDemo (const char* name);
void G_TimeDemo (const char* name);
bool G_CheckDemoStatus ();

void G_ExitLevel ();
void G_SecretExitLevel ();

void G_WorldDone ();

void G_Ticker ();
bool G_Responder (event_t*	ev);

void G_ScreenShot ();

constexpr size_t MAXDEMOSIZE = 512ULL * 1024ULL;
constexpr size_t SAVEGAMESIZE = 256ULL * 1024ULL + MAXDEMOSIZE;


#endif

