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
#include "../../idlib/precompiled.h"
#include "TraceModel.h"

#include <algorithm>
#include <utility>

/*
============
idTraceModel::SetupBox
============
*/
void idTraceModel::SetupBox( const idBounds &boxBounds ) {
	int i;

	if ( type != TRM_BOX ) {
		InitBox();
	}
	// offset to center
	offset = ( boxBounds[0] + boxBounds[1] ) * 0.5f;
	// set box vertices
	for ( i = 0; i < 8; i++ ) {
		verts[i][0] = boxBounds[(i^(i>>1))&1][0];
		verts[i][1] = boxBounds[(i>>1)&1][1];
		verts[i][2] = boxBounds[(i>>2)&1][2];
	}
	// set polygon plane distances
	polys[0].dist = -boxBounds[0][2];
	polys[1].dist = boxBounds[1][2];
	polys[2].dist = -boxBounds[0][1];
	polys[3].dist = boxBounds[1][0];
	polys[4].dist = boxBounds[1][1];
	polys[5].dist = -boxBounds[0][0];
	// set polygon bounds
	for ( i = 0; i < 6; i++ ) {
		polys[i].bounds = boxBounds;
	}
	polys[0].bounds[1][2] = boxBounds[0][2];
	polys[1].bounds[0][2] = boxBounds[1][2];
	polys[2].bounds[1][1] = boxBounds[0][1];
	polys[3].bounds[0][0] = boxBounds[1][0];
	polys[4].bounds[0][1] = boxBounds[1][1];
	polys[5].bounds[1][0] = boxBounds[0][0];

	bounds = boxBounds;
}

/*
============
idTraceModel::SetupBox

  The origin is placed at the center of the cube.
============
*/
void idTraceModel::SetupBox( const float size ) {
	idBounds boxBounds;

	const float halfSize = size * 0.5f;
	boxBounds[0].Set( -halfSize, -halfSize, -halfSize );
	boxBounds[1].Set( halfSize, halfSize, halfSize );
	SetupBox( boxBounds );
}

/*
============
idTraceModel::InitBox

  Initialize size independent box.
============
*/
void idTraceModel::InitBox() {
	type = TRM_BOX;
	numVerts = 8;
	numEdges = 12;
	numPolys = 6;

	// set box edges
	for ( size_t i = 0; i < 4; i++ ) {
		edges[ i + 1 ].v[0] = i;
		edges[ i + 1 ].v[1] = (i + 1) & 3;
		edges[ i + 5 ].v[0] = 4 + i;
		edges[ i + 5 ].v[1] = 4 + ((i + 1) & 3);
		edges[ i + 9 ].v[0] = i;
		edges[ i + 9 ].v[1] = 4 + i;
	}

	// all edges of a polygon go counter clockwise
	polys[0].numEdges = 4;
	polys[0].edges[0] = -4;
	polys[0].edges[1] = -3;
	polys[0].edges[2] = -2;
	polys[0].edges[3] = -1;
	polys[0].normal.Set( 0.0f, 0.0f, -1.0f );

	polys[1].numEdges = 4;
	polys[1].edges[0] = 5;
	polys[1].edges[1] = 6;
	polys[1].edges[2] = 7;
	polys[1].edges[3] = 8;
	polys[1].normal.Set( 0.0f, 0.0f, 1.0f );

	polys[2].numEdges = 4;
	polys[2].edges[0] = 1;
	polys[2].edges[1] = 10;
	polys[2].edges[2] = -5;
	polys[2].edges[3] = -9;
	polys[2].normal.Set( 0.0f, -1.0f,  0.0f );

	polys[3].numEdges = 4;
	polys[3].edges[0] = 2;
	polys[3].edges[1] = 11;
	polys[3].edges[2] = -6;
	polys[3].edges[3] = -10;
	polys[3].normal.Set( 1.0f,  0.0f,  0.0f );

	polys[4].numEdges = 4;
	polys[4].edges[0] = 3;
	polys[4].edges[1] = 12;
	polys[4].edges[2] = -7;
	polys[4].edges[3] = -11;
	polys[4].normal.Set( 0.0f,  1.0f,  0.0f );

	polys[5].numEdges = 4;
	polys[5].edges[0] = 4;
	polys[5].edges[1] = 9;
	polys[5].edges[2] = -8;
	polys[5].edges[3] = -12;
	polys[5].normal.Set( -1.0f,  0.0f,  0.0f );

	// convex model
	isConvex = true;

	GenerateEdgeNormals();
}

/*
============
idTraceModel::SetupOctahedron
============
*/
void idTraceModel::SetupOctahedron( const idBounds &octBounds ) {
	idVec3 v;

	if ( type != TRM_OCTAHEDRON ) {
		InitOctahedron();
	}

	offset = ( octBounds[0] + octBounds[1] ) * 0.5f;
	v[0] = octBounds[1][0] - offset[0];
	v[1] = octBounds[1][1] - offset[1];
	v[2] = octBounds[1][2] - offset[2];

	// set vertices
	verts[0].Set( offset.x + v[0], offset.y, offset.z );
	verts[1].Set( offset.x - v[0], offset.y, offset.z );
	verts[2].Set( offset.x, offset.y + v[1], offset.z );
	verts[3].Set( offset.x, offset.y - v[1], offset.z );
	verts[4].Set( offset.x, offset.y, offset.z + v[2] );
	verts[5].Set( offset.x, offset.y, offset.z - v[2] );

	// set polygons
	for ( int i = 0; std::cmp_less(i, numPolys); i++ ) {
		const int64 e0 = polys[i].edges[0];
		const int64 e1 = polys[i].edges[1];
		const size_t v0 = edges[abs(e0)].v[INT64_SIGNBITSET(e0)];
		const size_t v1 = edges[abs(e0)].v[INT64_SIGNBITNOTSET(e0)];
		const size_t v2 = edges[abs(e1)].v[INT64_SIGNBITNOTSET(e1)];
		// polygon plane
		polys[i].normal = ( verts[v1] - verts[v0] ).Cross( verts[v2] - verts[v0] );
		polys[i].normal.Normalize();
		polys[i].dist = polys[i].normal * verts[v0];
		// polygon bounds
		polys[i].bounds[0] = polys[i].bounds[1] = verts[v0];
		polys[i].bounds.AddPoint( verts[v1] );
		polys[i].bounds.AddPoint( verts[v2] );
	}

	// trm bounds
	bounds = octBounds;

	GenerateEdgeNormals();
}

/*
============
idTraceModel::SetupOctahedron

  The origin is placed at the center of the octahedron.
============
*/
void idTraceModel::SetupOctahedron( const float size ) {
	idBounds octBounds;

	const float halfSize = size * 0.5f;
	octBounds[0].Set( -halfSize, -halfSize, -halfSize );
	octBounds[1].Set( halfSize, halfSize, halfSize );
	SetupOctahedron( octBounds );
}

