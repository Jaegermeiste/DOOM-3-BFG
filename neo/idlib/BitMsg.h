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
#ifndef __BITMSG_H__
#define __BITMSG_H__

#pragma once

/*
================================================
idBitMsg operates on a sequence of individual bits. It handles byte ordering and 
avoids alignment errors. It allows concurrent writing and reading. The data set with Init 
is never free-d.
================================================
*/
class idBitMsg {
public:
					idBitMsg() { InitWrite(nullptr, 0 ); }
					idBitMsg( byte * data, const size_t length ) { InitWrite( data, length ); }
					idBitMsg( const byte * data, const size_t length ) { InitRead( data, length ); }

	// both read & write
	void			InitWrite( byte *data, size_t length );

	// read only
	void			InitRead( const byte *data, size_t length );

	// get data for writing
	byte *			GetWriteData() const;

	// get data for reading
	const byte *	GetReadData() const;

	// get the maximum message size
	size_t			GetMaxSize() const;

	// generate error if not set and message is overflowed
	void			SetAllowOverflow( bool set );

	// returns true if the message was overflowed
	bool			IsOverflowed() const;

	// size of the message in bytes
	size_t			GetSize() const;

	// set the message size
	void			SetSize(size_t size );

	// get current write bit
	int				GetWriteBit() const;

	// set current write bit
	void			SetWriteBit( int bit ) const;

	// returns number of bits written
	size_t			GetNumBitsWritten() const;

	// space left in bytes for writing
	size_t			GetRemainingSpace() const;

	// space left in bits for writing
	size_t			GetRemainingWriteBits() const;

	//------------------------
	// Write State
	//------------------------

	// save the write state
	void			SaveWriteState(size_t &s, size_t &b, uint64 &t ) const;

	// restore the write state
	void			RestoreWriteState(size_t s, size_t b, uint64 t );

	//------------------------
	// Reading
	//------------------------

	// bytes read so far
	size_t			GetReadCount() const;

	// set the number of bytes and bits read
	void			SetReadCount(size_t bytes ) const;

	// get current read bit
	int				GetReadBit() const;

	// set current read bit
	void			SetReadBit( int bit ) const;

	// returns number of bits read
	size_t			GetNumBitsRead() const;

	// number of bytes left to read
	size_t			GetRemainingData() const;

	// number of bits left to read
	size_t			GetRemainingReadBits() const;

	// save the read state
	void			SaveReadState(size_t &c, size_t &b ) const;

	// restore the read state
	void			RestoreReadState(size_t c, size_t b ) const;

	//------------------------
	// Writing
	//------------------------

	// begin writing
	void			BeginWriting();

	// write up to the next byte boundary
	void			WriteByteAlign();

	// write the specified number of bits
	void			WriteBits( int value, short numBits );

	void			WriteBool( bool c );
	void			WriteChar( int8 c );
	void			WriteByte( uint8 c );
	void			WriteShort( int16 c );
	void			WriteUShort( uint16 c );
	void			WriteLong( int32 c );
	void			WriteLongLong( int64 c );
	void			WriteFloat( float f );
	void			WriteFloat( float f, unsigned short exponentBits, unsigned short mantissaBits );
	void			WriteAngle8( float f );
	void			WriteAngle16( float f );
	void			WriteDir(const idVec3 &dir, short numBits);
	void			WriteString( const char *s, int64 maxLength = -1, bool make7Bit = true );
	void			WriteData( const void *data, size_t length );
	void			WriteNetadr( const netadr_t adr );

	void			WriteUNorm8(const float f ) { WriteByte( idMath::Ftob( f * 255.0f ) ); }
	void			WriteUNorm16(const float f ) { WriteUShort( idMath::Ftoi( f * 65535.0f ) ); }
	void			WriteNorm16(const float f ) { WriteShort( idMath::Ftoi( f * 32767.0f ) ); }

