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

//#include <d3d8.h>			// SMF
//#include <xgraphics.h>	// SMF





void I_ShutdownGraphics()
{
  // VVTODO: Shutdown Graphics
}



//
// I_StartFrame
//
void I_StartFrame ()
{
    // er?
}

static void I_CombineMouseEvent(const event_t* in, event_t* out)
{
	if (fabs(numeric_cast<float>(in->data1)) > fabs(numeric_cast<float>(out->data1)))
	{
		out->data1 = in->data1;
	}
	if (fabs(numeric_cast<float>(in->data2)) > fabs(numeric_cast<float>(out->data2)))
	{
		out->data2 = in->data2;
	}
	if (fabs(numeric_cast<float>(in->data3)) > fabs(numeric_cast<float>(out->data3)))
	{
		out->data3 = in->data3;
	}
}

static void I_GetEvents( controller_t *controller )
{
	event_t e_mouse = {}, e_joystick = {};

	e_mouse.type = ev_mouse;
	e_mouse.data1 = e_mouse.data2 = e_mouse.data3 = 0;
	e_joystick.type = ev_joystick;
	e_joystick.data1 = e_joystick.data2 = e_joystick.data3 = 0;

	size_t numEvents = I_PollMouseInputEvents();
	if (numEvents) 
	{
		event_t e;
	
		// right thumb stick
		for (size_t i = 0; i < numEvents; ++i)
		{
			I_ReturnMouseInputEvent(numeric_cast<index_t>(i), &e);
			if (e.type == ev_mouse)
			{
				I_CombineMouseEvent(&e, &e_mouse);
			}
			else if (e.type == ev_joystick)
			{
				I_CombineMouseEvent(&e, &e_joystick);
			}
		}
	}

	numEvents = I_PollJoystickInputEvents();
	if (numEvents) 
	{
		event_t e = {};
		for (size_t i = 0; i < numEvents; ++i)
		{
			I_ReturnJoystickInputEvent(i, &e);
			if (e.type == ev_keydown || e.type == ev_keyup) {
				D_PostEvent(&e);
			} else if (e.type == ev_joystick) {
				I_CombineMouseEvent(&e, &e_joystick);
			} else if (e.type == ev_mouse) {
				I_CombineMouseEvent(&e, &e_mouse);
			}
		}
	}

	D_PostEvent(&e_mouse);
	D_PostEvent(&e_joystick);
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit ()
{
    // what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate ()
{
// DHM - These buffers are not used
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
	if (scr)
	{
		memcpy(scr, ::g->screens[0], SCREENWIDTH * SCREENHEIGHT);
	}
}

static inline uint32 I_PackColor(const unsigned int a, const unsigned int r, const unsigned int g, const unsigned int b ) {
	uint32 color = 0;

	color |= (r & 255) << 24;
	color |= (g & 255) << 16;
	color |= (b & 255) << 8;
	color |= (a & 255);

	return color;
}

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
	if (palette)
	{
		// set the X colormap entries
		for (uint32& i : ::g->XColorMap)
		{
			const auto r = gammatable[::g->usegamma][*palette++];
			const auto g = gammatable[::g->usegamma][*palette++];
			const auto b = gammatable[::g->usegamma][*palette++];
			i = I_PackColor(0xff, r, g, b);
		}
	}
}

void I_InitGraphics()
{
}