/*
============
idTraceModel::InitOctahedron

  Initialize size independent octahedron.
============
*/
void idTraceModel::InitOctahedron() {

	type = TRM_OCTAHEDRON;
	numVerts = 6;
	numEdges = 12;
	numPolys = 8;

	// set edges
	edges[ 1].v[0] =  4; edges[ 1].v[1] =  0;
	edges[ 2].v[0] =  0; edges[ 2].v[1] =  2;
	edges[ 3].v[0] =  2; edges[ 3].v[1] =  4;
	edges[ 4].v[0] =  2; edges[ 4].v[1] =  1;
	edges[ 5].v[0] =  1; edges[ 5].v[1] =  4;
	edges[ 6].v[0] =  1; edges[ 6].v[1] =  3;
	edges[ 7].v[0] =  3; edges[ 7].v[1] =  4;
	edges[ 8].v[0] =  3; edges[ 8].v[1] =  0;
	edges[ 9].v[0] =  5; edges[ 9].v[1] =  2;
	edges[10].v[0] =  0; edges[10].v[1] =  5;
	edges[11].v[0] =  5; edges[11].v[1] =  1;
	edges[12].v[0] =  5; edges[12].v[1] =  3;

	// all edges of a polygon go counter clockwise
	polys[0].numEdges = 3;
	polys[0].edges[0] = 1;
	polys[0].edges[1] = 2;
	polys[0].edges[2] = 3;

	polys[1].numEdges = 3;
	polys[1].edges[0] = -3;
	polys[1].edges[1] = 4;
	polys[1].edges[2] = 5;

	polys[2].numEdges = 3;
	polys[2].edges[0] = -5;
	polys[2].edges[1] = 6;
	polys[2].edges[2] = 7;

	polys[3].numEdges = 3;
	polys[3].edges[0] = -7;
	polys[3].edges[1] = 8;
	polys[3].edges[2] = -1;

	polys[4].numEdges = 3;
	polys[4].edges[0] = 9;
	polys[4].edges[1] = -2;
	polys[4].edges[2] = 10;

	polys[5].numEdges = 3;
	polys[5].edges[0] = 11;
	polys[5].edges[1] = -4;
	polys[5].edges[2] = -9;

	polys[6].numEdges = 3;
	polys[6].edges[0] = 12;
	polys[6].edges[1] = -6;
	polys[6].edges[2] = -11;

	polys[7].numEdges = 3;
	polys[7].edges[0] = -10;
	polys[7].edges[1] = -8;
	polys[7].edges[2] = -12;

	// convex model
	isConvex = true;
}

/*
============
idTraceModel::SetupDodecahedron
============
*/
void idTraceModel::SetupDodecahedron( const idBounds &dodBounds ) {
	idVec3 a, b, c;

	if ( type != TRM_DODECAHEDRON ) {
		InitDodecahedron();
	}

	a[0] = a[1] = a[2] = 0.5773502691896257f; // 1.0f / ( 3.0f ) ^ 0.5f;
	b[0] = b[1] = b[2] = 0.3568220897730899f; // ( ( 3.0f - ( 5.0f ) ^ 0.5f ) / 6.0f ) ^ 0.5f;
	c[0] = c[1] = c[2] = 0.9341723589627156f; // ( ( 3.0f + ( 5.0f ) ^ 0.5f ) / 6.0f ) ^ 0.5f;
	const float d = 0.5f / c[0];
	float s = (dodBounds[1][0] - dodBounds[0][0]) * d;
	a[0] *= s;
	b[0] *= s;
	c[0] *= s;
	s = ( dodBounds[1][1] - dodBounds[0][1] ) * d;
	a[1] *= s;
	b[1] *= s;
	c[1] *= s;
	s = ( dodBounds[1][2] - dodBounds[0][2] ) * d;
	a[2] *= s;
	b[2] *= s;
	c[2] *= s;

	offset = ( dodBounds[0] + dodBounds[1] ) * 0.5f;

	// set vertices
	verts[ 0].Set( offset.x + a[0], offset.y + a[1], offset.z + a[2] );
	verts[ 1].Set( offset.x + a[0], offset.y + a[1], offset.z - a[2] );
	verts[ 2].Set( offset.x + a[0], offset.y - a[1], offset.z + a[2] );
	verts[ 3].Set( offset.x + a[0], offset.y - a[1], offset.z - a[2] );
	verts[ 4].Set( offset.x - a[0], offset.y + a[1], offset.z + a[2] );
	verts[ 5].Set( offset.x - a[0], offset.y + a[1], offset.z - a[2] );
	verts[ 6].Set( offset.x - a[0], offset.y - a[1], offset.z + a[2] );
	verts[ 7].Set( offset.x - a[0], offset.y - a[1], offset.z - a[2] );
	verts[ 8].Set( offset.x + b[0], offset.y + c[1], offset.z        );
	verts[ 9].Set( offset.x - b[0], offset.y + c[1], offset.z        );
	verts[10].Set( offset.x + b[0], offset.y - c[1], offset.z        );
	verts[11].Set( offset.x - b[0], offset.y - c[1], offset.z        );
	verts[12].Set( offset.x + c[0], offset.y       , offset.z + b[2] );
	verts[13].Set( offset.x + c[0], offset.y       , offset.z - b[2] );
	verts[14].Set( offset.x - c[0], offset.y       , offset.z + b[2] );
	verts[15].Set( offset.x - c[0], offset.y       , offset.z - b[2] );
	verts[16].Set( offset.x       , offset.y + b[1], offset.z + c[2] );
	verts[17].Set( offset.x       , offset.y - b[1], offset.z + c[2] );
	verts[18].Set( offset.x       , offset.y + b[1], offset.z - c[2] );
	verts[19].Set( offset.x       , offset.y - b[1], offset.z - c[2] );

	// set polygons
	for ( size_t i = 0; std::cmp_less(i, numPolys); i++ ) {
		const int64 e0 = polys[i].edges[0];
		const int64 e1 = polys[i].edges[1];
		const int64 e2 = polys[i].edges[2];
		const int64 e3 = polys[i].edges[3];
		const size_t v0 = edges[abs(e0)].v[INT64_SIGNBITSET(e0)];
		const size_t v1 = edges[abs(e0)].v[INT64_SIGNBITNOTSET(e0)];
		const size_t v2 = edges[abs(e1)].v[INT64_SIGNBITNOTSET(e1)];
		const size_t v3 = edges[abs(e2)].v[INT64_SIGNBITNOTSET(e2)];
		const size_t v4 = edges[abs(e3)].v[INT64_SIGNBITNOTSET(e3)];
		// polygon plane
		polys[i].normal = ( verts[v1] - verts[v0] ).Cross( verts[v2] - verts[v0] );
		polys[i].normal.Normalize();
		polys[i].dist = polys[i].normal * verts[v0];
		// polygon bounds
		polys[i].bounds[0] = polys[i].bounds[1] = verts[v0];
		polys[i].bounds.AddPoint( verts[v1] );
		polys[i].bounds.AddPoint( verts[v2] );
		polys[i].bounds.AddPoint( verts[v3] );
		polys[i].bounds.AddPoint( verts[v4] );
	}

	// trm bounds
	bounds = dodBounds;

	GenerateEdgeNormals();
}

/*
============
idTraceModel::SetupDodecahedron

  The origin is placed at the center of the octahedron.
============
*/
void idTraceModel::SetupDodecahedron( const float size ) {
	idBounds dodBounds;

	const float halfSize = size * 0.5f;
	dodBounds[0].Set( -halfSize, -halfSize, -halfSize );
	dodBounds[1].Set( halfSize, halfSize, halfSize );
	SetupDodecahedron( dodBounds );
}

