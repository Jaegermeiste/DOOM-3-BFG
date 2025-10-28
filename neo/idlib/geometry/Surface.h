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

#ifndef __SURFACE_H__
#define __SURFACE_H__

#pragma once

/*
===============================================================================

	Surface base class.

	A surface is tessellated to a triangle mesh with each edge shared by
	at most two triangles.

===============================================================================
*/

typedef struct surfaceEdge_s {
	index_t					verts[2];	// edge vertices always with ( verts[0] < verts[1] )
	index_t					tris[2];	// edge triangles
} surfaceEdge_t;


class idSurface {
public:
							idSurface() noexcept;
							explicit idSurface( const idSurface &surf );
							explicit idSurface( const idDrawVert *verts, const size_t numVerts, const index_t* indexes, const size_t numIndexes );
							~idSurface();

	
	const idDrawVert &		operator[]( const Ordinal auto index ) const;
	
	idDrawVert &			operator[]( const Ordinal auto index );
	idSurface &				operator+=( const idSurface &surf );

	[[nodiscard]] size_t				GetNumIndexes() const { return indexes.Num(); }
	[[nodiscard]] const index_t*		GetIndexes() const { return indexes.Ptr(); }
	[[nodiscard]] size_t				GetNumVertices() const { return verts.Num(); }
	[[nodiscard]] const idDrawVert *	GetVertices() const { return verts.Ptr(); }
	[[nodiscard]] const index_t*		GetEdgeIndexes() const { return edgeIndexes.Ptr(); }
	[[nodiscard]] const surfaceEdge_t *	GetEdges() const { return edges.Ptr(); }

	void					Clear();
	void					TranslateSelf( const idVec3 &translation );
	void					RotateSelf( const idMat3 &rotation );

							// splits the surface into a front and back surface, the surface itself stays unchanged
							// frontOnPlaneEdges and backOnPlaneEdges optionally store the indexes to the edges that lay on the split plane
							// returns a SIDE_?
	int						Split( const idPlane &plane, const float epsilon, idSurface **front, idSurface **back, index_t *frontOnPlaneEdges = nullptr, index_t *backOnPlaneEdges = nullptr) const;
							// cuts off the part at the back side of the plane, returns true if some part was at the front
							// if there is nothing at the front the number of points is set to zero
	bool					ClipInPlace( const idPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

							// returns true if each triangle can be reached from any other triangle by a traversal
	[[nodiscard]] bool		IsConnected() const;
							// returns true if the surface is closed
	[[nodiscard]] bool		IsClosed() const;
							// returns true if the surface is a convex hull
	[[nodiscard]] bool		IsPolytope( const float epsilon = 0.1f ) const;

	[[nodiscard]] float		PlaneDistance( const idPlane &plane ) const;
	[[nodiscard]] sides_e	PlaneSide( const idPlane &plane, const float epsilon = ON_EPSILON ) const;

							// returns true if the line intersects one of the surface triangles
	[[nodiscard]] bool		LineIntersection( const idVec3 &start, const idVec3 &end, bool backFaceCull = false ) const;
							// intersection point is start + dir * scale
	bool					RayIntersection( const idVec3 &start, const idVec3 &dir, float &scale, bool backFaceCull = false ) const;

protected:
	idList<idDrawVert, TAG_IDLIB_LIST_SURFACE>		verts;			// vertices
	idList<index_t, TAG_IDLIB_LIST_SURFACE>			indexes;		// 3 references to vertices for each triangle
	idList<surfaceEdge_t, TAG_IDLIB_LIST_SURFACE>	edges;			// edges
	idList<index_t, TAG_IDLIB_LIST_SURFACE>			edgeIndexes;	// 3 references to edges for each triangle, may be negative for reversed edge

protected:
	void					GenerateEdgeIndexes();
	[[nodiscard]] index_t	FindEdge( const Ordinal auto v1, const Ordinal auto v2 ) const;
};

/*
====================
idSurface::idSurface
====================
*/
ID_INLINE idSurface::idSurface() noexcept = default;

/*
=================
idSurface::idSurface
=================
*/
ID_INLINE idSurface::idSurface( const idDrawVert *verts, const size_t numVerts, const index_t* indexes, const size_t numIndexes ) {
	assert( verts != nullptr && indexes != NULL && numVerts > 0 && numIndexes > 0 );
	this->verts.SetNum( numVerts );
	memcpy( reinterpret_cast<void*>(this->verts.Ptr()), verts, numVerts * sizeof( verts[0] ) );
	this->indexes.SetNum( numIndexes );
	memcpy( this->indexes.Ptr(), indexes, numIndexes * sizeof( indexes[0] ) );
	GenerateEdgeIndexes();
}

/*
====================
idSurface::idSurface
====================
*/
ID_INLINE idSurface::idSurface( const idSurface &surf ) {
	this->verts = surf.verts;
	this->indexes = surf.indexes;
	this->edges = surf.edges;
	this->edgeIndexes = surf.edgeIndexes;
}

/*
====================
idSurface::~idSurface
====================
*/
ID_INLINE idSurface::~idSurface() = default;

/*
=================
idSurface::operator[]
=================
*/

ID_INLINE const idDrawVert &idSurface::operator[](const Ordinal auto index ) const {
	ORDINAL_CHECK(index, verts.Num());
	return verts[ index ];
};

/*
=================
idSurface::operator[]
=================
*/

ID_INLINE idDrawVert &idSurface::operator[](const Ordinal auto index ) {
	ORDINAL_CHECK(index, verts.Num());
	return verts[ index ];
};

/*
=================
idSurface::operator+=
=================
*/
ID_INLINE idSurface &idSurface::operator+=( const idSurface &surf ) {
	const auto n = verts.Num();
	const auto m = indexes.Num();

	const index_t n_idx = numeric_cast<index_t>(n);

	verts.Append( surf.verts );			// merge verts where possible ?
	indexes.Append( surf.indexes );
	for (size_t i = m; i < indexes.Num(); i++ ) {
		indexes[i] += n_idx;
	}
	GenerateEdgeIndexes();

	return *this;
}

/*
=================
idSurface::Clear
=================
*/
ID_INLINE void idSurface::Clear() {
	verts.Clear();
	indexes.Clear();
	edges.Clear();
	edgeIndexes.Clear();
}

/*
=================
idSurface::TranslateSelf
=================
*/
ID_INLINE void idSurface::TranslateSelf( const idVec3 &translation ) {
	for (size_t i = 0; i < verts.Num(); i++ ) {
		verts[i].xyz += translation;
	}
}

/*
=================
idSurface::RotateSelf
=================
*/
ID_INLINE void idSurface::RotateSelf( const idMat3 &rotation ) {
	for (size_t i = 0; i < verts.Num(); i++ ) {
		verts[i].xyz *= rotation;
		verts[i].SetNormal( verts[i].GetNormal() * rotation );
		verts[i].SetTangent( verts[i].GetTangent() * rotation );
	}
}

#endif /* !__SURFACE_H__ */
