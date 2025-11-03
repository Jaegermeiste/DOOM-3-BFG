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

#ifndef __I_VIDEO__
#define __I_VIDEO__

#pragma once

#include "doomtype.h"
#include "d_event.h"

#ifdef __GNUG__
#pragma interface
#endif

// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode
void I_InitGraphics ();


void I_ShutdownGraphics();

// Takes full 8 bit values.
void I_SetPalette (byte* palette);

void I_UpdateNoBlit ();
void I_FinishUpdate ();

// Wait for vertical retrace or pause a bit.
void I_WaitVBL(const ID_TIME_T count);

void I_ReadScreen (byte* scr);

void I_BeginRead ();
void I_EndRead ();

void I_InitInput ();

void I_ShutdownInput() ;
void I_InputFrame();

void I_UpdateControllerState();
struct controller_t;
size_t I_PollMouseInputEvents() ;
bool I_ReturnMouseInputEvent( const index_t n, event_t* e);
size_t I_PollJoystickInputEvents() ;
bool I_ReturnJoystickInputEvent( const index_t n, event_t* e);
void I_EndJoystickInputEvents( );

#endif

