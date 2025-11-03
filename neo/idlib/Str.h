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

#ifndef __STR_H__
#define __STR_H__

#pragma once

/*
===============================================================================

	Character string

===============================================================================
*/

#define ASSERT_ENUM_STRING( string, index )		( 1 / (int64)!( (string) - (index) ) ) ? #string : ""

enum utf8Encoding_t : uint8 {
	UTF8_PURE_ASCII,		// no characters with values > 127
	UTF8_ENCODED_BOM,		// characters > 128 encoded with UTF8, but no byte-order-marker at the beginning
	UTF8_ENCODED_NO_BOM,	// characters > 128 encoded with UTF8, with a byte-order-marker at the beginning
	UTF8_INVALID,			// has values > 127 but isn't valid UTF8 
	UTF8_INVALID_BOM		// has a byte-order-marker at the beginning, but isn't valued UTF8 -- it's messed up
};

// these library functions should not be used for cross platform compatibility
#define strcmp			idStr::Cmp		// use_idStr_Cmp
#define strncmp			use_idStr_Cmpn

#if defined( StrCmpN )
#undef StrCmpN
#endif
#define StrCmpN			use_idStr_Cmpn

#if defined( strcmpi )
#undef strcmpi
#endif
#define strcmpi			use_idStr_Icmp

#if defined( StrCmpI )
#undef StrCmpI
#endif
#define StrCmpI			use_idStr_Icmp

#if defined( StrCmpNI )
#undef StrCmpNI
#endif
#define StrCmpNI		use_idStr_Icmpn

#define stricmp			idStr::Icmp		// use_idStr_Icmp
#define _stricmp		use_idStr_Icmp
#define strcasecmp		use_idStr_Icmp
#define strnicmp		use_idStr_Icmpn
#define _strnicmp		use_idStr_Icmpn
#define _memicmp		use_idStr_Icmpn

#define snprintf		use_idStr_snPrintf
#define _snprintf		use_idStr_snPrintf
#define vsnprintf		use_idStr_vsnPrintf
#define _vsnprintf		use_idStr_vsnPrintf

class idVec4;

#ifndef FILE_HASH_SIZE
#define FILE_HASH_SIZE		1024
#endif

// color escape character
constexpr int C_COLOR_ESCAPE			= '^';
constexpr int C_COLOR_DEFAULT			= '0';
constexpr int C_COLOR_RED				= '1';
constexpr int C_COLOR_GREEN				= '2';
constexpr int C_COLOR_YELLOW			= '3';
constexpr int C_COLOR_BLUE				= '4';
constexpr int C_COLOR_CYAN				= '5';
constexpr int C_COLOR_ORANGE			= '6';
constexpr int C_COLOR_WHITE				= '7';
constexpr int C_COLOR_GRAY				= '8';
constexpr int C_COLOR_BLACK				= '9';

// color escape string
#define S_COLOR_DEFAULT				"^0"
#define S_COLOR_RED					"^1"
#define S_COLOR_GREEN				"^2"
#define S_COLOR_YELLOW				"^3"
#define S_COLOR_BLUE				"^4"
#define S_COLOR_CYAN				"^5"
#define S_COLOR_ORANGE				"^6"
#define S_COLOR_WHITE				"^7"
#define S_COLOR_GRAY				"^8"
#define S_COLOR_BLACK				"^9"

// make idStr a multiple of 16 bytes long
// don't make too large to keep memory requirements to a minimum
constexpr size_t STR_ALLOC_BASE		= 20;
constexpr size_t STR_ALLOC_BASE_NUM = 64;
constexpr size_t STR_ALLOC_GRAN		= 32;

constexpr auto STR_UNKNOWN_ENUM = "<unknown>";

typedef enum : uint8 {
	MEASURE_SIZE = 0,
	MEASURE_BANDWIDTH
} Measure_t;

class idStr {

public:
						idStr() noexcept;
						idStr( const StringLike auto &text );
	           explicit idStr( const FormattableNoStrings auto &value );
						idStr( const StringLike auto &text, size_t start, size_t end );
			   explicit idStr( const FormattableNoStrings auto& text, size_t start, size_t end);
						~idStr();

	[[nodiscard]] size_t			Size() const;
	[[nodiscard]] const char* 		c_str() const;
	operator			const char* () const;
	operator			const char* ();

	
	char				operator[]( const Ordinal auto index ) const;
	char &				operator[]( const Ordinal auto index );

	idStr &             operator=( const StringLike auto &text );
	idStr &             operator=( const FormattableNoStrings auto &value );

	friend idStr        operator+( const StringLike auto &a, const StringLike auto &b );
	friend idStr        operator+( const StringLike auto &a, const FormattableNoStrings auto &b );
	friend idStr        operator+( const FormattableNoStrings auto &a, const StringLike auto &b );

	idStr &				operator+=( const StringLike auto &a );
	idStr &				operator+=( const FormattableNoStrings auto &a );

						// case sensitive compare
	friend bool         operator==( const idStr& a, const idStr& b );
	friend bool         operator==( const idStr& a, const StringLike auto &b );
	friend bool         operator==( const idStr& a, const FormattableNoStrings auto &b );
	friend bool         operator==( const StringLike auto& a, const idStr& b );
	friend bool         operator==( const FormattableNoStrings auto &a, const idStr& b);
	friend bool    		operator==( const StringLike auto &a, const StringLike auto &b );

						// case sensitive compare
	friend bool         operator!=( const idStr& a, const idStr& b );
	friend bool         operator!=( const idStr& a, const StringLike auto& b );
	friend bool         operator!=( const idStr& a, const FormattableNoStrings auto &b );
	friend bool         operator!=( const StringLike auto& a, const idStr& b );
	friend bool         operator!=( const FormattableNoStrings auto &a, const idStr& b );
	friend bool    		operator!=( const StringLike auto &a, const StringLike auto &b );

