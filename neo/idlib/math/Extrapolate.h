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

#ifndef __MATH_EXTRAPOLATE_H__
#define __MATH_EXTRAPOLATE_H__

#pragma once

/*
==============================================================================================

	Extrapolation

==============================================================================================
*/

typedef enum extrapolation_e : uint8 {
	EXTRAPOLATION_NONE			= 0x01,	// no extrapolation, covered distance = duration * 0.001 * ( baseSpeed )
	EXTRAPOLATION_LINEAR		= 0x02,	// linear extrapolation, covered distance = duration * 0.001 * ( baseSpeed + speed )
	EXTRAPOLATION_ACCELLINEAR	= 0x04,	// linear acceleration, covered distance = duration * 0.001 * ( baseSpeed + 0.5 * speed )
	EXTRAPOLATION_DECELLINEAR	= 0x08,	// linear deceleration, covered distance = duration * 0.001 * ( baseSpeed + 0.5 * speed )
	EXTRAPOLATION_ACCELSINE		= 0x10,	// sinusoidal acceleration, covered distance = duration * 0.001 * ( baseSpeed + sqrt( 0.5 ) * speed )
	EXTRAPOLATION_DECELSINE		= 0x20,	// sinusoidal deceleration, covered distance = duration * 0.001 * ( baseSpeed + sqrt( 0.5 ) * speed )
	EXTRAPOLATION_NOSTOP		= 0x40	// do not stop at startTime + duration
} extrapolation_t;

template< class type >
class idExtrapolate {
public:
						idExtrapolate();

	void				Init( const ID_TIME_T startTime, const ID_TIME_T duration, const type &startValue, const type &baseSpeed, const type &speed, const extrapolation_t extrapolationType );
	type				GetCurrentValue( ID_TIME_T time ) const;
	type				GetCurrentSpeed( ID_TIME_T time ) const;
	[[nodiscard]] bool				IsDone(const ID_TIME_T time ) const { return ( !( extrapolationType & EXTRAPOLATION_NOSTOP ) && time >= startTime + duration ); }
	void				SetStartTime(const ID_TIME_T time ) { startTime = time; }
	[[nodiscard]] ID_TIME_T					GetStartTime() const { return startTime; }
	[[nodiscard]] ID_TIME_T					GetEndTime() const { return ( !( extrapolationType & EXTRAPOLATION_NOSTOP ) && duration > 0 ) ? startTime + duration : 0; }
	[[nodiscard]] ID_TIME_T					GetDuration() const { return duration; }
	void				SetStartValue( const type &value ) { startValue = value; }
	const type &		GetStartValue() const { return startValue; }
	const type &		GetBaseSpeed() const { return baseSpeed; }
	const type &		GetSpeed() const { return speed; }
	[[nodiscard]] extrapolation_t		GetExtrapolationType() const { return extrapolationType; }

private:
	extrapolation_t		extrapolationType;
	ID_TIME_T			startTime;
	ID_TIME_T			duration;
	type				startValue;
	type				baseSpeed;
	type				speed;
};

/*
====================
idExtrapolate::idExtrapolate
====================
*/
template< class type >
ID_INLINE idExtrapolate<type>::idExtrapolate() {
	extrapolationType = EXTRAPOLATION_NONE;
	startTime = duration = 0.0f;
	memset( &startValue, 0, sizeof( startValue ) );
	memset( &baseSpeed, 0, sizeof( baseSpeed ) );
	memset( &speed, 0, sizeof( speed ) );
}

/*
====================
idExtrapolate::Init
====================
*/
template< class type >
ID_INLINE void idExtrapolate<type>::Init( const ID_TIME_T startTime, const ID_TIME_T duration, const type &startValue, const type &baseSpeed, const type &speed, const extrapolation_t extrapolationType ) {
	this->extrapolationType = extrapolationType;
	this->startTime = startTime;
	this->duration = duration;
	this->startValue = startValue;
	this->baseSpeed = baseSpeed;
	this->speed = speed;
}

