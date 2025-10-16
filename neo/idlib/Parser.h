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

#ifndef __PARSER_H__
#define __PARSER_H__

#pragma once

/*
===============================================================================

	C/C++ compatible pre-compiler

===============================================================================
*/

constexpr auto DEFINE_FIXED = 0x0001;

typedef enum parserBuiltins_e : uint8
{
	BUILTIN_LINE = 1,
	BUILTIN_FILE = 2,
	BUILTIN_DATE = 3,
	BUILTIN_TIME = 4,
	BUILTIN_STDC = 5
} parserBuiltins_t;

typedef enum parserIndentType_e : uint16
{
	INDENT_NONE = 0x0000,
	INDENT_IF = 0x0001,
	INDENT_ELSE = 0x0002,
	INDENT_ELIF = 0x0004,
	INDENT_IFDEF = 0x0008,
	INDENT_IFNDEF = 0x0010
} parserIndentType_t;

// macro definitions
typedef struct define_s {
	char *			name;						// define name
	int				flags;						// define flags
	int				builtin;					// > 0 if builtin define
	size_t			numparms;					// number of define parameters
	idToken *		parms;						// define parameters
	idToken *		tokens;						// macro tokens (possibly containing parm tokens)
	struct define_s	*next;						// next defined macro in a list
	struct define_s	*hashnext;					// next define in the hash chain
} define_t;

// indents used for conditional compilation directives:
// #if, #else, #elif, #ifdef, #ifndef
typedef struct indent_s {
	parserIndentType_t	type;						// indent type
	int				    skip;						// true if skipping current indent
	idLexer *		    script;						// script the indent was in
	struct indent_s	*   next;						// next indent on the indent stack
} indent_t;


class idParser {

public:
					// constructor
					idParser();
					idParser( int flags );
					idParser( const char *filename, int flags = 0, bool OSPath = false );
					idParser( const char *ptr, size_t length, const char *name, int flags = 0 );
					// destructor
					~idParser();
					// load a source file
	bool			LoadFile( const char *filename, bool OSPath = false );
					// load a source from the given memory with the given length
					// NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
	bool			LoadMemory( const char *ptr, size_t length, const char *name );
					// free the current source
	void			FreeSource( bool keepDefines = false );
					// returns true if a source is loaded
	[[nodiscard]] bool	IsLoaded() const { return idParser::loaded; }
					// read a token from the source
	bool			ReadToken( idToken *token );
					// expect a certain token, reads the token when available
	bool			ExpectTokenString( const char *string );
					// expect a certain token type
	bool			ExpectTokenType( tokenType_t type, uint64 subtype, idToken *token );
					// expect a token
	bool			ExpectAnyToken( idToken *token );
					// returns true if the next token equals the given string and removes the token from the source
	bool			CheckTokenString( const char *string );
					// returns true if the next token equals the given type and removes the token from the source
	bool			CheckTokenType( int type, uint64 subtype, idToken *token );
					// returns true if the next token equals the given string but does not remove the token from the source
	bool			PeekTokenString( const char *string );
					// returns true if the next token equals the given type but does not remove the token from the source
	bool			PeekTokenType( tokenType_t type, uint64 subtype, idToken *token );
					// skip tokens until the given token string is read
	bool			SkipUntilString( const char *string );
					// skip the rest of the current line
	bool			SkipRestOfLine();
					// skip the braced section
	bool			SkipBracedSection( bool parseFirstBrace = true );
					// parse a braced section into a string
	const char*		ParseBracedSection( idStr& out, int tabs, bool parseFirstBrace, char intro, char outro );
					// parse a braced section into a string, maintaining indents and newlines
	const char *	ParseBracedSectionExact( idStr &out, int tabs = -1 ) const;
					// parse the rest of the line
	const char *	ParseRestOfLine( idStr &out );
					// unread the given token
	void			UnreadToken( idToken *token );
					// read a token only if on the current line
	bool			ReadTokenOnLine( idToken *token );
					// read a signed integer
	int				ParseInt();
					// read a boolean
	bool			ParseBool();
					// read a floating point number
	float			ParseFloat();
					// parse matrices with floats
	bool			Parse1DMatrix( int x, float *m );
	bool			Parse2DMatrix( int y, int x, float *m );
	bool			Parse3DMatrix( int z, int y, int x, float *m );
					// get the white space before the last read token
	size_t          GetLastWhiteSpace(idStr& whiteSpace) const;
					// Set a marker in the source file (there is only one marker)
	void			SetMarker();
					// Get the string from the marker to the current position
	void			GetStringFromMarker( idStr& out, bool clean = false );
					// add a define to the source
	bool			AddDefine( const char *string );
					// add builtin defines
	void			AddBuiltinDefines();
					// set the source include path
	void			SetIncludePath( const char *path );
					// set the punctuation set
	void			SetPunctuations( const punctuation_t *p );
					// returns a pointer to the punctuation with the given id
	[[nodiscard]] const char *	GetPunctuationFromId( const punctuationID_t id ) const;
					// get the id for the given punctuation
	int				GetPunctuationId( const char *p ) const;
					// set lexer flags
	void			SetFlags( int flags );
					// get lexer flags
	[[nodiscard]] int				GetFlags() const;
					// returns the current filename
	[[nodiscard]] const char *	GetFileName() const;
					// get current offset in current script
	[[nodiscard]] int64 GetFileOffset() const;
					// get file time for current script
	ID_TIME_T       GetFileTime() const;
					// returns the current line number
	[[nodiscard]] size_t		GetLineNum() const;
					// print an error message
	void			Error( VERIFY_FORMAT_STRING const char *str, ... ) const;
					// print a warning message
	void			Warning( VERIFY_FORMAT_STRING const char *str, ... ) const;
	// returns true if at the end of the file
	[[nodiscard]] bool			EndOfFile() const;
					// add a global define that will be added to all opened sources
	static bool		AddGlobalDefine( const char *string );
					// remove the given global define
	static bool		RemoveGlobalDefine( const char *name );
					// remove all global defines
	static void		RemoveAllGlobalDefines();
					// set the base folder to load files from
	static void		SetBaseFolder( const char *path );

private:
	bool			loaded;						// set when a source file is loaded from file or memory
	idStr			filename;					// file name of the script
	idStr			includepath;				// path to include files
	bool			OSPath;						// true if the file was loaded from an OS path
	const punctuation_t *punctuations;			// punctuations to use
	int				flags;						// flags used for script parsing
	idLexer *		scriptstack;				// stack with scripts of the source
	idToken *		tokens;						// tokens to read first
	define_t *		defines;					// list with macro definitions
	define_t **		definehash;					// hash chain with defines
	indent_t *		indentstack;				// stack with indents
	int				skip;						// > 0 if skipping conditional code
	const char*		marker_p;