						// case sensitive compare
	int					Cmp( const StringLikeOrEnum auto text ) const;
	int					Cmpn( const StringLikeOrEnum auto text, const size_t n ) const;
	int					CmpPrefix( const StringLikeOrEnum auto text ) const;

						// case insensitive compare
	int					Icmp( const StringLikeOrEnum auto text ) const;
	int					Icmpn( const StringLikeOrEnum auto text, const size_t n ) const;
	int					IcmpPrefix( const StringLikeOrEnum auto text ) const;

						// case insensitive compare ignoring color
	int					IcmpNoColor( const StringLikeOrEnum auto text ) const;

						// compares paths and makes sure folders come first
	int					IcmpPath( const StringLikeOrEnum auto text ) const;
	int					IcmpnPath( const StringLikeOrEnum auto text, const size_t n ) const;
	int					IcmpPrefixPath( const StringLikeOrEnum auto text ) const;

	[[nodiscard]] size_t				Length() const;
	[[nodiscard]] size_t				Allocated() const;
	void				Empty();
	[[nodiscard]] bool				IsEmpty() const;
	void				Clear();
	void				Append( const char a );
	void				Append( const idStr &text );
	void				Append( const StringLikeOrEnum auto text );
	template < Formattable T >
		requires (!StringLikeOrEnum< T >)
	void				Append( const T &value );
	void				Append( const char * text, const size_t len );
	void				Insert( const char * text, Ordinal auto index );
	void				Insert( const StringLikeOrEnum auto &text, Ordinal auto index );
	template < Formattable T >
		requires (!StringLikeOrEnum< T >)
	void				Insert( const T &value, Ordinal auto index );
	void				ToLower() const;
	void				ToUpper() const;
	[[nodiscard]] bool	IsNumeric() const;
	[[nodiscard]] bool	IsColor() const;
	[[nodiscard]] bool	HasLower() const;
	[[nodiscard]] bool	HasUpper() const;
	[[nodiscard]] size_t LengthWithoutColors() const;
	idStr &				RemoveColors();
	void				CapLength(size_t);
	void				Fill( const char ch, size_t newlen );

	ID_INLINE size_t		UTF8Length() const;
	
	ID_INLINE uint32		UTF8Char( Ordinal auto & idx ) const;
	static size_t			UTF8Length( const byte * s );
	
	static ID_INLINE uint32 UTF8Char( const char *  s, Ordinal auto& idx );
	
	static uint32			UTF8Char( const byte * s, Ordinal auto& idx );
	void					AppendUTF8Char( uint32 c );
	ID_INLINE void			ConvertToUTF8();
	static bool				IsValidUTF8( const uint8 * s, const size_t maxLen, utf8Encoding_t & encoding );
	static ID_INLINE bool	IsValidUTF8( const char *  s, const size_t maxLen, utf8Encoding_t & encoding ) { return IsValidUTF8( reinterpret_cast<const uint8*>(s), maxLen, encoding ); }
	static ID_INLINE bool	IsValidUTF8( const uint8 * s, const size_t maxLen );
	static ID_INLINE bool	IsValidUTF8( const char *  s, const size_t maxLen ) { return IsValidUTF8( reinterpret_cast<const uint8*>(s), maxLen ); }

	[[nodiscard]] index_t   Find( const char c ) const;
	[[nodiscard]] index_t   Find( const char c, const Ordinal auto start = 0, const Ordinal auto end = -1 ) const;
	[[nodiscard]] index_t   Find( const char* text, bool casesensitive = true ) const;
	[[nodiscard]] index_t   Find( const char* text, bool casesensitive, const Ordinal auto start ) const;
	[[nodiscard]] index_t   Find( const char* text, bool casesensitive, const Ordinal auto start, const Ordinal auto end ) const;
	bool				Filter( const char * filter, bool casesensitive ) const;
	[[nodiscard]] index_t	Last( const char c ) const;						// return the index to the last occurrence of 'c', returns -1 if not found
	const char* Left( size_t len, idStr &result ) const;			// store the leftmost 'len' characters in the result
	const char* Right( size_t len, idStr &result ) const;			// store the rightmost 'len' characters in the result
	const char* Mid( const Ordinal auto start, size_t len, idStr &result ) const;	// store 'len' characters starting at 'start' in result
	[[nodiscard]] idStr				Left( size_t len ) const;							// return the leftmost 'len' characters
	[[nodiscard]] idStr				Right( size_t len ) const;							// return the rightmost 'len' characters
	[[nodiscard]] idStr				Mid( const Ordinal auto start, size_t len ) const;				// return 'len' characters starting at 'start'
	void				Format( VERIFY_FORMAT_STRING const char * fmt, ... );					// perform a threadsafe sprintf to the string
	static idStr		FormatInt( const std::integral auto num, bool isCash = false );			// formats an integer as a value with commas
	static idStr		FormatCash( const std::integral auto num ) { return FormatInt( num, true ); }
	void				StripLeading( const char c );					// strip char from front as many times as the char occurs
	void				StripLeading( const char * string );				// strip string from front as many times as the string occurs
	bool				StripLeadingOnce( const char * string );			// strip string from front just once if it occurs
	void				StripTrailing( const char c );					// strip char from end as many times as the char occurs
	void				StripTrailing( const char * string );			// strip string from end as many times as the string occurs
	bool				StripTrailingOnce( const char * string );		// strip string from end just once if it occurs
	void				Strip( const char c );							// strip char from front and end as many times as the char occurs
	void				Strip( const char * string );					// strip string from front and end as many times as the string occurs
	void				StripTrailingWhitespace();				// strip trailing white space characters
	idStr &				StripQuotes();							// strip quotes around string
	bool				Replace( const char * old, const char * nw );
	[[nodiscard]] bool	ReplaceChar( const char old, const char nw ) const;
	ID_INLINE void		CopyRange( const char *  text, const Ordinal auto start, const Ordinal auto end );