/*
============
idTraceModel::InitDodecahedron

  Initialize size independent dodecahedron.
============
*/
void idTraceModel::InitDodecahedron() {

	type = TRM_DODECAHEDRON;
	numVerts = 20;
	numEdges = 30;
	numPolys = 12;

	// set edges
	edges[ 1].v[0] =  0; edges[ 1].v[1] =  8;
	edges[ 2].v[0] =  8; edges[ 2].v[1] =  9;
	edges[ 3].v[0] =  9; edges[ 3].v[1] =  4;
	edges[ 4].v[0] =  4; edges[ 4].v[1] = 16;
	edges[ 5].v[0] = 16; edges[ 5].v[1] =  0;
	edges[ 6].v[0] = 16; edges[ 6].v[1] = 17;
	edges[ 7].v[0] = 17; edges[ 7].v[1] =  2;
	edges[ 8].v[0] =  2; edges[ 8].v[1] = 12;
	edges[ 9].v[0] = 12; edges[ 9].v[1] =  0;
	edges[10].v[0] =  2; edges[10].v[1] = 10;
	edges[11].v[0] = 10; edges[11].v[1] =  3;
	edges[12].v[0] =  3; edges[12].v[1] = 13;
	edges[13].v[0] = 13; edges[13].v[1] = 12;
	edges[14].v[0] =  9; edges[14].v[1] =  5;
	edges[15].v[0] =  5; edges[15].v[1] = 15;
	edges[16].v[0] = 15; edges[16].v[1] = 14;
	edges[17].v[0] = 14; edges[17].v[1] =  4;
	edges[18].v[0] =  3; edges[18].v[1] = 19;
	edges[19].v[0] = 19; edges[19].v[1] = 18;
	edges[20].v[0] = 18; edges[20].v[1] =  1;
	edges[21].v[0] =  1; edges[21].v[1] = 13;
	edges[22].v[0] =  7; edges[22].v[1] = 11;
	edges[23].v[0] = 11; edges[23].v[1] =  6;
	edges[24].v[0] =  6; edges[24].v[1] = 14;
	edges[25].v[0] = 15; edges[25].v[1] =  7;
	edges[26].v[0] =  1; edges[26].v[1] =  8;
	edges[27].v[0] = 18; edges[27].v[1] =  5;
	edges[28].v[0] =  6; edges[28].v[1] = 17;
	edges[29].v[0] = 11; edges[29].v[1] = 10;
	edges[30].v[0] = 19; edges[30].v[1] =  7;

	// all edges of a polygon go counter clockwise
	polys[0].numEdges = 5;
	polys[0].edges[0] = 1;
	polys[0].edges[1] = 2;
	polys[0].edges[2] = 3;
	polys[0].edges[3] = 4;
	polys[0].edges[4] = 5;

	polys[1].numEdges = 5;
	polys[1].edges[0] = -5;
	polys[1].edges[1] = 6;
	polys[1].edges[2] = 7;
	polys[1].edges[3] = 8;
	polys[1].edges[4] = 9;

	polys[2].numEdges = 5;
	polys[2].edges[0] = -8;
	polys[2].edges[1] = 10;
	polys[2].edges[2] = 11;
	polys[2].edges[3] = 12;
	polys[2].edges[4] = 13;

	polys[3].numEdges = 5;
	polys[3].edges[0] = 14;
	polys[3].edges[1] = 15;
	polys[3].edges[2] = 16;
	polys[3].edges[3] = 17;
	polys[3].edges[4] = -3;

	polys[4].numEdges = 5;
	polys[4].edges[0] = 18;
	polys[4].edges[1] = 19;
	polys[4].edges[2] = 20;
	polys[4].edges[3] = 21;
	polys[4].edges[4] = -12;

	polys[5].numEdges = 5;
	polys[5].edges[0] = 22;
	polys[5].edges[1] = 23;
	polys[5].edges[2] = 24;
	polys[5].edges[3] = -16;
	polys[5].edges[4] = 25;

	polys[6].numEdges = 5;
	polys[6].edges[0] = -9;
	polys[6].edges[1] = -13;
	polys[6].edges[2] = -21;
	polys[6].edges[3] = 26;
	polys[6].edges[4] = -1;

	polys[7].numEdges = 5;
	polys[7].edges[0] = -26;
	polys[7].edges[1] = -20;
	polys[7].edges[2] = 27;
	polys[7].edges[3] = -14;
	polys[7].edges[4] = -2;

	polys[8].numEdges = 5;
	polys[8].edges[0] = -4;
	polys[8].edges[1] = -17;
	polys[8].edges[2] = -24;
	polys[8].edges[3] = 28;
	polys[8].edges[4] = -6;

	polys[9].numEdges = 5;
	polys[9].edges[0] = -23;
	polys[9].edges[1] = 29;
	polys[9].edges[2] = -10;
	polys[9].edges[3] = -7;
	polys[9].edges[4] = -28;

	polys[10].numEdges = 5;
	polys[10].edges[0] = -25;
	polys[10].edges[1] = -15;
	polys[10].edges[2] = -27;
	polys[10].edges[3] = -19;
	polys[10].edges[4] = 30;

	polys[11].numEdges = 5;
	polys[11].edges[0] = -30;
	polys[11].edges[1] = -18;
	polys[11].edges[2] = -11;
	polys[11].edges[3] = -29;
	polys[11].edges[4] = -22;

	// convex model
	isConvex = true;
}