	static define_t *globaldefines;				// list with global defines added to every source loaded

private:
	void			PushIndent( parserIndentType_t type, int skip );
	void			PopIndent( parserIndentType_t *type, int *skip );
	void			PushScript( idLexer *script );
	bool			ReadSourceToken( idToken *token );
	bool			ReadLine( idToken *token );
	bool			UnreadSourceToken( idToken *token );
	bool			ReadDefineParms( define_t *define, idToken **parms, const size_t maxparms );
	bool			StringizeTokens( idToken *tokens, idToken *token );
	bool			MergeTokens( idToken *t1, idToken *t2 );
	bool			ExpandBuiltinDefine( idToken *deftoken, define_t *define, idToken **firsttoken, idToken **lasttoken ) const;
	bool			ExpandDefine( idToken *deftoken, define_t *define, idToken **firsttoken, idToken **lasttoken );
	bool			ExpandDefineIntoSource( idToken *deftoken, define_t *define );
	void			AddGlobalDefinesToSource();
	define_t *		CopyDefine( define_t *define );
	define_t *		FindHashedDefine(define_t **definehash, const char *name);
	int				FindDefineParm( define_t *define, const char *name );
	void			AddDefineToHash(define_t *define, define_t **definehash);
	static void		PrintDefine( define_t *define );
	static void		FreeDefine( define_t *define );
	static define_t *FindDefine( define_t *defines, const char *name );
	static define_t *DefineFromString( const char *string);
	define_t *		CopyFirstDefine();
	bool			Directive_include();
	bool			Directive_undef();
	bool			Directive_if_def( parserIndentType_t type );
	bool			Directive_ifdef();
	bool			Directive_ifndef();
	bool			Directive_else();
	bool			Directive_endif();
	bool			EvaluateTokens( idToken *tokens, signed long int *intvalue, double *floatvalue, int integer );
	bool			Evaluate( signed long int *intvalue, double *floatvalue, int integer );
	bool			DollarEvaluate( signed long int *intvalue, double *floatvalue, int integer);
	bool			Directive_define();
	bool			Directive_elif();
	bool			Directive_if();
	bool			Directive_line();
	bool			Directive_error();
	bool			Directive_warning();
	bool			Directive_pragma();
	void			UnreadSignToken();
	bool			Directive_eval();
	bool			Directive_evalfloat();
	bool			ReadDirective();
	bool			DollarDirective_evalint();
	bool			DollarDirective_evalfloat();
	bool			ReadDollarDirective();
};

ID_INLINE const char *idParser::GetFileName() const {
	if ( idParser::scriptstack ) {
		return idParser::scriptstack->GetFileName();
	}
	else {
		return "";
	}
}

ID_INLINE int64 idParser::GetFileOffset() const {
	if ( idParser::scriptstack ) {
		return idParser::scriptstack->GetFileOffset();
	}
	else {
		return 0;
	}
}

ID_INLINE ID_TIME_T idParser::GetFileTime() const {
	if ( idParser::scriptstack ) {
		return idParser::scriptstack->GetFileTime();
	}
	else {
		return 0;
	}
}

ID_INLINE size_t idParser::GetLineNum() const {
	if ( idParser::scriptstack ) {
		return idParser::scriptstack->GetLineNum();
	}
	else {
		return 0;
	}
}

#endif /* !__PARSER_H__ */