	// file name methods
	[[nodiscard]] int	FileNameHash() const;						// hash key for the filename (skips extension)
	idStr &				BackSlashesToSlashes();					// convert slashes
	idStr &				SlashesToBackSlashes();					// convert slashes
	idStr &				SetFileExtension( const char * extension );		// set the given file extension
	idStr &				StripFileExtension();						// remove any file extension
	idStr &				StripAbsoluteFileExtension();				// remove any file extension looking from front (useful if there are multiple .'s)
	idStr &				DefaultFileExtension( const char * extension );	// if there's no file extension use the default
	idStr &				DefaultPath( const char * basepath );			// if there's no path use the default
	void				AppendPath( const char * text );					// append a partial path
	idStr &				StripFilename();							// remove the filename from a path
	idStr &				StripPath();								// remove the path from the filename
	void				ExtractFilePath( idStr &dest ) const;			// copy the file path to another string
	void				ExtractFileName( idStr &dest ) const;			// copy the filename to another string
	void				ExtractFileBase( idStr &dest ) const;			// copy the filename minus the extension to another string
	void				ExtractFileExtension( idStr &dest ) const;		// copy the file extension to another string
	bool				CheckExtension( const char * ext ) const;

	// char * methods to replace library functions
	static size_t	    Length( const char* s );
	static char *		ToLower( char *s );
	static char *		ToUpper( char *s );
	static bool			IsNumeric( const char *s );
	static bool			IsColor( const char *s );
	static bool			HasLower( const char *s );
	static bool			HasUpper( const char *s );
	static size_t		LengthWithoutColors( const char *s );
	static char *		RemoveColors( char *s );
	static int			Cmp( const char *s1, const char *s2 );
	static int			Cmpn( const char *s1, const char *s2, size_t n );
	static int			Icmp( const char *s1, const char *s2 );
	static int			Icmpn( const char *s1, const char *s2, size_t n );
	static int			IcmpNoColor( const char *s1, const char *s2 );
	static int			IcmpPath( const char *s1, const char *s2 );			// compares paths and makes sure folders come first
	static int			IcmpnPath( const char *s1, const char *s2, size_t n );	// compares paths and makes sure folders come first
	static void			Append( char *dest, size_t size, const char *src );
	static void			Copynz( char *dest, const char *src, size_t destsize );
	static int64        snPrintf( char* dest, size_t size, VERIFY_FORMAT_STRING const char* fmt, ... );
	static int64		vsnPrintf( char *dest, size_t size, const char *fmt, va_list argptr );
	static int64        FindChar(const char* str, const char c);
	static int64        FindChar( const char* str, const char c, const Ordinal auto start = 0, const Ordinal auto end = -1 );
	static int64        FindText(const char* str, const char* text, bool casesensitive = true);
	static int64        FindText( const char* str, const char* text, bool casesensitive = true, Ordinal auto start = 0, Ordinal auto end = -1 );
	static bool			Filter( const char *filter, const char *name, bool casesensitive );
	static void			StripMediaName( const char *name, idStr &mediaName );
	static bool			CheckExtension( const char *name, const char *ext );
	static const char *	FloatArrayToString( const float *array, const size_t length, const size_t precision );
	static const char *	CStyleQuote( const char *str );
	static const char *	CStyleUnQuote( const char *str );
	template <std::floating_point T>
	static T            AtoF(const char* str) noexcept;
	template <std::integral T>
	static T            AtoI(const char* str) noexcept;

	// Converts stored string back to enum of the same type.
	// Returns std::nullopt if the saved type doesn't match or conversion fails.
	template <typename E>
		requires std::is_enum_v<E>
	E GetEnum( const E fallback = E{} ) const noexcept;

	template < typename T >
		requires (StringLikeOrEnum< T > || Formattable < T >)
	[[nodiscard]] static const char* ToCString( const T &value_in, char* string_out, const size_t buffer_length, const EnumNameOptions options = {} ) noexcept;

	static size_t       WideToUtf8( const wchar_t* src, char* out, const size_t capacity ) noexcept; // Convert a wide string (UTF-16) to UTF-8 in-place.
	static size_t       WideCopy( const wchar_t* src, wchar_t* dst, const size_t capacity ) noexcept;

	// hash keys
	[[nodiscard]] static int			Hash( const StringLikeOrEnum auto &string );
	[[nodiscard]] static int			Hash( const StringLikeOrEnum auto& string, size_t length );
	[[nodiscard]] static int64	    	Hash64( const StringLikeOrEnum auto& string );
	[[nodiscard]] static int64	    	Hash64( const StringLikeOrEnum auto& string, size_t length );
	[[nodiscard]] static int			IHash( const StringLikeOrEnum auto& string );					// case insensitive
	[[nodiscard]] static int			IHash( const StringLikeOrEnum auto &string, size_t length );		// case insensitive
	[[nodiscard]] static int64	    	IHash64( const StringLikeOrEnum auto& string );					// case insensitive
	[[nodiscard]] static int64	    	IHash64( const StringLikeOrEnum auto& string, size_t length );		// case insensitive

	// character methods
	[[nodiscard]] static char			ToLower( const std::integral auto c );
	[[nodiscard]] static char			ToUpper( const std::integral auto c );
	[[nodiscard]] static bool			CharIsPrintable( const std::integral auto c );
	[[nodiscard]] static bool			CharIsLower( const std::integral auto c );
	[[nodiscard]] static bool			CharIsUpper( const std::integral auto c );
	[[nodiscard]] static bool			CharIsAlpha( const std::integral auto c );
	[[nodiscard]] static bool			CharIsNumeric( const std::integral auto c );
	[[nodiscard]] static bool			CharIsNewLine( const std::integral auto c );
	[[nodiscard]] static bool			CharIsTab( const std::integral auto c );
	[[nodiscard]] static index_t		ColorIndex( const std::integral auto c );
	[[nodiscard]] static idVec4 &		ColorForIndex( const Ordinal auto i );