/*
============
idTraceModel::SetupCylinder
============
*/
void idTraceModel::SetupCylinder( const idBounds &cylBounds, const size_t numSides ) {
	int64 i = 0;

	int64 n = idMath::integer_cast<int64>(numSides);
	n = std::max<int64>(n, 3);
	if (idMath::integer_cast<size_t>(n * 2) > MAX_TRACEMODEL_VERTS ) {
		idLib::common->Printf( "WARNING: idTraceModel::SetupCylinder: too many vertices\n" );
		n = MAX_TRACEMODEL_VERTS / 2;
	}
	if (idMath::integer_cast<size_t>(n * 3) > MAX_TRACEMODEL_EDGES ) {
		idLib::common->Printf( "WARNING: idTraceModel::SetupCylinder: too many sides\n" );
		n = MAX_TRACEMODEL_EDGES / 3;
	}
	if (idMath::integer_cast<size_t>(n + 2) > MAX_TRACEMODEL_POLYS ) {
		idLib::common->Printf( "WARNING: idTraceModel::SetupCylinder: too many polygons\n" );
		n = MAX_TRACEMODEL_POLYS - 2;
	}

	type = TRM_CYLINDER;
	numVerts = idMath::integer_cast<size_t>(n * 2);
	numEdges = idMath::integer_cast<size_t>(n * 3);
	numPolys = idMath::integer_cast<size_t>(n + 2);
	offset = ( cylBounds[0] + cylBounds[1] ) * 0.5f;
	const idVec3 halfSize = cylBounds[1] - offset;
	for ( i = 0; i < n; i++ ) {
		// verts
		const float angle = idMath::TWO_PI * idMath::Itof<float>(i) / idMath::Itof<float>(n);
		verts[i].x = cos( angle ) * halfSize.x + offset.x;
		verts[i].y = sin( angle ) * halfSize.y + offset.y;
		verts[i].z = -halfSize.z + offset.z;
		verts[n+i].x = verts[i].x;
		verts[n+i].y = verts[i].y;
		verts[n+i].z = halfSize.z + offset.z;
		// edges
		const int64 ii = i + 1;
		const int64 n2 = n << 1;
		edges[ii].v[0] = i;
		edges[ii].v[1] = ii % n;
		edges[n+ii].v[0] = edges[ii].v[0] + n;
		edges[n+ii].v[1] = edges[ii].v[1] + n;
		edges[n2+ii].v[0] = i;
		edges[n2+ii].v[1] = n + i;
		// vertical polygon edges
		polys[i].numEdges = 4;
		polys[i].edges[0] = ii;
		polys[i].edges[1] = n2 + (ii % n) + 1;
		polys[i].edges[2] = -(n + ii);
		polys[i].edges[3] = -(n2 + ii);
		// bottom and top polygon edges
		polys[n].edges[i] = -(n - i);
		polys[n+1].edges[i] = n + ii;
	}
	// bottom and top polygon numEdges
	polys[n].numEdges = n;
	polys[n+1].numEdges = n;
	// polygons
	for ( i = 0; i < n; i++ ) {
		// vertical polygon plane
		polys[i].normal = (verts[(i+1)%n] - verts[i]).Cross( verts[n+i] - verts[i] );
		polys[i].normal.Normalize();
		polys[i].dist = polys[i].normal * verts[i];
		// vertical polygon bounds
		polys[i].bounds.Clear();
		polys[i].bounds.AddPoint( verts[i] );
		polys[i].bounds.AddPoint( verts[(i+1)%n] );
		polys[i].bounds[0][2] = -halfSize.z + offset.z;
		polys[i].bounds[1][2] = halfSize.z + offset.z;
	}
	// bottom and top polygon plane
	polys[n].normal.Set( 0.0f, 0.0f, -1.0f );
	polys[n].dist = -cylBounds[0][2];
	polys[n+1].normal.Set( 0.0f, 0.0f, 1.0f );
	polys[n+1].dist = cylBounds[1][2];
	// trm bounds
	bounds = cylBounds;
	// bottom and top polygon bounds
	polys[n].bounds = bounds;
	polys[n].bounds[1][2] = bounds[0][2];
	polys[n+1].bounds = bounds;
	polys[n+1].bounds[0][2] = bounds[1][2];
	// convex model
	isConvex = true;

	GenerateEdgeNormals();
}

/*
============
idTraceModel::SetupCylinder

  The origin is placed at the center of the cylinder.
============
*/
void idTraceModel::SetupCylinder( const float height, const float width, const size_t numSides ) {
	idBounds cylBounds;

	const float halfHeight = height * 0.5f;
	const float halfWidth = width * 0.5f;
	cylBounds[0].Set( -halfWidth, -halfWidth, -halfHeight );
	cylBounds[1].Set( halfWidth, halfWidth, halfHeight );
	SetupCylinder( cylBounds, numSides );
}

/*
============
idTraceModel::SetupCone
============
*/
void idTraceModel::SetupCone( const idBounds &coneBounds, const size_t numSides ) {
	int64 i = 0;

	int64 n = idMath::integer_cast<int64>(numSides);
	if ( n < 2 ) {
		n = 3;
	}
	if (idMath::integer_cast<size_t>(n + 1) > MAX_TRACEMODEL_VERTS ) {
		idLib::common->Printf( "WARNING: idTraceModel::SetupCone: too many vertices\n" );
		n = MAX_TRACEMODEL_VERTS - 1;
	}
	if (idMath::integer_cast<size_t>(n * 2) > MAX_TRACEMODEL_EDGES ) {
		idLib::common->Printf( "WARNING: idTraceModel::SetupCone: too many edges\n" );
		n = MAX_TRACEMODEL_EDGES / 2;
	}
	if (idMath::integer_cast<size_t>(n + 1) > MAX_TRACEMODEL_POLYS ) {
		idLib::common->Printf( "WARNING: idTraceModel::SetupCone: too many polygons\n" );
		n = MAX_TRACEMODEL_POLYS - 1;
	}

	type = TRM_CONE;
	numVerts = idMath::integer_cast<size_t>(n + 1);
	numEdges = idMath::integer_cast<size_t>(n * 2);
	numPolys = idMath::integer_cast<size_t>(n + 1);
	offset = ( coneBounds[0] + coneBounds[1] ) * 0.5f;
	const idVec3 halfSize = coneBounds[1] - offset;
	verts[n].Set( 0.0f, 0.0f, halfSize.z + offset.z );
	for ( i = 0; i < n; i++ ) {
		// verts
		const float angle = idMath::TWO_PI * idMath::Itof<float>(i) / idMath::Itof<float>(n);
		verts[i].x = cos( angle ) * halfSize.x + offset.x;
		verts[i].y = sin( angle ) * halfSize.y + offset.y;
		verts[i].z = -halfSize.z + offset.z;
		// edges
		const int64 ii = i + 1;
		edges[ii].v[0] = i;
		edges[ii].v[1] = ii % n;
		edges[n+ii].v[0] = i;
		edges[n+ii].v[1] = n;
		// vertical polygon edges
		polys[i].numEdges = 3;
		polys[i].edges[0] = ii;
		polys[i].edges[1] = n + (ii % n) + 1;
		polys[i].edges[2] = -(n + ii);
		// bottom polygon edges
		polys[n].edges[i] = -(n - i);
	}
	// bottom polygon numEdges
	polys[n].numEdges = n;

	// polygons
	for ( i = 0; i < n; i++ ) {
		// polygon plane
		polys[i].normal = (verts[(i+1)%n] - verts[i]).Cross( verts[n] - verts[i] );
		polys[i].normal.Normalize();
		polys[i].dist = polys[i].normal * verts[i];
		// polygon bounds
		polys[i].bounds.Clear();
		polys[i].bounds.AddPoint( verts[i] );
		polys[i].bounds.AddPoint( verts[(i+1)%n] );
		polys[i].bounds.AddPoint( verts[n] );
	}
	// bottom polygon plane
	polys[n].normal.Set( 0.0f, 0.0f, -1.0f );
	polys[n].dist = -coneBounds[0][2];
	// trm bounds
	bounds = coneBounds;
	// bottom polygon bounds
	polys[n].bounds = bounds;
	polys[n].bounds[1][2] = bounds[0][2];
	// convex model
	isConvex = true;

	GenerateEdgeNormals();
}

/*
============
idTraceModel::SetupCone

  The origin is placed at the apex of the cone.
============
*/
void idTraceModel::SetupCone( const float height, const float width, const size_t numSides ) {
	idBounds coneBounds;

	const float halfWidth = width * 0.5f;
	coneBounds[0].Set( -halfWidth, -halfWidth, -height );
	coneBounds[1].Set( halfWidth, halfWidth, 0.0f );
	SetupCone( coneBounds, numSides );
}