	void			WriteDeltaChar(const int8 oldValue, const int8 newValue ) { WriteByte( newValue - oldValue ); }
	void			WriteDeltaByte(const uint8 oldValue, const uint8 newValue ) { WriteByte( newValue - oldValue ); }
	void			WriteDeltaShort(const int16 oldValue, const int16 newValue ) { WriteUShort( newValue - oldValue ); }
	void			WriteDeltaUShort(const uint16 oldValue, const uint16 newValue ) { WriteUShort( newValue - oldValue ); }
	void			WriteDeltaLong(const int32 oldValue, const int32 newValue ) { WriteLong( newValue - oldValue ); }
	void			WriteDeltaFloat(const float oldValue, const float newValue ) { WriteFloat( newValue - oldValue ); }
	void			WriteDeltaFloat(const float oldValue, const float newValue, const int exponentBits, const int mantissaBits ) { WriteFloat( newValue - oldValue, exponentBits, mantissaBits ); }

	bool			WriteDeltaDict( const idDict &dict, const idDict *base );

	template< int _max_, int _numBits_ >
	void			WriteQuantizedFloat( float value );
	template< int _max_, int _numBits_ >
	void			WriteQuantizedUFloat( float value );		// Quantize a float to a variable number of bits (assumes unsigned, uses simple quantization)

	template< typename T >
	void			WriteVectorFloat( const T & v ) { for ( int i = 0; i < v.GetDimension(); i++ ) { WriteFloat( v[i] ); } }
	template< typename T >
	void			WriteVectorUNorm8( const T & v ) { for ( int i = 0; i < v.GetDimension(); i++ ) { WriteUNorm8( v[i] ); } }
	template< typename T >
	void			WriteVectorUNorm16( const T & v ) { for ( int i = 0; i < v.GetDimension(); i++ ) { WriteUNorm16( v[i] ); } }
	template< typename T >
	void			WriteVectorNorm16( const T & v ) { for ( int i = 0; i < v.GetDimension(); i++ ) { WriteNorm16( v[i] ); } }

	// Compress a vector to a variable number of bits (assumes signed, uses simple quantization)
	template< typename T, int _max_, int _numBits_  >
	void			WriteQuantizedVector( const T & v ) { for ( int i = 0; i < v.GetDimension(); i++ ) { WriteQuantizedFloat< _max_, _numBits_ >( v[i] ); } }

	// begin reading.
	void			BeginReading() const;

	// read up to the next byte boundary
	void			ReadByteAlign() const;

	// read the specified number of bits
	int				ReadBits( int numBits ) const;

	bool			ReadBool() const;
	int				ReadChar() const;
	int				ReadByte() const;
	int				ReadShort() const;
	int				ReadUShort() const;
	int				ReadLong() const;
	int64			ReadLongLong() const;
	float			ReadFloat() const;
	float			ReadFloat( int exponentBits, int mantissaBits ) const;
	float			ReadAngle8() const;
	float			ReadAngle16() const;
	idVec3			ReadDir( int numBits ) const;
	size_t			ReadString( char *buffer, size_t bufferSize ) const;
	size_t			ReadString( idStr & str ) const;
	size_t			ReadData( void *data, size_t length ) const;
	void			ReadNetadr( netadr_t *adr ) const;

	float			ReadUNorm8() const { return ReadByte() / 255.0f; }
	float			ReadUNorm16() const { return ReadUShort() / 65535.0f; }
	float			ReadNorm16() const { return ReadShort() / 32767.0f; }

	int8			ReadDeltaChar(const int8 oldValue ) const { return oldValue + ReadByte(); }
	uint8			ReadDeltaByte(const uint8 oldValue ) const { return oldValue + ReadByte(); }
	int16			ReadDeltaShort(const int16 oldValue ) const { return oldValue + ReadUShort(); }
	uint16			ReadDeltaUShort(const uint16 oldValue ) const { return oldValue + ReadUShort(); }
	int32			ReadDeltaLong(const int32 oldValue ) const { return oldValue + ReadLong(); }
	float			ReadDeltaFloat(const float oldValue ) const { return oldValue + ReadFloat(); }
	float			ReadDeltaFloat(const float oldValue, const int exponentBits, const int mantissaBits ) const { return oldValue + ReadFloat( exponentBits, mantissaBits ); }
	bool			ReadDeltaDict( idDict &dict, const idDict *base ) const;

