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

#include "../precompiled.h"

//===============================================================
//
//  idMatX
//
//===============================================================

float	idMatX::temp[MATX_MAX_TEMP+4];
float *	idMatX::tempPtr = reinterpret_cast<float*>((reinterpret_cast<UINT_PTR>(idMatX::temp) + 15) & ~15);
size_t	idMatX::tempIndex = 0;


/*
============
idMatX::ChangeSize
============
*/
void idMatX::ChangeSize(const size_t rows, const size_t columns, const bool makeZero) {
	const size_t alloc = ( rows * columns + 3 ) & ~3;
	if (std::cmp_greater(alloc, alloced) && alloced != -1 ) {
		float *oldMat = mat;
		mat = static_cast<float*>(Mem_Alloc16(alloc * sizeof(float), TAG_MATH));
		if ( makeZero ) {
			memset( mat, 0, alloc * sizeof( float ) );
		}
		alloced = alloc;
		if ( oldMat ) {
			const size_t minRow = Min( numRows, rows );
			const size_t minColumn = Min( numColumns, columns );
			for (size_t i = 0; i < minRow; i++ ) {
				for (size_t j = 0; j < minColumn; j++ ) {
					mat[ i * columns + j ] = oldMat[ i * numColumns + j ];
				}
			}
			Mem_Free16( oldMat );
		}
	} else {
		if ( columns < numColumns ) {
			const size_t minRow = Min( numRows, rows );
			for (size_t i = 0; i < minRow; i++ ) {
				for (size_t j = 0; std::cmp_less(j, columns); j++ ) {
					mat[ i * columns + j ] = mat[ i * numColumns + j ];
				}
			}
		} else if ( columns > numColumns ) {
			for (size_t i = Min( numRows, rows ) - 1; i >= 0; i-- ) {
				if ( makeZero ) {
					for (size_t j = columns - 1; std::cmp_greater_equal(j, numColumns); j-- ) {
						mat[ i * columns + j ] = 0.0f;
					}
				}
				for (size_t j = numColumns - 1; j >= 0; j-- ) {
					mat[ i * columns + j ] = mat[ i * numColumns + j ];
				}
			}
		}
		if ( makeZero && rows > numRows ) {
			memset( mat + numRows * columns, 0, ( rows - numRows ) * columns * sizeof( float ) );
		}
	}
	numRows = rows;
	numColumns = columns;
	MATX_CLEAREND();
}

/*
============
idMatX::RemoveRow
============
*/

idMatX &idMatX::RemoveRow(const Ordinal auto r) {
	ORDINAL_CHECK(r, numRows);
	assert( r < numRows );

	numRows--;

	for (size_t i = r; i < numRows; i++ ) {
		memcpy( &mat[i * numColumns], &mat[( i + 1 ) * numColumns], numColumns * sizeof( float ) );
	}

	return *this;
}

/*
============
idMatX::RemoveColumn
============
*/

idMatX &idMatX::RemoveColumn(const Ordinal auto r) {
	ORDINAL_CHECK(r, numColumns);
	size_t i = 0;

	assert( r < numColumns );

	numColumns--;

	for ( i = 0; i < numRows - 1; i++ ) {
		memmove( &mat[i * numColumns + r], &mat[i * ( numColumns + 1 ) + r + 1], numColumns * sizeof( float ) );
	}
	memmove( &mat[i * numColumns + r], &mat[i * ( numColumns + 1 ) + r + 1], ( numColumns - r ) * sizeof( float ) );

	return *this;
}

/*
============
idMatX::RemoveRowColumn
============
*/

idMatX &idMatX::RemoveRowColumn(const Ordinal auto r) {
	ORDINAL_CHECK(r, numRows);
	ORDINAL_CHECK(r, numColumns);
	size_t i = 0;

	assert( r < numRows && r < numColumns );

	numRows--;
	numColumns--;

	if ( r > 0 ) {
		for ( i = 0; i < r - 1; i++ ) {
			memmove( &mat[i * numColumns + r], &mat[i * ( numColumns + 1 ) + r + 1], numColumns * sizeof( float ) );
		}
		memmove( &mat[i * numColumns + r], &mat[i * ( numColumns + 1 ) + r + 1], ( numColumns - r ) * sizeof( float ) );
	}

	memcpy( &mat[r * numColumns], &mat[( r + 1 ) * ( numColumns + 1 )], r * sizeof( float ) );

	for ( i = r; i < numRows - 1; i++ ) {
		memcpy( &mat[i * numColumns + r], &mat[( i + 1 ) * ( numColumns + 1 ) + r + 1], numColumns * sizeof( float ) );
	}
	memcpy( &mat[i * numColumns + r], &mat[( i + 1 ) * ( numColumns + 1 ) + r + 1], ( numColumns - r ) * sizeof( float ) );

	return *this;
}

/*
========================
idMatX::CopyLowerToUpperTriangle
========================
*/
void idMatX::CopyLowerToUpperTriangle() {
	assert( ( GetNumColumns() & 3 ) == 0 );
	assert( GetNumColumns() >= GetNumRows() );

#ifdef ID_WIN_X86_SSE_INTRIN

	const int n = GetNumColumns();
	const int m = GetNumRows();

	const int n0 = 0;
	const int n1 = n;
	const int n2 = ( n << 1 );
	const int n3 = ( n << 1 ) + n;
	const int n4 = ( n << 2 );

	const int b1 = ( ( m - 0 ) >> 1 ) & 1;	// ( m & 3 ) > 1
	const int b2 = ( ( m - 1 ) >> 1 ) & 1;	// ( m & 3 ) > 2 (provided ( m & 3 ) > 0)

	const int n1_masked = ( n & -b1 );
	const int n2_masked = ( n & -b1 ) + ( n & -b2 );

	const __m128 mask0 = __m128c( _mm_set_epi32(  0,  0,  0, -1 ) );
	const __m128 mask1 = __m128c( _mm_set_epi32(  0,  0, -1, -1 ) );
	const __m128 mask2 = __m128c( _mm_set_epi32(  0, -1, -1, -1 ) );
	const __m128 mask3 = __m128c( _mm_set_epi32( -1, -1, -1, -1 ) );

	const __m128 bottomMask[2] = { __m128c( _mm_set1_epi32( 0 ) ), __m128c( _mm_set1_epi32( -1 ) ) };

	float * __restrict basePtr = ToFloatPtr();

	for (size_t i = 0; i < m - 3; i += 4 ) {

		// copy top left diagonal 4x4 block elements
		__m128 r0 = _mm_and_ps( _mm_load_ps( basePtr + n0 ), mask0 );
		__m128 r1 = _mm_and_ps( _mm_load_ps( basePtr + n1 ), mask1 );
		__m128 r2 = _mm_and_ps( _mm_load_ps( basePtr + n2 ), mask2 );
		__m128 r3 = _mm_and_ps( _mm_load_ps( basePtr + n3 ), mask3 );

		__m128 t0 = _mm_unpacklo_ps( r0, r2 );	// x0, z0, x1, z1
		__m128 t1 = _mm_unpackhi_ps( r0, r2 );	// x2, z2, x3, z3
		__m128 t2 = _mm_unpacklo_ps( r1, r3 );	// y0, w0, y1, w1
		__m128 t3 = _mm_unpackhi_ps( r1, r3 );	// y2, w2, y3, w3

		__m128 s0 = _mm_unpacklo_ps( t0, t2 );	// x0, y0, z0, w0
		__m128 s1 = _mm_unpackhi_ps( t0, t2 );	// x1, y1, z1, w1
		__m128 s2 = _mm_unpacklo_ps( t1, t3 );	// x2, y2, z2, w2
		__m128 s3 = _mm_unpackhi_ps( t1, t3 );	// x3, y3, z3, w3

		r0 = _mm_or_ps( r0, s0 );
		r1 = _mm_or_ps( r1, s1 );
		r2 = _mm_or_ps( r2, s2 );
		r3 = _mm_or_ps( r3, s3 );

		_mm_store_ps( basePtr + n0, r0 );
		_mm_store_ps( basePtr + n1, r1 );
		_mm_store_ps( basePtr + n2, r2 );
		_mm_store_ps( basePtr + n3, r3 );

		// copy one column of 4x4 blocks to one row of 4x4 blocks
		const float * __restrict srcPtr = basePtr;
		float * __restrict dstPtr = basePtr;

		for (size_t j = i + 4; j < m - 3; j += 4 ) {
			srcPtr += n4;
			dstPtr += 4;

			__m128 r0 = _mm_load_ps( srcPtr + n0 );
			__m128 r1 = _mm_load_ps( srcPtr + n1 );
			__m128 r2 = _mm_load_ps( srcPtr + n2 );
			__m128 r3 = _mm_load_ps( srcPtr + n3 );

			__m128 t0 = _mm_unpacklo_ps( r0, r2 );	// x0, z0, x1, z1
			__m128 t1 = _mm_unpackhi_ps( r0, r2 );	// x2, z2, x3, z3
			__m128 t2 = _mm_unpacklo_ps( r1, r3 );	// y0, w0, y1, w1
			__m128 t3 = _mm_unpackhi_ps( r1, r3 );	// y2, w2, y3, w3

			r0 = _mm_unpacklo_ps( t0, t2 );			// x0, y0, z0, w0
			r1 = _mm_unpackhi_ps( t0, t2 );			// x1, y1, z1, w1
			r2 = _mm_unpacklo_ps( t1, t3 );			// x2, y2, z2, w2
			r3 = _mm_unpackhi_ps( t1, t3 );			// x3, y3, z3, w3

			_mm_store_ps( dstPtr + n0, r0 );
			_mm_store_ps( dstPtr + n1, r1 );
			_mm_store_ps( dstPtr + n2, r2 );
			_mm_store_ps( dstPtr + n3, r3 );
		}

		// copy the last partial 4x4 block elements
		if ( m & 3 ) {
			srcPtr += n4;
			dstPtr += 4;

			__m128 r0 = _mm_load_ps( srcPtr + n0 );
			__m128 r1 = _mm_and_ps( _mm_load_ps( srcPtr + n1_masked ), bottomMask[b1] );
			__m128 r2 = _mm_and_ps( _mm_load_ps( srcPtr + n2_masked ), bottomMask[b2] );
			__m128 r3 = _mm_setzero_ps();

			__m128 t0 = _mm_unpacklo_ps( r0, r2 );	// x0, z0, x1, z1
			__m128 t1 = _mm_unpackhi_ps( r0, r2 );	// x2, z2, x3, z3
			__m128 t2 = _mm_unpacklo_ps( r1, r3 );	// y0, w0, y1, w1
			__m128 t3 = _mm_unpackhi_ps( r1, r3 );	// y2, w2, y3, w3

			r0 = _mm_unpacklo_ps( t0, t2 );			// x0, y0, z0, w0
			r1 = _mm_unpackhi_ps( t0, t2 );			// x1, y1, z1, w1
			r2 = _mm_unpacklo_ps( t1, t3 );			// x2, y2, z2, w2
			r3 = _mm_unpackhi_ps( t1, t3 );			// x3, y3, z3, w3

			_mm_store_ps( dstPtr + n0, r0 );
			_mm_store_ps( dstPtr + n1, r1 );
			_mm_store_ps( dstPtr + n2, r2 );
			_mm_store_ps( dstPtr + n3, r3 );
		}

		basePtr += n4 + 4;
	}

	// copy the lower right partial diagonal 4x4 block elements
	if ( m & 3 ) {
		__m128 r0 = _mm_and_ps( _mm_load_ps( basePtr + n0 ), mask0 );
		__m128 r1 = _mm_and_ps( _mm_load_ps( basePtr + n1_masked ), _mm_and_ps( mask1, bottomMask[b1] ) );
		__m128 r2 = _mm_and_ps( _mm_load_ps( basePtr + n2_masked ), _mm_and_ps( mask2, bottomMask[b2] ) );
		__m128 r3 = _mm_setzero_ps();

		__m128 t0 = _mm_unpacklo_ps( r0, r2 );	// x0, z0, x1, z1
		__m128 t1 = _mm_unpackhi_ps( r0, r2 );	// x2, z2, x3, z3
		__m128 t2 = _mm_unpacklo_ps( r1, r3 );	// y0, w0, y1, w1
		__m128 t3 = _mm_unpackhi_ps( r1, r3 );	// y2, w2, y3, w3

		__m128 s0 = _mm_unpacklo_ps( t0, t2 );	// x0, y0, z0, w0
		__m128 s1 = _mm_unpackhi_ps( t0, t2 );	// x1, y1, z1, w1
		__m128 s2 = _mm_unpacklo_ps( t1, t3 );	// x2, y2, z2, w2

		r0 = _mm_or_ps( r0, s0 );
		r1 = _mm_or_ps( r1, s1 );
		r2 = _mm_or_ps( r2, s2 );

		_mm_store_ps( basePtr + n2_masked, r2 );
		_mm_store_ps( basePtr + n1_masked, r1 );
		_mm_store_ps( basePtr + n0, r0 );
	}

#else

	const size_t n = GetNumColumns();
	const size_t m = GetNumRows();
	for (size_t i = 0; i < m; i++ ) {
		const float * __restrict ptr = ToFloatPtr() + ( i + 1 ) * n + i;
		float * __restrict dstPtr = ToFloatPtr() + i * n;
		for (size_t j = i + 1; j < m; j++ ) {
			dstPtr[j] = ptr[0];
			ptr += n;
		}
	}

#endif

#ifdef _DEBUG
	for ( size_t i = 0; std::cmp_less(i, numRows); i++ ) {
		for ( size_t j = 0; std::cmp_less(j, numRows); j++ ) {
			assert( std::equal_to<>()(mat[ i * numColumns + j ], mat[ j * numColumns + i ] ));
		}
	}
#endif
}
/*
============
idMatX::IsOrthogonal

  returns true if (*this) * this->Transpose() == Identity
============
*/
bool idMatX::IsOrthogonal( const float epsilon ) const {
	if ( !IsSquare() ) {
		return false;
	}

	const float* ptr1 = mat;
	for ( size_t i = 0; std::cmp_less(i, numRows); i++ ) {
		for (size_t j = 0; std::cmp_less(j, numColumns); j++ ) {
			const float* ptr2 = mat + j;
			float sum = ptr1[0] * ptr2[0] - static_cast<float>(i == j);
			for (size_t n = 1; std::cmp_less(n, numColumns); n++ ) {
				ptr2 += numColumns;
				sum += ptr1[n] * ptr2[0];
			}
			if ( idMath::Fabs( sum ) > epsilon ) {
				return false;
			}
		}
		ptr1 += numColumns;
	}
	return true;
}

/*
============
idMatX::IsOrthonormal

  returns true if (*this) * this->Transpose() == Identity and the length of each column vector is 1
============
*/
bool idMatX::IsOrthonormal( const float epsilon ) const {
	float *ptr2, sum;

	if ( !IsSquare() ) {
		return false;
	}

	const float* ptr1 = mat;
	for (size_t i = 0; std::cmp_less(i, numRows); i++ ) {
		for (size_t j = 0; std::cmp_less(j, numColumns); j++ ) {
			ptr2 = mat + j;
			sum = ptr1[0] * ptr2[0] - static_cast<float>(i == j);
			for (size_t n = 1; std::cmp_less(n, numColumns); n++ ) {
				ptr2 += numColumns;
				sum += ptr1[n] * ptr2[0];
			}
			if ( idMath::Fabs( sum ) > epsilon ) {
				return false;
			}
		}
		ptr1 += numColumns;

		ptr2 = mat + i;
		sum = ptr2[0] * ptr2[0] - 1.0f;
		for (size_t j = 1; std::cmp_less(j, numRows); j++ ) {
			ptr2 += numColumns;
			sum += ptr2[j] * ptr2[j];
		}
		if ( idMath::Fabs( sum ) > epsilon ) {
			return false;
		}
	}
	return true;
}

/*
============
idMatX::IsPMatrix

  returns true if the matrix is a P-matrix
  A square matrix is a P-matrix if all its principal minors are positive.
============
*/
bool idMatX::IsPMatrix( const float epsilon ) const {
	size_t i = 0, j = 0;
	idMatX m;

	if ( !IsSquare() ) {
		return false;
	}

	if ( numRows <= 0 ) {
		return true;
	}

	if ( (*this)[0][0] <= epsilon ) {
		return false;
	}

	if ( numRows <= 1 ) {
		return true;
	}

	m.SetData( numRows - 1, numColumns - 1, MATX_ALLOCA(( numRows - 1 ) * ( numColumns - 1 )));

	for ( i = 1; std::cmp_less(i, numRows); i++ ) {
		for ( j = 1; std::cmp_less(j, numColumns); j++ ) {
			m[i-1][j-1] = (*this)[i][j];
		}
	}

	if ( !m.IsPMatrix( epsilon ) ) {
		return false;
	}

	for ( i = 1; std::cmp_less(i, numRows); i++ ) {
		const float d = (*this)[i][0] / (*this)[0][0];
		for ( j = 1; std::cmp_less(j, numColumns); j++ ) {
			m[i-1][j-1] = (*this)[i][j] - d * (*this)[0][j];
		}
	}

	if ( !m.IsPMatrix( epsilon ) ) {
		return false;
	}

	return true;
}

/*
============
idMatX::IsZMatrix

  returns true if the matrix is a Z-matrix
  A square matrix M is a Z-matrix if M[i][j] <= 0 for all i != j.
============
*/
bool idMatX::IsZMatrix( const float epsilon ) const {
	if ( !IsSquare() ) {
		return false;
	}

	for (size_t i = 0; std::cmp_less(i, numRows); i++ ) {
		for (size_t j = 0; std::cmp_less(j, numColumns); j++ ) {
			if ( (*this)[i][j] > epsilon && i != j ) {
				return false;
			}
		}
	}
	return true;
}

/*
============
idMatX::IsPositiveDefinite

  returns true if the matrix is Positive Definite (PD)
  A square matrix M of order n is said to be PD if y'My > 0 for all vectors y of dimension n, y != 0.
============
*/
bool idMatX::IsPositiveDefinite( const float epsilon ) const {
	size_t i = 0, j = 0;
	idMatX m;

	// the matrix must be square
	if ( !IsSquare() ) {
		return false;
	}

	// copy matrix
	m.SetData( numRows, numColumns, MATX_ALLOCA(numRows * numColumns));
	m = *this;

	// add transpose
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		for ( j = 0; std::cmp_less(j, numColumns); j++ ) {
			m[i][j] += (*this)[j][i];
		}
	}

	// test Positive Definiteness with Gaussian pivot steps
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {

		for ( j = i; std::cmp_less(j, numColumns); j++ ) {
			if ( m[j][j] <= epsilon ) {
				return false;
			}
		}

		const float d = 1.0f / m[i][i];
		for ( j = i + 1; std::cmp_less(j, numColumns); j++ ) {
			const float s = d * m[j][i];
			m[j][i] = 0.0f;
			for (size_t k = i + 1; std::cmp_less(k, numRows); k++ ) {
				m[j][k] -= s * m[i][k];
			}
		}
	}

	return true;
}

/*
============
idMatX::IsSymmetricPositiveDefinite

  returns true if the matrix is Symmetric Positive Definite (PD)
============
*/
bool idMatX::IsSymmetricPositiveDefinite( const float epsilon ) const {
	idMatX m;

	// the matrix must be symmetric
	if ( !IsSymmetric( epsilon ) ) {
		return false;
	}

	// copy matrix
	m.SetData( numRows, numColumns, MATX_ALLOCA(numRows * numColumns));
	m = *this;

	// being able to obtain Cholesky factors is both a necessary and sufficient condition for positive definiteness
	return m.Cholesky_Factor();
}

