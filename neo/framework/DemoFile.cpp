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

idCVar idDemoFile::com_logDemos( "com_logDemos", "0", CVAR_SYSTEM | CVAR_BOOL, "Write demo.log with debug information in it" );
idCVar idDemoFile::com_compressDemos( "com_compressDemos", "1", CVAR_SYSTEM | CVAR_INTEGER | CVAR_ARCHIVE, "Compression scheme for demo files\n0: None    (Fast, large files)\n1: LZW     (Fast to compress, Fast to decompress, medium/small files)\n2: LZSS    (Slow to compress, Fast to decompress, small files)\n3: Huffman (Fast to compress, Slow to decompress, medium files)\nSee also: The 'CompressDemo' command" );
idCVar idDemoFile::com_preloadDemos( "com_preloadDemos", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_ARCHIVE, "Load the whole demo in to RAM before running it" );

#define DEMO_MAGIC concat(GAME_NAME, " RDEMO")

/*
================
idDemoFile::idDemoFile
================
*/
idDemoFile::idDemoFile() {
	f = nullptr;
	fLog = nullptr;
	log = false;
	fileImage = nullptr;
	compressor = nullptr;
	writing = false;
}

/*
================
idDemoFile::~idDemoFile
================
*/
idDemoFile::~idDemoFile() {
	Close();
}

/*
================
idDemoFile::AllocCompressor
================
*/
idCompressor *idDemoFile::AllocCompressor(const int type ) {
	switch ( type ) {
	case 0: return idCompressor::AllocNoCompression();
	default:
	case 1: return idCompressor::AllocLZW();
	case 2: return idCompressor::AllocLZSS();
	case 3: return idCompressor::AllocHuffman();
	}
}

/*
================
idDemoFile::OpenForReading
================
*/
bool idDemoFile::OpenForReading( const char *fileName ) {
	static constexpr int magicLen = sizeof(DEMO_MAGIC) / sizeof(DEMO_MAGIC[0]);
	char magicBuffer[magicLen];
	int compression;

	Close();

	f = fileSystem->OpenFileRead( fileName );
	if ( !f ) {
		return false;
	}

	int fileLength = f->Length();

	if ( com_preloadDemos.GetBool() ) {
		fileImage = static_cast<byte*>(Mem_Alloc(fileLength, TAG_CRAP));
		f->Read( fileImage, fileLength );
		fileSystem->CloseFile( f );
		f = new (TAG_SYSTEM) idFile_Memory( va( "preloaded(%s)", fileName ), (const char *)fileImage, fileLength );
	}

	if ( com_logDemos.GetBool() ) {
		fLog = fileSystem->OpenFileWrite( "demoread.log" );
	}

	writing = false;

	f->Read(magicBuffer, magicLen);
	if ( memcmp(magicBuffer, DEMO_MAGIC, magicLen) == 0 ) {
		f->ReadInt( compression );
	} else {
		// Ideally we would error out if the magic string isn't there,
		// but for backwards compatibility we are going to assume it's just an uncompressed demo file
		compression = 0;
		f->Rewind();
	}

	compressor = AllocCompressor( compression );
	compressor->Init( f, false, 8 );

	return true;
}

/*
================
idDemoFile::SetLog
================
*/
void idDemoFile::SetLog(const bool b, const char *p) {
	log = b;
	if (p) {
		logStr = p;
	}
}

/*
================
idDemoFile::Log
================
*/
void idDemoFile::Log(const char *p) const
{
	if ( fLog && p && *p ) {
		fLog->Write( p, strlen(p) );
	}
}