	friend int64		sprintf( idStr &dest, const char *fmt, ... );
	friend int64		vsprintf( idStr &dest, const char *fmt, va_list ap );

	static size_t       ItoA(char* buffer, const size_t buffer_size, const std::integral auto value);
	static size_t       FtoA(char* buffer, const size_t buffer_size, const std::floating_point auto value, std::chars_format fmt = std::chars_format::fixed);

	void				ReAllocate(size_t amount, bool keepold );				// reallocate string data buffer
	void				FreeData();									// free allocated string memory

						// format value in the given measurement with the best unit, returns the best unit
	int					BestUnit( const char *format, float value, Measure_t measure );
						// format value in the requested unit and measurement
	void				SetUnit( const char *format, float value, int unit, Measure_t measure );

	static void			InitMemory();
	static void			ShutdownMemory();
	static void			PurgeMemory();
	static void			ShowMemoryUsage_f( const idCmdArgs &args );

	[[nodiscard]] size_t				DynamicMemoryUsed() const;
	static idStr		FormatNumber( const std::integral auto number );

protected:
	size_t				len = 0;
	char *				data = nullptr;
	uint32				allocedAndFlag;	// top bit is used to store a flag that indicates if the string data is static or not
	char				baseBuffer[ STR_ALLOC_BASE ];

	void				EnsureAlloced( size_t amount, bool keepold = true );	// ensure string data buffer is large enough

	// sets the data point to the specified buffer... note that this ignores makes the passed buffer empty and ignores
	// anything currently in the idStr's dynamic buffer.  This method is intended to be called only from a derived class's constructor.
	ID_INLINE void		SetStaticBuffer( char * buffer, const size_t bufferLength );

private:
	std::type_index original_type = { typeid(void*) };
	std::type_index current_type = { typeid(void*) };

	EnumNameOptions enumNameOptions;

	// initialize string using base buffer... call ONLY FROM CONSTRUCTOR
	ID_INLINE void		Construct();										

	static constexpr uint32	STATIC_BIT	= 31u;
	static constexpr uint32	STATIC_MASK	= 1u << STATIC_BIT;
	static constexpr uint32	ALLOCED_MASK = STATIC_MASK - 1;


	ID_INLINE size_t	GetAlloced() const { return allocedAndFlag & ALLOCED_MASK; }
	ID_INLINE void		SetAlloced( const size_t a ) { allocedAndFlag = ( allocedAndFlag & STATIC_MASK ) | ( a & ALLOCED_MASK); }

	ID_INLINE bool		IsStatic() const { return ( allocedAndFlag & STATIC_MASK ) != 0; }
	ID_INLINE void		SetStatic( const bool isStatic ) { allocedAndFlag = ( allocedAndFlag & ALLOCED_MASK ) | ( isStatic << STATIC_BIT ); }

	size_t CopyToData( const char * text, const size_t textLength = 0);

public:
	static constexpr index_t	INVALID_POSITION = -1;
};

char *					va( VERIFY_FORMAT_STRING const char *fmt, ... );

/*
================================================================================================

	Sort routines for sorting idList<idStr>

================================================================================================
*/

class idSort_Str : public idSort_Quick< idStr, idSort_Str > {
public:
	[[nodiscard]] int Compare( const idStr & a, const idStr & b ) const { return a.Icmp( b ); }
};

class idSort_PathStr : public idSort_Quick< idStr, idSort_PathStr > {
public:
	[[nodiscard]] int Compare( const idStr & a, const idStr & b ) const { return a.IcmpPath( b ); }
};

/*
========================
idStr::Construct
========================
*/
ID_INLINE void idStr::Construct() {
	SetStatic( false );
	SetAlloced( STR_ALLOC_BASE );
	data = baseBuffer;
	len = 0;
	data[ 0 ] = '\0';
#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
	memset( baseBuffer, 0, sizeof( baseBuffer ) );
#endif
}


ID_INLINE void idStr::EnsureAlloced(const size_t amount, const bool keepold ) {
	// static string's can't reallocate
	if ( IsStatic() ) {
		release_assert( std::cmp_less_equal(amount, GetAlloced() ) )
		return;
	}
	if (std::cmp_greater(amount, GetAlloced())) {
		ReAllocate( amount, keepold );
	}
}

/*
========================
idStr::SetStaticBuffer
========================
*/
ID_INLINE void idStr::SetStaticBuffer( char * buffer, const size_t bufferLength ) {
	// this should only be called on a freshly constructed idStr
	assert( data == baseBuffer );
	data = buffer;
	len = 0;
	SetAlloced( bufferLength );
	SetStatic( true );
}

ID_INLINE idStr::idStr() noexcept {
	Construct();

	original_type = std::type_index(typeid(void*));
	current_type = original_type;
}

ID_INLINE idStr::idStr( const StringLike auto &text ) {
	Construct();

	original_type = std::type_index(typeid(text));

	operator=(text);
}

ID_INLINE idStr::idStr(const FormattableNoStrings auto &value) {
	Construct();

	original_type = std::type_index(typeid(value));

	operator=(value);
}

ID_INLINE idStr::idStr( const StringLike auto &text, size_t start, size_t end ) {
	Construct();

	const char * string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring_text = idStr::ToCString(text, string_buffer, sizeof(string_buffer));

	size_t l = strlen(cstring_text);

	end = Min(end, l);
	if ( start > l ) {
		start = l;
	} else if ( start < 0 ) {
		start = 0;
	}

	l = end - start;
	l = Max(l, 0);

	EnsureAlloced( l + 1 );

	if (data)
	{
		for (size_t i = 0; std::cmp_less(i, l); i++) {
			data[i] = cstring_text[start + i];
		}

		data[l] = '\0';
		len = l;
	}
}