/*
============
idTraceModel::SetupBone

  The origin is placed at the center of the bone.
============
*/
void idTraceModel::SetupBone( const float length, const float width ) {
	const float halfLength = length * 0.5f;

	if ( type != TRM_BONE ) {
		InitBone();
	}
	// offset to center
	offset.Set( 0.0f, 0.0f, 0.0f );
	// set vertices
	verts[0].Set( 0.0f, 0.0f, -halfLength );
	verts[1].Set( 0.0f, width * -0.5f, 0.0f );
	verts[2].Set( width * 0.5f, width * 0.25f, 0.0f );
	verts[3].Set( width * -0.5f, width * 0.25f, 0.0f );
	verts[4].Set( 0.0f, 0.0f, halfLength );
	// set bounds
	bounds[0].Set( width * -0.5f, width * -0.5f, -halfLength );
	bounds[1].Set( width * 0.5f, width * 0.25f, halfLength );
	// poly plane normals
	polys[0].normal = ( verts[2] - verts[0] ).Cross( verts[1] - verts[0] );
	polys[0].normal.Normalize();
	polys[2].normal.Set( -polys[0].normal[0], polys[0].normal[1], polys[0].normal[2] );
	polys[3].normal.Set( polys[0].normal[0], polys[0].normal[1], -polys[0].normal[2] );
	polys[5].normal.Set( -polys[0].normal[0], polys[0].normal[1], -polys[0].normal[2] );
	polys[1].normal = (verts[3] - verts[0]).Cross(verts[2] - verts[0]);
	polys[1].normal.Normalize();
	polys[4].normal.Set( polys[1].normal[0], polys[1].normal[1], -polys[1].normal[2] );
	// poly plane distances
	for ( size_t i = 0; i < 6; i++ ) {
		polys[i].dist = polys[i].normal * verts[ edges[ abs(polys[i].edges[0]) ].v[0] ];
		polys[i].bounds.Clear();
		for ( size_t j = 0; j < 3; j++ ) {
			const int64 edgeNum = polys[i].edges[j];
			polys[i].bounds.AddPoint( verts[ edges[abs(edgeNum)].v[edgeNum < 0] ] );
		}
	}

	GenerateEdgeNormals();
}

/*
============
idTraceModel::InitBone

  Initialize size independent bone.
============
*/
void idTraceModel::InitBone() {
	type = TRM_BONE;
	numVerts = 5;
	numEdges = 9;
	numPolys = 6;

	// set bone edges
	for ( size_t i = 0; i < 3; i++ ) {
		edges[ i + 1 ].v[0] = 0;
		edges[ i + 1 ].v[1] = i + 1;
		edges[ i + 4 ].v[0] = 1 + i;
		edges[ i + 4 ].v[1] = 1 + ((i + 1) % 3);
		edges[ i + 7 ].v[0] = i + 1;
		edges[ i + 7 ].v[1] = 4;
	}

	// all edges of a polygon go counter clockwise
	polys[0].numEdges = 3;
	polys[0].edges[0] = 2;
	polys[0].edges[1] = -4;
	polys[0].edges[2] = -1;

	polys[1].numEdges = 3;
	polys[1].edges[0] = 3;
	polys[1].edges[1] = -5;
	polys[1].edges[2] = -2;

	polys[2].numEdges = 3;
	polys[2].edges[0] = 1;
	polys[2].edges[1] = -6;
	polys[2].edges[2] = -3;

	polys[3].numEdges = 3;
	polys[3].edges[0] = 4;
	polys[3].edges[1] = 8;
	polys[3].edges[2] = -7;

	polys[4].numEdges = 3;
	polys[4].edges[0] = 5;
	polys[4].edges[1] = 9;
	polys[4].edges[2] = -8;

	polys[5].numEdges = 3;
	polys[5].edges[0] = 6;
	polys[5].edges[1] = 7;
	polys[5].edges[2] = -9;

	// convex model
	isConvex = true;
}

/*
============
idTraceModel::SetupPolygon
============
*/
void idTraceModel::SetupPolygon( const idVec3 *v, const size_t count ) {
	int64 i = 0, j = 0;

	type = TRM_POLYGON;
	numVerts = count;
	// times three because we need to be able to turn the polygon into a volume
	if ( numVerts * 3 > MAX_TRACEMODEL_EDGES ) {
		idLib::common->Printf( "WARNING: idTraceModel::SetupPolygon: too many vertices\n" );
		numVerts = MAX_TRACEMODEL_EDGES / 3;
	}

	numEdges = numVerts;
	numPolys = 2;
	// set polygon planes
	polys[0].numEdges = numEdges;
	polys[0].normal = ( v[1] - v[0] ).Cross( v[2] - v[0] );
	polys[0].normal.Normalize();
	polys[0].dist = polys[0].normal * v[0];
	polys[1].numEdges = numEdges;
	polys[1].normal = -polys[0].normal;
	polys[1].dist = -polys[0].dist;
	// setup verts, edges and polygons
	polys[0].bounds.Clear();
	idVec3 mid = vec3_origin;
	for ( i = 0, j = 1; std::cmp_less(i, numVerts); i++, j++ ) {
		if (std::cmp_greater_equal(j, numVerts)) {
			j = 0;
		}
		verts[i] = v[i];
		edges[i+1].v[0] = i;
		edges[i+1].v[1] = j;
		edges[i+1].normal = polys[0].normal.Cross( v[i] - v[j] );
		edges[i+1].normal.Normalize();
		polys[0].edges[i] = i + 1;
		polys[1].edges[i] = -(idMath::integer_cast<int>(numVerts) - i);
		polys[0].bounds.AddPoint( verts[i] );
		mid += v[i];
	}
	polys[1].bounds = polys[0].bounds;
	// offset to center
	offset = mid * (1.0f / idMath::Itof<float>(numVerts));
	// total bounds
	bounds = polys[0].bounds;
	// considered non convex because the model has no volume
	isConvex = false;
}

/*
============
idTraceModel::SetupPolygon
============
*/
void idTraceModel::SetupPolygon( const idWinding &w ) {
	idVec3* verts = static_cast<idVec3*>(_alloca16(w.GetNumPoints() * sizeof( idVec3 )));
	for ( size_t i = 0; i < w.GetNumPoints(); i++ ) {
		verts[i] = w[i].ToVec3();
	}
	SetupPolygon( verts, w.GetNumPoints() );
}

/*
============
idTraceModel::VolumeFromPolygon
============
*/
void idTraceModel::VolumeFromPolygon( idTraceModel &trm, const float thickness ) const {
	trm = *this;
	trm.type = TRM_POLYGONVOLUME;
	trm.numVerts = numVerts * 2;
	trm.numEdges = numEdges * 3;
	trm.numPolys = numEdges + 2;
	for (size_t i = 0; i < numEdges; i++ ) {
		trm.verts[ numVerts + i ] = verts[i] - thickness * polys[0].normal;
		trm.edges[ numEdges + i + 1 ].v[0] = numVerts + i;
		trm.edges[ numEdges + i + 1 ].v[1] = numVerts + (i+1) % numVerts;
		trm.edges[ numEdges * 2 + i + 1 ].v[0] = i;
		trm.edges[ numEdges * 2 + i + 1 ].v[1] = numVerts + i;
		trm.polys[1].edges[i] = -idMath::integer_cast<int64>(numEdges + i + 1);
		trm.polys[2+i].numEdges = 4;
		trm.polys[2+i].edges[0] = -idMath::integer_cast<int64>(i + 1);
		trm.polys[2+i].edges[1] = idMath::integer_cast<int64>(numEdges*2 + i + 1);
		trm.polys[2+i].edges[2] = idMath::integer_cast<int64>(numEdges + i + 1);
		trm.polys[2+i].edges[3] = -idMath::integer_cast<int64>(numEdges*2 + (i+1) % numEdges + 1);
		trm.polys[2+i].normal = (verts[(i + 1) % numVerts] - verts[i]).Cross( polys[0].normal );
		trm.polys[2+i].normal.Normalize();
		trm.polys[2+i].dist = trm.polys[2+i].normal * verts[i];
	}
	trm.polys[1].dist = trm.polys[1].normal * trm.verts[ numEdges ];

	trm.GenerateEdgeNormals();
}

