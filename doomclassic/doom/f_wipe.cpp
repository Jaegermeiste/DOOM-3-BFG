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

#include "z_zone.h"
#include "i_video.h"
#include "i_system.h"
#include "v_video.h"
#include "m_random.h"
#include "doomdef.h"
#include "f_wipe.h"

#include <utility>

//
//                       SCREEN WIPE PACKAGE
//

// when zero, stop the wipe


static void
wipe_shittyColMajorXform ( short*	array,  const size_t width,  const size_t height )
{
	//dest = (short*) DoomLib::Z_Malloc(width*height*2, PU_STATIC, 0 );
	short* dest = new short[width * height];

    for(size_t y = 0; y < height; ++y)
    {
	    for(size_t x = 0; x < width; ++x)
	    {
		    dest[x * height + y] = array[y * width + x];
	    }
    }

    memcpy(array, dest, width*height*2);

    //Z_Free(dest);
	delete[] dest;
}


static void wipe_initMelt ( const size_t width, const size_t height )
{
	size_t i = 0;
	int r = 0;
    
    // copy start screen to main screen
    memcpy(::g->wipe_scr, ::g->wipe_scr_start, width*height);
    
    // makes this wipe faster (in theory)
    // to have stuff in column-major format
    wipe_shittyColMajorXform(reinterpret_cast<short*>(::g->wipe_scr_start), width / 2, height);
    wipe_shittyColMajorXform(reinterpret_cast<short*>(::g->wipe_scr_end), width / 2, height);
    
    // setup initial column positions
    // (::g->wipe_y<0 => not ready to scroll yet)
    ::g->wipe_y = static_cast<int32*>(DoomLib::Z_Malloc(width * sizeof(int), PU_STATIC, nullptr));

    ::g->wipe_y[0] = -(M_Random()%16);

	for (i = 1; std::cmp_less(i, width); ++i)
	{
		r = (M_Random()%3) - 1;

		::g->wipe_y[i] = ::g->wipe_y[i - 1] + r;

		if (::g->wipe_y[i] > 0)
		{
			::g->wipe_y[i] = 0;
		}
		else if (::g->wipe_y[i] == -16)
		{
			::g->wipe_y[i] = -15;
		}
	}
}

static bool wipe_doMelt( const size_t width, const size_t height, ID_TIME_T ticks ) {
	bool	done = true;

	const index_t width_index = numeric_cast<index_t>(width / 2);

	while (ticks--)
	{
		for (size_t i = 0; std::cmp_less(i, width_index); i++)
		{
			if (::g->wipe_y[i]<0) {

				::g->wipe_y[i]++;
				done = false;
			}
			else if (std::cmp_less(::g->wipe_y[i], height)) {
				size_t dy = (::g->wipe_y[i] < 16 * GLOBAL_IMAGE_SCALER) ? ::g->wipe_y[i] + 1 : 8 * GLOBAL_IMAGE_SCALER;

				if (::g->wipe_y[i]+dy >= height)
				{
					dy = height - ::g->wipe_y[i];
				}

				short* s = &reinterpret_cast<short*>(::g->wipe_scr_end)[i * height + ::g->wipe_y[i]];
				short* d = &reinterpret_cast<short*>(::g->wipe_scr)[::g->wipe_y[i] * width + i];

				size_t  j   = 0;
				index_t idx = 0;
				for (j = dy; j > 0; --j)
				{
					d[idx] = *(s++);
					idx += width_index;
				}

				::g->wipe_y[i] += numeric_cast<BASE_TYPE(::g->wipe_y)>(dy);

				s = &reinterpret_cast<short*>(::g->wipe_scr_start)[i * height];
				d = &reinterpret_cast<short*>(::g->wipe_scr)[::g->wipe_y[i] * width_index + i];

				idx = 0;
				for (j = height - ::g->wipe_y[i]; j > 0; --j)
				{
					d[idx] = *(s++);
					idx += width_index;
				}

				done = false;
			}
		}
	}

	return done;
}

static void wipe_exitMelt()
{
    Z_Free(::g->wipe_y);
	::g->wipe_y = nullptr;
}

static void wipe_StartScreen ()
{
    ::g->wipe_scr_start = ::g->screens[2];
    I_ReadScreen(::g->wipe_scr_start);
}

static void wipe_EndScreen (const int x,  const int y,  const size_t width,  const size_t height )
{
    ::g->wipe_scr_end = ::g->screens[3];
    I_ReadScreen(::g->wipe_scr_end);
    V_DrawBlock(x, y, 0, width, height, ::g->wipe_scr_start); // restore start scr.
}

static bool wipe_ScreenWipe ( const size_t width,  const size_t height, const ID_TIME_T ticks )
{
	bool rc = false;

	// initial stuff
	if (!::g->go)
	{
		::g->go = true;
		::g->wipe_scr = ::g->screens[0];

		wipe_initMelt(width, height);
	}

	// do a piece of wipe-in
	V_MarkRect(0, 0, width, height);

	rc = wipe_doMelt(width, height, ticks);

	// final stuff
	if (rc)
	{
		::g->go = false;
		wipe_exitMelt();
	}

	return !::g->go;
}

