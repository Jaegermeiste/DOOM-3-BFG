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

#ifndef __D_NET__
#define __D_NET__

#pragma once

#include "d_player.h"


#ifdef __GNUG__
#pragma interface
#endif


//
// Network play related stuff.
// There is a data struct that stores network
//  communication related stuff, and another
//  one that defines the actual packets to
//  be transmitted.
//

constexpr uint32 DOOMCOM_ID = 0x12345678l;

// Max computers/players in a game.
constexpr size_t MAXNETNODES = 16;          // https://doomwiki.org/wiki/Static_limits


// Networking and tick handling related.
constexpr size_t BACKUPTICS = 64;

typedef enum command_e : uint8
{
    CMD_SEND	= 1,
    CMD_GET	= 2

} command_t;


//
// Network packet data.
//
typedef struct doomdata_s
{
    // High bit is retransmit request.
    uint32		checksum;
    // Only valid if NCMD_RETRANSMIT.
    ID_TIME_T	retransmitfrom;
	
	byte		sourceDest;
    
    ID_TIME_T	starttic;
    index_t		player;
    size_t		numtics;
    ticcmd_t	cmds[BACKUPTICS];

} doomdata_t;




struct doomcom_t
{
    // Supposed to be DOOMCOM_ID?
    uint32		id;
    
    // DOOM executes an int to execute commands.
    short		intnum;		
    // Communication between DOOM and the driver.
    // Is CMD_SEND or CMD_GET.
	command_t	command;
    // Is dest for send, set by get (-1 = no packet).
    index_t		remotenode;
    
    // Number of bytes in doomdata to be sent
    size_t		datalength;

    // Info common to all nodes.
    // Console is allways node 0.
	size_t		numnodes;
    // Flag: 1 = no duplication, 2-5 = dup for slow nets.
    short		ticdup;
    // Flag: 1 = send a backup tic in every packet.
    bool		extratics;
    // Flag: 1 = deathmatch.
    bool		deathmatch;
    // Flag: -1 = new game, 0-5 = load savegame
    index_t		savegame;
	index_t		episode;	// 1-3
	index_t		map;		// 1-9
    short		skill;		// 1-5

    // Info specific to this node.
    index_t		consoleplayer;
	size_t		numplayers;
    
    // These are related to the 3-display mode,
    //  in which two drones looking left and right
    //  were used to render two additional views
    //  on two additional computers.
    // Probably not operational anymore.
    // 1 = left, 0 = center, -1 = right
    short		angleoffset;
    // 1 = drone
    short		drone;		

    // The packet data to be sent.
    doomdata_t		data;
    
} ;

struct netNode_s
{
	ID_TIME_T   nettics;
	bool		nodeingame;		// set false as nodes leave game
	bool		remoteresend;	// set when local needs tics
	ID_TIME_T	resendto;		// set when remote needs tics
	size_t		resendcount;
};

class idUserCmdMgr;

// Create any new ticcmds and broadcast to other players.
void NetUpdate ( idUserCmdMgr * userCmdMgr );

// Broadcasts special packets to other players
//  to notify of game exit
void D_QuitNetGame ();

//? how many ticks to run?
bool TryRunTics ();


#endif



