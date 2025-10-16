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
#include <utility>

#include "../precompiled.h"

#include "Simd_Generic.h"
#include "Simd_SSE.h"

static idSIMDProcessor	*	processor = nullptr;			// pointer to SIMD processor
static idSIMDProcessor *	generic = nullptr;				// pointer to generic SIMD implementation
idSIMDProcessor *	SIMDProcessor = nullptr;

/*
================
idSIMD::Init
================
*/
void idSIMD::Init() {
	generic = new (TAG_MATH) idSIMD_Generic;
	generic->cpuid = CPUID_GENERIC;
	processor = nullptr;
	SIMDProcessor = generic;
}

/*
============
idSIMD::InitProcessor
============
*/
void idSIMD::InitProcessor( const char *module, const bool forceGeneric ) {
	idSIMDProcessor *newProcessor = nullptr;

	const cpuid_t cpuid = idLib::sys->GetProcessorId();

	if ( forceGeneric ) {

		newProcessor = generic;

	} else {

		if ( processor == nullptr) {
			if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) ) {
				processor = new (TAG_MATH) idSIMD_SSE;
			} else {
				processor = generic;
			}
			processor->cpuid = cpuid;
		}

		newProcessor = processor;
	}

	if ( newProcessor != SIMDProcessor ) {
		SIMDProcessor = newProcessor;
		idLib::common->Printf( "%s using %s for SIMD processing\n", module, SIMDProcessor->GetName() );
	}

	if ( cpuid & CPUID_FTZ ) {
		idLib::sys->FPU_SetFTZ( true );
		idLib::common->Printf( "enabled Flush-To-Zero mode\n" );
	}

	if ( cpuid & CPUID_DAZ ) {
		idLib::sys->FPU_SetDAZ( true );
		idLib::common->Printf( "enabled Denormals-Are-Zero mode\n" );
	}
}

/*
================
idSIMD::Shutdown
================
*/
void idSIMD::Shutdown() {
	if ( processor != generic ) {
		delete processor;
	}
	delete generic;
	generic = nullptr;
	processor = nullptr;
	SIMDProcessor = nullptr;
}


//===============================================================
//
// Test code
//
//===============================================================

constexpr size_t COUNT = 999;			// data count (odd to catch edge cases)
#define BIG_COUNT	(COUNT*5)		// Some tests need a larger count
constexpr size_t NUMTESTS = 2048;		// number of tests

constexpr auto RANDOM_SEED = 1013904223L;	//((int)idLib::sys->GetClockTicks())

static idSIMDProcessor *p_simd = nullptr;
static idSIMDProcessor *p_generic = nullptr;

#ifdef ID_WIN32
#define TIME_TYPE int
#elif defined (ID_WIN64)
#define TIME_TYPE uint64_t
#endif

static TIME_TYPE baseClocks = 0;

#pragma warning(disable : 4731)     // frame pointer register 'ebx' modified by inline assembly code

static long saved_ebx = 0;

#ifdef ID_WIN32
#define StartRecordTime( start )			\
	__asm mov saved_ebx, ebx				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov start, eax					\
	__asm xor eax, eax						\
	__asm cpuid

#define StopRecordTime( end )				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov end, eax						\
	__asm mov ebx, saved_ebx				\
	__asm xor eax, eax						\
	__asm cpuid
#elif defined (ID_WIN64)
// Read TSC with proper serialization.
// Pattern: LFENCE; RDTSC ... RDTSCP; LFENCE
static inline void StartRecordTime(TIME_TYPE &start) {
	_mm_lfence();                 // serialize before start
	start = __rdtsc();             // t0 (not serializing itself)
}

static inline void StopRecordTime(TIME_TYPE &end) {
	unsigned aux;
	const uint64_t t = __rdtscp(&aux);  // serializing read
	_mm_lfence();                 // keep following loads/stores after
	end = t;
}
#endif // ID_WIN32

#define GetBest( start, end, best )			\
	if ( !(best) || (end) - (start) < (best) ) {	\
		(best) = (end) - (start);					\
	}


/*
============
PrintClocks
============
*/
static void PrintClocks(const char *string, const int dataCount, TIME_TYPE clocks, TIME_TYPE otherClocks = 0 ) {
	idLib::common->Printf( string );
	for ( size_t i = idStr::LengthWithoutColors(string); i < 48; i++ ) {
		idLib::common->Printf(" ");
	}
	clocks -= baseClocks;
	if ( otherClocks && clocks ) {
		otherClocks -= baseClocks;
		const float p = static_cast<float>(otherClocks) / static_cast<float>(clocks);
		idLib::common->Printf( "c = %4d, clcks = %5d, %.1fX\n", dataCount, clocks, p );
	} else {
		idLib::common->Printf( "c = %4d, clcks = %5d\n", dataCount, clocks );
	}
}

