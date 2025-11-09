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
#pragma hdrstop
#include <algorithm>
#include <utility>

#include "precompiled.h"

/*
================================================================================================

	idBitMsg

================================================================================================
*/

/*
========================
idBitMsg::CheckOverflow
========================
*/
bool idBitMsg::CheckOverflow( const size_t numBits ) {
	if ( numBits > GetRemainingWriteBits() ) {
		if ( !allowOverflow ) {
			idLib::FatalError( "idBitMsg: overflow without allowOverflow set; maxsize=%i size=%i numBits=%i numRemainingWriteBits=%i",
				GetMaxSize(), GetSize(), numBits, GetRemainingWriteBits() );
		}
		if ( numBits > ( maxSize << 3 ) ) {
			idLib::FatalError( "idBitMsg: %i bits is > full message size", numBits );
		}
		idLib::Printf( "idBitMsg: overflow\n" );
		BeginWriting();
		overflowed = true;

		return true;
	}

	return false;
}

/*
========================
idBitMsg::GetByteSpace
========================
*/
byte *idBitMsg::GetByteSpace( const size_t length ) {
	if ( !writeData ) {
		idLib::FatalError( "idBitMsg::GetByteSpace: cannot write to message" );
	}

	// round up to the next byte
	WriteByteAlign();

	// check for overflow
	CheckOverflow( length << 3 );

	byte* ptr = writeData + curSize;
	curSize += length;

	return ptr;
}

#define NBM( x ) (uint64)( ( 1LL << x ) - 1 )
static uint64 maskForNumBits64[33] = {	NBM( 0x00 ), NBM( 0x01 ), NBM( 0x02 ), NBM( 0x03 ),
										NBM( 0x04 ), NBM( 0x05 ), NBM( 0x06 ), NBM( 0x07 ),
										NBM( 0x08 ), NBM( 0x09 ), NBM( 0x0A ), NBM( 0x0B ),
										NBM( 0x0C ), NBM( 0x0D ), NBM( 0x0E ), NBM( 0x0F ),
										NBM( 0x10 ), NBM( 0x11 ), NBM( 0x12 ), NBM( 0x13 ),
										NBM( 0x14 ), NBM( 0x15 ), NBM( 0x16 ), NBM( 0x17 ),
										NBM( 0x18 ), NBM( 0x19 ), NBM( 0x1A ), NBM( 0x1B ),
										NBM( 0x1C ), NBM( 0x1D ), NBM( 0x1E ), NBM( 0x1F ), 0xFFFFFFFF };

/*
========================
idBitMsg::WriteBits

If the number of bits is negative a sign is included.
========================
*/
void idBitMsg::WriteBits ( const int value, short numBits ) {
	if ( !writeData ) {
		idLib::FatalError( "idBitMsg::WriteBits: cannot write to message" );
	}

	// check if the number of bits is valid
	if ( numBits == 0 || numBits < -31 || numBits > 32 ) {
		idLib::FatalError( "idBitMsg::WriteBits: bad numBits %i", numBits );
	}

	// check for value overflows
	if ( numBits != 32 ) {
		if ( numBits > 0 ) {
			if ( value > ( 1 << numBits ) - 1 ) {
				idLib::FatalError( "idBitMsg::WriteBits: value overflow %d %d", 
									  value, numBits );

			} else if ( value < 0 ) {
				idLib::FatalError( "idBitMsg::WriteBits: value overflow %d %d", 
									  value, numBits );
			}
		} else {
			const unsigned shift = ( -1 - numBits );
			const int r = 1 << shift;
			if ( value > r - 1 ) {
				idLib::FatalError( "idBitMsg::WriteBits: value overflow %d %d", 
									  value, numBits );

			} else if ( value < -r ) {
				idLib::FatalError( "idBitMsg::WriteBits: value overflow %d %d", 
									  value, numBits );
			}
		}
	}

	if ( numBits < 0 ) {
		numBits = -numBits;
	}

	// check for msg overflow
	if ( CheckOverflow( numBits ) ) {
		return;
	}

	// Merge value with possible previous leftover
	tempValue |= (static_cast<int64>(value) & maskForNumBits64[numBits] ) << writeBit;
	
	writeBit += numBits;
	
	// Flush 8 bits (1 byte) at a time
	while ( writeBit >= 8 ) {
		writeData[curSize++] = tempValue & 255;
		tempValue >>= 8;
		writeBit -= 8;
	}
	
	// Write leftover now, in case this is the last WriteBits call
	if ( writeBit > 0 ) {
		writeData[curSize] = tempValue & 255;
	}
}

