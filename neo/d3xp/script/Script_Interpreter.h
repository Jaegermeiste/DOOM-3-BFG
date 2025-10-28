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

#ifndef __SCRIPT_INTERPRETER_H__
#define __SCRIPT_INTERPRETER_H__

#define MAX_STACK_DEPTH 	64
#define LOCALSTACK_SIZE 	6144

typedef struct prstack_s {
	int 				s;
	const function_t	*f;
	int 				stackbase;
} prstack_t;

class idInterpreter {
private:
	prstack_t			callStack[ MAX_STACK_DEPTH ];
	size_t 				callStackDepth;
	size_t 				maxStackDepth;

	byte				localstack[ LOCALSTACK_SIZE ];
	size_t 				localstackUsed;
	size_t 				localstackBase;
	size_t 				maxLocalstackUsed;

	const function_t	*currentFunction;
	address_t			instructionPointer;

	int					popParms;
	const idEventDef	*multiFrameEvent;
	idEntity			*eventEntity;

	idThread			*thread;

	void				PopParms( const size_t numParms );
	void				PushString( const char *string );
	void				Push( int value );
	static const char			*FloatToString( float value );
	void				AppendString( idVarDef *def, const char *from );
	void				SetString( idVarDef *def, const char *from );
	const char			*GetString( idVarDef *def );
	varEval_t			GetVariable( idVarDef *def );
	idEntity			*GetEntity( index_t entnum ) const;
	idScriptObject		*GetScriptObject( index_t entnum ) const;
	void				NextInstruction( address_t position );

	void				LeaveFunction( idVarDef *returnDef );
	void				CallEvent( const function_t *func, const size_t argsize );
	void				CallSysEvent( const function_t *func, const size_t argsize );

public:
	bool				doneProcessing;
	bool				threadDying;
	bool				terminateOnExit;
	bool				debug;

						idInterpreter();

	// save games
	void				Save( idSaveGame *savefile ) const;				// archives object for save game file
	void				Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void				SetThread( idThread *pThread );

	void				StackTrace() const;

	int					CurrentLine() const;
	const char			*CurrentFile() const;

	void				Error( VERIFY_FORMAT_STRING const char *fmt, ... ) const;
	void				Warning( VERIFY_FORMAT_STRING const char *fmt, ... ) const;
	void				DisplayInfo() const;

	bool				BeginMultiFrameEvent( idEntity *ent, const idEventDef *event );
	void				EndMultiFrameEvent( idEntity *ent, const idEventDef *event );
	bool				MultiFrameEventInProgress() const;

	void				ThreadCall( idInterpreter *source, const function_t *func, int args );
	void				EnterFunction( const function_t *func, bool clearStack );
	void				EnterObjectFunction( idEntity *self, const function_t *func, bool clearStack );

	bool				Execute();
	void				Reset();

	bool				GetRegisterValue( const char *name, idStr &out, int scopeDepth );
	int					GetCallstackDepth() const;
	const prstack_t		*GetCallstack() const;
	const function_t	*GetCurrentFunction() const;
	idThread			*GetThread() const;

};

/*
====================
idInterpreter::PopParms
====================
*/
ID_INLINE void idInterpreter::PopParms( const size_t numParms ) {
	// pop our parms off the stack
	if ( localstackUsed < numParms ) {
		Error( "locals stack underflow\n" );
	}

	localstackUsed -= numParms;
}

/*
====================
idInterpreter::Push
====================
*/
ID_INLINE void idInterpreter::Push(const int value ) {
	if ( localstackUsed + sizeof( int ) > LOCALSTACK_SIZE ) {
		Error( "Push: locals stack overflow\n" );
	}
	*reinterpret_cast<int*>(&localstack[localstackUsed])	= value;
	localstackUsed += sizeof( int );
}

/*
====================
idInterpreter::PushString
====================
*/
ID_INLINE void idInterpreter::PushString( const char *string ) {
	if ( localstackUsed + MAX_STRING_LEN > LOCALSTACK_SIZE ) {
		Error( "PushString: locals stack overflow\n" );
	}
	idStr::Copynz( reinterpret_cast<char*>(&localstack[localstackUsed]), string, MAX_STRING_LEN );
	localstackUsed += MAX_STRING_LEN;
}

/*
====================
idInterpreter::FloatToString
====================
*/
ID_INLINE const char *idInterpreter::FloatToString( float value ) {
	static char	text[ 32 ] = {};

	if ( std::equal_to<>()(value, std::trunc(value)) ) {
		sprintf( text, "%lld", numeric_cast<int64>(value) );
	} else {
		sprintf( text, "%f", value );
	}
	return text;
}

/*
====================
idInterpreter::AppendString
====================
*/
ID_INLINE void idInterpreter::AppendString( idVarDef *def, const char *from ) {
	if ( def->initialized == idVarDef::stackVariable ) {
		idStr::Append( reinterpret_cast<char*>(&localstack[localstackBase + def->value.stackOffset]), MAX_STRING_LEN, from );
	} else {
		idStr::Append( def->value.stringPtr, MAX_STRING_LEN, from );
	}
}

/*
====================
idInterpreter::SetString
====================
*/
ID_INLINE void idInterpreter::SetString( idVarDef *def, const char *from ) {
	if ( def->initialized == idVarDef::stackVariable ) {
		idStr::Copynz( reinterpret_cast<char*>(&localstack[localstackBase + def->value.stackOffset]), from, MAX_STRING_LEN );
	} else {
		idStr::Copynz( def->value.stringPtr, from, MAX_STRING_LEN );
	}
}

/*
====================
idInterpreter::GetString
====================
*/
ID_INLINE const char *idInterpreter::GetString( idVarDef *def ) {
	if ( def->initialized == idVarDef::stackVariable ) {
		return reinterpret_cast<char*>(&localstack[localstackBase + def->value.stackOffset]);
	} else {
		return def->value.stringPtr;
	}
}

/*
====================
idInterpreter::GetVariable
====================
*/
ID_INLINE varEval_t idInterpreter::GetVariable( idVarDef *def ) {
	if ( def->initialized == idVarDef::stackVariable ) {
		varEval_t val;
		val.intPtr = reinterpret_cast<int*>(&localstack[localstackBase + def->value.stackOffset]);
		return val;
	} else {
		return def->value;
	}
}

/*
================
idInterpreter::GetEntity
================
*/
ID_INLINE idEntity *idInterpreter::GetEntity(const index_t entnum ) const{
	ORDINAL_CHECK( entnum, MAX_GENTITIES + 1 );  // Offset 0-base
	if ( ( entnum > 0 ) && ( entnum <= MAX_GENTITIES ) ) {
		return gameLocal.entities[ entnum - 1 ];
	}
	return nullptr;
}

/*
================
idInterpreter::GetScriptObject
================
*/
ID_INLINE idScriptObject *idInterpreter::GetScriptObject(const index_t entnum ) const {
	ORDINAL_CHECK(entnum, MAX_GENTITIES + 1);  // Offset 0-base

	if ( ( entnum > 0 ) && ( entnum <= MAX_GENTITIES ) ) {
		idEntity* ent = gameLocal.entities[entnum - 1];
		if ( ent && ent->scriptObject.data ) {
			return &ent->scriptObject;
		}
	}
	return nullptr;
}

/*
====================
idInterpreter::NextInstruction
====================
*/
ID_INLINE void idInterpreter::NextInstruction(const address_t position ) {
	// Before we execute an instruction, we increment instructionPointer,
	// therefore we need to compensate for that here.
	instructionPointer = position - 1;
}

#endif /* !__SCRIPT_INTERPRETER_H__ */