	template< int _max_, int _numBits_ >
	float			ReadQuantizedFloat() const;
	template< int _max_, int _numBits_ >
	float			ReadQuantizedUFloat() const;

	template< typename T >
	void			ReadVectorFloat( T & v ) const { for ( int i = 0; i < v.GetDimension(); i++ ) { v[i] = ReadFloat(); } }
	template< typename T >
	void			ReadVectorUNorm8( T & v ) const { for ( int i = 0; i < v.GetDimension(); i++ ) { v[i] = ReadUNorm8(); } }
	template< typename T >
	void			ReadVectorUNorm16( T & v ) const { for ( int i = 0; i < v.GetDimension(); i++ ) { v[i] = ReadUNorm16(); } }
	template< typename T >
	void			ReadVectorNorm16( T & v ) const { for ( int i = 0; i < v.GetDimension(); i++ ) { v[i] = ReadNorm16(); } }
	template< typename T, int _max_, int _numBits_ >
	void			ReadQuantizedVector( T & v ) const { for ( int i = 0; i < v.GetDimension(); i++ ) { v[i] = ReadQuantizedFloat< _max_, _numBits_ >(); } }

	static int		DirToBits( const idVec3 &dir, size_t numBits );
	static idVec3	BitsToDir( int bits, size_t numBits );

	void			SetHasChanged(const bool b ) { hasChanged = b; }
	bool			HasChanged() const { return hasChanged; }

private:
	byte *			writeData;		// pointer to data for writing
	const byte *	readData;		// pointer to data for reading
	size_t			maxSize;		// maximum size of message in bytes
	size_t			curSize;		// current size of message in bytes
	mutable int 	writeBit;		// number of bits written to the last written byte
	mutable size_t	readCount;		// number of bytes read so far
	mutable int 	readBit;		// number of bits read from the last read byte
	bool			allowOverflow;	// if false, generate error when the message is overflowed
	bool			overflowed;		// set true if buffer size failed (with allowOverflow set)
	bool			hasChanged;		// Hack

	mutable uint64	tempValue;

private:
	bool			CheckOverflow(size_t numBits );
	byte *			GetByteSpace(size_t length );
};

/*
========================
idBitMsg::InitWrite
========================
*/
ID_INLINE void idBitMsg::InitWrite( byte *data, const size_t length ) {
	writeData = data;
	readData = data;
	maxSize = length;
	curSize = 0;

	writeBit = 0;
	readCount = 0;
	readBit = 0;
	allowOverflow = false;
	overflowed = false;
	
	tempValue = 0;
}

/*
========================
idBitMsg::InitRead
========================
*/
ID_INLINE void idBitMsg::InitRead( const byte *data, const size_t length ) {
	writeData = nullptr;
	readData = data;
	maxSize = length;
	curSize = length;

	writeBit = 0;
	readCount = 0;
	readBit = 0;
	allowOverflow = false;
	overflowed = false;

	tempValue = 0;
}

/*
========================
idBitMsg::GetWriteData
========================
*/
ID_INLINE byte *idBitMsg::GetWriteData() const
{
	return writeData;
}

/*
========================
idBitMsg::GetReadData
========================
*/
ID_INLINE const byte *idBitMsg::GetReadData() const {
	return readData;
}

/*
========================
idBitMsg::GetMaxSize
========================
*/
ID_INLINE size_t idBitMsg::GetMaxSize() const {
	return maxSize;
}

/*
========================
idBitMsg::SetAllowOverflow
========================
*/
ID_INLINE void idBitMsg::SetAllowOverflow(const bool set ) {
	allowOverflow = set;
}

/*
========================
idBitMsg::IsOverflowed
========================
*/
ID_INLINE bool idBitMsg::IsOverflowed() const {
	return overflowed;
}