/*
========================
idBitMsg::WriteString
========================
*/
void idBitMsg::WriteString( const char * s, const size_t maxLength, const bool make7Bit ) {
	if ( !s ) {
		WriteData( "", 1 );
	} else {
		size_t i = 0;

		size_t l = idStr::Length(s);
		if ( maxLength > 0 && std::cmp_greater_equal(l, maxLength)) {
			l = maxLength - 1;
		}
		byte* dataPtr = GetByteSpace(l + 1);
		const byte* bytePtr = reinterpret_cast<const byte*>(s);
		if ( make7Bit ) {
			for ( i = 0; i < l; i++ ) {
				if ( bytePtr[i] > 127 ) {
					dataPtr[i] = '.';
				} else {
					dataPtr[i] = bytePtr[i];
				}
			}
		} else {
			for ( i = 0; i < l; i++ ) {
				dataPtr[i] = bytePtr[i];
			}
		}
		dataPtr[i] = '\0';
	}
}

/*
========================
idBitMsg::WriteData
========================
*/
void idBitMsg::WriteData( const void *data, const size_t length ) {
	memcpy( GetByteSpace( length ), data, length );
}

/*
========================
idBitMsg::WriteNetadr
========================
*/
void idBitMsg::WriteNetadr( const netadr_t &adr ) {
	WriteByte(adr.type);
	WriteULong(adr.flags);

	// Make IPv4 mapped to IPv6 if necessary
	uint8 outAddr16[16] = {};

	memset(outAddr16, 0, 16);
	if (adr.type == NA_IPv4 || adr.type == NA_BROADCAST || adr.type == NA_LOOPBACK) {
		// IPv4 path → write IPv6-mapped form ::ffff:w.x.y.z
		outAddr16[10] = 0xFF; outAddr16[11] = 0xFF;
		outAddr16[12] = adr.addr.ip4[0];
		outAddr16[13] = adr.addr.ip4[1];
		outAddr16[14] = adr.addr.ip4[2];
		outAddr16[15] = adr.addr.ip4[3];
	}
	else { // NA_IPv6
		memcpy(outAddr16, adr.addr.ip6, 16);
	}

	WriteData(outAddr16, 16 );
	WriteUShort(adr.port);

	// IPv6 specific
	WriteULong(adr.v6_scope_id);
	WriteByte(adr.v6_mcast_scope);
}

/*
========================
idBitMsg::WriteDeltaDict
========================
*/
template < Formattable T >
bool idBitMsg::WriteDeltaDict( const idDict<T> &dict, const idDict<T> *base ) {
	size_t i = 0;
	const idKeyValue<T> *kv = nullptr, *basekv = nullptr;
	bool changed = false;

	if ( base != nullptr) {

		for ( i = 0; i < dict.GetNumKeyVals(); i++ ) {
			kv = dict.GetKeyVal( i );
			if ( kv )
			{
				basekv = base->FindKey(kv->GetKey());

				if (basekv == nullptr || basekv->GetValue().Icmp(kv->GetValue()) != 0) {
					WriteString(kv->GetKey());
					WriteString(kv->GetValue());
					changed = true;
				}
			}
		}

		WriteString( "" );

		for ( i = 0; i < base->GetNumKeyVals(); i++ ) {
			basekv = base->GetKeyVal( i );

			if ( basekv )
			{
				kv = dict.FindKey(basekv->GetKey());
				if (kv == nullptr) {
					WriteString(basekv->GetKey());
					changed = true;
				}
			}
		}

		WriteString( "" );

	} else {

		for ( i = 0; i < dict.GetNumKeyVals(); i++ ) {
			kv = dict.GetKeyVal( i );

			if (kv) {
				WriteString(kv->GetKey());
				WriteString(kv->GetValue());
				changed = true;
			}
		}
		WriteString( "" );

		WriteString( "" );

	}

	return changed;
}

