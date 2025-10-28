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

#ifndef __SAVEGAME_H__
#define __SAVEGAME_H__

#pragma once

/*

Save game related helper classes.

*/

class idSaveGame {
public:
							idSaveGame( idFile *savefile, idFile *stringFile, int inVersion );
							~idSaveGame();

	void					Close();

	void					WriteDecls();
	
	void					AddObject( const idClass *obj );
	void					Resize( const int count ) { objects.Resize( count ); }
	void					WriteObjectList();

	void					Write( const void *buffer, size_t len ) const;
	void					WriteInt( const int value ) const;
	void					WriteInt( const std::integral auto &value) const;
	void					WriteJoint( const jointHandle_t value ) const;
	void					WriteShort( const short value ) const;
	void					WriteByte( const byte value ) const;
	void					WriteSignedChar( const signed char value ) const;
	void					WriteFloat( const float value ) const;
	void					WriteBool( const bool value ) const;
	void					WriteString( const char *string );
	void					WriteVec2( const idVec2 &vec ) const;
	void					WriteVec3( const idVec3 &vec ) const;
	void					WriteVec4( const idVec4 &vec ) const;
	void					WriteVec6( const idVec6 &vec ) const;
	void					WriteWinding( const idWinding &winding ) const;
	void					WriteBounds( const idBounds &bounds ) const;
	void					WriteMat3( const idMat3 &mat ) const;
	void					WriteAngles( const idAngles &angles ) const;
	void					WriteObject( const idClass *obj );
	void					WriteStaticObject( const idClass &obj );
	void					WriteDict( const idDict *dict );
	void					WriteMaterial( const idMaterial *material );
	void					WriteSkin( const idDeclSkin *skin );
	void					WriteParticle( const idDeclParticle *particle );
	void					WriteFX( const idDeclFX *fx );
	void					WriteSoundShader( const idSoundShader *shader );
	void					WriteModelDef( const class idDeclModelDef *modelDef );
	void					WriteModel( const idRenderModel *model );
	void					WriteUserInterface( const idUserInterface *ui, bool unique );
	void					WriteRenderEntity( const renderEntity_t &renderEntity );
	void					WriteRenderLight( const renderLight_t &renderLight );
	void					WriteRefSound( const refSound_t &refSound );
	void					WriteRenderView( const renderView_t &view );
	void					WriteUsercmd( const usercmd_t &usercmd );
	void					WriteContactInfo( const contactInfo_t &contactInfo );
	void					WriteTrace( const trace_t &trace );
	void					WriteTraceModel( const idTraceModel &trace );
	void					WriteClipModel( const class idClipModel *clipModel );
	void					WriteSoundCommands() const;

	void					WriteBuildNumber( const int value );

	int						GetBuildNumber() const { return version; }

	size_t					GetCurrentSaveSize() const { return file->Length(); }

private:
	idFile *				file;
	idFile *				stringFile;
	idCompressor *			compressor;

	idList<const idClass *>	objects;
	int						version;

	void					CallSave_r( const idTypeInfo *cls, const idClass *obj );

	struct stringTableIndex_s {
		idStr		string;
		size_t		offset;
	};

	idHashIndex						stringHash;
	idList< stringTableIndex_s >	stringTable;
	size_t							curStringTableOffset;

};

class idRestoreGame {
public:
							idRestoreGame( idFile * savefile, idFile * stringTableFile, int saveVersion );
							~idRestoreGame();

	void					ReadDecls();

	void					CreateObjects();
	void					RestoreObjects();
	void					DeleteObjects();

	void					Error( VERIFY_FORMAT_STRING const char *fmt, ... );

	void					Read( void *buffer, const size_t len ) const;
	void					ReadInt( int &value ) const;
	void					ReadInt( std::integral auto& value );
	void					ReadJoint( jointHandle_t &value ) const;
	void					ReadShort( short &value ) const;
	void					ReadShort( std::integral auto& value );
	void					ReadByte( byte &value ) const;
	void					ReadSignedChar( signed char &value ) const;
	void					ReadFloat( float &value ) const;
	void					ReadBool( bool &value ) const;
	void					ReadString( idStr &string );
	void					ReadVec2( idVec2 &vec ) const;
	void					ReadVec3( idVec3 &vec ) const;
	void					ReadVec4( idVec4 &vec ) const;
	void					ReadVec6( idVec6 &vec ) const;
	void					ReadWinding( idWinding &winding );
	void					ReadBounds( idBounds &bounds ) const;
	void					ReadMat3( idMat3 &mat ) const;
	void					ReadAngles( idAngles &angles ) const;
	void					ReadObject( idClass *&obj );
	void					ReadStaticObject( idClass &obj );
	void					ReadDict( idDict *dict );
	void					ReadMaterial( const idMaterial *&material );
	void					ReadSkin( const idDeclSkin *&skin );
	void					ReadParticle( const idDeclParticle *&particle );
	void					ReadFX( const idDeclFX *&fx );
	void					ReadSoundShader( const idSoundShader *&shader );
	void					ReadModelDef( const idDeclModelDef *&modelDef );
	void					ReadModel( idRenderModel *&model );
	void					ReadUserInterface( idUserInterface *&ui );
	void					ReadRenderEntity( renderEntity_t &renderEntity );
	void					ReadRenderLight( renderLight_t &renderLight );
	void					ReadRefSound( refSound_t &refSound );
	void					ReadRenderView( renderView_t &view );
	void					ReadUsercmd( usercmd_t &usercmd );
	void					ReadContactInfo( contactInfo_t &contactInfo );
	void					ReadTrace( trace_t &trace );
	void					ReadTraceModel( idTraceModel &trace );
	void					ReadClipModel( idClipModel *&clipModel );
	void					ReadSoundCommands() const;

	//						Used to retrieve the saved game buildNumber from within class Restore methods
	int64					GetBuildNumber() const { return version; }

private:
	idFile *		file;
	idFile *		stringFile;
	idList<idClass *, TAG_SAVEGAMES>		objects;
	int64					version;
	size_t					stringTableOffset;

	void					CallRestore_r( const idTypeInfo *cls, idClass *obj );
};

#endif /* !__SAVEGAME_H__*/
