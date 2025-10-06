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
#include "../precompiled.h"

//===============================================================
//
//	idODE_Euler
//
//===============================================================

/*
=============
idODE_Euler::idODE_Euler
=============
*/
idODE_Euler::idODE_Euler( const int dim, const deriveFunction_t dr, const void *ud ) {
	dimension = dim;
	derivatives = new (TAG_MATH) float[dim];
	derive = dr;
	userData = ud;
}

/*
=============
idODE_Euler::~idODE_Euler
=============
*/
idODE_Euler::~idODE_Euler() {
	delete[] derivatives;
}

/*
=============
idODE_Euler::Evaluate
=============
*/
float idODE_Euler::Evaluate( const float *state, float *newState, const float t0, const float t1 ) {
	derive( t0, userData, state, derivatives );
	float delta = t1 - t0;
	for ( int i = 0; i < dimension; i++ ) {
		newState[i] = state[i] + delta * derivatives[i];
	}
	return delta;
}

//===============================================================
//
//	idODE_Midpoint
//
//===============================================================

/*
=============
idODE_Midpoint::idODE_Midpoint
=============
*/
idODE_Midpoint::idODE_Midpoint( const int dim, const deriveFunction_t dr, const void *ud ) {
	dimension = dim;
	tmpState = new (TAG_MATH) float[dim];
	derivatives = new (TAG_MATH) float[dim];
	derive = dr;
	userData = ud;
}

/*
=============
idODE_Midpoint::~idODE_Midpoint
=============
*/
idODE_Midpoint::~idODE_Midpoint() {
	delete tmpState;
	delete derivatives;
}

/*
=============
idODE_Midpoint::~Evaluate
=============
*/
float idODE_Midpoint::Evaluate( const float *state, float *newState, const float t0, const float t1 ) {
	int i;

	double delta = t1 - t0;
	double halfDelta = delta * 0.5;
    // first step
	derive( t0, userData, state, derivatives );
	for ( i = 0; i < dimension; i++ ) {
		tmpState[i] = state[i] + halfDelta * derivatives[i];
	}
    // second step
	derive( t0 + halfDelta, userData, tmpState, derivatives );

	for ( i = 0; i < dimension; i++ ) {
		newState[i] = state[i] + delta * derivatives[i];
	}
	return delta;
}

//===============================================================
//
//	idODE_RK4
//
//===============================================================

/*
=============
idODE_RK4::idODE_RK4
=============
*/
idODE_RK4::idODE_RK4( const int dim, const deriveFunction_t dr, const void *ud ) {
	dimension = dim;
	derive = dr;
	userData = ud;
	tmpState = new (TAG_MATH) float[dim];
	d1 = new (TAG_MATH) float[dim];
	d2 = new (TAG_MATH) float[dim];
	d3 = new (TAG_MATH) float[dim];
	d4 = new (TAG_MATH) float[dim];
}

/*
=============
idODE_RK4::~idODE_RK4
=============
*/
idODE_RK4::~idODE_RK4() {
	delete tmpState;
	delete d1;
	delete d2;
	delete d3;
	delete d4;
}

/*
=============
idODE_RK4::Evaluate
=============
*/
float idODE_RK4::Evaluate( const float *state, float *newState, const float t0, const float t1 ) {
	int i;

	double delta = t1 - t0;
	double halfDelta = delta * 0.5;
	// first step
	derive( t0, userData, state, d1 );
	for ( i = 0; i < dimension; i++ ) {
		tmpState[i] = state[i] + halfDelta * d1[i];
	}
	// second step
	derive( t0 + halfDelta, userData, tmpState, d2 );
	for ( i = 0; i < dimension; i++ ) {
		tmpState[i] = state[i] + halfDelta * d2[i];
	}
	// third step
	derive( t0 + halfDelta, userData, tmpState, d3 );
	for ( i = 0; i < dimension; i++ ) {
		tmpState[i] = state[i] + delta * d3[i];
	}
	// fourth step
	derive( t0 + delta, userData, tmpState, d4 );

	double sixthDelta = delta * (1.0 / 6.0);
	for ( i = 0; i < dimension; i++ ) {
		newState[i] = state[i] + sixthDelta * (d1[i] + 2.0 * (d2[i] + d3[i]) + d4[i]);
	}
	return delta;
}