/*
====================
idExtrapolate::GetCurrentValue
====================
*/
template< class type >
ID_INLINE type idExtrapolate<type>::GetCurrentValue( ID_TIME_T time ) const {
	if ( time < startTime ) {
		return startValue;
	}

	if ( !( extrapolationType &	EXTRAPOLATION_NOSTOP ) && ( time > startTime + duration ) ) {
		time = startTime + duration;
	}

	switch ( extrapolationType & ~EXTRAPOLATION_NOSTOP ) {
		case EXTRAPOLATION_NONE: {
			const float deltaTime = numeric_cast<float>( time - startTime ) * 0.001f;
			return startValue + deltaTime * baseSpeed;
		}
		case EXTRAPOLATION_LINEAR: {
			const float deltaTime = numeric_cast<float>( time - startTime ) * 0.001f;
			return startValue + deltaTime * ( baseSpeed + speed );
		}
		case EXTRAPOLATION_ACCELLINEAR: {
			if ( duration == 0 ) {
				return startValue;
			} else {
				const float deltaTime = numeric_cast<float>( time - startTime ) / numeric_cast<float>(duration);
				const float s = ( 0.5f * deltaTime * deltaTime ) * ( numeric_cast<float>(duration) * 0.001f );
				return startValue + deltaTime * baseSpeed + s * speed;
			}
		}
		case EXTRAPOLATION_DECELLINEAR: {
			if ( duration == 0 ) {
				return startValue;
			} else {
				const float deltaTime = numeric_cast<float>( time - startTime ) / numeric_cast<float>(duration);
				const float s = ( deltaTime - ( 0.5f * deltaTime * deltaTime ) ) * (numeric_cast<float>(duration) * 0.001f );
				return startValue + deltaTime * baseSpeed + s * speed;
			}
		}
		case EXTRAPOLATION_ACCELSINE: {
			if ( duration == 0 ) {
				return startValue;
			} else {
				const float deltaTime = numeric_cast<float>( time - startTime ) / numeric_cast<float>(duration);
				const float s = ( 1.0f - idMath::Cos( deltaTime * idMath::HALF_PI ) ) * numeric_cast<float>(duration) * 0.001f * idMath::SQRT_1OVER2;
				return startValue + deltaTime * baseSpeed + s * speed;
			}
		}
		case EXTRAPOLATION_DECELSINE: {
			if ( duration == 0 ) {
				return startValue;
			} else {
				const float deltaTime = numeric_cast<float>( time - startTime ) / numeric_cast<float>(duration);
				const float s = idMath::Sin( deltaTime * idMath::HALF_PI ) * numeric_cast<float>(duration) * 0.001f * idMath::SQRT_1OVER2;
				return startValue + deltaTime * baseSpeed + s * speed;
			}
		}
	}
	return startValue;
}

/*
====================
idExtrapolate::GetCurrentSpeed
====================
*/
template< class type >
ID_INLINE type idExtrapolate<type>::GetCurrentSpeed( const ID_TIME_T time ) const {
	if ( time < startTime || duration == 0 ) {
		return ( startValue - startValue ); //-V501
	}

	if ( !( extrapolationType &	EXTRAPOLATION_NOSTOP ) && ( time > startTime + duration ) ) {
		return ( startValue - startValue ); //-V501
	}

	switch( extrapolationType & ~EXTRAPOLATION_NOSTOP ) {
		case EXTRAPOLATION_NONE: {
			return baseSpeed;
		}
		case EXTRAPOLATION_LINEAR: {
			return baseSpeed + speed;
		}
		case EXTRAPOLATION_ACCELLINEAR: {
			const float deltaTime = numeric_cast<float>( time - startTime ) / numeric_cast<float>(duration);
			const float s = deltaTime;
			return baseSpeed + s * speed;
		}
		case EXTRAPOLATION_DECELLINEAR: {
			const float deltaTime = numeric_cast<float>( time - startTime ) / numeric_cast<float>(duration);
			const float s = 1.0f - deltaTime;
			return baseSpeed + s * speed;
		}
		case EXTRAPOLATION_ACCELSINE: {
			const float deltaTime = numeric_cast<float>( time - startTime ) / numeric_cast<float>(duration);
			const float s = idMath::Sin( deltaTime * idMath::HALF_PI );
			return baseSpeed + s * speed;
		}
		case EXTRAPOLATION_DECELSINE: {
			const float deltaTime = numeric_cast<float>( time - startTime ) / numeric_cast<float>(duration);
			const float s = idMath::Cos( deltaTime * idMath::HALF_PI );
			return baseSpeed + s * speed;
		}
		default: {
			return baseSpeed;
		}
	}
}

#endif /* !__MATH_EXTRAPOLATE_H__ */