ID_INLINE idStr::idStr(const FormattableNoStrings auto& value, size_t start, size_t end) {
	Construct();

	const char* string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring_value = idStr::ToCString(value, string_buffer, sizeof(string_buffer));

	size_t l = strlen(cstring_value);

	end = Min(end, l);
	if (start > l) {
		start = l;
	}
	else if (start < 0) {
		start = 0;
	}

	l = end - start;
	l = Max(l, 0);

	EnsureAlloced(l + 1);

	if (data)
	{
		for (size_t i = 0; std::cmp_less(i, l); i++) {
			data[i] = cstring_value[start + i];
		}

		data[l] = '\0';
		len = l;
	}
}

ID_INLINE idStr::~idStr() {
	FreeData();
}

ID_INLINE size_t idStr::Size() const {
	return sizeof( *this ) + Allocated();
}

ID_INLINE const char *idStr::c_str() const {
	return data;
}

ID_INLINE idStr::operator const char *() {
	return c_str();
}

ID_INLINE idStr::operator const char *() const {
	return c_str();
}


ID_INLINE char idStr::operator[]( const Ordinal auto index ) const {
	ORDINAL_CHECK(index, len + 1);
	return data[ index ];
}


ID_INLINE char &idStr::operator[]( const Ordinal auto index ) {
	ORDINAL_CHECK(index, len + 1);
	return data[ index ];
}

