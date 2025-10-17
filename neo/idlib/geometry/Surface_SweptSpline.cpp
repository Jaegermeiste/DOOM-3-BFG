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

/*
====================
idSurface_SweptSpline::SetSpline
====================
*/
void idSurface_SweptSpline::SetSpline( idCurve_Spline<idVec4> *spline ) {
	if ( this->spline ) {
		delete this->spline;
	}
	this->spline = spline;
}

/*
====================
idSurface_SweptSpline::SetSweptSpline
====================
*/
void idSurface_SweptSpline::SetSweptSpline( idCurve_Spline<idVec4> *sweptSpline ) {
	if ( this->sweptSpline ) {
		delete this->sweptSpline;
	}
	this->sweptSpline = sweptSpline;
}

/*
====================
idSurface_SweptSpline::SetSweptCircle

  Sets the swept spline to a NURBS circle.
====================
*/
void idSurface_SweptSpline::SetSweptCircle( const float radius ) {
	idCurve_NURBS<idVec4> *nurbs = new (TAG_IDLIB_SURFACE) idCurve_NURBS<idVec4>();
	nurbs->Clear();
	nurbs->AddValue(   0, idVec4(  radius,  radius, 0.0f, 0.00f ) );
	nurbs->AddValue( 100, idVec4( -radius,  radius, 0.0f, 0.25f ) );
	nurbs->AddValue( 200, idVec4( -radius, -radius, 0.0f, 0.50f ) );
	nurbs->AddValue( 300, idVec4(  radius, -radius, 0.0f, 0.75f ) );
	nurbs->SetBoundaryType( idCurve_NURBS<idVec4>::BT_CLOSED );
	nurbs->SetCloseTime( 100 );
	if ( sweptSpline ) {
		delete sweptSpline;
	}
	sweptSpline = nurbs;
}

/*
====================
idSurface_SweptSpline::GetFrame
====================
*/
void idSurface_SweptSpline::GetFrame( const idMat3 &previousFrame, const idVec3 dir, idMat3 &newFrame ) {
	idMat3 axis = {};

	idVec3 d = dir;
	d.Normalize();
	idVec3 v = d.Cross(previousFrame[2]);
	v.Normalize();

	const float a = idMath::ACos(previousFrame[2] * d) * 0.5f;
	const float c = idMath::Cos(a);
	const float s = idMath::Sqrt(1.0f - c * c);

	const float x = v[0] * s;
	const float y = v[1] * s;
	const float z = v[2] * s;

	const float x2 = x + x;
	const float y2 = y + y;
	const float z2 = z + z;
	const float xx = x * x2;
	const float xy = x * y2;
	const float xz = x * z2;
	const float yy = y * y2;
	const float yz = y * z2;
	const float zz = z * z2;
	const float wx = c * x2;
	const float wy = c * y2;
	const float wz = c * z2;

	axis[0][0] = 1.0f - ( yy + zz );
	axis[0][1] = xy - wz;
	axis[0][2] = xz + wy;
	axis[1][0] = xy + wz;
	axis[1][1] = 1.0f - ( xx + zz );
	axis[1][2] = yz - wx;
	axis[2][0] = xz - wy;
	axis[2][1] = yz + wx;
	axis[2][2] = 1.0f - ( xx + yy );

	newFrame = previousFrame * axis;

	newFrame[2] = dir;
	newFrame[2].Normalize();
	newFrame[1].Cross( newFrame[ 2 ], newFrame[ 0 ] );
	newFrame[1].Normalize();
	newFrame[0].Cross( newFrame[ 1 ], newFrame[ 2 ] );
	newFrame[0].Normalize();
}