/*
========================
idBitMsg::GetSize
========================
*/
ID_INLINE size_t idBitMsg::GetSize() const {
	return curSize + ( writeBit != 0 );
}

/*
========================
idBitMsg::SetSize
========================
*/
ID_INLINE void idBitMsg::SetSize(const size_t size ) {
	assert( writeBit == 0 );
	
	if ( size > maxSize ) {
		curSize = maxSize;
	} else {
		curSize = size;
	}
}

/*
========================
idBitMsg::GetWriteBit
========================
*/
ID_INLINE int idBitMsg::GetWriteBit() const {
	return writeBit;
}

/*
========================
idBitMsg::SetWriteBit
========================
*/
ID_INLINE void idBitMsg::SetWriteBit(const int bit ) const
{
	// see idBitMsg::WriteByteAlign
	assert( false );
	writeBit = bit & 7;
	if ( writeBit ) {
		writeData[curSize - 1] &= ( 1 << writeBit ) - 1;
	}
}

/*
========================
idBitMsg::GetNumBitsWritten
========================
*/
ID_INLINE size_t idBitMsg::GetNumBitsWritten() const {
	return ( curSize << 3 ) + writeBit;
}

/*
========================
idBitMsg::GetRemainingSpace
========================
*/
ID_INLINE size_t idBitMsg::GetRemainingSpace() const {
	return maxSize - GetSize();
}

/*
========================
idBitMsg::GetRemainingWriteBits
========================
*/
ID_INLINE size_t idBitMsg::GetRemainingWriteBits() const {
	return ( maxSize << 3 ) - GetNumBitsWritten();
}

/*
========================
idBitMsg::SaveWriteState
========================
*/
ID_INLINE void idBitMsg::SaveWriteState(size_t &s, size_t &b, uint64 &t ) const {
	s = curSize;
	b = writeBit;
	t = tempValue;
}

/*
========================
idBitMsg::RestoreWriteState
========================
*/
ID_INLINE void idBitMsg::RestoreWriteState(const size_t s, const size_t b, const uint64 t ) {
	curSize = s;
	writeBit = b & 7;
	if ( writeBit ) {
		writeData[curSize] &= ( 1 << writeBit ) - 1;
	}
	tempValue = t;
}

/*
========================
idBitMsg::GetReadCount
========================
*/
ID_INLINE size_t idBitMsg::GetReadCount() const {
	return readCount;
}

/*
========================
idBitMsg::SetReadCount
========================
*/
ID_INLINE void idBitMsg::SetReadCount(const size_t bytes ) const
{
	readCount = bytes;
}

/*
========================
idBitMsg::GetReadBit
========================
*/
ID_INLINE int idBitMsg::GetReadBit() const {
	return readBit;
}

/*
========================
idBitMsg::SetReadBit
========================
*/
ID_INLINE void idBitMsg::SetReadBit(const int bit ) const
{
	readBit = bit & 7;
}

/*
========================
idBitMsg::GetNumBitsRead
========================
*/
ID_INLINE size_t idBitMsg::GetNumBitsRead() const {
	return ((readCount << 3) - ((static_cast<unsigned long long>(8) - readBit) & 7));
}

/*
========================
idBitMsg::GetRemainingData
========================
*/
ID_INLINE size_t idBitMsg::GetRemainingData() const {
	assert( writeBit == 0 );
	return curSize - readCount;
}

/*
========================
idBitMsg::GetRemainingReadBits
========================
*/
ID_INLINE size_t idBitMsg::GetRemainingReadBits() const {
	assert( writeBit == 0 );
	return ( curSize << 3 ) - GetNumBitsRead();
}

/*
========================
idBitMsg::SaveReadState
========================
*/
ID_INLINE void idBitMsg::SaveReadState(size_t &c, size_t &b ) const {
	assert( writeBit == 0 );
	c = readCount;
	b = readBit;
}

