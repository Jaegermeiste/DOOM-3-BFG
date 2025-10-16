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

#ifndef __MAPFILE_H__
#define __MAPFILE_H__

#pragma once

/*
===============================================================================

	Reads or writes the contents of .map files into a standard internal
	format, which can then be moved into private formats for collision
	detection, map processing, or editor use.

	No validation (duplicate planes, null area brushes, etc) is performed.
	There are no limits to the number of any of the elements in maps.
	The order of entities, brushes, and sides is maintained.

===============================================================================
*/

constexpr int OLD_MAP_VERSION					= 1;
constexpr int CURRENT_MAP_VERSION				= 2;
constexpr int DEFAULT_CURVE_SUBDIVISION			= 4;
constexpr float DEFAULT_CURVE_MAX_ERROR			= 4.0f;
constexpr float DEFAULT_CURVE_MAX_ERROR_CD		= 24.0f;
constexpr float DEFAULT_CURVE_MAX_LENGTH		= -1.0f;
constexpr float DEFAULT_CURVE_MAX_LENGTH_CD		= -1.0f;


class idMapPrimitive {
public:
	typedef enum mapPrimitiveType_e : int8 { TYPE_INVALID = -1, TYPE_BRUSH, TYPE_PATCH } mapPrimitiveType_t;

	idDict					epairs;

							idMapPrimitive() noexcept { type = TYPE_INVALID; }
	virtual					~idMapPrimitive() = default;
	[[nodiscard]] mapPrimitiveType_t GetType() const { return type; }

protected:
	mapPrimitiveType_t		type;
};


class idMapBrushSide {
	friend class idMapBrush;

public:
							idMapBrushSide() noexcept;
							~idMapBrushSide() = default;
	[[nodiscard]] const char *	GetMaterial() const { return material; }
	void					SetMaterial( const char *p ) { material = p; }
	[[nodiscard]] const idPlane &	GetPlane() const { return plane; }
	void					SetPlane( const idPlane &p ) { plane = p; }
	void					SetTextureMatrix( const idVec3 mat[2] ) { texMat[0] = mat[0]; texMat[1] = mat[1]; }
	void					GetTextureMatrix( idVec3 &mat1, idVec3 &mat2 ) const
	{ mat1 = texMat[0]; mat2 = texMat[1]; }
	void					GetTextureVectors( idVec4 v[2] ) const;

protected:
	idStr					material;
	idPlane					plane;
	idVec3					texMat[2];
	idVec3					origin;
};

ID_INLINE idMapBrushSide::idMapBrushSide() noexcept {
	plane.Zero();
	texMat[0].Zero();
	texMat[1].Zero();
	origin.Zero();
}


class idMapBrush : public idMapPrimitive {
public:
							idMapBrush() noexcept { type = TYPE_BRUSH; sides.Resize( 8, 4 ); }
							~idMapBrush() override { sides.DeleteContents( true ); }
	static idMapBrush *		Parse( idLexer &src, const idVec3 &origin, bool newFormat = true, float version = CURRENT_MAP_VERSION );
	static idMapBrush *		ParseQ3( idLexer &src, const idVec3 &origin );
	
	bool					Write( idFile *fp, Ordinal auto primitiveNum, const idVec3 &origin ) const;
	[[nodiscard]] size_t	GetNumSides() const { return sides.Num(); }
	size_t					AddSide( idMapBrushSide *side ) { return sides.Append( side ); }

	[[nodiscard]] idMapBrushSide* GetSide(const Ordinal auto i) const { ORDINAL_CHECK(i, sides.Num());  return sides[i]; }
	[[nodiscard]] unsigned int	GetGeometryCRC() const;

protected:
	idList<idMapBrushSide*, TAG_IDLIB_LIST_MAP> sides;
};


class idMapPatch : public idMapPrimitive, public idSurface_Patch {
public:
							idMapPatch() noexcept;
							idMapPatch(size_t maxPatchWidth, size_t maxPatchHeight );
							~idMapPatch() override = default;
	static idMapPatch *		Parse( idLexer &src, const idVec3 &origin, bool patchDef3 = true, float version = CURRENT_MAP_VERSION );
	