/*
====================
idSurface_SweptSpline::Tessellate

  tesselate the surface
====================
*/
void idSurface_SweptSpline::Tessellate( const size_t splineSubdivisions, const size_t sweptSplineSubdivisions ) {
	size_t i = 0, j = 0, offset = 0;
	ID_TIME_T t = 0;
	idVec4 splinePos = {}, splineD1 = {};
	idMat3 splineMat = {};

	if ( !spline || !sweptSpline ) {
		idSurface::Clear();
		return;
	}

	verts.SetNum( splineSubdivisions * sweptSplineSubdivisions );

	// calculate the points and first derivatives for the swept spline
	ID_TIME_T totalTime = sweptSpline->GetTime(sweptSpline->GetNumValues() - 1) - sweptSpline->GetTime(0) + sweptSpline->GetCloseTime();
	const size_t sweptSplineDiv = std::equal_to<>()(sweptSpline->GetBoundaryType(), idCurve_Spline<idVec3>::BT_CLOSED)
		                     ? sweptSplineSubdivisions
		                     : sweptSplineSubdivisions - 1;
	const size_t baseOffset = (splineSubdivisions - 1) * sweptSplineSubdivisions;
	for ( i = 0; i < sweptSplineSubdivisions; i++ ) {
		t = idMath::integer_cast<ID_TIME_T>(idMath::Itof<double>(totalTime) * (idMath::Itof<double>(i) / idMath::Itof<double>(sweptSplineDiv)));
		splinePos = sweptSpline->GetCurrentValue( t );
		splineD1 = sweptSpline->GetCurrentFirstDerivative( t );
		verts[baseOffset+i].xyz = splinePos.ToVec3();
		verts[baseOffset+i].SetTexCoordS( splinePos.w );
		verts[baseOffset+i].SetTangent( splineD1.ToVec3() );
	}

	// sweep the spline
	totalTime = spline->GetTime( spline->GetNumValues() - 1 ) - spline->GetTime( 0 ) + spline->GetCloseTime();
	const size_t splineDiv = std::equal_to<>()(spline->GetBoundaryType(), idCurve_Spline<idVec3>::BT_CLOSED)
		                ? splineSubdivisions
		                : splineSubdivisions - 1;
	splineMat.Identity();
	for ( i = 0; i < splineSubdivisions; i++ ) {
		t = idMath::integer_cast<ID_TIME_T>(idMath::Itof<double>(totalTime) * (idMath::Itof<double>(i) / idMath::Itof<double>(splineDiv)));

		splinePos = spline->GetCurrentValue( t );
		splineD1 = spline->GetCurrentFirstDerivative( t );

		GetFrame( splineMat, splineD1.ToVec3(), splineMat );

		offset = i * sweptSplineSubdivisions;
		for ( j = 0; j < sweptSplineSubdivisions; j++ ) {
			idDrawVert *v = &verts[offset+j];
			v->xyz = splinePos.ToVec3() + verts[baseOffset+j].xyz * splineMat;
			v->SetTexCoord( verts[baseOffset+j].GetTexCoord().x, splinePos.w );
			v->SetTangent( verts[baseOffset+j].GetTangent() * splineMat );
			v->SetBiTangent( splineD1.ToVec3() );
			idVec3 tempNormal = v->GetBiTangent().Cross(v->GetTangent());
			tempNormal.Normalize();
			v->SetNormal( tempNormal );
			v->color[0] = v->color[1] = v->color[2] = v->color[3] = 0;
		}
	}

	indexes.SetNum( splineDiv * sweptSplineDiv * 2 * 3 );

	// create indexes for the triangles
	for ( offset = i = 0; i < splineDiv; i++ ) {

		const size_t i0 = (i + 0) * sweptSplineSubdivisions;
		const size_t i1 = (i + 1) % splineSubdivisions * sweptSplineSubdivisions;

		for ( j = 0; j < sweptSplineDiv; j++ ) {

			const size_t j0 = (j + 0);
			const size_t j1 = (j + 1) % sweptSplineSubdivisions;

			indexes[offset++] = i0 + j0;
			indexes[offset++] = i0 + j1;
			indexes[offset++] = i1 + j1;

			indexes[offset++] = i1 + j1;
			indexes[offset++] = i1 + j0;
			indexes[offset++] = i0 + j0;
		}
	}

	GenerateEdgeIndexes();
}