/*
========================
idBitMsg::RestoreReadState
========================
*/
ID_INLINE void idBitMsg::RestoreReadState(const size_t c, const size_t b ) const
{
	assert( writeBit == 0 );
	readCount = c;
	readBit = b & 7;
}

/*
========================
idBitMsg::BeginWriting
========================
*/
ID_INLINE void idBitMsg::BeginWriting() {
	curSize = 0;
	overflowed = false;
	writeBit = 0;
	tempValue = 0;
}

/*
========================
idBitMsg::WriteByteAlign
========================
*/
ID_INLINE void idBitMsg::WriteByteAlign() {
	// it is important that no uninitialized data slips in the msg stream,
	// because we use memcmp to decide if entities have changed and wether we should transmit them
	// this function has the potential to leave uninitialized bits into the stream,
	// however idBitMsg::WriteBits is properly initializing the byte to 0 so hopefully we are still safe
	// adding this extra check just in case
	curSize += writeBit != 0;
	assert( writeBit == 0 || ( ( writeData[curSize - 1] >> writeBit ) == 0 ) ); // had to early out writeBit == 0 because when writeBit == 0 writeData[curSize - 1] may be the previous byte written and trigger false positives
	writeBit = 0;
	tempValue = 0;
}

/*
========================
idBitMsg::WriteBool
========================
*/
ID_INLINE void idBitMsg::WriteBool(const bool c ) {
	WriteBits( c, 1 );
}

/*
========================
idBitMsg::WriteChar
========================
*/
ID_INLINE void idBitMsg::WriteChar(const int8 c ) {
	WriteBits( c, -8 );
}

/*
========================
idBitMsg::WriteByte
========================
*/
ID_INLINE void idBitMsg::WriteByte(const uint8 c ) {
	WriteBits( c, 8 );
}

/*
========================
idBitMsg::WriteShort
========================
*/
ID_INLINE void idBitMsg::WriteShort(const int16 c ) {
	WriteBits( c, -16 );
}

/*
========================
idBitMsg::WriteUShort
========================
*/
ID_INLINE void idBitMsg::WriteUShort(const uint16 c ) {
	WriteBits( c, 16 );
}

/*
========================
idBitMsg::WriteLong
========================
*/
ID_INLINE void idBitMsg::WriteLong(const int32 c ) {
	WriteBits( c, 32 );
}

/*
========================
idBitMsg::WriteLongLong
========================
*/
ID_INLINE void idBitMsg::WriteLongLong(const int64 c ) {
	const int a = c;
	const int b = c >> 32;
	WriteBits( a, 32 );
	WriteBits( b, 32 );
}

/*
========================
idBitMsg::WriteFloat
========================
*/
ID_INLINE void idBitMsg::WriteFloat( float f ) {
	WriteBits( *reinterpret_cast<int *>(&f), 32 );
}

/*
========================
idBitMsg::WriteFloat
========================
*/
ID_INLINE void idBitMsg::WriteFloat(const float f, const unsigned short exponentBits, const unsigned short mantissaBits ) {
	const int bits = idMath::FloatToBits( f, exponentBits, mantissaBits );
	WriteBits(bits, static_cast<short>(1) + exponentBits + mantissaBits);
}

/*
========================
idBitMsg::WriteAngle8
========================
*/
ID_INLINE void idBitMsg::WriteAngle8(const float f ) {
	WriteByte( ANGLE2BYTE( f ) );
}

/*
========================
idBitMsg::WriteAngle16
========================
*/
ID_INLINE void idBitMsg::WriteAngle16(const float f ) {
	WriteShort( ANGLE2SHORT(f) );
}

/*
========================
idBitMsg::WriteDir
========================
*/
ID_INLINE void idBitMsg::WriteDir(const idVec3 &dir, const short numBits) {
	WriteBits( DirToBits( dir, numBits ), numBits );
}

/*
========================
idBitMsg::BeginReading
========================
*/
ID_INLINE void idBitMsg::BeginReading() const {
	readCount = 0;
	readBit = 0;
	
	writeBit = 0;
	tempValue = 0;
}

