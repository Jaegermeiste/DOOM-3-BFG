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

#include "precompiled.h"
#pragma hdrstop


/*
===============
FloatCRC
===============
*/
static ID_INLINE unsigned int FloatCRC( float f ) {
	return *reinterpret_cast<unsigned int*>(&f);
}

/*
===============
StringCRC
===============
*/
static ID_INLINE unsigned int StringCRC( const char *str ) {
	unsigned int crc = 0;
	//const unsigned char* ptr = reinterpret_cast<const unsigned char*>(str);
	for ( unsigned int i = 0; str[i]; i++ ) {
		crc ^= str[i] << (i & 3);
	}
	return crc;
}

/*
=================
ComputeAxisBase

WARNING : special case behavior of atan2(y,x) <-> atan(y/x) might not be the same everywhere when x == 0
rotation by (0,RotY,RotZ) assigns X to normal
=================
*/
static void ComputeAxisBase( const idVec3 &normal, idVec3 &texS, idVec3 &texT ) {
	idVec3 n = {};

	// do some cleaning
	n[0] = ( idMath::Fabs( normal[0] ) < 1e-6f ) ? 0.0f : normal[0];
	n[1] = ( idMath::Fabs( normal[1] ) < 1e-6f ) ? 0.0f : normal[1];
	n[2] = ( idMath::Fabs( normal[2] ) < 1e-6f ) ? 0.0f : normal[2];

	const float RotY = -atan2(n[2], idMath::Sqrt(n[1] * n[1] + n[0] * n[0]));
	const float RotZ = atan2(n[1], n[0]);
	// rotate (0,1,0) and (0,0,1) to compute texS and texT
	texS[0] = -sin(RotZ);
	texS[1] = cos(RotZ);
	texS[2] = 0;
	// the texT vector is along -Z ( T texture coordinates axis )
	texT[0] = -sin(RotY) * cos(RotZ);
	texT[1] = -sin(RotY) * sin(RotZ);
	texT[2] = -cos(RotY);
}

/*
=================
idMapBrushSide::GetTextureVectors
=================
*/
void idMapBrushSide::GetTextureVectors( idVec4 v[2] ) const {
	idVec3 texX, texY;

	ComputeAxisBase( plane.Normal(), texX, texY );
	for ( size_t i = 0; i < 2; i++ ) {
		v[i][0] = texX[0] * texMat[i][0] + texY[0] * texMat[i][1];
		v[i][1] = texX[1] * texMat[i][0] + texY[1] * texMat[i][1];
		v[i][2] = texX[2] * texMat[i][0] + texY[2] * texMat[i][1];
		v[i][3] = texMat[i][2] + ( origin * v[i].ToVec3() );
	}
}

