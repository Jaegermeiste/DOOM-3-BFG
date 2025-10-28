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

#ifndef __LEXER_H__
#define __LEXER_H__

#pragma once

/*
===============================================================================

	Lexicographical parser

	Does not use memory allocation during parsing. The lexer uses no
	memory allocation if a source is loaded with LoadMemory().
	However, idToken may still allocate memory for large strings.
	
	A number directly following the escape character '\' in a string is
	assumed to be in decimal format instead of octal. Binary numbers of
	the form 0b.. or 0B.. can also be used.

===============================================================================
*/

// lexer flags
typedef enum lexerFlags_e : uint16 {
	LEXFL_NOERRORS						= BIT(0),	// don't print any errors
	LEXFL_NOWARNINGS					= BIT(1),	// don't print any warnings
	LEXFL_NOFATALERRORS					= BIT(2),	// errors aren't fatal
	LEXFL_NOSTRINGCONCAT				= BIT(3),	// multiple strings separated by whitespaces are not concatenated
	LEXFL_NOSTRINGESCAPECHARS			= BIT(4),	// no escape characters inside strings
	LEXFL_NODOLLARPRECOMPILE			= BIT(5),	// don't use the $ sign for precompilation
	LEXFL_NOBASEINCLUDES				= BIT(6),	// don't include files embraced with < >
	LEXFL_ALLOWPATHNAMES				= BIT(7),	// allow path separators in names
	LEXFL_ALLOWNUMBERNAMES				= BIT(8),	// allow names to start with a number
	LEXFL_ALLOWIPADDRESSES				= BIT(9),	// allow ip addresses to be parsed as numbers
	LEXFL_ALLOWFLOATEXCEPTIONS			= BIT(10),	// allow float exceptions like 1.#INF or 1.#IND to be parsed
	LEXFL_ALLOWMULTICHARLITERALS		= BIT(11),	// allow multi character literals
	LEXFL_ALLOWBACKSLASHSTRINGCONCAT	= BIT(12),	// allow multiple strings separated by '\' to be concatenated
	LEXFL_ONLYSTRINGS					= BIT(13)	// parse as whitespace deliminated strings (quoted strings keep quotes)
} lexerFlags_t;

// punctuation ids
typedef enum punctuationID_e : uint8 {
	P_NONE,
	P_RSHIFT_ASSIGN,
	P_LSHIFT_ASSIGN,
	P_PARMS,
	P_PRECOMPMERGE,

	P_LOGIC_AND,
	P_LOGIC_OR,
	P_LOGIC_GEQ,
	P_LOGIC_LEQ,
	P_LOGIC_EQ,
	P_LOGIC_UNEQ,

	P_MUL_ASSIGN,
	P_DIV_ASSIGN,
	P_MOD_ASSIGN,
	P_ADD_ASSIGN,
	P_SUB_ASSIGN,
	P_INC,
	P_DEC,

	P_BIN_AND_ASSIGN,
	P_BIN_OR_ASSIGN,
	P_BIN_XOR_ASSIGN,
	P_RSHIFT,
	P_LSHIFT,

	P_POINTERREF,
	P_CPP1,
	P_CPP2,
	P_MUL,
	P_DIV,
	P_MOD,
	P_ADD,
	P_SUB,
	P_ASSIGN,

	P_BIN_AND,
	P_BIN_OR,
	P_BIN_XOR,
	P_BIN_NOT,

	P_LOGIC_NOT,
	P_LOGIC_GREATER,
	P_LOGIC_LESS,

	P_REF,
	P_COMMA,
	P_SEMICOLON,
	P_COLON,
	P_QUESTIONMARK,

	P_PARENTHESESOPEN,
	P_PARENTHESESCLOSE,
	P_BRACEOPEN,
	P_BRACECLOSE,
	P_SQBRACKETOPEN,
	P_SQBRACKETCLOSE,
	P_BACKSLASH,

	P_PRECOMP,
	P_DOLLAR
} punctuationID_t;

// punctuation
typedef struct punctuation_s
{
	const char *p;						// punctuation character(s)
	punctuationID_t n;				// punctuation id
} punctuation_t;


class idLexer {

	friend class idParser;

public:
					// constructor
					idLexer();
					idLexer( int flags );
					idLexer( const char *filename, int flags = 0, bool OSPath = false );
					idLexer( const char *ptr, size_t length, const char *name, int flags = 0 );
					// destructor
					~idLexer();
					// load a script from the given file at the given offset with the given length
	int				LoadFile( const char *filename, bool OSPath = false );
					// load a script from the given memory with the given length and a specified line offset,
					// so source strings extracted from a file can still refer to proper line numbers in the file
					// NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
	int				LoadMemory(const char *ptr, size_t length, const char *name, size_t startLine = 1);
					// free the script
	void			FreeSource();
					// returns true if a script is loaded
	[[nodiscard]] int				IsLoaded() const { return idLexer::loaded; };
					// read a token
	int				ReadToken( idToken *token );
					// expect a certain token, reads the token when available
	int				ExpectTokenString( const char *string );
					// expect a certain token type
	int				ExpectTokenType( tokenType_e type, const uint64 subtype, idToken *token);
					// expect a token
	int				ExpectAnyToken( idToken *token );
					// returns true when the token is available
	int				CheckTokenString( const char *string );
					// returns true an reads the token when a token with the given type is available
	int				CheckTokenType( tokenType_e type, uint64 subtype, idToken *token );
					// returns true if the next token equals the given string but does not remove the token from the source
	int				PeekTokenString( const char *string );
					// returns true if the next token equals the given type but does not remove the token from the source
	int				PeekTokenType( tokenType_e type, uint64 subtype, idToken *token );
					// skip tokens until the given token string is read
	int				SkipUntilString( const char *string );
					// skip the rest of the current line
	int				SkipRestOfLine();
					// skip the braced section
	int				SkipBracedSection( bool parseFirstBrace = true );
	// skips spaces, tabs, C-like comments etc. Returns false if there is no token left to read.
	bool			SkipWhiteSpace( bool currentLine );
					// unread the given token
	void			UnreadToken( const idToken *token );
					// read a token only if on the same line
	int				ReadTokenOnLine( idToken *token );
		