/*
================
idDemoFile::OpenForWriting
================
*/
bool idDemoFile::OpenForWriting( const char *fileName ) {
	Close();

	f = fileSystem->OpenFileWrite( fileName );
	if ( f == nullptr) {
		return false;
	}

	if ( com_logDemos.GetBool() ) {
		fLog = fileSystem->OpenFileWrite( "demowrite.log" );
	}

	writing = true;

	f->Write(DEMO_MAGIC, sizeof(DEMO_MAGIC));
	f->WriteInt( com_compressDemos.GetInteger() );
	f->Flush();

	compressor = AllocCompressor( com_compressDemos.GetInteger() );
	compressor->Init( f, true, 8 );

	return true;
}

/*
================
idDemoFile::Close
================
*/
void idDemoFile::Close() {
	if ( writing && compressor ) {
		compressor->FinishCompress();
	}

	if ( f ) {
		fileSystem->CloseFile( f );
		f = nullptr;
	}
	if ( fLog ) {
		fileSystem->CloseFile( fLog );
		fLog = nullptr;
	}
	if ( fileImage ) {
		Mem_Free( fileImage );
		fileImage = nullptr;
	}
	if ( compressor ) {
		delete compressor;
		compressor = nullptr;
	}

	demoStrings.DeleteContents( true );
}

/*
================
idDemoFile::ReadHashString
================
*/
const char *idDemoFile::ReadHashString() {
	index_t	 index = -1;

	if ( log && fLog ) {
		const char *text = va( "%s > Reading hash string\n", logStr.c_str() );
		fLog->Write( text, strlen( text ) );
	} 

	ReadInt64( index );

	if ( index == -1 ) {
		// read a new string for the table
		idStr	*str = new (TAG_SYSTEM) idStr;
		
		idStr data;
		ReadString( data );
		*str = data;
		
		demoStrings.Append( str );

		return *str;
	}

	if ( index < -1 || index >= demoStrings.Num() ) {
		Close();
		common->Error( "demo hash index out of range" );
	}

	return demoStrings[index]->c_str();
}

/*
================
idDemoFile::WriteHashString
================
*/
void idDemoFile::WriteHashString( const char *str ) {
	if ( log && fLog ) {
		const char *text = va( "%s > Writing hash string\n", logStr.c_str() );
		fLog->Write( text, strlen( text ) );
	}
	// see if it is already in the has table
	for ( index_t i = 0 ; i < demoStrings.Num() ; i++ ) {
		if ( !strcmp( demoStrings[i]->c_str(), str ) ) {
			WriteInt64( i );
			return;
		}
	}

	// add it to our table and the demo table
	idStr	*copy = new (TAG_SYSTEM) idStr( str );
//common->Printf( "hash:%i = %s\n", demoStrings.Num(), str );
	demoStrings.Append( copy );
	int cmd = -1;	
	WriteInt( cmd );
	WriteString( str );
}

/*
================
idDemoFile::ReadDict
================
*/
template<class T>
void idDemoFile::ReadDict( idDict<T> &dict ) {
	int c = 0;

	dict.Clear();
	ReadInt( c );
	for ( index_t i = 0; i < c; i++ ) {
		idStr key = ReadHashString();
		idStr val = ReadHashString();
		dict.Set( key, val );
	}
}

/*
================
idDemoFile::WriteDict
================
*/
template<class T>
void idDemoFile::WriteDict( const idDict<T> &dict ) {
	const size_t c = dict.GetNumKeyVals();
	WriteInt( c );
	for ( int i = 0; i < c; i++ ) {
		WriteHashString( dict.GetKeyVal( i )->GetKey() );
		WriteHashString( dict.GetKeyVal( i )->GetValue() );
	}
}

/*
 ================
 idDemoFile::Read
 ================
 */
int idDemoFile::Read( void *buffer, const size_t len ) const
{
	int read = compressor->Read( buffer, len );
	if ( read == 0 && len >= 4 ) {
		*static_cast<demoSystem_t*>(buffer) = DS_FINISHED;
	}
	return read;
}

/*
 ================
 idDemoFile::Write
 ================
 */
int idDemoFile::Write( const void *buffer, const size_t len ) const
{
	return compressor->Write( buffer, len );
}