/*
============
idTraceModel::GenerateEdgeNormals
============
*/
constexpr auto SHARP_EDGE_DOT = -0.7f;

size_t idTraceModel::GenerateEdgeNormals() {
	size_t i = 0;

	for ( i = 0; i <= numEdges; i++ ) {
		edges[i].normal.Zero();
	}

	size_t numSharpEdges = 0;
	for ( i = 0; i < numPolys; i++ ) {
		const traceModelPoly_t* poly = polys + i;
		for ( size_t j = 0; j < poly->numEdges; j++ ) {
			const int64 edgeNum = poly->edges[j];
			traceModelEdge_t* edge = edges + abs(edgeNum);
			if ( edge->normal[0] == 0.0f && edge->normal[1] == 0.0f && edge->normal[2] == 0.0f ) {
				edge->normal = poly->normal;
			}
			else {
				const float dot = edge->normal * poly->normal;
				// if the two planes make a very sharp edge
				if ( dot < SHARP_EDGE_DOT ) {
					// max length normal pointing outside both polygons
					idVec3 dir = verts[edge->v[edgeNum > 0]] - verts[edge->v[edgeNum < 0]];
					edge->normal = edge->normal.Cross( dir ) + poly->normal.Cross( -dir );
					edge->normal *= ( 0.5f / ( 0.5f + 0.5f * SHARP_EDGE_DOT ) ) / edge->normal.Length();
					numSharpEdges++;
				}
				else {
					edge->normal = ( 0.5f / ( 0.5f + 0.5f * dot ) ) * ( edge->normal + poly->normal );
				}
			}
		}
	}
	return numSharpEdges;
}

/*
============
idTraceModel::Translate
============
*/
void idTraceModel::Translate( const idVec3 &translation ) {
	size_t i = 0;

	for ( i = 0; i < numVerts; i++ ) {
		verts[i] += translation;
	}
	for ( i = 0; i < numPolys; i++ ) {
		polys[i].dist += polys[i].normal * translation;
		polys[i].bounds[0] += translation;
		polys[i].bounds[1] += translation;
	}
	offset += translation;
	bounds[0] += translation;
	bounds[1] += translation;
}

/*
============
idTraceModel::Rotate
============
*/
void idTraceModel::Rotate( const idMat3 &rotation ) {
	size_t i = 0;

	for ( i = 0; i < numVerts; i++ ) {
		verts[i] *= rotation;
	}

	bounds.Clear();
	for ( i = 0; i < numPolys; i++ ) {
		polys[i].normal *= rotation;
		polys[i].bounds.Clear();
		int64 edgeNum = 0;
		for ( size_t j = 0; j < polys[i].numEdges; j++ ) {
			edgeNum = polys[i].edges[j];
			polys[i].bounds.AddPoint( verts[edges[abs(edgeNum)].v[INT64_SIGNBITSET(edgeNum)]] );
		}
		polys[i].dist = polys[i].normal * verts[edges[abs(edgeNum)].v[INT64_SIGNBITSET(edgeNum)]];
		bounds += polys[i].bounds;
	}

	GenerateEdgeNormals();
}

/*
============
idTraceModel::Shrink
============
*/
void idTraceModel::Shrink( const float m ) {
	size_t i = 0;
	int64 edgeNum = 0;
	const traceModelEdge_t *edge = nullptr;

	if ( type == TRM_POLYGON ) {
		for ( i = 0; i < numEdges; i++ ) {
			edgeNum = polys[0].edges[i];
			edge = &edges[abs(edgeNum)];
			idVec3 dir = verts[edge->v[INT64_SIGNBITSET(edgeNum)]] - verts[edge->v[INT64_SIGNBITNOTSET(edgeNum)]];
			if ( dir.Normalize() < 2.0f * m ) {
				continue;
			}
			dir *= m;
			verts[ edge->v[ 0 ] ] -= dir;
			verts[ edge->v[ 1 ] ] += dir;
		}
		return;
	}

	for ( i = 0; i < numPolys; i++ ) {
		polys[i].dist -= m;

		for ( size_t j = 0; j < polys[i].numEdges; j++ ) {
			edgeNum = polys[i].edges[j];
			edge = &edges[abs(edgeNum)];
			verts[ edge->v[ INT64_SIGNBITSET(edgeNum) ] ] -= polys[i].normal * m;
		}
	}
}

/*
============
idTraceModel::Compare
============
*/
bool idTraceModel::Compare( const idTraceModel &trm ) const {
	size_t i = 0;

	if ( type != trm.type || numVerts != trm.numVerts || 
			numEdges != trm.numEdges || numPolys != trm.numPolys ) {
		return false;
	}
	if ( bounds != trm.bounds || offset != trm.offset ) {
		return false;
	}

	switch( type ) {
		case TRM_INVALID:
		case TRM_BOX:
		case TRM_OCTAHEDRON:
		case TRM_DODECAHEDRON:
		case TRM_CYLINDER:
		case TRM_CONE:
			break;
		case TRM_BONE:
		case TRM_POLYGON:
		case TRM_POLYGONVOLUME:
		case TRM_CUSTOM:
			for ( i = 0; i < trm.numVerts; i++ ) {
				if ( verts[i] != trm.verts[i] ) {
					return false;
				}
			}
			break;
	}
	return true;
}

/*
============
idTraceModel::GetPolygonArea
============
*/
float idTraceModel::GetPolygonArea(const size_t polyNum ) const {
	if ( polyNum < 0 || polyNum >= numPolys ) {
		return 0.0f;
	}
	const traceModelPoly_t* poly = &polys[polyNum];
	float total = 0.0f;
	const idVec3 base = verts[edges[abs(poly->edges[0])].v[INT64_SIGNBITSET(poly->edges[0])]];
	for ( size_t i = 0; i < poly->numEdges; i++ ) {
		idVec3 v1 = verts[edges[abs(poly->edges[i])].v[INT64_SIGNBITSET(poly->edges[i])]] - base;
		idVec3 v2 = verts[edges[abs(poly->edges[i])].v[INT64_SIGNBITNOTSET(poly->edges[i])]] - base;
		idVec3 cross = v1.Cross(v2);
		total += cross.Length();
	}
	return total * 0.5f;
}

