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

#include <algorithm>
#include <utility>

#include "Precompiled.h"
#include "globaldata.h"


#include "m_menu.h"
#include "i_system.h"
#include "i_video.h"
#include "i_net.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

#include "doomlib.h"
#include "Main.h"
#include "d3xp/Game_local.h"


void I_GetEvents( controller_t * );
void D_ProcessEvents (); 
void G_BuildTiccmd (ticcmd_t *cmd, idUserCmdMgr *, ID_TIME_T newTics ); 
void D_DoAdvanceDemo ();

extern bool globalNetworking;

//
// NETWORKING
//
// ::g->gametic is the tic about to (or currently being) run
// ::g->maketic is the tick that hasn't had control made for it yet
// ::g->nettics[] has the maketics for all ::g->players 
//
// a ::g->gametic cannot be run until ::g->nettics[] > ::g->gametic for all ::g->players
//




constexpr ID_TIME_T NET_TIMEOUT = (1 * TICRATE);





//
//
//
static size_t NetbufferSize ()
{
	constexpr size_t size = sizeof((static_cast<doomdata_t*>(nullptr)->cmds[::g->netbuffer->numtics]));

	return size;
}

//
// Checksum 
//
static uint32 NetbufferChecksum ()
{
	uint32 c = 0x1234567;

	if ( globalNetworking ) {
		const size_t l = (NetbufferSize() - sizeof((static_cast<doomdata_t*>(nullptr)->retransmitfrom)) / 4);
		for (size_t i = 0 ; i<l ; ++i)
		{
			c += reinterpret_cast<uint32*>(&::g->netbuffer->retransmitfrom)[i] * (i+1);
		}
	}

	return c & NCMD_CHECKSUM;
}

//
//
//
static ID_TIME_T ExpandTics (const ID_TIME_T low)
{
	const auto delta = low - (::g->maketic & 0xff);

	if (delta >= -64 && delta <= 64)
	{
		return (::g->maketic&~0xff) + low;
	}
	if (delta > 64)
	{
		return (::g->maketic&~0xff) - 256 + low;
	}
	if (delta < -64)
	{
		return (::g->maketic&~0xff) + 256 + low;
	}

	I_Error ("ExpandTics: strange value '%i' at ::g->maketic = '%i'", low, ::g->maketic);

	return 0;
}



//
// HSendPacket
//
static void HSendPacket (const Ordinal auto	node, const int	flags )
{
	::g->netbuffer->checksum = NetbufferChecksum () | flags;

	if (!node)
	{
		::g->reboundstore = *::g->netbuffer;
		::g->reboundpacket = true;
		return;
	}

	if (::g->demoplayback)
	{
		return;
	}

	if (!::g->netgame)
	{
		I_Error ("Tried to transmit to another node");
	}

	::g->doomcom.command = CMD_SEND;
	::g->doomcom.remotenode = node;
	::g->doomcom.datalength = NetbufferSize ();

	if (::g->debugfile)
	{
		ID_TIME_T		realretrans = 0;
		if (::g->netbuffer->checksum & NCMD_RETRANSMIT)
		{
			realretrans = ExpandTics (::g->netbuffer->retransmitfrom);
		}
		else
		{
			realretrans = -1;
		}

		std::ignore = fprintf (::g->debugfile,"send (%lld + %llu, R %lld) [%llu] ",
		         ExpandTics(::g->netbuffer->starttic),
		         ::g->netbuffer->numtics, realretrans, ::g->doomcom.datalength);

		for (size_t i = 0; i < ::g->doomcom.datalength; ++i)
		{
			std::ignore = fprintf(::g->debugfile,"%i ", reinterpret_cast<byte*>(::g->netbuffer)[i]);
		}

		std::ignore = fprintf (::g->debugfile,"\n");
	}

	I_NetCmd ();
}

