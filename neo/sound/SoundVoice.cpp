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

#include "../idlib/precompiled.h"

#include "snd_local.h"

idCVar s_subFraction( "s_subFraction", "0.5", CVAR_ARCHIVE | CVAR_FLOAT, "Amount of each sound to send to the LFE channel" );

idVec3 idSoundVoice_Base::speakerPositions[idWaveFile::CHANNEL_INDEX_MAX];
index_t idSoundVoice_Base::speakerLeft[idWaveFile::CHANNEL_INDEX_MAX] = {0 };
index_t idSoundVoice_Base::speakerRight[idWaveFile::CHANNEL_INDEX_MAX] = {0 };
size_t idSoundVoice_Base::dstChannels = 0;
unsigned int idSoundVoice_Base::dstMask = 0;
index_t idSoundVoice_Base::dstCenter = -1;
index_t idSoundVoice_Base::dstLFE = -1;
index_t idSoundVoice_Base::dstMap[MAX_CHANNELS_PER_VOICE] = { 0 };
index_t idSoundVoice_Base::invMap[idWaveFile::CHANNEL_INDEX_MAX] = { 0 };
float idSoundVoice_Base::omniLevel = 1.0f;

/*
========================
idSoundVoice_Base::idSoundVoice_Base
========================
*/
idSoundVoice_Base::idSoundVoice_Base() :
position( 0.0f ),
gain( 1.0f ),
centerChannel( 0.0f ),
pitch( 1.0f ),
innerRadius( 32.0f ),
occlusion( 0.0f ),
channelMask( 0 ),
innerSampleRangeSqr( 0.0f ),
outerSampleRangeSqr( 0.0f )
{
}