/*
============
idTraceModel::GetOrderedSilhouetteEdges
============
*/
size_t idTraceModel::GetOrderedSilhouetteEdges(const bool edgeIsSilEdge[MAX_TRACEMODEL_EDGES+1], int64 silEdges[MAX_TRACEMODEL_EDGES]) const {
	size_t i = 0, j = 0;
	int64 unsortedSilEdges[MAX_TRACEMODEL_EDGES] = {};

	size_t numSilEdges = 0;
	for ( i = 1; i <= numEdges; i++ ) {
		if ( edgeIsSilEdge[i] ) {
			unsortedSilEdges[numSilEdges++] = idMath::integer_cast<int64>(i);
		}
	}

	silEdges[0] = unsortedSilEdges[0];
	unsortedSilEdges[0] = -1;
	size_t nextSilVert = edges[silEdges[0]].v[0];
	for ( i = 1; i < numSilEdges; i++ ) {
		for ( j = 1; j < numSilEdges; j++ ) {
			const int64 edgeNum = unsortedSilEdges[j];
			if ( edgeNum >= 0 ) {
				if ( edges[edgeNum].v[0] == nextSilVert ) {
					nextSilVert = edges[edgeNum].v[1];
					silEdges[i] = edgeNum;
					break;
				}
				if ( edges[edgeNum].v[1] == nextSilVert ) {
					nextSilVert = edges[edgeNum].v[0];
					silEdges[i] = -edgeNum;
					break;
				}
			}
		}
		if ( j >= numSilEdges ) {
			silEdges[i] = 1;	// shouldn't happen
		}
		unsortedSilEdges[j] = -1;
	}
	return numSilEdges;
}

/*
============
idTraceModel::GetProjectionSilhouetteEdges
============
*/
size_t idTraceModel::GetProjectionSilhouetteEdges(const idVec3 &projectionOrigin, int64 silEdges[MAX_TRACEMODEL_EDGES]) const {
	bool edgeIsSilEdge[MAX_TRACEMODEL_EDGES+1];

	memset( edgeIsSilEdge, 0, sizeof( edgeIsSilEdge ) );

	for (size_t i = 0; i < numPolys; i++ ) {
		const traceModelPoly_t* poly = &polys[i];
		int64 edgeNum = poly->edges[0];
		idVec3 dir = verts[edges[abs(edgeNum)].v[INT64_SIGNBITSET(edgeNum)]] - projectionOrigin;
		if ( dir * poly->normal < 0.0f ) {
			for (size_t j = 0; j < poly->numEdges; j++ ) {
				edgeNum = poly->edges[j];
				edgeIsSilEdge[abs(edgeNum)] ^= 1;
			}
		}
	}

	return GetOrderedSilhouetteEdges( edgeIsSilEdge, silEdges );
}

/*
============
idTraceModel::GetParallelProjectionSilhouetteEdges
============
*/
size_t idTraceModel::GetParallelProjectionSilhouetteEdges( const idVec3 &projectionDir, int64 silEdges[MAX_TRACEMODEL_EDGES] ) const {
	bool edgeIsSilEdge[MAX_TRACEMODEL_EDGES+1];

	memset( edgeIsSilEdge, 0, sizeof( edgeIsSilEdge ) );

	for (size_t i = 0; i < numPolys; i++ ) {
		const traceModelPoly_t* poly = &polys[i];
		if ( projectionDir * poly->normal < 0.0f ) {
			for (size_t j = 0; j < poly->numEdges; j++ ) {
				const int64 edgeNum = poly->edges[j];
				edgeIsSilEdge[abs(edgeNum)] ^= 1;
			}
		}
	}

	return GetOrderedSilhouetteEdges( edgeIsSilEdge, silEdges );
}


/*

  credits to Brian Mirtich for his paper "Fast and Accurate Computation of Polyhedral Mass Properties"

*/

typedef struct projectionIntegrals_s {
	float P1 = 0.0f;
	float Pa = 0.0f, Pb = 0.0f;
	float Paa = 0.0f, Pab = 0.0f, Pbb = 0.0f;
	float Paaa = 0.0f, Paab = 0.0f, Pabb = 0.0f, Pbbb = 0.0f;
} projectionIntegrals_t;

/*
============
idTraceModel::ProjectionIntegrals
============
*/
void idTraceModel::ProjectionIntegrals( size_t polyNum, size_t a, size_t b, struct projectionIntegrals_s &integrals ) const {
	const traceModelPoly_t *poly;
	size_t i = 0;
	int64 edgeNum = 0;
	idVec3 v1, v2;
	float a0 = 0.0f, a1 = 0.0f, da = 0.0f;
	float b0 = 0.0f, b1 = 0.0f, db = 0.0f;
	float a0_2 = 0.0f, a0_3 = 0.0f, a0_4 = 0.0f, b0_2 = 0.0f, b0_3 = 0.0f, b0_4 = 0.0f;
	float a1_2 = 0.0f, a1_3 = 0.0f, b1_2 = 0.0f, b1_3 = 0.0f;
	float C1 = 0.0f, Ca = 0.0f, Caa = 0.0f, Caaa = 0.0f, Cb = 0.0f, Cbb = 0.0f, Cbbb = 0.0f;
	float Cab = 0.0f, Kab = 0.0f, Caab = 0.0f, Kaab = 0.0, Cabb = 0.0f, Kabb = 0.0f;

	memset(&integrals, 0, sizeof(projectionIntegrals_t));
	poly = &polys[polyNum];
	for ( i = 0; i < poly->numEdges; i++ ) {
		edgeNum = poly->edges[i];
		v1 = verts[ edges[ abs(edgeNum) ].v[ edgeNum < 0 ] ];
		v2 = verts[ edges[ abs(edgeNum) ].v[ edgeNum > 0 ] ];
		a0 = v1[a];
		b0 = v1[b];
		a1 = v2[a];
		b1 = v2[b];
		da = a1 - a0;
		db = b1 - b0;
		a0_2 = a0 * a0;
		a0_3 = a0_2 * a0;
		a0_4 = a0_3 * a0;
		b0_2 = b0 * b0;
		b0_3 = b0_2 * b0;
		b0_4 = b0_3 * b0;
		a1_2 = a1 * a1;
		a1_3 = a1_2 * a1; 
		b1_2 = b1 * b1;
		b1_3 = b1_2 * b1;

		C1 = a1 + a0;
		Ca = a1 * C1 + a0_2;
		Caa = a1 * Ca + a0_3;
		Caaa = a1 * Caa + a0_4;
		Cb = b1 * (b1 + b0) + b0_2;
		Cbb = b1 * Cb + b0_3;
		Cbbb = b1 * Cbb + b0_4;
		Cab = 3 * a1_2 + 2 * a1 * a0 + a0_2;
		Kab = a1_2 + 2 * a1 * a0 + 3 * a0_2;
		Caab = a0 * Cab + 4 * a1_3;
		Kaab = a1 * Kab + 4 * a0_3;
		Cabb = 4 * b1_3 + 3 * b1_2 * b0 + 2 * b1 * b0_2 + b0_3;
		Kabb = b1_3 + 2 * b1_2 * b0 + 3 * b1 * b0_2 + 4 * b0_3;

		integrals.P1 += db * C1;
		integrals.Pa += db * Ca;
		integrals.Paa += db * Caa;
		integrals.Paaa += db * Caaa;
		integrals.Pb += da * Cb;
		integrals.Pbb += da * Cbb;
		integrals.Pbbb += da * Cbbb;
		integrals.Pab += db * (b1 * Cab + b0 * Kab);
		integrals.Paab += db * (b1 * Caab + b0 * Kaab);
		integrals.Pabb += da * (a1 * Cabb + a0 * Kabb);
	}

	integrals.P1 *= (1.0f / 2.0f);
	integrals.Pa *= (1.0f / 6.0f);
	integrals.Paa *= (1.0f / 12.0f);
	integrals.Paaa *= (1.0f / 20.0f);
	integrals.Pb *= (1.0f / -6.0f);
	integrals.Pbb *= (1.0f / -12.0f);
	integrals.Pbbb *= (1.0f / -20.0f);
	integrals.Pab *= (1.0f / 24.0f);
	integrals.Paab *= (1.0f / 60.0f);
	integrals.Pabb *= (1.0f / -60.0f);
}