/*
========================
idBitMsg::ReadBits

If the number of bits is negative a sign is included.
========================
*/
int idBitMsg::ReadBits( int16 numBits ) const {
	if ( !readData ) {
		idLib::FatalError( "idBitMsg::ReadBits: cannot read from message" );
	}

	// check if the number of bits is valid
	if ( numBits == 0 || std::cmp_less(numBits, -31) || numBits > 32 ) {
		idLib::FatalError( "idBitMsg::ReadBits: bad numBits %i", numBits );
	}

	int value = 0;
	int valueBits = 0;

	size_t bitCount = 0;
	bool   sign = false;
	if ( numBits < 0 ) {
		bitCount = -numBits;
		sign = true;
	} else {
		bitCount = numBits;
		sign = false;
	}

	// check for overflow
	if (bitCount > GetRemainingReadBits() ) {
		return -1;
	}

	while (std::cmp_less(valueBits, bitCount)) {
		if ( readBit == 0 ) {
			readCount++;
		}
		int get = 8 - readBit;
		get = numeric_cast<decltype(get)>(Min(get, bitCount - valueBits));
		int fraction = readData[readCount - 1];
		fraction >>= readBit;
		fraction &= ( 1 << get ) - 1;
		value |= fraction << valueBits;

		valueBits += get;
		readBit = ( readBit + get ) & 7;
	}

	if ( sign ) {
		if ( value & ( 1 << (bitCount - 1 ) ) ) {
			value |= -1 ^ ( ( 1 << bitCount) - 1 );
		}
	}

	return value;
}

/*
========================
idBitMsg::ReadString
========================
*/
size_t idBitMsg::ReadString( char * buffer, const size_t bufferSize ) const {
	ReadByteAlign();
	size_t l = 0;
	while( 1 ) {
		int c = ReadByte();
		if ( c <= 0 || c >= 255 ) {
			break;
		}
		// translate all fmt spec to avoid crash bugs in string routines
		if ( c == '%' ) {
			c = '.';
		}

		// we will read past any excessively long string, so
		// the following data can be read, but the string will
		// be truncated
		if ( l < bufferSize - 1 ) {
			buffer[l] = c;
			l++;
		}
	}
	
	buffer[l] = 0;
	return l;
}

/*
========================
idBitMsg::ReadString
========================
*/
size_t idBitMsg::ReadString( idStr & str ) const {
	ReadByteAlign();

	size_t cnt = 0;
	for (size_t i = readCount; std::cmp_less(i, curSize); i++ ) {
		if ( readData[i] == 0 ) {
			break;
		}
		cnt++;
	}

	str.Clear();
	str.Append( reinterpret_cast<const char*>(readData) + readCount, cnt );
	readCount += cnt + 1;

	return str.Length();
}

/*
========================
idBitMsg::ReadData
========================
*/
size_t idBitMsg::ReadData( void *data, const size_t length ) const {
	ReadByteAlign();
	const size_t cnt = readCount;

	if ( readCount + length > curSize ) {
		if ( data ) {
			memcpy( data, readData + readCount, GetRemainingData() );
		}
		readCount = curSize;
	} else {
		if ( data ) {
			memcpy( data, readData + readCount, length );
		}
		readCount += length;
	}

	return ( readCount - cnt );
}

/*
========================
idBitMsg::ReadNetadr
========================
*/
static int NetAdr_IsV6MappedV4(const uint8 in16[16]) {
	static constexpr uint8 zero10[10] = { 0 };
	return memcmp(in16, zero10, 10) == 0 && in16[10] == 0xFF && in16[11] == 0xFF;
}

static int NetAdr_IsV6Loopback(const uint8 in16[16]) {
	for (int i = 0; i < 15; ++i)
	{
		if (in16[i] != 0)
		{
			return 0;
		}
	}
	return in16[15] == 1;
}

