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

#if defined( MACOS_X )
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/*
===============================================================================

	idLib

===============================================================================
*/

idSys *			idLib::sys			= nullptr;
idCommon *		idLib::common		= nullptr;
idCVarSystem *	idLib::cvarSystem	= nullptr;
idFileSystem *	idLib::fileSystem	= nullptr;
size_t			idLib::frameNumber	= 0;
bool			idLib::mainThreadInitialized = false;
ID_TLS			idLib::isMainThread = 0;

char idException::error[2048] = {};

/*
================
idLib::Init
================
*/
void idLib::Init() {

	assert( sizeof( bool ) == 1 );

	isMainThread = 1;
	mainThreadInitialized = true;	// note that the thread-local isMainThread is now valid

	// initialize little/big endian conversion
	Swap_Init();

	// init string memory allocator
	idStr::InitMemory();

	// initialize generic SIMD implementation
	idSIMD::Init();

	// initialize math
	idMath::Init();

	// test idMatX
	//idMatX::Test();

	// test idPolynomial
#ifdef _DEBUG
	idPolynomial::Test();
#endif

	// initialize the dictionary string pools
	idDict<>::Init();
}

/*
================
idLib::ShutDown
================
*/
void idLib::ShutDown() {

	// shut down the dictionary string pools
	idDict<>::Shutdown();

	// shut down the string memory allocator
	idStr::ShutdownMemory();

	// shut down the SIMD engine
	idSIMD::Shutdown();
}


/*
===============================================================================

	Colors

===============================================================================
*/

idVec4	colorBlack	= idVec4( 0.00f, 0.00f, 0.00f, 1.00f );
idVec4	colorWhite	= idVec4( 1.00f, 1.00f, 1.00f, 1.00f );
idVec4	colorRed	= idVec4( 1.00f, 0.00f, 0.00f, 1.00f );
idVec4	colorGreen	= idVec4( 0.00f, 1.00f, 0.00f, 1.00f );
idVec4	colorBlue	= idVec4( 0.00f, 0.00f, 1.00f, 1.00f );
idVec4	colorYellow	= idVec4( 1.00f, 1.00f, 0.00f, 1.00f );
idVec4	colorMagenta= idVec4( 1.00f, 0.00f, 1.00f, 1.00f );
idVec4	colorCyan	= idVec4( 0.00f, 1.00f, 1.00f, 1.00f );
idVec4	colorOrange	= idVec4( 1.00f, 0.50f, 0.00f, 1.00f );
idVec4	colorPurple	= idVec4( 0.60f, 0.00f, 0.60f, 1.00f );
idVec4	colorPink	= idVec4( 0.73f, 0.40f, 0.48f, 1.00f );
idVec4	colorBrown	= idVec4( 0.40f, 0.35f, 0.08f, 1.00f );
idVec4	colorLtGrey	= idVec4( 0.75f, 0.75f, 0.75f, 1.00f );
idVec4	colorMdGrey	= idVec4( 0.50f, 0.50f, 0.50f, 1.00f );
idVec4	colorDkGrey	= idVec4( 0.25f, 0.25f, 0.25f, 1.00f );

/*
================
PackColor
================
*/
dword PackColor( const idVec4 &color ) {
	const byte dx = idMath::Ftob( color.x * 255.0f );
	const byte dy = idMath::Ftob( color.y * 255.0f );
	const byte dz = idMath::Ftob( color.z * 255.0f );
	const byte dw = idMath::Ftob( color.w * 255.0f );
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
}