/*
========================
idSoundVoice_Base::InitSurround
========================
*/
void idSoundVoice_Base::InitSurround(const size_t outputChannels, const unsigned int channelMask ) {
	static constexpr float UNIT_SPHERE_30_DEGREES       = 0.86602540378443864676372317075294f; // √3/2
	static constexpr float UNIT_SPHERE_45_DEGREES       = 0.70710678118654752440084436210485f;
	static constexpr float UNIT_SPHERE_COS_22_5_DEGREES = 0.92387953251128675612818318939679f; // cos(22.5)
	static constexpr float UNIT_SPHERE_SIN_22_5_DEGREES = 0.3826834323650897717284599840304f;  // sin(22.5)

	speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_LEFT			].Set(  UNIT_SPHERE_30_DEGREES,                                  UNIT_SPHERE_30_DEGREES,                                   0.0f );	                // 30 degrees XY,    base plane Z; used to be 45 degrees XY
	speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT			].Set(  UNIT_SPHERE_30_DEGREES,                                 -UNIT_SPHERE_30_DEGREES,                                   0.0f );	                // 330 degrees XY,   base plane Z; used to be 315 degrees XY
	speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_CENTER			].Set(  0.0f,							                           1.0f,                                                     0.0f );	                // 0 degrees XY,     base plane Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY		].Set(  speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_CENTER].x, speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_CENTER].y,-UNIT_SPHERE_30_DEGREES );	// 0 degrees XY,     below center front Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_LEFT			].Set( -UNIT_SPHERE_30_DEGREES,                                  UNIT_SPHERE_30_DEGREES,                                   0.0f );	                // 135 degrees XY,   base plane Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_RIGHT			].Set( -UNIT_SPHERE_30_DEGREES,                                 -UNIT_SPHERE_30_DEGREES,                                   0.0f );	                // 225 degrees XY,   base plane Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER	].Set(  UNIT_SPHERE_COS_22_5_DEGREES,                            UNIT_SPHERE_SIN_22_5_DEGREES,                             0.0f );	                // 22.5 degrees XY,  base plane Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER	].Set(  UNIT_SPHERE_COS_22_5_DEGREES,                           -UNIT_SPHERE_SIN_22_5_DEGREES,                             0.0f );	                // 337.5 degrees XY, base plane Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_CENTER			].Set( -1.0f,							                           0.0f,                                                     0.0f );	                // 180 degrees XY,   base plane Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_SIDE_LEFT			].Set(  0.0f,						                               1.0f,                                                     0.0f );	                // 90 degrees XY,    base plane Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_SIDE_RIGHT			].Set(  0.0f,					                            	  -1.0f,                                                     0.0f );	                // 270 degrees XY,   base plane Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_TOP_CENTER           ].Set(  0.0f,                                                    0.0f,                                                     1.0f);	                // 0 degrees XY,     directly overhead Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_TOP_FRONT_LEFT       ].Set(  speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_LEFT  ].x, speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_LEFT  ].y, UNIT_SPHERE_45_DEGREES);	// above FL XY,      45 degrees overhead Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_TOP_FRONT_CENTER     ].Set(  speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_CENTER].x, speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_CENTER].y, UNIT_SPHERE_45_DEGREES);	// above FC XY,      45 degrees overhead Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_TOP_FRONT_RIGHT      ].Set(  speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT ].x, speakerPositions[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT ].y, UNIT_SPHERE_45_DEGREES);	// above FR XY,      45 degrees overhead Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_TOP_BACK_LEFT        ].Set(  speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_LEFT   ].x, speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_LEFT   ].y, UNIT_SPHERE_45_DEGREES);	// above BL XY,      45 degrees overhead Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_TOP_BACK_CENTER      ].Set(  speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_CENTER ].x, speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_CENTER ].y, UNIT_SPHERE_45_DEGREES);	// above BC XY,      45 degrees overhead Z
	speakerPositions[idWaveFile::CHANNEL_INDEX_TOP_BACK_RIGHT       ].Set(  speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_RIGHT  ].x, speakerPositions[idWaveFile::CHANNEL_INDEX_BACK_RIGHT  ].y, UNIT_SPHERE_45_DEGREES);	// above BR XY,      45 degrees overhead Z

	// Rotate left one position
	speakerLeft[idWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_LEFT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_FRONT_LEFT] = idWaveFile::CHANNEL_INDEX_SIDE_LEFT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_SIDE_LEFT] = idWaveFile::CHANNEL_INDEX_BACK_LEFT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_BACK_LEFT] = idWaveFile::CHANNEL_INDEX_BACK_CENTER;
	speakerLeft[idWaveFile::CHANNEL_INDEX_BACK_CENTER] = idWaveFile::CHANNEL_INDEX_BACK_RIGHT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_BACK_RIGHT] = idWaveFile::CHANNEL_INDEX_SIDE_RIGHT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_SIDE_RIGHT] = idWaveFile::CHANNEL_INDEX_FRONT_RIGHT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT] = idWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER;
	speakerLeft[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER;

	speakerLeft[idWaveFile::CHANNEL_INDEX_FRONT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_CENTER;
	speakerLeft[idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY] = idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY;

	speakerLeft[idWaveFile::CHANNEL_INDEX_TOP_CENTER] = idWaveFile::CHANNEL_INDEX_TOP_CENTER;
	speakerLeft[idWaveFile::CHANNEL_INDEX_TOP_FRONT_CENTER] = idWaveFile::CHANNEL_INDEX_TOP_FRONT_LEFT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_TOP_FRONT_LEFT] = idWaveFile::CHANNEL_INDEX_TOP_BACK_LEFT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_TOP_BACK_LEFT] = idWaveFile::CHANNEL_INDEX_TOP_BACK_CENTER;
	speakerLeft[idWaveFile::CHANNEL_INDEX_TOP_BACK_CENTER] = idWaveFile::CHANNEL_INDEX_TOP_BACK_RIGHT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_TOP_BACK_RIGHT] = idWaveFile::CHANNEL_INDEX_TOP_FRONT_RIGHT;
	speakerLeft[idWaveFile::CHANNEL_INDEX_TOP_FRONT_RIGHT] = idWaveFile::CHANNEL_INDEX_TOP_FRONT_CENTER;

	// Rotate right one position
	speakerRight[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_RIGHT;
	speakerRight[idWaveFile::CHANNEL_INDEX_FRONT_RIGHT] = idWaveFile::CHANNEL_INDEX_SIDE_RIGHT;
	speakerRight[idWaveFile::CHANNEL_INDEX_SIDE_RIGHT] = idWaveFile::CHANNEL_INDEX_BACK_RIGHT;
	speakerRight[idWaveFile::CHANNEL_INDEX_BACK_RIGHT] = idWaveFile::CHANNEL_INDEX_BACK_CENTER;
	speakerRight[idWaveFile::CHANNEL_INDEX_BACK_CENTER] = idWaveFile::CHANNEL_INDEX_BACK_LEFT;
	speakerRight[idWaveFile::CHANNEL_INDEX_BACK_LEFT] = idWaveFile::CHANNEL_INDEX_SIDE_LEFT;
	speakerRight[idWaveFile::CHANNEL_INDEX_SIDE_LEFT] = idWaveFile::CHANNEL_INDEX_FRONT_LEFT;
	speakerRight[idWaveFile::CHANNEL_INDEX_FRONT_LEFT] = idWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER;
	speakerRight[idWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER;

	speakerRight[idWaveFile::CHANNEL_INDEX_FRONT_CENTER] = idWaveFile::CHANNEL_INDEX_FRONT_CENTER;
	speakerRight[idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY] = idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY;

	speakerRight[idWaveFile::CHANNEL_INDEX_TOP_CENTER] = idWaveFile::CHANNEL_INDEX_TOP_CENTER;
	speakerRight[idWaveFile::CHANNEL_INDEX_TOP_FRONT_CENTER] = idWaveFile::CHANNEL_INDEX_TOP_FRONT_RIGHT;
	speakerRight[idWaveFile::CHANNEL_INDEX_TOP_FRONT_RIGHT] = idWaveFile::CHANNEL_INDEX_TOP_BACK_RIGHT;
	speakerRight[idWaveFile::CHANNEL_INDEX_TOP_BACK_RIGHT] = idWaveFile::CHANNEL_INDEX_TOP_BACK_CENTER;
	speakerRight[idWaveFile::CHANNEL_INDEX_TOP_BACK_CENTER] = idWaveFile::CHANNEL_INDEX_TOP_BACK_LEFT;
	speakerRight[idWaveFile::CHANNEL_INDEX_TOP_BACK_LEFT] = idWaveFile::CHANNEL_INDEX_TOP_FRONT_LEFT;
	speakerRight[idWaveFile::CHANNEL_INDEX_TOP_FRONT_LEFT] = idWaveFile::CHANNEL_INDEX_TOP_FRONT_CENTER;

	dstChannels = outputChannels;
	dstMask = channelMask;

	// dstMap maps a destination channel to a speaker
	// invMap maps a speaker to a destination channel
	dstLFE = -1;
	dstCenter = -1;
	memset( dstMap, 0, sizeof( dstMap ) );
	memset( invMap, 0, sizeof( invMap ) );
	for (index_t i = 0, c = 0; i < idWaveFile::CHANNEL_INDEX_MAX && std::cmp_less(c, MAX_CHANNELS_PER_VOICE); i++ ) {
		if ( dstMask & BIT(i) ) {
			if ( i == idWaveFile::CHANNEL_INDEX_LOW_FREQUENCY ) {
				dstLFE = c;
			}
			if ( i == idWaveFile::CHANNEL_INDEX_FRONT_CENTER ) {
				dstCenter = c;
			}
			dstMap[c] = i;
			invMap[i] = c++;
		} else {
			// Remove this speaker from the chain
			const index_t right = speakerRight[i];
			const index_t left = speakerLeft[i];
			speakerRight[left] = right;
			speakerLeft[right] = left;
		}
	}
	assert( ( dstLFE == -1 ) || ( ( dstMask & idWaveFile::CHANNEL_MASK_LOW_FREQUENCY ) != 0 ) );
	assert( ( dstCenter == -1 ) || ( ( dstMask & idWaveFile::CHANNEL_MASK_FRONT_CENTER ) != 0 ) );

	float omniChannels = numeric_cast<float>(dstChannels);
	if ( dstMask & idWaveFile::CHANNEL_MASK_LOW_FREQUENCY ) {
		omniChannels -= 1.0f;
	}
	if ( dstMask & idWaveFile::CHANNEL_MASK_FRONT_CENTER ) {
		omniChannels -= 1.0f;
	}
	if ( omniChannels > 0.0f ) {
		omniLevel = 1.0f / omniChannels;
	} else {
		// This happens in mono mode
		omniLevel = 1.0f;
	}
}

/*
========================
idSoundVoice_Base::CalculateSurround
========================
*/
void idSoundVoice_Base::CalculateSurround(const size_t srcChannels, float pLevelMatrix[ MAX_CHANNELS_PER_VOICE * MAX_CHANNELS_PER_VOICE ], const float scale ) {
	// Hack for mono
	if ( dstChannels == 1 ) {
		if ( srcChannels == 1 ) {
			pLevelMatrix[ 0 ] = scale;
		} else if ( srcChannels == 2 ) {
			pLevelMatrix[ 0 ] = scale * 0.7071f;
			pLevelMatrix[ 1 ] = scale * 0.7071f;
		}
		return;
	}

#define MATINDEX( src, dst ) ( srcChannels * (dst) + (src) )

	const float subFraction = s_subFraction.GetFloat();

	if ( srcChannels == 1 ) {
		//idVec2 p2 = position.ToVec2();
		idVec3 p2 = position;

		float centerFraction = centerChannel;

		const float sqrLength = p2.LengthSqr();
		if ( sqrLength <= 0.01f ) {
			// If we are on top of the listener, simply route all channels to each speaker equally
			for ( size_t i = 0; i < dstChannels; i++ ) {
				pLevelMatrix[MATINDEX( 0, i )] = omniLevel;
			}
		} else {
			const float invLength = idMath::InvSqrt( sqrLength );
			const float distance = ( invLength * sqrLength );
			p2 *= invLength;

			float spatialize = 1.0f;
			if ( distance < innerRadius ) {
				spatialize = distance / innerRadius;
			}
			float omni = omniLevel * ( 1.0f - spatialize );

			if ( dstCenter != -1 ) {
				centerFraction *= Max( 0.0f, p2.x );
				spatialize *= ( 1.0f - centerFraction );
				omni *= ( 1.0f - centerFraction );
			}

			float channelDots[MAX_CHANNELS_PER_VOICE] = { 0 };
			for ( size_t i = 0; i < dstChannels; i++ ) {
				// Calculate the contribution to each destination channel
				channelDots[i] = speakerPositions[dstMap[i]] * p2;
			}
			// Find the speaker nearest to the sound
			int channelA = 0;
			for ( size_t i = 1; i < dstChannels; i++ ) {
				if ( channelDots[i] > channelDots[channelA] ) {
					channelA = i;
				}
			}
			const index_t speakerA = dstMap[channelA];

			// Find the 2nd nearest speaker
			index_t speakerB = 0;
			const float speakerACross = ( speakerPositions[speakerA].x * p2.y ) - ( speakerPositions[speakerA].y * p2.x );
			if ( speakerACross > 0.0f ) {
				speakerB = speakerLeft[speakerA];
			} else {
				speakerB = speakerRight[speakerA];
			}
			const index_t channelB = invMap[speakerB];

			// Divide the amplitude between the 2 closest speakers
			const float distA = ( speakerPositions[speakerA] - p2 ).Length();
			const float distB = ( speakerPositions[speakerB] - p2 ).Length();
			const float distCinv = 1.0f / ( distA + distB );
			float volumes[MAX_CHANNELS_PER_VOICE] = { 0 };
			volumes[channelA] = ( distB * distCinv );
			volumes[channelB] = ( distA * distCinv );
			for ( size_t i = 0; i < dstChannels; i++ ) {
				pLevelMatrix[MATINDEX( 0, i )] = ( volumes[i] * spatialize ) + omni;
			}
		}
		if ( dstLFE != -1 ) {
			pLevelMatrix[MATINDEX( 0, dstLFE )] = subFraction;
		}
		if ( dstCenter != -1 ) {
			pLevelMatrix[MATINDEX( 0, dstCenter )] = centerFraction;
		}
	} else if ( srcChannels == 2 ) {
		pLevelMatrix[ MATINDEX( 0, 0 ) ] = 1.0f;
		pLevelMatrix[ MATINDEX( 1, 1 ) ] = 1.0f;
		if ( dstLFE != -1 ) {
			pLevelMatrix[ MATINDEX( 0, dstLFE ) ] = subFraction * 0.5f;
			pLevelMatrix[ MATINDEX( 1, dstLFE ) ] = subFraction * 0.5f;
		}
	} else {
		idLib::Warning( "We don't support %d channel sound files", srcChannels );
	}
	for ( size_t i = 0; i < srcChannels * dstChannels; i++ ) {
		pLevelMatrix[ i ] *= scale;
	}
}