ID_INLINE idStr operator+( const StringLike auto &a, const StringLike auto &b ) {
	idStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idStr &idStr::operator+=( const StringLike auto &a ) {
	Append( a );
	return *this;
}

ID_INLINE idStr &idStr::operator+=( const FormattableNoStrings auto &a ) {
	Append( a );
	return *this;
}

ID_INLINE bool operator==( const idStr& a, const idStr& b ) {
	return (!idStr::Cmp(a.data, b.data));
}

ID_INLINE bool operator==( const idStr& a, const StringLike auto &b ) {
	assert(safe_not_equal(b, nullptr));
	
	char b_string_buffer[MAX_STRING_CHARS] = {};

	return (!idStr::Cmp(a.data, idStr::ToCString(b, b_string_buffer, sizeof(b_string_buffer))));
}

ID_INLINE bool operator==( const idStr &a, const FormattableNoStrings auto &b ) {
	assert(safe_not_equal(b, nullptr));

	char b_string_buffer[MAX_STRING_CHARS] = {};

	return (!idStr::Cmp(a.data, idStr::ToCString(b, b_string_buffer, sizeof(b_string_buffer))));
}

ID_INLINE bool operator==( const StringLike auto &a, const idStr &b ) {
	assert(safe_not_equal(a, nullptr));

	char a_string_buffer[MAX_STRING_CHARS] = {};

	return (!idStr::Cmp(idStr::ToCString(a, a_string_buffer, sizeof(a_string_buffer)), b.data));
}

ID_INLINE bool operator==( const FormattableNoStrings auto &a, const idStr& b ) {
	assert(safe_not_equal(a, nullptr));

	char a_string_buffer[MAX_STRING_CHARS] = {};

	return (!idStr::Cmp(idStr::ToCString(a, a_string_buffer, sizeof(a_string_buffer)), b.data));
}

ID_INLINE bool operator==( const StringLike auto &a, const StringLike auto &b ) {
	assert(safe_not_equal(a, nullptr));
	assert(safe_not_equal(b, nullptr));
	
	char a_string_buffer[MAX_STRING_CHARS] = {};
	char b_string_buffer[MAX_STRING_CHARS] = {};

	return (!idStr::Cmp(idStr::ToCString(a, a_string_buffer, sizeof(a_string_buffer)), idStr::ToCString(b, b_string_buffer, sizeof(b_string_buffer))));
}

ID_INLINE bool operator==( const StringLike auto &a, const FormattableNoStrings auto &b ) {
	assert(safe_not_equal(a, nullptr));
	assert(safe_not_equal(b, nullptr));

	char a_string_buffer[MAX_STRING_CHARS] = {};
	char b_string_buffer[MAX_STRING_CHARS] = {};

	return (!idStr::Cmp(idStr::ToCString(a, a_string_buffer, sizeof(a_string_buffer)), idStr::ToCString(b, b_string_buffer, sizeof(b_string_buffer))));
}

ID_INLINE bool operator==( const FormattableNoStrings auto &a, const StringLike auto &b ) {
	assert(safe_not_equal(a, nullptr));
	assert(safe_not_equal(b, nullptr));

	char a_string_buffer[MAX_STRING_CHARS] = {};
	char b_string_buffer[MAX_STRING_CHARS] = {};

	return (!idStr::Cmp(idStr::ToCString(a, a_string_buffer, sizeof(a_string_buffer)), idStr::ToCString(b, b_string_buffer, sizeof(b_string_buffer))));
}

ID_INLINE bool operator!=( const idStr& a, const idStr& b ) {
	return !operator==(a, b);
}

ID_INLINE bool operator!=( const idStr& a, const StringLike auto& b ) {
	return !operator==(a, b);
}

ID_INLINE bool operator!=( const idStr& a, const FormattableNoStrings auto &b ) {
	return !operator==(a, b);
}

ID_INLINE bool operator!=( const StringLike auto& a, const idStr& b ) {
	return !operator==(a, b);
}

ID_INLINE bool operator!=( const FormattableNoStrings auto &a, const idStr &b ) {
	return !operator==(a, b);
}

ID_INLINE bool operator!=( const StringLike auto &a, const StringLike auto &b ) {
	return !operator==( a, b );
}

ID_INLINE bool operator!=( const StringLike auto &a, const FormattableNoStrings auto &b ) {
	return !operator==( a, b );
}

ID_INLINE bool operator!=( const FormattableNoStrings auto &a, const StringLike auto &b ) {
	return !operator==( a, b );
}

ID_INLINE int idStr::Cmp( const StringLikeOrEnum auto text ) const {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	return idStr::Cmp( data, idStr::ToCString(text, string_buffer, sizeof(string_buffer)) );
}

ID_INLINE int idStr::Cmpn( const StringLikeOrEnum auto text, const size_t n ) const {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	return idStr::Cmpn( data, idStr::ToCString(text, string_buffer, sizeof(string_buffer)), n );
}

ID_INLINE int idStr::CmpPrefix( const StringLikeOrEnum auto text ) const {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring_text = idStr::ToCString(text, string_buffer, sizeof(string_buffer));

	return idStr::Cmpn( data, cstring_text, strlen(cstring_text) );
}

ID_INLINE int idStr::Icmp( const StringLikeOrEnum auto text ) const {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	return idStr::Icmp( data, idStr::ToCString(text, string_buffer, sizeof(string_buffer)));
}

ID_INLINE int idStr::Icmpn( const StringLikeOrEnum auto text, const size_t n ) const {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	return idStr::Icmpn( data, idStr::ToCString(text, string_buffer, sizeof(string_buffer)), n );
}

ID_INLINE int idStr::IcmpPrefix( const StringLikeOrEnum auto text ) const {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring_text = idStr::ToCString(text, string_buffer, sizeof(string_buffer));

	return idStr::Icmpn( data, cstring_text, strlen(cstring_text) );
}

ID_INLINE int idStr::IcmpNoColor( const StringLikeOrEnum auto text ) const {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	return idStr::IcmpNoColor( data, idStr::ToCString(text, string_buffer, sizeof(string_buffer)));
}

ID_INLINE int idStr::IcmpPath( const StringLikeOrEnum auto text ) const {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	return idStr::IcmpPath( data, idStr::ToCString(text, string_buffer, sizeof(string_buffer)));
}

ID_INLINE int idStr::IcmpnPath( const StringLikeOrEnum auto text, const size_t n ) const {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	return idStr::IcmpnPath( data, idStr::ToCString(text, string_buffer, sizeof(string_buffer)), n );
}

ID_INLINE int idStr::IcmpPrefixPath( const StringLikeOrEnum auto text ) const {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring_text = idStr::ToCString(text, string_buffer, sizeof(string_buffer));

	return idStr::IcmpnPath( data, cstring_text, strlen(cstring_text) );
}

ID_INLINE size_t idStr::Length() const {
	return len;
}

ID_INLINE size_t idStr::Allocated() const {
	if ( data != baseBuffer ) {
		return GetAlloced();
	} else {
		return 0;
	}
}

ID_INLINE void idStr::Empty() {
	EnsureAlloced( 1 );
	if (data)
	{
		data[0] = '\0';
		len = 0;
	}
}

ID_INLINE bool idStr::IsEmpty() const {
	return ( idStr::Cmp( data, "" ) == 0 );
}

ID_INLINE void idStr::Clear() {
	if ( IsStatic() ) {
		if (data)
		{
			len = 0;
			data[0] = '\0';
		}
		return;
	}
	FreeData();
	Construct();
}

ID_INLINE void idStr::Append( const char a ) {
	const char char_array[] = { a, '\0' }; // creates a null-terminated string
	const char* string_pointer = char_array;            // p now points to "x"
	this->Append(string_pointer, 1);
}

ID_INLINE void idStr::Append( const idStr &text ) {
	this->Append(text.c_str(), text.Length());
}

ID_INLINE void idStr::Append( const StringLikeOrEnum auto text ) {

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring_text = idStr::ToCString(text, string_buffer, sizeof(string_buffer));
	const size_t cstring_length = strlen(cstring_text);

	this->Append(cstring_text, cstring_length);
}

template < Formattable T >
	requires (!StringLikeOrEnum< T >)
ID_INLINE void idStr::Append(const T &value) {

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring_value = idStr::ToCString(value, string_buffer, sizeof(string_buffer));
	const size_t cstring_length = strlen(cstring_value);

	this->Append(cstring_value, cstring_length);
}

ID_INLINE void idStr::Append( const char * text, const size_t l ) {
	if ( text && l ) {
		const size_t newLen = len + l;

		EnsureAlloced( newLen + 1 );

		if (data)
		{
			for (size_t i = 0; text[i] && i < l; i++) {
				data[len + i] = text[i];
			}
			len = newLen;
			data[len] = '\0';

			current_type = std::type_index(typeid(const char *));
		}
	}
}

ID_INLINE void idStr::Insert(const char * text, Ordinal auto index) {
	assert(safe_not_equal(text, nullptr));

	if (index < 0) {
		index = 0;
	}
	else if (std::cmp_greater(index, len)) {
		index = numeric_cast<BASE_TYPE(index)>(len);
	}

	const size_t l = strlen(text);

	EnsureAlloced(len + l + 1);

	if (data)
	{
		size_t i = 0;

		for (i = len; std::cmp_greater_equal(i, index); i--) {
			data[i + l] = data[i];
		}

		for (i = 0; i < l; i++) {
			data[index + i] = text[i];
		}

		len += l;

		current_type = std::type_index(typeid(const char*));
	}
}

ID_INLINE void idStr::Insert( const StringLikeOrEnum auto &text, Ordinal auto index ) {
	assert(safe_not_equal(text, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	this->Insert(idStr::ToCString(text, string_buffer, sizeof(string_buffer)), index);
}

template < Formattable T >
	requires (!StringLikeOrEnum< T >)
ID_INLINE void idStr::Insert(const T &value, Ordinal auto index) {
	assert(safe_not_equal(value, nullptr));

	char string_buffer[MAX_STRING_CHARS] = {};

	this->Insert(idStr::ToCString(value, string_buffer, sizeof(string_buffer)), index);
}

ID_INLINE void idStr::ToLower() const
{
	if (data)
	{
		for (index_t i = 0; data[i]; ++i) {
			if (CharIsUpper(data[i])) {
				data[i] += ('a' - 'A');
			}
		}
	}
}

ID_INLINE void idStr::ToUpper() const
{
	if (data)
	{
		for (index_t i = 0; data[i]; ++i) {
			if (CharIsLower(data[i])) {
				data[i] -= ('a' - 'A');
			}
		}
	}
}

ID_INLINE bool idStr::IsNumeric() const {
	return idStr::IsNumeric( data );
}

ID_INLINE bool idStr::IsColor() const {
	return idStr::IsColor( data );
}

ID_INLINE bool idStr::HasLower() const {
	return idStr::HasLower( data );
}

ID_INLINE bool idStr::HasUpper() const {
	return idStr::HasUpper( data );
}

ID_INLINE idStr &idStr::RemoveColors() {
	idStr::RemoveColors( data );
	len = Length( data );
	return *this;
}

ID_INLINE size_t idStr::LengthWithoutColors() const {
	return idStr::LengthWithoutColors( data );
}

ID_INLINE void idStr::CapLength(const size_t newlen ) {
	if ( len <= newlen ) {
		return;
	}
	if (data)
	{
		data[newlen] = 0;
		len = newlen;
	}
}

ID_INLINE void idStr::Fill( const char ch, const size_t newlen ) {
	EnsureAlloced( newlen + 1 );
	if (data)
	{
		len = newlen;
		memset(data, ch, len);
		data[len] = 0;
	}
}

/*
========================
idStr::UTF8Length
========================
*/
ID_INLINE size_t idStr::UTF8Length() const
{
	return UTF8Length( reinterpret_cast<byte*>(data) );
}

/*
========================
idStr::UTF8Char
========================
*/

ID_INLINE uint32 idStr::UTF8Char( Ordinal auto& idx ) const
{
	ORDINAL_CHECK(idx, MAX_STRING_CHARS);
	return UTF8Char( reinterpret_cast<byte*>(data), idx );
}

/*
========================
idStr::ConvertToUTF8
========================
*/
ID_INLINE void idStr::ConvertToUTF8() {
	idStr temp( *this );
	Clear();
	for( size_t index = 0; index < temp.Length(); ++index ) {
		AppendUTF8Char( temp[index] );
	}
}

/*
========================
idStr::UTF8Char
========================
*/

ID_INLINE uint32 idStr::UTF8Char( const char * s, Ordinal auto& idx ) {
	ORDINAL_CHECK(idx, MAX_STRING_CHARS);
	return UTF8Char(reinterpret_cast<const byte*>(s), idx );
}

/*
========================
idStr::IsValidUTF8
========================
*/
ID_INLINE bool idStr::IsValidUTF8( const uint8 * s, const size_t maxLen ) {
	utf8Encoding_t encoding = {};
	return IsValidUTF8( s, maxLen, encoding );
}

ID_INLINE index_t idStr::Find( const char c ) const {
	return idStr::FindChar(data, c, 0, -1);
}

ID_INLINE index_t idStr::Find( const char c, const Ordinal auto start, const Ordinal auto end ) const {
	return idStr::FindChar( data, c, start, end);
}

ID_INLINE index_t idStr::Find( const char* text, const bool casesensitive ) const {
	return idStr::FindText(data, text, casesensitive, 0, -1);
}

ID_INLINE index_t idStr::Find( const char* text, const bool casesensitive, const Ordinal auto start ) const {
	return idStr::FindText(data, text, casesensitive, start, -1);
}

ID_INLINE index_t idStr::Find( const char* text, const bool casesensitive, const Ordinal auto start, const Ordinal auto end ) const {
	return idStr::FindText( data, text, casesensitive, start, end );
}

ID_INLINE bool idStr::Filter( const char *filter, const bool casesensitive ) const {
	return idStr::Filter( filter, data, casesensitive );
}

ID_INLINE const char *idStr::Left( const size_t len, idStr &result ) const {
	return Mid( 0, len, result );
}

ID_INLINE const char *idStr::Right( const size_t len, idStr &result ) const {
	if ( len >= Length() ) {
		result = *this;
		return result;
	}
	return Mid( Length() - len, len, result );
}

ID_INLINE idStr idStr::Left( const size_t len ) const {
	return Mid( 0, len );
}

ID_INLINE idStr idStr::Right( const size_t len ) const {
	if ( len >= Length() ) {
		return *this;
	}
	return Mid( Length() - len, len );
}

ID_INLINE void idStr::Strip( const char c ) {
	StripLeading( c );
	StripTrailing( c );
}

ID_INLINE void idStr::Strip( const char *string ) {
	StripLeading( string );
	StripTrailing( string );
}

ID_INLINE bool idStr::CheckExtension( const char *ext ) const
{
	return idStr::CheckExtension( data, ext );
}

ID_INLINE size_t idStr::Length( const char *s ) {
	size_t i;
	for ( i = 0; s[i]; i++ ) {}
	return i;
}

ID_INLINE char *idStr::ToLower( char *s ) {
	for ( size_t i = 0; s[i]; i++ ) {
		if ( CharIsUpper( s[i] ) ) {
			s[i] += ( 'a' - 'A' );
		}
	}
	return s;
}

ID_INLINE char *idStr::ToUpper( char *s ) {
	for ( size_t i = 0; s[i]; i++ ) {
		if ( CharIsLower( s[i] ) ) {
			s[i] -= ( 'a' - 'A' );
		}
	}
	return s;
}

ID_INLINE int idStr::Hash( const StringLikeOrEnum auto& string ) {

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring = idStr::ToCString(string, string_buffer, sizeof(string_buffer));
	const size_t cstring_length = strlen(cstring);

	int hash = 0;
	for ( index_t i = 0; std::cmp_less(i ,cstring_length); ++i ) {
		hash += ( *cstring++ ) * ( numeric_cast<int>(i) + 119 );
	}
	return hash;
}

ID_INLINE int idStr::Hash( const StringLikeOrEnum auto& string, const size_t length ) {

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring = idStr::ToCString(string, string_buffer, sizeof(string_buffer));
	const size_t cstring_length = Min(length, strlen(cstring));

	int hash = 0;
	for (index_t i = 0; std::cmp_less(i, cstring_length); ++i) {
		hash += (*cstring++) * (numeric_cast<int>(i) + 119);
	}
	return hash;
}

ID_INLINE int64 idStr::Hash64(const StringLikeOrEnum auto& string) {

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring = idStr::ToCString(string, string_buffer, sizeof(string_buffer));
	const size_t cstring_length = strlen(cstring);

	int64 hash = 0;
	for (index_t i = 0; std::cmp_less(i, cstring_length); ++i) {
		hash += (*cstring++) * (numeric_cast<int64>(i) + 119);
	}
	return hash;
}

ID_INLINE int64 idStr::Hash64(const StringLikeOrEnum auto& string, const size_t length) {

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring = idStr::ToCString(string, string_buffer, sizeof(string_buffer));
	const size_t cstring_length = Min(length, strlen(cstring));

	int64 hash = 0;
	for (index_t i = 0; std::cmp_less(i, cstring_length); ++i) {
		hash += (*cstring++) * (numeric_cast<int64>(i) + 119);
	}
	return hash;
}

ID_INLINE int idStr::IHash( const StringLikeOrEnum auto& string ) {

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring = idStr::ToCString(string, string_buffer, sizeof(string_buffer));
	const size_t cstring_length = strlen(cstring);

	int hash = 0;
	for (index_t i = 0; std::cmp_less(i, cstring_length); ++i) {
		hash += ToLower(*cstring++) * (numeric_cast<int>(i) + 119);
	}
	return hash;
}

ID_INLINE int idStr::IHash( const StringLikeOrEnum auto& string, const size_t length ) {

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring = idStr::ToCString(string, string_buffer, sizeof(string_buffer));
	const size_t cstring_length = Min(length, strlen(cstring));

	int hash = 0;
	for (index_t i = 0; std::cmp_less(i, cstring_length); ++i) {
		hash += (*cstring++) * (numeric_cast<int>(i) + 119);
	}
	return hash;
}

ID_INLINE int64 idStr::IHash64(const StringLikeOrEnum auto& string) {

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring = idStr::ToCString(string, string_buffer, sizeof(string_buffer));
	const size_t cstring_length = strlen(cstring);

	int64 hash = 0;
	for (index_t i = 0; std::cmp_less(i, cstring_length); ++i) {
		hash += ToLower(*cstring++) * (numeric_cast<int64>(i) + 119);
	}
	return hash;
}

ID_INLINE int64 idStr::IHash64(const StringLikeOrEnum auto& string, const size_t length) {

	char string_buffer[MAX_STRING_CHARS] = {};

	const char* cstring = idStr::ToCString(string, string_buffer, sizeof(string_buffer));
	const size_t cstring_length = Min(length, strlen(cstring));

	int64 hash = 0;
	for (index_t i = 0; std::cmp_less(i, cstring_length); ++i) {
		hash += (*cstring++) * (numeric_cast<int64>(i) + 119);
	}
	return hash;
}

ID_INLINE bool idStr::IsColor( const char *s ) {
	return ( s[0] == C_COLOR_ESCAPE && s[1] != '\0' && s[1] != ' ' );
}

ID_INLINE char idStr::ToLower( const std::integral auto c ) {
	if ( c <= 'Z' && c >= 'A' ) {
		return ( c + ( 'a' - 'A' ) );
	}
	return numeric_cast<char>(c);
}

ID_INLINE char idStr::ToUpper( const std::integral auto c ) {
	if ( c >= 'a' && c <= 'z' ) {
		return ( c - ( 'a' - 'A' ) );
	}
	return numeric_cast<char>(c);
}

ID_INLINE bool idStr::CharIsPrintable( const std::integral auto c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 0x20 && c <= 0x7E ) || ( c >= 0xA1 && c <= 0xFF );
}

ID_INLINE bool idStr::CharIsLower( const std::integral auto c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 'a' && c <= 'z' ) || ( c >= 0xE0 && c <= 0xFF );
}

ID_INLINE bool idStr::CharIsUpper( const std::integral auto c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c <= 'Z' && c >= 'A' ) || ( c >= 0xC0 && c <= 0xDF );
}