//
// HGetPacket
// Returns false if no packet is waiting
//
static bool HGetPacket ()
{	
	if (::g->reboundpacket)
	{
		*::g->netbuffer = ::g->reboundstore;
		::g->doomcom.remotenode = 0;
		::g->reboundpacket = false;
		return true;
	}

	if (!::g->netgame)
	{
		return false;
	}

	if (::g->demoplayback)
	{
		return false;
	}

	::g->doomcom.command = CMD_GET;
	I_NetCmd ();

	if (::g->doomcom.remotenode == -1)
	{
		return false;
	}

	if (::g->doomcom.datalength != NetbufferSize ())
	{
		if (::g->debugfile)
		{
			std::ignore =  fprintf(::g->debugfile, "bad packet length '%llu'\n", ::g->doomcom.datalength);
		}
		return false;
	}

	// ALAN NETWORKING -- this fails a lot on 4 player split debug!!
	// TODO: Networking
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	if ( !gameLocal->IsSplitscreen() && NetbufferChecksum() != (::g->netbuffer->checksum&NCMD_CHECKSUM) )
	{
		if (::g->debugfile) {
			fprintf (::g->debugfile,"bad packet checksum\n");
		}

		return false;
	}
#endif

	if (::g->debugfile)
	{
		if (::g->netbuffer->checksum & NCMD_SETUP)
		{
			std::ignore = fprintf (::g->debugfile,"setup packet\n");
		}
		else
		{
			ID_TIME_T realretrans = 0;

			if (::g->netbuffer->checksum & NCMD_RETRANSMIT)
			{
				realretrans = ExpandTics (::g->netbuffer->retransmitfrom);
			}
			else
			{
				realretrans = -1;
			}

			std::ignore = fprintf (::g->debugfile,"get %lld = (%lld + %llu, R %lld)[%llu] ",
			         ::g->doomcom.remotenode,
			         ExpandTics(::g->netbuffer->starttic),
			         ::g->netbuffer->numtics, realretrans, ::g->doomcom.datalength);

			for (size_t i = 0 ; i < ::g->doomcom.datalength ; ++i)
			{
				std::ignore = fprintf (::g->debugfile,"%i ",reinterpret_cast<byte*>(::g->netbuffer)[i]);
			}
			std::ignore = fprintf (::g->debugfile,"\n");
		}
	}
	return true;	
}


//
// GetPackets
//

static void GetPackets ()
{
	while ( HGetPacket() )
	{
		if (::g->netbuffer->checksum & NCMD_SETUP)
		{
			continue; // extra setup packet
		}

		index_t netconsole = ::g->netbuffer->player & ~PL_DRONE;
		index_t netnode = ::g->doomcom.remotenode;

		// to save bytes, only the low byte of tic numbers are sent
		// Figure out what the rest of the bytes are
		ID_TIME_T realstart = ExpandTics(::g->netbuffer->starttic);		
		ID_TIME_T realend = (realstart + numeric_cast<ID_TIME_T>(::g->netbuffer->numtics));

		// check for exiting the game
		if (::g->netbuffer->checksum & NCMD_EXIT)
		{
			if (!::g->netNodes[netnode].nodeingame)
			{
				continue;
			}
			::g->netNodes[netnode].nodeingame = false;
			::g->players[netconsole].playerInGame = false;
			strncpy_s (::g->exitmsg, PLAYER_LEFT_GAME, strlen(PLAYER_LEFT_GAME));
			::g->exitmsg[7] = numeric_cast<BASE_TYPE(::g->exitmsg)>(::g->exitmsg[7] + netconsole);
			::g->players[::g->consoleplayer].message = ::g->exitmsg;

			if( ::g->demorecording ) {
				G_CheckDemoStatus();
			}
			continue;
		}

		// check for a remote game kill
/*
		if (::g->netbuffer->checksum & NCMD_KILL)
			I_Error ("Killed by network driver");
*/

		::g->players[netconsole].node = netnode;

		// check for retransmit request
		if ( ::g->netNodes[netnode].resendcount <= 0 
			&& (::g->netbuffer->checksum & NCMD_RETRANSMIT) )
		{
			::g->netNodes[netnode].resendto = ExpandTics(::g->netbuffer->retransmitfrom);
			if (::g->debugfile)
			{
				std::ignore = fprintf (::g->debugfile,"retransmit from %lld\n", ::g->netNodes[netnode].resendto);
			}
			::g->netNodes[netnode].resendcount = RESENDCOUNT;
		}
		else
		{
			--::g->netNodes[netnode].resendcount;
		}

		// check for out of order / duplicated packet		
		if (realend == ::g->netNodes[netnode].nettics)
		{
			continue;
		}

		if (realend < ::g->netNodes[netnode].nettics)
		{
			if (::g->debugfile)
			{
				std::ignore = fprintf (::g->debugfile,
				         "out of order packet (%lld + %llu)\n" ,
				         realstart,::g->netbuffer->numtics);
			}
			continue;
		}

		// check for a missed packet
		if (realstart > ::g->netNodes[netnode].nettics)
		{
			// stop processing until the other system resends the missed tics
			if (::g->debugfile)
			{
				std::ignore = fprintf (::g->debugfile,
				         "missed tics from %lld (%lld - %lld)\n",
				         netnode, realstart, ::g->netNodes[netnode].nettics);
			}
			::g->netNodes[netnode].remoteresend = true;
			continue;
		}

		// update command store from the packet
		{
			::g->netNodes[netnode].remoteresend = false;

			ID_TIME_T start = ::g->netNodes[netnode].nettics - realstart;		
			ticcmd_t* src = &::g->netbuffer->cmds[start];

			while (::g->netNodes[netnode].nettics < realend)
			{
				ticcmd_t* dest = &::g->netcmds[netconsole][::g->netNodes[netnode].nettics % BACKUPTICS];
				++::g->netNodes[netnode].nettics;
				*dest = *src;
				src++;
			}
		}
	}
}


