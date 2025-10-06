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

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Unzip.h"

/*
=================
FS_WriteFloatString
=================
*/
int FS_WriteFloatString( char *buf, const char *fmt, va_list argPtr ) {
	long i;
	unsigned long u;
	double f;
	char *str;
	int index;
	idStr tmp, format;

	index = 0;

	while( *fmt ) {
		switch( *fmt ) {
			case '%':
				format = "";
				format += *fmt++;
				while ( (*fmt >= '0' && *fmt <= '9') ||
						*fmt == '.' || *fmt == '-' || *fmt == '+' || *fmt == '#') {
					format += *fmt++;
				}
				format += *fmt;
				switch( *fmt ) {
					case 'f':
					case 'e':
					case 'E':
					case 'g':
					case 'G':
						f = va_arg( argPtr, double );
						if ( format.Length() <= 2 ) {
							// high precision floating point number without trailing zeros
							sprintf( tmp, "%1.10f", f );
							tmp.StripTrailing( '0' );
							tmp.StripTrailing( '.' );
							index += sprintf( buf+index, "%s", tmp.c_str() );
						}
						else {
							index += sprintf( buf+index, format.c_str(), f );
						}
						break;
					case 'd':
					case 'i':
						i = va_arg( argPtr, long );
						index += sprintf( buf+index, format.c_str(), i );
						break;
					case 'u':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'o':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'x':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'X':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'c':
						i = va_arg( argPtr, long );
						index += sprintf( buf+index, format.c_str(), (char) i );
						break;
					case 's':
						str = va_arg( argPtr, char * );
						index += sprintf( buf+index, format.c_str(), str );
						break;
					case '%':
						index += sprintf( buf+index, format.c_str() ); //-V618
						break;
					default:
						common->Error( "FS_WriteFloatString: invalid format %s", format.c_str() );
						break;
				}
				fmt++;
				break;
			case '\\':
				fmt++;
				switch( *fmt ) {
					case 't':
						index += sprintf( buf+index, "\t" );
						break;
					case 'v':
						index += sprintf( buf+index, "\v" );
						break;
					case 'n':
						index += sprintf( buf+index, "\n" );
						break;
					case '\\':
						index += sprintf( buf+index, "\\" );
						break;
					default:
						common->Error( "FS_WriteFloatString: unknown escape character \'%c\'", *fmt );
						break;
				}
				fmt++;
				break;
			default:
				index += sprintf( buf+index, "%c", *fmt );
				fmt++;
				break;
		}
	}

	return index;
}

/*
=================================================================================

idFile

=================================================================================
*/

/*
=================
idFile::GetName
=================
*/
const char *idFile::GetName() const {
	return "";
}

/*
=================
idFile::GetFullPath
=================
*/
const char *idFile::GetFullPath() const {
	return "";
}

/*
=================
idFile::Read
=================
*/
size_t idFile::Read( void *buffer, size_t len ) {
	common->FatalError( "idFile::Read: cannot read from idFile" );
	return 0;
}

/*
=================
idFile::Write
=================
*/
size_t idFile::Write( const void *buffer, size_t len ) {
	common->FatalError( "idFile::Write: cannot write to idFile" );
	return 0;
}

/*
=================
idFile::Length
=================
*/
size_t idFile::Length() const {
	return 0;
}

/*
=================
idFile::Timestamp
=================
*/
ID_TIME_T idFile::Timestamp() const {
	return 0;
}

/*
=================
idFile::Tell
=================
*/
size_t idFile::Tell() const {
	return 0;
}

/*
=================
idFile::ForceFlush
=================
*/
void idFile::ForceFlush() {
}

/*
=================
idFile::Flush
=================
*/
void idFile::Flush() {
}

/*
=================
idFile::Seek
=================
*/
short idFile::Seek(size_t offset, fsOrigin_t origin ) {
	return -1;
}

/*
=================
idFile::Rewind
=================
*/
void idFile::Rewind() {
	Seek( 0, FS_SEEK_SET );
}

/*
=================
idFile::Printf
=================
*/
size_t idFile::Printf( const char *fmt, ... ) {
	char buf[MAX_PRINT_MSG];
	va_list argptr;

	va_start( argptr, fmt );
	size_t length = idStr::vsnPrintf(buf, MAX_PRINT_MSG - 1, fmt, argptr);
	va_end( argptr );

	// so notepad formats the lines correctly
  	idStr	work( buf );
 	work.Replace( "\n", "\r\n" );
  
  	return Write( work.c_str(), work.Length() );
}