/*
========================
idBitMsg::ReadByteAlign
========================
*/
ID_INLINE void idBitMsg::ReadByteAlign() const {
	readBit = 0;
}

/*
========================
idBitMsg::ReadBool
========================
*/
ID_INLINE bool idBitMsg::ReadBool() const {
	return ( ReadBits( 1 ) == 1 ) ? true : false;
}

/*
========================
idBitMsg::ReadChar
========================
*/
ID_INLINE int idBitMsg::ReadChar() const {
	return static_cast<signed char>(ReadBits(-8));
}

/*
========================
idBitMsg::ReadByte
========================
*/
ID_INLINE int idBitMsg::ReadByte() const {
	return static_cast<unsigned char>(ReadBits(8));
}

/*
========================
idBitMsg::ReadShort
========================
*/
ID_INLINE int idBitMsg::ReadShort() const {
	return static_cast<short>(ReadBits(-16));
}

/*
========================
idBitMsg::ReadUShort
========================
*/
ID_INLINE int idBitMsg::ReadUShort() const {
	return static_cast<unsigned short>(ReadBits(16));
}

/*
========================
idBitMsg::ReadLong
========================
*/
ID_INLINE int idBitMsg::ReadLong() const {
	return ReadBits( 32 );
}

/*
========================
idBitMsg::ReadLongLong
========================
*/
ID_INLINE int64 idBitMsg::ReadLongLong() const {
	const int64 a = ReadBits( 32 );
	const int64 b = ReadBits( 32 );
	const int64 c = ( 0x00000000ffffffff & a ) | ( b << 32 );
	return c;
}

/*
========================
idBitMsg::ReadFloat
========================
*/
ID_INLINE float idBitMsg::ReadFloat() const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadBits( 32 );
	return value;
}

/*
========================
idBitMsg::ReadFloat
========================
*/
ID_INLINE float idBitMsg::ReadFloat(const int exponentBits, const int mantissaBits ) const {
	const int bits = ReadBits( 1 + exponentBits + mantissaBits );
	return idMath::BitsToFloat( bits, exponentBits, mantissaBits );
}

/*
========================
idBitMsg::ReadAngle8
========================
*/
ID_INLINE float idBitMsg::ReadAngle8() const {
	return BYTE2ANGLE( ReadByte() );
}

/*
========================
idBitMsg::ReadAngle16
========================
*/
ID_INLINE float idBitMsg::ReadAngle16() const {
	return SHORT2ANGLE( ReadShort() );
}

/*
========================
idBitMsg::ReadDir
========================
*/
ID_INLINE idVec3 idBitMsg::ReadDir(const int numBits ) const {
	return BitsToDir( ReadBits( numBits ), numBits );
}

/*
========================
idBitMsg::WriteQuantizedFloat
========================
*/
template< int _max_, int _numBits_ >
ID_INLINE void idBitMsg::WriteQuantizedFloat( float value ) {
	enum { storeMax = ( 1 << ( _numBits_ - 1 ) ) - 1 };
	if ( _max_ > storeMax ) {
		// Scaling down (scale should be < 1)
		const float scale = static_cast<float>(storeMax) / static_cast<float>(_max_);
		WriteBits( idMath::ClampInt( -storeMax, storeMax, idMath::Ftoi( value * scale ) ), -_numBits_ );	
	} else {
		// Scaling up (scale should be >= 1) (Preserve whole numbers when possible)
		enum { scale = storeMax / _max_ };
		WriteBits( idMath::ClampInt( -storeMax, storeMax, idMath::Ftoi( value * scale ) ), -_numBits_ );	
	}
}

