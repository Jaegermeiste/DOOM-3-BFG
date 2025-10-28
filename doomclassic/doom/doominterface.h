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

#ifndef DOOM_INTERFACE_H
#define DOOM_INTERFACE_H

#pragma once

//#include "doomlib.h"

#include <vector>
#include <string>

class idUserCmdMgr;

class DoomInterface
{
public:
			DoomInterface();
	virtual ~DoomInterface();

	typedef int ( *NoParamCallback)();
	
	void Startup( size_t players, bool multiplayer = false );
	bool Frame( ID_TIME_T time, idUserCmdMgr * userCmdMgr );
	void Shutdown();
	void QuitCurrentGame();
	void EndDMGame();

	// PS3
	//void InitGraphics( int player = -1, int width = TEXTUREWIDTH, int height = TEXTUREHEIGHT, D3DCOLOR *pBuffer = NULL, D3DCOLOR *pBuffer2 = NULL );
	void SetPostGlobalsCallback( NoParamCallback cb );
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	void SetNetworking( DoomLib::RecvFunc recv, DoomLib::SendFunc send, DoomLib::SendRemoteFunc sendRemote );
#endif
	size_t GetNumPlayers() const;

	static index_t CurrentPlayer();

			static void	SetMultiplayerPlayers(index_t localPlayerIndex, size_t playerCount, index_t localPlayer, idList<idStr> playerAddresses );

protected:
	size_t				numplayers;

	bool				bFinished[4];

	ID_TIME_T			lastTicRun;
};


 #endif
