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

#include "precompiled.h"
#pragma hdrstop

#ifdef USE_STRING_DATA_ALLOCATOR
static idDynamicBlockAlloc<char, 1<<18, 128, TAG_STRING>	stringDataAllocator;
#endif

static idVec4	g_color_table[16] =
{
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(1.0f, 0.0f, 0.0f, 1.0f), // S_COLOR_RED
	idVec4(0.0f, 1.0f, 0.0f, 1.0f), // S_COLOR_GREEN
	idVec4(1.0f, 1.0f, 0.0f, 1.0f), // S_COLOR_YELLOW
	idVec4(0.0f, 0.0f, 1.0f, 1.0f), // S_COLOR_BLUE
	idVec4(0.0f, 1.0f, 1.0f, 1.0f), // S_COLOR_CYAN
	idVec4(1.0f, 0.5f, 0.0f, 1.0f), // S_COLOR_ORANGE
	idVec4(1.0f, 1.0f, 1.0f, 1.0f), // S_COLOR_WHITE
	idVec4(0.5f, 0.5f, 0.5f, 1.0f), // S_COLOR_GRAY
	idVec4(0.0f, 0.0f, 0.0f, 1.0f), // S_COLOR_BLACK
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
};

static const char *units[2][4] =
{
	{ "B", "KB", "MB", "GB" },
	{ "B/s", "KB/s", "MB/s", "GB/s" }
};

/*
============
idStr::ColorForIndex
============
*/
idVec4 & idStr::ColorForIndex(const Ordinal auto i ) {
	ORDINAL_CHECK(i, 16);
	return g_color_table[ i & 15 ];
}

/*
============
idStr::ReAllocate
============
*/
void idStr::ReAllocate(const size_t amount, const bool keepold ) {
	char	*newbuffer = nullptr;
	size_t	newsize = 0;

	//assert( data );
	assert( amount > 0 );

	const size_t mod = amount % STR_ALLOC_GRAN;
	if ( !mod ) {
		newsize = amount;
	}
	else {
		newsize = amount + STR_ALLOC_GRAN - mod;
	}
	SetAlloced( newsize );

#ifdef USE_STRING_DATA_ALLOCATOR
	newbuffer = stringDataAllocator.Alloc( GetAlloced() );
#else
	newbuffer = new (TAG_STRING) char[ GetAlloced() ]{0};
#endif
	if ( keepold && data ) {
		data[ len ] = '\0';
		strncpy_s( newbuffer, newsize, data, len );
	}

	if ( data && data != baseBuffer ) {
#ifdef USE_STRING_DATA_ALLOCATOR
		stringDataAllocator.Free( data );
#else
		delete [] data;
#endif
	}

	data = newbuffer;
}

/*
============
idStr::FreeData
============
*/
void idStr::FreeData() {
	if ( IsStatic() ) {
		return;
	}

	if ( data && data != baseBuffer ) {
#ifdef USE_STRING_DATA_ALLOCATOR
		stringDataAllocator.Free( data );
#else
		delete[] data;
#endif
		data = baseBuffer;
	}
}

/*
============
idStr::operator=
============
*/
void idStr::operator=( const char *text ) {
	if ( !text ) {
		// safe behavior if NULL
		EnsureAlloced( 1, false );
		data[ 0 ] = '\0';
		len = 0;
		return;
	}

	if ( text == data ) {
		return; // copying same thing
	}

	// check if we're aliasing
	if ( text >= data && text <= data + len ) {
		size_t i = 0;
		const size_t diff = text - data;

		assert( strlen( text ) < (unsigned)len );

		for ( i = 0; text[ i ]; i++ ) {
			data[ i ] = text[ i ];
		}

		data[ i ] = '\0';

		len -= diff;

		return;
	}

	const size_t l = strlen(text);
	EnsureAlloced( l + 1, false );
	strcpy( data, text );
	len = l;
}

/*
============
idStr::FindChar

returns -1 if not found otherwise the index of the char
============
*/
index_t idStr::FindChar(const char* str, const char c) {
	return FindChar(str, c, 0, -1);
}

index_t idStr::FindChar(const char* str, const char c, const Ordinal auto start, const Ordinal auto end) {
	size_t calculated_end = 0;

	if (end < 0 ) 
	{
		calculated_end = strlen(str) - 1;
	}
	else 
	{
		calculated_end = numeric_cast<size_t>(end);
	}

	for ( size_t i = numeric_cast<size_t>(start); std::cmp_less_equal(i, calculated_end); ++i ) {
		if ( str[i] == c ) 
		{
			return numeric_cast<index_t>(i);
		}
	}
	return -1;
}

/*
============
idStr::FindText

returns -1 if not found otherwise the index of the text
============
*/
index_t idStr::FindText(const char* str, const char* text, const bool casesensitive) {
	return FindText(str, text,casesensitive, 0, -1);
}

index_t idStr::FindText(const char* str, const char* text, const bool casesensitive, const Ordinal auto start, const Ordinal auto end) {
	size_t j = 0;
	size_t calculated_end = 0;

	if (end < 0)
	{
		calculated_end = strlen(str) - 1;
	}
	else 
	{
		calculated_end = numeric_cast<size_t>(end);
	}

	const size_t l = calculated_end - strlen(text);
	for (size_t i = numeric_cast<size_t>(start); i <= l; ++i ) {
		if ( casesensitive ) {
			for ( j = 0; text[j]; j++ ) {
				if ( str[i+j] != text[j] ) {
					break;
				}
			}
		} else {
			for ( j = 0; text[j]; j++ ) {
				if ( ::toupper( str[i+j] ) != ::toupper( text[j] ) ) {
					break;
				}
			}
		}
		if ( !text[j] ) {
			return numeric_cast<index_t>(i);
		}
	}
	return -1;
}