//===============================================================
//
//	idODE_RK4Adaptive
//
//===============================================================

/*
=============
idODE_RK4Adaptive::idODE_RK4Adaptive
=============
*/
idODE_RK4Adaptive::idODE_RK4Adaptive( const int dim, const deriveFunction_t dr, const void *ud ) {
	dimension = dim;
	derive = dr;
	userData = ud;
	maxError = 0.01f;
	tmpState = new (TAG_MATH) float[dim];
	d1 = new (TAG_MATH) float[dim];
	d1half = new (TAG_MATH) float [dim];
	d2 = new (TAG_MATH) float[dim];
	d3 = new (TAG_MATH) float[dim];
	d4 = new (TAG_MATH) float[dim];
}

/*
=============
idODE_RK4Adaptive::~idODE_RK4Adaptive
=============
*/
idODE_RK4Adaptive::~idODE_RK4Adaptive() {
	delete tmpState;
	delete d1;
	delete d1half;
	delete d2;
	delete d3;
	delete d4;
}

/*
=============
idODE_RK4Adaptive::SetMaxError
=============
*/
void idODE_RK4Adaptive::SetMaxError( const float err ) {
	if ( err > 0.0f ) {
		maxError = err;
	}
}

/*
=============
idODE_RK4Adaptive::Evaluate
=============
*/
float idODE_RK4Adaptive::Evaluate( const float *state, float *newState, const float t0, const float t1 ) {
	double error;
	int i;

	double delta = t1 - t0;

	for ( int n = 0; n < 4; n++ ) {

		double halfDelta = delta * 0.5;
		double fourthDelta = delta * 0.25;

		// first step of first half delta
		derive( t0, userData, state, d1 );
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + fourthDelta * d1[i];
		}
		// second step of first half delta
		derive( t0 + fourthDelta, userData, tmpState, d2 );
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + fourthDelta * d2[i];
		}
		// third step of first half delta
		derive( t0 + fourthDelta, userData, tmpState, d3 );
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + halfDelta * d3[i];
		}
		// fourth step of first half delta
		derive( t0 + halfDelta, userData, tmpState, d4 );

		double sixthDelta = halfDelta * (1.0 / 6.0);
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + sixthDelta * (d1[i] + 2.0 * (d2[i] + d3[i]) + d4[i]);
		}

		// first step of second half delta
		derive( t0 + halfDelta, userData, tmpState, d1half );
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + fourthDelta * d1half[i];
		}
		// second step of second half delta
		derive( t0 + halfDelta + fourthDelta, userData, tmpState, d2 );
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + fourthDelta * d2[i];
		}
		// third step of second half delta
		derive( t0 + halfDelta + fourthDelta, userData, tmpState, d3 );
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + halfDelta * d3[i];
		}
		// fourth step of second half delta
		derive( t0 + delta, userData, tmpState, d4 );

		sixthDelta = halfDelta * (1.0/6.0);
		for ( i = 0; i < dimension; i++ ) {
			newState[i] = state[i] + sixthDelta * (d1[i] + 2.0 * (d2[i] + d3[i]) + d4[i]);
		}

		// first step of full delta
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + halfDelta * d1[i];
		}
		// second step of full delta
		derive( t0 + halfDelta, userData, tmpState, d2 );
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + halfDelta * d2[i];
		}
		// third step of full delta
		derive( t0 + halfDelta, userData, tmpState, d3 );
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + delta * d3[i];
		}
		// fourth step of full delta
		derive( t0 + delta, userData, tmpState, d4 );

		sixthDelta = delta * (1.0/6.0);
		for ( i = 0; i < dimension; i++ ) {
			tmpState[i] = state[i] + sixthDelta * (d1[i] + 2.0 * (d2[i] + d3[i]) + d4[i]);
		}

		// get max estimated error
        double max = 0.0;
		for ( i = 0; i < dimension; i++ ) {
			error = idMath::Fabs( (newState[i] - tmpState[i]) / (delta * d1[i] + 1e-10) );
			if ( error > max ) {
				max = error;
			}
        }
		error = max / maxError;

        if ( error <= 1.0f ) {
			return delta * 4.0;
		}
		if ( delta <= 1e-7 ) {
			return delta;
		}
		delta *= 0.25;
	}
	return delta;
}