/*
=================
idMapPatch::Parse
=================
*/
idMapPatch *idMapPatch::Parse( idLexer &src, const idVec3 &origin, const bool patchDef3, const float version ) {
	float		info[7];
	idToken		token;

	if ( !src.ExpectTokenString( "{" ) ) {
		return nullptr;
	}

	// read the material (we had an implicit 'textures/' in the old format...)
	if ( !src.ReadToken( &token ) ) {
		src.Error( "idMapPatch::Parse: unexpected EOF" );
		return nullptr;
	}

	// Parse it
	if (patchDef3) {
		if ( !src.Parse1DMatrix( 7, info ) ) {
			src.Error( "idMapPatch::Parse: unable to Parse patchDef3 info" );
			return nullptr;
		}
	} else {
		if ( !src.Parse1DMatrix( 5, info ) ) {
			src.Error( "idMapPatch::Parse: unable to parse patchDef2 info" );
			return nullptr;
		}
	}

	idMapPatch *patch = new (TAG_IDLIB) idMapPatch(numeric_cast<int>(info[0]), numeric_cast<int>(info[1]) );

	patch->SetSize(numeric_cast<int>(info[0]), numeric_cast<int>(info[1]));
	if ( version < 2.0f ) {
		patch->SetMaterial( "textures/" + token );
	} else {
		patch->SetMaterial( token );
	}

	if ( patchDef3 ) {
		patch->SetHorzSubdivisions(numeric_cast<int>(info[2]) );
		patch->SetVertSubdivisions(numeric_cast<int>(info[3]) );
		patch->SetExplicitlySubdivided( true );
	}

	if ( patch->GetWidth() < 0 || patch->GetHeight() < 0 ) {
		src.Error( "idMapPatch::Parse: bad size" );
		delete patch;
		return nullptr;
	}

	// these were written out in the wrong order, IMHO
	if ( !src.ExpectTokenString( "(" ) ) {
		src.Error( "idMapPatch::Parse: bad patch vertex data" );
		delete patch;
		return nullptr;
	}


	for ( size_t j = 0; j < patch->GetWidth(); j++ ) {
		if ( !src.ExpectTokenString( "(" ) ) {
			src.Error( "idMapPatch::Parse: bad vertex row data" );
			delete patch;
			return nullptr;
		}
		for (size_t i = 0; i < patch->GetHeight(); i++ ) {
			float v[5];

			if ( !src.Parse1DMatrix( 5, v ) ) {
				src.Error( "idMapPatch::Parse: bad vertex column data" );
				delete patch;
				return nullptr;
			}

			idDrawVert* vert = &((*patch)[i * patch->GetWidth() + j]);
			vert->xyz[0] = v[0] - origin[0];
			vert->xyz[1] = v[1] - origin[1];
			vert->xyz[2] = v[2] - origin[2];
			vert->SetTexCoord( v[3], v[4] );
		}
		if ( !src.ExpectTokenString( ")" ) ) {
			delete patch;
			src.Error( "idMapPatch::Parse: unable to parse patch control points" );
			return nullptr;
		}
	}

	if ( !src.ExpectTokenString( ")" ) ) {
		src.Error( "idMapPatch::Parse: unable to parse patch control points, no closure" );
		delete patch;
		return nullptr;
	}

	// read any key/value pairs
	while( src.ReadToken( &token ) ) {
		if ( token == "}" ) {
			src.ExpectTokenString( "}" );
			break;
		}
		if ( token.type == TT_STRING ) {
			idStr key = token;
			src.ExpectTokenType(TT_STRING, 0, &token );
			patch->epairs.Set( key, token );
		}
	}

	return patch;
}

/*
============
idMapPatch::Write
============
*/

bool idMapPatch::Write( idFile *fp, const Ordinal auto primitiveNum, const idVec3 &origin ) const {
	ORDINAL_CHECK(primitiveNum, UINT64_MAX);
	if ( GetExplicitlySubdivided() ) {
		fp->WriteFloatString( "// primitive %d\n{\n patchDef3\n {\n", primitiveNum );
		fp->WriteFloatString( "  \"%s\"\n  ( %d %d %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight(), GetHorzSubdivisions(), GetVertSubdivisions());
	} else {
		fp->WriteFloatString( "// primitive %d\n{\n patchDef2\n {\n", primitiveNum );
		fp->WriteFloatString( "  \"%s\"\n  ( %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight());
	}

	fp->WriteFloatString( "  (\n" );
	for (size_t i = 0; i < GetWidth(); i++ ) {
		fp->WriteFloatString( "   ( " );
		for (size_t j = 0; j < GetHeight(); j++ ) {
			const idDrawVert* v = &verts[j * GetWidth() + i];
			idVec2 st = v->GetTexCoord();
			fp->WriteFloatString( " ( %f %f %f %f %f )", v->xyz[0] + origin[0],
								v->xyz[1] + origin[1], v->xyz[2] + origin[2], st[0], st[1] );
		}
		fp->WriteFloatString( " )\n" );
	}
	fp->WriteFloatString( "  )\n }\n}\n" );

	return true;
}

/*
===============
idMapPatch::GetGeometryCRC
===============
*/
uint32 idMapPatch::GetGeometryCRC() const {
	uint32 crc = numeric_cast<uint32>(GetHorzSubdivisions()) ^ numeric_cast<uint32>(GetVertSubdivisions());
	for (size_t i = 0; i < GetWidth(); i++ ) {
		for (size_t j = 0; j < GetHeight(); j++ ) {
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.x );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.y );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.z );
		}
	}

	crc ^= StringCRC( GetMaterial() );

	return crc;
}