/*
============
GetBaseClocks
============
*/
static void GetBaseClocks() {
	TIME_TYPE start = 0, end = 0, bestClocks = 0;

	for ( unsigned short i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	baseClocks = bestClocks;
}

/*
============
TestMinMax
============
*/
static void TestMinMax() {
	int i;
	TIME_TYPE start, end;
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( idVec2 v2src0[COUNT] );
	ALIGN16( idVec3 v3src0[COUNT] );
	ALIGN16( idDrawVert drawVerts[COUNT] );
	ALIGN16( triIndex_t indexes[COUNT] );
	float min = 0.0f, max = 0.0f, min2 = 0.0f, max2 = 0.0f;
	idVec2 v2min, v2max, v2min2, v2max2;
	idVec3 vmin, vmax, vmin2, vmax2;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		v2src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v2src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][2] = srnd.CRandomFloat() * 10.0f;
		drawVerts[i].xyz = v3src0[i];
		indexes[i] = i;
	}

	idLib::common->Printf("====================================\n" );

	TIME_TYPE bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		min = idMath::INFINITY;
		max = -idMath::INFINITY;
		StartRecordTime( start );
		p_generic->MinMax( min, max, fsrc0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( float[] )", COUNT, bestClocksGeneric );

	TIME_TYPE bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( min2, max2, fsrc0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	const char* result = (min == min2 && max == max2) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( v2min, v2max, v2src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idVec2[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( v2min2, v2max2, v2src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( v2min == v2min2 && v2max == v2max2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idVec2[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idDrawVert[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idDrawVert[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, drawVerts, indexes, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( idDrawVert[], indexes[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, drawVerts, indexes, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( idDrawVert[], indexes[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMemcpy
============
*/
static void TestMemcpy() {
	TIME_TYPE start = 0, end = 0;
	int i = 0;
	byte test0[BIG_COUNT] = {};
	byte test1[BIG_COUNT] = {};

	idRandom random( RANDOM_SEED );
	for ( i = 0; i < BIG_COUNT; i++ ) {
		test0[i] = random.RandomInt( 255 );
	}

	idLib::common->Printf("====================================\n" );

	TIME_TYPE bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Memcpy( test1, test0, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Memcpy()", BIG_COUNT, bestClocksGeneric );

	for ( i = 0; i < BIG_COUNT; i++ ) {
		test0[i] = random.RandomInt( 255 );
	}

	TIME_TYPE bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Memcpy( test1, test0, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}
	for ( i = 0; i < BIG_COUNT; i++ ) {
		if ( test1[i] != test0[i] ) {
			break;
		}
	}
	const char* result = (i >= BIG_COUNT) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Memcpy() %s", result), BIG_COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMemset
============
*/
static void TestMemset() {
	TIME_TYPE start, end;
	int i;
	byte test0[BIG_COUNT];

	idRandom random( RANDOM_SEED );
	int j = 1 + random.RandomInt(254);

	idLib::common->Printf("====================================\n" );

	TIME_TYPE bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Memset()", BIG_COUNT, bestClocksGeneric );

	j = 1 + random.RandomInt( 254 );

	TIME_TYPE bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}
	for ( i = 0; i < BIG_COUNT; i++ ) {
		if (std::cmp_not_equal(test0[i], j)) {
			break;
		}
	}
	const char* result = (i >= BIG_COUNT) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Memset() %s", result), BIG_COUNT, bestClocksSIMD, bestClocksGeneric );

	j = 0;

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Memset( 0 )", BIG_COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Memset( test0, j, BIG_COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}
	for ( i = 0; i < BIG_COUNT; i++ ) {
		if (std::cmp_not_equal(test0[i], j)) {
			break;
		}
	}
	result = ( i >= BIG_COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Memset( 0 ) %s", result), BIG_COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestBlendJoints
============
*/
static void TestBlendJoints() {
	size_t i = 0, j = 0;
	TIME_TYPE start = 0, end = 0;
	idTempArray< idJointQuat > baseJoints( COUNT );
	idTempArray< idJointQuat > joints1( COUNT );
	idTempArray< idJointQuat > joints2( COUNT );
	idTempArray< idJointQuat > blendJoints( COUNT );
	idTempArray< size_t > index( COUNT );
	constexpr float lerp = 0.3f;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		idAngles angles = {};
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].w = 0.0f;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		blendJoints[i].q = angles.ToQuat();
		blendJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].w = 0.0f;
		index[i] = i;
	}

	TIME_TYPE bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints1[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_generic->BlendJoints( joints1.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->BlendJoints()", COUNT, bestClocksGeneric );

	TIME_TYPE bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints2[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_simd->BlendJoints( joints2.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) ) {
			break;
		}
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-2f ) ) {
			break;
		}
	}
	const char* result = (i >= COUNT) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->BlendJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestBlendJoints
============
*/
static void TestBlendJointsFast() {
	size_t i = 0, j = 0;
	TIME_TYPE start = 0, end = 0;
	idTempArray< idJointQuat > baseJoints( COUNT );
	idTempArray< idJointQuat > joints1( COUNT );
	idTempArray< idJointQuat > joints2( COUNT );
	idTempArray< idJointQuat > blendJoints( COUNT );
	idTempArray< size_t > index( COUNT );
	constexpr float lerp = 0.3f;

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		idAngles angles = {};
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].w = 0.0f;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		blendJoints[i].q = angles.ToQuat();
		blendJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].w = 0.0f;
		index[i] = i;
	}

	TIME_TYPE bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints1[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_generic->BlendJointsFast( joints1.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->BlendJointsFast()", COUNT, bestClocksGeneric );

	TIME_TYPE bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints2[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_simd->BlendJointsFast( joints2.Ptr(), blendJoints.Ptr(), lerp, index.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) ) {
			break;
		}
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-2f ) ) {
			break;
		}
	}
	const char* result = (i >= COUNT) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->BlendJointsFast() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestConvertJointQuatsToJointMats
============
*/
static void TestConvertJointQuatsToJointMats() {
	size_t i = 0;
	TIME_TYPE start = 0, end = 0;
	idTempArray< idJointQuat > baseJoints( COUNT );
	idTempArray< idJointMat > joints1( COUNT );
	idTempArray< idJointMat > joints2( COUNT );

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		idAngles angles = {};
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
	}

	TIME_TYPE bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ConvertJointQuatsToJointMats( joints1.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ConvertJointQuatsToJointMats()", COUNT, bestClocksGeneric );

	TIME_TYPE bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ConvertJointQuatsToJointMats( joints2.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].Compare( joints2[i], 1e-4f ) ) {
			break;
		}
	}
	const char* result = (i >= COUNT) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ConvertJointQuatsToJointMats() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestConvertJointMatsToJointQuats
============
*/
static void TestConvertJointMatsToJointQuats() {
	size_t i = 0;
	TIME_TYPE start = 0, end = 0;
	idTempArray< idJointMat > baseJoints( COUNT );
	idTempArray< idJointQuat > joints1( COUNT );
	idTempArray< idJointQuat > joints2( COUNT );

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		idAngles angles = {};
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].SetRotation( angles.ToMat3() );
		idVec3 v = {};
		v[0] = srnd.CRandomFloat() * 10.0f;
		v[1] = srnd.CRandomFloat() * 10.0f;
		v[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].SetTranslation( v );
	}

	TIME_TYPE bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ConvertJointMatsToJointQuats( joints1.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ConvertJointMatsToJointQuats()", COUNT, bestClocksGeneric );

	TIME_TYPE bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ConvertJointMatsToJointQuats( joints2.Ptr(), baseJoints.Ptr(), COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-4f ) ) {
			break;
		}
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-4f ) ) {
			break;
		}
	}
	const char* result = (i >= COUNT) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ConvertJointMatsToJointQuats() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestTransformJoints
============
*/
static void TestTransformJoints() {
	size_t i = 0, j = 0;
	TIME_TYPE start, end;
	idTempArray< idJointMat > joints( COUNT+1 );
	idTempArray< idJointMat > joints1( COUNT+1 );
	idTempArray< idJointMat > joints2( COUNT+1 );
	idTempArray< size_t > parents( COUNT+1 );

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i <= COUNT; i++ ) {
		idAngles angles = {};
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		idVec3 v = {};
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
		parents[i] = i - 1;
	}

	TIME_TYPE bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints1[j] = joints[j];
		}
		StartRecordTime( start );
		p_generic->TransformJoints( joints1.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformJoints()", COUNT, bestClocksGeneric );

	TIME_TYPE bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints2[j] = joints[j];
		}
		StartRecordTime( start );
		p_simd->TransformJoints( joints2.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 1; i <= COUNT; i++ ) {
		if ( !joints1[i].Compare( joints2[i], 1e-3f ) ) {
			break;
		}
	}
	const char* result = (i >= COUNT) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestUntransformJoints
============
*/
static void TestUntransformJoints() {
	size_t i = 0, j = 0;
	TIME_TYPE start = 0, end = 0;
	idTempArray< idJointMat > joints( COUNT+1 );
	idTempArray< idJointMat > joints1( COUNT+1 );
	idTempArray< idJointMat > joints2( COUNT+1 );
	idTempArray< size_t > parents( COUNT+1 );

	idRandom srnd( RANDOM_SEED );

	for ( i = 0; i <= COUNT; i++ ) {
		idAngles angles = {};
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		idVec3 v = {};
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
		parents[i] = i - 1;
	}

	TIME_TYPE bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints1[j] = joints[j];
		}
		StartRecordTime( start );
		p_generic->UntransformJoints( joints1.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->UntransformJoints()", COUNT, bestClocksGeneric );

	TIME_TYPE bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints2[j] = joints[j];
		}
		StartRecordTime( start );
		p_simd->UntransformJoints( joints2.Ptr(), parents.Ptr(), 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 1; i <= COUNT; i++ ) {
		if ( !joints1[i].Compare( joints2[i], 1e-3f ) ) {
			break;
		}
	}
	const char* result = (i >= COUNT) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->UntransformJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMath
============
*/
static void TestMath() {
	size_t i = 0;
	TIME_TYPE start = 0, end = 0, bestClocks = 0;

	idLib::common->Printf("====================================\n" );

	float tst = -1.0f;
	float tst2 = 1.0f;
	float testvar = 1.0f;
	idRandom rnd;

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = fabs( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "            fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		int tmp = * reinterpret_cast<int*>(&tst);
		tmp &= 0x7FFFFFFF;
		tst = * reinterpret_cast<float*>(&tmp);
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = 10.0f + 100.0f * rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.01f;
		tst = 10.0f + 100.0f * rnd.RandomFloat();
	}
	PrintClocks( "            sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "    idMath::Sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sqrt16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "  idMath::Sqrt16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Sin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Sin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Sin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Cos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Cos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Cos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Cos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		idMath::SinCos( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::SinCos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		idMath::SinCos16( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "idMath::SinCos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Tan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     idMath::Tan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Tan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   idMath::Tan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ASin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * idMath::ONEOVER_PI;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ASin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ASin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * idMath::ONEOVER_PI;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ASin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ACos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * idMath::ONEOVER_PI;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ACos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ACos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * idMath::ONEOVER_PI;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ACos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ATan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::ATan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::ATan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::ATan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Pow( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Pow( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Pow16( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Pow16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Exp( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Exp( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = idMath::Exp16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Exp16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = idMath::Log( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    idMath::Log( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = idMath::Log16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  idMath::Log16( tst )", 1, bestClocks );

	idLib::common->Printf( "testvar = %f\n", testvar );

	idMat3 resultMat3 = {};
	idQuat resultQuat = {};

	idQuat fromQuat = idAngles( 30, 45, 0 ).ToQuat();
	idQuat toQuat = idAngles( 45, 0, 0 ).ToQuat();
	idCQuat cq = idAngles( 30, 45, 0 ).ToQuat().ToCQuat();
	idAngles ang = idAngles( 30, 40, 50 );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultMat3 = fromQuat.ToMat3();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "       idQuat::ToMat3()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat.Slerp( fromQuat, toQuat, 0.3f );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "        idQuat::Slerp()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat = cq.ToQuat();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "      idCQuat::ToQuat()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat = ang.ToQuat();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "     idAngles::ToQuat()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultMat3 = ang.ToMat3();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "     idAngles::ToMat3()", 1, bestClocks );
}

/*
============
idSIMD::Test_f
============
*/
void idSIMD::Test_f( const idCmdArgs &args ) {

	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );

	p_simd = processor;
	p_generic = generic;

	if ( idStr::Length( args.Argv( 1 ) ) != 0 ) {
		const cpuid_t cpuid = idLib::sys->GetProcessorId();
		idStr argString = args.Args();

		argString.Replace( " ", "" );

		if ( idStr::Icmp( argString, "SSE" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_SSE ) ) {
				common->Printf( "CPU does not support MMX & SSE\n" );
				return;
			}
			p_simd = new (TAG_MATH) idSIMD_SSE;
		} else {
			common->Printf( "invalid argument, use: MMX, 3DNow, SSE, SSE2, SSE3, AltiVec\n" );
			return;
		}
	}

	idLib::common->SetRefreshOnPrint( true );

	idLib::common->Printf( "using %s for SIMD processing\n", p_simd->GetName() );

	GetBaseClocks();

	TestMath();
	TestMinMax();
	TestMemcpy();
	TestMemset();

	idLib::common->Printf("====================================\n" );

	TestBlendJoints();
	TestBlendJointsFast();
	TestConvertJointQuatsToJointMats();
	TestConvertJointMatsToJointQuats();
	TestTransformJoints();
	TestUntransformJoints();

	idLib::common->Printf("====================================\n" );

	idLib::common->SetRefreshOnPrint( false );

	if ( p_simd != processor ) {
		delete p_simd;
	}
	p_simd = nullptr;
	p_generic = nullptr;

	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
}