/*
=================
idFile::VPrintf
=================
*/
size_t idFile::VPrintf( const char *fmt, const va_list args ) {
	char buf[MAX_PRINT_MSG];

	const size_t length = idStr::vsnPrintf(buf, MAX_PRINT_MSG - 1, fmt, args);
	return Write( buf, length );
}

/*
=================
idFile::WriteFloatString
=================
*/
size_t idFile::WriteFloatString( const char *fmt, ... ) {
	char buf[MAX_PRINT_MSG];
	va_list argPtr;

	va_start( argPtr, fmt );
	const size_t len = FS_WriteFloatString(buf, fmt, argPtr);
	va_end( argPtr );

	return Write( buf, len );
}

/*
 =================
 idFile::ReadInt
 =================
 */
size_t idFile::ReadInt( int &value ) {
	const size_t result = Read( &value, sizeof( value ) );
	value = LittleLong(value);
	return result;
}

/*
 =================
 idFile::ReadUnsignedInt
 =================
 */
size_t idFile::ReadUnsignedInt( unsigned int &value ) {
	const size_t result = Read( &value, sizeof( value ) );
	value = LittleLong(value);
	return result;
}

/*
 =================
 idFile::ReadShort
 =================
 */
size_t idFile::ReadShort( short &value ) {
	const size_t result = Read( &value, sizeof( value ) );
	value = LittleShort(value);
	return result;
}

/*
 =================
 idFile::ReadUnsignedShort
 =================
 */
size_t idFile::ReadUnsignedShort( unsigned short &value ) {
	const size_t result = Read( &value, sizeof( value ) );
	value = LittleShort(value);
	return result;
}

/*
 =================
 idFile::ReadChar
 =================
 */
size_t idFile::ReadChar( char &value ) {
	return Read( &value, sizeof( value ) );
}

/*
 =================
 idFile::ReadUnsignedChar
 =================
 */
size_t idFile::ReadUnsignedChar( unsigned char &value ) {
	return Read( &value, sizeof( value ) );
}

/*
 =================
 idFile::ReadFloat
 =================
 */
size_t idFile::ReadFloat( float &value ) {
	const size_t result = Read( &value, sizeof( value ) );
	value = LittleFloat(value);
	return result;
}

/*
 =================
 idFile::ReadBool
 =================
 */
size_t idFile::ReadBool( bool &value ) {
	unsigned char c;
	const size_t result = ReadUnsignedChar( c );
	value = c ? true : false;
	return result;
}

/*
 =================
 idFile::ReadString
 =================
 */
size_t idFile::ReadString( idStr &string ) {
	int len = 0;
	size_t result = 0;
	
	ReadInt( len );
	if ( len >= 0 ) {
		string.Fill( ' ', len );
		result = Read( &string[ 0 ], len );
	}
	return result;
}

/*
 =================
 idFile::ReadVec2
 =================
 */
size_t idFile::ReadVec2( idVec2 &vec ) {
	const size_t result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );
	return result;
}

/*
 =================
 idFile::ReadVec3
 =================
 */
size_t idFile::ReadVec3( idVec3 &vec ) {
	const size_t result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );
	return result;
}

/*
 =================
 idFile::ReadVec4
 =================
 */
size_t idFile::ReadVec4( idVec4 &vec ) {
	const size_t result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );
	return result;
}

/*
 =================
 idFile::ReadVec6
 =================
 */
size_t idFile::ReadVec6( idVec6 &vec ) {
	const size_t result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );
	return result;
}

/*
 =================
 idFile::ReadMat3
 =================
 */
size_t idFile::ReadMat3( idMat3 &mat ) {
	const size_t result = Read( &mat, sizeof( mat ) );
	LittleRevBytes( &mat, sizeof(float), sizeof(mat)/sizeof(float) );
	return result;
}

/*
 =================
 idFile::WriteInt
 =================
 */