/*
=================
idMapBrush::Parse
=================
*/
idMapBrush *idMapBrush::Parse( idLexer &src, const idVec3 &origin, const bool newFormat, const float version ) {
	idVec3 planepts[3] = {{}, {}, {}};
	idToken token;
	idList<idMapBrushSide*> sides;
	idDict<> epairs;

	if ( !src.ExpectTokenString( "{" ) ) {
		return nullptr;
	}

	do {
		if ( !src.ReadToken( &token ) ) {
			src.Error( "idMapBrush::Parse: unexpected EOF" );
			sides.DeleteContents( true );
			return nullptr;
		}
		if ( token == "}" ) {
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do {
			// if token is a brace
			if ( token == "(" ) {
				break;
			}
			// the token should be a key string for a key/value pair
			if ( token.type != TT_STRING ) {
				src.Error( "idMapBrush::Parse: unexpected %s, expected ( or epair key string", token.c_str() );
				sides.DeleteContents( true );
				return nullptr;
			}

			idStr key = token;

			if ( !src.ReadTokenOnLine( &token ) || token.type != TT_STRING ) {
				src.Error( "idMapBrush::Parse: expected epair value string not found" );
				sides.DeleteContents( true );
				return nullptr;
			}

			epairs.Set( key, token );

			// try to read the next key
			if ( !src.ReadToken( &token ) ) {
				src.Error( "idMapBrush::Parse: unexpected EOF" );
				sides.DeleteContents( true );
				return nullptr;
			}
		} while (1);

		src.UnreadToken( &token );

		idMapBrushSide* side = new(TAG_IDLIB) idMapBrushSide();
		sides.Append(side);

		if ( newFormat ) {
			if ( !src.Parse1DMatrix( 4, side->plane.ToFloatPtr() ) ) {
				src.Error( "idMapBrush::Parse: unable to read brush side plane definition" );
				sides.DeleteContents( true );
				return nullptr;
			}
		} else {
			// read the three point plane definition
			if (!src.Parse1DMatrix( 3, planepts[0].ToFloatPtr() ) ||
				!src.Parse1DMatrix( 3, planepts[1].ToFloatPtr() ) ||
				!src.Parse1DMatrix( 3, planepts[2].ToFloatPtr() ) ) {
				src.Error( "idMapBrush::Parse: unable to read brush side plane definition" );
				sides.DeleteContents( true );
				return nullptr;
			}

			planepts[0] -= origin;
			planepts[1] -= origin;
			planepts[2] -= origin;

			side->plane.FromPoints( planepts[0], planepts[1], planepts[2] );
		}

		// read the texture matrix
		// this is odd, because the texmat is 2D relative to default planar texture axis
		if ( !src.Parse2DMatrix( 2, 3, side->texMat[0].ToFloatPtr() ) ) {
			src.Error( "idMapBrush::Parse: unable to read brush side texture matrix" );
			sides.DeleteContents( true );
			return nullptr;
		}
		side->origin = origin;
		
		// read the material
		if ( !src.ReadTokenOnLine( &token ) ) {
			src.Error( "idMapBrush::Parse: unable to read brush side material" );
			sides.DeleteContents( true );
			return nullptr;
		}

		// we had an implicit 'textures/' in the old format...
		if ( version < 2.0f ) {
			side->material = "textures/" + token;
		} else {
			side->material = token;
		}

		// Q2 allowed override of default flags and values, but we don't any more
		if ( src.ReadTokenOnLine( &token ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				if ( src.ReadTokenOnLine( &token ) ) {
				}
			}
		}
	} while( 1 );

	if ( !src.ExpectTokenString( "}" ) ) {
		sides.DeleteContents( true );
		return nullptr;
	}

	idMapBrush *brush = new (TAG_IDLIB) idMapBrush();
	for (size_t i = 0; i < sides.Num(); i++ ) {
		brush->AddSide( sides[i] );
	}

	brush->epairs = epairs;

	return brush;
}

/*
=================
idMapBrush::ParseQ3
=================
*/
idMapBrush *idMapBrush::ParseQ3( idLexer &src, const idVec3 &origin ) {
	int shift[2] = {};
	float scale[2] = {};
	idVec3 planepts[3] = {};
	idToken token;
	idList<idMapBrushSide*> sides;
	const idDict<> epairs;

	do {
		if ( src.CheckTokenString( "}" ) ) {
			break;
		}

		idMapBrushSide* side = new(TAG_IDLIB) idMapBrushSide();
		sides.Append( side );

		// read the three point plane definition
		if (!src.Parse1DMatrix( 3, planepts[0].ToFloatPtr() ) ||
			!src.Parse1DMatrix( 3, planepts[1].ToFloatPtr() ) ||
			!src.Parse1DMatrix( 3, planepts[2].ToFloatPtr() ) ) {
			src.Error( "idMapBrush::ParseQ3: unable to read brush side plane definition" );
			sides.DeleteContents( true );
			return nullptr;
		}

		planepts[0] -= origin;
		planepts[1] -= origin;
		planepts[2] -= origin;

		side->plane.FromPoints( planepts[0], planepts[1], planepts[2] );

		// read the material
		if ( !src.ReadTokenOnLine( &token ) ) {
			src.Error( "idMapBrush::ParseQ3: unable to read brush side material" );
			sides.DeleteContents( true );
			return nullptr;
		}

		// we have an implicit 'textures/' in the old format
		side->material = "textures/" + token;

		// read the texture shift, rotate and scale
		shift[0] = src.ParseInt();
		shift[1] = src.ParseInt();
		std::ignore = src.ParseInt();
		scale[0] = src.ParseFloat();
		scale[1] = src.ParseFloat();
		side->texMat[0] = idVec3( 0.03125f, 0.0f, 0.0f );
		side->texMat[1] = idVec3( 0.0f, 0.03125f, 0.0f );
		side->origin = origin;
		
		// Q2 allowed override of default flags and values, but we don't any more
		if ( src.ReadTokenOnLine( &token ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				if ( src.ReadTokenOnLine( &token ) ) {
				}
			}
		}
	} while( 1 );

	idMapBrush *brush = new (TAG_IDLIB) idMapBrush();
	for (size_t i = 0; i < sides.Num(); i++ ) {
		brush->AddSide( sides[i] );
	}

	brush->epairs = epairs;

	return brush;
}

/*
============
idMapBrush::Write
============
*/

bool idMapBrush::Write( idFile *fp, const Ordinal auto primitiveNum, const idVec3 &origin ) const {
	size_t i = 0;

	fp->WriteFloatString( "// primitive %d\n{\n brushDef3\n {\n", primitiveNum );

	// write brush epairs
	for ( i = 0; i < epairs.GetNumKeyVals(); i++) {
		fp->WriteFloatString( "  \"%s\" \"%s\"\n", epairs.GetKeyVal(i)->GetKey().c_str(), epairs.GetKeyVal(i)->GetValue().c_str());
	}

	// write brush sides
	for ( i = 0; i < GetNumSides(); i++ ) {
		idMapBrushSide* side = GetSide(i);
		fp->WriteFloatString( "  ( %f %f %f %f ) ", side->plane[0], side->plane[1], side->plane[2], side->plane[3] );
		fp->WriteFloatString( "( ( %f %f %f ) ( %f %f %f ) ) \"%s\" 0 0 0\n",
							side->texMat[0][0], side->texMat[0][1], side->texMat[0][2],
								side->texMat[1][0], side->texMat[1][1], side->texMat[1][2],
									side->material.c_str() );
	}

	fp->WriteFloatString( " }\n}\n" );

	return true;
}

/*
===============
idMapBrush::GetGeometryCRC
===============
*/
unsigned int idMapBrush::GetGeometryCRC() const {
	unsigned int crc = 0;
	for (size_t i = 0; i < GetNumSides(); i++ ) {
		const idMapBrushSide* mapSide = GetSide(i);
		for (size_t j = 0; j < 4; j++ ) {
			crc ^= FloatCRC( mapSide->GetPlane()[j] );
		}
		crc ^= StringCRC( mapSide->GetMaterial() );
	}

	return crc;
}

/*
================
idMapEntity::Parse
================
*/
idMapEntity *idMapEntity::Parse( idLexer &src, const bool worldSpawn, const float version ) {
	idToken	token;
	idMapBrush *mapBrush;
	idVec3 origin = {};
	double v1 = 0.0, v2 = 0.0, v3 = 0.0;

	if ( !src.ReadToken(&token) ) {
		return nullptr;
	}

	if ( token != "{" ) {
		src.Error( "idMapEntity::Parse: { not found, found %s", token.c_str() );
		return nullptr;
	}

	idMapEntity* mapEnt = new(TAG_IDLIB) idMapEntity();

	if ( worldSpawn ) {
		mapEnt->primitives.Resize( 1024, 256 );
	}

	origin.Zero();
	bool worldent = false;
	do {
		if ( !src.ReadToken(&token) ) {
			src.Error( "idMapEntity::Parse: EOF without closing brace" );
			return nullptr;
		}
		if ( token == "}" ) {
			break;
		}

		if ( token == "{" ) {
			// parse a brush or patch
			if ( !src.ReadToken( &token ) ) {
				src.Error( "idMapEntity::Parse: unexpected EOF" );
				return nullptr;
			}

			if ( worldent ) {
				origin.Zero();
			}

			// if is it a brush: brush, brushDef, brushDef2, brushDef3
			if ( token.Icmpn( "brush", 5 ) == 0 ) {
				mapBrush = idMapBrush::Parse( src, origin, ( !token.Icmp( "brushDef2" ) || !token.Icmp( "brushDef3" ) ), version );
				if ( !mapBrush ) {
					return nullptr;
				}
				mapEnt->AddPrimitive( mapBrush );
			}
			// if is it a patch: patchDef2, patchDef3
			else if ( token.Icmpn( "patch", 5 ) == 0 ) {
				idMapPatch* mapPatch = idMapPatch::Parse(src, origin, !token.Icmp("patchDef3"), version);
				if ( !mapPatch ) {
					return nullptr;
				}
				mapEnt->AddPrimitive( mapPatch );
			}
			// assume it's a brush in Q3 or older style
			else {
				src.UnreadToken( &token );
				mapBrush = idMapBrush::ParseQ3( src, origin );
				if ( !mapBrush ) {
					return nullptr;
				}
				mapEnt->AddPrimitive( mapBrush );
			}
		} else {
			idStr key, value;

			// parse a key / value pair
			key = token;
			src.ReadTokenOnLine( &token );
			value = token;

			// strip trailing spaces that sometimes get accidentally
			// added in the editor
			value.StripTrailingWhitespace();
			key.StripTrailingWhitespace();

			mapEnt->epairs.Set( key, value );

			if ( !idStr::Icmp( key, "origin" ) ) {
				// scanf into doubles, then assign, so it is idVec size independent
				v1 = v2 = v3 = 0;
				sscanf( value, "%lf %lf %lf", &v1, &v2, &v3 );
				origin.x = idMath::Dtof(v1);
				origin.y = idMath::Dtof(v2);
				origin.z = idMath::Dtof(v3);
			}
			else if ( !idStr::Icmp( key, "classname" ) && !idStr::Icmp( value, "worldspawn" ) ) {
				worldent = true;
			}
		}
	} while( 1 );

	return mapEnt;
}

/*
============
idMapEntity::Write
============
*/

bool idMapEntity::Write( idFile *fp, const Ordinal auto entityNum ) const {
	ORDINAL_CHECK(entityNum, UINT64_MAX);
	size_t i = 0;
	idVec3 origin;

	fp->WriteFloatString( "// entity %d\n{\n", entityNum );

	// write entity epairs
	for ( i = 0; i < epairs.GetNumKeyVals(); i++) {
		fp->WriteFloatString( "\"%s\" \"%s\"\n", epairs.GetKeyVal(i)->GetKey().c_str(), epairs.GetKeyVal(i)->GetValue().c_str());
	}

	epairs.GetVector( "origin", "0 0 0", origin );

	// write primitives
	for ( i = 0; i < GetNumPrimitives(); i++ ) {
		idMapPrimitive* mapPrim = GetPrimitive(i);

		if (mapPrim)
		{
			switch (mapPrim->GetType()) {
			case idMapPrimitive::TYPE_BRUSH:
				dynamic_cast<idMapBrush*>(mapPrim)->Write(fp, i, origin);
				break;
			case idMapPrimitive::TYPE_PATCH:
				dynamic_cast<idMapPatch*>(mapPrim)->Write(fp, i, origin);
				break;
			case idMapPrimitive::TYPE_INVALID:
			default:
				break;
			}
		}
	}

	fp->WriteFloatString( "}\n" );

	return true;
}

/*
===============
idMapEntity::RemovePrimitiveData
===============
*/
void idMapEntity::RemovePrimitiveData() {
	primitives.DeleteContents(true);
}

/*
===============
idMapEntity::GetGeometryCRC
===============
*/
unsigned int idMapEntity::GetGeometryCRC() const {
	unsigned int crc = 0;
	for (size_t i = 0; i < GetNumPrimitives(); i++ ) {
		idMapPrimitive* mapPrim = GetPrimitive(i);

		if (mapPrim)
		{
			switch (mapPrim->GetType()) {
			case idMapPrimitive::TYPE_BRUSH:
				crc ^= dynamic_cast<idMapBrush*>(mapPrim)->GetGeometryCRC();
				break;
			case idMapPrimitive::TYPE_PATCH:
				crc ^= dynamic_cast<idMapPatch*>(mapPrim)->GetGeometryCRC();
				break;
			case idMapPrimitive::TYPE_INVALID:
			default:
				break;
			}
		}
	}

	return crc;
}

/*
===============
idMapFile::Parse
===============
*/
bool idMapFile::Parse( const char *filename, const bool ignoreRegion, const bool osPath ) {
	// no string concatenation for epairs and allow path names for materials
	idLexer src( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
	idToken token;
	idMapEntity *mapEnt = nullptr;

	name = filename;
	name.StripFileExtension();
	idStr fullName = name;
	hasPrimitiveData = false;

	if ( !ignoreRegion ) {
		// try loading a .reg file first
		fullName.SetFileExtension( "reg" );
		src.LoadFile( fullName, osPath );
	}

	if ( !src.IsLoaded() ) {
		// now try a .map file
		fullName.SetFileExtension( "map" );
		src.LoadFile( fullName, osPath );
		if ( !src.IsLoaded() ) {
			// didn't get anything at all
			return false;
		}
	}

	version = OLD_MAP_VERSION;
	fileTime = src.GetFileTime();
	entities.DeleteContents( true );

	if ( src.CheckTokenString( "Version" ) ) {
		src.ReadTokenOnLine( &token );
		version = token.GetFloatValue();
	}

	while( true ) {
		mapEnt = idMapEntity::Parse( src, ( entities.Num() == 0 ), version );
		if ( !mapEnt ) {
			break;
		}

		if (mapEnt)
		{
			entities.Append(mapEnt);
		}
	}

	SetGeometryCRC();

	// if the map has a worldspawn
	if ( entities.Num() ) {
		size_t i = 0;

		// "removeEntities" "classname" can be set in the worldspawn to remove all entities with the given classname
		const idKeyValue<> *removeEntities = entities[0]->epairs.MatchPrefix( "removeEntities", nullptr);
		while ( removeEntities ) {
			RemoveEntities( removeEntities->GetValue() );
			removeEntities = entities[0]->epairs.MatchPrefix( "removeEntities", removeEntities );
		}

		// "overrideMaterial" "material" can be set in the worldspawn to reset all materials
		idStr material = {};
		if ( entities[0]->epairs.GetString( "overrideMaterial", "", material ) ) {
			for ( i = 0; i < entities.Num(); i++ ) {
				mapEnt = entities[i];

				if (mapEnt)
				{
					for (size_t j = 0; j < mapEnt->GetNumPrimitives(); j++) {
						idMapPrimitive* mapPrimitive = mapEnt->GetPrimitive(j);
						switch (mapPrimitive->GetType()) {
						case idMapPrimitive::TYPE_BRUSH: {
							const idMapBrush* mapBrush = dynamic_cast<idMapBrush*>(mapPrimitive);
							for (size_t k = 0; k < mapBrush->GetNumSides(); k++) {
								mapBrush->GetSide(k)->SetMaterial(material);
							}
							break;
						}
						case idMapPrimitive::TYPE_PATCH: {
							dynamic_cast<idMapPatch*>(mapPrimitive)->SetMaterial(material);
							break;
						}
						case idMapPrimitive::TYPE_INVALID:
						default:
							break;
						}
					}
				}
			}
		}

		// force all entities to have a name key/value pair
		if ( entities[0]->epairs.GetBool( "forceEntityNames" ) ) {
			for ( i = 1; i < entities.Num(); i++ ) {
				mapEnt = entities[i];
				if (mapEnt)
				{
					if (!mapEnt->epairs.FindKey("name")) {
						mapEnt->epairs.Set("name", va("%s%d", mapEnt->epairs.GetString("classname", "forcedName"), i));
					}
				}
			}
		}

		// move the primitives of any func_group entities to the worldspawn
		if ( entities[0]->epairs.GetBool( "moveFuncGroups" ) ) {
			for ( i = 1; i < entities.Num(); i++ ) {
				mapEnt = entities[i];
				if (mapEnt)
				{
					if (idStr::Icmp(mapEnt->epairs.GetString("classname"), "func_group") == 0) {
						entities[0]->primitives.Append(mapEnt->primitives);
						mapEnt->primitives.Clear();
					}
				}
			}
		}
	}

	hasPrimitiveData = true;
	return true;
}

/*
============
idMapFile::Write
============
*/
bool idMapFile::Write( const char *fileName, const char *ext, const bool fromBasePath ) {
	idStr qpath;
	idFile *fp = nullptr;

	qpath = fileName;
	qpath.SetFileExtension( ext );

	idLib::common->Printf( "writing %s...\n", qpath.c_str() );

	if ( fromBasePath ) {
		fp = idLib::fileSystem->OpenFileWrite( qpath, "fs_basepath" );
	}
	else {
		fp = idLib::fileSystem->OpenExplicitFileWrite( qpath );
	}

	if ( !fp ) {
		idLib::common->Warning( "Couldn't open %s\n", qpath.c_str() );
		return false;
	}

	fp->WriteFloatString( "Version %f\n", static_cast<float>(CURRENT_MAP_VERSION) );

	for ( size_t i = 0; i < entities.Num(); i++ ) {
		entities[i]->Write( fp, i );
	}

	idLib::fileSystem->CloseFile( fp );

	return true;
}

/*
===============
idMapFile::SetGeometryCRC
===============
*/
void idMapFile::SetGeometryCRC() {
	geometryCRC = 0;
	for ( size_t i = 0; i < entities.Num(); i++ ) {
		geometryCRC ^= entities[i]->GetGeometryCRC();
	}
}

/*
===============
idMapFile::AddEntity
===============
*/
size_t idMapFile::AddEntity( idMapEntity *mapEnt ) {
	const size_t ret = entities.Append( mapEnt );
	return ret;
}

/*
===============
idMapFile::FindEntity
===============
*/
idMapEntity *idMapFile::FindEntity( const char *name ) {
	for ( size_t i = 0; i < entities.Num(); i++ ) {
		idMapEntity *ent = entities[i];
		if ( idStr::Icmp( ent->epairs.GetString( "name" ), name ) == 0 ) {
			return ent;
		}
	}
	return nullptr;
}

/*
===============
idMapFile::RemoveEntity
===============
*/
void idMapFile::RemoveEntity( idMapEntity *mapEnt ) {
	entities.Remove( mapEnt );
	delete mapEnt;
}

/*
===============
idMapFile::RemoveEntity
===============
*/
void idMapFile::RemoveEntities( const char *classname ) {
	for ( size_t i = 0; i < entities.Num(); i++ ) {
		const idMapEntity *ent = entities[i];
		if ( idStr::Icmp( ent->epairs.GetString( "classname" ), classname ) == 0 ) {
			delete entities[i];
			entities.RemoveIndex( i );
			i--;
		}
	}
}

/*
===============
idMapFile::RemoveAllEntities
===============
*/
void idMapFile::RemoveAllEntities() {
	entities.DeleteContents( true );
	hasPrimitiveData = false;
}

/*
===============
idMapFile::RemovePrimitiveData
===============
*/
void idMapFile::RemovePrimitiveData() {
	for ( size_t i = 0; i < entities.Num(); i++ ) {
		idMapEntity *ent = entities[i];
		ent->RemovePrimitiveData();
	}
	hasPrimitiveData = false;
}

/*
===============
idMapFile::NeedsReload
===============
*/
bool idMapFile::NeedsReload() {
	if ( name.Length() ) {
		ID_TIME_T time = FILE_NOT_FOUND_TIMESTAMP;
		if ( idLib::fileSystem->ReadFile( name, nullptr, &time ) > 0 ) {
			return ( time > fileTime );
		}
	}
	return true;
}
