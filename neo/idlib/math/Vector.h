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

#ifndef __MATH_VECTOR_H__
#define __MATH_VECTOR_H__

#pragma once

/*
===============================================================================

  Vector classes

===============================================================================
*/

#include <cmath>

#include "../containers/Array.h" // for idTupleSize

constexpr auto VECTOR_EPSILON = 0.001f;

class idAngles;
class idPolar3;
class idMat3;

//===============================================================
//
//	idVec2 - 2D vector
//
//===============================================================

class idVec2 {
public:
	float			x;
	float			y;

					idVec2() noexcept = default;
					explicit idVec2(float x, float y );

	void 			Set(float x, float y );
	void			Zero();

	float			operator[]( const Ordinal auto index ) const;
	float &			operator[]( const Ordinal auto index );
	idVec2			operator-() const;
	float			operator*( const idVec2 &a ) const;
	idVec2			operator*( float a ) const;
	idVec2			operator*( const std::integral auto a ) const;
	idVec2			operator/( float a ) const;
	idVec2			operator/( const std::integral auto a ) const;
	idVec2			operator+( const idVec2 &a ) const;
	idVec2			operator-( const idVec2 &a ) const;
	idVec2 &		operator+=( const idVec2 &a );
	idVec2 &		operator-=( const idVec2 &a );
	idVec2 &		operator/=( const idVec2 &a );
	idVec2 &		operator/=( float a );
	idVec2 &        operator/=( const std::integral auto a );
	idVec2 &		operator*=( float a );
	idVec2 &        operator*=( const std::integral auto a );

	friend idVec2	operator*( float a, idVec2 b );

	[[nodiscard]] idVec2			Scale( const idVec2 &a ) const;

	[[nodiscard]] bool			Compare( const idVec2 &a ) const;							// exact compare, no epsilon
	[[nodiscard]] bool			Compare( const idVec2 &a, float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idVec2 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idVec2 &a ) const;						// exact compare, no epsilon

	[[nodiscard]] float			Length() const;
	[[nodiscard]] float			LengthFast() const;
	[[nodiscard]] float			LengthSqr() const;
	float			Normalize();			// returns length
	float			NormalizeFast();		// returns length
	[[nodiscard]] idVec2			Truncate( float length ) const;	// cap length
	void			Clamp( const idVec2 &min, const idVec2 &max );
	void			Snap();				// snap to closest integer value
	void			SnapInt();			// snap towards integer (floor)

	[[nodiscard]] static size_t	GetDimension();

	[[nodiscard]] const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	[[nodiscard]] const char *	ToString( const size_t precision = 2 ) const;

	void			Lerp( const idVec2 &v1, const idVec2 &v2, float l );
};

extern idVec2 vec2_origin;
#define vec2_zero vec2_origin