/*
========================
idBitMsg::WriteQuantizedUFloat
========================
*/
template< int _max_, int _numBits_ >
ID_INLINE void idBitMsg::WriteQuantizedUFloat( float value ) {
	enum { storeMax = ( 1 << _numBits_ ) - 1 };
	if ( _max_ > storeMax ) {
		// Scaling down (scale should be < 1)
		const float scale = static_cast<float>(storeMax) / static_cast<float>(_max_);
		WriteBits( idMath::ClampInt( 0, storeMax, idMath::Ftoi( value * scale ) ), _numBits_ );	
	} else {
		// Scaling up (scale should be >= 1) (Preserve whole numbers when possible)
		enum { scale = storeMax / _max_ };
		WriteBits( idMath::ClampInt( 0, storeMax, idMath::Ftoi( value * scale ) ), _numBits_ );	
	}
}

/*
========================
idBitMsg::ReadQuantizedFloat
========================
*/
template< int _max_, int _numBits_ >
ID_INLINE float idBitMsg::ReadQuantizedFloat() const {
	enum { storeMax = ( 1 << ( _numBits_ - 1 ) ) - 1 };
	if ( _max_ > storeMax ) {
		// Scaling down (scale should be < 1)
		const float invScale = static_cast<float>(_max_) / static_cast<float>(storeMax);
		return static_cast<float>(ReadBits(-_numBits_)) * invScale;
	} else {
		// Scaling up (scale should be >= 1) (Preserve whole numbers when possible)
		// Scale will be a whole number.  
		// We use a float to get rid of (potential divide by zero) which is handled above, but the compiler is dumb
		const float scale = storeMax / _max_;
		const float invScale = 1.0f / scale;
		return static_cast<float>(ReadBits(-_numBits_)) * invScale;
	}
}

/*
========================
idBitMsg::ReadQuantizedUFloat
========================
*/
template< int _max_, int _numBits_ >
float idBitMsg::ReadQuantizedUFloat() const {
	enum { storeMax = ( 1 << _numBits_ ) - 1 };
	if ( _max_ > storeMax ) {
		// Scaling down (scale should be < 1)
		const float invScale = static_cast<float>(_max_) / static_cast<float>(storeMax);
		return static_cast<float>(ReadBits(_numBits_)) * invScale;
	} else {
		// Scaling up (scale should be >= 1) (Preserve whole numbers when possible)
		// Scale will be a whole number.  
		// We use a float to get rid of (potential divide by zero) which is handled above, but the compiler is dumb
		const float scale = storeMax / _max_;	
		const float invScale = 1.0f / scale;
		return static_cast<float>(ReadBits(_numBits_)) * invScale;
	}
}

/*
================
WriteFloatArray
Writes all the values from the array to the bit message.
================
*/
template< class _arrayType_ >
void WriteFloatArray( idBitMsg & message, const _arrayType_ & sourceArray ) {
	for( int i = 0; i < idTupleSize< _arrayType_ >::value; ++i ) {
		message.WriteFloat( sourceArray[i] );
	}
}

/*
================
WriteFloatArrayDelta
Writes _num_ values from the array to the bit message.
================
*/
template< class _arrayType_ >
void WriteDeltaFloatArray( idBitMsg & message, const _arrayType_ & oldArray, const _arrayType_ & newArray ) {
	for( int i = 0; i < idTupleSize< _arrayType_ >::value; ++i ) {
		message.WriteDeltaFloat( oldArray[i], newArray[i] );
	}
}

/*
================
ReadFloatArray
Reads _num_ values from the array to the bit message.
================
*/
template< class _arrayType_ >
_arrayType_ ReadFloatArray( const idBitMsg & message ) {
	_arrayType_ result;

	for( int i = 0; i < idTupleSize< _arrayType_ >::value; ++i ) {
		result[i] = message.ReadFloat();
	}

	return result;
}

/*
================
ReadDeltaFloatArray
Reads _num_ values from the array to the bit message.
================
*/
template< class _arrayType_ >
_arrayType_ ReadDeltaFloatArray( const idBitMsg & message, const _arrayType_ & oldArray ) {
	_arrayType_ result;

	for( int i = 0; i < idTupleSize< _arrayType_ >::value; ++i ) {
		result[i] = message.ReadDeltaFloat( oldArray[i] );
	}

	return result;
}

#endif /* !__BITMSG_H__ */
