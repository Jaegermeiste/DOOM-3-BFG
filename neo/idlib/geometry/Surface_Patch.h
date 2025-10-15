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

#ifndef __SURFACE_PATCH_H__
#define __SURFACE_PATCH_H__

#pragma once

/*
===============================================================================

	Bezier patch surface.

===============================================================================
*/

class idSurface_Patch : public idSurface {

public:
						idSurface_Patch() noexcept;
						idSurface_Patch(size_t maxPatchWidth, size_t maxPatchHeight );
						idSurface_Patch( const idSurface_Patch &patch );
						~idSurface_Patch();

	void				SetSize(size_t patchWidth, size_t patchHeight );
	size_t				GetWidth() const;
	size_t				GetHeight() const;

						// subdivide the patch mesh based on error
	void				Subdivide( float maxHorizontalError, float maxVerticalError, float maxLength, bool genNormals = false );
						// subdivide the patch up to an explicit number of horizontal and vertical subdivisions
	void				SubdivideExplicit(size_t horzSubdivisions, size_t vertSubdivisions, bool genNormals, bool removeLinear = false );

protected:
	size_t				width;			// width of patch
	size_t				height;			// height of patch
	size_t				maxWidth;		// maximum width allocated for
	size_t				maxHeight;		// maximum height allocated for
	bool				expanded;		// true if vertices are spaced out

private:
						// put the approximation points on the curve
	void				PutOnCurve();
						// remove columns and rows with all points on one line
	void				RemoveLinearColumnsRows();
						// resize verts buffer
	void				ResizeExpanded(size_t height, size_t width );
						// space points out over maxWidth * maxHeight buffer
	void				Expand();
						// move all points to the start of the verts buffer
	void				Collapse();
						// project a point onto a vector to calculate maximum curve error
	void				ProjectPointOntoVector( const idVec3 &point, const idVec3 &vStart, const idVec3 &vEnd, idVec3 &vProj );
						// generate normals
	void				GenerateNormals();
						// generate triangle indexes
	void				GenerateIndexes();
						// lerp point from two patch point
	void				LerpVert( const idDrawVert &a, const idDrawVert &b, idDrawVert &out ) const;
						// sample a single 3x3 patch
	static void			SampleSinglePatchPoint( const idDrawVert ctrl[3][3], float u, float v, idDrawVert *out );
	void				SampleSinglePatch( const idDrawVert ctrl[3][3], size_t baseCol, size_t baseRow, size_t width, size_t horzSub, size_t vertSub, idDrawVert *outVerts ) const;
};

/*
=================
idSurface_Patch::idSurface_Patch
=================
*/
ID_INLINE idSurface_Patch::idSurface_Patch() noexcept {
	height = width = maxHeight = maxWidth = 0;
	expanded = false;
}

/*
=================
idSurface_Patch::idSurface_Patch
=================
*/
ID_INLINE idSurface_Patch::idSurface_Patch(const size_t maxPatchWidth, const size_t maxPatchHeight ) {
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum( maxWidth * maxHeight );
	expanded = false;
}

/*
=================
idSurface_Patch::idSurface_Patch
=================
*/
ID_INLINE idSurface_Patch::idSurface_Patch( const idSurface_Patch &patch ) {
	(*this) = patch;
}

/*
=================
idSurface_Patch::~idSurface_Patch
=================
*/
ID_INLINE idSurface_Patch::~idSurface_Patch() = default;

/*
=================
idSurface_Patch::GetWidth
=================
*/
ID_INLINE size_t idSurface_Patch::GetWidth() const {
	return width;
}

/*
=================
idSurface_Patch::GetHeight
=================
*/
ID_INLINE size_t idSurface_Patch::GetHeight() const {
	return height;
}

#endif /* !__SURFACE_PATCH_H__ */