ID_INLINE idVec2::idVec2( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

ID_INLINE void idVec2::Set( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

ID_INLINE void idVec2::Zero() {
	x = y = 0.0f;
}

ID_INLINE bool idVec2::Compare( const idVec2 &a ) const {
	return ( (std::equal_to<>()(x, a.x) ) && (std::equal_to<>()(y, a.y) ) );
}

ID_INLINE bool idVec2::Compare( const idVec2 &a, const float epsilon ) const {
	if ( idMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}
			
	if ( idMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool idVec2::operator==( const idVec2 &a ) const {
	return Compare( a );
}

ID_INLINE bool idVec2::operator!=( const idVec2 &a ) const {
	return !Compare( a );
}


float idVec2::operator[](const Ordinal auto index ) const {
	ORDINAL_CHECK(index, 2);
	return ( &x )[ index ];
}


float& idVec2::operator[](const Ordinal auto index ) {
	ORDINAL_CHECK(index, 2);
	return ( &x )[ index ];
}

ID_INLINE float idVec2::Length() const {
	return idMath::Sqrt( x * x + y * y );
}

ID_INLINE float idVec2::LengthFast() const {
	const float sqrLength = x * x + y * y;
	return sqrLength * idMath::InvSqrt( sqrLength );
}

ID_INLINE float idVec2::LengthSqr() const {
	return ( x * x + y * y );
}

ID_INLINE float idVec2::Normalize() {
	const float sqrLength = x * x + y * y;
	const float invLength = idMath::InvSqrt(sqrLength);
	x *= invLength;
	y *= invLength;
	return invLength * sqrLength;
}

ID_INLINE float idVec2::NormalizeFast() {
	const float lengthSqr = x * x + y * y;
	const float invLength = idMath::InvSqrt(lengthSqr);
	x *= invLength;
	y *= invLength;
	return invLength * lengthSqr;
}

ID_INLINE idVec2 idVec2::Truncate(const float length ) const {
	if ( length < idMath::FLT_SMALLEST_NON_DENORMAL ) {
		return vec2_zero;
	} else {
		const float length2 = LengthSqr();
		if ( length2 > length * length ) {
			const float ilength = length * idMath::InvSqrt( length2 );
			return *this * ilength;
		}
	}
	return *this;
}

ID_INLINE void idVec2::Clamp( const idVec2 &min, const idVec2 &max ) {
	if ( x < min.x ) {
		x = min.x;
	} else if ( x > max.x ) {
		x = max.x;
	}
	if ( y < min.y ) {
		y = min.y;
	} else if ( y > max.y ) {
		y = max.y;
	}
}

ID_INLINE void idVec2::Snap() {
	x = std::floor( x + 0.5f );
	y = std::floor( y + 0.5f );
}

ID_INLINE void idVec2::SnapInt() {
	x = static_cast<float>(static_cast<int>(x));
	y = static_cast<float>(static_cast<int>(y));
}

ID_INLINE idVec2 idVec2::operator-() const {
	return idVec2( -x, -y );
}
	
ID_INLINE idVec2 idVec2::operator-( const idVec2 &a ) const {
	return idVec2( x - a.x, y - a.y );
}

ID_INLINE float idVec2::operator*( const idVec2 &a ) const {
	return x * a.x + y * a.y;
}

ID_INLINE idVec2 idVec2::operator*( const float a ) const {
	return idVec2( x * a, y * a );
}

idVec2 idVec2::operator*( const std::integral auto a ) const {
	return operator*(numeric_cast<float>(a));
}

ID_INLINE idVec2 idVec2::operator/( const float a ) const {
	const float inva = 1.0f / a;
	return idVec2( x * inva, y * inva );
}

idVec2 idVec2::operator/( const std::integral auto a ) const {
	return operator/(numeric_cast<float>(a));
}

ID_INLINE idVec2 operator*( const float a, const idVec2 b ) {
	return idVec2( b.x * a, b.y * a );
}

ID_INLINE idVec2 idVec2::operator+( const idVec2 &a ) const {
	return idVec2( x + a.x, y + a.y );
}

ID_INLINE idVec2 &idVec2::operator+=( const idVec2 &a ) {
	x += a.x;
	y += a.y;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator/=( const idVec2 &a ) {
	x /= a.x;
	y /= a.y;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator/=( const float a ) {
	const float inva = 1.0f / a;
	x *= inva;
	y *= inva;

	return *this;
}

idVec2 &idVec2::operator/=( const std::integral auto a ) {
	return &operator/=(numeric_cast<float>(a));
}

ID_INLINE idVec2 &idVec2::operator-=( const idVec2 &a ) {
	x -= a.x;
	y -= a.y;

	return *this;
}

ID_INLINE idVec2 &idVec2::operator*=( const float a ) {
	x *= a;
	y *= a;

	return *this;
}

idVec2 &idVec2::operator*=( const std::integral auto a ) {
	return operator*=(numeric_cast<float>(a));
}

ID_INLINE idVec2 idVec2::Scale( const idVec2 &a ) const {
	return idVec2( x * a.x, y * a.y );
}

ID_INLINE size_t idVec2::GetDimension()
{
	return 2;
}

ID_INLINE const float *idVec2::ToFloatPtr() const {
	return &x;
}

ID_INLINE float *idVec2::ToFloatPtr() {
	return &x;
}


//===============================================================
//
//	idVec3 - 3D vector
//
//===============================================================

class idVec3 {
public:	
	float			x;
	float			y;
	float			z;

					idVec3() noexcept = default;
					explicit idVec3( const float xyz ) { Set( xyz, xyz, xyz ); }
					explicit idVec3(float x, float y, float z );

	void 			Set(float x, float y, float z );
	void			Zero();

	float			operator[]( const Ordinal auto index ) const;
	float &			operator[]( const Ordinal auto index );
	idVec3			operator-() const;
	idVec3 &		operator=( const idVec3 &a );		// required because of a MSVC 6 & 7 bug
	float			operator*( const idVec3 &a ) const;
	idVec3			operator*( float a ) const;
	idVec3			operator*( const std::integral auto a ) const;
	idVec3			operator/( float a ) const;
	idVec3			operator/( const std::integral auto a ) const;
	idVec3			operator+( const idVec3 &a ) const;
	idVec3			operator-( const idVec3 &a ) const;
	idVec3 &		operator+=( const idVec3 &a );
	idVec3 &		operator-=( const idVec3 &a );
	idVec3 &		operator/=( const idVec3 &a );
	idVec3 &		operator/=( float a );
	idVec3 &        operator/=( const std::integral auto a );
	idVec3 &		operator*=( float a );
	idVec3 &        operator*=( const std::integral auto a );

	friend idVec3	operator*(float a, const idVec3& b );

	[[nodiscard]] bool			Compare( const idVec3 &a ) const;							// exact compare, no epsilon
	[[nodiscard]] bool			Compare( const idVec3 &a, float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idVec3 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idVec3 &a ) const;						// exact compare, no epsilon

	bool			FixDegenerateNormal();	// fix degenerate axial cases
	bool			FixDenormals();			// change tiny numbers to zero

	[[nodiscard]] idVec3			Cross( const idVec3 &a ) const;
	idVec3 &		Cross( const idVec3 &a, const idVec3 &b );
	[[nodiscard]] float			Length() const;
	[[nodiscard]] float			LengthSqr() const;
	[[nodiscard]] float			LengthFast() const;
	float			Normalize();				// returns length
	float			NormalizeFast();			// returns length
	[[nodiscard]] idVec3			Truncate( float length ) const;		// cap length
	void			Clamp( const idVec3 &min, const idVec3 &max );
	void			Snap();					// snap to closest integer value
	void			SnapInt();				// snap towards integer (floor)

	static size_t	GetDimension();

	[[nodiscard]] float			ToYaw() const;
	[[nodiscard]] float			ToPitch() const;
	[[nodiscard]] idAngles		ToAngles() const;
	[[nodiscard]] idPolar3		ToPolar() const;
	[[nodiscard]] idMat3			ToMat3() const;		// vector should be normalized
	[[nodiscard]] const idVec2 &	ToVec2() const;
	idVec2 &		ToVec2();
	[[nodiscard]] const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	[[nodiscard]] const char *	ToString( const size_t precision = 2 ) const;

	void			NormalVectors( idVec3 &left, idVec3 &down ) const;	// vector should be normalized
	void			OrthogonalBasis( idVec3 &left, idVec3 &up ) const;

	void			ProjectOntoPlane( const idVec3 &normal, float overBounce = 1.0f );
	bool			ProjectAlongPlane( const idVec3 &normal, float epsilon, float overBounce = 1.0f );
	void			ProjectSelfOntoSphere(float radius );

	void			Lerp( const idVec3 &v1, const idVec3 &v2, float l );
	void			SLerp( const idVec3 &v1, const idVec3 &v2, float l );
};

extern idVec3 vec3_origin;
#define vec3_zero vec3_origin

ID_INLINE idVec3::idVec3( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}


float idVec3::operator[](const Ordinal auto index ) const {
	ORDINAL_CHECK(index, 3);
	return ( &x )[ index ];
}


float &idVec3::operator[](const Ordinal auto index ) {
	ORDINAL_CHECK(index, 3);
	return ( &x )[ index ];
}

ID_INLINE void idVec3::Set( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

ID_INLINE void idVec3::Zero() {
	x = y = z = 0.0f;
}

ID_INLINE idVec3 idVec3::operator-() const {
	return idVec3( -x, -y, -z );
}

ID_INLINE idVec3 &idVec3::operator=( const idVec3 &a )
= default;

ID_INLINE idVec3 idVec3::operator-( const idVec3 &a ) const {
	return idVec3( x - a.x, y - a.y, z - a.z );
}

ID_INLINE float idVec3::operator*( const idVec3 &a ) const {
	return x * a.x + y * a.y + z * a.z;
}

ID_INLINE idVec3 idVec3::operator*( const float a ) const {
	return idVec3( x * a, y * a, z * a );
}

ID_INLINE idVec3 idVec3::operator*( const std::integral auto a ) const {
	return operator*(numeric_cast<float>(a));
}

ID_INLINE idVec3 idVec3::operator/( const float a ) const {
	const float inva = 1.0f / a;
	return idVec3( x * inva, y * inva, z * inva );
}

ID_INLINE idVec3 idVec3::operator/( const std::integral auto a ) const {
	return operator/(numeric_cast<float>(a));
}

ID_INLINE idVec3 operator*( const float a, const idVec3& b ) {
	return idVec3( b.x * a, b.y * a, b.z * a );
}

ID_INLINE idVec3 operator/( const float a, const idVec3& b ) {
	return idVec3( a / b.x, a / b.y, a / b.z );
}

ID_INLINE idVec3 idVec3::operator+( const idVec3 &a ) const {
	return idVec3( x + a.x, y + a.y, z + a.z );
}

ID_INLINE idVec3 &idVec3::operator+=( const idVec3 &a ) {
	x += a.x;
	y += a.y;
	z += a.z;

	return *this;
}

ID_INLINE idVec3 &idVec3::operator/=( const idVec3 &a ) {
	x /= a.x;
	y /= a.y;
	z /= a.z;

	return *this;
}

ID_INLINE idVec3 &idVec3::operator/=( const float a ) {
	const float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	z *= inva;

	return *this;
}

idVec3& idVec3::operator/=( const std::integral auto a ) {
	return operator/=(numeric_cast<float>(a));
}

ID_INLINE idVec3 &idVec3::operator-=( const idVec3 &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;

	return *this;
}

ID_INLINE idVec3 &idVec3::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;

	return *this;
}

idVec3& idVec3::operator*=( const std::integral auto a ) {
	return operator*=(numeric_cast<float>(a));
}

ID_INLINE bool idVec3::Compare( const idVec3 &a ) const {
	return ( (std::equal_to<>()(x, a.x) ) && (std::equal_to<>()(y, a.y) ) && (std::equal_to<>()(z, a.z) ) );
}

ID_INLINE bool idVec3::Compare( const idVec3 &a, const float epsilon ) const {
	if ( idMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}
			
	if ( idMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool idVec3::operator==( const idVec3 &a ) const {
	return Compare( a );
}

ID_INLINE bool idVec3::operator!=( const idVec3 &a ) const {
	return !Compare( a );
}

ID_INLINE float idVec3::NormalizeFast() {
	const float sqrLength = x * x + y * y + z * z;
	const float invLength = idMath::InvSqrt(sqrLength);
	x *= invLength;
	y *= invLength;
	z *= invLength;
	return invLength * sqrLength;
}

ID_INLINE bool idVec3::FixDegenerateNormal() {
	if ( x == 0.0f ) {
		if ( y == 0.0f ) {
			if ( z > 0.0f ) {
				if ( z != 1.0f ) {
					z = 1.0f;
					return true;
				}
			} else {
				if ( z != -1.0f ) {
					z = -1.0f;
					return true;
				}
			}
			return false;
		} else if ( z == 0.0f ) {
			if ( y > 0.0f ) {
				if ( y != 1.0f ) {
					y = 1.0f;
					return true;
				}
			} else {
				if ( y != -1.0f ) {
					y = -1.0f;
					return true;
				}
			}
			return false;
		}
	} else if ( y == 0.0f ) {
		if ( z == 0.0f ) {
			if ( x > 0.0f ) {
				if ( x != 1.0f ) {
					x = 1.0f;
					return true;
				}
			} else {
				if ( x != -1.0f ) {
					x = -1.0f;
					return true;
				}
			}
			return false;
		}
	}
	if ( idMath::Fabs( x ) == 1.0f ) {
		if ( y != 0.0f || z != 0.0f ) {
			y = z = 0.0f;
			return true;
		}
		return false;
	} else if ( idMath::Fabs( y ) == 1.0f ) {
		if ( x != 0.0f || z != 0.0f ) {
			x = z = 0.0f;
			return true;
		}
		return false;
	} else if ( idMath::Fabs( z ) == 1.0f ) {
		if ( x != 0.0f || y != 0.0f ) {
			x = y = 0.0f;
			return true;
		}
		return false;
	}
	return false;
}

ID_INLINE bool idVec3::FixDenormals() {
	bool denormal = false;
	if (std::fabs( x ) < 1e-30f ) {
		x = 0.0f;
		denormal = true;
	}
	if (std::fabs( y ) < 1e-30f ) {
		y = 0.0f;
		denormal = true;
	}
	if (std::fabs( z ) < 1e-30f ) {
		z = 0.0f;
		denormal = true;
	}
	return denormal;
}

ID_INLINE idVec3 idVec3::Cross( const idVec3 &a ) const {
	return idVec3( y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x );
}

ID_INLINE idVec3 &idVec3::Cross( const idVec3 &a, const idVec3 &b ) {
	x = a.y * b.z - a.z * b.y;
	y = a.z * b.x - a.x * b.z;
	z = a.x * b.y - a.y * b.x;

	return *this;
}

ID_INLINE float idVec3::Length() const {
	return idMath::Sqrt( x * x + y * y + z * z );
}

ID_INLINE float idVec3::LengthSqr() const {
	return ( x * x + y * y + z * z );
}

ID_INLINE float idVec3::LengthFast() const {
	const float sqrLength = x * x + y * y + z * z;
	return sqrLength * idMath::InvSqrt( sqrLength );
}

ID_INLINE float idVec3::Normalize() {
	const float sqrLength = x * x + y * y + z * z;
	const float invLength = idMath::InvSqrt(sqrLength);
	x *= invLength;
	y *= invLength;
	z *= invLength;
	return invLength * sqrLength;
}

ID_INLINE idVec3 idVec3::Truncate( const float length ) const {
	if ( length < idMath::FLT_SMALLEST_NON_DENORMAL ) {
		return vec3_zero;
	} else {
		const float length2 = LengthSqr();
		if ( length2 > length * length ) {
			const float ilength = length * idMath::InvSqrt( length2 );
			return *this * ilength;
		}
	}
	return *this;
}

ID_INLINE void idVec3::Clamp( const idVec3 &min, const idVec3 &max ) {
	if ( x < min.x ) {
		x = min.x;
	} else if ( x > max.x ) {
		x = max.x;
	}
	if ( y < min.y ) {
		y = min.y;
	} else if ( y > max.y ) {
		y = max.y;
	}
	if ( z < min.z ) {
		z = min.z;
	} else if ( z > max.z ) {
		z = max.z;
	}
}

ID_INLINE void idVec3::Snap() {
	x = std::floor( x + 0.5f );
	y = std::floor( y + 0.5f );
	z = std::floor( z + 0.5f );
}

ID_INLINE void idVec3::SnapInt() {
	x = static_cast<float>(static_cast<int>(x));
	y = static_cast<float>(static_cast<int>(y));
	z = static_cast<float>(static_cast<int>(z));
}

ID_INLINE size_t idVec3::GetDimension()
{
	return 3;
}

ID_INLINE const idVec2 &idVec3::ToVec2() const {
	return *reinterpret_cast<const idVec2 *>(this);
}

ID_INLINE idVec2 &idVec3::ToVec2() {
	return *reinterpret_cast<idVec2 *>(this);
}

ID_INLINE const float *idVec3::ToFloatPtr() const {
	return &x;
}

ID_INLINE float *idVec3::ToFloatPtr() {
	return &x;
}

ID_INLINE void idVec3::NormalVectors( idVec3 &left, idVec3 &down ) const {
	float d = x * x + y * y;
	if ( !(d == 0.0f) ) {
		left[0] = 1.0f;
		left[1] = 0.0f;
		left[2] = 0.0f;
	} else {
		d = idMath::InvSqrt( d );
		left[0] = -y * d;
		left[1] = x * d;
		left[2] = 0.0f;
	}
	down = left.Cross( *this );
}

ID_INLINE void idVec3::OrthogonalBasis( idVec3 &left, idVec3 &up ) const {
	float l = 0.0f, s = 0.0f;

	if ( idMath::Fabs( z ) > 0.7f ) {
		l = y * y + z * z;
		s = idMath::InvSqrt( l );
		up[0] = 0.0f;
		up[1] = z * s;
		up[2] = -y * s;
		left[0] = l * s;
		left[1] = -x * up[2];
		left[2] = x * up[1];
	}
	else {
		l = x * x + y * y;
		s = idMath::InvSqrt( l );
		left[0] = -y * s;
		left[1] = x * s;
		left[2] = 0.0f;
		up[0] = -z * left[1];
		up[1] = z * left[0];
		up[2] = l * s;
	}
}

ID_INLINE void idVec3::ProjectOntoPlane( const idVec3 &normal, const float overBounce ) {
	float backoff = *this * normal;
	
	if ( overBounce != 1.0 ) {
		if ( backoff < 0 ) {
			backoff *= overBounce;
		} else {
			backoff /= overBounce;
		}
	}

	*this -= backoff * normal;
}

ID_INLINE bool idVec3::ProjectAlongPlane( const idVec3 &normal, const float epsilon, const float overBounce ) {
	idVec3 cross = this->Cross(normal).Cross((*this));
	// normalize so a fixed epsilon can be used
	cross.Normalize();
	const float len = normal * cross;
	if ( idMath::Fabs( len ) < epsilon ) {
		return false;
	}
	cross *= overBounce * ( normal * (*this) ) / len;
	(*this) -= cross;
	return true;
}

//===============================================================
//
//	idTupleSize< idVec3 > - Specialization to get the size
//	of an idVec3 generically.
//
//===============================================================

template<>
struct idTupleSize< idVec3 > {
	enum { value = 3 };
};

//===============================================================
//
//	idVec4 - 4D vector
//
//===============================================================

class idVec4 {
public:	
	float			x;
	float			y;
	float			z;
	float			w;

					idVec4() noexcept = default;
	explicit        idVec4( const float x ) { Set( x, x, x, x ); }
					explicit idVec4( const float x, const float y, const float z, const float w ) { Set( x, y, z, w ); }

	void 			Set(float x, float y, float z, float w );
	void			Zero();

	float			operator[]( const Ordinal auto index ) const;
	float &			operator[]( const Ordinal auto index );
	idVec4			operator-() const;
	float			operator*( const idVec4 &a ) const;
	idVec4			operator*( float a ) const;
	idVec4			operator*( const std::integral auto a ) const;
	idVec4			operator/( float a ) const;
	idVec4			operator/( const std::integral auto a ) const;
	idVec4			operator+( const idVec4 &a ) const;
	idVec4			operator-( const idVec4 &a ) const;
	idVec4 &		operator+=( const idVec4 &a );
	idVec4 &		operator-=( const idVec4 &a );
	idVec4 &		operator/=( const idVec4 &a );
	idVec4 &		operator/=( float a );
	idVec4 &        operator/=( const std::integral auto a );
	idVec4 &		operator*=( float a );
	idVec4 &        operator*=( const std::integral auto a );

	friend idVec4	operator*(float a, idVec4 b );

	[[nodiscard]] idVec4			Multiply( const idVec4 & a ) const;

	[[nodiscard]] bool			Compare( const idVec4 &a ) const;							// exact compare, no epsilon
	[[nodiscard]] bool			Compare( const idVec4 &a, float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idVec4 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idVec4 &a ) const;						// exact compare, no epsilon

	[[nodiscard]] float			Length() const;
	[[nodiscard]] float			LengthSqr() const;
	float			Normalize();			// returns length
	float			NormalizeFast();		// returns length

	[[nodiscard]] static size_t	GetDimension();

	[[nodiscard]] const idVec2 &	ToVec2() const;
	idVec2 &		ToVec2();
	[[nodiscard]] const idVec3 &	ToVec3() const;
	idVec3 &		ToVec3();
	[[nodiscard]] const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	[[nodiscard]] const char *	ToString( const size_t precision = 2 ) const;

	void			Lerp( const idVec4 &v1, const idVec4 &v2, float l );
};

extern idVec4 vec4_origin;
#define vec4_zero vec4_origin

ID_INLINE void idVec4::Set( const float x, const float y, const float z, const float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ID_INLINE void idVec4::Zero() {
	x = y = z = w = 0.0f;
}


ID_INLINE float idVec4::operator[]( const Ordinal auto index ) const {
	ORDINAL_CHECK(index, 4);
	return ( &x )[ index ];
}


ID_INLINE float& idVec4::operator[]( const Ordinal auto index ) {
	ORDINAL_CHECK(index, 4);
	return ( &x )[ index ];
}

ID_INLINE idVec4 idVec4::operator-() const {
	return idVec4( -x, -y, -z, -w );
}

ID_INLINE idVec4 idVec4::operator-( const idVec4 &a ) const {
	return idVec4( x - a.x, y - a.y, z - a.z, w - a.w );
}

ID_INLINE float idVec4::operator*( const idVec4 &a ) const {
	return x * a.x + y * a.y + z * a.z + w * a.w;
}

ID_INLINE idVec4 idVec4::operator*( const float a ) const {
	return idVec4( x * a, y * a, z * a, w * a );
}

idVec4 idVec4::operator*( const std::integral auto a ) const {
	return operator*(numeric_cast<float>(a));
}

ID_INLINE idVec4 idVec4::operator/( const float a ) const {
	const float inva = 1.0f / a;
	return idVec4( x * inva, y * inva, z * inva, w * inva );
}

idVec4 idVec4::operator/( const std::integral auto a ) const {
	return operator/(numeric_cast<float>(a));
}

ID_INLINE idVec4 operator*( const float a, const idVec4 b ) {
	return idVec4( b.x * a, b.y * a, b.z * a, b.w * a );
}

ID_INLINE idVec4 idVec4::operator+( const idVec4 &a ) const {
	return idVec4( x + a.x, y + a.y, z + a.z, w + a.w );
}

ID_INLINE idVec4 &idVec4::operator+=( const idVec4 &a ) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

ID_INLINE idVec4 &idVec4::operator/=( const idVec4 &a ) {
	x /= a.x;
	y /= a.y;
	z /= a.z;
	w /= a.w;

	return *this;
}

ID_INLINE idVec4 &idVec4::operator/=( const float a ) {
	const float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	z *= inva;
	w *= inva;

	return *this;
}

idVec4 &idVec4::operator/=( const std::integral auto a ) {
	return operator/=(numeric_cast<float>(a));
}

ID_INLINE idVec4 &idVec4::operator-=( const idVec4 &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

ID_INLINE idVec4 &idVec4::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

idVec4 &idVec4::operator*=( const std::integral auto a ) {
	return operator*=(numeric_cast<float>(a));
}

ID_INLINE idVec4 idVec4::Multiply( const idVec4 & a ) const {
	return idVec4( x * a.x, y * a.y, z * a.z, w * a.w );
}

ID_INLINE bool idVec4::Compare( const idVec4 &a ) const {
	return ( (std::equal_to<>()(x, a.x) ) && (std::equal_to<>()(y, a.y) ) && (std::equal_to<>()(z, a.z) ) && std::equal_to<>()(w, a.w) );
}

ID_INLINE bool idVec4::Compare( const idVec4 &a, const float epsilon ) const {
	if ( idMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}
			
	if ( idMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( w - a.w ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool idVec4::operator==( const idVec4 &a ) const {
	return Compare( a );
}

ID_INLINE bool idVec4::operator!=( const idVec4 &a ) const {
	return !Compare( a );
}

ID_INLINE float idVec4::Length() const {
	return idMath::Sqrt( x * x + y * y + z * z + w * w );
}

ID_INLINE float idVec4::LengthSqr() const {
	return ( x * x + y * y + z * z + w * w );
}

ID_INLINE float idVec4::Normalize() {
	const float sqrLength = x * x + y * y + z * z + w * w;
	const float invLength = idMath::InvSqrt(sqrLength);
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

ID_INLINE float idVec4::NormalizeFast() {
	const float sqrLength = x * x + y * y + z * z + w * w;
	const float invLength = idMath::InvSqrt(sqrLength);
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

ID_INLINE size_t idVec4::GetDimension()
{
	return 4;
}

ID_INLINE const idVec2 &idVec4::ToVec2() const {
	return *reinterpret_cast<const idVec2 *>(this);
}

ID_INLINE idVec2 &idVec4::ToVec2() {
	return *reinterpret_cast<idVec2 *>(this);
}

ID_INLINE const idVec3 &idVec4::ToVec3() const {
	return *reinterpret_cast<const idVec3 *>(this);
}

ID_INLINE idVec3 &idVec4::ToVec3() {
	return *reinterpret_cast<idVec3 *>(this);
}

ID_INLINE const float *idVec4::ToFloatPtr() const {
	return &x;
}

ID_INLINE float *idVec4::ToFloatPtr() {
	return &x;
}


//===============================================================
//
//	idVec5 - 5D vector
//
//===============================================================

class idVec5 {
public:
	float			x = 0.0f;
	float			y = 0.0f;
	float			z = 0.0f;
	float			s = 0.0f;
	float			t = 0.0f;

					idVec5() noexcept;
					explicit idVec5( const idVec3 &xyz, const idVec2 &st );
					explicit idVec5(float x, float y, float z, float s, float t );

	float			operator[]( const Ordinal auto index ) const;
	float &			operator[]( const Ordinal auto index );
	idVec5 &		operator=( const idVec3 &a );

	[[nodiscard]] static size_t	GetDimension();

	[[nodiscard]] const idVec3 &	ToVec3() const;
	idVec3 &		ToVec3();
	[[nodiscard]] const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	[[nodiscard]] const char *	ToString( size_t precision = 2 ) const;

	void			Lerp( const idVec5 &v1, const idVec5 &v2, float l );
};

extern idVec5 vec5_origin;
#define vec5_zero vec5_origin

ID_INLINE idVec5::idVec5() noexcept = default;

ID_INLINE idVec5::idVec5( const idVec3 &xyz, const idVec2 &st ) {
	x = xyz.x;
	y = xyz.y;
	z = xyz.z;
	s = st[0];
	t = st[1];
}

ID_INLINE idVec5::idVec5( const float x, const float y, const float z, const float s, const float t ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->s = s;
	this->t = t;
}


float idVec5::operator[](const Ordinal auto index ) const {
	ORDINAL_CHECK(index, 5);
	return ( &x )[ index ];
}


float& idVec5::operator[](const Ordinal auto index ) {
	ORDINAL_CHECK(index, 5);
	return ( &x )[ index ];
}

ID_INLINE idVec5 &idVec5::operator=( const idVec3 &a ) { 
	x = a.x;
	y = a.y;
	z = a.z;
	s = t = 0;
	return *this;
}

ID_INLINE size_t idVec5::GetDimension()
{
	return 5;
}

ID_INLINE const idVec3 &idVec5::ToVec3() const {
	return *reinterpret_cast<const idVec3 *>(this);
}

ID_INLINE idVec3 &idVec5::ToVec3() {
	return *reinterpret_cast<idVec3 *>(this);
}

ID_INLINE const float *idVec5::ToFloatPtr() const {
	return &x;
}

ID_INLINE float *idVec5::ToFloatPtr() {
	return &x;
}


//===============================================================
//
//	idVec6 - 6D vector
//
//===============================================================

class idVec6 {
public:	
					idVec6() noexcept;
					explicit idVec6( const float *a );
					explicit idVec6(float a1, float a2, float a3, float a4, float a5, float a6 );

	void 			Set(float a1, float a2, float a3, float a4, float a5, float a6 );
	void			Zero();

	float			operator[]( const Ordinal auto index ) const;
	float &			operator[]( const Ordinal auto index );
	idVec6			operator-() const;
	idVec6			operator*(float a ) const;
	idVec6			operator/(float a ) const;
	float			operator*( const idVec6 &a ) const;
	idVec6			operator-( const idVec6 &a ) const;
	idVec6			operator+( const idVec6 &a ) const;
	idVec6 &		operator*=( float a );
	idVec6 &        operator*=( const std::integral auto a );
	idVec6 &		operator/=( float a );
	idVec6 &        operator/=( const std::integral auto a );
	idVec6 &		operator+=( const idVec6 &a );
	idVec6 &		operator-=( const idVec6 &a );

	friend idVec6	operator*(float a, const idVec6& b );

	[[nodiscard]] bool			Compare( const idVec6 &a ) const;							// exact compare, no epsilon
	[[nodiscard]] bool			Compare( const idVec6 &a, float epsilon ) const;		// compare with epsilon
	bool			operator==(	const idVec6 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const idVec6 &a ) const;						// exact compare, no epsilon

	[[nodiscard]] float			Length() const;
	[[nodiscard]] float			LengthSqr() const;
	float			Normalize();			// returns length
	float			NormalizeFast();		// returns length

	[[nodiscard]] static size_t		GetDimension();

	
	const idVec3 &	SubVec3( const Ordinal auto index ) const;
	
	idVec3 &		SubVec3( const Ordinal auto index );
	[[nodiscard]] const float *	ToFloatPtr() const;
	float *			ToFloatPtr();
	[[nodiscard]] const char *	ToString( const size_t precision = 2 ) const;

private:
	float			p[6] = {};
};

extern idVec6 vec6_origin;
#define vec6_zero vec6_origin
extern idVec6 vec6_infinity;

ID_INLINE idVec6::idVec6() noexcept = default;

ID_INLINE idVec6::idVec6( const float *a ) {
	memcpy( p, a, 6 * sizeof( float ) );
}

ID_INLINE idVec6::idVec6( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ID_INLINE idVec6 idVec6::operator-() const {
	return idVec6( -p[0], -p[1], -p[2], -p[3], -p[4], -p[5] );
}


float idVec6::operator[](const Ordinal auto index ) const {
	ORDINAL_CHECK(index, 6);
	return p[index];
}


float &idVec6::operator[](const Ordinal auto index ) {
	ORDINAL_CHECK(index, 6);
	return p[index];
}

ID_INLINE idVec6 idVec6::operator*( const float a ) const {
	return idVec6( p[0]*a, p[1]*a, p[2]*a, p[3]*a, p[4]*a, p[5]*a );
}

ID_INLINE float idVec6::operator*( const idVec6 &a ) const {
	return p[0] * a[0] + p[1] * a[1] + p[2] * a[2] + p[3] * a[3] + p[4] * a[4] + p[5] * a[5];
}

ID_INLINE idVec6 idVec6::operator/( const float a ) const {
	assert( a != 0.0f );
	const float inva = 1.0f / a;
	return idVec6( p[0]*inva, p[1]*inva, p[2]*inva, p[3]*inva, p[4]*inva, p[5]*inva );
}

ID_INLINE idVec6 idVec6::operator+( const idVec6 &a ) const {
	return idVec6( p[0] + a[0], p[1] + a[1], p[2] + a[2], p[3] + a[3], p[4] + a[4], p[5] + a[5] );
}

ID_INLINE idVec6 idVec6::operator-( const idVec6 &a ) const {
	return idVec6( p[0] - a[0], p[1] - a[1], p[2] - a[2], p[3] - a[3], p[4] - a[4], p[5] - a[5] );
}

ID_INLINE idVec6 &idVec6::operator*=( const float a ) {
	p[0] *= a;
	p[1] *= a;
	p[2] *= a;
	p[3] *= a;
	p[4] *= a;
	p[5] *= a;
	return *this;
}

ID_INLINE idVec6& idVec6::operator*=( const std::integral auto a ) {
	return operator*=(numeric_cast<float>(a));
}

ID_INLINE idVec6 &idVec6::operator/=( const float a ) {
	assert( a != 0.0f );
	const float inva = 1.0f / a;
	p[0] *= inva;
	p[1] *= inva;
	p[2] *= inva;
	p[3] *= inva;
	p[4] *= inva;
	p[5] *= inva;
	return *this;
}

ID_INLINE idVec6& idVec6::operator/=( const std::integral auto a ) {
	return operator/=(numeric_cast<float>(a));
}

ID_INLINE idVec6 &idVec6::operator+=( const idVec6 &a ) {
	p[0] += a[0];
	p[1] += a[1];
	p[2] += a[2];
	p[3] += a[3];
	p[4] += a[4];
	p[5] += a[5];
	return *this;
}

ID_INLINE idVec6 &idVec6::operator-=( const idVec6 &a ) {
	p[0] -= a[0];
	p[1] -= a[1];
	p[2] -= a[2];
	p[3] -= a[3];
	p[4] -= a[4];
	p[5] -= a[5];
	return *this;
}

ID_INLINE idVec6 operator*( const float a, const idVec6& b ) {
	return b * a;
}

ID_INLINE bool idVec6::Compare( const idVec6 &a ) const {
	return ( ( std::equal_to<>()(p[0], a[0]) ) && (std::equal_to<>()(p[1], a[1]) ) && (std::equal_to<>()(p[2], a[2]) ) &&
			(std::equal_to<>()(p[3], a[3]) ) && (std::equal_to<>()(p[4], a[4]) ) && (std::equal_to<>()(p[5], a[5]) ) );
}

ID_INLINE bool idVec6::Compare( const idVec6 &a, const float epsilon ) const {
	if ( idMath::Fabs( p[0] - a[0] ) > epsilon ) {
		return false;
	}
			
	if ( idMath::Fabs( p[1] - a[1] ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( p[2] - a[2] ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( p[3] - a[3] ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( p[4] - a[4] ) > epsilon ) {
		return false;
	}

	if ( idMath::Fabs( p[5] - a[5] ) > epsilon ) {
		return false;
	}

	return true;
}

ID_INLINE bool idVec6::operator==( const idVec6 &a ) const {
	return Compare( a );
}

ID_INLINE bool idVec6::operator!=( const idVec6 &a ) const {
	return !Compare( a );
}

ID_INLINE void idVec6::Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ID_INLINE void idVec6::Zero() {
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0.0f;
}

ID_INLINE float idVec6::Length() const {
	return idMath::Sqrt( p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5] );
}

ID_INLINE float idVec6::LengthSqr() const {
	return ( p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5] );
}

ID_INLINE float idVec6::Normalize() {
	const float sqrLength = p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5];
	const float invLength = idMath::InvSqrt(sqrLength);
	p[0] *= invLength;
	p[1] *= invLength;
	p[2] *= invLength;
	p[3] *= invLength;
	p[4] *= invLength;
	p[5] *= invLength;
	return invLength * sqrLength;
}

ID_INLINE float idVec6::NormalizeFast() {
	const float sqrLength = p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5];
	const float invLength = idMath::InvSqrt(sqrLength);
	p[0] *= invLength;
	p[1] *= invLength;
	p[2] *= invLength;
	p[3] *= invLength;
	p[4] *= invLength;
	p[5] *= invLength;
	return invLength * sqrLength;
}

ID_INLINE size_t idVec6::GetDimension()
{
	return 6;
}


const idVec3 &idVec6::SubVec3( const Ordinal auto index ) const {
	ORDINAL_CHECK(index, 3);
	return *reinterpret_cast<const idVec3 *>(p + index * 3);
}


idVec3 &idVec6::SubVec3( const Ordinal auto index ) {
	ORDINAL_CHECK(index, 3);
	return *reinterpret_cast<idVec3 *>(p + index * 3);
}

ID_INLINE const float *idVec6::ToFloatPtr() const {
	return p;
}

ID_INLINE float *idVec6::ToFloatPtr() {
	return p;
}

//===============================================================
//
//	idPolar3
//
//===============================================================

class idPolar3 {
public:	
	float			radius, theta, phi;

					idPolar3() noexcept = default;
					explicit idPolar3(float radius, float theta, float phi );

	void 			Set(float radius, float theta, float phi );

	float			operator[]( const Ordinal auto index ) const;
	float &			operator[]( const Ordinal auto index );
	idPolar3		operator-() const;
	idPolar3 &		operator=( const idPolar3 &a );

	[[nodiscard]] idVec3			ToVec3() const;
};

ID_INLINE idPolar3::idPolar3( const float radius, const float theta, const float phi ) {
	assert( radius > 0 );
	this->radius = radius;
	this->theta = theta;
	this->phi = phi;
}
	
ID_INLINE void idPolar3::Set( const float radius, const float theta, const float phi ) {
	assert( radius > 0 );
	this->radius = radius;
	this->theta = theta;
	this->phi = phi;
}


float idPolar3::operator[](const Ordinal auto index ) const {
	ORDINAL_CHECK(index, 3);
	return ( &radius )[ index ];
}


float &idPolar3::operator[](const Ordinal auto index ) {
	ORDINAL_CHECK(index, 3);
	return ( &radius )[ index ];
}

ID_INLINE idPolar3 idPolar3::operator-() const {
	return idPolar3( radius, -theta, -phi );
}

ID_INLINE idPolar3 &idPolar3::operator=( const idPolar3 &a ) = default;

ID_INLINE idVec3 idPolar3::ToVec3() const {
	float sp, cp, st, ct;
	idMath::SinCos( phi, sp, cp );
	idMath::SinCos( theta, st, ct );
 	return idVec3( cp * radius * ct, cp * radius * st, radius * sp );
}


/*
===============================================================================

	Old 3D vector macros, should no longer be used.

===============================================================================
*/

#define	VectorMA( v, s, b, o )		((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))

#endif /* !__MATH_VECTOR_H__ */