/*
============
idMatX::IsPositiveSemiDefinite

  returns true if the matrix is Positive Semi Definite (PSD)
  A square matrix M of order n is said to be PSD if y'My >= 0 for all vectors y of dimension n, y != 0.
============
*/
bool idMatX::IsPositiveSemiDefinite( const float epsilon ) const {
	int i, j, k;
	idMatX m;

	// the matrix must be square
	if ( !IsSquare() ) {
		return false;
	}

	// copy original matrix
	m.SetData( numRows, numColumns, MATX_ALLOCA(numRows * numColumns));
	m = *this;

	// add transpose
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		for ( j = 0; std::cmp_less(j, numColumns); j++ ) {
			m[i][j] += (*this)[j][i];
		}
	}

	// test Positive Semi Definiteness with Gaussian pivot steps
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {

		for ( j = i; std::cmp_less(j, numColumns); j++ ) {
			if ( m[j][j] < -epsilon ) {
				return false;
			}
			if ( m[j][j] > epsilon ) {
				continue;
			}
			for ( k = 0; std::cmp_less(k, numRows); k++ ) {
				if ( idMath::Fabs( m[k][j] ) > epsilon ) {
					return false;
				}
				if ( idMath::Fabs( m[j][k] ) > epsilon ) {
					return false;
				}
			}
		}

		if ( m[i][i] <= epsilon ) {
			continue;
		}

		const float d = 1.0f / m[i][i];
		for ( j = i + 1; std::cmp_less(j, numColumns); j++ ) {
			const float s = d * m[j][i];
			m[j][i] = 0.0f;
			for ( k = i + 1; std::cmp_less(k, numRows); k++ ) {
				m[j][k] -= s * m[i][k];
			}
		}
	}

	return true;
}

/*
============
idMatX::IsSymmetricPositiveSemiDefinite

  returns true if the matrix is Symmetric Positive Semi Definite (PSD)
============
*/
bool idMatX::IsSymmetricPositiveSemiDefinite( const float epsilon ) const {

	// the matrix must be symmetric
	if ( !IsSymmetric( epsilon ) ) {
		return false;
	}

	return IsPositiveSemiDefinite( epsilon );
}

/*
============
idMatX::LowerTriangularInverse

  in-place inversion of the lower triangular matrix
============
*/
bool idMatX::LowerTriangularInverse() {
	for ( size_t i = 0; std::cmp_less(i, numRows); i++ ) {
		double d = (*this)[i][i];
		if ( d == 0.0 ) {
			return false;
		}

		d = 1.0f / d;

		(*this)[i][i] = idMath::Dtof(d);

		for (size_t j = 0; j < i; j++ ) {
			double sum = 0.0f;
			for (size_t k = j; k < i; k++ ) {
				sum -= (*this)[i][k] * (*this)[k][j];
			}
			(*this)[i][j] = idMath::Dtof(sum * d);
		}
	}
	return true;
}

/*
============
idMatX::UpperTriangularInverse

  in-place inversion of the upper triangular matrix
============
*/
bool idMatX::UpperTriangularInverse() {
	for (int64 i = idMath::integer_cast<int64>(numRows) - 1; i >= 0; i-- ) {
		double d = (*this)[i][i];
		if ( d == 0.0 ) {
			return false;
		}

		d = 1.0f / d;

		(*this)[i][i] = idMath::Dtof(d);

		for (size_t j = numRows - 1; std::cmp_greater(j, i); j-- ) {
			double sum = 0.0f;
			for (size_t k = j; std::cmp_greater(k, i); k-- ) {
				sum -= (*this)[i][k] * (*this)[k][j];
			}
			(*this)[i][j] = idMath::Dtof(sum * d);
		}
	}
	return true;
}

/*
=============
idMatX::ToString
=============
*/
const char *idMatX::ToString(const int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
============
idMatX::Update_RankOne

  Updates the matrix to obtain the matrix: A + alpha * v * w'
============
*/
void idMatX::Update_RankOne( const idVecX &v, const idVecX &w, const float alpha ) {
	assert( v.GetSize() >= numRows );
	assert( w.GetSize() >= numColumns );

	for (size_t i = 0; std::cmp_less(i, numRows); i++ ) {
		const float s = alpha * v[i];
		for (size_t j = 0; std::cmp_less(j, numColumns); j++ ) {
			(*this)[i][j] += s * w[j];
		}
	}
}

/*
============
idMatX::Update_RankOneSymmetric

  Updates the matrix to obtain the matrix: A + alpha * v * v'
============
*/
void idMatX::Update_RankOneSymmetric( const idVecX &v, const float alpha ) {
	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );

	for (size_t i = 0; std::cmp_less(i, numRows); i++ ) {
		const float s = alpha * v[i];
		for (size_t j = 0; std::cmp_less(j, numColumns); j++ ) {
			(*this)[i][j] += s * v[j];
		}
	}
}

/*
============
idMatX::Update_RowColumn

  Updates the matrix to obtain the matrix:

      [ 0  a  0 ]
  A + [ d  b  e ]
      [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1], d = w[0,r-1], w[r] = 0.0f, e = w[r+1,numColumns-1]
============
*/

void idMatX::Update_RowColumn(const idVecX &v, const idVecX &w, const Ordinal auto r) {
	ORDINAL_CHECK(r, numRows);
	ORDINAL_CHECK(r, numColumns);
	size_t i = 0;

	assert( w[r] == 0.0f );
	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );

	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		(*this)[i][r] += v[i];
	}
	for ( i = 0; std::cmp_less(i, numColumns); i++ ) {
		(*this)[r][i] += w[i];
	}
}

/*
============
idMatX::Update_RowColumnSymmetric

  Updates the matrix to obtain the matrix:

      [ 0  a  0 ]
  A + [ a  b  c ]
      [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1]
============
*/

void idMatX::Update_RowColumnSymmetric(const idVecX &v, const Ordinal auto r) {
	ORDINAL_CHECK(r, numRows);
	size_t i = 0;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );

	for ( i = 0; i < r; i++ ) {
		(*this)[i][r] += v[i];
		(*this)[r][i] += v[i];
	}
	(*this)[r][r] += v[r];
	for ( i = r+1; std::cmp_less(i, numRows); i++ ) {
		(*this)[i][r] += v[i];
		(*this)[r][i] += v[i];
	}
}

/*
============
idMatX::Update_Increment

  Updates the matrix to obtain the matrix:

  [ A  a ]
  [ c  b ]

  where: a = v[0,numRows-1], b = v[numRows], c = w[0,numColumns-1]], w[numColumns] = 0
============
*/
void idMatX::Update_Increment( const idVecX &v, const idVecX &w ) {
	int i;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );
	assert( w.GetSize() >= numColumns+1 );

	ChangeSize( numRows+1, numColumns+1, false );

	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		(*this)[i][numColumns-1] = v[i];
	}
	for ( i = 0; i < numColumns-1; i++ ) {
		(*this)[numRows-1][i] = w[i];
	}
}

/*
============
idMatX::Update_IncrementSymmetric

  Updates the matrix to obtain the matrix:

  [ A  a ]
  [ a  b ]

  where: a = v[0,numRows-1], b = v[numRows]
============
*/
void idMatX::Update_IncrementSymmetric( const idVecX &v ) {
	int i;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );

	ChangeSize( numRows+1, numColumns+1, false );

	for ( i = 0; i < numRows-1; i++ ) {
		(*this)[i][numColumns-1] = v[i];
	}
	for ( i = 0; std::cmp_less(i, numColumns); i++ ) {
		(*this)[numRows-1][i] = v[i];
	}
}

/*
============
idMatX::Update_Decrement

  Updates the matrix to obtain a matrix with row r and column r removed.
============
*/

void idMatX::Update_Decrement(const Ordinal auto r) {
	ORDINAL_CHECK(r, numRows);
	ORDINAL_CHECK(r, numColumns);
	RemoveRowColumn( r );
}

/*
============
idMatX::Inverse_GaussJordan

  in-place inversion using Gauss-Jordan elimination
============
*/
bool idMatX::Inverse_GaussJordan() {
	int64 j = 0;
	size_t k = 0, c = 0;
	float d = 0.0f;

	assert( numRows == numColumns );

	size_t*columnIndex = static_cast<size_t*>(_alloca16(numRows * sizeof(size_t)));
	size_t*rowIndex = static_cast<size_t*>(_alloca16(numRows * sizeof(size_t)));
	bool *pivot = static_cast<bool*>(_alloca16(numRows * sizeof( bool )));

	memset( pivot, 0, numRows * sizeof( bool ) );

	// elimination with full pivoting
	for (size_t i = 0; std::cmp_less(i, numRows); i++ ) {

		// search the whole matrix except for pivoted rows for the maximum absolute value
		float max = 0.0f;
		size_t r = c = 0;
		for ( j = 0; std::cmp_less(j, numRows); j++ ) {
			if ( !pivot[j] ) {
				for ( k = 0; std::cmp_less(k, numRows); k++ ) {
					if ( !pivot[k] ) {
						d = idMath::Fabs( (*this)[j][k] );
						if ( d > max ) {
							max = d;
							r = j;
							c = k;
						}
					}
				}
			}
		}

		if ( max == 0.0f ) {
			// matrix is not invertible
			return false;
		}

		pivot[c] = true;

		// swap rows such that entry (c,c) has the pivot entry
		if ( r != c ) {
			SwapRows( r, c );
		}

		// keep track of the row permutation
		rowIndex[i] = r;
		columnIndex[i] = c;

		// scale the row to make the pivot entry equal to 1
		d = 1.0f / (*this)[c][c];
		(*this)[c][c] = 1.0f;
		for ( k = 0; std::cmp_less(k, numRows); k++ ) {
			(*this)[c][k] *= d;
		}

		// zero out the pivot column entries in the other rows
		for ( j = 0; std::cmp_less(j, numRows); j++ ) {
			if (std::cmp_not_equal(j, c)) {
				d = (*this)[j][c];
				(*this)[j][c] = 0.0f;
				for ( k = 0; std::cmp_less(k, numRows); k++ ) {
					(*this)[j][k] -= (*this)[c][k] * d;
				}
			}
		}
	}

	// reorder rows to store the inverse of the original matrix
	for ( j = idMath::integer_cast<int64>(numRows) - 1; j >= 0; j-- ) {
		if ( rowIndex[j] != columnIndex[j] ) {
			for ( k = 0; std::cmp_less(k, numRows); k++ ) {
				d = (*this)[k][rowIndex[j]];
				(*this)[k][rowIndex[j]] = (*this)[k][columnIndex[j]];
				(*this)[k][columnIndex[j]] = d;
			}
		}
	}

	return true;
}

/*
============
idMatX::Inverse_UpdateRankOne

  Updates the in-place inverse using the Sherman-Morrison formula to obtain the inverse for the matrix: A + alpha * v * w'
============
*/
bool idMatX::Inverse_UpdateRankOne( const idVecX &v, const idVecX &w, float alpha ) {
	idVecX y, z;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );

	y.SetData( numRows, VECX_ALLOCA(numRows));
	z.SetData( numRows, VECX_ALLOCA(numRows));

	Multiply( y, v );
	TransposeMultiply( z, w );
	const float beta = 1.0f + (w * y);

	if ( beta == 0.0f ) {
		return false;
	}

	alpha /= beta;

	for (size_t i = 0; std::cmp_less(i, numRows); i++ ) {
		const float s = y[i] * alpha;
		for (size_t j = 0; std::cmp_less(j, numColumns); j++ ) {
			(*this)[i][j] -= s * z[j];
		}
	}
	return true;
}

/*
============
idMatX::Inverse_UpdateRowColumn

  Updates the in-place inverse to obtain the inverse for the matrix:

      [ 0  a  0 ]
  A + [ d  b  e ]
      [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1], d = w[0,r-1], w[r] = 0.0f, e = w[r+1,numColumns-1]
============
*/

bool idMatX::Inverse_UpdateRowColumn(const idVecX &v, const idVecX &w, const Ordinal auto r) {
	ORDINAL_CHECK(r, numRows);
	ORDINAL_CHECK(r, numColumns);
	idVecX s;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );
	assert( r >= 0 && std::cmp_less(r, numRows ) && std::cmp_less(r, numColumns ) );
	assert( w[r] == 0.0f );

	s.SetData( Max( numRows, numColumns ), VECX_ALLOCA(Max( numRows, numColumns )));
	s.Zero();
	s[r] = 1.0f;

	if ( !Inverse_UpdateRankOne( v, s, 1.0f ) ) {
		return false;
	}
	if ( !Inverse_UpdateRankOne( s, w, 1.0f ) ) {
		return false;
	}
	return true;
}

/*
============
idMatX::Inverse_UpdateIncrement

  Updates the in-place inverse to obtain the inverse for the matrix:

  [ A  a ]
  [ c  b ]

  where: a = v[0,numRows-1], b = v[numRows], c = w[0,numColumns-1], w[numColumns] = 0
============
*/
bool idMatX::Inverse_UpdateIncrement( const idVecX &v, const idVecX &w ) {
	idVecX v2;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );
	assert( w.GetSize() >= numColumns+1 );

	ChangeSize( numRows+1, numColumns+1, true );
	(*this)[numRows-1][numRows-1] = 1.0f;

	v2.SetData( numRows, VECX_ALLOCA(numRows));
	v2 = v;
	v2[numRows-1] -= 1.0f;

	return Inverse_UpdateRowColumn( v2, w, numRows-1 );
}

/*
============
idMatX::Inverse_UpdateDecrement

  Updates the in-place inverse to obtain the inverse of the matrix with row r and column r removed.
  v and w should store the column and row of the original matrix respectively.
============
*/

bool idMatX::Inverse_UpdateDecrement( const idVecX &v, const idVecX &w, const Ordinal auto r ) {
	idVecX v1, w1;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( w.GetSize() >= numColumns );
	assert( r >= 0 && std::cmp_less(r, numRows) && std::cmp_less(r, numColumns ));

	v1.SetData( numRows, VECX_ALLOCA(numRows));
	w1.SetData( numRows, VECX_ALLOCA(numRows));

	// update the row and column to identity
	v1 = -v;
	w1 = -w;
	v1[r] += 1.0f;
	w1[r] = 0.0f;

	if ( !Inverse_UpdateRowColumn( v1, w1, r ) ) {
		return false;
	}

	// physically remove the row and column
	Update_Decrement( r );

	return true;
}

/*
============
idMatX::Inverse_Solve

  Solve Ax = b with A inverted
============
*/
void idMatX::Inverse_Solve( idVecX &x, const idVecX &b ) const {
	Multiply( x, b );
}

/*
============
idMatX::LU_Factor

  in-place factorization: LU
  L is a triangular matrix stored in the lower triangle.
  L has ones on the diagonal that are not stored.
  U is a triangular matrix stored in the upper triangle.
  If index != NULL partial pivoting is used for numerical stability.
  If index != NULL it must point to an array of numRow integers and is used to keep track of the row permutation.
  If det != NULL the determinant of the matrix is calculated and stored.
============
*/
bool idMatX::LU_Factor(size_t* index, float *det) {
	size_t i = 0, j = 0, k = 0;
	double t = 0.0, d = 0.0;

	// if partial pivoting should be used
	if ( index != nullptr ) {
		for ( i = 0; std::cmp_less(i, numRows); i++ ) {
			index[i] = i;
		}
	}

	double w = 1.0f;
	const size_t min = Min(numRows, numColumns);
	for ( i = 0; i < min; i++ ) {

		size_t newi = i;
		double s = idMath::Fabs((*this)[i][i]);

		if (index != nullptr) {
			// find the largest absolute pivot
			for ( j = i + 1; std::cmp_less(j, numRows); j++ ) {
				t = idMath::Fabs( (*this)[j][i] );
				if ( t > s ) {
					newi = j;
					s = t;
				}
			}
		}

		if ( s == 0.0f ) {
			return false;
		}

		if ( newi != i && (index != nullptr)) {

			w = -w;

			// swap index elements
			k = index[i];
			index[i] = index[newi];
			index[newi] = k;

			// swap rows
			for ( j = 0; std::cmp_less(j, numColumns); j++ ) {
				t = (*this)[newi][j];
				(*this)[newi][j] = (*this)[i][j];
				(*this)[i][j] = idMath::Dtof(t);
			}
		}

		if (std::cmp_less(i, numRows)) {
			d = 1.0f / (*this)[i][i];
			for ( j = i + 1; std::cmp_less(j, numRows); j++ ) {
				(*this)[j][i] *= idMath::Dtof(d);
			}
		}

		if ( i < min-1 ) {
			for ( j = i + 1; std::cmp_less(j, numRows); j++ ) {
				d = (*this)[j][i];
				for ( k = i + 1; std::cmp_less(k, numColumns); k++ ) {
					(*this)[j][k] -= idMath::Dtof(d) * (*this)[i][k];
				}
			}
		}
	}

	if ( det ) {
		for ( i = 0; std::cmp_less(i, numRows); i++ ) {
			w *= (*this)[i][i];
		}
		*det = idMath::Dtof(w);
	}

	return true;
}   

/*
============
idMatX::LU_UpdateRankOne

  Updates the in-place LU factorization to obtain the factors for the matrix: LU + alpha * v * w'
============
*/
bool idMatX::LU_UpdateRankOne(const idVecX &v, const idVecX &w, const float alpha, size_t* index) {
	size_t i = 0, j = 0;
	double d = 0.0;

	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );

	float* y = static_cast<float*>(_alloca16(v.GetSize() * sizeof( float )));
	float* z = static_cast<float*>(_alloca16(w.GetSize() * sizeof( float )));

	if (std::not_equal_to<>()(index, nullptr)) {
		for ( i = 0; std::cmp_less(i, numRows); i++ ) {
			y[i] = alpha * v[index[i]];
		}
	} else {
		for ( i = 0; std::cmp_less(i, numRows); i++ ) {
			y[i] = alpha * v[i];
		}
	}

	memcpy( z, w.ToFloatPtr(), w.GetSize() * sizeof( float ) );

	const size_t max = Min(numRows, numColumns);
	for ( i = 0; i < max; i++ ) {
		double diag = (*this)[i][i];

		const double p0 = y[i];
		const double p1 = z[i];
		diag += p0 * p1;

		if ( diag == 0.0 ) {
			return false;
		}

		const double beta = p1 / diag;

		(*this)[i][i] = idMath::Dtof(diag);

		for ( j = i+1; std::cmp_less(j, numColumns); j++ ) {

			d = (*this)[i][j];

			d += p0 * z[j];
			z[j] -= idMath::Dtof(beta * d);

			(*this)[i][j] = idMath::Dtof(d);
		}

		for ( j = i+1; std::cmp_less(j, numRows); j++ ) {

			d = (*this)[j][i];

			y[j] -= idMath::Dtof(p0 * d);
			d += beta * y[j];

			(*this)[j][i] = idMath::Dtof(d);
		}
	}
	return true;
}

/*
============
idMatX::LU_UpdateRowColumn

  Updates the in-place LU factorization to obtain the factors for the matrix:

       [ 0  a  0 ]
  LU + [ d  b  e ]
       [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1], d = w[0,r-1], w[r] = 0.0f, e = w[r+1,numColumns-1]
============
*/
bool idMatX::LU_UpdateRowColumn(const idVecX &v, const idVecX &w, const Ordinal auto r, size_t* index) {
#if 0

	idVecX s;

	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );
	assert( r >= 0 && r < numRows && r < numColumns );
	assert( w[r] == 0.0f );

	s.SetData( Max( numRows, numColumns ), VECX_ALLOCA( Max( numRows, numColumns ) ) );
	s.Zero();
	s[r] = 1.0f;

	if ( !LU_UpdateRankOne( v, s, 1.0f, index ) ) {
		return false;
	}
	if ( !LU_UpdateRankOne( s, w, 1.0f, index ) ) {
		return false;
	}
	return true;