/*
============
idStr::Filter

Returns true if the string conforms the given filter.
Several metacharacter may be used in the filter.

*          match any string of zero or more characters
?          match any single character
[abc...]   match any of the enclosed characters; a hyphen can
           be used to specify a range (e.g. a-z, A-Z, 0-9)

============
*/
bool idStr::Filter( const char *filter, const char *name, const bool casesensitive ) {
	idStr buf = {};

	while(*filter) {
		if (*filter == '*') {
			filter++;
			buf.Empty();
			for (size_t i = 0; *filter; i++) {
				if ( *filter == '*' || *filter == '?' || (*filter == '[' && *(filter+1) != '[') ) {
					break;
				}
				buf += *filter;
				if ( *filter == '[' ) {
					filter++;
				}
				filter++;
			}
			if ( buf.Length() ) {
				const index_t index = idStr(name).Find(buf.c_str(), casesensitive);
				if ( index == -1 ) {
					return false;
				}
				name += index + strlen(buf);
			}
		}
		else if (*filter == '?') {
			filter++;
			name++;
		}
		else if (*filter == '[') {
			if ( *(filter+1) == '[' ) {
				if ( *name != '[' ) {
					return false;
				}
				filter += 2;
				name++;
			}
			else {
				filter++;
				bool found = false;
				while(*filter && !found) {
					if (*filter == ']' && *(filter+1) != ']') {
						break;
					}
					if (*(filter+1) == '-' && *(filter+2) && (*(filter+2) != ']' || *(filter+3) == ']')) {
						if (casesensitive) {
							if (*name >= *filter && *name <= *(filter+2)) {
								found = true;
							}
						}
						else {
							if ( ::toupper(*name) >= ::toupper(*filter) && ::toupper(*name) <= ::toupper(*(filter+2)) ) {
								found = true;
							}
						}
						filter += 3;
					}
					else {
						if (casesensitive) {
							if (*filter == *name) {
								found = true;
							}
						}
						else {
							if ( ::toupper(*filter) == ::toupper(*name) ) {
								found = true;
							}
						}
						filter++;
					}
				}
				if (!found) {
					return false;
				}
				while(*filter) {
					if ( *filter == ']' && *(filter+1) != ']' ) {
						break;
					}
					filter++;
				}
				filter++;
				name++;
			}
		}
		else {
			if (casesensitive) {
				if (*filter != *name) {
					return false;
				}
			}
			else {
				if ( ::toupper(*filter) != ::toupper(*name) ) {
					return false;
				}
			}
			filter++;
			name++;
		}
	}
	return true;
}

/*
=============
idStr::StripMediaName

  makes the string lower case, replaces backslashes with forward slashes, and removes extension
=============
*/
void idStr::StripMediaName( const char *name, idStr &mediaName ) {
	mediaName.Empty();

	for ( char c = *name; c; c = *(++name) ) {
		// truncate at an extension
		if ( c == '.' ) {
			break;
		}
		// convert backslashes to forward slashes
		if ( c == '\\' ) {
			mediaName.Append( '/' );
		} else {
			mediaName.Append( idStr::ToLower( c ) );
		}
	}
}

/*
=============
idStr::CheckExtension
=============
*/
bool idStr::CheckExtension( const char *name, const char *ext ) {
	const char *s1 = name + Length( name ) - 1;
	const char *s2 = ext + Length( ext ) - 1;

	do {
		const auto c1 = *s1--;
		const auto c2 = *s2--;

		auto d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return false;
		}
	} while( s1 > name && s2 > ext );

	return ( s1 >= name );
}

/*
=============
idStr::FloatArrayToString
=============
*/
const char *idStr::FloatArrayToString( const float *array, const size_t length, const size_t precision ) {
	static size_t index = 0;
	static char str[4][16384] = {};	// in case called by nested functions
	char format[16] = {};

	// use an array of string so that multiple calls won't collide
	char* s = str[index];
	index = (index + 1) & 3;

	idStr::snPrintf( format, sizeof( format ), "%%.%df", precision );
	index_t n = idStr::snPrintf(s, sizeof(str[0]), format, array[0]);
	if ( precision > 0 ) {
		while( n > 0 && s[n-1] == '0' )
		{
			s[--n] = '\0';
		}
		while( n > 0 && s[n-1] == '.' )
		{
			s[--n] = '\0';
		}
	}
	idStr::snPrintf( format, sizeof( format ), " %%.%df", precision );
	for ( size_t i = 1; i < length; i++ ) {
		n += idStr::snPrintf( s + n, sizeof( str[0] ) - n, format, array[i] );
		if ( precision > 0 ) {
			while( n > 0 && s[n-1] == '0' )
			{
				s[--n] = '\0';
			}
			while( n > 0 && s[n-1] == '.' )
			{
				s[--n] = '\0';
			}
		}
	}
	return s;
}

/*
========================
idStr::CStyleQuote
========================
*/
const char *idStr::CStyleQuote( const char *str ) {
	static index_t index = 0;
	static char buffers[4][16384] = {};	// in case called by nested functions
	size_t i = 0;

	char* buf = buffers[index];
	index = ( index + 1 ) & 3;

	buf[0] = '\"';
	for ( i = 1; i < sizeof( buffers[0] ) - 2; i++ ) {
		const int c = *str++;
		switch( c ) {
			case '\0': buf[i++] = '\"'; buf[i] = '\0'; return buf;
			case '\\': buf[i++] = '\\'; buf[i] = '\\'; break;
			case '\n': buf[i++] = '\\'; buf[i] = 'n'; break;
			case '\r': buf[i++] = '\\'; buf[i] = 'r'; break;
			case '\t': buf[i++] = '\\'; buf[i] = 't'; break;
			case '\v': buf[i++] = '\\'; buf[i] = 'v'; break;
			case '\b': buf[i++] = '\\'; buf[i] = 'b'; break;
			case '\f': buf[i++] = '\\'; buf[i] = 'f'; break;
			case '\a': buf[i++] = '\\'; buf[i] = 'a'; break;
			case '\'': buf[i++] = '\\'; buf[i] = '\''; break;
			case '\"': buf[i++] = '\\'; buf[i] = '\"'; break;
			case '\?': buf[i++] = '\\'; buf[i] = '\?'; break;
			default: buf[i] = c; break;
		}
	}
	buf[i++] = '\"';
	buf[i] = '\0';
	return buf;
}

/*
========================
idStr::CStyleUnQuote
========================
*/
const char *idStr::CStyleUnQuote( const char *str ) {
	if ( str[0] != '\"' ) {
		return str;
	}

	static size_t index = 0;
	static char buffers[4][16384] = {};	// in case called by nested functions
	size_t i = 0;

	char* buf = buffers[index];
	index = ( index + 1 ) & 3;

	str++;
	for ( i = 0; i < sizeof( buffers[0] ) - 1; i++ ) {
		auto c = *str++;
		if ( c == '\0' ) {
			break;
		} else if ( c == '\\' ) {
			c = *str++;
			switch( c ) {
				case '\\': buf[i] = '\\'; break;
				case 'n': buf[i] = '\n'; break;
				case 'r': buf[i] = '\r'; break;
				case 't': buf[i] = '\t'; break;
				case 'v': buf[i] = '\v'; break;
				case 'b': buf[i] = '\b'; break;
				case 'f': buf[i] = '\f'; break;
				case 'a': buf[i] = '\a'; break;
				case '\'': buf[i] = '\''; break;
				case '\"': buf[i] = '\"'; break;
				case '\?': buf[i] = '\?'; break;
			}
		} else {
			buf[i] = c;
		}
	}
	assert( buf[i-1] == '\"' );
	buf[i-1] = '\0';
	return buf;
}

/*
============
idStr::Last

returns -1 if not found otherwise the index of the char
============
*/
index_t idStr::Last( const char c ) const {
	for(size_t i = Length(); i > 0; i-- ) {
		if ( data[ i - 1 ] == c ) {
			return numeric_cast<index_t>(i) - 1;
		}
	}

	return -1;
}