size_t idFile::WriteInt( const int value ) {
	const int v = LittleLong(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteUnsignedInt
 =================
 */
size_t idFile::WriteUnsignedInt( const unsigned int value ) {
	const unsigned int v = LittleLong(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteShort
 =================
 */
size_t idFile::WriteShort( const short value ) {
	const short v = LittleShort(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteUnsignedShort
 =================
 */
size_t idFile::WriteUnsignedShort( const unsigned short value ) {
	const unsigned short v = LittleShort(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteChar
 =================
 */
size_t idFile::WriteChar( const char value ) {
	return Write( &value, sizeof( value ) );
}

/*
 =================
 idFile::WriteUnsignedChar
 =================
 */
size_t idFile::WriteUnsignedChar( const unsigned char value ) {
	return Write( &value, sizeof( value ) );
}

/*
 =================
 idFile::WriteFloat
 =================
 */
size_t idFile::WriteFloat( const float value ) {
	const float v = LittleFloat(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteBool
 =================
 */
size_t idFile::WriteBool( const bool value ) {
	const unsigned char c = value;
	return WriteUnsignedChar( c );
}

/*
 =================
 idFile::WriteString
 =================
 */
size_t idFile::WriteString( const char *value ) {
	const size_t len = strlen( value );
	WriteInt( len );
    return Write( value, len );
}

/*
 =================
 idFile::WriteVec2
 =================
 */
size_t idFile::WriteVec2( const idVec2 &vec ) {
	idVec2 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteVec3
 =================
 */
size_t idFile::WriteVec3( const idVec3 &vec ) {
	idVec3 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteVec4
 =================
 */
size_t idFile::WriteVec4( const idVec4 &vec ) {
	idVec4 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteVec6
 =================
 */
size_t idFile::WriteVec6( const idVec6 &vec ) {
	idVec6 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteMat3
 =================
 */
size_t idFile::WriteMat3( const idMat3 &mat ) {
	idMat3 v = mat;
	LittleRevBytes(&v, sizeof(float), sizeof(v)/sizeof(float) );
	return Write( &v, sizeof( v ) );
}

/*
=================================================================================

idFile_Memory

=================================================================================
*/


/*
=================
idFile_Memory::idFile_Memory
=================
*/
idFile_Memory::idFile_Memory() {
	name = "*unknown*";
	maxSize = 0;
	fileSize = 0;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = nullptr;
	curPtr = nullptr;
}

/*
=================
idFile_Memory::idFile_Memory
=================
*/
idFile_Memory::idFile_Memory( const char *name ) {
	this->name = name;
	maxSize = 0;
	fileSize = 0;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = nullptr;
	curPtr = nullptr;
}

/*
=================
idFile_Memory::idFile_Memory
=================
*/
idFile_Memory::idFile_Memory( const char *name, char *data, const int length ) {
	this->name = name;
	maxSize = length;
	fileSize = 0;
	allocated = length;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = data;
	curPtr = data;
}

/*
=================
idFile_Memory::idFile_Memory
=================
*/
idFile_Memory::idFile_Memory( const char *name, const char *data, const int length ) {
	this->name = name;
	maxSize = 0;
	fileSize = length;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_READ );
	filePtr = const_cast<char *>(data);
	curPtr = const_cast<char *>(data);
}

/*
=================
idFile_Memory::TakeDataOwnership

this also makes the file read only
=================
*/
void idFile_Memory::TakeDataOwnership() {
	if ( filePtr != nullptr && fileSize > 0 ) {
		maxSize = 0;
		mode = ( 1 << FS_READ );
		allocated = fileSize;
	}
}

/*
=================
idFile_Memory::~idFile_Memory
=================
*/
idFile_Memory::~idFile_Memory() {
	if ( filePtr && allocated > 0 && maxSize == 0 ) {
		Mem_Free( filePtr );
	}
}

/*
=================
idFile_Memory::Read
=================
*/
size_t idFile_Memory::Read( void *buffer, size_t len ) {

	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "idFile_Memory::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	if ( curPtr + len > filePtr + fileSize ) {
		len = filePtr + fileSize - curPtr;
	}
	memcpy( buffer, curPtr, len );
	curPtr += len;
	return len;
}

idCVar memcpyImpl( "memcpyImpl", "0", 0, "Which implementation of memcpy to use for idFile_Memory::Write() [0/1 - standard (1 eliminates branch misprediction), 2 - auto-vectorized]" );
void * memcpy2( void * __restrict b, const void * __restrict a, size_t n ) {
	auto s1 = (char *)b;
	auto s2 = (const char *)a;
	for ( ; 0 < n; --n ) {
		*s1++ = *s2++;
	}
	return b;
}

/*
=================
idFile_Memory::Write
=================
*/
idHashTableT< int, int > histogram;
CONSOLE_COMMAND( outputHistogram, "", 0 ) {
	for ( int i = 0; i < histogram.Num(); i++ ) {
		int key;
		histogram.GetIndexKey( i, key );
		const int * value = histogram.GetIndex( i );

		idLib::Printf( "%d\t%d\n", key, *value );
	}
}

CONSOLE_COMMAND( clearHistogram, "", 0 ) {
	histogram.Clear();
}

size_t idFile_Memory::Write( const void *buffer, const size_t len ) {
	if ( len == 0 ) {
		// ~4% falls into this case for some reason...
		return 0;
	}

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "idFile_Memory::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	const int alloc = curPtr + len + 1 - filePtr - allocated; // need room for len+1
	if ( alloc > 0 ) {
		if ( maxSize != 0 ) {
			common->Error( "idFile_Memory::Write: exceeded maximum size %d", maxSize );
			return 0;
		}
		const int extra = granularity * ( 1 + alloc / granularity );
		auto newPtr = (char *) Mem_Alloc( allocated + extra, TAG_IDFILE );
		if ( allocated ) {
			memcpy( newPtr, filePtr, allocated );
		}
		allocated += extra;
		curPtr = newPtr + ( curPtr - filePtr );		
		if ( filePtr ) {
			Mem_Free( filePtr );
		}
		filePtr = newPtr;
	}

	//memcpy( curPtr, buffer, len );
	memcpy2( curPtr, buffer, len );

#if 0
	if ( memcpyImpl.GetInteger() == 0 ) {
		memcpy( curPtr, buffer, len );
	} else if ( memcpyImpl.GetInteger() == 1 ) {
		memcpy( curPtr, buffer, len );
	} else if ( memcpyImpl.GetInteger() == 2 ) {
		memcpy2( curPtr, buffer, len );
	}
#endif

#if 0
	int * value;
	if ( histogram.Get( len, &value ) && value != NULL ) {
		(*value)++;
	} else {
		histogram.Set( len, 1 );
	}
#endif

	curPtr += len;
	fileSize += len;
	filePtr[ fileSize ] = 0; // len + 1
	return len;
}

/*
=================
idFile_Memory::Length
=================
*/
size_t idFile_Memory::Length() const {
	return fileSize;
}

/*
========================
idFile_Memory::SetLength
========================
*/
void idFile_Memory::SetLength(const size_t len ) {
	PreAllocate( len );
	fileSize = len;
}

/*
========================
idFile_Memory::PreAllocate
========================
*/
void idFile_Memory::PreAllocate(const size_t len ) {
	if ( len > allocated ) {
		if ( maxSize != 0 ) {
			idLib::Error( "idFile_Memory::SetLength: exceeded maximum size %d", maxSize );
		}
		auto newPtr = (char *)Mem_Alloc( len, TAG_IDFILE );
		if ( allocated > 0 ) {
			memcpy( newPtr, filePtr, allocated );
		}
		allocated = len;
		curPtr = newPtr + ( curPtr - filePtr );		
		if ( filePtr != nullptr) {
			Mem_Free( filePtr );
		}
		filePtr = newPtr;
	}
}

/*
=================
idFile_Memory::Timestamp
=================
*/
ID_TIME_T idFile_Memory::Timestamp() const {
	return 0;
}

/*
=================
idFile_Memory::Tell
=================
*/
size_t idFile_Memory::Tell() const {
	return ( curPtr - filePtr );
}

/*
=================
idFile_Memory::ForceFlush
=================
*/
void idFile_Memory::ForceFlush() {
}

/*
=================
idFile_Memory::Flush
=================
*/
void idFile_Memory::Flush() {
}

/*
=================
idFile_Memory::Seek

  returns zero on success and -1 on failure
=================
*/
short idFile_Memory::Seek(const size_t offset, const fsOrigin_t origin ) {

	switch( origin ) {
		case FS_SEEK_CUR: {
			curPtr += offset;
			break;
		}
		case FS_SEEK_END: {
			curPtr = filePtr + fileSize - offset;
			break;
		}
		case FS_SEEK_SET: {
			curPtr = filePtr + offset;
			break;
		}
		default: {
			common->FatalError( "idFile_Memory::Seek: bad origin for %s\n", name.c_str() );
			return -1;
		}
	}
	if ( curPtr < filePtr ) {
		curPtr = filePtr;
		return -1;
	}
	if ( curPtr > filePtr + fileSize ) {
		curPtr = filePtr + fileSize;
		return -1;
	}
	return 0;
}

/*
========================
idFile_Memory::SetMaxLength 
========================
*/
void idFile_Memory::SetMaxLength(const size_t len ) {
	const size_t oldLength = fileSize;

	SetLength( len );

	maxSize = len;
	fileSize = oldLength;
}

/*
=================
idFile_Memory::MakeReadOnly
=================
*/
void idFile_Memory::MakeReadOnly() {
	mode = ( 1 << FS_READ );
	Rewind();
}

/*
========================
idFile_Memory::MakeWritable
========================
*/
void idFile_Memory::MakeWritable() {
	mode = ( 1 << FS_WRITE );
	Rewind();
}

/*
=================
idFile_Memory::Clear
=================
*/
void idFile_Memory::Clear(const bool freeMemory ) {
	fileSize = 0;
	granularity = 16384;
	if ( freeMemory ) {
		allocated = 0;
		Mem_Free( filePtr );
		filePtr = nullptr;
		curPtr = nullptr;
	} else {
		curPtr = filePtr;
	}
}

/*
=================
idFile_Memory::SetData
=================
*/
void idFile_Memory::SetData( const char *data, const int length ) {
	maxSize = 0;
	fileSize = length;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_READ );
	filePtr = const_cast<char *>(data);
	curPtr = const_cast<char *>(data);
}

/*
========================
idFile_Memory::TruncateData
========================
*/
void idFile_Memory::TruncateData(const size_t len ) {
	if ( len > allocated ) {
		idLib::Error( "idFile_Memory::TruncateData: len (%d) exceeded allocated size (%d)", len, allocated );
	} else {
		fileSize = len;
	}
}

/*
=================================================================================

idFile_BitMsg

=================================================================================
*/

/*
=================
idFile_BitMsg::idFile_BitMsg
=================
*/
idFile_BitMsg::idFile_BitMsg( idBitMsg &msg ) {
	name = "*unknown*";
	mode = ( 1 << FS_WRITE );
	this->msg = &msg;
}

/*
=================
idFile_BitMsg::idFile_BitMsg
=================
*/
idFile_BitMsg::idFile_BitMsg( const idBitMsg &msg ) {
	name = "*unknown*";
	mode = ( 1 << FS_READ );
	this->msg = const_cast<idBitMsg *>(&msg);
}

/*
=================
idFile_BitMsg::~idFile_BitMsg
=================
*/
idFile_BitMsg::~idFile_BitMsg() {
}

/*
=================
idFile_BitMsg::Read
=================
*/
size_t idFile_BitMsg::Read( void *buffer, const size_t len ) {

	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "idFile_BitMsg::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	return msg->ReadData( buffer, len );
}

/*
=================
idFile_BitMsg::Write
=================
*/
size_t idFile_BitMsg::Write( const void *buffer, const size_t len ) {

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "idFile_Memory::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	msg->WriteData( buffer, len );
	return len;
}

/*
=================
idFile_BitMsg::Length
=================
*/
size_t idFile_BitMsg::Length() const {
	return msg->GetSize();
}

/*
=================
idFile_BitMsg::Timestamp
=================
*/
ID_TIME_T idFile_BitMsg::Timestamp() const {
	return 0;
}

/*
=================
idFile_BitMsg::Tell
=================
*/
size_t idFile_BitMsg::Tell() const {
	if ( mode == FS_READ ) {
		return msg->GetReadCount();
	} else {
		return msg->GetSize();
	}
}

/*
=================
idFile_BitMsg::ForceFlush
=================
*/
void idFile_BitMsg::ForceFlush() {
}

/*
=================
idFile_BitMsg::Flush
=================
*/
void idFile_BitMsg::Flush() {
}

/*
=================
idFile_BitMsg::Seek

  returns zero on success and -1 on failure
=================
*/
short idFile_BitMsg::Seek(size_t offset, fsOrigin_t origin ) {
	return -1;
}


/*
=================================================================================

idFile_Permanent

=================================================================================
*/

/*
=================
idFile_Permanent::idFile_Permanent
=================
*/
idFile_Permanent::idFile_Permanent() {
	name = "invalid";
	o = nullptr;
	mode = 0;
	fileSize = 0;
	handleSync = false;
}

/*
=================
idFile_Permanent::~idFile_Permanent
=================
*/
idFile_Permanent::~idFile_Permanent() {
	if ( o ) {
		CloseHandle( o );
	}
}

/*
=================
idFile_Permanent::Read

Properly handles partial reads
=================
*/
size_t idFile_Permanent::Read( void *buffer, const size_t len ) {
	if ( !(mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "idFile_Permanent::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	if ( !o ) {
		return 0;
	}

	auto buf = static_cast<byte*>(buffer);

	size_t remaining = len;
	unsigned short tries = 0;
	while( remaining ) {
		const size_t block = remaining;
		DWORD bytesRead;
		if ( !ReadFile( o, buf, block, &bytesRead, nullptr) ) {
			idLib::Warning( "idFile_Permanent::Read failed with %d from %s", GetLastError(), name.c_str() );
		}
		const int read = bytesRead;
		if ( read == 0 ) {
			// we might have been trying to read from a CD, which
			// sometimes returns a 0 read on windows
			if ( !tries ) {
				tries = 1;
			}
			else {
				return len-remaining;
			}
		}

		if ( read == -1 ) {
			common->FatalError( "idFile_Permanent::Read: -1 bytes read from %s", name.c_str() );
		}

		remaining -= read;
		buf += read;
	}
	return len;
}

/*
=================
idFile_Permanent::Write

Properly handles partial writes
=================
*/
size_t idFile_Permanent::Write( const void *buffer, const size_t len ) {
	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "idFile_Permanent::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	if ( !o ) {
		return 0;
	}

	auto buf = static_cast<const byte*>(buffer);

	size_t remaining = len;
	unsigned short tries = 0;
	while( remaining ) {
		const size_t block = remaining;
		DWORD bytesWritten;
		WriteFile( o, buf, block, &bytesWritten, nullptr);
		const size_t written = bytesWritten;
		if ( written == 0 ) {
			if ( !tries ) {
				tries = 1;
			}
			else {
				common->Printf( "idFile_Permanent::Write: 0 bytes written to %s\n", name.c_str() );
				return 0;
			}
		}

		if ( written == -1 ) {
			common->Printf( "idFile_Permanent::Write: -1 bytes written to %s\n", name.c_str() );
			return 0;
		}

		remaining -= written;
		buf += written;
		fileSize += written;
	}
	if ( handleSync ) {
		Flush();
	}
	return len;
}

/*
=================
idFile_Permanent::ForceFlush
=================
*/
void idFile_Permanent::ForceFlush() {
	FlushFileBuffers( o );
}

/*
=================
idFile_Permanent::Flush
=================
*/
void idFile_Permanent::Flush() {
	FlushFileBuffers( o );
}

/*
=================
idFile_Permanent::Tell
=================
*/
size_t idFile_Permanent::Tell() const {
	return SetFilePointer( o, 0, nullptr, FILE_CURRENT );
}

/*
================
idFile_Permanent::Length
================
*/
size_t idFile_Permanent::Length() const {
	return fileSize;
}

/*
================
idFile_Permanent::Timestamp
================
*/
ID_TIME_T idFile_Permanent::Timestamp() const {
	ID_TIME_T ts = Sys_FileTimeStamp( o );
	return ts;
}

/*
=================
idFile_Permanent::Seek

  returns zero on success and -1 on failure
=================
*/
short idFile_Permanent::Seek(const size_t offset, const fsOrigin_t origin ) {
	DWORD retVal = INVALID_SET_FILE_POINTER;
	switch( origin ) {
		case FS_SEEK_CUR: retVal = SetFilePointer( o, offset, nullptr, FILE_CURRENT ); break;
		case FS_SEEK_END: retVal = SetFilePointer( o, offset, nullptr, FILE_END ); break;
		case FS_SEEK_SET: retVal = SetFilePointer( o, offset, nullptr, FILE_BEGIN ); break;
	}
	return ( retVal == INVALID_SET_FILE_POINTER ) ? -1 : 0;
}

#if 1
/*
=================================================================================

idFile_Cached

=================================================================================
*/

/*
=================
idFile_Cached::idFile_Cached
=================
*/
idFile_Cached::idFile_Cached() : idFile_Permanent() {
	internalFilePos = 0;
	bufferedStartOffset = 0;
	bufferedEndOffset = 0;
	buffered = nullptr;
}

/*
=================
idFile_Cached::~idFile_Cached
=================
*/
idFile_Cached::~idFile_Cached() {
	Mem_Free( buffered );
}

/*
=================
idFile_ReadBuffered::BufferData

Buffer a section of the file
=================
*/
void idFile_Cached::CacheData(const uint64 offset, const uint64 length ) {
	Mem_Free( buffered );
	bufferedStartOffset = offset;
	bufferedEndOffset = offset + length;
	buffered = ( byte* )Mem_Alloc( length, TAG_RESOURCE );
	if ( buffered == nullptr) {
		return;
	}
	int internalFilePos = idFile_Permanent::Tell();
	idFile_Permanent::Seek( offset, FS_SEEK_SET );
	idFile_Permanent::Read( buffered, length );
	idFile_Permanent::Seek( internalFilePos, FS_SEEK_SET );
}

/*
=================
idFile_ReadBuffered::Read

=================
*/
size_t idFile_Cached::Read( void *buffer, const size_t len ) {
	if ( internalFilePos >= bufferedStartOffset && internalFilePos + len < bufferedEndOffset ) {
		// this is in the buffer
		memcpy( buffer, (void*)&buffered[ internalFilePos - bufferedStartOffset ], len );
		internalFilePos += len;
		return len;
	}
	const int read = idFile_Permanent::Read( buffer, len );
	if ( read != -1 ) {
		internalFilePos += static_cast<int64>(read);
	}
	return read;
}



/*
=================
idFile_Cached::Tell
=================
*/
size_t idFile_Cached::Tell() const {
	return internalFilePos;
}

/*
=================
idFile_Cached::Seek

  returns zero on success and -1 on failure
=================
*/
short idFile_Cached::Seek(const size_t offset, const fsOrigin_t origin ) {
	if ( origin == FS_SEEK_SET && offset >= bufferedStartOffset && offset < bufferedEndOffset ) {
		// don't do anything to the actual file ptr, just update or internal position
		internalFilePos = offset;
		return 0;
	}

	const int retVal = idFile_Permanent::Seek( offset, origin );
	internalFilePos = idFile_Permanent::Tell();
	return retVal;
}
#endif

/*
=================================================================================

idFile_InZip

=================================================================================
*/

/*
=================
idFile_InZip::idFile_InZip
=================
*/
idFile_InZip::idFile_InZip() {
	name = "invalid";
	zipFilePos = 0;
	fileSize = 0;
	memset( &z, 0, sizeof( z ) );
}

/*
=================
idFile_InZip::~idFile_InZip
=================
*/
idFile_InZip::~idFile_InZip() {
	unzCloseCurrentFile( z );
	unzClose( z );
}

/*
=================
idFile_InZip::Read

Properly handles partial reads
=================
*/
size_t idFile_InZip::Read( void *buffer, const size_t len ) {
	const int l = unzReadCurrentFile( z, buffer, len );
	return l;
}

/*
=================
idFile_InZip::Write
=================
*/
size_t idFile_InZip::Write( const void *buffer, size_t len ) {
	common->FatalError( "idFile_InZip::Write: cannot write to the zipped file %s", name.c_str() );
	return 0;
}

/*
=================
idFile_InZip::ForceFlush
=================
*/
void idFile_InZip::ForceFlush() {
	common->FatalError( "idFile_InZip::ForceFlush: cannot flush the zipped file %s", name.c_str() );
}

/*
=================
idFile_InZip::Flush
=================
*/
void idFile_InZip::Flush() {
	common->FatalError( "idFile_InZip::Flush: cannot flush the zipped file %s", name.c_str() );
}

/*
=================
idFile_InZip::Tell
=================
*/
size_t idFile_InZip::Tell() const {
	return unztell( z );
}

/*
================
idFile_InZip::Length
================
*/
size_t idFile_InZip::Length() const {
	return fileSize;
}

/*
================
idFile_InZip::Timestamp
================
*/
ID_TIME_T idFile_InZip::Timestamp() const {
	return 0;
}

/*
=================
idFile_InZip::Seek

  returns zero on success and -1 on failure
=================
*/
#define ZIP_SEEK_BUF_SIZE	(1<<15)

short idFile_InZip::Seek(size_t offset, const fsOrigin_t origin ) {
	char *buf = nullptr;

	switch( origin ) {
		case FS_SEEK_END: {
			offset = fileSize - offset;
		}
		case FS_SEEK_SET: {
			// set the file position in the zip file (also sets the current file info)
			unzSetCurrentFileInfoPosition( z, zipFilePos );
			unzOpenCurrentFile( z );
			if ( offset <= 0 ) {
				return 0;
			}
		}
		case FS_SEEK_CUR: {
			size_t i = 0;
			size_t res = 0;
			buf = static_cast<char*>(_alloca16(ZIP_SEEK_BUF_SIZE));
			for ( i = 0; i < ( offset - ZIP_SEEK_BUF_SIZE ); i += ZIP_SEEK_BUF_SIZE ) {
				res = unzReadCurrentFile( z, buf, ZIP_SEEK_BUF_SIZE );
				if ( res < ZIP_SEEK_BUF_SIZE ) {
					return -1;
				}
			}
			res = i + unzReadCurrentFile( z, buf, offset - i );
			return ( res == offset ) ? 0 : -1;
		}
		default: {
			common->FatalError( "idFile_InZip::Seek: bad origin for %s\n", name.c_str() );
			break;
		}
	}
	return -1;
}

#if 1

/*
=================================================================================

idFile_InnerResource

=================================================================================
*/

/*
=================
idFile_InnerResource::idFile_InnerResource
=================
*/
idFile_InnerResource::idFile_InnerResource( const char *_name, idFile *rezFile, const int _offset, const int _len ) {
	name = _name;
	offset = _offset;
	length = _len;
	resourceFile = rezFile;
	internalFilePos = 0;
	resourceBuffer = nullptr;
}

/*
=================
idFile_InnerResource::~idFile_InnerResource
=================
*/
idFile_InnerResource::~idFile_InnerResource() {
	if ( resourceBuffer != nullptr) {
		fileSystem->FreeResourceBuffer();
	}
}

/*
=================
idFile_InnerResource::Read

Properly handles partial reads
=================
*/
size_t idFile_InnerResource::Read( void *buffer, size_t len ) {
	if ( resourceFile == nullptr) {
		return 0;
	}

	if ( internalFilePos + len > length ) {
		len = length - internalFilePos;
	}

	int read = 0; //fileSystem->ReadFromBGL( resourceFile, (byte*)buffer, offset + internalFilePos, len );

	if ( read != len ) {
		if ( resourceBuffer != nullptr) {
			memcpy( buffer, &resourceBuffer[ internalFilePos ], len );
			read = len;
		} else {
			read = fileSystem->ReadFromBGL( resourceFile, buffer, offset + internalFilePos, len );
		}
	}

	internalFilePos += read;

	return read;
}

/*
=================
idFile_InnerResource::Tell
=================
*/
size_t idFile_InnerResource::Tell() const {
	return internalFilePos;
}


/*
=================
idFile_InnerResource::Seek

  returns zero on success and -1 on failure
=================
*/

short idFile_InnerResource::Seek(const size_t offset, const fsOrigin_t origin ) {
	switch( origin ) {
		case FS_SEEK_END: {
			internalFilePos = length - offset - 1;
			return 0;
		}
		case FS_SEEK_SET: {
			internalFilePos = offset;
			if ( internalFilePos >= 0 && internalFilePos < length ) {
				return 0;
			}
			return -1;
		}
		case FS_SEEK_CUR: {
			internalFilePos += offset;
			if ( internalFilePos >= 0 && internalFilePos < length ) {
				return 0;
			}
			return -1;
		}
		default: {
			common->FatalError( "idFile_InnerResource::Seek: bad origin for %s\n", name.c_str() );
			break;
		}
	}
	return -1;
}
#endif

/*
================================================================================================

idFileLocal

================================================================================================
*/

/*
========================
idFileLocal::~idFileLocal

Destructor that will destroy (close) the managed file when this wrapper class goes out of scope.
========================
*/
idFileLocal::~idFileLocal() {
	if ( file != nullptr) {
		delete file;
		file = nullptr;
	}
}

static auto testEndianNessFilename = "temp.bin";
struct testEndianNess_t {
	testEndianNess_t() {
		a = 0x12345678;
		b = 0x12345678;
		c = 3.0f;
		d = -4.0f;
		e = "test";
		f = idVec3( 1.0f, 2.0f, -3.0f );
		g = false;
		h = true;
		for ( int index = 0; index < sizeof( i ); index++ ) {
			i[index] = 0x37;
		}
	}
	bool operator==( const testEndianNess_t & test ) const {
		return a == test.a &&
			b == test.b &&
			c == test.c &&
			d == test.d &&
			e == test.e &&
			f == test.f &&
			g == test.g &&
			h == test.h &&
			( memcmp( i, test.i, sizeof( i ) ) == 0 );
	}
	int				a;
	unsigned int	b;
	float			c;
	float			d;
	idStr			e;
	idVec3			f;
	bool			g;
	bool			h;
	byte			i[10];
};
CONSOLE_COMMAND( testEndianNessWrite, "Tests the read/write compatibility between platforms", 0 ) {
	const idFileLocal file( fileSystem->OpenFileWrite( testEndianNessFilename ) );
	if ( file == nullptr) {
		idLib::Printf( "Couldn't open the %s testfile.\n", testEndianNessFilename );
		return;
	}

	testEndianNess_t testData;

	file->WriteBig( testData.a );
	file->WriteBig( testData.b );
	file->WriteFloat( testData.c );
	file->WriteFloat( testData.d );
	file->WriteString( testData.e );
	file->WriteVec3( testData.f );
	file->WriteBig( testData.g );
	file->WriteBig( testData.h );
	file->Write( testData.i, sizeof( testData.i )/ sizeof( testData.i[0] ) );
}

CONSOLE_COMMAND( testEndianNessRead, "Tests the read/write compatibility between platforms", 0 ) {
	const idFileLocal file( fileSystem->OpenFileRead( testEndianNessFilename ) );
	if ( file == nullptr) {
		idLib::Printf( "Couldn't find the %s testfile.\n", testEndianNessFilename );
		return;
	}

	const testEndianNess_t srcData;
	testEndianNess_t testData;

	memset( &testData, 0, sizeof( testData ) );

	file->ReadBig( testData.a );
	file->ReadBig( testData.b );
	file->ReadFloat( testData.c );
	file->ReadFloat( testData.d );
	file->ReadString( testData.e );
	file->ReadVec3( testData.f );
	file->ReadBig( testData.g );
	file->ReadBig( testData.h );
	file->Read( testData.i, sizeof( testData.i )/ sizeof( testData.i[0] ) );

	assert( srcData == testData );
}

CONSOLE_COMMAND( testEndianNessReset, "Tests the read/write compatibility between platforms", 0 ) {
	fileSystem->RemoveFile( testEndianNessFilename );
}