void idBitMsg::ReadNetadr( netadr_t *adr ) const {
	adr->type = static_cast<netadrtype_t>(ReadByte());
	adr->flags = ReadULong();

	uint8 inAddr16[16] = {};
	ReadData( &inAddr16, 16 );

	if (NetAdr_IsV6MappedV4(inAddr16)) {
		adr->type = NA_IPv4;
		adr->addr.ip4[0] = inAddr16[12];
		adr->addr.ip4[1] = inAddr16[13];
		adr->addr.ip4[2] = inAddr16[14];
		adr->addr.ip4[3] = inAddr16[15];
		adr->v6_scope_id = 0;
	}
	else {
		adr->type = NA_IPv6;
		memcpy(adr->addr.ip6, inAddr16, 16);
		if (NetAdr_IsV6Loopback(adr->addr.ip6)) {
			adr->type = NA_LOOPBACK;
			adr->v6_scope_id = 0;
		}
	}

	adr->port = ReadUShort();

	// IPv6 Specific
	adr->v6_scope_id = ReadULong();
	adr->v6_mcast_scope = ReadByte();
}

/*
========================
idBitMsg::ReadDeltaDict
========================
*/
template < Formattable T >
bool idBitMsg::ReadDeltaDict( idDict<T> &dict, const idDict<T> *base ) const {
	char		key[MAX_STRING_CHARS];
	char		value[MAX_STRING_CHARS];
	bool		changed = false;

	if ( base != nullptr) {
		dict = *base;
	} else {
		dict.Clear();
	}

	while( ReadString( key, sizeof( key ) ) != 0 ) {
		ReadString( value, sizeof( value ) );
		dict.Set( key, value );
		changed = true;
	}

	while( ReadString( key, sizeof( key ) ) != 0 ) {
		dict.Delete( key );
		changed = true;
	}

	return changed;
}

/*
========================
idBitMsg::DirToBits
========================
*/
int idBitMsg::DirToBits( const idVec3 &dir, short numBits ) {
	assert( numBits >= 6 && numBits <= 32 );
	assert( dir.LengthSqr() - 1.0f < 0.01f );

	numBits /= 3;
	const int max = (1 << (numBits - 1)) - 1;
	const float maxf = numeric_cast<float>(max);
	const float bias = 0.5f / maxf;

	int bits = IEEE_FLT_SIGNBITSET(dir.x) << (numBits * 3 - 1);
	bits |= ( numeric_cast<int>( ( idMath::Fabs( dir.x ) + bias ) * maxf ) ) << ( numBits * 2 );
	bits |= IEEE_FLT_SIGNBITSET( dir.y ) << ( numBits * 2 - 1 );
	bits |= ( numeric_cast<int>( ( idMath::Fabs( dir.y ) + bias ) * maxf ) ) << ( numBits * 1 );
	bits |= IEEE_FLT_SIGNBITSET( dir.z ) << ( numBits * 1 - 1 );
	bits |= ( numeric_cast<int>( ( idMath::Fabs( dir.z ) + bias ) * maxf ) ) << ( numBits * 0 );
	return bits;
}

/*
========================
idBitMsg::BitsToDir
========================
*/
idVec3 idBitMsg::BitsToDir(const int bits, short numBits ) {
	static float sign[2] = { 1.0f, -1.0f };
	idVec3 dir = {};

	assert( numBits >= 6 && numBits <= 32 );

	numBits /= 3;
	const int max = (1 << (numBits - 1)) - 1;
	const float maxf = numeric_cast<float>(max);
	const float invMax = 1.0f / maxf;

	dir.x = sign[( bits >> ( numBits * 3 - 1 ) ) & 1] * numeric_cast<float>( ( bits >> ( numBits * 2 ) ) & max ) * invMax;

	dir.y = sign[( bits >> ( numBits * 2 - 1 ) ) & 1] * numeric_cast<float>( ( bits >> ( numBits * 1 ) ) & max ) * invMax;

	dir.z = sign[( bits >> ( numBits * 1 - 1 ) ) & 1] * numeric_cast<float>( ( bits >> ( numBits * 0 ) ) & max ) * invMax;

	dir.NormalizeFast();
	return dir;
}
