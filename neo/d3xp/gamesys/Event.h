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
/*
sys_event.h

Event are used for scheduling tasks and for linking script commands.
*/
#ifndef __SYS_EVENT_H__
#define __SYS_EVENT_H__

#pragma once

constexpr size_t D_EVENT_MAXARGS     = 8;			// if changed, enable the CREATE_EVENT_CODE define in Event.cpp to generate switch statement for idClass::ProcessEventArgPtr.
												// running the game will then generate c:\doom\base\events.txt, the contents of which should be copied into the switch statement.

constexpr auto D_EVENT_VOID        = static_cast<char>(0);
constexpr auto D_EVENT_INTEGER     = 'd';
constexpr auto D_EVENT_FLOAT       = 'f';
constexpr auto D_EVENT_VECTOR      = 'v';
constexpr auto D_EVENT_STRING      = 's';
constexpr auto D_EVENT_ENTITY      = 'e';
constexpr auto D_EVENT_ENTITY_NULL = 'E';			// event can handle NULL entity pointers
constexpr auto D_EVENT_TRACE       = 't';

constexpr size_t MAX_EVENTS          = 4096;

class idClass;
class idTypeInfo;

class idEventDef {
private:
	const char					*name;
	const char					*formatspec;
	size_t      				formatspecIndex;
	char						returnType;
	size_t						numargs;
	size_t						argsize;
	size_t						argOffset[ D_EVENT_MAXARGS ];
	size_t						eventnum;
	const idEventDef *			next;

	static idEventDef *			eventDefList[MAX_EVENTS];
	static size_t				numEventDefs;

public:
								idEventDef( const char *command, const char *formatspec = nullptr, char returnType = 0 );
								
	const char					*GetName() const;
	const char					*GetArgFormat() const;
	unsigned int				GetFormatspecIndex() const;
	char						GetReturnType() const;
	size_t						GetEventNum() const;
	size_t						GetNumArgs() const;
	size_t						GetArgSize() const;
	size_t						GetArgOffset( index_t arg ) const;

	static size_t				NumEventCommands();
	static const idEventDef		*GetEventCommand( size_t eventnum );
	static const idEventDef		*FindEvent( const char *name );
};

class idSaveGame;
class idRestoreGame;

class idEvent {
private:
	const idEventDef			*eventdef;
	byte						*data;
	ID_TIME_T					time;
	idClass						*object;
	const idTypeInfo			*typeinfo;

	idLinkList<idEvent>			eventNode;

	static idDynamicBlockAlloc<byte, 16 * 1024, 256> eventDataAllocator;


public:
	static bool					initialized;

								~idEvent();

	static idEvent				*Alloc( const idEventDef *evdef, const size_t numargs, va_list args );
	static void					CopyArgs( const idEventDef *evdef, const size_t numargs, va_list args, address_t data[ D_EVENT_MAXARGS ]  );
	
	void						Free();
	void						Schedule( idClass *obj, const idTypeInfo *type, ID_TIME_T time );
	byte						*GetData() const;

	static void					CancelEvents( const idClass *obj, const idEventDef *evdef = nullptr);
	static void					ClearEventList();
	static void					ServiceEvents();
	static void					ServiceFastEvents();
	static void					Init();
	static void					Shutdown();

	// save games
	static void					Save( idSaveGame *savefile );					// archives object for save game file
	static void					Restore( idRestoreGame *savefile );				// unarchives object from save game file
	static void					SaveTrace( idSaveGame *savefile, const trace_t &trace );
	static void					RestoreTrace( idRestoreGame *savefile, trace_t &trace );
	
};

/*
================
idEvent::GetData
================
*/
ID_INLINE byte *idEvent::GetData() const
{
	return data;
}

/*
================
idEventDef::GetName
================
*/
ID_INLINE const char *idEventDef::GetName() const {
	return name;
}

/*
================
idEventDef::GetArgFormat
================
*/
ID_INLINE const char *idEventDef::GetArgFormat() const {
	return formatspec;
}

/*
================
idEventDef::GetFormatspecIndex
================
*/
ID_INLINE unsigned int idEventDef::GetFormatspecIndex() const {
	return formatspecIndex;
}

/*
================
idEventDef::GetReturnType
================
*/
ID_INLINE char idEventDef::GetReturnType() const {
	return returnType;
}

/*
================
idEventDef::GetNumArgs
================
*/
ID_INLINE size_t idEventDef::GetNumArgs() const {
	return numargs;
}

/*
================
idEventDef::GetArgSize
================
*/
ID_INLINE size_t idEventDef::GetArgSize() const {
	return argsize;
}

/*
================
idEventDef::GetArgOffset
================
*/
ID_INLINE size_t idEventDef::GetArgOffset(const index_t arg ) const {
	assert( ( arg >= 0 ) && ( arg < D_EVENT_MAXARGS ) );
	return argOffset[ arg ];
}

/*
================
idEventDef::GetEventNum
================
*/
ID_INLINE size_t idEventDef::GetEventNum() const {
	return eventnum;
}

#endif /* !__SYS_EVENT_H__ */