//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//

void NetUpdate ( idUserCmdMgr * userCmdMgr )
{
	bool update = true;

	// check time
	ID_TIME_T nowtime = I_GetTime() / ::g->ticdup;
	ID_TIME_T newtics = nowtime - ::g->gametime;
	::g->gametime = nowtime;

	if (newtics <= 0) 	// nothing new to update
	{
		update = false;
	}

	if (update)
	{
		size_t i = 0;

		if (::g->skiptics <= newtics)
		{
			newtics -= ::g->skiptics;
			::g->skiptics = 0;
		}
		else
		{
			::g->skiptics -= newtics;
			newtics = 0;
		}

		::g->netbuffer->player = ::g->consoleplayer;

		// build new ticcmds for console player
		ID_TIME_T gameticdiv = ::g->gametic / ::g->ticdup;
		for (i = 0; std::cmp_less(i, newtics); ++i)
		{
			//I_GetEvents( ::g->I_StartTicCallback () );
			D_ProcessEvents();
			if (std::cmp_greater_equal(::g->maketic - gameticdiv, BACKUPTICS / 2 - 1)) {
				printf("Out of room for ticcmds: maketic = %lld, gameticdiv = %lld\n", ::g->maketic, gameticdiv);
				break;          // can't hold any more
			}

			//I_Printf ("mk:%i ",::g->maketic);

			// Grab the latest tech5 command

			G_BuildTiccmd(&::g->localcmds[::g->maketic % BACKUPTICS], userCmdMgr, newtics);
			::g->maketic++;
		}


		if (::g->singletics)
		{
			return; // single tic update is synchronous
		}

		// send the packet to the other ::g->nodes
		for (i = 0; i < ::g->doomcom.numnodes; ++i) {
			ID_TIME_T realstart = 0;

			if (::g->netNodes[i].nodeingame) {
				::g->netbuffer->starttic = realstart = ::g->netNodes[i].resendto;
				::g->netbuffer->numtics = ::g->maketic - realstart;
				if (::g->netbuffer->numtics > BACKUPTICS)
				{
					I_Error("NetUpdate: ::g->netbuffer->numtics > BACKUPTICS");
				}

				::g->netNodes[i].resendto = ::g->maketic - ::g->doomcom.extratics;

				for (size_t j = 0; j < ::g->netbuffer->numtics; j++)
				{
					::g->netbuffer->cmds[j] = ::g->localcmds[(realstart + j) % BACKUPTICS];
				}

				if (::g->netNodes[i].remoteresend)
				{
					::g->netbuffer->retransmitfrom = ::g->netNodes[i].nettics;
					HSendPacket(i, NCMD_RETRANSMIT);
				}
				else
				{
					::g->netbuffer->retransmitfrom = 0;
					HSendPacket(i, 0);
				}
			}
		}
	}

	// listen for other packets
	GetPackets ();
}



//
// CheckAbort
//
static void CheckAbort ()
{
	// DHM - Time starts at 0 tics when starting a multiplayer game, so we can
	// check for timeouts easily.  If we're still waiting after N seconds, abort.
	if ( I_GetTime() > NET_TIMEOUT ) {
		// TODO: Show error & leave net game.
		I_Warning( "NET GAME TIMED OUT!\n" );
		//gameLocal->showFatalErrorMessage( XuiLookupStringTable(globalStrings,L"Timed out waiting for match start.") );


		D_QuitNetGame();

		session->QuitMatch();
		common->Dialog().AddDialog( GDM_OPPONENT_CONNECTION_LOST, DIALOG_ACCEPT, nullptr, nullptr, false );
	}
}