typedef struct polygonIntegrals_s {
	float Fa = 0.0f, Fb = 0.0f, Fc = 0.0f;
	float Faa = 0.0f, Fbb = 0.0f, Fcc = 0.0f;
	float Faaa = 0.0f, Fbbb = 0.0f, Fccc = 0.0f;
	float Faab = 0.0f, Fbbc = 0.0f, Fcca = 0.0f;
} polygonIntegrals_t;

/*
============
idTraceModel::PolygonIntegrals
============
*/
void idTraceModel::PolygonIntegrals(const size_t polyNum, const size_t a, const size_t b, const size_t c, struct polygonIntegrals_s &integrals ) const {
	projectionIntegrals_t pi;

	ProjectionIntegrals( polyNum, a, b, pi );

	idVec3 n = polys[polyNum].normal;
	const float w = -polys[polyNum].dist;
	const float k1 = 1.0f / n[c];
	const float k2 = k1 * k1;
	const float k3 = k2 * k1;
	const float k4 = k3 * k1;

	integrals.Fa = k1 * pi.Pa;
	integrals.Fb = k1 * pi.Pb;
	integrals.Fc = -k2 * (n[a] * pi.Pa + n[b] * pi.Pb + w * pi.P1);

	integrals.Faa = k1 * pi.Paa;
	integrals.Fbb = k1 * pi.Pbb;
	integrals.Fcc = k3 * (Square(n[a]) * pi.Paa + 2 * n[a] * n[b] * pi.Pab + Square(n[b]) * pi.Pbb
			+ w * (2 * (n[a] * pi.Pa + n[b] * pi.Pb) + w * pi.P1));

	integrals.Faaa = k1 * pi.Paaa;
	integrals.Fbbb = k1 * pi.Pbbb;
	integrals.Fccc = -k4 * (Cube(n[a]) * pi.Paaa + 3 * Square(n[a]) * n[b] * pi.Paab 
			+ 3 * n[a] * Square(n[b]) * pi.Pabb + Cube(n[b]) * pi.Pbbb
			+ 3 * w * (Square(n[a]) * pi.Paa + 2 * n[a] * n[b] * pi.Pab + Square(n[b]) * pi.Pbb)
			+ w * w * (3 * (n[a] * pi.Pa + n[b] * pi.Pb) + w * pi.P1));

	integrals.Faab = k1 * pi.Paab;
	integrals.Fbbc = -k2 * (n[a] * pi.Pabb + n[b] * pi.Pbbb + w * pi.Pbb);
	integrals.Fcca = k3 * (Square(n[a]) * pi.Paaa + 2 * n[a] * n[b] * pi.Paab + Square(n[b]) * pi.Pabb
			+ w * (2 * (n[a] * pi.Paa + n[b] * pi.Pab) + w * pi.Pa));
}

typedef struct volumeIntegrals_s {
	float T0 = 0.0f;
	idVec3 T1;
	idVec3 T2;
	idVec3 TP;
} volumeIntegrals_t;

/*
============
idTraceModel::VolumeIntegrals
============
*/
void idTraceModel::VolumeIntegrals( struct volumeIntegrals_s &integrals ) const {
	polygonIntegrals_t pi;
	size_t c = 0;

	memset( &integrals, 0, sizeof(volumeIntegrals_t) );
	for ( size_t i = 0; i < numPolys; i++ ) {
		const traceModelPoly_t* poly = &polys[i];

		const float nx = idMath::Fabs(poly->normal[0]);
		const float ny = idMath::Fabs(poly->normal[1]);
		const float nz = idMath::Fabs(poly->normal[2]);
		if ( nx > ny && nx > nz ) {
			c = 0;
		}
		else {
			c = (ny > nz) ? 1 : 2;
		}
		const size_t a = (c + 1) % 3;
		const size_t b = (a + 1) % 3;

		PolygonIntegrals( i, a, b, c, pi );

		integrals.T0 += poly->normal[0] * ((a == 0) ? pi.Fa : ((b == 0) ? pi.Fb : pi.Fc));

		integrals.T1[a] += poly->normal[a] * pi.Faa;
		integrals.T1[b] += poly->normal[b] * pi.Fbb;
		integrals.T1[c] += poly->normal[c] * pi.Fcc;
		integrals.T2[a] += poly->normal[a] * pi.Faaa;
		integrals.T2[b] += poly->normal[b] * pi.Fbbb;
		integrals.T2[c] += poly->normal[c] * pi.Fccc;
		integrals.TP[a] += poly->normal[a] * pi.Faab;
		integrals.TP[b] += poly->normal[b] * pi.Fbbc;
		integrals.TP[c] += poly->normal[c] * pi.Fcca;
	}

	integrals.T1 *= 0.5f;
	integrals.T2 *= (1.0f / 3.0f);
	integrals.TP *= 0.5f;
}

/*
============
idTraceModel::GetMassProperties
============
*/
void idTraceModel::GetMassProperties( const float density, float &mass, idVec3 &centerOfMass, idMat3 &inertiaTensor ) const {
	volumeIntegrals_t integrals;

	// if polygon trace model
	if ( type == TRM_POLYGON ) {
		idTraceModel trm;

		VolumeFromPolygon( trm, 1.0f );
		trm.GetMassProperties( density, mass, centerOfMass, inertiaTensor );
		return;
	}

	VolumeIntegrals( integrals );

	// if no volume
	if ( integrals.T0 == 0.0f ) {
		mass = 1.0f;
		centerOfMass.Zero();
		inertiaTensor.Identity();
		return;
	}

	// mass of model
	mass = density * integrals.T0;
	// center of mass
	centerOfMass = integrals.T1 / integrals.T0;
	// compute inertia tensor
	inertiaTensor[0][0] = density * (integrals.T2[1] + integrals.T2[2]);
	inertiaTensor[1][1] = density * (integrals.T2[2] + integrals.T2[0]);
	inertiaTensor[2][2] = density * (integrals.T2[0] + integrals.T2[1]);
	inertiaTensor[0][1] = inertiaTensor[1][0] = - density * integrals.TP[0];
	inertiaTensor[1][2] = inertiaTensor[2][1] = - density * integrals.TP[1];
	inertiaTensor[2][0] = inertiaTensor[0][2] = - density * integrals.TP[2];
	// translate inertia tensor to center of mass
	inertiaTensor[0][0] -= mass * (centerOfMass[1]*centerOfMass[1] + centerOfMass[2]*centerOfMass[2]);
	inertiaTensor[1][1] -= mass * (centerOfMass[2]*centerOfMass[2] + centerOfMass[0]*centerOfMass[0]);
	inertiaTensor[2][2] -= mass * (centerOfMass[0]*centerOfMass[0] + centerOfMass[1]*centerOfMass[1]);
	inertiaTensor[0][1] = inertiaTensor[1][0] += mass * centerOfMass[0] * centerOfMass[1];
	inertiaTensor[1][2] = inertiaTensor[2][1] += mass * centerOfMass[1] * centerOfMass[2];
	inertiaTensor[2][0] = inertiaTensor[0][2] += mass * centerOfMass[2] * centerOfMass[0];
}