	bool					Write( idFile *fp, Ordinal auto primitiveNum, const idVec3 &origin ) const;
	[[nodiscard]] const char *	GetMaterial() const { return material; }
	void					SetMaterial( const char *p ) { material = p; }
	[[nodiscard]] size_t	GetHorzSubdivisions() const { return horzSubdivisions; }
	[[nodiscard]] size_t	GetVertSubdivisions() const { return vertSubdivisions; }
	[[nodiscard]] bool		GetExplicitlySubdivided() const { return explicitSubdivisions; }
	void					SetHorzSubdivisions(const size_t n ) { horzSubdivisions = n; }
	void					SetVertSubdivisions(const size_t n ) { vertSubdivisions = n; }
	void					SetExplicitlySubdivided(const bool b ) { explicitSubdivisions = b; }
	[[nodiscard]] unsigned int			GetGeometryCRC() const;

protected:
	idStr					material;
	size_t					horzSubdivisions;
	size_t					vertSubdivisions;
	bool					explicitSubdivisions;
};

ID_INLINE idMapPatch::idMapPatch() noexcept {
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxHeight = 0;
	expanded = false;
}

ID_INLINE idMapPatch::idMapPatch(const size_t maxPatchWidth, const size_t maxPatchHeight ) {
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum( maxWidth * maxHeight );
	expanded = false;
}


class idMapEntity {
	friend class			idMapFile;

public:
	idDict					epairs;

public:
							idMapEntity() noexcept { epairs.SetHashSize( 64 ); }
							~idMapEntity() { primitives.DeleteContents( true ); }
	static idMapEntity *	Parse( idLexer &src, bool worldSpawn = false, float version = CURRENT_MAP_VERSION );
	
	bool					Write( idFile *fp, Ordinal auto entityNum ) const;
	[[nodiscard]] size_t	GetNumPrimitives() const { return primitives.Num(); }

	[[nodiscard]] idMapPrimitive*  GetPrimitive( const Ordinal auto i ) const { ORDINAL_CHECK(i, primitives.Num()); return primitives[i]; }
	void					AddPrimitive( idMapPrimitive *p ) { primitives.Append( p ); }
	[[nodiscard]] unsigned int	GetGeometryCRC() const;
	void					RemovePrimitiveData();

protected:
	idList<idMapPrimitive*, TAG_IDLIB_LIST_MAP>	primitives;
};


class idMapFile {
public:
							idMapFile() noexcept;
							~idMapFile() { entities.DeleteContents( true ); }

							// filename does not require an extension
							// normally this will use a .reg file instead of a .map file if it exists,
							// which is what the game and dmap want, but the editor will want to always
							// load a .map file
	bool					Parse( const char *filename, bool ignoreRegion = false, bool osPath = false );
	bool					Write( const char *fileName, const char *ext, bool fromBasePath = true );
							// get the number of entities in the map
	[[nodiscard]] size_t	GetNumEntities() const { return entities.Num(); }
							// get the specified entity
	idMapEntity*            GetEntity( const Ordinal auto i ) const { ORDINAL_CHECK(i, entities.Num());  return entities[i]; }
							// get the name without file extension
	[[nodiscard]] const char *	GetName() const { return name; }
							// get the file time
	ID_TIME_T				GetFileTime() const { return fileTime; }
							// get CRC for the map geometry
							// texture coordinates and entity key/value pairs are not taken into account
	[[nodiscard]] unsigned int	GetGeometryCRC() const { return geometryCRC; }
							// returns true if the file on disk changed
	bool					NeedsReload();

	size_t					AddEntity( idMapEntity *mapentity );
	idMapEntity *			FindEntity( const char *name );
	void					RemoveEntity( idMapEntity *mapEnt );
	void					RemoveEntities( const char *classname );
	void					RemoveAllEntities();
	void					RemovePrimitiveData();
	[[nodiscard]] bool					HasPrimitiveData() const { return hasPrimitiveData; }

protected:
	float					version;
	ID_TIME_T				fileTime;
	unsigned int			geometryCRC;
	idList<idMapEntity *, TAG_IDLIB_LIST_MAP>	entities;
	idStr					name;
	bool					hasPrimitiveData;

private:
	void					SetGeometryCRC();
};

ID_INLINE idMapFile::idMapFile() noexcept {
	version = CURRENT_MAP_VERSION;
	fileTime = 0;
	geometryCRC = 0;
	entities.Resize( 1024, 256 );
	hasPrimitiveData = false;
}

#endif /* !__MAPFILE_H__ */
