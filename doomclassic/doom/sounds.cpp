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


#include "doomtype.h"
#include "sounds.h"

//
// Information about all the music
//



//
// Information about all the sfx
//

sfxinfo_t S_sfx[128] =
{
  // S_sfx[0] needs to be a dummy for odd reasons.
  { "none", false,  0, nullptr, -1, -1, nullptr },

  { "pistol", false, 64, nullptr, -1, -1, nullptr },
  { "shotgn", false, 64, nullptr, -1, -1, nullptr },
  { "sgcock", false, 64, nullptr, -1, -1, nullptr },
  { "dshtgn", false, 64, nullptr, -1, -1, nullptr },
  { "dbopn", false, 64, nullptr, -1, -1, nullptr },
  { "dbcls", false, 64, nullptr, -1, -1, nullptr },
  { "dbload", false, 64, nullptr, -1, -1, nullptr },
  { "plasma", false, 64, nullptr, -1, -1, nullptr },
  { "bfg", false, 64, nullptr, -1, -1, nullptr },
  { "sawup", false, 64, nullptr, -1, -1, nullptr },
  { "sawidl", false, 118, nullptr, -1, -1, nullptr },
  { "sawful", false, 64, nullptr, -1, -1, nullptr },
  { "sawhit", false, 64, nullptr, -1, -1, nullptr },
  { "rlaunc", false, 64, nullptr, -1, -1, nullptr },
  { "rxplod", false, 70, nullptr, -1, -1, nullptr },
  { "firsht", false, 70, nullptr, -1, -1, nullptr },
  { "firxpl", false, 70, nullptr, -1, -1, nullptr },
  { "pstart", false, 100, nullptr, -1, -1, nullptr },
  { "pstop", false, 100, nullptr, -1, -1, nullptr },
  { "doropn", false, 100, nullptr, -1, -1, nullptr },
  { "dorcls", false, 100, nullptr, -1, -1, nullptr },
  { "stnmov", false, 119, nullptr, -1, -1, nullptr },
  { "swtchn", false, 78, nullptr, -1, -1, nullptr },
  { "swtchx", false, 78, nullptr, -1, -1, nullptr },
  { "plpain", false, 96, nullptr, -1, -1, nullptr },
  { "dmpain", false, 96, nullptr, -1, -1, nullptr },
  { "popain", false, 96, nullptr, -1, -1, nullptr },
  { "vipain", false, 96, nullptr, -1, -1, nullptr },
  { "mnpain", false, 96, nullptr, -1, -1, nullptr },
  { "pepain", false, 96, nullptr, -1, -1, nullptr },
  { "slop", false, 78, nullptr, -1, -1, nullptr },
  { "itemup", true, 78, nullptr, -1, -1, nullptr },
  { "wpnup", true, 78, nullptr, -1, -1, nullptr },
  { "oof", false, 96, nullptr, -1, -1, nullptr },
  { "telept", false, 32, nullptr, -1, -1, nullptr },
  { "posit1", true, 98, nullptr, -1, -1, nullptr },
  { "posit2", true, 98, nullptr, -1, -1, nullptr },
  { "posit3", true, 98, nullptr, -1, -1, nullptr },
  { "bgsit1", true, 98, nullptr, -1, -1, nullptr },
  { "bgsit2", true, 98, nullptr, -1, -1, nullptr },
  { "sgtsit", true, 98, nullptr, -1, -1, nullptr },
  { "cacsit", true, 98, nullptr, -1, -1, nullptr },
  { "brssit", true, 94, nullptr, -1, -1, nullptr },
  { "cybsit", true, 92, nullptr, -1, -1, nullptr },
  { "spisit", true, 90, nullptr, -1, -1, nullptr },
  { "bspsit", true, 90, nullptr, -1, -1, nullptr },
  { "kntsit", true, 90, nullptr, -1, -1, nullptr },
  { "vilsit", true, 90, nullptr, -1, -1, nullptr },
  { "mansit", true, 90, nullptr, -1, -1, nullptr },
  { "pesit", true, 90, nullptr, -1, -1, nullptr },
  { "sklatk", false, 70, nullptr, -1, -1, nullptr },
  { "sgtatk", false, 70, nullptr, -1, -1, nullptr },
  { "skepch", false, 70, nullptr, -1, -1, nullptr },
  { "vilatk", false, 70, nullptr, -1, -1, nullptr },
  { "claw", false, 70, nullptr, -1, -1, nullptr },
  { "skeswg", false, 70, nullptr, -1, -1, nullptr },
  { "pldeth", false, 32, nullptr, -1, -1, nullptr },
  { "pdiehi", false, 32, nullptr, -1, -1, nullptr },
  { "podth1", false, 70, nullptr, -1, -1, nullptr },
  { "podth2", false, 70, nullptr, -1, -1, nullptr },
  { "podth3", false, 70, nullptr, -1, -1, nullptr },
  { "bgdth1", false, 70, nullptr, -1, -1, nullptr },
  { "bgdth2", false, 70, nullptr, -1, -1, nullptr },
  { "sgtdth", false, 70, nullptr, -1, -1, nullptr },
  { "cacdth", false, 70, nullptr, -1, -1, nullptr },
  { "skldth", false, 70, nullptr, -1, -1, nullptr },
  { "brsdth", false, 32, nullptr, -1, -1, nullptr },
  { "cybdth", false, 32, nullptr, -1, -1, nullptr },
  { "spidth", false, 32, nullptr, -1, -1, nullptr },
  { "bspdth", false, 32, nullptr, -1, -1, nullptr },
  { "vildth", false, 32, nullptr, -1, -1, nullptr },
  { "kntdth", false, 32, nullptr, -1, -1, nullptr },
  { "pedth", false, 32, nullptr, -1, -1, nullptr },
  { "skedth", false, 32, nullptr, -1, -1, nullptr },
  { "posact", true, 120, nullptr, -1, -1, nullptr },
  { "bgact", true, 120, nullptr, -1, -1, nullptr },
  { "dmact", true, 120, nullptr, -1, -1, nullptr },
  { "bspact", true, 100, nullptr, -1, -1, nullptr },
  { "bspwlk", true, 100, nullptr, -1, -1, nullptr },
  { "vilact", true, 100, nullptr, -1, -1, nullptr },
  { "noway", false, 78, nullptr, -1, -1, nullptr },
  { "barexp", false, 60, nullptr, -1, -1, nullptr },
  { "punch", false, 64, nullptr, -1, -1, nullptr },
  { "hoof", false, 70, nullptr, -1, -1, nullptr },
  { "metal", false, 70, nullptr, -1, -1, nullptr },
  { "chgun", false, 64, &S_sfx[sfx_pistol], 150, 0, nullptr },
  { "tink", false, 60, nullptr, -1, -1, nullptr },
  { "bdopn", false, 100, nullptr, -1, -1, nullptr },
  { "bdcls", false, 100, nullptr, -1, -1, nullptr },
  { "itmbk", false, 100, nullptr, -1, -1, nullptr },
  { "flame", false, 32, nullptr, -1, -1, nullptr },
  { "flamst", false, 32, nullptr, -1, -1, nullptr },
  { "getpow", false, 60, nullptr, -1, -1, nullptr },
  { "bospit", false, 70, nullptr, -1, -1, nullptr },
  { "boscub", false, 70, nullptr, -1, -1, nullptr },
  { "bossit", false, 70, nullptr, -1, -1, nullptr },
  { "bospn", false, 70, nullptr, -1, -1, nullptr },
  { "bosdth", false, 70, nullptr, -1, -1, nullptr },
  { "manatk", false, 70, nullptr, -1, -1, nullptr },
  { "mandth", false, 70, nullptr, -1, -1, nullptr },
  { "sssit", false, 70, nullptr, -1, -1, nullptr },
  { "ssdth", false, 70, nullptr, -1, -1, nullptr },
  { "keenpn", false, 70, nullptr, -1, -1, nullptr },
  { "keendt", false, 70, nullptr, -1, -1, nullptr },
  { "skeact", false, 70, nullptr, -1, -1, nullptr },
  { "skesit", false, 70, nullptr, -1, -1, nullptr },
  { "skeatk", false, 70, nullptr, -1, -1, nullptr },
  { "radio", false, 60, nullptr, -1, -1, nullptr } 
};