#else

	size_t i = 0, j = 0, rp = 0;
	double beta1 = 0.0, p0 = 0.0, d = 0.0;

	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );
	assert( r >= 0 && std::cmp_less(r, numColumns) && std::cmp_less(r, numRows ));
	assert( w[r] == 0.0f );

	float* y0 = static_cast<float*>(_alloca16(v.GetSize() * sizeof( float )));
	float* z0 = static_cast<float*>(_alloca16(w.GetSize() * sizeof( float )));
	float* y1 = static_cast<float*>(_alloca16(v.GetSize() * sizeof( float )));
	float* z1 = static_cast<float*>(_alloca16(w.GetSize() * sizeof( float )));

	if (std::not_equal_to<>()(index, nullptr)) {
		for ( i = 0; std::cmp_less(i, numRows); i++ ) {
			y0[i] = v[index[i]];
		}
		rp = r;
		for ( i = 0; std::cmp_less(i, numRows); i++ ) {
			if ( index[i] == r ) {
				rp = i;
				break;
			}
		}
	} else {
		memcpy( y0, v.ToFloatPtr(), v.GetSize() * sizeof( float ) );
		rp = r;
	}

	memset( y1, 0, v.GetSize() * sizeof( float ) );
	y1[rp] = 1.0f;

	memset( z0, 0, w.GetSize() * sizeof( float ) );
	z0[r] = 1.0f;

	memcpy( z1, w.ToFloatPtr(), w.GetSize() * sizeof( float ) );

	// update the beginning of the to be updated row and column
	const size_t min = Min(r, rp);
	for ( i = 0; i < min; i++ ) {
		p0 = y0[i];
		beta1 = z1[i] / (*this)[i][i];

		(*this)[i][r] += idMath::Dtof(p0);
		for ( j = i+1; std::cmp_less(j, numColumns); j++ ) {
			z1[j] -= idMath::Dtof(beta1) * (*this)[i][j];
		}
		for ( j = i+1; std::cmp_less(j, numRows); j++ ) {
			y0[j] -= idMath::Dtof(p0) * (*this)[j][i];
		}
		(*this)[rp][i] += idMath::Dtof(beta1);
	}

	// update the lower right corner starting at r,r
	const size_t max = Min(numRows, numColumns);
	for ( i = min; i < max; i++ ) {
		double diag = (*this)[i][i];

		p0 = y0[i];
		const double p1 = z0[i];
		diag += p0 * p1;

		if ( diag == 0.0f ) {
			return false;
		}

		const double beta0 = p1 / diag;

		const double q0 = y1[i];
		const double q1 = z1[i];
		diag += q0 * q1;

		if ( diag == 0.0f ) {
			return false;
		}

		beta1 = q1 / diag;

		(*this)[i][i] = idMath::Dtof(diag);

		for ( j = i+1; std::cmp_less(j, numColumns); j++ ) {

			d = (*this)[i][j];

			d += p0 * z0[j];
			z0[j] -= idMath::Dtof(beta0 * d);

			d += q0 * z1[j];
			z1[j] -= idMath::Dtof(beta1 * d);

			(*this)[i][j] = idMath::Dtof(d);
		}

		for ( j = i+1; std::cmp_less(j, numRows); j++ ) {

			d = (*this)[j][i];

			y0[j] -= idMath::Dtof(p0 * d);
			d += beta0 * y0[j];

			y1[j] -= idMath::Dtof(q0 * d);
			d += beta1 * y1[j];

			(*this)[j][i] = idMath::Dtof(d);
		}
	}
	return true;

#endif
}

/*
============
idMatX::LU_UpdateIncrement

  Updates the in-place LU factorization to obtain the factors for the matrix:

  [ A  a ]
  [ c  b ]

  where: a = v[0,numRows-1], b = v[numRows], c = w[0,numColumns-1], w[numColumns] = 0
============
*/
bool idMatX::LU_UpdateIncrement(const idVecX &v, const idVecX &w, size_t* index) {
	size_t i = 0, j = 0;
	float sum = 0.0f;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );
	assert( w.GetSize() >= numColumns+1 );

	ChangeSize( numRows+1, numColumns+1, true );

	// add row to L
	for ( i = 0; i < numRows - 1; i++ ) {
		sum = w[i];
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[numRows - 1][j] * (*this)[j][i];
		}
		(*this)[numRows - 1 ][i] = sum / (*this)[i][i];
	}

	// add row to the permutation index
	if (std::not_equal_to<>()(index, nullptr)) {
		index[numRows - 1] = numRows - 1;
	}

	// add column to U
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		if (std::not_equal_to<>()(index, nullptr)) {
			sum = v[index[i]];
		} else {
			sum = v[i];
		}
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[i][j] * (*this)[j][numRows - 1];
		}
		(*this)[i][numRows - 1] = sum;
	}

	return true;
}

/*
============
idMatX::LU_UpdateDecrement

  Updates the in-place LU factorization to obtain the factors for the matrix with row r and column r removed.
  v and w should store the column and row of the original matrix respectively.
  If index != NULL then u should store row index[r] of the original matrix. If index == NULL then u = w.
============
*/
bool idMatX::LU_UpdateDecrement(const idVecX &v, const idVecX &w, const idVecX &u, const Ordinal auto r, size_t* index) {
	idVecX v1, w1;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );
	assert( r >= 0 && std::cmp_less(r, numRows) && std::cmp_less(r, numColumns ));

	v1.SetData( numRows, VECX_ALLOCA(numRows));
	w1.SetData( numRows, VECX_ALLOCA(numRows));

	if (std::not_equal_to<>()(index, nullptr)) {
		size_t p = 0;
		size_t i = 0;

		// find the pivot row
		for ( p = i = 0; std::cmp_less(i, numRows); i++ ) {
			if ( index[i] == r ) {
				p = i;
				break;
			}
		}

		// update the row and column to identity
		v1 = -v;
		w1 = -u;

		if ( p != r ) {
			SwapValues( v1[index[r]], v1[index[p]] );
			SwapValues( index[r], index[p] );
		}

		v1[r] += 1.0f;
		w1[r] = 0.0f;

		if ( !LU_UpdateRowColumn( v1, w1, r, index ) ) {
			return false;
		}

		if ( p != r ) {

			if ( idMath::Fabs( u[p] ) < 1e-4f ) {
				// NOTE: an additional row interchange is required for numerical stability
			}

			// move row index[r] of the original matrix to row index[p] of the original matrix
			v1.Zero();
			v1[index[p]] = 1.0f;
			w1 = u - w;

			if ( !LU_UpdateRankOne( v1, w1, 1.0f, index ) ) {
				return false;
			}
		}

		// remove the row from the permutation index
		for ( i = r; i < numRows - 1; i++ ) {
			index[i] = index[i+1];
		}
		for ( i = 0; i < numRows - 1; i++ ) {
			if ( index[i] > r ) {
				index[i]--;
			}
		}

	} else {

		v1 = -v;
		w1 = -w;
		v1[r] += 1.0f;
		w1[r] = 0.0f;

		if ( !LU_UpdateRowColumn( v1, w1, r, index ) ) {
			return false;
		}
	}

	// physically remove the row and column
	Update_Decrement( r );

	return true;
}

/*
============
idMatX::LU_Solve

  Solve Ax = b with A factored in-place as: LU
============
*/
void idMatX::LU_Solve(idVecX &x, const idVecX &b, const size_t* index) const {
	int64 i = 0, j = 0;
	double sum = 0.0;

	assert( x.GetSize() == numColumns && b.GetSize() == numRows );

	// solve L
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		if (std::not_equal_to<>()(index, nullptr)) {
			sum = b[index[i]];
		} else {
			sum = b[i];
		}
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = idMath::Dtof(sum);
	}

	// solve U
	for ( i = idMath::integer_cast<int64>(numRows) - 1; i >= 0; i-- ) {
		sum = x[i];
		for ( j = i + 1; std::cmp_less(j, numRows); j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = idMath::Dtof(sum / (*this)[i][i]);
	}
}