//
// D_ArbitrateNetStart
//
static bool D_ArbitrateNetStart ()
{
	::g->autostart = true;
	if (::g->doomcom.consoleplayer)
	{
		// listen for setup info from key player
		CheckAbort ();
		if (!HGetPacket ())
		{
			return false;
		}
		if (::g->netbuffer->checksum & NCMD_SETUP)
		{
			printf( "Received setup info\n" );

			if (::g->netbuffer->player != VERSION)
			{
				I_Error ("Different DOOM versions cannot play a net game!");
			}
			::g->startskill = static_cast<skill_t>(::g->netbuffer->retransmitfrom & 15);
			::g->deathmatch = (::g->netbuffer->retransmitfrom & 0xc0) >> 6;
			::g->nomonsters = (::g->netbuffer->retransmitfrom & 0x20) > 0;
			::g->respawnparm = (::g->netbuffer->retransmitfrom & 0x10) > 0;
			// VV original xbox doom :: don't do this.. it will be setup from the launcher
			//::g->startmap = ::g->netbuffer->starttic & 0x3f;
			//::g->startepisode = ::g->netbuffer->starttic >> 6;
			return true;
		}
		return false;
	}
	else
	{
		size_t i = 0;
		// key player, send the setup info
		CheckAbort ();
		for (i = 0; std::cmp_less(i, ::g->doomcom.numnodes); ++i)
		{
			printf( "Sending setup info to node %llu\n", i );

			::g->netbuffer->retransmitfrom = ::g->startskill;
			if (::g->deathmatch)
			{
				::g->netbuffer->retransmitfrom |= (::g->deathmatch<<6);
			}
			if (::g->nomonsters)
			{
				::g->netbuffer->retransmitfrom |= 0x20;
			}
			if (::g->respawnparm)
			{
				::g->netbuffer->retransmitfrom |= 0x10;
			}
			::g->netbuffer->starttic = ::g->startepisode * 64 + ::g->startmap;
			::g->netbuffer->player = VERSION;
			::g->netbuffer->numtics = 0;
			HSendPacket (i, NCMD_SETUP);
		}

		while (HGetPacket ())
		{
			::g->gotinfo[::g->netbuffer->player&0x7f] = true;
		}

		for (i = 1; std::cmp_less(i, ::g->doomcom.numnodes); ++i) {
			if (!::g->gotinfo[i])
			{
				break;
			}
		}

		if (std::cmp_greater_equal(i, ::g->doomcom.numnodes))
		{
			return true;
		}

		return false;
	}
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//

static void D_CheckNetGame ()
{
	for (size_t i = 0; std::cmp_less(i, MAXNETNODES); ++i)
	{
		::g->netNodes[i].nodeingame = false;
		::g->netNodes[i].nettics = 0;
		::g->netNodes[i].remoteresend = false;	// set when local needs tics
		::g->netNodes[i].resendto = 0;		// which tic to start sending
	}

	// I_InitNetwork sets ::g->doomcom and ::g->netgame
	I_InitNetwork ();
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	if (::g->doomcom.id != DOOMCOM_ID)
	{
		I_Error ("Doomcom buffer invalid!");
	}
#endif

	::g->netbuffer = &::g->doomcom.data;
	::g->consoleplayer = ::g->displayplayer = ::g->doomcom.consoleplayer;
}

static bool D_PollNetworkStart()
{
	size_t             i = 0;
	if (::g->netgame)
	{
		if (D_ArbitrateNetStart () == false)
		{
			return false;
		}
	}

	I_Printf ("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i\n",
		::g->startskill, ::g->deathmatch, ::g->startmap, ::g->startepisode);

	// read values out of ::g->doomcom
	::g->ticdup = ::g->doomcom.ticdup;
	::g->maxsend = BACKUPTICS / (2ULL * ::g->ticdup) - 1;
	::g->maxsend = Max(::g->maxsend, 1);

	for (i = 0; std::cmp_less(i, ::g->doomcom.numplayers); ++i)
	{
		::g->players[i].playerInGame = true;
	}
	for (i = 0; std::cmp_less(i, ::g->doomcom.numnodes); ++i)
	{
		::g->netNodes[i].nodeingame = true;
	}

	I_Printf ("player %i of %i (%i ::g->nodes)\n",
	          ::g->consoleplayer + 1, ::g->doomcom.numplayers, ::g->doomcom.numnodes);

	return true;
}


//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other ::g->players
//
void D_QuitNetGame ()
{
	size_t i = 0;

	if ( (!::g->netgame && !::g->usergame) || ::g->consoleplayer == -1 || ::g->demoplayback || ::g->netbuffer == nullptr)
	{
		return;
	}

	// send a quit packet to the other nodes
	::g->netbuffer->player = ::g->consoleplayer;
	::g->netbuffer->numtics = 0;

	for (i = 1; std::cmp_less(i, ::g->doomcom.numnodes); ++i ) {
		if ( ::g->netNodes[i].nodeingame ) {
			HSendPacket( i, NCMD_EXIT );
		}
	}
	DoomLib::SendNetwork();

	for (i = 1; std::cmp_less(i, MAXNETNODES); ++i)
	{
		::g->netNodes[i].nodeingame = false;
		::g->netNodes[i].nettics = 0;
		::g->netNodes[i].remoteresend = false;	// set when local needs tics
		::g->netNodes[i].resendto = 0;		// which tic to start sending
	}

	//memset (&::g->doomcom, 0, sizeof(::g->doomcom) );

	// Reset singleplayer state
	::g->doomcom.id = DOOMCOM_ID;
	::g->doomcom.ticdup = 1;
	::g->doomcom.extratics = false;
	::g->doomcom.numplayers = ::g->doomcom.numnodes = 1;
	::g->doomcom.deathmatch = false;
	::g->doomcom.consoleplayer = 0;
	::g->netgame = false;

	::g->netbuffer = &::g->doomcom.data;
	::g->consoleplayer = ::g->displayplayer = ::g->doomcom.consoleplayer;

	::g->ticdup = ::g->doomcom.ticdup;
	::g->maxsend = BACKUPTICS / ( 2ULL * ::g->ticdup ) - 1;
	::g->maxsend = Max(::g->maxsend, 1);

	for (i = 0; std::cmp_less(i, ::g->doomcom.numplayers); ++i)
	{
		::g->players[i].playerInGame = true;
	}
	for (i = 0; std::cmp_less(i, ::g->doomcom.numnodes); ++i)
	{
		::g->netNodes[i].nodeingame = true;
	}
}



//
// TryRunTics
//
static bool TryRunTics ( idUserCmdMgr * userCmdMgr )
{
	size_t		i = 0;
	index_t		lowtic_node = -1;

	// get real tics		
	::g->trt_entertic = I_GetTime() / ::g->ticdup;
	::g->trt_realtics = ::g->trt_entertic - ::g->oldtrt_entertics;
	::g->oldtrt_entertics = ::g->trt_entertic;

	// get available tics
	NetUpdate ( userCmdMgr );

	::g->trt_lowtic = std::numeric_limits<BASE_TYPE(::g->trt_lowtic)>::max();
	::g->trt_numplaying = 0;

	for (i = 0; std::cmp_less(i, ::g->doomcom.numnodes); ++i) {

		if (::g->netNodes[i].nodeingame) {
			::g->trt_numplaying++;

			if (::g->netNodes[i].nettics < ::g->trt_lowtic) {
				::g->trt_lowtic = ::g->netNodes[i].nettics;
				lowtic_node = numeric_cast<BASE_TYPE(lowtic_node)>(i);
			}
		}
	}

	::g->trt_availabletics = ::g->trt_lowtic - ::g->gametic/::g->ticdup;

	// decide how many tics to run
	if (::g->trt_realtics < ::g->trt_availabletics-1) {
		::g->trt_counts = ::g->trt_realtics+1;
	} else if (::g->trt_realtics < ::g->trt_availabletics) {
		::g->trt_counts = ::g->trt_realtics;
	} else {
		::g->trt_counts = ::g->trt_availabletics;
	}

	::g->trt_counts = Max(::g->trt_counts, 1);

	::g->frameon++;

	if (::g->debugfile) {
		std::ignore = fprintf (::g->debugfile, "=======real: %lld  avail: %lld  game: %llu\n", ::g->trt_realtics, ::g->trt_availabletics, ::g->trt_counts);
	}

	if ( !::g->demoplayback )
	{	
		// ideally ::g->nettics[0] should be 1 - 3 tics above ::g->trt_lowtic
		// if we are consistently slower, speed up time
		for (i = 0; std::cmp_less(i, ::g->players.Num()); ++i) {
			if (::g->players[i].playerInGame) {
				break;
			}
		}

		if (std::cmp_equal(::g->consoleplayer, i)) {
			// the key player does not adapt
		}
		else {
			if (::g->netNodes[0].nettics <= ::g->netNodes[::g->players[i].node].nettics)	{
				--::g->gametime;
				//OutputDebugString("-");
			}

			::g->frameskip[::g->frameon&3] = (::g->oldnettics > ::g->netNodes[::g->players[i].node].nettics);
			::g->oldnettics = ::g->netNodes[0].nettics;

			if (::g->frameskip[0] && ::g->frameskip[1] && ::g->frameskip[2] && ::g->frameskip[3]) {
				::g->skiptics = 1;
				//OutputDebugString("+");
			}
		}
	}

	// wait for new tics if needed
	if (std::cmp_less(::g->trt_lowtic, ::g->gametic/::g->ticdup + ::g->trt_counts)	)
	{
		if (::g->trt_lowtic < ::g->gametic/::g->ticdup) {
			I_Error ("TryRunTics: ::g->trt_lowtic < gametic");
		}

		if ( ::g->lastnettic == 0 ) {
			::g->lastnettic = ::g->trt_entertic;
		}
		ID_TIME_T lagtime = ::g->trt_entertic - ::g->lastnettic;

		// Detect if a client has stopped sending updates, remove them from the game after 5 secs.
		if ( common->IsMultiplayer() && (!::g->demoplayback && ::g->netgame) && lagtime >= TICRATE ) {

			if ( lagtime > NET_TIMEOUT ) {

				if ( lowtic_node == ::g->players[::g->consoleplayer].node ) {

#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
#ifndef __PS3__
					gameLocal->showFatalErrorMessage( XuiLookupStringTable(globalStrings,NET_PLAYER_DISCONNECTED) );
					gameLocal->Interface.QuitCurrentGame();
#endif
#endif
				} else {
					if (::g->netNodes[lowtic_node].nodeingame) {
						index_t consoleNum = lowtic_node;

						for (size_t i = 0; std::cmp_less(i, ::g->doomcom.numnodes); ++i ) {
							if ( ::g->players[i].node == lowtic_node ) {
								consoleNum = numeric_cast<index_t>(i);
								break;
							}
						}

						::g->netNodes[lowtic_node].nodeingame = false;
						::g->players[consoleNum].playerInGame = false;
						strncpy_s(::g->exitmsg, NET_PLAYER1_DISCONNECT, strlen(NET_PLAYER1_DISCONNECT));
						::g->exitmsg[7] = numeric_cast<BASE_TYPE(::g->exitmsg)>(::g->exitmsg[7] + consoleNum);
						::g->players[::g->consoleplayer].message = ::g->exitmsg;

						// Stop a demo record now, as playback doesn't support losing players
						G_CheckDemoStatus();
					}
				}
			}
		} 

		return false;
	}

	::g->lastnettic = 0;

	// run the count * ::g->ticdup tics
	while (::g->trt_counts--)
	{
		for (i = 0; std::cmp_less(i, ::g->ticdup); ++i)
		{
			if (::g->gametic / ::g->ticdup > ::g->trt_lowtic) {
				I_Error ("gametic(%d) greater than trt_lowtic(%d), trt_counts(%d)", ::g->gametic, ::g->trt_lowtic, ::g->trt_counts );
				return false;
			}

			if (::g->advancedemo) {
				D_DoAdvanceDemo ();
			}

			M_Ticker ();
			G_Ticker ();
			::g->gametic++;

			// modify command for duplicated tics
			if (std::cmp_not_equal(i, ::g->ticdup-1))
			{
				size_t buf = (::g->gametic / ::g->ticdup) % BACKUPTICS; 
				for (size_t j = 0; std::cmp_less(j, MAXPLAYERS); ++j)
				{
					ticcmd_t* cmd = &::g->netcmds[j][buf];

					if (cmd->buttons & BT_SPECIAL)
					{
						cmd->buttons = 0;
					}
				}
			}
		}

		NetUpdate ( userCmdMgr );	// check for new console commands
	}

	return true;
}