					//Returns the rest of the current line
	const char*		ReadRestOfLine(idStr& out);

					// read a signed integer
	int				ParseInt();
	int64			ParseInt64();
					// read a boolean
	bool			ParseBool();
					// read a floating posize_t number.  If errorFlag is NULL, a non-numeric token will
					// issue an Error().  If it isn't NULL, it will issue a Warning() and set *errorFlag = true
	float			ParseFloat( bool *errorFlag = nullptr);
					// parse matrices with floats
	int				Parse1DMatrix( int x, float *m );
	int				Parse2DMatrix( int y, int x, float *m );
	int				Parse3DMatrix( int z, int y, int x, float *m );
					// parse a braced section into a string
	const char *	ParseBracedSection( idStr &out );
					// parse a braced section into a string, maintaining indents and newlines
	const char *	ParseBracedSectionExact ( idStr &out, int tabs = -1 );
					// parse the rest of the line
	const char *	ParseRestOfLine( idStr &out );
					// pulls the entire line, including the \n at the end
	const char *	ParseCompleteLine( idStr &out );
					// retrieves the white space characters before the last read token
	size_t GetLastWhiteSpace(idStr& whiteSpace) const;
					// returns start index into text buffer of last white space
	[[nodiscard]] int64 GetLastWhiteSpaceStart() const;
					// returns end index into text buffer of last white space
	[[nodiscard]] int64 GetLastWhiteSpaceEnd() const;
					// set an array with punctuations, NULL restores default C/C++ set, see default_punctuations for an example
	void			SetPunctuations( const punctuation_t *p );
					// returns a pointer to the punctuation with the given id
	[[nodiscard]] const char *	GetPunctuationFromId( const punctuationID_t id ) const;
					// get the id for the given punctuation
	punctuationID_t GetPunctuationId( const char *p ) const;
					// set lexer flags
	void			SetFlags( int flags );
					// get lexer flags
	[[nodiscard]] int				GetFlags() const;
					// reset the lexer
	void			Reset();
					// returns true if at the end of the file
	[[nodiscard]] bool			EndOfFile() const;
					// returns the current filename
	const char *	GetFileName();
					// get offset in script
	[[nodiscard]] int64		    GetFileOffset() const;
					// get file time
	ID_TIME_T       GetFileTime() const;
					// returns the current line number
	[[nodiscard]] size_t	        GetLineNum() const;
					// print an error message
	void			Error( VERIFY_FORMAT_STRING const char *str, ... );
					// print a warning message
	void			Warning( VERIFY_FORMAT_STRING const char *str, ... ) const;
					// returns true if Error() was called with LEXFL_NOFATALERRORS or LEXFL_NOERRORS set
	[[nodiscard]] bool			HadError() const;

					// set the base folder to load files from
	static void		SetBaseFolder( const char *path );

private:
	int				loaded;					// set when a script file is loaded from file or memory
	idStr			filename;				// file name of the script
	int				allocated;				// true if buffer memory was allocated
	const char *	buffer;					// buffer containing the script
	const char *	script_p;				// current pointer in the script
	const char *	end_p;					// pointer to the end of the script
	const char *	lastScript_p;			// script pointer before reading token
	const char *	whiteSpaceStart_p;		// start of last white space
	const char *	whiteSpaceEnd_p;		// end of last white space
	ID_TIME_T		fileTime;				// file time
	size_t			length;					// length of the script in bytes
	size_t			line;					// current line in script
	size_t			lastline;				// line before reading token
	int				tokenavailable;			// set by unreadToken
	int				flags;					// several script flags
	const punctuation_t *punctuations;		// the punctuations used in the script
	int *			punctuationtable;		// ASCII table with punctuations
	int *			nextpunctuation;		// next punctuation in chain
	idToken			token;					// available token
	idLexer *		next;					// next script in a chain
	bool			hadError;				// set by idLexer::Error, even if the error is supressed

	static char		baseFolder[ 256 ];		// base folder to load files from

private:
	void			CreatePunctuationTable( const punctuation_t *punctuations );
	int				ReadWhiteSpace();
	int				ReadEscapeCharacter( char *ch );
	int				ReadString( idToken *token, int quote );
	int				ReadName( idToken *token );
	int				ReadNumber( idToken *token );
	int				ReadPunctuation( idToken *token );
	int				ReadPrimitive( idToken *token );
	bool			CheckString( const char *str ) const;
	[[nodiscard]] size_t			NumLinesCrossed() const;
};

ID_INLINE const char *idLexer::GetFileName() {
	return idLexer::filename;
}

ID_INLINE int64 idLexer::GetFileOffset() const
{
	return idLexer::script_p - idLexer::buffer;
}

ID_INLINE ID_TIME_T idLexer::GetFileTime() const
{
	return idLexer::fileTime;
}

ID_INLINE size_t idLexer::GetLineNum() const
{
	return idLexer::line;
}

ID_INLINE void idLexer::SetFlags(const int flags ) {
	idLexer::flags = flags;
}

ID_INLINE int idLexer::GetFlags() const
{
	return idLexer::flags;
}

#endif /* !__LEXER_H__ */