/*
============
idMatX::LU_Inverse

  Calculates the inverse of the matrix which is factored in-place as LU
============
*/
void idMatX::LU_Inverse(idMatX &inv, const size_t* index) const {
	idVecX x, b;

	assert( numRows == numColumns );

	x.SetData( numRows, VECX_ALLOCA(numRows));
	b.SetData( numRows, VECX_ALLOCA(numRows));
	b.Zero();
	inv.SetSize( numRows, numColumns );

	for (size_t i = 0; std::cmp_less(i, numRows); i++ ) {

		b[i] = 1.0f;
		LU_Solve( x, b, index );
		for (size_t j = 0; std::cmp_less(j, numRows); j++ ) {
			inv[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
}

/*
============
idMatX::LU_UnpackFactors

  Unpacks the in-place LU factorization.
============
*/
void idMatX::LU_UnpackFactors( idMatX &L, idMatX &U ) const {
	size_t j = 0;

	L.Zero( numRows, numColumns );
	U.Zero( numRows, numColumns );
	for (size_t i = 0; std::cmp_less(i, numRows); i++ ) {
		for ( j = 0; j < i; j++ ) {
			L[i][j] = (*this)[i][j];
		}
		L[i][i] = 1.0f;
		for ( j = i; std::cmp_less(j, numColumns); j++ ) {
			U[i][j] = (*this)[i][j];
		}
	}
}

/*
============
idMatX::LU_MultiplyFactors

  Multiplies the factors of the in-place LU factorization to form the original matrix.
============
*/
void idMatX::LU_MultiplyFactors(idMatX &m, const size_t* index) const {
	size_t rp = 0;
	double sum = 0.0;

	m.SetSize( numRows, numColumns );

	for ( size_t r = 0; std::cmp_less(r, numRows); r++ ) {

		if (std::not_equal_to<>()(index, nullptr)) {
			rp = index[r];
		} else {
			rp = r;
		}

		// calculate row of matrix
		for ( size_t i = 0; std::cmp_less(i, numColumns); i++ ) {
			if ( i >= r ) {
				sum = (*this)[r][i];
			} else {
				sum = 0.0f;
			}
			for ( size_t j = 0; j <= i && j < r; j++ ) {
				sum += (*this)[r][j] * (*this)[j][i];
			}
			m[rp][i] = idMath::Dtof(sum);
		}
	}
}

/*
============
idMatX::QR_Factor

  in-place factorization: QR
  Q is an orthogonal matrix represented as a product of Householder matrices stored in the lower triangle and c.
  R is a triangular matrix stored in the upper triangle except for the diagonal elements which are stored in d.
  The initial matrix has to be square.
============
*/
bool idMatX::QR_Factor( idVecX &c, idVecX &d ) {
	size_t i = 0;
	double s = 0.0;
	bool singular = false;

	assert( numRows == numColumns );
	assert( c.GetSize() >= numRows && d.GetSize() >= numRows );

	for ( size_t k = 0; k < numRows-1; k++ ) {

		double scale = 0.0f;
		for ( i = k; std::cmp_less(i, numRows); i++ ) {
			s = idMath::Fabs( (*this)[i][k] );
			scale = std::max(s, scale);
		}
		if ( scale == 0.0f ) {
			singular = true;
			c[k] = d[k] = 0.0f;
		} else {

			s = 1.0f / scale;
			for ( i = k; std::cmp_less(i, numRows); i++ ) {
				(*this)[i][k] *= idMath::Dtof(s);
			}

			double sum = 0.0f;
			for ( i = k; std::cmp_less(i, numRows); i++ ) {
				s = (*this)[i][k];
				sum += s * s;
			}

			s = idMath::Sqrt( sum );
			if ( (*this)[k][k] < 0.0f ) {
				s = -s;
			}
			(*this)[k][k] += idMath::Dtof(s);
			c[k] = idMath::Dtof(s * (*this)[k][k]);
			d[k] = idMath::Dtof(-scale * s);

			for ( size_t j = k + 1; std::cmp_less(j, numRows); j++ ) {

				sum = 0.0f;
				for ( i = k; std::cmp_less(i, numRows); i++ ) {
					sum += (*this)[i][k] * (*this)[i][j];
				}
				const double t = sum / c[k];
				for ( i = k; std::cmp_less(i, numRows); i++ ) {
					(*this)[i][j] -= idMath::Dtof(t * (*this)[i][k]);
				}
			}
		}
	}
	d[numRows-1] = (*this)[ (numRows-1) ][ (numRows-1) ];
	if ( d[numRows-1] == 0.0f ) {
		singular = true;
	}

	return !singular;
}

/*
============
idMatX::QR_Rotate

  Performs a JacobOrdinal auto rotation on the rows i and i+1 of the unpacked QR factors.
============
*/
void idMatX::QR_Rotate(idMatX &R, const Ordinal auto i, const float a, const float b) {
	size_t j = 0;
	float f = 0.0f, c = 0.0f, s = 0.0f, w = 0.0f, y = 0.0f;

	if ( a == 0.0f ) {
		c = 0.0f;
		s = ( b >= 0.0f ) ? 1.0f : -1.0f;
	} else if ( idMath::Fabs( a ) > idMath::Fabs( b ) ) {
		f = b / a;
		c = idMath::Fabs( 1.0f / idMath::Sqrt( 1.0f + f * f ) );
		if ( a < 0.0f ) {
			c = -c;
		}
		s = f * c;
	} else {
		f = a / b;
		s = idMath::Fabs( 1.0f / idMath::Sqrt( 1.0f + f * f ) );
		if ( b < 0.0f ) {
			s = -s;
		}
		c = f * s;
	}
	for ( j = i; std::cmp_less(j, numRows); j++ ) {
		y = R[i][j];
		w = R[i+1][j];
		R[i][j] = c * y - s * w;
		R[i+1][j] = s * y + c * w;
	}
	for ( j = 0; std::cmp_less(j, numRows); j++ ) {
		y = (*this)[j][i];
		w = (*this)[j][i+1];
		(*this)[j][i] = c * y - s * w;
		(*this)[j][i+1] = s * y + c * w;
	}
}

/*
============
idMatX::QR_UpdateRankOne

  Updates the unpacked QR factorization to obtain the factors for the matrix: QR + alpha * v * w'
============
*/
bool idMatX::QR_UpdateRankOne( idMatX &R, const idVecX &v, const idVecX &w, const float alpha ) {
	size_t i = 0, k = 0;
	float f = 0.0f;
	idVecX u;

	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );

	u.SetData( v.GetSize(), VECX_ALLOCA(v.GetSize()));
	TransposeMultiply( u, v );
	u *= alpha;

	for ( k = v.GetSize()-1; k > 0; k-- ) {
		if ( u[k] != 0.0f ) {
			break;
		}
	}
	for ( i = k-1; i >= 0; i-- ) {
		QR_Rotate( R, i, u[i], -u[i+1] );
		if ( u[i] == 0.0f ) {
			u[i] = idMath::Fabs( u[i+1] );
		} else if ( idMath::Fabs( u[i] ) > idMath::Fabs( u[i+1] ) ) {
			f = u[i+1] / u[i];
			u[i] = idMath::Fabs( u[i] ) * idMath::Sqrt( 1.0f + f * f );
		} else {
			f = u[i] / u[i+1];
			u[i] = idMath::Fabs( u[i+1] ) * idMath::Sqrt( 1.0f + f * f );
		}
	}
	for ( i = 0; i < v.GetSize(); i++ ) {
		R[0][i] += u[0] * w[i];
	}
	for ( i = 0; i < k; i++ ) {
		QR_Rotate( R, i, -R[i][i], R[i+1][i] );
	}
	return true;
}

/*
============
idMatX::QR_UpdateRowColumn

  Updates the unpacked QR factorization to obtain the factors for the matrix:

       [ 0  a  0 ]
  QR + [ d  b  e ]
       [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1], d = w[0,r-1], w[r] = 0.0f, e = w[r+1,numColumns-1]
============
*/

bool idMatX::QR_UpdateRowColumn(idMatX &R, const idVecX &v, const idVecX &w, const Ordinal auto r) {
	idVecX s;

	assert( v.GetSize() >= numColumns );
	assert( w.GetSize() >= numRows );
	assert( r >= 0 && std::cmp_less(r, numRows) && std::cmp_less(r, numColumns ));
	assert( w[r] == 0.0f );

	s.SetData( Max( numRows, numColumns ), VECX_ALLOCA(Max( numRows, numColumns )));
	s.Zero();
	s[r] = 1.0f;

	if ( !QR_UpdateRankOne( R, v, s, 1.0f ) ) {
		return false;
	}
	if ( !QR_UpdateRankOne( R, s, w, 1.0f ) ) {
		return false;
	}
	return true;
}

/*
============
idMatX::QR_UpdateIncrement

  Updates the unpacked QR factorization to obtain the factors for the matrix:

  [ A  a ]
  [ c  b ]

  where: a = v[0,numRows-1], b = v[numRows], c = w[0,numColumns-1], w[numColumns] = 0
============
*/
bool idMatX::QR_UpdateIncrement( idMatX &R, const idVecX &v, const idVecX &w ) {
	idVecX v2;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );
	assert( w.GetSize() >= numColumns+1 );

	ChangeSize( numRows+1, numColumns+1, true );
	(*this)[numRows-1][numRows-1] = 1.0f;

	R.ChangeSize( R.numRows+1, R.numColumns+1, true );
	R[R.numRows-1][R.numRows-1] = 1.0f;

	v2.SetData( numRows, VECX_ALLOCA(numRows));
	v2 = v;
	v2[numRows-1] -= 1.0f;

	return QR_UpdateRowColumn( R, v2, w, numRows-1 );
}

/*
============
idMatX::QR_UpdateDecrement

  Updates the unpacked QR factorization to obtain the factors for the matrix with row r and column r removed.
  v and w should store the column and row of the original matrix respectively.
============
*/

bool idMatX::QR_UpdateDecrement( idMatX &R, const idVecX &v, const idVecX &w, const Ordinal auto r ) {
	idVecX v1, w1;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( w.GetSize() >= numColumns );
	assert( r >= 0 && std::cmp_less(r, numRows) && std::cmp_less(r, numColumns ));

	v1.SetData( numRows, VECX_ALLOCA(numRows));
	w1.SetData( numRows, VECX_ALLOCA(numRows));

	// update the row and column to identity
	v1 = -v;
	w1 = -w;
	v1[r] += 1.0f;
	w1[r] = 0.0f;

	if ( !QR_UpdateRowColumn( R, v1, w1, r ) ) {
		return false;
	}

	// physically remove the row and column
	Update_Decrement( r );
	R.Update_Decrement( r );

	return true;
}

/*
============
idMatX::QR_Solve

  Solve Ax = b with A factored in-place as: QR
============
*/
void idMatX::QR_Solve( idVecX &x, const idVecX &b, const idVecX &c, const idVecX &d ) const {
	int64 i = 0;
	size_t j = 0;
	double sum = 0.0;

	assert( numRows == numColumns );
	assert( x.GetSize() >= numRows && b.GetSize() >= numRows );
	assert( c.GetSize() >= numRows && d.GetSize() >= numRows );

	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		x[i] = b[i];
	}

	// multiply b with transpose of Q
	for ( i = 0; std::cmp_less(i, numRows-1); i++ ) {

		sum = 0.0f;
		for ( j = i; std::cmp_less(j, numRows); j++ ) {
			sum += (*this)[j][i] * x[j];
		}
		const double t = sum / c[i];
		for ( j = i; std::cmp_less(j, numRows); j++ ) {
			x[j] -= idMath::Dtof(t * (*this)[j][i]);
		}
	}

	// backsubstitution with R
	for ( i = idMath::integer_cast<int64>(numRows)-1; i >= 0; i-- ) {

		sum = x[i];
		for ( j = i + 1; std::cmp_less(j, numRows); j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = idMath::Dtof(sum / d[i]);
	}
}

/*
============
idMatX::QR_Solve

  Solve Ax = b with A factored as: QR
============
*/
void idMatX::QR_Solve( idVecX &x, const idVecX &b, const idMatX &R ) const {
	assert( numRows == numColumns );

	// multiply b with transpose of Q
	TransposeMultiply( x, b );

	// backsubstitution with R
	for ( int64 i = idMath::integer_cast<int64>(numRows) - 1; i >= 0; i-- ) {

		double sum = x[i];
		for ( size_t j = i + 1; std::cmp_less(j, numRows); j++ ) {
			sum -= R[i][j] * x[j];
		}
		x[i] = idMath::Dtof(sum / R[i][i]);
	}
}

/*
============
idMatX::QR_Inverse

  Calculates the inverse of the matrix which is factored in-place as: QR
============
*/
void idMatX::QR_Inverse( idMatX &inv, const idVecX &c, const idVecX &d ) const {
	idVecX x, b;

	assert( numRows == numColumns );

	x.SetData( numRows, VECX_ALLOCA(numRows));
	b.SetData( numRows, VECX_ALLOCA(numRows));
	b.Zero();
	inv.SetSize( numRows, numColumns );

	for (size_t i = 0; std::cmp_less(i, numRows); i++ ) {

		b[i] = 1.0f;
		QR_Solve( x, b, c, d );
		for (size_t j = 0; std::cmp_less(j, numRows); j++ ) {
			inv[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
}

/*
============
idMatX::QR_UnpackFactors

  Unpacks the in-place QR factorization.
============
*/
void idMatX::QR_UnpackFactors( idMatX &Q, idMatX &R, const idVecX &c, const idVecX &d ) const {
	size_t i = 0, j = 0, k = 0;

	Q.Identity( numRows, numColumns );
	for ( i = 0; i < numColumns-1; i++ ) {
		if ( c[i] == 0.0f ) {
			continue;
		}
		for ( j = 0; std::cmp_less(j, numRows); j++ ) {
			double sum = 0.0f;
			for ( k = i; std::cmp_less(k, numColumns); k++ ) {
				sum += (*this)[k][i] * Q[j][k];
			}
			sum /= c[i];
			for ( k = i; std::cmp_less(k, numColumns); k++ ) {
				Q[j][k] -= idMath::Dtof(sum * (*this)[k][i]);
			}
		}
	}

	R.Zero( numRows, numColumns );
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		R[i][i] = d[i];
		for ( j = i+1; std::cmp_less(j, numColumns); j++ ) {
			R[i][j] = (*this)[i][j];
		}
	}
}

/*
============
idMatX::QR_MultiplyFactors

  Multiplies the factors of the in-place QR factorization to form the original matrix.
============
*/
void idMatX::QR_MultiplyFactors( idMatX &m, const idVecX &c, const idVecX &d ) const {
	size_t i = 0, j = 0, k = 0;
	double sum = 0.0;
	idMatX Q;

	Q.Identity( numRows, numColumns );
	for ( i = 0; i < numColumns-1; i++ ) {
		if ( c[i] == 0.0f ) {
			continue;
		}
		for ( j = 0; std::cmp_less(j, numRows); j++ ) {
			sum = 0.0f;
			for ( k = i; std::cmp_less(k, numColumns); k++ ) {
				sum += (*this)[k][i] * Q[j][k];
			}
			sum /= c[i];
			for ( k = i; std::cmp_less(k, numColumns); k++ ) {
				Q[j][k] -= idMath::Dtof(sum * (*this)[k][i]);
			}
		}
	}

	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		for ( j = 0; std::cmp_less(j, numColumns); j++ ) {
			sum = Q[i][j] * d[i];
			for ( k = 0; k < i; k++ ) {
				sum += Q[i][k] * (*this)[j][k];
			}
			m[i][j] = idMath::Dtof(sum);
		}
	}
}

/*
============
idMatX::Pythag

  Computes (a^2 + b^2)^1/2 without underflow or overflow.
============
*/
float idMatX::Pythag(const float a, const float b ) const {
	double ct = 0.0;

	const double at = idMath::Fabs(a);
	const double bt = idMath::Fabs(b);

	if ( (at > bt) && (at != 0.0) )
	{
		ct = bt / at;
		return idMath::Dtof(at * idMath::Sqrt( 1.0f + ct * ct ));
	} else {
		if ( bt != 0.0 ) {
			ct = at / bt;
			return idMath::Dtof(bt * idMath::Sqrt( 1.0f + ct * ct ));
		} else {
			return 0.0f;
		}
	}
}

double idMatX::Pythag(const double a, const double b) const {
	double ct = 0.0;

	const double at = idMath::Fabs(a);
	const double bt = idMath::Fabs(b);

	if ((at > bt) && (at != 0.0))
	{
		ct = bt / at;
		return at * idMath::Sqrt(1.0 + ct * ct);
	}
	else {
		if (bt != 0.0) {
			ct = at / bt;
			return bt * idMath::Sqrt(1.0 + ct * ct);
		}
		else {
			return 0.0;
		}
	}
}

/*
============
idMatX::SVD_BiDiag
============
*/
void idMatX::SVD_BiDiag( idVecX &w, idVecX &rv1, float &anorm ) {
	size_t j = 0, k = 0;
	double f = 0.0, h = 0.0, s = 0.0, scale = 0.0;

	anorm = 0.0f;
	double g = s = scale = 0.0f;
	for ( size_t i = 0; std::cmp_less(i, numColumns); i++ ) {
		const size_t l = i + 1;
		rv1[i] = idMath::Dtof(scale * g);
		g = s = scale = 0.0;
		if (std::cmp_less(i, numRows)) {
			for ( k = i; std::cmp_less(k, numRows); k++ ) {
				scale += idMath::Fabs( (*this)[k][i] );
			}
			if ( scale != 0.0 ) {
				for ( k = i; std::cmp_less(k, numRows); k++ ) {
					(*this)[k][i] = idMath::Dtof((*this)[k][i] / scale);
					s += (*this)[k][i] * (*this)[k][i];
				}
				f = (*this)[i][i];
				g = idMath::Sqrt( s );
				if ( f >= 0.0f ) {
					g = -g;
				}
				h = f * g - s;
				(*this)[i][i] = idMath::Dtof(f - g);
				if ( i != (numColumns-1) ) {
					for ( j = l; std::cmp_less(j, numColumns); j++ ) {
						for ( s = 0.0f, k = i; std::cmp_less(k, numRows); k++ ) {
							s += (*this)[k][i] * (*this)[k][j];
						}
						f = s / h;
						for ( k = i; std::cmp_less(k, numRows); k++ ) {
							(*this)[k][j] += idMath::Dtof(f * (*this)[k][i]);
						}
					}
				}
				for ( k = i; std::cmp_less(k, numRows); k++ ) {
					(*this)[k][i] *= idMath::Dtof(scale);
				}
			}
		}
		w[i] = idMath::Dtof(scale * g);
		g = s = scale = 0.0f;
		if (std::cmp_less(i, numRows) && i != (numColumns-1) ) {
			for ( k = l; std::cmp_less(k, numColumns); k++ ) {
				scale += idMath::Fabs( (*this)[i][k] );
			}
			if ( scale != 0.0 ) {
				for ( k = l; std::cmp_less(k, numColumns); k++ ) {
					(*this)[i][k] = idMath::Dtof((*this)[i][k] / scale);
					s += (*this)[i][k] * (*this)[i][k];
				}
				f = (*this)[i][l];
				g = idMath::Sqrt( s );
				if ( f >= 0.0f ) {
					g = -g;
				}
				h = 1.0f / ( f * g - s );
				(*this)[i][l] = idMath::Dtof(f - g);
				for ( k = l; std::cmp_less(k, numColumns); k++ ) {
					rv1[k] = idMath::Dtof((*this)[i][k] * h);
				}
				if ( i != (numRows-1) ) {
					for ( j = l; std::cmp_less(j, numRows); j++ ) {
						for ( s = 0.0f, k = l; std::cmp_less(k, numColumns); k++ ) {
							s += (*this)[j][k] * (*this)[i][k];
						}
						for ( k = l; std::cmp_less(k, numColumns); k++ ) {
							(*this)[j][k] += idMath::Dtof(s * rv1[k]);
						}
					}
				}
				for ( k = l; std::cmp_less(k, numColumns); k++ ) {
					(*this)[i][k] *= idMath::Dtof(scale);
				}
			}
		}
		const double r = idMath::Fabs(w[i]) + idMath::Fabs(rv1[i]);
		anorm = idMath::Dtof(std::max<double>(r, anorm));
	}
}

/*
============
idMatX::SVD_InitialWV
============
*/
void idMatX::SVD_InitialWV( idVecX &w, idMatX &V, idVecX &rv1 ) {
	int64 i = 0;
	size_t j = 0, k = 0, l = 0;
	double s = 0.0;

	double g = 0.0;
	for ( i = idMath::integer_cast<int64>(numColumns) - 1; i >= 0; i-- ) {
		l = i + 1;
		if ( i < (idMath::integer_cast<int64>(numColumns) - 1)) {
			if ( g != 0.0 ) {
				for ( j = l; std::cmp_less(j, numColumns); j++ ) {
					V[j][i] = idMath::Dtof(((*this)[i][j] / (*this)[i][l]) / g);
				}
				// double division to reduce underflow
				for ( j = l; std::cmp_less(j, numColumns); j++ ) {
					for ( s = 0.0f, k = l; std::cmp_less(k, numColumns); k++ ) {
						s += (*this)[i][k] * V[k][j];
					}
					for ( k = l; std::cmp_less(k, numColumns); k++ ) {
						V[k][j] += idMath::Dtof(s * V[k][i]);
					}
				}
			}
			for ( j = l; std::cmp_less(j, numColumns); j++ ) {
				V[i][j] = V[j][i] = 0.0f;
			}
		}
		V[i][i] = 1.0f;
		g = rv1[i];
	}
	for ( i = idMath::integer_cast<int64>(numColumns) - 1; i >= 0; i-- ) {
		l = i + 1;
		g = w[i];
		if ( i < (idMath::integer_cast<int64>(numColumns) - 1) ) {
			for ( j = l; std::cmp_less(j, numColumns); j++ ) {
				(*this)[i][j] = 0.0f;
			}
		}
		if ( g != 0.0 ) {
			g = 1.0 / g;
			if ( i != (idMath::integer_cast<int64>(numColumns) - 1) ) {
				for ( j = l; std::cmp_less(j, numColumns); j++ ) {
					for ( s = 0.0f, k = l; std::cmp_less(k, numRows); k++ ) {
						s += (*this)[k][i] * (*this)[k][j];
					}
					const double f = (s / (*this)[i][i]) * g;
					for ( k = i; std::cmp_less(k, numRows); k++ ) {
						(*this)[k][j] += idMath::Dtof(f * (*this)[k][i]);
					}
				}
			}
			for ( j = i; std::cmp_less(j, numRows); j++ ) {
				(*this)[j][i] *= idMath::Dtof(g);
			}
		}
		else {
			for ( j = i; std::cmp_less(j, numRows); j++ ) {
				(*this)[j][i] = 0.0f;
			}
		}
		(*this)[i][i] += 1.0f;
	}
}

/*
============
idMatX::SVD_Factor

  in-place factorization: U * Diag(w) * V.Transpose()
  known as the Singular Value Decomposition.
  U is a column-orthogonal matrix which overwrites the original matrix.
  w is a diagonal matrix with all elements >= 0 which are the singular values.
  V is the transpose of an orthogonal matrix.
============
*/
bool idMatX::SVD_Factor( idVecX &w, idMatX &V ) {
	int64 i = 0, l = 0;
	size_t j = 0, jj = 0;
	double c = 0.0, f = 0.0, h = 0.0, s = 0.0, y = 0.0, z = 0.0, g = 0.0;
	float anorm = 0.0f;
	idVecX rv1;

	if ( numRows < numColumns ) {
		return false;
	}

	rv1.SetData( numColumns, VECX_ALLOCA(numColumns));
	rv1.Zero();
	w.Zero( numColumns );
	V.Zero( numColumns, numColumns );

	SVD_BiDiag( w, rv1, anorm );
	SVD_InitialWV( w, V, rv1 );

	for ( int64 k = idMath::integer_cast<int64>(numColumns) - 1; k >= 0; k-- ) {
		for (size_t its = 1; its <= 30; its++ ) {
			int flag = 1;
			size_t nm = 0;
			for ( l = k; l >= 0; l-- ) {
				nm = l - 1;
				if ( std::equal_to<>()(idMath::Fabs( rv1[l] ) + anorm, anorm) /* idMath::Fabs( rv1[l] ) < idMath::FLT_EPSILON */ ) {
					flag = 0;
					break;
				}
				if ( std::equal_to<>()(idMath::Fabs( w[nm] ) + anorm, anorm ) /* idMath::Fabs( w[nm] ) < idMath::FLT_EPSILON */ ) {
					break;
				}
			}
			if ( flag ) {
				c = 0.0;
				s = 1.0;
				for ( i = l; i <= k; i++ ) {
					f = s * rv1[i];

					if (std::not_equal_to<>()(idMath::Fabs( f ) + anorm, anorm) /* idMath::Fabs( f ) > idMath::FLT_EPSILON */ ) {
						g = w[i];
						h = Pythag( f, g );
						w[i] = idMath::Dtof(h);
						h = 1.0 / h;
						c = g * h;
						s = -f * h;
						for ( j = 0; std::cmp_less(j, numRows); j++ ) {
							y = (*this)[j][nm];
							z = (*this)[j][i];
							(*this)[j][nm] = idMath::Dtof(y * c + z * s);
							(*this)[j][i] = idMath::Dtof(z * c - y * s);
						}
					}
				}
			}
			z = w[k];
			if ( l == k ) {
				if ( z < 0.0 ) {
					w[k] = idMath::Dtof(-z);
					for ( j = 0; std::cmp_less(j, numColumns); j++ ) {
						V[j][k] = -V[j][k];
					}
				}
				break;
			}
			if ( its == 30 ) {
				return false;		// no convergence
			}
			double x = w[l];
			nm = k - 1;
			y = w[nm];
			g = rv1[nm];
			h = rv1[k];
			f = ( ( y - z ) * ( y + z ) + ( g - h ) * ( g + h ) ) / ( 2.0f * h * y );
			g = Pythag( f, 1.0 );
			const double r = (f >= 0.0 ? g : -g);
			f= ( ( x - z ) * ( x + z ) + h * ( ( y / ( f + r ) ) - h ) ) / x;
			c = s = 1.0;
			for ( j = l; j <= nm; j++ ) {
				i = idMath::integer_cast<int64>(j + 1);
				g = rv1[i];
				y = w[i];
				h = s * g;
				g = c * g;
				z = Pythag( f, h );
				rv1[j] = idMath::Dtof(z);
				c = f / z;
				s = h / z;
				f = x * c + g * s;
				g = g * c - x * s;
				h = y * s;
				y = y * c;
				for ( jj = 0; std::cmp_less(jj, numColumns); jj++ ) {
					x = V[jj][j];
					z = V[jj][i];
					V[jj][j] = idMath::Dtof(x * c + z * s);
					V[jj][i] = idMath::Dtof(z * c - x * s);
				}
				z = Pythag( f, h );
				w[j] = idMath::Dtof(z);
				if ( z != 0.0 ) {
					z = 1.0 / z;
					c = f * z;
					s = h * z;
				}
				f = ( c * g ) + ( s * y );
				x = ( c * y ) - ( s * g );
				for ( jj = 0; std::cmp_less(jj, numRows); jj++ ) {
					y = (*this)[jj][j];
					z = (*this)[jj][i];
					(*this)[jj][j] = idMath::Dtof(y * c + z * s);
					(*this)[jj][i] = idMath::Dtof(z * c - y * s);
				}
			}
			rv1[l] = 0.0f;
			rv1[k] = idMath::Dtof(f);
			w[k] = idMath::Dtof(x);
		}
	}
	return true;
}

/*
============
idMatX::SVD_Solve

  Solve Ax = b with A factored as: U * Diag(w) * V.Transpose()
============
*/
void idMatX::SVD_Solve( idVecX &x, const idVecX &b, const idVecX &w, const idMatX &V ) const {
	int i, j;
	double sum;
	idVecX tmp;

	assert( x.GetSize() >= numColumns );
	assert( b.GetSize() >= numColumns );
	assert( w.GetSize() == numColumns );
	assert( V.GetNumRows() == numColumns && V.GetNumColumns() == numColumns );

	tmp.SetData( numColumns, VECX_ALLOCA(numColumns));

	for ( i = 0; std::cmp_less(i, numColumns); i++ ) {
		sum = 0.0f;
		if ( w[i] >= idMath::FLT_EPSILON ) {
			for ( j = 0; std::cmp_less(j, numRows); j++ ) {
				sum += (*this)[j][i] * b[j];
			}
			sum /= w[i];
		}
		tmp[i] = idMath::Dtof(sum);
	}
	for ( i = 0; std::cmp_less(i, numColumns); i++ ) {
		sum = 0.0f;
		for ( j = 0; std::cmp_less(j, numColumns); j++ ) {
			sum += V[i][j] * tmp[j];
		}
		x[i] = idMath::Dtof(sum);
	}
}

/*
============
idMatX::SVD_Inverse

  Calculates the inverse of the matrix which is factored in-place as: U * Diag(w) * V.Transpose()
============
*/
void idMatX::SVD_Inverse( idMatX &inv, const idVecX &w, const idMatX &V ) const {
	size_t i = 0, j = 0;

	assert( numRows == numColumns );

	idMatX V2 = V;

	// V * [diag(1/w[i])]
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		double wi = w[i];
		wi = ( wi < idMath::FLT_EPSILON ) ? 0.0f : 1.0f / wi;
		for ( j = 0; std::cmp_less(j, numColumns); j++ ) {
			V2[j][i] *= idMath::Dtof(wi);
		}
	}

	// V * [diag(1/w[i])] * Ut
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		for ( j = 0; std::cmp_less(j, numColumns); j++ ) {
			double sum = V2[i][0] * (*this)[j][0];
			for (size_t k = 1; std::cmp_less(k, numColumns); k++ ) {
				sum += V2[i][k] * (*this)[j][k];
			}
			inv[i][j] = idMath::Dtof(sum);
		}
	}
}

/*
============
idMatX::SVD_MultiplyFactors

  Multiplies the factors of the in-place SVD factorization to form the original matrix.
============
*/
void idMatX::SVD_MultiplyFactors( idMatX &m, const idVecX &w, const idMatX &V ) const {
	size_t i = 0;

	m.SetSize( numRows, V.GetNumRows() );

	for (size_t r = 0; std::cmp_less(r, numRows); r++ ) {
		// calculate row of matrix
		if ( w[r] >= idMath::FLT_EPSILON ) {
			for ( i = 0; i < V.GetNumRows(); i++ ) {
				double sum = 0.0f;
				for (size_t j = 0; std::cmp_less(j, numColumns); j++ ) {
					sum += (*this)[r][j] * V[i][j];
				}
				m[r][i] = idMath::Dtof(sum * w[r]);
			}
		} else {
			for ( i = 0; i < V.GetNumRows(); i++ ) {
				m[r][i] = 0.0f;
			}
		}
	}
}