/*
========================
idStr::Format

perform a threadsafe sprintf to the string
========================
*/
void idStr::Format( const char *fmt, ... ) {
	va_list argptr = {};
	char text[MAX_PRINT_MSG] = {};

	va_start( argptr, fmt );
	const auto len = idStr::vsnPrintf( text, sizeof( text ) - 1, fmt, argptr );
	va_end( argptr );
	text[ sizeof( text ) - 1 ] = '\0';

	if ( numeric_cast<size_t>(len) >= sizeof( text ) - 1 ) {
		idLib::common->FatalError( "Tried to set a large buffer using %s", fmt );
	}
	*this = text;
}

/*
========================
idStr::FormatInt

Formats integers with commas for readability.
========================
*/
idStr idStr::FormatInt( const std::integral auto num, const bool isCash ) {
	char text[STR_ALLOC_BASE_NUM] = {};
	
	auto [ptr, ec] = std::to_chars(reinterpret_cast<char*>(&text), reinterpret_cast<char*>(&text) + sizeof(text) - 1, num);
	if (ec == std::errc()) {
		// Success
		*ptr = '\0';  // null terminate
	}
	else
	{
		// Error handling, truncate to empty string
		text[0] = '\0';
		return {};
	}

	idStr val(text);

	const size_t len = val.Length();

	for (size_t i = 0 ; i < ( ( len - 1 ) / 3 ); i++ ) {
		const size_t pos = val.Length() - ( ( i + 1 ) * 3 + i );
		if ( pos > 1 || val[0] != '-' ) {
			val.Insert( ',', pos );
		}
	}

	if ( isCash ) {
		val.Insert( '$', val[0] == '-' ? 1 : 0 );
	}

	return val;
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripLeading( const char c ) {
	while( data[ 0 ] == c ) {
		memmove( &data[ 0 ], &data[ 1 ], len );
		len--;
	}
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripLeading( const char *string ) {
	const size_t l = strlen(string);
	if ( l > 0 ) {
		while ( !Cmpn( string, l ) ) {
			memmove( data, data + l, len - l + 1 );
			len -= l;
		}
	}
}

/*
============
idStr::StripLeadingOnce
============
*/
bool idStr::StripLeadingOnce( const char *string ) {
	const size_t l = strlen(string);
	if ( ( l > 0 ) && !Cmpn( string, l ) ) {
		memmove( data, data + l, len - l + 1 );
		len -= l;
		return true;
	}
	return false;
}

/*
============
idStr::StripTrailing
============
*/
void idStr::StripTrailing( const char c ) {
	for(size_t i = Length(); i > 0 && data[ i - 1 ] == c; i-- ) {
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripTrailing( const char *string ) {
	const size_t l = strlen(string);
	if ( l > 0 ) {
		while ( ( len >= l ) && !Cmpn( string, data + len - l, l ) ) {
			len -= l;
			data[len] = '\0';
		}
	}
}

/*
============
idStr::StripTrailingOnce
============
*/
bool idStr::StripTrailingOnce( const char *string ) {
	const size_t l = strlen(string);
	if ( ( l > 0 ) && ( len >= l ) && !Cmpn( string, data + len - l, l ) ) {
		len -= l;
		data[len] = '\0';
		return true;
	}
	return false;
}

/*
============
idStr::Replace
============
*/
bool  idStr::ReplaceChar( const char old, const char nw ) const
{
	bool replaced = false;
	for ( size_t i = 0; i < Length(); i++ ) {
		if ( data[i] == old ) {
			data[i] = nw;
			replaced = true;
		}
	}
	return replaced;
}

/*
============
idStr::Replace
============
*/
bool idStr::Replace( const char *old, const char *nw ) {
	const size_t oldLen = strlen( old );
	const size_t newLen = strlen( nw );

	// Work out how big the new string will be
	int count = 0;
	for (size_t i = 0; i < Length(); i++ ) {
		if ( idStr::Cmpn( &data[i], old, oldLen ) == 0 ) {
			count++;
			i += oldLen - 1;
		}
	}

	if ( count ) {
		idStr oldString( data );

		EnsureAlloced( len + ( ( newLen - oldLen ) * count ) + 2, false );

		// Replace the old data with the new data
		size_t j = 0;
		for (size_t i = 0; i < oldString.Length(); i++ ) {
			if ( idStr::Cmpn( &oldString[i], old, oldLen ) == 0 ) {
				memcpy( data + j, nw, newLen );
				i += oldLen - 1;
				j += newLen;
			} else {
				data[j] = oldString[i];
				j++;
			}
		}
		data[j] = 0;
		len = strlen( data );
		return true;
	}
	return false;
}

/*
============
idStr::Mid
============
*/
const char *idStr::Mid( const Ordinal auto start, size_t len, idStr &result ) const {
	result.Empty();

	const size_t i = Length();
	if ( i == 0 || len <= 0 || std::cmp_greater_equal(start, i) ) {
		return nullptr;
	}

	if ( start + len >= i ) {
		len = i - start;
	}

	result.Append( &data[ start ], len );
	return result;
}

/*
============
idStr::Mid
============
*/
idStr idStr::Mid( const Ordinal auto start, size_t len ) const {
	idStr result = {};

	const size_t i = Length();
	if ( i == 0 || len <= 0 || std::cmp_greater_equal(start, i) ) {
		return result;
	}

	if ( start + len >= i ) {
		len = i - start;
	}

	result.Append( &data[ start ], len );
	return result;
}

/*
============
idStr::StripTrailingWhitespace
============
*/
void idStr::StripTrailingWhitespace() {
	// cast to unsigned char to prevent stripping off high-ASCII characters
	for( size_t i = Length(); i > 0 && static_cast<unsigned char>(data[i - 1]) <= ' '; i-- ) {
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
idStr::StripQuotes

Removes the quotes from the beginning and end of the string
============
*/
idStr& idStr::StripQuotes ()
{
	if ( data[0] != '\"' )
	{
		return *this;
	}
	
	// Remove the trailing quote first
	if ( data[len-1] == '\"' )
	{
		data[len-1] = '\0';
		len--;
	}

	// Strip the leading quote now
	len--;	
	memmove( &data[ 0 ], &data[ 1 ], len );
	data[len] = '\0';
	
	return *this;
}

/*
=====================================================================

  filename methods

=====================================================================
*/

/*
============
idStr::FileNameHash
============
*/
int idStr::FileNameHash() const {
	long hash = 0;
	int i = 0;
	while( data[i] != '\0' ) {
		char letter = idStr::ToLower(data[i]);
		if ( letter == '.' ) {
			break;				// don't include extension
		}
		if ( letter =='\\' ) {
			letter = '/';
		}
		hash += static_cast<long>(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}

/*
============
idStr::BackSlashesToSlashes
============
*/
idStr &idStr::BackSlashesToSlashes() {
	for ( size_t i = 0; std::cmp_less(i, len); i++ ) {
		if ( data[ i ] == '\\' ) {
			data[ i ] = '/';
		}
	}
	return *this;
}

/*
============
idStr::SlashesToBackSlashes
============
*/
idStr &idStr::SlashesToBackSlashes() {
	for ( size_t i = 0; std::cmp_less(i, len); i++ ) {
		if ( data[ i ] == '/' ) {
			data[ i ] = '\\';
		}
	}
	return *this;
}

/*
============
idStr::SetFileExtension
============
*/
idStr &idStr::SetFileExtension( const char *extension ) {
	StripFileExtension();
	if ( *extension != '.' ) {
		Append( '.' );
	}
	Append( extension );
	return *this;
}

/*
============
idStr::StripFileExtension
============
*/
idStr &idStr::StripFileExtension() {
	for (size_t i = len - 1; i >= 0; i-- ) {
		if ( data[i] == '.' ) {
			data[i] = '\0';
			len = i;
			break;
		}
	}
	return *this;
}

/*
============
idStr::StripAbsoluteFileExtension
============
*/
idStr &idStr::StripAbsoluteFileExtension() {
	for ( size_t i = 0; std::cmp_less(i, len); i++ ) {
		if ( data[i] == '.' ) {
			data[i] = '\0';
			len = i;
			break;
		}
	}

	return *this;
}

/*
==================
idStr::DefaultFileExtension
==================
*/
idStr &idStr::DefaultFileExtension( const char *extension ) {
	// do nothing if the string already has an extension
	for (size_t i = len - 1; i >= 0; i-- ) {
		if ( data[i] == '.' ) {
			return *this;
		}
	}
	if ( *extension != '.' ) {
		Append( '.' );
	}
	Append( extension );
	return *this;
}

/*
==================
idStr::DefaultPath
==================
*/
idStr &idStr::DefaultPath( const char *basepath ) {
	if ( ( ( *this )[ 0 ] == '/' ) || ( ( *this )[ 0 ] == '\\' ) ) {
		// absolute path location
		return *this;
	}

	*this = basepath + *this;
	return *this;
}

/*
====================
idStr::AppendPath
====================
*/
void idStr::AppendPath( const char *text ) {
	int i = 0;

	if ( text && text[i] ) {
		size_t pos = len;
		EnsureAlloced( len + strlen( text ) + 2 );

		if ( pos ) {
			if ( data[ pos-1 ] != '/' ) {
				data[ pos++ ] = '/';
			}
		}
		if ( text[i] == '/' ) {
			i++;
		}

		for ( ; text[ i ]; i++ ) {
			if ( text[ i ] == '\\' ) {
				data[ pos++ ] = '/';
			} else {
				data[ pos++ ] = text[ i ];
			}
		}
		len = pos;
		data[ pos ] = '\0';
	}
}

/*
==================
idStr::StripFilename
==================
*/
idStr &idStr::StripFilename() {
	size_t pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos ] != '/' ) && ( ( *this )[ pos ] != '\\' ) ) {
		pos--;
	}

	if ( pos < 0 ) {
		pos = 0;
	}

	CapLength( pos );
	return *this;
}

/*
==================
idStr::StripPath
==================
*/
idStr &idStr::StripPath() {
	size_t pos = Length();
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	*this = Right( Length() - pos );
	return *this;
}

/*
====================
idStr::ExtractFilePath
====================
*/
void idStr::ExtractFilePath( idStr &dest ) const {
	//
	// back up until a \ or the start
	//
	size_t pos = Length();
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	Left( pos, dest );
}

/*
====================
idStr::ExtractFileName
====================
*/
void idStr::ExtractFileName( idStr &dest ) const {
	//
	// back up until a \ or the start
	//
	size_t pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	Right( Length() - pos, dest );
}

/*
====================
idStr::ExtractFileBase
====================
*/
void idStr::ExtractFileBase( idStr &dest ) const {
	//
	// back up until a \ or the start
	//
	size_t pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	const size_t start = pos;
	while( ( pos < Length() ) && ( ( *this )[ pos ] != '.' ) ) {
		pos++;
	}

	Mid( start, pos - start, dest );
}

/*
====================
idStr::ExtractFileExtension
====================
*/
void idStr::ExtractFileExtension( idStr &dest ) const {
	//
	// back up until a . or the start
	//
	size_t pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '.' ) ) {
		pos--;
	}

	if ( !pos ) {
		// no extension
		dest.Empty();
	} else {
		Right( Length() - pos, dest );
	}
}


/*
=====================================================================

  char * methods to replace library functions

=====================================================================
*/

template <std::floating_point T>
T idStr::AtoF(const char* str) noexcept {
	if (!str)
	{
		return T(0);
	}

	// Skip leading whitespace
	const char* p = str;
	while (*p && std::isspace(static_cast<unsigned char>(*p)))
	{
		++p;
	}

	// Capture optional sign (don't advance past it for the actual parse)
	bool neg_sign = false;
	if (*p == '+' || *p == '-')
	{
		neg_sign = (*p == '-');
	}

	// Check for special tokens after the sign
	const char* t = (*p == '+' || *p == '-') ? p + 1 : p;
	auto ci_eq = [](const char a, const char b) {
		return std::tolower(static_cast<unsigned char>(a)) ==
			std::tolower(static_cast<unsigned char>(b));
		};
	// "inf"
	if (t[0] && t[1] && t[2] && ci_eq(t[0], 'i') && ci_eq(t[1], 'n') && ci_eq(t[2], 'f')) {
		return neg_sign ? -std::numeric_limits<T>::infinity()
			: std::numeric_limits<T>::infinity();
	}
	// "infinity"
	if (t[0] && t[1] && t[2] && t[3] && t[4] && t[5] && t[6] && t[7] &&
		ci_eq(t[0], 'i') && ci_eq(t[1], 'n') && ci_eq(t[2], 'f') &&
		ci_eq(t[3], 'i') && ci_eq(t[4], 'n') && ci_eq(t[5], 'i') &&
		ci_eq(t[6], 't') && ci_eq(t[7], 'y')) {
		return neg_sign ? -std::numeric_limits<T>::infinity()
			: std::numeric_limits<T>::infinity();
	}
	// "nan" (payloads like nan(foo) are accepted by checking only the prefix)
	if (t[0] && t[1] && t[2] && ci_eq(t[0], 'n') && ci_eq(t[1], 'a') && ci_eq(t[2], 'n')) {
		T qn = std::numeric_limits<T>::quiet_NaN();
		return neg_sign ? -qn : qn;
	}

	// Try std::from_chars first (locale-independent, fast)
	{
		T parsed = 0;
		const char* endp = p + std::strlen(p);
		auto res = std::from_chars(p, endp, parsed, std::chars_format::general);
		if (res.ec == std::errc{})
		{
			return parsed; // success
		}
		if (res.ec == std::errc::result_out_of_range) {
			// Overflow: clamp using observed leading sign
			return neg_sign ? -(std::numeric_limits<T>::max)() : (std::numeric_limits<T>::max)();
		}
		// else fall through to strtod for exotic formats (e.g., hex-floats)
	}

	// Fallback: strtod, then cast/clamp to T
	errno = 0;
	char* tail = nullptr;
	double d = std::strtod(p, &tail);
	if (tail == p)
	{
		return T(0); // no conversion
	}
	if (std::isnan(d)) {
		T qn = std::numeric_limits<T>::quiet_NaN();
		return std::signbit(d) ? -qn : qn;      // preserve sign if present
	}
	if (std::isinf(d)) {
		T inf = std::numeric_limits<T>::infinity();
		return std::signbit(d) ? -inf : inf;
	}

	// Clamp on ERANGE overflow; tiny underflow will round toward 0 on cast
	if (errno == ERANGE) {
		constexpr double tmax = static_cast<double>((std::numeric_limits<T>::max)());
		if (std::fabs(d) > tmax)
		{
			return static_cast<T>(d < 0 ? -tmax : tmax);
		}
	}

	// Final safety clamp to T's range
	constexpr double tmax = static_cast<double>((std::numeric_limits<T>::max)());
	if (d > tmax)
	{
		return static_cast<T>(tmax);
	}
	if (d < -tmax)
	{
		return static_cast<T>(-tmax);
	}
	return static_cast<T>(d);
}

template <std::integral T>
T idStr::AtoI(const char* str) noexcept {
	if (!str)
	{
		return T(0);
	}

	// Skip leading whitespace (atoi behavior)
	const char* p = str;
	while (*p && std::isspace(static_cast<unsigned char>(*p)))
	{
		++p;
	}

	// Optional sign
	bool neg = false;
	if (*p == '+' || *p == '-') { neg = (*p == '-'); ++p; }

	// Parse digits using from_chars into the widest unsigned accumulator
	// (we'll apply the sign and clamp to T below).
	unsigned long long u = 0;
	const char* end = p + std::strlen(p);
	auto [ptr, ec] = std::from_chars(p, end, u, 10);

	// No digits parsed → return 0 (like atoi)
	if (ptr == p)
	{
		return T(0);
	}

	// Now clamp to T's range.
	if constexpr (std::is_unsigned_v<T>) {
		// For unsigned targets: negative input clamps to 0; positive overflow clamps to max().
		if (neg)
		{
			return T(0);
		}
		const unsigned long long umax =
			numeric_cast<unsigned long long>((std::numeric_limits<T>::max)());
		return (u > umax) ? (std::numeric_limits<T>::max)() : static_cast<T>(u);
	}
	else {
		// Signed targets:
		// Let Tmax = max(T), TminAbs = |min(T)| = Tmax + 1 (two's complement).
		constexpr unsigned long long tmax_u64 =
			static_cast<unsigned long long>((std::numeric_limits<T>::max)());
		constexpr unsigned long long tmin_abs_u64 = tmax_u64 + 1ULL; // |min|

		if (neg) {
			// If |value| >= |min|, clamp to min (avoid overflow on casting).
			if (u >= tmin_abs_u64)
			{
				return (std::numeric_limits<T>::min)();
			}
			// Safe to negate within signed 64-bit range.
			long long s64 = -static_cast<long long>(u);
			return static_cast<T>(s64);
		}
		else {
			if (u > tmax_u64)
			{
				return (std::numeric_limits<T>::max)();
			}
			long long s64 = static_cast<long long>(u);
			return static_cast<T>(s64);
		}
	}
}

/*
============
idStr::WideToUtf8

Convert a wide string (UTF-16) to UTF-8 in-place, always null-terminates if capacity > 0. Returns the number of bytes written (not counting the null terminator).
============
*/
size_t idStr::WideToUtf8( const wchar_t* src, char* out, const size_t capacity ) noexcept {
	if (!out || capacity == 0)
	{
		return 0;
	}

	out[0] = '\0';

	if (!src)
	{
		return 0;
	}

#if defined (ID_WIN64) || defined(ID_WIN32)
	// --- Windows path ---
	const int needed = WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
	if (needed <= 0)
	{
		return 0;
	}

	// If it fits entirely:
	if (numeric_cast<size_t>(needed) <= capacity) {
		const int written = WideCharToMultiByte(CP_UTF8, 0, src, -1, out, numeric_cast<int>(capacity), nullptr, nullptr);

		return (written > 0) ? numeric_cast<size_t>(written - 1) : 0; // exclude null terminator
	}

	// Otherwise truncate safely
	const int written = WideCharToMultiByte(CP_UTF8, 0, src, -1, out, numeric_cast<int>(capacity), nullptr, nullptr);

	if (written <= 0) {
		out[capacity - 1] = '\0';
		return 0;
	}

	out[capacity - 1] = '\0';

	return capacity - 1;

#else
	// --- POSIX / macOS / Linux path ---
	try {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv = {};
		std::string tmp = conv.to_bytes(src);
		size_t n = tmp.size();
		if (n >= capacity)
		{
			n = capacity - 1; // truncate
		}
		std::memcpy(out, tmp.data(), n);
		out[n] = '\0';
		return n;
	}
	catch (...) {
		out[0] = '\0';
		return 0;
	}
#endif
}

/*
============
idStr::WideCopy

Copy a wide string (UTF-16) to UTF-16
============
*/
size_t idStr::WideCopy( const wchar_t* src, wchar_t* dst, const size_t capacity ) noexcept {
	if (!dst || capacity == 0)
	{
		return 0;
	}
	dst[0] = L'\0';
	if (!src)
	{
		return 0;
	}
#if defined(_MSC_VER)
	// wcsncpy_s always null-terminates up to cap
	wcsncpy_s(dst, capacity, src, _TRUNCATE);
	return wcsnlen(dst, capacity);
#else
	size_t i = 0;
	for (; i + 1 < capacity && src[i] != L'\0'; ++i)
	{
		dst[i] = src[i];
	}
	dst[i] = L'\0';
	return i;
#endif
}


/*
============
idStr::IsNumeric

Checks a string to see if it contains only numerical values.
============
*/
bool idStr::IsNumeric( const char *s ) {
	if ( *s == '-' ) {
		s++;
	}

	bool dot = false;
	for ( size_t i = 0; s[i]; i++ ) {
		if ( !isdigit( static_cast<const unsigned char>(s[i]) ) ) {
			if ( ( s[ i ] == '.' ) && !dot ) {
				dot = true;
				continue;
			}
			return false;
		}
	}

	return true;
}

/*
============
idStr::HasLower

Checks if a string has any lowercase chars
============
*/
bool idStr::HasLower( const char *s ) {
	if ( !s ) {
		return false;
	}
	
	while ( *s ) {
		if ( CharIsLower( *s ) ) {
			return true;
		}
		s++;
	}
	
	return false;
}

/*
============
idStr::HasUpper
	
Checks if a string has any uppercase chars
============
*/
bool idStr::HasUpper( const char *s ) {
	if ( !s ) {
		return false;
	}
	
	while ( *s ) {
		if ( CharIsUpper( *s ) ) {
			return true;
		}
		s++;
	}
	
	return false;
}

/*
================
idStr::Cmp
================
*/
int idStr::Cmp( const char *s1, const char *s2 ) {
	int c1 = 0;

	do {
		c1 = *s1++;
		const int c2 = *s2++;

		const int d = c1 - c2;
		if ( d ) {
			return ( INTEGER_SIGN_BIT_IS_NOT_SET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
idStr::Cmpn
================
*/
int idStr::Cmpn( const char *s1, const char *s2, size_t n ) {
	int c1 = 0;

	assert( n >= 0 );

	do {
		c1 = *s1++;
		const int c2 = *s2++;

		if ( !n-- ) {
			return 0;		// strings are equal until end point
		}

		const int d = c1 - c2;
		if ( d ) {
			return ( INTEGER_SIGN_BIT_IS_NOT_SET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
idStr::Icmp
================
*/
int idStr::Icmp( const char *s1, const char *s2 ) {
	int c1;

	do {
		c1 = *s1++;
		const int c2 = *s2++;

		int d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return ( INTEGER_SIGN_BIT_IS_NOT_SET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
idStr::Icmpn
================
*/
int idStr::Icmpn( const char *s1, const char *s2, size_t n ) {
	int c1 = 0;

	assert( n >= 0 );

	do {
		c1 = *s1++;
		const int c2 = *s2++;

		if ( !n-- ) {
			return 0;		// strings are equal until end point
		}

		int d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return ( INTEGER_SIGN_BIT_IS_NOT_SET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
idStr::Icmp
================
*/
int idStr::IcmpNoColor( const char *s1, const char *s2 ) {
	int c1;

	do {
		while ( idStr::IsColor( s1 ) ) {
			s1 += 2;
		}
		while ( idStr::IsColor( s2 ) ) {
			s2 += 2;
		}
		c1 = *s1++;
		const int c2 = *s2++;

		int d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return ( INTEGER_SIGN_BIT_IS_NOT_SET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
idStr::IcmpPath
================
*/
int idStr::IcmpPath( const char *s1, const char *s2 ) {
	int c1, c2, d;

#if 0
//#if !defined( ID_PC_WIN )
	idLib::common->Printf( "WARNING: IcmpPath used on a case-sensitive filesystem?\n" );
#endif

	do {
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c1 == '\\' ) {
				d += ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 == '\\' ) {
				d -= ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			// make sure folders come first
			while( c1 ) {
				if ( c1 == '/' || c1 == '\\' ) {
					break;
				}
				c1 = *s1++;
			}
			while( c2 ) {
				if ( c2 == '/' || c2 == '\\' ) {
					break;
				}
				c2 = *s2++;
			}
			if ( c1 && !c2 ) {
				return -1;
			} else if ( !c1 && c2 ) {
				return 1;
			}
			// same folder depth so use the regular compare
			return ( INTEGER_SIGN_BIT_IS_NOT_SET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;
}

/*
================
idStr::IcmpnPath
================
*/
int idStr::IcmpnPath( const char *s1, const char *s2, size_t n ) {
	int c1 = 0, c2 = 0, d = 0;

#if 0
//#if !defined( ID_PC_WIN )
	idLib::common->Printf( "WARNING: IcmpPath used on a case-sensitive filesystem?\n" );
#endif

	assert( n >= 0 );

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;		// strings are equal until end point
		}

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c1 == '\\' ) {
				d += ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 == '\\' ) {
				d -= ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			// make sure folders come first
			while( c1 ) {
				if ( c1 == '/' || c1 == '\\' ) {
					break;
				}
				c1 = *s1++;
			}
			while( c2 ) {
				if ( c2 == '/' || c2 == '\\' ) {
					break;
				}
				c2 = *s2++;
			}
			if ( c1 && !c2 ) {
				return -1;
			} else if ( !c1 && c2 ) {
				return 1;
			}
			// same folder depth so use the regular compare
			return ( INTEGER_SIGN_BIT_IS_NOT_SET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;
}

/*
=============
idStr::Copynz
 
Safe strncpy that ensures a trailing zero
=============
*/
void idStr::Copynz( char *dest, const char *src, const size_t destsize ) {
	if ( !src ) {
		idLib::common->Warning( "idStr::Copynz: NULL src" );
		return;
	}
	if ( destsize < 1 ) {
		idLib::common->Warning( "idStr::Copynz: destsize < 1" ); 
		return;
	}

	strncpy( dest, src, destsize-1 );
    dest[destsize-1] = 0;
}

/*
================
idStr::Append

  never goes past bounds or leaves without a terminating 0
================
*/
void idStr::Append( char *dest, const size_t size, const char *src ) {
	const size_t l1 = strlen(dest);
	if ( l1 >= size ) {
		idLib::common->Error( "idStr::Append: already overflowed" );
	}
	idStr::Copynz( dest + l1, src, size - l1 );
}

/*
========================
idStr::IsValidUTF8
========================
*/
bool idStr::IsValidUTF8( const uint8 * s, const size_t maxLen, utf8Encoding_t & encoding ) {
	struct local_t {
		static int GetNumEncodedUTF8Bytes( const uint8 c ) {
			if ( c < 0x80 ) {
				return 1;
			} else if ( ( c >> 5 ) == 0x06 ) {
				// 2 byte encoding - the next byte must begin with
				return 2;
			} else if ( ( c >> 4 ) == 0x0E ) {
				// 3 byte encoding
				return 3;
			} else if ( ( c >> 5 ) == 0x1E ) {
				// 4 byte encoding
				return 4;
			} 
			// this isn't a valid UTF-8 precursor character
			return 0;
		}
		static bool RemainingCharsAreUTF8FollowingBytes(const uint8 * s, const size_t curChar, const size_t maxLen, const size_t num) {
			if ( maxLen - curChar < num ) {
				return false;
			}
			for (size_t i = curChar + 1; i <= curChar + num; i++ ) {
				if ( s[ i ] == '\0' ) {
					return false;
				}
				if ( ( s[ i ] >> 6 ) != 0x02 ) {
					return false;
				}
			}
			return true;
		}
	};

	// check for byte-order-marker
	encoding = UTF8_PURE_ASCII;
	utf8Encoding_t utf8Type = UTF8_ENCODED_NO_BOM;
	if ( maxLen > 3 && s[ 0 ] == 0xEF && s[ 1 ] == 0xBB && s[ 2 ] == 0xBF ) {
		utf8Type = UTF8_ENCODED_BOM;
	}

	for ( size_t i = 0; s[ i ] != '\0' && std::cmp_less(i, maxLen); i++ ) {
		const size_t numBytes = local_t::GetNumEncodedUTF8Bytes( s[ i ] );
		if ( numBytes == 1 ) {
			continue;	// just low ASCII
		} else if ( numBytes == 2 ) {
			// 2 byte encoding - the next byte must begin with bit pattern 10
			if ( !local_t::RemainingCharsAreUTF8FollowingBytes( s, i, maxLen, 1 ) ) {
				return false;
			}
			// skip over UTF-8 character
			i += 1;
			encoding = utf8Type;
		} else if ( numBytes == 3 ) {
			// 3 byte encoding - the next 2 bytes must begin with bit pattern 10
			if ( !local_t::RemainingCharsAreUTF8FollowingBytes( s, i, maxLen, 2 ) ) {
				return false;
			}
			// skip over UTF-8 character
			i += 2;
			encoding = utf8Type;
		} else if ( numBytes == 4 ) {
			// 4 byte encoding - the next 3 bytes must begin with bit pattern 10
			if ( !local_t::RemainingCharsAreUTF8FollowingBytes( s, i, maxLen, 3 ) ) {
				return false;
			}
			// skip over UTF-8 character
			i += 3;
			encoding = utf8Type;
		} else {
			// this isn't a valid UTF-8 character
			if ( utf8Type == UTF8_ENCODED_BOM ) {
				encoding = UTF8_INVALID_BOM;
			} else {
				encoding = UTF8_INVALID;
			}
			return false;
		}
	}
	return true;
}

/*
========================
idStr::UTF8Length
========================
*/
size_t idStr::UTF8Length( const byte * s ) {
	size_t mbLen = 0;
	size_t charLen = 0;
	while ( s[ mbLen ] != '\0' ) {
		const uint32 cindex = s[mbLen];
		if ( cindex < 0x80 ) {
			mbLen++;
		} else {
			size_t trailing = 0;
			if ( cindex >= 0xc0 ) {
				static const byte trailingBytes[ 64 ] = { 
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
				};
				trailing = trailingBytes[ cindex - 0xc0 ];
			}
			mbLen += trailing + 1; 
		}
		charLen++;
	}
	return charLen;
}


/*
========================
idStr::AppendUTF8Char
========================
*/
void idStr::AppendUTF8Char(const uint32 c ) {
	if ( c < 0x80 ) {
		Append( static_cast<char>(c) );
	} else if ( c < 0x800 ) { // 11 bits
		Append( static_cast<char>(0xC0 | (c >> 6)) );
		Append( static_cast<char>(0x80 | (c & 0x3F)) );
	} else if ( c < 0x10000 ) { // 16 bits
		Append( static_cast<char>(0xE0 | (c >> 12)) );
		Append( static_cast<char>(0x80 | ((c >> 6) & 0x3F)) );
		Append( static_cast<char>(0x80 | (c & 0x3F)) );
	} else if ( c < 0x200000 ) {	// 21 bits
		Append( static_cast<char>(0xF0 | (c >> 18)) );
		Append( static_cast<char>(0x80 | ((c >> 12) & 0x3F)) );
		Append( static_cast<char>(0x80 | ((c >> 6) & 0x3F)) );
		Append( static_cast<char>(0x80 | (c & 0x3F)) );
	} else {
		// UTF-8 can encode up to 6 bytes. Why don't we support that?
		// This is an invalid Unicode character
		Append( '?' );
	}
}

/*
========================
idStr::UTF8Char
========================
*/
uint32 idStr::UTF8Char( const byte * s, Ordinal auto& idx ) {
	ORDINAL_CHECK(idx, MAX_STRING_CHARS);

	if ( (idx >= 0) && (s != nullptr) ) {
		while ( s[ idx ] != '\0' ) {
			uint32 cindex = s[ idx ];
			if ( cindex < 0x80 ) {
				++idx;
				return cindex;
			}
			int trailing = 0;
			if ( cindex >= 0xc0 ) {
				static const byte trailingBytes[ 64 ] = { 
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
				};
				trailing = trailingBytes[ cindex - 0xc0 ];
			}
			static const uint32 trailingMask[ 6 ] = { 0x0000007f, 0x0000001f, 0x0000000f, 0x00000007, 0x00000003, 0x00000001 };
			cindex &= trailingMask[ trailing  ];
			while ( trailing-- > 0 ) {
				cindex <<= 6;
				cindex += s[ ++idx ] & 0x0000003f;
			}
			++idx;
			return cindex;
		}
	}
	++idx;
	return 0;	// return a null terminator if out of range
}

/*
================
idStr::LengthWithoutColors
================
*/
size_t idStr::LengthWithoutColors( const char *s ) {
	if ( !s ) {
		return 0;
	}

	size_t len = 0;
	const char* p = s;
	while( *p ) {
		if ( idStr::IsColor( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}

/*
================
idStr::RemoveColors
================
*/
char *idStr::RemoveColors( char *string ) {
	char c = 0;

	const char* s = string;
	char* d = string;
	while( (c = *s) != 0 ) {
		if ( idStr::IsColor( s ) ) {
			s++;
		}		
		else {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

/*
================
idStr::snPrintf
================
*/
index_t idStr::snPrintf( char *dest, const size_t size, const char *fmt, ...) {
	va_list argptr = nullptr;
	char buffer[32000] = {};	// big, but small enough to fit in PPC stack

	va_start( argptr, fmt );
	index_t len = vsprintf(buffer, fmt, argptr);
	va_end( argptr );
	if ( numeric_cast<size_t>(len) >= sizeof( buffer ) ) {
		idLib::common->Error( "idStr::snPrintf: overflowed buffer" );
	}
	if (std::cmp_greater_equal(len, size)) {
		idLib::common->Warning( "idStr::snPrintf: overflow of %i in %i\n", len, size );
		len = numeric_cast<index_t>(size);
	}
	idStr::Copynz( dest, buffer, size );
	return len;
}

/*
============
idStr::vsnPrintf

vsnprintf portability:

C99 standard: vsnprintf returns the number of characters (excluding the trailing
'\0') which would have been written to the final string if enough space had been available
snprintf and vsnprintf do not write more than size bytes (including the trailing '\0')

win32: _vsnprintf returns the number of characters written, not including the terminating null character,
or a negative value if an output error occurs. If the number of characters to write exceeds count, then count 
characters are written and -1 is returned and no trailing '\0' is added.

idStr::vsnPrintf: always appends a trailing '\0', returns number of characters written (not including terminal \0)
or returns -1 on failure or if the buffer would be overflowed.
============
*/
int64 idStr::vsnPrintf( char *dest, const size_t size, const char *fmt, const va_list argptr ) {
	size_t buffer_count = 0;

	if (size > 0)
	{
		buffer_count = size - 1;
	}
#undef _vsnprintf
	const int64 ret = _vsnprintf(dest, buffer_count, fmt, argptr);
#define _vsnprintf	use_idStr_vsnPrintf
	dest[buffer_count] = '\0';
	if ( ret < 0 || std::cmp_greater_equal(ret, size)) {
		return -1;
	}
	return ret;
}

/*
============
sprintf

Sets the value of the string using a printf interface.
============
*/
int64 sprintf( idStr &string, const char *fmt, ... ) {
	va_list argptr = nullptr;
	char buffer[32000] = {};
	
	va_start( argptr, fmt );
	const int64 l = idStr::vsnPrintf(buffer, sizeof(buffer) - 1, fmt, argptr);
	va_end( argptr );
	buffer[sizeof(buffer)-1] = '\0';

	string = buffer;
	return l;
}

/*
============
vsprintf

Sets the value of the string using a vprintf interface.
============
*/
int64 vsprintf( idStr &string, const char *fmt, const va_list argptr ) {
	char buffer[32000] = {};

	const int64 l = idStr::vsnPrintf(buffer, sizeof(buffer) - 1, fmt, argptr);
	buffer[sizeof(buffer)-1] = '\0';
	
	string = buffer;
	return l;
}

/*
============
va

does a varargs printf into a temp buffer
NOTE: not thread safe
============
*/
char *va( const char *fmt, ... ) {
	va_list argptr = nullptr;
	static index_t index = 0;
	static char string[4][16384];	// in case called by nested functions

	char* buf = string[index];
	index = (index + 1) & 3;

	va_start( argptr, fmt );
	vsprintf( buf, fmt, argptr );
	va_end( argptr );

	return buf;
}



/*
============
idStr::BestUnit
============
*/
int idStr::BestUnit( const char *format, float value, const Measure_t measure ) {
	int unit = 1;
	while ( unit <= 3 && ( 1 << ( unit * 10 ) < value ) ) {
		unit++;
	}
	unit--;
	value /= 1 << ( unit * 10 );
	sprintf( *this, format, value );
	*this += " ";
	*this += units[ measure ][ unit ];
	return unit;
}

/*
============
idStr::SetUnit
============
*/
void idStr::SetUnit( const char *format, float value, const int unit, const Measure_t measure ) {
	value /= 1 << ( unit * 10 );
	sprintf( *this, format, value );
	*this += " ";
	*this += units[ measure ][ unit ];	
}

/*
================
idStr::InitMemory
================
*/
void idStr::InitMemory() {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Init();
#endif
}

/*
================
idStr::ShutdownMemory
================
*/
void idStr::ShutdownMemory() {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Shutdown();
#endif
}

/*
================
idStr::PurgeMemory
================
*/
void idStr::PurgeMemory() {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.FreeEmptyBaseBlocks();
#endif
}

/*
================
idStr::ShowMemoryUsage_f
================
*/
void idStr::ShowMemoryUsage_f( const idCmdArgs &args ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	idLib::common->Printf( "%6d KB string memory (%d KB free in %d blocks, %d empty base blocks)\n",
		stringDataAllocator.GetBaseBlockMemory() >> 10, stringDataAllocator.GetFreeBlockMemory() >> 10,
			stringDataAllocator.GetNumFreeBlocks(), stringDataAllocator.GetNumEmptyBaseBlocks() );
#endif
}

/*
================
idStr::FormatNumber
================
*/
struct formatList_t {
	uint64			gran;
	size_t			count;
};

// elements of list need to descend in size
static formatList_t formatList[] = {
	{.gran = 1000000000000000000, .count = 0 },
	{.gran = 1000000000000000,    .count = 0 },
	{.gran = 1000000000000,       .count = 0 },
	{.gran = 1000000000,          .count = 0 },
	{.gran = 1000000,             .count = 0 },
	{.gran = 1000,                .count = 0 }
};

static size_t numFormatList = std::size(formatList);


idStr idStr::FormatNumber( std::integral auto number ) {
	idStr string;
	bool hit = false;

	// reset
	for ( size_t i = 0; i < numFormatList; i++ ) {
		formatList_t *li = formatList + i;
		li->count = 0;
	}

	// main loop
	do {
		hit = false;

		for ( size_t i = 0; i < numFormatList; i++ ) {
			formatList_t *li = formatList + i;

			if ( std::cmp_greater_equal(number, li->gran) ) {
				li->count++;
				number -= li->gran;
				hit = true;
				break;
			}
		}
	} while ( hit );

	// print out
	bool found = false;

	for ( size_t i = 0; i < numFormatList; i++ ) {
		const formatList_t *li = formatList + i;

		if ( li->count ) {
			if ( !found ) {
				string += va( "%i,", li->count );
			} else {
				string += va( "%3.3i,", li->count );
			}
			found = true;
		}
		else if ( found ) {
			string += va( "%3.3i,", li->count );
		}
	}

	if ( found ) {
		string += va( "%3.3i", number );
	}
	else {
		string += va( "%i", number );
	}

	// pad to proper size
	const size_t count = 11 - string.Length();

	for (size_t i = 0; i < count; i++ ) {
		string.Insert( " ", 0 );
	}

	return string;
}

size_t idStr::ItoA(char* buffer, const size_t buffer_size, const std::integral auto value)
{
	assert(buffer && buffer_size > 1);

	auto [ptr, ec] = std::to_chars(buffer, buffer + buffer_size - 1, value, 10);
	if (ec == std::errc()) 
	{
		// No error
		*ptr = '\0';  // null terminate
		return static_cast<size_t>(ptr - buffer);
	}

	// On failure (e.g., buffer too small), null terminate and return 0
	*buffer = '\0';
	return 0;
}

size_t idStr::FtoA(char* buffer, const size_t buffer_size, const std::floating_point auto value, std::chars_format fmt)
{
	assert(buffer && buffer_size > 1);

	// Handle NaN/inf manually, since std::to_chars may not format them portably
	if (std::isnan(value)) {
		constexpr const char* s = "nan";
		size_t size = 3;
		if (size < buffer_size) {
			std::memcpy(buffer, s, size + 1);
			return size;
		}

		// Truncate on failure
		*buffer = '\0';
		return 0;
	}
	if (std::isinf(value)) {
		const char* s = (value > 0) ? "inf" : "-inf";
		size_t size = std::strlen(s);
		if (size < buffer_size) {
			std::memcpy(buffer, s, size + 1);
			return size;
		}

		// Truncate on failure
		*buffer = '\0';
		return 0;
	}

	auto [ptr, ec] = std::to_chars(buffer, buffer + buffer_size - 1, value, fmt);
	if (ec == std::errc()) {
		// No error
		*ptr = '\0';  // null terminate
		return static_cast<size_t>(ptr - buffer);
	}

	// Truncate on failure (e.g., buffer too small)
	*buffer = '\0';
	return 0;
}

CONSOLE_COMMAND( testStrId, "prints a localized string", nullptr ) {
	if ( args.Argc() != 2 ) {
		idLib::Printf( "need a str id like 'STR_SWF_ACCEPT' without the hash, it gets parsed as a separate argument\n" );
		return;
	}

	const idStrId str( va( "#%s", args.Argv( 1 ) ) );
	idLib::Printf( "%s = %s\n", args.Argv( 1 ), str.GetLocalizedString() );
}