ID_INLINE bool idStr::CharIsAlpha( const std::integral auto c ) {
	// test for regular ascii and western European high-ascii chars
	return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
			 ( c >= 0xC0 && c <= 0xFF ) );
}

ID_INLINE bool idStr::CharIsNumeric( const std::integral auto c ) {
	return ( c <= '9' && c >= '0' );
}

ID_INLINE bool idStr::CharIsNewLine( const std::integral auto c ) {
	return ( c == '\n' || c == '\r' || c == '\v' );
}

ID_INLINE bool idStr::CharIsTab( const std::integral auto c ) {
	return ( c == '\t' );
}

ID_INLINE index_t idStr::ColorIndex( const std::integral auto c ) {
	return ( c & 15 );
}

ID_INLINE size_t idStr::DynamicMemoryUsed() const {
	return ( data == baseBuffer ) ? 0 : GetAlloced();
}

/*
========================
idStr::CopyRange
========================
*/
ID_INLINE void idStr::CopyRange( const char * text, const Ordinal auto start, const Ordinal auto end ) {
	ORDINAL_CHECK(start, len);
	ORDINAL_CHECK(end, len);
	assert(std::cmp_less_equal(start, end));
	size_t l = numeric_cast<size_t>(end - start);
	
	EnsureAlloced( l + 1 );
	if (data)
	{
		for (size_t i = 0; i < l; i++) {
			data[i] = text[start + i];
		}

		data[l] = '\0';
		len = l;
	}
}

#endif /* !__STR_H__ */