/*
============
idMatX::Cholesky_Factor

  in-place Cholesky factorization: LL'
  L is a triangular matrix stored in the lower triangle.
  The upper triangle is not cleared.
  The initial matrix has to be symmetric positive definite.
============
*/
bool idMatX::Cholesky_Factor() {
	size_t k = 0;
	double sum = 0;

	assert( numRows == numColumns );

	double* invSqrt = static_cast<double*>(_alloca16(numRows * sizeof( double )));

	for ( size_t i = 0; std::cmp_less(i, numRows); i++ ) {

		for ( size_t j = 0; j < i; j++ ) {

			sum = (*this)[i][j];
			for ( k = 0; k < j; k++ ) {
				sum -= (*this)[i][k] * (*this)[j][k];
			}
			(*this)[i][j] = idMath::Dtof(sum * invSqrt[j]);
		}

		sum = (*this)[i][i];
		for ( k = 0; k < i; k++ ) {
			sum -= (*this)[i][k] * (*this)[i][k];
		}

		if ( sum <= 0.0f ) {
			return false;
		}

		invSqrt[i] = idMath::InvSqrt( sum );
		(*this)[i][i] = idMath::Dtof(invSqrt[i] * sum);
	}
	return true;
}

/*
============
idMatX::Cholesky_UpdateRankOne

  Updates the in-place Cholesky factorization to obtain the factors for the matrix: LL' + alpha * v * v'
  If offset > 0 only the lower right corner starting at (offset, offset) is updated.
============
*/
bool idMatX::Cholesky_UpdateRankOne( const idVecX &v, float alpha, const size_t offset ) {
	double newDiag = 0.0;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( offset >= 0 && std::cmp_less(offset, numRows ));

	float* y = static_cast<float*>(_alloca16(v.GetSize() * sizeof( float )));
	memcpy( y, v.ToFloatPtr(), v.GetSize() * sizeof( float ) );

	for ( size_t i = offset; std::cmp_less(i, numColumns); i++ ) {
		const double p = y[i];
		const double diag = (*this)[i][i];
		const double invDiag = 1.0f / diag;
		const double diagSqr = diag * diag;
		const double newDiagSqr = diagSqr + alpha * p * p;

		if ( newDiagSqr <= 0.0f ) {
			return false;
		}

		newDiag = idMath::Sqrt(newDiagSqr);
		(*this)[i][i] = idMath::Dtof(newDiag);

		alpha = idMath::Dtof(alpha / newDiagSqr);
		const double beta = p * alpha;
		alpha = idMath::Dtof(alpha * diagSqr);

		for ( size_t j = i + 1; std::cmp_less(j, numRows); j++ ) {

			double d = (*this)[j][i] * invDiag;

			y[j] -= idMath::Dtof(p * d);
			d += beta * y[j];

			(*this)[j][i] = idMath::Dtof(d * newDiag);
		}
	}
	return true;
}

/*
============
idMatX::Cholesky_UpdateRowColumn

  Updates the in-place Cholesky factorization to obtain the factors for the matrix:

        [ 0  a  0 ]
  LL' + [ a  b  c ]
        [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1]
============
*/

bool idMatX::Cholesky_UpdateRowColumn( const idVecX &v, const Ordinal auto r ) {
	size_t i = 0, j = 0;
	double sum = 0.0;
	idVecX addSub;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( r >= 0 && std::cmp_less(r, numRows ));

	addSub.SetData( numColumns, static_cast<float*>(_alloca16(numColumns * sizeof( float ))) );

	if ( r == 0 ) {

		if ( numColumns == 1 ) {
			const double v0 = v[0];
			sum = (*this)[0][0];
			sum = sum * sum; 
			sum = sum + v0; 
			if ( sum <= 0.0f ) {
				return false;
			}
			(*this)[0][0] = idMath::Dtof(idMath::Sqrt( sum ));
			return true;
		}
		for ( i = 0; std::cmp_less(i, numColumns); i++ ) {
			addSub[i] = v[i];
		}

	} else {

		double* original = static_cast<double*>(_alloca16(numColumns * sizeof( double )));

		// calculate original row/column of matrix
		for ( i = 0; std::cmp_less(i, numRows); i++ ) {
			sum = 0.0f;
			for ( j = 0; j <= i; j++ ) {
				sum += (*this)[r][j] * (*this)[i][j];
			}
			original[i] = sum;
		}

		// solve for y in L * y = original + v
		for ( i = 0; i < r; i++ ) {
			sum = original[i] + v[i];
			for ( j = 0; j < i; j++ ) {
				sum -= (*this)[r][j] * (*this)[i][j];
			}
			(*this)[r][i] = idMath::Dtof(sum / (*this)[i][i]);
		}

		// if the last row/column of the matrix is updated
		if ( r == numColumns - 1 ) {
			// only calculate new diagonal
			sum = original[r] + v[r];
			for ( j = 0; j < r; j++) {
				sum -= (*this)[r][j] * (*this)[r][j];
			}
			if ( sum <= 0.0f ) {
				return false;
			}
			(*this)[r][r] = idMath::Dtof(idMath::Sqrt( sum ));
			return true;
		}

		// calculate the row/column to be added to the lower right sub matrix starting at (r, r)
		for ( i = r; std::cmp_less(i, numColumns); i++ ) {
			sum = 0.0f;
			for ( j = 0; j <= r; j++ ) {
				sum += (*this)[r][j] * (*this)[i][j];
			}
			addSub[i] = v[i] - idMath::Dtof( sum - original[i] );
		}
	}

	// add row/column to the lower right sub matrix starting at (r, r)

#if 0

	idVecX v1, v2;
	double d;

	v1.SetData( numColumns, (float *) _alloca16( numColumns * sizeof( float ) ) );
	v2.SetData( numColumns, (float *) _alloca16( numColumns * sizeof( float ) ) );

	d = idMath::SQRT_1OVER2;
	v1[r] = ( 0.5f * addSub[r] + 1.0f ) * d;
	v2[r] = ( 0.5f * addSub[r] - 1.0f ) * d;
	for ( i = r+1; i < numColumns; i++ ) {
		v1[i] = v2[i] = addSub[i] * d;
	}

	// update
	if ( !Cholesky_UpdateRankOne( v1, 1.0f, r ) ) {
		return false;
	}
	// downdate
	if ( !Cholesky_UpdateRankOne( v2, -1.0f, r ) ) {
		return false;
	}

#else

	double newDiag = 0.0;

	double* v1 = static_cast<double*>(_alloca16(numColumns * sizeof(double)));
	double* v2 = static_cast<double*>(_alloca16(numColumns * sizeof(double)));

	double d = idMath::SQRT_1OVER2;
	v1[r] = ( 0.5f * addSub[r] + 1.0f ) * d;
	v2[r] = ( 0.5f * addSub[r] - 1.0f ) * d;
	for ( i = r+1; std::cmp_less(i, numColumns); i++ ) {
		v1[i] = v2[i] = addSub[i] * d;
	}

	double alpha1 = 1.0f;
	double alpha2 = -1.0f;

	// simultaneous update/downdate of the sub matrix starting at (r, r)
	for ( i = r; std::cmp_less(i, numColumns); i++ ) {
		const double p1 = v1[i];
		const double diag = (*this)[i][i];
		const double invDiag = 1.0f / diag;
		double diagSqr = diag * diag;
		double newDiagSqr = diagSqr + alpha1 * p1 * p1;

		if ( newDiagSqr <= 0.0f ) {
			return false;
		}

		alpha1 /= newDiagSqr;
		const double beta1 = p1 * alpha1;
		alpha1 *= diagSqr;

		const double p2 = v2[i];
		diagSqr = newDiagSqr;
		newDiagSqr = diagSqr + alpha2 * p2 * p2;

		if ( newDiagSqr <= 0.0f ) {
			return false;
		}

		newDiag = idMath::Sqrt(newDiagSqr);
		(*this)[i][i] = idMath::Dtof(newDiag);

		alpha2 /= newDiagSqr;
		const double beta2 = p2 * alpha2;
		alpha2 *= diagSqr;

		for ( j = i+1; std::cmp_less(j, numRows); j++ ) {

			d = (*this)[j][i] * invDiag;

			v1[j] -= p1 * d;
			d += beta1 * v1[j];

			v2[j] -= p2 * d;
			d += beta2 * v2[j];

			(*this)[j][i] = idMath::Dtof(d * newDiag);
		}
	}

#endif

	return true;
}

/*
============
idMatX::Cholesky_UpdateIncrement

  Updates the in-place Cholesky factorization to obtain the factors for the matrix:

  [ A  a ]
  [ a  b ]

  where: a = v[0,numRows-1], b = v[numRows]
============
*/
bool idMatX::Cholesky_UpdateIncrement( const idVecX &v ) {
	size_t i = 0;
	double sum = 0.0;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );

	ChangeSize( numRows+1, numColumns+1, false );

	double* x = static_cast<double*>(_alloca16(numRows * sizeof( double )));

	// solve for x in L * x = v
	for ( i = 0; i < numRows - 1; i++ ) {
		sum = v[i];
		for (size_t j = 0; std::cmp_less(j, i); j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = sum / (*this)[i][i];
	}

	// calculate new row of L and calculate the square of the diagonal entry
	sum = v[numRows - 1];
	for ( i = 0; i < numRows - 1; i++ ) {
		(*this)[numRows - 1][i] = idMath::Dtof(x[i]);
		sum -= x[i] * x[i];
	}

	if ( sum <= 0.0f ) {
		return false;
	}

	// store the diagonal entry
	(*this)[numRows - 1][numRows - 1] = idMath::Dtof(idMath::Sqrt( sum ));

	return true;
}

/*
============
idMatX::Cholesky_UpdateDecrement

  Updates the in-place Cholesky factorization to obtain the factors for the matrix with row r and column r removed.
  v should store the row of the original matrix.
============
*/

bool idMatX::Cholesky_UpdateDecrement( const idVecX &v, const Ordinal auto r ) {
	idVecX v1;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( r >= 0 && std::cmp_less(r, numRows ));

	v1.SetData( numRows, VECX_ALLOCA(numRows));

	// update the row and column to identity
	v1 = -v;
	v1[r] += 1.0f;

	// NOTE:	msvc compiler bug: the this pointer stored in edi is expected to stay
	//			untouched when calling Cholesky_UpdateRowColumn in the if statement
#if 0
	if ( !Cholesky_UpdateRowColumn( v1, r ) ) {
#else
	const bool ret = Cholesky_UpdateRowColumn( v1, r );
	if ( !ret ) {
#endif
		return false;
	}

	// physically remove the row and column
	Update_Decrement( r );

	return true;
}

/*
============
idMatX::Cholesky_Solve

  Solve Ax = b with A factored in-place as: LL'
============
*/
void idMatX::Cholesky_Solve( idVecX &x, const idVecX &b ) const {
	int64 i = 0, j = 0;
	double sum = 0.0;

	assert( numRows == numColumns );
	assert( x.GetSize() >= numRows && b.GetSize() >= numRows );

	// solve L
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		sum = b[i];
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = idMath::Dtof(sum / (*this)[i][i]);
	}

	// solve Lt
	for ( i = idMath::integer_cast<int64>(numRows) - 1; i >= 0; i-- ) {
		sum = x[i];
		for ( j = i + 1; std::cmp_less(j, numRows); j++ ) {
			sum -= (*this)[j][i] * x[j];
		}
		x[i] = idMath::Dtof(sum / (*this)[i][i]);
	}
}