/*
================
UnpackColor
================
*/
void UnpackColor( const dword color, idVec4 &unpackedColor ) {
	unpackedColor.Set( numeric_cast<float>( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
						numeric_cast<float>( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
						numeric_cast<float>( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
						numeric_cast<float>( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ) );
}

/*
================
PackColor
================
*/
dword PackColor( const idVec3 &color ) {
	const byte dx = idMath::Ftob( color.x * 255.0f );
	const byte dy = idMath::Ftob( color.y * 255.0f );
	const byte dz = idMath::Ftob( color.z * 255.0f );
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 );
}

/*
================
UnpackColor
================
*/
void UnpackColor( const dword color, idVec3 &unpackedColor ) {
	unpackedColor.Set(numeric_cast<float>( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
						numeric_cast<float>( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
						numeric_cast<float>( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ) );
}

/*
===============
idLib::FatalError
===============
*/
NO_RETURN void idLib::FatalError( const char *fmt, ... ) {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->FatalError( "%s", text );
}

/*
===============
idLib::Error
===============
*/
NO_RETURN void idLib::Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[MAX_STRING_CHARS] = {};

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Error( "%s", text );
}

/*
===============
idLib::Warning
===============
*/
void idLib::Warning( const char *fmt, ... ) {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Warning( "%s", text );
}

/*
===============
idLib::WarningIf
===============
*/
void idLib::WarningIf( const bool test, const char *fmt, ... ) {
	if ( !test ) {
		return;
	}

	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Warning( "%s", text );
}

/*
===============
idLib::Printf
===============
*/
void idLib::Printf( const char *fmt, ... ) {
	va_list		argptr;
	va_start( argptr, fmt );
	if ( common ) {
		common->VPrintf( fmt, argptr );
	}
	va_end( argptr );
}

/*
===============
idLib::PrintfIf
===============
*/
void idLib::PrintfIf( const bool test, const char *fmt, ... ) {
	if ( !test ) {
		return;
	}

	va_list		argptr;
	va_start( argptr, fmt );
	common->VPrintf( fmt, argptr );
	va_end( argptr );
}

/*
===============================================================================

	Byte order functions

===============================================================================
*/

// can't just use function pointers, or dll linkage can mess up
static int16        	(*_BigShort)( const int16 l );
static int16        	(*_LittleShort)( const int16 l );
static uint16	        (*_BigUShort)( const uint16 l);
static uint16	        (*_LittleUShort)( const uint16 l);
static int32      		(*_BigLong)( const int32 l );
static int32       		(*_LittleLong)( const int32 l );
static uint32		    (*_BigULong)( const uint32 l );
static uint32		    (*_LittleULong)( const uint32 l );
static int64       	    (*_BigLongLong)( const int64 l );
static int64            (*_LittleLongLong)( const int64 l );
static uint64           (*_BigULongLong)( const uint64 l );
static uint64           (*_LittleULongLong)( const uint64 l );
static float	        (*_BigFloat)( const float f );
static float	        (*_LittleFloat)( const float f );
static double	        (*_BigDouble)( const double d );
static double	        (*_LittleDouble)( const double d );
static long double	    (*_BigLongDouble)( const long double ld );
static long double	    (*_LittleLongDouble)( const long double ld );
static void		        (*_BigRevBytes)( void *bp, const size_t elsize, const size_t elcount );
static void		        (*_LittleRevBytes)( void *bp, const size_t elsize, const size_t elcount );
static void             (*_LittleBitField)( void *bp, const size_t elsize );
static void		        (*_SixtetsForInt)( byte *out, const int32 src );
static int32	        (*_IntForSixtets)( const byte *in );
static void		        (*_SixtetsForUInt)( byte* out, const uint32 src );
static uint32           (*_UIntForSixtets)( const byte* in );
static void		        (*_SixtetsForInt64)( byte *out, const int64 src );
static int64	        (*_Int64ForSixtets)( const byte *in );
static void		        (*_SixtetsForUInt64)( byte* out, const uint64 src );
static uint64           (*_UInt64ForSixtets)( const byte* in );

int16               	BigShort( const int16 l ) { return _BigShort( l ); }
int16               	LittleShort( const int16 l ) { return _LittleShort( l ); }
uint16      	        BigUShort( const uint16 l) { return _BigUShort(l); }
uint16      	        LittleUShort( const uint16 l) { return _LittleUShort(l); }
int32             		BigLong( const int32 l ) { return _BigLong( l ); }
int32             		LittleLong( const int32 l ) { return _LittleLong( l ); }
uint32    		        BigULong( const uint32 l) { return _BigULong(l); }
uint32    		        LittleULong( const uint32 l) { return _LittleULong(l); }
int64              	    BigLongLong( const int64 l) { return _BigLongLong(l); }
int64             	    LittleLongLong( const int64 l) { return _LittleLongLong(l); }
uint64     	            BigULongLong( const uint64 l) { return _BigULongLong(l); }
uint64     	            LittleULongLong( const uint64 l) { return _LittleULongLong(l); }
float               	BigFloat( const float f ) { return _BigFloat( f ); }
float                 	LittleFloat( const float f ) { return _LittleFloat( f ); }
double               	BigDouble( const double d ) { return _BigDouble( d ); }
double                 	LittleDouble( const double d ) { return _LittleDouble( d ); }
long double             BigLongDouble( const long double ld ) { return _BigLongDouble( ld ); }
long double             LittleLongDouble( const long double ld ) { return _LittleLongDouble( ld ); }
void                	BigRevBytes( void *bp, const size_t elsize, const size_t elcount ) { _BigRevBytes( bp, elsize, elcount ); }
void                	LittleRevBytes( void *bp, const size_t elsize, const size_t elcount ){ _LittleRevBytes( bp, elsize, elcount ); }
void                	LittleBitField( void *bp, const size_t elsize ){ _LittleBitField( bp, elsize ); }

void                 	SixtetsForInt( byte *out, const int32 src) { _SixtetsForInt( out, src ); }
int32              		IntForSixtets( const byte *in ) { return _IntForSixtets( in ); }
void                 	SixtetsForUInt( byte* out, const uint32 src ) { _SixtetsForUInt(out, src); }
uint32              	UIntForSixtets( const byte* in ) { return _UIntForSixtets(in); }
void                 	SixtetsForInt64( byte* out, const int64 src ) { _SixtetsForInt64(out, src); }
int64              		Int64ForSixtets( const byte* in ) { return _Int64ForSixtets(in); }
void                 	SixtetsForUInt64( byte* out, const uint64 src ) { _SixtetsForUInt64(out, src); }
uint64              	UInt64ForSixtets( const byte* in ) { return _UInt64ForSixtets(in); }

/*
================
ShortSwap
================
*/
static int16 ShortSwap(const int16 l ) {
	const byte b1 = l & 255;
	const byte b2 = (l >> 8) & 255;

	return numeric_cast<int16>(numeric_cast<int32>(b1<<8) + b2);
}

/*
================
ShortNoSwap
================
*/
static int16 ShortNoSwap(const int16 l ) {
	return l;
}

/*
================
UShortSwap
================
*/
static uint16 UShortSwap(const uint16 l) {
	const byte b1 = l & 255;
	const byte b2 = (l >> 8) & 255;

	return numeric_cast<uint16>(b1 << 8) + b2;
}

/*
================
UShortNoSwap
================
*/
static uint16 UShortNoSwap(const uint16 l) {
	return l;
}

/*
================
LongSwap
================
*/
static int32 LongSwap (const int32 l ) {
	const byte b1 =  l        & 255;
	const byte b2 = (l >> 8)  & 255;
	const byte b3 = (l >> 16) & 255;
	const byte b4 = (l >> 24) & 255;

	return (static_cast<int32>(b1)<<24) + (static_cast<int32>(b2)<<16) + (static_cast<int32>(b3)<<8) + b4;
}

/*
================
LongNoSwap
================
*/
static int32	LongNoSwap(const int32 l ) {
	return l;
}

/*
================
ULongSwap
================
*/
static uint32 ULongSwap(const uint32 l) {
	const byte b1 = l & 255;
	const byte b2 = (l >> 8) & 255;
	const byte b3 = (l >> 16) & 255;
	const byte b4 = (l >> 24) & 255;

	return (static_cast<uint32>(b1) << 24) + (static_cast<uint32>(b2) << 16) + (static_cast<uint32>(b3) << 8) + b4;
}

/*
================
ULongNoSwap
================
*/
static uint32	ULongNoSwap(const uint32 l) {
	return l;
}

/*
================
LongLongSwap
================
*/
static int64 LongLongSwap(const int64 l) {
	const byte b1 =  l        & 255;
	const byte b2 = (l >> 8)  & 255;
	const byte b3 = (l >> 16) & 255;
	const byte b4 = (l >> 24) & 255;
	const byte b5 = (l >> 32) & 255;
	const byte b6 = (l >> 40) & 255;
	const byte b7 = (l >> 48) & 255;
	const byte b8 = (l >> 56) & 255;

	return (static_cast<int64>(b1) << 56) + (static_cast<int64>(b2) << 48) + (static_cast<int64>(b3) << 40) + (static_cast<int64>(b4) << 32) + (static_cast<int64>(b5) << 24) + (static_cast<int64>(b6) << 16) + (static_cast<int64>(b7) << 8) + b8;
}

/*
================
LongLongNoSwap
================
*/
static int64	LongLongNoSwap(const int64 l) {
	return l;
}


/*
================
ULongLongSwap
================
*/
static uint64 ULongLongSwap(const uint64 l) {
	const byte b1 = l & 255;
	const byte b2 = (l >> 8) & 255;
	const byte b3 = (l >> 16) & 255;
	const byte b4 = (l >> 24) & 255;
	const byte b5 = (l >> 32) & 255;
	const byte b6 = (l >> 40) & 255;
	const byte b7 = (l >> 48) & 255;
	const byte b8 = (l >> 56) & 255;

	return (static_cast<uint64>(b1) << 56) + (static_cast<uint64>(b2) << 48) + (static_cast<uint64>(b3) << 40) + (static_cast<uint64>(b4) << 32) + (static_cast<uint64>(b5) << 24) + (static_cast<uint64>(b6) << 16) + (static_cast<uint64>(b7) << 8) + b8;
}

/*
================
ULongNoSwap
================
*/
static uint64	ULongLongNoSwap(const uint64 l) {
	return l;
}

/*
================
FloatSwap
================
*/
static float FloatSwap( const float f ) {
	/*union {
		float	f;
		byte	b[4];
	} dat1 = {}, dat2 = {};
	
	
	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;*/

	uint8 b[4] = {};
	memcpy(b, &f, sizeof(b));
	std::swap(b[0], b[3]);
	std::swap(b[1], b[2]);
	float out = 0.0f;
	memcpy(&out, b, sizeof(b));
	return out;
}

/*
================
FloatNoSwap
================
*/
static float FloatNoSwap( const float f ) {
	return f;
}

// -------------------- DOUBLE VARIANTS --------------------
static double DoubleSwap( const double d ) {
	uint8 b[8] = {};
	memcpy(b, &d, sizeof(b));

	for (size_t i = 0; i < sizeof(b) / 2; ++i) {
		std::swap(b[i], b[sizeof(b) - 1 - i]);
	}

	double out = 0.0;
	memcpy(&out, b, sizeof(b));

	return out;
}

static double DoubleNoSwap( const double d ) {
	return d;
}

// -------------------- LONG DOUBLE VARIANTS --------------------
// Note: size of long double varies by platform (8, 10, 12, or 16 bytes).
// This version safely handles any size without assumptions.
static long double LongDoubleSwap( const long double ld ) {
	uint8 b[sizeof(long double)];
	memcpy(b, &ld, sizeof(b));

	for (size_t i = 0; i < sizeof(b) / 2; ++i) {
		std::swap(b[i], b[sizeof(b) - 1 - i]);
	}

	long double out = 0.0;
	memcpy(&out, b, sizeof(b));

	return out;
}

static long double LongDoubleNoSwap( const long double ld ) {
	return ld;
}

/*
=====================================================================
RevBytesSwap

Reverses byte order in place.

INPUTS
   bp       bytes to reverse
   elsize   size of the underlying data type
   elcount  number of elements to swap

RESULTS
   Reverses the byte order in each of elcount elements.
===================================================================== */
static void RevBytesSwap( void *bp, const size_t elsize, const size_t elcount ) {
	byte* q = nullptr;
	byte* p = static_cast<byte*>(bp);

	size_t element_count_internal = elcount;

	if ( elsize == 2 ) {
		q = p + 1;
		while ( element_count_internal-- ) {
			*p ^= *q;
			*q ^= *p;
			*p ^= *q;
			p += 2;
			q += 2;
		}
		return;
	}

	while ( element_count_internal-- ) {
		q = p + elsize - 1;
		while ( p < q ) {
			*p ^= *q;
			*q ^= *p;
			*p ^= *q;
			++p;
			--q;
		}
		p += elsize >> 1;
	}
}

/*
 =====================================================================
 RevBytesSwap
 
 Reverses byte order in place, then reverses bits in those bytes
 
 INPUTS
 bp       bitfield structure to reverse
 elsize   size of the underlying data type
 
 RESULTS
 Reverses the bitfield of size elsize.
 ===================================================================== */
static void RevBitFieldSwap( void *bp, const size_t elsize) {
	LittleRevBytes( bp, elsize, 1 );

	byte* p = static_cast<byte*>(bp);
	size_t element_size_internal = elsize;

	while ( element_size_internal-- ) {
		byte v = *p;
		byte t = 0;
		for ( index_t i = 7; i >= 0; i-- ) {
			t <<= 1;
			v >>= 1;
			t |= v & 1;
		}
		*p++ = t;
	}
}

/*
================
RevBytesNoSwap
================
*/
static void RevBytesNoSwap( void *bp, const size_t elsize, const size_t elcount ) {
	return;
}

/*
 ================
 RevBytesNoSwap
 ================
 */
static void RevBitFieldNoSwap( void *bp, const size_t elsize ) {
	return;
}

/*
================
SixtetsForIntLittle
================
*/
static void SixtetsForIntLittle( byte *out, int32 src) {
	/*const byte *b = reinterpret_cast<byte*>(&src);
	out[0] = ( b[0] & 0xfc ) >> 2;
	out[1] = ( ( b[0] & 0x3 ) << 4 ) + ( ( b[1] & 0xf0 ) >> 4 );
	out[2] = ( ( b[1] & 0xf ) << 2 ) + ( ( b[2] & 0xc0 ) >> 6 );
	out[3] = b[2] & 0x3f;*/

	uint8 b[4] = {};
	memcpy( b, &src, sizeof( b ));

	out[0] = static_cast<byte>( ( b[0] & 0xFC ) >> 2 );
	out[1] = static_cast<byte>((( b[0] & 0x03 ) << 4 ) | (( b[1] & 0xF0 ) >> 4 ));
	out[2] = static_cast<byte>((( b[1] & 0x0F ) << 2 ) | (( b[2] & 0xC0 ) >> 6 ));
	out[3] = static_cast<byte>(   b[2] & 0x3F );
}

/*
================
SixtetsForIntBig
TTimo: untested - that's the version from initial base64 encode
================
*/
static void SixtetsForIntBig( byte *out, int32 src) {
	/*for( size_t i = 0 ; i < 4 ; i++ ) {
		out[i] = src & 0x3f;
		src >>= 6;
	}*/

	uint32 v = 0;

	memcpy( &v, &src, sizeof( v )); // safe reinterpretation

	out[0] = static_cast<byte>(( v )       & 0x3F );
	out[1] = static_cast<byte>(( v >> 6  ) & 0x3F );
	out[2] = static_cast<byte>(( v >> 12 ) & 0x3F );
	out[3] = static_cast<byte>(( v >> 18 ) & 0x3F );
}

/*
================
IntForSixtetsLittle
================
*/
static int32 IntForSixtetsLittle( const byte *in ) {
	/*int32 ret = 0;
	byte *b = reinterpret_cast<byte*>(&ret);
	b[0] |= in[0] << 2;
	b[0] |= ( in[1] & 0x30 ) >> 4;
	b[1] |= ( in[1] & 0xf ) << 4;
	b[1] |= ( in[2] & 0x3c ) >> 2;
	b[2] |= ( in[2] & 0x3 ) << 6;
	b[2] |= in[3];
	return ret;*/

	uint8 b[4] = {};

	b[0] = static_cast<uint8>((  in[0]          << 2 ) | (( in[1] & 0x30 ) >> 4 ));
	b[1] = static_cast<uint8>((( in[1] & 0x0F ) << 4 ) | (( in[2] & 0x3C ) >> 2 ));
	b[2] = static_cast<uint8>((( in[2] & 0x03 ) << 6 ) | (  in[3] & 0x3F ));

	int32 ret = 0;

	memcpy( &ret, b, sizeof( b ));

	return ret;
}

/*
================
IntForSixtetsBig
TTimo: untested - that's the version from initial base64 decode
================
*/
static int32 IntForSixtetsBig( const byte *in ) {
	/*int32 ret = 0;
	ret |= in[0];
	ret |= in[1] << 6;
	ret |= in[2] << 2*6;
	ret |= in[3] << 3*6;
	return ret;*/

	uint32 v = 0;

	v |= static_cast<uint32>( in[0] );
	v |= static_cast<uint32>( in[1] ) << 6;
	v |= static_cast<uint32>( in[2] ) << 12;
	v |= static_cast<uint32>( in[3] ) << 18;

	int32 ret = 0;

	memcpy( &ret, &v, sizeof( ret ));

	return ret;
}

// ----------------------------- 32-bit (unsigned) -----------------------------

static void SixtetsForUIntLittle( byte* out, const uint32 src ) {
	// Use only the lowest 24 bits (3 bytes), identical layout as signed version.
	uint8 b[4] = {};

	memcpy( b, &src, sizeof( b ));

	out[0] = static_cast<byte>((  b[0] & 0xFC ) >> 2);
	out[1] = static_cast<byte>((( b[0] & 0x03 ) << 4) | (( b[1] & 0xF0 ) >> 4 ));
	out[2] = static_cast<byte>((( b[1] & 0x0F ) << 2) | (( b[2] & 0xC0 ) >> 6 ));
	out[3] = static_cast<byte>((  b[2] & 0x3F ));
}

static void SixtetsForUIntBig( byte* out, const uint32 src ) {
	// Packs the lowest 24 bits as four 6-bit groups.
	uint32 v = static_cast<uint32>( src );

	out[0] = static_cast<byte>((v) & 0x3F);
	out[1] = static_cast<byte>((v >> 6) & 0x3F);
	out[2] = static_cast<byte>((v >> 12) & 0x3F);
	out[3] = static_cast<byte>((v >> 18) & 0x3F);
}

static uint32 UIntForSixtetsLittle( const byte* in ) {
	uint8 b[4] = {}; // reconstruct 3 bytes into the low 24 bits

	b[0] |= static_cast<uint8>(  in[0]          << 2 );
	b[0] |= static_cast<uint8>(( in[1] & 0x30 ) >> 4 );
	b[1] |= static_cast<uint8>(( in[1] & 0x0F ) << 4 );
	b[1] |= static_cast<uint8>(( in[2] & 0x3C ) >> 2 );
	b[2] |= static_cast<uint8>(( in[2] & 0x03 ) << 6 );
	b[2] |= static_cast<uint8>(  in[3] & 0x3F );

	uint32 ret = 0;

	memcpy( &ret, b, sizeof( b ));

	return ret;
}

static uint32 UIntForSixtetsBig( const byte* in ) {
	// Reassemble low 24 bits from four sixtets.
	uint32 ret = 0;

	ret |= static_cast<uint32>( in[0] );
	ret |= static_cast<uint32>( in[1] ) << 6;
	ret |= static_cast<uint32>( in[2] ) << 12;
	ret |= static_cast<uint32>( in[3] ) << 18;

	return ret;
}

// ----------------------------- 64-bit (signed) -------------------------------
// 48-bit payload (6 bytes) <-> 8 sixtets. High 16 bits are zeroed on decode.

static void SixtetsForInt64Little( byte* out, const int64 src ) {
	uint8 b[8] = {};

	memcpy( b, &src, sizeof( b )); // little-endian: b[0]..b[5] are the low 6 bytes

	// First 3 bytes -> 4 sixtets
	out[0] = static_cast<byte>((  b[0] & 0xFC ) >> 2 );
	out[1] = static_cast<byte>((( b[0] & 0x03 ) << 4 ) | (( b[1] & 0xF0 ) >> 4 ));
	out[2] = static_cast<byte>((( b[1] & 0x0F ) << 2 ) | (( b[2] & 0xC0 ) >> 6 ));
	out[3] = static_cast<byte>((  b[2] & 0x3F ));
	// Next 3 bytes -> 4 sixtets
	out[4] = static_cast<byte>((  b[3] & 0xFC ) >> 2 );
	out[5] = static_cast<byte>((( b[3] & 0x03 ) << 4 ) | (( b[4] & 0xF0 ) >> 4 ));
	out[6] = static_cast<byte>((( b[4] & 0x0F ) << 2 ) | (( b[5] & 0xC0 ) >> 6 ));
	out[7] = static_cast<byte>((  b[5] & 0x3F ));
}

static void SixtetsForInt64Big( byte* out, const int64 src ) {
	// Use the low 48 bits.
	uint64 v = static_cast<uint64>( src ) & 0x0000FFFFFFFFFFFFull;

	out[0] = static_cast<byte>(( v )       & 0x3F );
	out[1] = static_cast<byte>(( v >> 6  ) & 0x3F );
	out[2] = static_cast<byte>(( v >> 12 ) & 0x3F );
	out[3] = static_cast<byte>(( v >> 18 ) & 0x3F );
	out[4] = static_cast<byte>(( v >> 24 ) & 0x3F );
	out[5] = static_cast<byte>(( v >> 30 ) & 0x3F );
	out[6] = static_cast<byte>(( v >> 36 ) & 0x3F );
	out[7] = static_cast<byte>(( v >> 42 ) & 0x3F );
}

static int64 Int64ForSixtetsLittle( const byte* in ) {
	uint8 b[8] = {}; // we'll fill b[0..5]

	b[0] = static_cast<uint8>((  in[0]          << 2 ) | (( in[1] & 0x30 ) >> 4 ));
	b[1] = static_cast<uint8>((( in[1] & 0x0F ) << 4 ) | (( in[2] & 0x3C ) >> 2 ));
	b[2] = static_cast<uint8>((( in[2] & 0x03 ) << 6 ) | (  in[3] & 0x3F ));
	b[3] = static_cast<uint8>((  in[4]          << 2 ) | (( in[5] & 0x30 ) >> 4 ));
	b[4] = static_cast<uint8>((( in[5] & 0x0F ) << 4 ) | (( in[6] & 0x3C ) >> 2 ));
	b[5] = static_cast<uint8>((( in[6] & 0x03 ) << 6 ) | (  in[7] & 0x3F ));

	uint64 ret = 0;
	// Only the lowest 6 bytes are defined by these 8 sixtets; high bytes zero.
	memcpy( &ret, b, 6 );

	return numeric_cast<int64>( ret );
}

static int64 Int64ForSixtetsBig( const byte* in ) {
	uint64 ret = 0;

	ret |= static_cast<uint64>( in[0] );
	ret |= static_cast<uint64>( in[1] ) << 6;
	ret |= static_cast<uint64>( in[2] ) << 12;
	ret |= static_cast<uint64>( in[3] ) << 18;
	ret |= static_cast<uint64>( in[4] ) << 24;
	ret |= static_cast<uint64>( in[5] ) << 30;
	ret |= static_cast<uint64>( in[6] ) << 36;
	ret |= static_cast<uint64>( in[7] ) << 42;

	return numeric_cast<int64>(ret);
}

// ----------------------------- 64-bit (unsigned) -----------------------------

static void SixtetsForUInt64Little( byte* out, const uint64 src ) {
	SixtetsForInt64Little(out, static_cast<int64>(src));
}

static void SixtetsForUInt64Big( byte* out, const uint64 src ) {
	uint64 v = src & 0x0000FFFFFFFFFFFFull;

	out[0] = static_cast<byte>(( v)        & 0x3F );
	out[1] = static_cast<byte>(( v >> 6  ) & 0x3F );
	out[2] = static_cast<byte>(( v >> 12 ) & 0x3F );
	out[3] = static_cast<byte>(( v >> 18 ) & 0x3F );
	out[4] = static_cast<byte>(( v >> 24 ) & 0x3F );
	out[5] = static_cast<byte>(( v >> 30 ) & 0x3F );
	out[6] = static_cast<byte>(( v >> 36 ) & 0x3F );
	out[7] = static_cast<byte>(( v >> 42 ) & 0x3F );
}

static uint64 UInt64ForSixtetsLittle( const byte* in ) {
	return numeric_cast<uint64>( Int64ForSixtetsLittle( in ));
}

static uint64 UInt64ForSixtetsBig( const byte* in ) {
	return numeric_cast<uint64>( Int64ForSixtetsBig( in ));
}


/*
================
Swap_Init
================
*/
void Swap_Init() {
	constexpr byte	swaptest[2] = {1,0};

	// set the byte swapping variables in a portable manner	
	if ( *reinterpret_cast<const int16 *>(swaptest) == 1) {
		// little endian ex: x86, x64
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigUShort = UShortSwap;
		_LittleUShort = UShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigULong = ULongSwap;
		_LittleULong = ULongNoSwap;
		_BigLongLong = LongLongSwap;
		_LittleLongLong = LongLongNoSwap;
		_BigULongLong = ULongLongSwap;
		_LittleULongLong = ULongLongNoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
		_BigDouble = DoubleSwap;
		_LittleDouble = DoubleNoSwap;
		_BigLongDouble = LongDoubleSwap;
		_LittleLongDouble = LongDoubleNoSwap;
		_BigRevBytes = RevBytesSwap;
		_LittleRevBytes = RevBytesNoSwap;
		_LittleBitField = RevBitFieldNoSwap;
		_SixtetsForInt = SixtetsForIntLittle;
		_IntForSixtets = IntForSixtetsLittle;
		_SixtetsForUInt = SixtetsForUIntLittle;
		_UIntForSixtets = UIntForSixtetsLittle;
		_SixtetsForInt64 = SixtetsForInt64Little;
		_Int64ForSixtets = Int64ForSixtetsLittle;
		_SixtetsForUInt64 = SixtetsForUInt64Little;
		_UInt64ForSixtets = UInt64ForSixtetsLittle;
	} else {
		// big endian ex: ppc
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigUShort = UShortNoSwap;
		_LittleUShort = UShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigULong = ULongNoSwap;
		_LittleULong = ULongSwap;
		_BigLongLong = LongLongNoSwap;
		_LittleLongLong = LongLongSwap;
		_BigULongLong = ULongLongNoSwap;
		_LittleULongLong = ULongLongSwap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
		_BigDouble = DoubleNoSwap;
		_LittleDouble = DoubleSwap;
		_BigLongDouble = LongDoubleNoSwap;
		_LittleLongDouble = LongDoubleSwap;
		_BigRevBytes = RevBytesNoSwap;
		_LittleRevBytes = RevBytesSwap;
		_LittleBitField = RevBitFieldSwap;
		_SixtetsForInt = SixtetsForIntBig;
		_IntForSixtets = IntForSixtetsBig;
		_SixtetsForUInt = SixtetsForUIntBig;
		_UIntForSixtets = UIntForSixtetsBig;
		_SixtetsForInt64 = SixtetsForInt64Big;
		_Int64ForSixtets = Int64ForSixtetsBig;
		_SixtetsForUInt64 = SixtetsForUInt64Big;
		_UInt64ForSixtets = UInt64ForSixtetsBig;
	}
}

/*
==========
Swap_IsBigEndian
==========
*/
bool Swap_IsBigEndian() {
	constexpr byte	swaptest[2] = {1,0};
	return *reinterpret_cast<const int16 *>(swaptest) != 1;
}


/*
========================
BreakOnListGrowth

debug tool to find uses of idlist that are dynamically growing
========================
*/
void BreakOnListGrowth() {
}

/*
========================
BreakOnListDefault
========================
*/
void BreakOnListDefault() {
}