/*
============
idMatX::Cholesky_Inverse

  Calculates the inverse of the matrix which is factored in-place as: LL'
============
*/
void idMatX::Cholesky_Inverse( idMatX &inv ) const {
	idVecX x, b;

	assert( numRows == numColumns );

	x.SetData( numRows, VECX_ALLOCA(numRows));
	b.SetData( numRows, VECX_ALLOCA(numRows));
	b.Zero();
	inv.SetSize( numRows, numColumns );

	for ( size_t i = 0; std::cmp_less(i, numRows); i++ ) {

		b[i] = 1.0f;
		Cholesky_Solve( x, b );
		for ( size_t j = 0; std::cmp_less(j, numRows); j++ ) {
			inv[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
}

/*
============
idMatX::Cholesky_MultiplyFactors

  Multiplies the factors of the in-place Cholesky factorization to form the original matrix.
============
*/
void idMatX::Cholesky_MultiplyFactors( idMatX &m ) const {
	m.SetSize( numRows, numColumns );

	for ( size_t r = 0; std::cmp_less(r, numRows); r++ ) {

		// calculate row of matrix
		for ( size_t i = 0; std::cmp_less(i, numRows); i++ ) {
			double sum = 0.0f;
			for ( size_t j = 0; j <= i && j <= r; j++ ) {
				sum += (*this)[r][j] * (*this)[i][j];
			}
			m[r][i] = idMath::Dtof(sum);
		}
	}
}

/*
============
idMatX::LDLT_Factor

  in-place factorization: LDL'
  L is a triangular matrix stored in the lower triangle.
  L has ones on the diagonal that are not stored.
  D is a diagonal matrix stored on the diagonal.
  The upper triangle is not cleared.
  The initial matrix has to be symmetric.
============
*/
bool idMatX::LDLT_Factor() {
	size_t j = 0;
	double d = 0.0;

	assert( numRows == numColumns );

	double* v = static_cast<double*>(_alloca16(numRows * sizeof( double )));

	for ( size_t i = 0; std::cmp_less(i, numRows); i++ ) {

		double sum = (*this)[i][i];
		for ( j = 0; j < i; j++ ) {
			d = (*this)[i][j];
		    v[j] = (*this)[j][j] * d;
		    sum -= v[j] * d;
		}

		if ( sum == 0.0f ) {
			return false;
		}

		(*this)[i][i] = idMath::Dtof(sum);
		d = 1.0f / sum;

		for ( j = i + 1; std::cmp_less(j, numRows); j++ ) {
		    sum = (*this)[j][i];
			for ( size_t k = 0; k < i; k++ ) {
				sum -= (*this)[j][k] * v[k];
			}
		    (*this)[j][i] = idMath::Dtof(sum * d);
		}
	}

	return true;
}

/*
============
idMatX::LDLT_UpdateRankOne

  Updates the in-place LDL' factorization to obtain the factors for the matrix: LDL' + alpha * v * v'
  If offset > 0 only the lower right corner starting at (offset, offset) is updated.
============
*/
bool idMatX::LDLT_UpdateRankOne( const idVecX &v, float alpha, const size_t offset ) {
	double newDiag = 0.0;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( offset >= 0 && std::cmp_less(offset, numRows ));

	float* y = static_cast<float*>(_alloca16(v.GetSize() * sizeof( float )));
	memcpy( y, v.ToFloatPtr(), v.GetSize() * sizeof( float ) );

	for ( size_t i = offset; std::cmp_less(i, numColumns); i++ ) {
		const double p = y[i];
		const double diag = (*this)[i][i];
		newDiag = diag + alpha * p * p;
		(*this)[i][i] = idMath::Dtof(newDiag);

		if ( newDiag == 0.0f ) {
			return false;
		}

		alpha = idMath::Dtof(alpha / newDiag);
		const double beta = p * alpha;
		alpha = idMath::Dtof(alpha * diag);

		for ( size_t j = i + 1; std::cmp_less(j, numRows); j++ ) {

			double d = (*this)[j][i];

			y[j] -= idMath::Dtof(p * d);
			d += beta * y[j];

			(*this)[j][i] = idMath::Dtof(d);
		}
	}

	return true;
}

/*
============
idMatX::LDLT_UpdateRowColumn

  Updates the in-place LDL' factorization to obtain the factors for the matrix:

         [ 0  a  0 ]
  LDL' + [ a  b  c ]
         [ 0  c  0 ]

  where: a = v[0,r-1], b = v[r], c = v[r+1,numRows-1]
============
*/

bool idMatX::LDLT_UpdateRowColumn(const idVecX &v, const Ordinal auto r) {
	size_t i = 0, j = 0;
	idVecX addSub;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( r >= 0 && std::cmp_less(r, numRows ));

	addSub.SetData( numColumns, static_cast<float*>(_alloca16(numColumns * sizeof( float ))) );

	if ( r == 0 ) {

		if ( numColumns == 1 ) {
			(*this)[0][0] += v[0];
			return true;
		}
		for ( i = 0; std::cmp_less(i, numColumns); i++ ) {
			addSub[i] = v[i];
		}

	} else {
		double sum = 0.0;

		double* original = static_cast<double*>(_alloca16(numColumns * sizeof( double )));
		float* y = static_cast<float*>(_alloca16(numColumns * sizeof( float )));

		// calculate original row/column of matrix
		for ( i = 0; i < r; i++ ) {
			y[i] = (*this)[r][i] * (*this)[i][i];
		}
		for ( i = 0; std::cmp_less(i, numColumns); i++ ) {
			if ( i < r ) {
				sum = (*this)[i][i] * (*this)[r][i];
			} else if ( i == r ) {
				sum = (*this)[r][r];
			} else {
				sum = (*this)[r][r] * (*this)[i][r];
			}
			for ( j = 0; j < i && j < r; j++ ) {
				sum += (*this)[i][j] * y[j];
			}
			original[i] = sum;
		}

		// solve for y in L * y = original + v
		for ( i = 0; i < r; i++ ) {
			sum = original[i] + v[i];
			for ( j = 0; j < i; j++ ) {
				sum -= (*this)[i][j] * y[j];
			}
			y[i] = idMath::Dtof(sum);
		}

		// calculate new row of L
		for ( i = 0; i < r; i++ ) {
			(*this)[r][i] = y[i] / (*this)[i][i];
		}

		// if the last row/column of the matrix is updated
		if ( r == numColumns - 1 ) {
			// only calculate new diagonal
			sum = original[r] + v[r];
			for ( j = 0; j < r; j++ ) {
				sum -= (*this)[r][j] * y[j];
			}
			if ( sum == 0.0f ) {
				return false;
			}
			(*this)[r][r] = idMath::Dtof(sum);
			return true;
		}

		// calculate the row/column to be added to the lower right sub matrix starting at (r, r)
		for ( i = 0; i < r; i++ ) {
			y[i] = (*this)[r][i] * (*this)[i][i];
		}
		for ( i = r; std::cmp_less(i, numColumns); i++ ) {
			if ( i == r ) {
				sum = (*this)[r][r];
			} else {
				sum = (*this)[r][r] * (*this)[i][r];
			}
			for ( j = 0; j < r; j++ ) {
				sum += (*this)[i][j] * y[j];
			}
			addSub[i] = v[i] - idMath::Dtof( sum - original[i] );
		}
	}

	// add row/column to the lower right sub matrix starting at (r, r)

#if 0

	idVecX v1, v2;
	double d;

	v1.SetData( numColumns, (float *) _alloca16( numColumns * sizeof( float ) ) );
	v2.SetData( numColumns, (float *) _alloca16( numColumns * sizeof( float ) ) );

	d = idMath::SQRT_1OVER2;
	v1[r] = ( 0.5f * addSub[r] + 1.0f ) * d;
	v2[r] = ( 0.5f * addSub[r] - 1.0f ) * d;
	for ( i = r+1; i < numColumns; i++ ) {
		v1[i] = v2[i] = addSub[i] * d;
	}

	// update
	if ( !LDLT_UpdateRankOne( v1, 1.0f, r ) ) {
		return false;
	}
	// downdate
	if ( !LDLT_UpdateRankOne( v2, -1.0f, r ) ) {
		return false;
	}

#else

	double* v1 = static_cast<double*>(_alloca16(numColumns * sizeof( double )));
	double* v2 = static_cast<double*>(_alloca16(numColumns * sizeof( double )));

	double d = idMath::SQRT_1OVER2;
	v1[r] = ( 0.5f * addSub[r] + 1.0f ) * d;
	v2[r] = ( 0.5f * addSub[r] - 1.0f ) * d;
	for ( i = r+1; std::cmp_less(i, numColumns); i++ ) {
		v1[i] = v2[i] = addSub[i] * d;
	}

	double alpha1 = 1.0f;
	double alpha2 = -1.0f;

	// simultaneous update/downdate of the sub matrix starting at (r, r)
	for ( i = r; std::cmp_less(i, numColumns); i++ ) {

		double diag = (*this)[i][i];
		const double p1 = v1[i];
		double newDiag = diag + alpha1 * p1 * p1;

		if ( newDiag == 0.0f ) {
			return false;
		}

		alpha1 /= newDiag;
		const double beta1 = p1 * alpha1;
		alpha1 *= diag;

		diag = newDiag;
		const double p2 = v2[i];
		newDiag = diag + alpha2 * p2 * p2;

		if ( newDiag == 0.0f ) {
			return false;
		}

		alpha2 /= newDiag;
		const double beta2 = p2 * alpha2;
		alpha2 *= diag;

		(*this)[i][i] = idMath::Dtof(newDiag);

		for ( j = i+1; std::cmp_less(j, numRows); j++ ) {

			d = (*this)[j][i];

			v1[j] -= p1 * d;
			d += beta1 * v1[j];

			v2[j] -= p2 * d;
			d += beta2 * v2[j];

			(*this)[j][i] = idMath::Dtof(d);
		}
	}

#endif

	return true;
}

/*
============
idMatX::LDLT_UpdateIncrement

  Updates the in-place LDL' factorization to obtain the factors for the matrix:

  [ A  a ]
  [ a  b ]

  where: a = v[0,numRows-1], b = v[numRows]
============
*/
bool idMatX::LDLT_UpdateIncrement( const idVecX &v ) {
	size_t i = 0;
	double sum = 0.0, d = 0.0;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows+1 );

	ChangeSize( numRows+1, numColumns+1, false );

	double* x = static_cast<double*>(_alloca16(numRows * sizeof( double )));

	// solve for x in L * x = v
	for ( i = 0; i < numRows - 1; i++ ) {
		sum = v[i];
		for (size_t j = 0; std::cmp_less(j, i); j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = sum;
	}

	// calculate new row of L and calculate the diagonal entry
	sum = v[numRows - 1];
	for ( i = 0; i < numRows - 1; i++ ) {
		(*this)[numRows - 1][i] = idMath::Dtof(d = x[i] / (*this)[i][i]);
		sum -= d * x[i];
	}

	if ( sum == 0.0f ) {
		return false;
	}

	// store the diagonal entry
	(*this)[numRows - 1][numRows - 1] = idMath::Dtof(sum);

	return true;
}

/*
============
idMatX::LDLT_UpdateDecrement

  Updates the in-place LDL' factorization to obtain the factors for the matrix with row r and column r removed.
  v should store the row of the original matrix.
============
*/

bool idMatX::LDLT_UpdateDecrement(const idVecX &v, const Ordinal auto r) {
	idVecX v1;

	assert( numRows == numColumns );
	assert( v.GetSize() >= numRows );
	assert( r >= 0 && std::cmp_less(r, numRows ));

	v1.SetData( numRows, VECX_ALLOCA(numRows));

	// update the row and column to identity
	v1 = -v;
	v1[r] += 1.0f;

	// NOTE:	msvc compiler bug: the this pointer stored in edi is expected to stay
	//			untouched when calling LDLT_UpdateRowColumn in the if statement
#if 0
	if ( !LDLT_UpdateRowColumn( v1, r ) ) {
#else
	const bool ret = LDLT_UpdateRowColumn( v1, r );
	if ( !ret ) {
#endif
		return false;
	}

	// physically remove the row and column
	Update_Decrement( r );

	return true;
}

/*
============
idMatX::LDLT_Solve

  Solve Ax = b with A factored in-place as: LDL'
============
*/
void idMatX::LDLT_Solve( idVecX &x, const idVecX &b ) const {
	int64 i = 0, j = 0;
	double sum = 0.0;

	assert( numRows == numColumns );
	assert( x.GetSize() >= numRows && b.GetSize() >= numRows );

	// solve L
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		sum = b[i];
		for ( j = 0; j < i; j++ ) {
			sum -= (*this)[i][j] * x[j];
		}
		x[i] = idMath::Dtof(sum);
	}

	// solve D
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		x[i] /= (*this)[i][i];
	}

	// solve Lt
	for ( i = idMath::integer_cast<int64>(numRows) - 2; i >= 0; i-- ) {
		sum = x[i];
		for ( j = i + 1; std::cmp_less(j, numRows); j++ ) {
			sum -= (*this)[j][i] * x[j];
		}
		x[i] = idMath::Dtof(sum);
	}
}

/*
============
idMatX::LDLT_Inverse

  Calculates the inverse of the matrix which is factored in-place as: LDL'
============
*/
void idMatX::LDLT_Inverse( idMatX &inv ) const {
	idVecX x, b;

	assert( numRows == numColumns );

	x.SetData( numRows, VECX_ALLOCA(numRows));
	b.SetData( numRows, VECX_ALLOCA(numRows));
	b.Zero();
	inv.SetSize( numRows, numColumns );

	for (size_t i = 0; std::cmp_less(i, numRows); i++ ) {

		b[i] = 1.0f;
		LDLT_Solve( x, b );
		for ( size_t j = 0; std::cmp_less(j, numRows); j++ ) {
			inv[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
}

/*
============
idMatX::LDLT_UnpackFactors

  Unpacks the in-place LDL' factorization.
============
*/
void idMatX::LDLT_UnpackFactors( idMatX &L, idMatX &D ) const {
	L.Zero( numRows, numColumns );
	D.Zero( numRows, numColumns );
	for ( size_t i = 0; std::cmp_less(i, numRows); i++ ) {
		for ( size_t j = 0; j < i; j++ ) {
			L[i][j] = (*this)[i][j];
		}
		L[i][i] = 1.0f;
		D[i][i] = (*this)[i][i];
	}
}

/*
============
idMatX::LDLT_MultiplyFactors

  Multiplies the factors of the in-place LDL' factorization to form the original matrix.
============
*/
void idMatX::LDLT_MultiplyFactors( idMatX &m ) const {
	size_t i = 0;
	double sum = 0.0;

	float* v = static_cast<float*>(_alloca16(numRows * sizeof( float )));
	m.SetSize( numRows, numColumns );

	for ( size_t r = 0; std::cmp_less(r, numRows); r++ ) {

		// calculate row of matrix
		for ( i = 0; i < r; i++ ) {
			v[i] = (*this)[r][i] * (*this)[i][i];
		}
		for ( i = 0; std::cmp_less(i, numColumns); i++ ) {
			if ( i < r ) {
				sum = (*this)[i][i] * (*this)[r][i];
			} else if ( i == r ) {
				sum = (*this)[r][r];
			} else {
				sum = (*this)[r][r] * (*this)[i][r];
			}
			for ( size_t j = 0; j < i && j < r; j++ ) {
				sum += (*this)[i][j] * v[j];
			}
			m[r][i] = idMath::Dtof(sum);
		}
	}
}

/*
============
idMatX::TriDiagonal_ClearTriangles
============
*/
void idMatX::TriDiagonal_ClearTriangles() {
	assert( numRows == numColumns );
	for ( size_t i = 0; i < numRows-2; i++ ) {
		for ( size_t j = i + 2; std::cmp_less(j, numColumns); j++ ) {
			(*this)[i][j] = 0.0f;
			(*this)[j][i] = 0.0f;
		}
	}
}

/*
============
idMatX::TriDiagonal_Solve

  Solve Ax = b with A being tridiagonal.
============
*/
bool idMatX::TriDiagonal_Solve( idVecX &x, const idVecX &b ) const {
	int64 i = 0;
	idVecX tmp;

	assert( numRows == numColumns );
	assert( x.GetSize() >= numRows && b.GetSize() >= numRows );

	tmp.SetData( numRows, VECX_ALLOCA(numRows));

	float d = (*this)[0][0];
	if ( d == 0.0f ) {
		return false;
	}
	d = 1.0f / d;
	x[0] = b[0] * d;
	for ( i = 1; std::cmp_less(i, numRows); i++ ) {
		tmp[i] = (*this)[i-1][i] * d;
		d = (*this)[i][i] - (*this)[i][i-1] * tmp[i];
		if ( d == 0.0f ) {
			return false;
		}
		d = 1.0f / d;
		x[i] = ( b[i] - (*this)[i][i-1] * x[i-1] ) * d;
	}
	for ( i = idMath::integer_cast<int64>(numRows) - 2; i >= 0; i-- ) {
		x[i] -= tmp[i+1] * x[i+1];
	}
	return true;
}

/*
============
idMatX::TriDiagonal_Inverse

  Calculates the inverse of a tri-diagonal matrix.
============
*/
void idMatX::TriDiagonal_Inverse( idMatX &inv ) const {
	idVecX x, b;

	assert( numRows == numColumns );

	x.SetData( numRows, VECX_ALLOCA(numRows));
	b.SetData( numRows, VECX_ALLOCA(numRows));
	b.Zero();
	inv.SetSize( numRows, numColumns );

	for ( size_t i = 0; std::cmp_less(i, numRows); i++ ) {

		b[i] = 1.0f;
		TriDiagonal_Solve( x, b );
		for ( size_t j = 0; std::cmp_less(j, numRows); j++ ) {
			inv[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
}

/*
============
idMatX::HouseholderReduction

  Householder reduction to symmetric tri-diagonal form.
  The original matrix is replaced by an orthogonal matrix effecting the accumulated householder transformations.
  The diagonal elements of the diagonal matrix are stored in diag.
  The off-diagonal elements of the diagonal matrix are stored in subd.
  The initial matrix has to be symmetric.
============
*/
void idMatX::HouseholderReduction( idVecX &diag, idVecX &subd ) {
	int64 i0 = 0, i1 = 0, i2 = 0, i3 = 0;

	assert( numRows == numColumns );

	diag.SetSize( numRows );
	subd.SetSize( numRows );

	for ( i0 = idMath::integer_cast<int64>(numRows)-1, i3 = idMath::integer_cast<int64>(numRows)-2; i0 >= 1; i0--, i3-- ) {
		float h = 0.0f;

		if ( i3 > 0 ) {
			float scale = 0.0f;
			for ( i2 = 0; i2 <= i3; i2++ ) {
				scale += idMath::Fabs( (*this)[i0][i2] );
			}
			if ( std::equal_to<>()(scale, 0.0f) ) {
				subd[i0] = (*this)[i0][i3];
			} else {
				const float invScale = 1.0f / scale;
				for (i2 = 0; i2 <= i3; i2++)
				{
					(*this)[i0][i2] *= invScale;
					h += (*this)[i0][i2] * (*this)[i0][i2];
				}
				float f = (*this)[i0][i3];
				float g = idMath::Sqrt(h);
				if ( f > 0.0f ) {
					g = -g;
				}
				subd[i0] = scale * g;
				h -= f * g;
				(*this)[i0][i3] = f - g;
				f = 0.0f;
				const float invH = 1.0f / h;
				for (i1 = 0; i1 <= i3; i1++) {
					(*this)[i1][i0] = (*this)[i0][i1] * invH;
					g = 0.0f;
					for (i2 = 0; i2 <= i1; i2++) {
						g += (*this)[i1][i2] * (*this)[i0][i2];
					}
					for (i2 = i1+1; i2 <= i3; i2++) {
						g += (*this)[i2][i1] * (*this)[i0][i2];
					}
					subd[i1] = g * invH;
					f += subd[i1] * (*this)[i0][i1];
				}
				const float halfFdivH = 0.5f * f * invH;
				for ( i1 = 0; i1 <= i3; i1++ ) {
					f = (*this)[i0][i1];
					g = subd[i1] - halfFdivH * f;
					subd[i1] = g;
					for ( i2 = 0; i2 <= i1; i2++ ) {
						(*this)[i1][i2] -= f * subd[i2] + g * (*this)[i0][i2];
					}
				}
            }
		} else {
			subd[i0] = (*this)[i0][i3];
		}

		diag[i0] = h;
	}

	diag[0] = 0.0f;
	subd[0] = 0.0f;
	for ( i0 = 0, i3 = -1; i0 <= idMath::integer_cast<int64>(numRows)-1; i0++, i3++ ) {
		if ( std::not_equal_to<>()(diag[i0], 0.0) ) {
			for ( i1 = 0; i1 <= i3; i1++ ) {
				float sum = 0.0f;
				for (i2 = 0; i2 <= i3; i2++) {
					sum += (*this)[i0][i2] * (*this)[i2][i1];
				}
				for ( i2 = 0; i2 <= i3; i2++ ) {
					(*this)[i2][i1] -= sum * (*this)[i2][i0];
				}
			}
		}
		diag[i0] = (*this)[i0][i0];
		(*this)[i0][i0] = 1.0f;
		for ( i1 = 0; i1 <= i3; i1++ ) {
			(*this)[i1][i0] = 0.0f;
			(*this)[i0][i1] = 0.0f;
		}
	}

	// re-order
	for ( i0 = 1, i3 = 0; std::cmp_less(i0, numRows); i0++, i3++ ) {
		subd[i3] = subd[i0];
	}
	subd[numRows-1] = 0.0f;
}

/*
============
idMatX::QL

  QL algorithm with implicit shifts to determine the eigenvalues and eigenvectors of a symmetric tri-diagonal matrix.
  diag contains the diagonal elements of the symmetric tri-diagonal matrix on input and is overwritten with the eigenvalues.
  subd contains the off-diagonal elements of the symmetric tri-diagonal matrix and is destroyed.
  This matrix has to be either the identity matrix to determine the eigenvectors for a symmetric tri-diagonal matrix,
  or the matrix returned by the Householder reduction to determine the eigenvalues for the original symmetric matrix.
============
*/
bool idMatX::QL( idVecX &diag, idVecX &subd ) {
	constexpr size_t maxIter = 32;
	size_t i1 = 0, i2 = 0;

	assert( numRows == numColumns );

	for (size_t i0 = 0; std::cmp_less(i0, numRows); i0++ ) {
		for ( i1 = 0; i1 < maxIter; i1++ ) {
			for ( i2 = i0; i2 <= numRows - 2; i2++ ) {
				const float a = idMath::Fabs(diag[i2]) + idMath::Fabs(diag[i2 + 1]);
				if ( std::equal_to<>()(idMath::Fabs( subd[i2] ) + a,a) ) {
					break;
				}
			}
			if ( i2 == i0 ) {
				break;
			}

			float g = (diag[i0 + 1] - diag[i0]) / (2.0f * subd[i0]);
			float r = idMath::Sqrt(g * g + 1.0f);
			if ( g < 0.0f ) {
				g = diag[i2] - diag[i0] + subd[i0] / ( g - r );
			} else {
				g = diag[i2] - diag[i0] + subd[i0] / ( g + r );
			}
			float s = 1.0f;
			float c = 1.0f;
			float p = 0.0f;
			for (size_t i3 = i2 - 1; i3 >= i0; i3-- ) {
				float f = s * subd[i3];
				const float b = c * subd[i3];
				if ( idMath::Fabs( f ) >= idMath::Fabs( g ) ) {
					c = g / f;
					r = idMath::Sqrt( c * c + 1.0f );
					subd[i3+1] = f * r;
					s = 1.0f / r;
					c *= s;
				} else {
					s = f / g;
					r = idMath::Sqrt( s * s + 1.0f );
					subd[i3+1] = g * r;
					c = 1.0f / r;
					s *= c;
				}
				g = diag[i3+1] - p;
				r = ( diag[i3] - g ) * s + 2.0f * b * c;
				p = s * r;
				diag[i3+1] = g + p;
				g = c * r - b;

				for (size_t i4 = 0; std::cmp_less(i4, numRows); i4++ ) {
					f = (*this)[i4][i3+1];
					(*this)[i4][i3+1] = s * (*this)[i4][i3] + c * f;
					(*this)[i4][i3] = c * (*this)[i4][i3] - s * f;
				}
			}
			diag[i0] -= p;
			subd[i0] = g;
			subd[i2] = 0.0f;
		}
		if ( i1 == maxIter ) {
			return false;
		}
	}
	return true;
}

/*
============
idMatX::Eigen_SolveSymmetricTriDiagonal

  Determine eigen values and eigen vectors for a symmetric tri-diagonal matrix.
  The eigen values are stored in 'eigenValues'.
  Column i of the original matrix will store the eigen vector corresponding to the eigenValues[i].
  The initial matrix has to be symmetric tri-diagonal.
============
*/
bool idMatX::Eigen_SolveSymmetricTriDiagonal( idVecX &eigenValues ) {
	idVecX subd;

	assert( numRows == numColumns );

	subd.SetData( numRows, VECX_ALLOCA(numRows));
	eigenValues.SetSize( numRows );

	for (size_t i = 0; i < numRows-1; i++ ) {
		eigenValues[i] = (*this)[i][i];
		subd[i] = (*this)[i+1][i];
	}
	eigenValues[numRows-1] = (*this)[numRows-1][numRows-1];

	Identity();

	return QL( eigenValues, subd );
}

/*
============
idMatX::Eigen_SolveSymmetric

  Determine eigen values and eigen vectors for a symmetric matrix.
  The eigen values are stored in 'eigenValues'.
  Column i of the original matrix will store the eigen vector corresponding to the eigenValues[i].
  The initial matrix has to be symmetric.
============
*/
bool idMatX::Eigen_SolveSymmetric( idVecX &eigenValues ) {
	idVecX subd;

	assert( numRows == numColumns );

	subd.SetData( numRows, VECX_ALLOCA(numRows));
	eigenValues.SetSize( numRows );

	HouseholderReduction( eigenValues, subd );
	return QL( eigenValues, subd );
}

/*
============
idMatX::HessenbergReduction

  Reduction to Hessenberg form.
============
*/
void idMatX::HessenbergReduction( idMatX &H ) {
	size_t i = 0, j = 0;
	constexpr size_t low = 0;
	const size_t high = numRows - 1;
	float f = 0.0f, g = 0.0f;
	idVecX v;

	v.SetData( numRows, VECX_ALLOCA(numRows));

	for ( size_t m = low + 1; m <= high - 1; m++ ) 
	{
		float scale = 0.0f;
		for ( i = m; i <= high; i++ ) {
			scale = scale + idMath::Fabs( H[i][m-1] );
		}
		if ( std::not_equal_to<>()(scale, 0.0f) ) {

			// compute Householder transformation.
			float h = 0.0f;
			for ( i = high; i >= m; i-- ) {
				v[i] = H[i][m-1] / scale;
				h += v[i] * v[i];
			}
			g = idMath::Sqrt( h );
			if ( v[m] > 0.0f ) {
				g = -g;
			}
			h = h - v[m] * g;
			v[m] = v[m] - g;

			// apply Householder similarity transformation
			// H = (I-u*u'/h)*H*(I-u*u')/h)
			for ( j = m; std::cmp_less(j, numRows); j++) {
				f = 0.0f;
				for ( i = high; i >= m; i-- ) {
					f += v[i] * H[i][j];
				}
				f = f / h;
				for ( i = m; i <= high; i++ ) {
					H[i][j] -= f * v[i];
				}
			}

			for ( i = 0; i <= high; i++ ) {
				f = 0.0f;
				for ( j = high; j >= m; j-- ) {
					f += v[j] * H[i][j];
				}
				f = f / h;
				for ( j = m; j <= high; j++ ) {
					H[i][j] -= f * v[j];
				}
			}
			v[m] = scale * v[m];
			H[m][m-1] = scale * g;
		}
	}

	// accumulate transformations
	Identity();
	for ( int64 m = idMath::integer_cast<int64>(high) - 1; std::cmp_greater_equal(m, low) + 1; m-- ) {
		if ( H[m][m-1] != 0.0f ) {
			for ( i = m + 1; i <= high; i++ ) {
				v[i] = H[i][m-1];
			}
			for ( j = m; j <= high; j++ ) {
				g = 0.0f;
				for ( i = m; i <= high; i++ ) {
					g += v[i] * (*this)[i][j];
				}
				// float division to avoid possible underflow
				g = ( g / v[m] ) / H[m][m-1];
				for ( i = m; i <= high; i++ ) {
					(*this)[i][j] += g * v[i];
				}
			}
		}
	}
}

/*
============
idMatX::ComplexDivision

  Complex scalar division.
============
*/
void idMatX::ComplexDivision(const float xr, const float xi, const float yr, const float yi, float &cdivr, float &cdivi ) {
	float r = 0.0f, d = 0.0f;
	if ( idMath::Fabs( yr ) > idMath::Fabs( yi ) ) {
		r = yi / yr;
		d = yr + r * yi;
		cdivr = ( xr + r * xi ) / d;
		cdivi = ( xi - r * xr ) / d;
	} else {
		r = yr / yi;
		d = yi + r * yr;
		cdivr = ( r * xr + xi ) / d;
		cdivi = ( r * xi - xr ) / d;
	}
}

/*
============
idMatX::HessenbergToRealSchur

  Reduction from Hessenberg to real Schur form.
============
*/
bool idMatX::HessenbergToRealSchur( idMatX &H, idVecX &realEigenValues, idVecX &imaginaryEigenValues ) {
	size_t i = 0;
	int64 j = 0;
	size_t k = 0;
	int64 n = idMath::integer_cast<int64>(numRows) - 1;
	int64 low = 0;
	int64 high = idMath::integer_cast<int64>(numRows) - 1;
	float eps = 2e-16f, exshift = 0.0f;
	float p = 0.0f, q = 0.0f, r = 0.0f, s = 0.0f, z = 0.0f, t, w, x, y;

	// store roots isolated by balanc and compute matrix norm
	float norm = 0.0f;
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		if (std::cmp_less(i, low) || std::cmp_greater(i, high)) {
			realEigenValues[i] = H[i][i];
			imaginaryEigenValues[i] = 0.0f;
		}
		for ( j = Max( i - 1, 0ULL ); std::cmp_less(j, numRows); j++ ) {
			norm = norm + idMath::Fabs( H[i][j] );
		}
	}

	size_t iter = 0;
	while(std::cmp_greater_equal(n, low)) {

		// look for single small sub-diagonal element
		size_t l = n;
		while (std::cmp_greater(l, low)) {
			s = idMath::Fabs( H[l-1][l-1] ) + idMath::Fabs( H[l][l] );
			if ( s == 0.0f ) {
				s = norm;
			}
			if ( idMath::Fabs( H[l][l-1] ) < eps * s ) {
				break;
			}
			l--;
		}
	   
		// check for convergence
		if (std::cmp_equal(l, n)) {			// one root found
			H[n][n] = H[n][n] + exshift;
			realEigenValues[n] = H[n][n];
			imaginaryEigenValues[n] = 0.0f;
			n--;
			iter = 0;
		} else if (std::cmp_equal(l, n-1)) {	// two roots found
			w = H[n][n-1] * H[n-1][n];
			p = ( H[n-1][n-1] - H[n][n] ) / 2.0f;
			q = p * p + w;
			z = idMath::Sqrt( idMath::Fabs( q ) );
			H[n][n] = H[n][n] + exshift;
			H[n-1][n-1] = H[n-1][n-1] + exshift;
			x = H[n][n];

			if ( q >= 0.0f ) {		// real pair
				if ( p >= 0.0f ) {
					z = p + z;
				} else {
					z = p - z;
				}
				realEigenValues[n-1] = x + z;
				realEigenValues[n] = realEigenValues[n-1];
				if ( z != 0.0f ) {
					realEigenValues[n] = x - w / z;
				}
				imaginaryEigenValues[n-1] = 0.0f;
				imaginaryEigenValues[n] = 0.0f;
				x = H[n][n-1];
				s = idMath::Fabs( x ) + idMath::Fabs( z );
				p = x / s;
				q = z / s;
				r = idMath::Sqrt( p * p + q * q );
				p = p / r;
				q = q / r;

				// modify row
				for ( j = n-1; std::cmp_less(j, numRows); j++ ) {
					z = H[n-1][j];
					H[n-1][j] = q * z + p * H[n][j];
					H[n][j] = q * H[n][j] - p * z;
				}

				// modify column
				for ( i = 0; std::cmp_less_equal(i, n); i++ ) {
					z = H[i][n-1];
					H[i][n-1] = q * z + p * H[i][n];
					H[i][n] = q * H[i][n] - p * z;
				}

				// accumulate transformations
				for ( i = low; std::cmp_less_equal(i, high); i++ ) {
					z = (*this)[i][n-1];
					(*this)[i][n-1] = q * z + p * (*this)[i][n];
					(*this)[i][n] = q * (*this)[i][n] - p * z;
				}
			} else {		// complex pair
				realEigenValues[n-1] = x + p;
				realEigenValues[n] = x + p;
				imaginaryEigenValues[n-1] = z;
				imaginaryEigenValues[n] = -z;
			}
			n = n - 2;
			iter = 0;

		} else {	// no convergence yet

			// form shift
			x = H[n][n];
			y = 0.0f;
			w = 0.0f;
			if (std::cmp_less(l, n)) {
				y = H[n-1][n-1];
				w = H[n][n-1] * H[n-1][n];
			}

			// Wilkinson's original ad hoc shift
			if ( iter == 10 ) {
				exshift += x;
				for ( i = low; std::cmp_less_equal(i, n); i++ ) {
					H[i][i] -= x;
				}
				s = idMath::Fabs( H[n][n-1] ) + idMath::Fabs( H[n-1][n-2] );
				x = y = 0.75f * s;
				w = -0.4375f * s * s;
			}

			// new ad hoc shift
			if ( iter == 30 ) {
				s = ( y - x ) / 2.0f;
				s = s * s + w;
				if ( s > 0 ) {
					s = idMath::Sqrt( s );
					if ( y < x ) {
						s = -s;
					}
					s = x - w / ( ( y - x ) / 2.0f + s );
					for ( i = low; std::cmp_less_equal(i, n); i++ ) {
						H[i][i] -= s;
					}
					exshift += s;
					x = y = w = 0.964f;
				}
			}

			iter = iter + 1;

			// look for two consecutive small sub-diagonal elements
			size_t m = 0;
			for( m = n-2; m >= l; m-- ) {
				z = H[m][m];
				r = x - z;
				s = y - z;
				p = ( r * s - w ) / H[m+1][m] + H[m][m+1];
				q = H[m+1][m+1] - z - r - s;
				r = H[m+2][m+1];
				s = idMath::Fabs( p ) + idMath::Fabs( q ) + idMath::Fabs( r );
				p = p / s;
				q = q / s;
				r = r / s;
				if ( m == l ) {
					break;
				}
				if ( idMath::Fabs( H[m][m-1] ) * ( idMath::Fabs( q ) + idMath::Fabs( r ) ) <
						eps * ( idMath::Fabs( p ) * ( idMath::Fabs( H[m-1][m-1] ) + idMath::Fabs( z ) + idMath::Fabs( H[m+1][m+1] ) ) ) ) {
					break;
				}
			}

			for ( i = m+2; std::cmp_less_equal(i, n); i++ ) {
				H[i][i-2] = 0.0f;
				if ( i > m+2 ) {
					H[i][i-3] = 0.0f;
				}
			}

			// double QR step involving rows l:n and columns m:n
			for ( k = m; std::cmp_less_equal(k, n-1); k++ ) {
				bool notlast = (std::cmp_not_equal(k, n-1));
				if ( k != m ) {
					p = H[k][k-1];
					q = H[k+1][k-1];
					r = ( notlast ? H[k+2][k-1] : 0.0f );
					x = idMath::Fabs( p ) + idMath::Fabs( q ) + idMath::Fabs( r );
					if ( x != 0.0f ) {
						p = p / x;
						q = q / x;
						r = r / x;
					}
				}
				if ( x == 0.0f ) {
					break;
				}
				s = idMath::Sqrt( p * p + q * q + r * r );
				if ( p < 0.0f ) {
					s = -s;
				}
				if ( s != 0.0f ) {
					if ( k != m ) {
						H[k][k-1] = -s * x;
					} else if ( l != m ) {
						H[k][k-1] = -H[k][k-1];
					}
					p = p + s;
					x = p / s;
					y = q / s;
					z = r / s;
					q = q / p;
					r = r / p;

					// modify row
					for ( j = k; std::cmp_less(j, numRows); j++ ) {
						p = H[k][j] + q * H[k+1][j];
						if ( notlast ) {
							p = p + r * H[k+2][j];
							H[k+2][j] = H[k+2][j] - p * z;
						}
						H[k][j] = H[k][j] - p * x;
						H[k+1][j] = H[k+1][j] - p * y;
					}

					// modify column
					for ( i = 0; std::cmp_less_equal(i, std::min<int64>( n, k + 3 )); i++ ) {
						p = x * H[i][k] + y * H[i][k+1];
						if ( notlast ) {
							p = p + z * H[i][k+2];
							H[i][k+2] = H[i][k+2] - p * r;
						}
						H[i][k] = H[i][k] - p;
						H[i][k+1] = H[i][k+1] - p * q;
					}

					// accumulate transformations
					for ( i = low; std::cmp_less_equal(i, high); i++ ) {
						p = x * (*this)[i][k] + y * (*this)[i][k+1];
						if ( notlast ) {
							p = p + z * (*this)[i][k+2];
							(*this)[i][k+2] = (*this)[i][k+2] - p * r;
						}
						(*this)[i][k] = (*this)[i][k] - p;
						(*this)[i][k+1] = (*this)[i][k+1] - p * q;
					}
				}
			}
		}
	}
	
	// backsubstitute to find vectors of upper triangular form
	if ( std::not_equal_to<>()(norm, 0.0f) ) {
		return false;
	}

	for ( n = idMath::integer_cast<int64>(numRows) - 1; n >= 0; n-- ) {
		p = realEigenValues[n];
		q = imaginaryEigenValues[n];

		if ( q == 0.0f ) {		// real vector
			size_t l = n;
			H[n][n] = 1.0f;
			for ( i = n-1; std::cmp_greater_equal(i, 0); i-- ) {
				w = H[i][i] - p;
				r = 0.0f;
				for ( j = l; std::cmp_less_equal(j, n); j++ ) {
					r = r + H[i][j] * H[j][n];
				}
				if ( imaginaryEigenValues[i] < 0.0f ) {
					z = w;
					s = r;
				} else {
					l = i;
					if ( imaginaryEigenValues[i] == 0.0f ) {
						if ( w != 0.0f ) {
							H[i][n] = -r / w;
						} else {
							H[i][n] = -r / ( eps * norm );
						}
					} else {		// solve real equations
						x = H[i][i+1];
						y = H[i+1][i];
						q = ( realEigenValues[i] - p ) * ( realEigenValues[i] - p ) + imaginaryEigenValues[i] * imaginaryEigenValues[i];
						t = ( x * s - z * r ) / q;
						H[i][n] = t;
						if ( idMath::Fabs(x) > idMath::Fabs( z ) ) {
							H[i+1][n] = ( -r - w * t ) / x;
						} else {
							H[i+1][n] = ( -s - y * t ) / z;
						}
					}

					// overflow control
					t = idMath::Fabs(H[i][n]);
					if ( ( eps * t ) * t > 1 ) {
						for ( j = i; std::cmp_less_equal(j, n); j++ ) {
							H[j][n] = H[j][n] / t;
						}
					}
				}
			}
		} else if ( q < 0.0f ) {	// complex vector
			size_t l = n-1;

			// last vector component imaginary so matrix is triangular
			if ( idMath::Fabs( H[n][n-1] ) > idMath::Fabs( H[n-1][n] ) ) {
				H[n-1][n-1] = q / H[n][n-1];
				H[n-1][n] = -( H[n][n] - p ) / H[n][n-1];
			} else {
				ComplexDivision( 0.0f, -H[n-1][n], H[n-1][n-1]-p, q, H[n-1][n-1], H[n-1][n] );
			}
			H[n][n-1] = 0.0f;
			H[n][n] = 1.0f;
			for ( i = n-2; i >= 0; i-- ) {
				float ra = 0.0f, sa = 0.0f;

				for ( j = l; std::cmp_less_equal(j, n); j++ ) {
					ra = ra + H[i][j] * H[j][n-1];
					sa = sa + H[i][j] * H[j][n];
				}
				w = H[i][i] - p;

				if ( imaginaryEigenValues[i] < 0.0f ) {
					z = w;
					r = ra;
					s = sa;
				} else {
					l = i;
					if ( std::equal_to<>()(imaginaryEigenValues[i], 0.0f) ) {
						ComplexDivision( -ra, -sa, w, q, H[i][n-1], H[i][n] );
					} else {
						float vi = 0.0f;
						float vr = 0.0f;
						// solve complex equations
						x = H[i][i+1];
						y = H[i+1][i];
						vr = ( realEigenValues[i] - p ) * ( realEigenValues[i] - p ) + imaginaryEigenValues[i] * imaginaryEigenValues[i] - q * q;
						vi = ( realEigenValues[i] - p ) * 2.0f * q;
						if ( vr == 0.0f && vi == 0.0f ) {
							vr = eps * norm * ( idMath::Fabs( w ) + idMath::Fabs( q ) + idMath::Fabs( x ) + idMath::Fabs( y ) + idMath::Fabs( z ) );
						}
						ComplexDivision( x * r - z * ra + q * sa, x * s - z * sa - q * ra, vr, vi, H[i][n-1], H[i][n] );
						if ( idMath::Fabs( x ) > ( idMath::Fabs( z ) + idMath::Fabs( q ) ) ) {
							H[i+1][n-1] = ( -ra - w * H[i][n-1] + q * H[i][n] ) / x;
							H[i+1][n] = ( -sa - w * H[i][n] - q * H[i][n-1] ) / x;
						} else {
							ComplexDivision( -r - y * H[i][n-1], -s - y * H[i][n], z, q, H[i+1][n-1], H[i+1][n] );
						}
					}

					// overflow control
					t = Max( idMath::Fabs( H[i][n-1] ), idMath::Fabs( H[i][n] ) );
					if ( ( eps * t ) * t > 1 ) {
						for ( j = i; std::cmp_less_equal(j, n); j++ ) {
							H[j][n-1] = H[j][n-1] / t;
							H[j][n] = H[j][n] / t;
						}
					}
				}
			}
		}
	}

	// vectors of isolated roots
	for ( i = 0; std::cmp_less(i, numRows); i++ ) {
		if (std::cmp_less(i, low) || std::cmp_greater(i, high)) {
			for ( j = idMath::integer_cast<int64>(i); std::cmp_less(j, numRows); j++ ) {
				(*this)[i][j] = H[i][j];
			}
		}
	}

	// back transformation to get eigenvectors of original matrix
	for ( j = idMath::integer_cast<int64>(numRows) - 1; std::cmp_greater_equal(j, low); j-- ) {
		for ( i = low; std::cmp_less_equal(i, high); i++ ) {
			z = 0.0f;
			for ( k = low; std::cmp_less_equal(k, std::min<int64>( j, high )); k++ ) {
				z = z + (*this)[i][k] * H[k][j];
			}
			(*this)[i][j] = z;
		}
	}

	return true;
}

/*
============
idMatX::Eigen_Solve

  Determine eigen values and eigen vectors for a square matrix.
  The eigen values are stored in 'realEigenValues' and 'imaginaryEigenValues'.
  Column i of the original matrix will store the eigen vector corresponding to the realEigenValues[i] and imaginaryEigenValues[i].
============
*/
bool idMatX::Eigen_Solve( idVecX &realEigenValues, idVecX &imaginaryEigenValues ) {
	assert( numRows == numColumns );

	realEigenValues.SetSize( numRows );
	imaginaryEigenValues.SetSize( numRows );

	idMatX H = *this;

    // reduce to Hessenberg form
    HessenbergReduction( H );

    // reduce Hessenberg to real Schur form
    return HessenbergToRealSchur( H, realEigenValues, imaginaryEigenValues );
}

/*
============
idMatX::Eigen_SortIncreasing
============
*/
void idMatX::Eigen_SortIncreasing( idVecX &eigenValues ) {
	for ( size_t i = 0, j = 0; i <= numRows - 2; i++ ) {
		j = i;
		float min = eigenValues[j];
		for ( size_t k = i + 1; std::cmp_less(k, numRows); k++ ) {
			if ( eigenValues[k] < min ) {
				j = k;
				min = eigenValues[j];
			}
		}
		if ( j != i ) {
			eigenValues.SwapElements( i, j );
			SwapColumns( i, j );
		}
	}
}

/*
============
idMatX::Eigen_SortDecreasing
============
*/
void idMatX::Eigen_SortDecreasing( idVecX &eigenValues ) {
	for ( size_t i = 0, j = 0; i <= numRows - 2; i++ ) {
		j = i;
		float max = eigenValues[j];
		for ( size_t k = i + 1; std::cmp_less(k, numRows); k++ ) {
			if ( eigenValues[k] > max ) {
				j = k;
				max = eigenValues[j];
			}
		}
		if ( j != i ) {
			eigenValues.SwapElements( i, j );
			SwapColumns( i, j );
		}
	}
}

/*
============
idMatX::DeterminantGeneric
============
*/
float idMatX::DeterminantGeneric() const {
	float det = 0.0f;
	idMatX tmp;

	size_t* index = static_cast<size_t*>(_alloca16(numRows * sizeof(size_t)));
	tmp.SetData( numRows, numColumns, MATX_ALLOCA(numRows * numColumns));
	tmp = *this;

	if ( !tmp.LU_Factor( index, &det ) ) {
		return 0.0f;
	}

	return det;
}

/*
============
idMatX::InverseSelfGeneric
============
*/
bool idMatX::InverseSelfGeneric() {
	idMatX tmp;
	idVecX x, b;

	size_t* index = static_cast<size_t*>(_alloca16(numRows * sizeof(size_t)));
	tmp.SetData( numRows, numColumns, MATX_ALLOCA(numRows * numColumns));
	tmp = *this;

	if ( !tmp.LU_Factor( index ) ) {
		return false;
	}

	x.SetData( numRows, VECX_ALLOCA(numRows));
	b.SetData( numRows, VECX_ALLOCA(numRows));
	b.Zero();

	for ( size_t i = 0; std::cmp_less(i, numRows); i++ ) {

		b[i] = 1.0f;
		tmp.LU_Solve( x, b, index );
		for ( size_t j = 0; std::cmp_less(j, numRows); j++ ) {
			(*this)[j][i] = x[j];
		}
		b[i] = 0.0f;
	}
	return true;
}

/*
============
idMatX::Test
============
*/
void idMatX::Test() {
	idMatX original, m1, m2, m3, q1, q2, r1, r2;
	idVecX v, w, u, c, d;
	size_t offset = 0, size = 0, *index1 = nullptr, *index2 = nullptr;

	size = 6;
	original.Random( size, size, 0 );
	original = original * original.Transpose();

	index1 = static_cast<size_t*>(_alloca16(( size + 1 ) * sizeof( index1[0] )));
	index2 = static_cast<size_t*>(_alloca16(( size + 1 ) * sizeof( index2[0] )));

	/*
		idMatX::LowerTriangularInverse
	*/

	m1 = original;
	m1.ClearUpperTriangle();
	m2 = m1;

	m2.InverseSelf();
	m1.LowerTriangularInverse();

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::LowerTriangularInverse failed" );
	}

	/*
		idMatX::UpperTriangularInverse
	*/

	m1 = original;
	m1.ClearLowerTriangle();
	m2 = m1;

	m2.InverseSelf();
	m1.UpperTriangularInverse();

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::UpperTriangularInverse failed" );
	}

	/*
		idMatX::Inverse_GaussJordan
	*/

	m1 = original;

	m1.Inverse_GaussJordan();
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		idLib::common->Warning( "idMatX::Inverse_GaussJordan failed" );
	}

	/*
		idMatX::Inverse_UpdateRankOne
	*/

	m1 = original;
	m2 = original;

	w.Random( size, 1 );
	v.Random( size, 2 );

	// invert m1
	m1.Inverse_GaussJordan();

	// modify and invert m2 
	m2.Update_RankOne( v, w, 1.0f );
	if ( !m2.Inverse_GaussJordan() ) {
		assert( 0 );
	}

	// update inverse of m1
	m1.Inverse_UpdateRankOne( v, w, 1.0f );

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::Inverse_UpdateRankOne failed" );
	}

	/*
		idMatX::Inverse_UpdateRowColumn
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		v.Random( size, 1 );
		w.Random( size, 2 );
		w[offset] = 0.0f;

		// invert m1
		m1.Inverse_GaussJordan();

		// modify and invert m2
		m2.Update_RowColumn( v, w, offset );
		if ( !m2.Inverse_GaussJordan() ) {
			assert( 0 );
		}

		// update inverse of m1
		m1.Inverse_UpdateRowColumn( v, w, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			idLib::common->Warning( "idMatX::Inverse_UpdateRowColumn failed" );
		}
	}

	/*
		idMatX::Inverse_UpdateIncrement
	*/

	m1 = original;
	m2 = original;

	v.Random( size + 1, 1 );
	w.Random( size + 1, 2 );
	w[size] = 0.0f;

	// invert m1
	m1.Inverse_GaussJordan();

	// modify and invert m2 
	m2.Update_Increment( v, w );
	if ( !m2.Inverse_GaussJordan() ) {
		assert( 0 );
	}

	// update inverse of m1
	m1.Inverse_UpdateIncrement( v, w );

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::Inverse_UpdateIncrement failed" );
	}

	/*
		idMatX::Inverse_UpdateDecrement
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		v.SetSize( 6 );
		w.SetSize( 6 );
		for (size_t i = 0; i < size; i++ ) {
			v[i] = original[i][offset];
			w[i] = original[offset][i];
		}

		// invert m1
		m1.Inverse_GaussJordan();

		// modify and invert m2
		m2.Update_Decrement( offset );
		if ( !m2.Inverse_GaussJordan() ) {
			assert( 0 );
		}

		// update inverse of m1
		m1.Inverse_UpdateDecrement( v, w, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			idLib::common->Warning( "idMatX::Inverse_UpdateDecrement failed" );
		}
	}

	/*
		idMatX::LU_Factor
	*/

	m1 = original;

	m1.LU_Factor(nullptr);	// no pivoting
	m1.LU_UnpackFactors( m2, m3 );
	m1 = m2 * m3;

	if ( !original.Compare( m1, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::LU_Factor failed" );
	}

	/*
		idMatX::LU_UpdateRankOne
	*/

	m1 = original;
	m2 = original;

	w.Random( size, 1 );
	v.Random( size, 2 );

	// factor m1
	m1.LU_Factor( index1 );

	// modify and factor m2 
	m2.Update_RankOne( v, w, 1.0f );
	if ( !m2.LU_Factor( index2 ) ) {
		assert( 0 );
	}
	m2.LU_MultiplyFactors( m3, index2 );
	m2 = m3;

	// update factored m1
	m1.LU_UpdateRankOne( v, w, 1.0f, index1 );
	m1.LU_MultiplyFactors( m3, index1 );
	m1 = m3;

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::LU_UpdateRankOne failed" );
	}

	/*
		idMatX::LU_UpdateRowColumn
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		v.Random( size, 1 );
		w.Random( size, 2 );
		w[offset] = 0.0f;

		// factor m1
		m1.LU_Factor( index1 );

		// modify and factor m2
		m2.Update_RowColumn( v, w, offset );
		if ( !m2.LU_Factor( index2 ) ) {
			assert( 0 );
		}
		m2.LU_MultiplyFactors( m3, index2 );
		m2 = m3;

		// update m1
		m1.LU_UpdateRowColumn( v, w, offset, index1  );
		m1.LU_MultiplyFactors( m3, index1 );
		m1 = m3;

		if ( !m1.Compare( m2, 1e-3f ) ) {
			idLib::common->Warning( "idMatX::LU_UpdateRowColumn failed" );
		}
	}

	/*
		idMatX::LU_UpdateIncrement
	*/

	m1 = original;
	m2 = original;

	v.Random( size + 1, 1 );
	w.Random( size + 1, 2 );
	w[size] = 0.0f;

	// factor m1
	m1.LU_Factor( index1 );

	// modify and factor m2 
	m2.Update_Increment( v, w );
	if ( !m2.LU_Factor( index2 ) ) {
		assert( 0 );
	}
	m2.LU_MultiplyFactors( m3, index2 );
	m2 = m3;

	// update factored m1
	m1.LU_UpdateIncrement( v, w, index1 );
	m1.LU_MultiplyFactors( m3, index1 );
	m1 = m3;

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::LU_UpdateIncrement failed" );
	}

	/*
		idMatX::LU_UpdateDecrement
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		v.SetSize( 6 );
		w.SetSize( 6 );
		for (size_t i = 0; std::cmp_less(i, size); i++ ) {
			v[i] = original[i][offset];
			w[i] = original[offset][i];
		}

		// factor m1
		m1.LU_Factor( index1 );

		// modify and factor m2
		m2.Update_Decrement( offset );
		if ( !m2.LU_Factor( index2 ) ) {
			assert( 0 );
		}
		m2.LU_MultiplyFactors( m3, index2 );
		m2 = m3;

		u.SetSize( 6 );
		for (size_t i = 0; std::cmp_less(i, size); i++ ) {
			u[i] = original[index1[offset]][i];
		}

		// update factors of m1
		m1.LU_UpdateDecrement( v, w, u, offset, index1 );
		m1.LU_MultiplyFactors( m3, index1 );
		m1 = m3;

		if ( !m1.Compare( m2, 1e-3f ) ) {
			idLib::common->Warning( "idMatX::LU_UpdateDecrement failed" );
		}
	}

	/*
		idMatX::LU_Inverse
	*/

	m2 = original;

	m2.LU_Factor(nullptr);
	m2.LU_Inverse( m1, nullptr);
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		idLib::common->Warning( "idMatX::LU_Inverse failed" );
	}

	/*
		idMatX::QR_Factor
	*/

	c.SetSize( size );
	d.SetSize( size );

	m1 = original;

	m1.QR_Factor( c, d );
	m1.QR_UnpackFactors( q1, r1, c, d );
	m1 = q1 * r1;

	if ( !original.Compare( m1, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::QR_Factor failed" );
	}

	/*
		idMatX::QR_UpdateRankOne
	*/

	c.SetSize( size );
	d.SetSize( size );

	m1 = original;
	m2 = original;

	w.Random( size, 0 );
	v = w;

	// factor m1
	m1.QR_Factor( c, d );
	m1.QR_UnpackFactors( q1, r1, c, d );

	// modify and factor m2 
	m2.Update_RankOne( v, w, 1.0f );
	if ( !m2.QR_Factor( c, d ) ) {
		assert( 0 );
	}
	m2.QR_UnpackFactors( q2, r2, c, d );
	m2 = q2 * r2;

	// update factored m1
	q1.QR_UpdateRankOne( r1, v, w, 1.0f );
	m1 = q1 * r1;

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::QR_UpdateRankOne failed" );
	}

	/*
		idMatX::QR_UpdateRowColumn
	*/

	for ( offset = 0; offset < size; offset++ ) {
		c.SetSize( size );
		d.SetSize( size );

		m1 = original;
		m2 = original;

		v.Random( size, 1 );
		w.Random( size, 2 );
		w[offset] = 0.0f;

		// factor m1
		m1.QR_Factor( c, d );
		m1.QR_UnpackFactors( q1, r1, c, d );

		// modify and factor m2
		m2.Update_RowColumn( v, w, offset );
		if ( !m2.QR_Factor( c, d ) ) {
			assert( 0 );
		}
		m2.QR_UnpackFactors( q2, r2, c, d );
		m2 = q2 * r2;

		// update m1
		q1.QR_UpdateRowColumn( r1, v, w, offset );
		m1 = q1 * r1;

		if ( !m1.Compare( m2, 1e-3f ) ) {
			idLib::common->Warning( "idMatX::QR_UpdateRowColumn failed" );
		}
	}

	/*
		idMatX::QR_UpdateIncrement
	*/

	c.SetSize( size+1 );
	d.SetSize( size+1 );

	m1 = original;
	m2 = original;

	v.Random( size + 1, 1 );
	w.Random( size + 1, 2 );
	w[size] = 0.0f;

	// factor m1
	m1.QR_Factor( c, d );
	m1.QR_UnpackFactors( q1, r1, c, d );

	// modify and factor m2 
	m2.Update_Increment( v, w );
	if ( !m2.QR_Factor( c, d ) ) {
		assert( 0 );
	}
	m2.QR_UnpackFactors( q2, r2, c, d );
	m2 = q2 * r2;

	// update factored m1
	q1.QR_UpdateIncrement( r1, v, w );
	m1 = q1 * r1;

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::QR_UpdateIncrement failed" );
	}

	/*
		idMatX::QR_UpdateDecrement
	*/

	for ( offset = 0; offset < size; offset++ ) {
		c.SetSize( size+1 );
		d.SetSize( size+1 );

		m1 = original;
		m2 = original;

		v.SetSize( 6 );
		w.SetSize( 6 );
		for ( size_t i = 0; std::cmp_less(i, size); i++ ) {
			v[i] = original[i][offset];
			w[i] = original[offset][i];
		}

		// factor m1
		m1.QR_Factor( c, d );
		m1.QR_UnpackFactors( q1, r1, c, d );

		// modify and factor m2
		m2.Update_Decrement( offset );
		if ( !m2.QR_Factor( c, d ) ) {
			assert( 0 );
		}
		m2.QR_UnpackFactors( q2, r2, c, d );
		m2 = q2 * r2;

		// update factors of m1
		q1.QR_UpdateDecrement( r1, v, w, offset );
		m1 = q1 * r1;

		if ( !m1.Compare( m2, 1e-3f ) ) {
			idLib::common->Warning( "idMatX::QR_UpdateDecrement failed" );
		}
	}

	/*
		idMatX::QR_Inverse
	*/

	m2 = original;

	m2.QR_Factor( c, d );
	m2.QR_Inverse( m1, c, d );
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		idLib::common->Warning( "idMatX::QR_Inverse failed" );
	}

	/*
		idMatX::SVD_Factor
	*/

	m1 = original;
	m3.Zero( size, size );
	w.Zero( size );

	m1.SVD_Factor( w, m3 );
	m2.Diag( w );
	m3.TransposeSelf();
	m1 = m1 * m2 * m3;

	if ( !original.Compare( m1, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::SVD_Factor failed" );
	}

	/*
		idMatX::SVD_Inverse
	*/

	m2 = original;

	m2.SVD_Factor( w, m3 );
	m2.SVD_Inverse( m1, w, m3 );
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		idLib::common->Warning( "idMatX::SVD_Inverse failed" );
	}

	/*
		idMatX::Cholesky_Factor
	*/

	m1 = original;

	m1.Cholesky_Factor();
	m1.Cholesky_MultiplyFactors( m2 );

	if ( !original.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::Cholesky_Factor failed" );
	}

	/*
		idMatX::Cholesky_UpdateRankOne
	*/

	m1 = original;
	m2 = original;

	w.Random( size, 0 );

	// factor m1
	m1.Cholesky_Factor();
	m1.ClearUpperTriangle();

	// modify and factor m2 
	m2.Update_RankOneSymmetric( w, 1.0f );
	if ( !m2.Cholesky_Factor() ) {
		assert( 0 );
	}
	m2.ClearUpperTriangle();

	// update factored m1
	m1.Cholesky_UpdateRankOne( w, 1.0f, 0 );

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::Cholesky_UpdateRankOne failed" );
	}

	/*
		idMatX::Cholesky_UpdateRowColumn
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		// factor m1
		m1.Cholesky_Factor();
		m1.ClearUpperTriangle();

		int pdtable[] = { 1, 0, 1, 0, 0, 0 };
		w.Random( size, pdtable[offset] );
		w *= 0.1f;

		// modify and factor m2
		m2.Update_RowColumnSymmetric( w, offset );
		if ( !m2.Cholesky_Factor() ) {
			assert( 0 );
		}
		m2.ClearUpperTriangle();

		// update m1
		m1.Cholesky_UpdateRowColumn( w, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			idLib::common->Warning( "idMatX::Cholesky_UpdateRowColumn failed" );
		}
	}

	/*
		idMatX::Cholesky_UpdateIncrement
	*/

	m1.Random( size + 1, size + 1, 0 );
	m3 = m1 * m1.Transpose();

	m1.SquareSubMatrix( m3, size );
	m2 = m1;

	w.SetSize( size + 1 );
	for ( size_t i = 0; i < size + 1; i++ ) {
		w[i] = m3[size][i];
	}

	// factor m1
	m1.Cholesky_Factor();

	// modify and factor m2 
	m2.Update_IncrementSymmetric( w );
	if ( !m2.Cholesky_Factor() ) {
		assert( 0 );
	}

	// update factored m1
	m1.Cholesky_UpdateIncrement( w );

	m1.ClearUpperTriangle();
	m2.ClearUpperTriangle();

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::Cholesky_UpdateIncrement failed" );
	}

	/*
		idMatX::Cholesky_UpdateDecrement
	*/

	for ( offset = 0; offset < size; offset += size - 1 ) {
		m1 = original;
		m2 = original;

		v.SetSize( 6 );
		for ( size_t i = 0; std::cmp_less(i, size); i++ ) {
			v[i] = original[i][offset];
		}

		// factor m1
		m1.Cholesky_Factor();

		// modify and factor m2
		m2.Update_Decrement( offset );
		if ( !m2.Cholesky_Factor() ) {
			assert( 0 );
		}

		// update factors of m1
		m1.Cholesky_UpdateDecrement( v, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			idLib::common->Warning( "idMatX::Cholesky_UpdateDecrement failed" );
		}
	}

	/*
		idMatX::Cholesky_Inverse
	*/

	m2 = original;

	m2.Cholesky_Factor();
	m2.Cholesky_Inverse( m1 );
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		idLib::common->Warning( "idMatX::Cholesky_Inverse failed" );
	}

	/*
		idMatX::LDLT_Factor
	*/

	m1 = original;

	m1.LDLT_Factor();
	m1.LDLT_MultiplyFactors( m2 );

	if ( !original.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::LDLT_Factor failed" );
	}

	m1.LDLT_UnpackFactors( m2, m3 );
	m2 = m2 * m3 * m2.Transpose();

	if ( !original.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::LDLT_Factor failed" );
	}

	/*
		idMatX::LDLT_UpdateRankOne
	*/

	m1 = original;
	m2 = original;

	w.Random( size, 0 );

	// factor m1
	m1.LDLT_Factor();
	m1.ClearUpperTriangle();

	// modify and factor m2 
	m2.Update_RankOneSymmetric( w, 1.0f );
	if ( !m2.LDLT_Factor() ) {
		assert( 0 );
	}
	m2.ClearUpperTriangle();

	// update factored m1
	m1.LDLT_UpdateRankOne( w, 1.0f, 0 );

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::LDLT_UpdateRankOne failed" );
	}

	/*
		idMatX::LDLT_UpdateRowColumn
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		w.Random( size, 0 );

		// factor m1
		m1.LDLT_Factor();
		m1.ClearUpperTriangle();

		// modify and factor m2
		m2.Update_RowColumnSymmetric( w, offset );
		if ( !m2.LDLT_Factor() ) {
			assert( 0 );
		}
		m2.ClearUpperTriangle();

		// update m1
		m1.LDLT_UpdateRowColumn( w, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			idLib::common->Warning( "idMatX::LDLT_UpdateRowColumn failed" );
		}
	}

	/*
		idMatX::LDLT_UpdateIncrement
	*/

	m1.Random( size + 1, size + 1, 0 );
	m3 = m1 * m1.Transpose();

	m1.SquareSubMatrix( m3, size );
	m2 = m1;

	w.SetSize( size + 1 );
	for ( size_t i = 0; i < size + 1; i++ ) {
		w[i] = m3[size][i];
	}

	// factor m1
	m1.LDLT_Factor();

	// modify and factor m2 
	m2.Update_IncrementSymmetric( w );
	if ( !m2.LDLT_Factor() ) {
		assert( 0 );
	}

	// update factored m1
	m1.LDLT_UpdateIncrement( w );

	m1.ClearUpperTriangle();
	m2.ClearUpperTriangle();

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::LDLT_UpdateIncrement failed" );
	}

	/*
		idMatX::LDLT_UpdateDecrement
	*/

	for ( offset = 0; offset < size; offset++ ) {
		m1 = original;
		m2 = original;

		v.SetSize( 6 );
		for ( size_t i = 0; std::cmp_less(i, size); i++ ) {
			v[i] = original[i][offset];
		}

		// factor m1
		m1.LDLT_Factor();

		// modify and factor m2
		m2.Update_Decrement( offset );
		if ( !m2.LDLT_Factor() ) {
			assert( 0 );
		}

		// update factors of m1
		m1.LDLT_UpdateDecrement( v, offset );

		if ( !m1.Compare( m2, 1e-3f ) ) {
			idLib::common->Warning( "idMatX::LDLT_UpdateDecrement failed" );
		}
	}

	/*
		idMatX::LDLT_Inverse
	*/

	m2 = original;

	m2.LDLT_Factor();
	m2.LDLT_Inverse( m1 );
	m1 *= original;

	if ( !m1.IsIdentity( 1e-4f ) ) {
		idLib::common->Warning( "idMatX::LDLT_Inverse failed" );
	}

	/*
		idMatX::Eigen_SolveSymmetricTriDiagonal
	*/

	m3 = original;
	m3.TriDiagonal_ClearTriangles();
	m1 = m3;

	v.SetSize( size );

	m1.Eigen_SolveSymmetricTriDiagonal( v );

	m3.TransposeMultiply( m2, m1 );

	for ( size_t i = 0; std::cmp_less(i, size); i++ ) {
		for ( size_t j = 0; std::cmp_less(j, size); j++ ) {
			m1[i][j] *= v[j];
		}
	}

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::Eigen_SolveSymmetricTriDiagonal failed" );
	}

	/*
		idMatX::Eigen_SolveSymmetric
	*/

	m3 = original;
	m1 = m3;

	v.SetSize( size );

	m1.Eigen_SolveSymmetric( v );

	m3.TransposeMultiply( m2, m1 );

	for ( size_t i = 0; std::cmp_less(i, size); i++ ) {
		for ( size_t j = 0; std::cmp_less(j, size); j++ ) {
			m1[i][j] *= v[j];
		}
	}

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::Eigen_SolveSymmetric failed" );
	}

	/*
		idMatX::Eigen_Solve
	*/

	m3 = original;
	m1 = m3;

	v.SetSize( size );
	w.SetSize( size );

	m1.Eigen_Solve( v, w );

	m3.TransposeMultiply( m2, m1 );

	for ( size_t i = 0; std::cmp_less(i, size); i++ ) {
		for ( size_t j = 0; std::cmp_less(j, size); j++ ) {
			m1[i][j] *= v[j];
		}
	}

	if ( !m1.Compare( m2, 1e-4f ) ) {
		idLib::common->Warning( "idMatX::Eigen_Solve failed" );
	}
}
