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


#include "../Game_local.h"

CLASS_DECLARATION( idPhysics, idPhysics_StaticMulti )
END_CLASS

staticPState_t defaultState;
staticInterpolatePState_t defaultInterpolateState;

/*
================
idPhysics_StaticMulti::idPhysics_StaticMulti
================
*/
idPhysics_StaticMulti::idPhysics_StaticMulti() {
	self = nullptr;
	hasMaster = false;
	isOrientated = false;

	defaultState.origin.Zero();
	defaultState.axis.Identity();
	defaultState.localOrigin.Zero();
	defaultState.localAxis.Identity();

	defaultInterpolateState.origin.Zero();
	defaultInterpolateState.axis = defaultState.axis.ToQuat();
	defaultInterpolateState.localAxis = defaultInterpolateState.axis;
	defaultInterpolateState.localOrigin.Zero();

	current.SetNum( 1 );
	current[0] = defaultState;
	previous.SetNum( 1 );
	previous[0] = defaultInterpolateState;
	next.SetNum( 1 );
	next[0] = defaultInterpolateState;
	clipModels.SetNum( 1 );
	clipModels[0] = nullptr;
}

/*
================
idPhysics_StaticMulti::~idPhysics_StaticMulti
================
*/
idPhysics_StaticMulti::~idPhysics_StaticMulti() {
	if ( self && self->GetPhysics() == this ) {
		self->SetPhysics(nullptr);
	}
	idForce::DeletePhysics( this );
	for ( size_t i = 0; i < clipModels.Num(); i++ ) {
		delete clipModels[i];
	}
}

/*
================
idPhysics_StaticMulti::Save
================
*/
void idPhysics_StaticMulti::Save( idSaveGame *savefile ) const {
	size_t i = 0;

	savefile->WriteObject( self );

	savefile->WriteInt(current.Num());
	for  ( i = 0; i < current.Num(); i++ ) {
		savefile->WriteVec3( current[i].origin );
		savefile->WriteMat3( current[i].axis );
		savefile->WriteVec3( current[i].localOrigin );
		savefile->WriteMat3( current[i].localAxis );
	}

	savefile->WriteInt( clipModels.Num() );
	for ( i = 0; i < clipModels.Num(); i++ ) {
		savefile->WriteClipModel( clipModels[i] );
	}

	savefile->WriteBool(hasMaster);
	savefile->WriteBool(isOrientated);
}

/*
================
idPhysics_StaticMulti::Restore
================
*/
void idPhysics_StaticMulti::Restore( idRestoreGame *savefile ) {
	size_t i = 0, num = 0;

	savefile->ReadObject( reinterpret_cast<idClass *&>( self ) );

	savefile->ReadInt(num);
	current.AssureSize( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadVec3( current[i].origin );
		savefile->ReadMat3( current[i].axis );
		savefile->ReadVec3( current[i].localOrigin );
		savefile->ReadMat3( current[i].localAxis );
	}

	savefile->ReadInt(num);
	clipModels.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadClipModel( clipModels[i] );
	}

	savefile->ReadBool(hasMaster);
	savefile->ReadBool(isOrientated);
}

/*
================
idPhysics_StaticMulti::SetSelf
================
*/
void idPhysics_StaticMulti::SetSelf( idEntity *e ) {
	assert( e );
	self = e;
}

/*
================
idPhysics_StaticMulti::RemoveIndex
================
*/
void idPhysics_StaticMulti::RemoveIndex( const index_t id, const bool freeClipModel ) {
	if ( id < 0 || std::cmp_greater_equal(id, clipModels.Num()) ) {
		return;
	}
	if ( clipModels[id] && freeClipModel ) {
		delete clipModels[id];
		clipModels[id] = nullptr;
	}
	clipModels.RemoveIndex( id );
	current.RemoveIndex( id );
}

/*
================
idPhysics_StaticMulti::SetClipModel
================
*/
void idPhysics_StaticMulti::SetClipModel( idClipModel *model, float density, const index_t id, const bool freeOld ) {
	size_t i = 0;

	assert( self );

	if ( std::cmp_greater_equal(id, clipModels.Num()) ) {
		current.AssureSize( id+1, defaultState );
		clipModels.AssureSize( id+1, nullptr);
	}

	if ( clipModels[id] && clipModels[id] != model && freeOld ) {
		delete clipModels[id];
	}
	clipModels[id] = model;
	if ( clipModels[id] ) {
		clipModels[id]->Link( gameLocal.clip, self, id, current[id].origin, current[id].axis );

	}

	for ( i = clipModels.Num() - 1; i >= 1; i-- ) {
		if ( clipModels[i] ) {
			break;
		}
	}
	current.SetNum( i+1 );
	clipModels.SetNum( i+1 );

	// Assure that on first setup, our next/previous is the same as current. 
	previous.SetNum( current.Num() );
	next.SetNum( previous.Num() );
	for( size_t curIdx = 0; curIdx < current.Num(); curIdx++ ) {
		previous[curIdx] = ConvertPStateToInterpolateState( current[curIdx] );
		previous[curIdx] = next[curIdx];
	}
}

/*
================
idPhysics_StaticMulti::GetClipModel
================
*/
idClipModel *idPhysics_StaticMulti::GetClipModel( const index_t id ) const {
	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) && clipModels[id] ) {
		return clipModels[id];
	}
	return gameLocal.clip.DefaultClipModel();
}

/*
================
idPhysics_StaticMulti::GetNumClipModels
================
*/
size_t idPhysics_StaticMulti::GetNumClipModels() const {
	return clipModels.Num();
}

/*
================
idPhysics_StaticMulti::SetMass
================
*/
void idPhysics_StaticMulti::SetMass( float mass, index_t id ) {
}

/*
================
idPhysics_StaticMulti::GetMass
================
*/
float idPhysics_StaticMulti::GetMass( index_t id ) const {
	return 0.0f;
}

/*
================
idPhysics_StaticMulti::SetContents
================
*/
void idPhysics_StaticMulti::SetContents(const int contents, const index_t id ) {
	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) ) {
		if ( clipModels[id] ) {
			clipModels[id]->SetContents( contents );
		}
	} else if ( id == -1 ) {
		for ( size_t i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				clipModels[i]->SetContents( contents );
			}
		}
	}
}

/*
================
idPhysics_StaticMulti::GetContents
================
*/
int idPhysics_StaticMulti::GetContents( const index_t id ) const {
	int contents = 0;

	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) ) {
		if ( clipModels[id] ) {
			contents = clipModels[id]->GetContents();
		}
	} else if ( id == -1 ) {
		for ( size_t i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				contents |= clipModels[i]->GetContents();
			}
		}
	}
	return contents;
}

/*
================
idPhysics_StaticMulti::SetClipMask
================
*/
void idPhysics_StaticMulti::SetClipMask( int mask, const index_t id ) {
}

/*
================
idPhysics_StaticMulti::GetClipMask
================
*/
int idPhysics_StaticMulti::GetClipMask( const index_t id ) const {
	return 0;
}

/*
================
idPhysics_StaticMulti::GetBounds
================
*/
const idBounds &idPhysics_StaticMulti::GetBounds( const index_t id ) const {
	static idBounds bounds = {};

	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) ) {
		if ( clipModels[id] ) {
			return clipModels[id]->GetBounds();
		}
	}
	if ( id == -1 ) {
		size_t i = 0;
		bounds.Clear();
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				bounds.AddBounds( clipModels[i]->GetAbsBounds() );
			}
		}
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				bounds[0] -= clipModels[i]->GetOrigin();
				bounds[1] -= clipModels[i]->GetOrigin();
				break;
			}
		}
		return bounds;
	}
	return bounds_zero;
}

/*
================
idPhysics_StaticMulti::GetAbsBounds
================
*/
const idBounds &idPhysics_StaticMulti::GetAbsBounds( const index_t id ) const {
	static idBounds absBounds;

	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) ) {
		if ( clipModels[id] ) {
			return clipModels[id]->GetAbsBounds();
		}
	}
	if ( id == -1 ) {
		absBounds.Clear();
		for ( size_t i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				absBounds.AddBounds( clipModels[i]->GetAbsBounds() );
			}
		}
		return absBounds;
	}
	return bounds_zero;
}

/*
================
idPhysics_StaticMulti::Evaluate
================
*/
bool idPhysics_StaticMulti::Evaluate( const ID_TIME_T timeStepMSec, ID_TIME_T endTimeMSec ) {
	idVec3 masterOrigin = {};
	idMat3 masterAxis = {};

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		for ( size_t i = 0; i < clipModels.Num(); i++ ) {
			current[i].origin = masterOrigin + current[i].localOrigin * masterAxis;
			if ( isOrientated ) {
				current[i].axis = current[i].localAxis * masterAxis;
			} else {
				current[i].axis = current[i].localAxis;
			}
			if ( clipModels[i] ) {
				clipModels[i]->Link( gameLocal.clip, self, i, current[i].origin, current[i].axis );
			}
		}

		// FIXME: return false if master did not move
		return true;
	}
	return false;
}

/*
================
idPhysics_StaticMulti::Interpolate
================
*/
bool idPhysics_StaticMulti::Interpolate( const double fraction ) {
	// If the sizes don't match, just use the latest version.
	// TODO: This might cause visual snapping, is there a better solution?
	if ( current.Num() != previous.Num() ||
		 current.Num() != next.Num() ) {
		current.SetNum( next.Num() );
		for ( size_t i = 0; i < next.Num(); ++i ) {
			current[i] = InterpolateStaticPState( next[i], next[i], 1.0f );
		}
		return true;
	}

	for ( size_t i = 0; i < current.Num(); ++i ) {
		current[i] = InterpolateStaticPState( previous[i], next[i], fraction );
	}

	return true;
}

/*
================
idPhysics_StaticMulti::UpdateTime
================
*/
void idPhysics_StaticMulti::UpdateTime( ID_TIME_T endTimeMSec ) {
}

/*
================
idPhysics_StaticMulti::GetTime
================
*/
index_t idPhysics_StaticMulti::GetTime() const {
	return 0;
}

/*
================
idPhysics_StaticMulti::GetImpactInfo
================
*/
void idPhysics_StaticMulti::GetImpactInfo( const index_t id, const idVec3 &point, impactInfo_t *info ) const {
	memset( static_cast<void*>(info), 0, sizeof( *info ) );
}

/*
================
idPhysics_StaticMulti::ApplyImpulse
================
*/
void idPhysics_StaticMulti::ApplyImpulse( const index_t id, const idVec3 &point, const idVec3 &impulse ) {
}

/*
================
idPhysics_StaticMulti::AddForce
================
*/
void idPhysics_StaticMulti::AddForce( const index_t id, const idVec3 &point, const idVec3 &force ) {
}

/*
================
idPhysics_StaticMulti::Activate
================
*/
void idPhysics_StaticMulti::Activate() {
}

/*
================
idPhysics_StaticMulti::PutToRest
================
*/
void idPhysics_StaticMulti::PutToRest() {
}

/*
================
idPhysics_StaticMulti::IsAtRest
================
*/
bool idPhysics_StaticMulti::IsAtRest() const {
	return true;
}

/*
================
idPhysics_StaticMulti::GetRestStartTime
================
*/
ID_TIME_T idPhysics_StaticMulti::GetRestStartTime() const {
	return 0;
}

/*
================
idPhysics_StaticMulti::IsPushable
================
*/
bool idPhysics_StaticMulti::IsPushable() const {
	return false;
}

/*
================
idPhysics_StaticMulti::SaveState
================
*/
void idPhysics_StaticMulti::SaveState() {
}

/*
================
idPhysics_StaticMulti::RestoreState
================
*/
void idPhysics_StaticMulti::RestoreState() {
}

/*
================
idPhysics_StaticMulti::SetOrigin
================
*/
void idPhysics_StaticMulti::SetOrigin( const idVec3 &newOrigin, const index_t id ) {
	idVec3 masterOrigin = {};
	idMat3 masterAxis = {};

	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) ) {
		current[id].localOrigin = newOrigin;
		if ( hasMaster ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			current[id].origin = masterOrigin + newOrigin * masterAxis;
		} else {
			current[id].origin = newOrigin;
		}
		if ( clipModels[id] ) {
			clipModels[id]->Link( gameLocal.clip, self, id, current[id].origin, current[id].axis );
		}
	} else if ( id == -1 ) {
		if ( hasMaster ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			Translate( masterOrigin + masterAxis * newOrigin - current[0].origin );
		} else {
			Translate( newOrigin - current[0].origin );
		}
	}
}

/*
================
idPhysics_StaticMulti::SetAxis
================
*/
void idPhysics_StaticMulti::SetAxis( const idMat3 &newAxis, const index_t id ) {
	idVec3 masterOrigin = {};
	idMat3 masterAxis = {};

	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) ) {
		current[id].localAxis = newAxis;
		if ( hasMaster && isOrientated ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			current[id].axis = newAxis * masterAxis;
		} else {
			current[id].axis = newAxis;
		}
		if ( clipModels[id] ) {
			clipModels[id]->Link( gameLocal.clip, self, id, current[id].origin, current[id].axis );
		}
	} else if ( id == -1 ) {
		idMat3 axis = {};

		if ( hasMaster ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			axis = current[0].axis.Transpose() * ( newAxis * masterAxis );
		} else {
			axis = current[0].axis.Transpose() * newAxis;
		}
		idRotation rotation = axis.ToRotation();
		rotation.SetOrigin( current[0].origin );

		Rotate( rotation );
	}
}

/*
================
idPhysics_StaticMulti::Translate
================
*/
void idPhysics_StaticMulti::Translate( const idVec3 &translation, const index_t id ) {
	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) ) {
		current[id].localOrigin += translation;
		current[id].origin += translation;

		if ( clipModels[id] ) {
			clipModels[id]->Link( gameLocal.clip, self, id, current[id].origin, current[id].axis );
		}
	} else if ( id == -1 ) {
		for ( size_t i = 0; i < clipModels.Num(); i++ ) {
			current[i].localOrigin += translation;
			current[i].origin += translation;

			if ( clipModels[i] ) {
				clipModels[i]->Link( gameLocal.clip, self, i, current[i].origin, current[i].axis );
			}
		}
	}
}

/*
================
idPhysics_StaticMulti::Rotate
================
*/
void idPhysics_StaticMulti::Rotate( const idRotation &rotation, const index_t id ) {
	idVec3 masterOrigin = {};
	idMat3 masterAxis = {};

	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) ) {
		current[id].origin *= rotation;
		current[id].axis *= rotation.ToMat3();

		if ( hasMaster ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			current[id].localAxis *= rotation.ToMat3();
			current[id].localOrigin = ( current[id].origin - masterOrigin ) * masterAxis.Transpose();
		} else {
			current[id].localAxis = current[id].axis;
			current[id].localOrigin = current[id].origin;
		}

		if ( clipModels[id] ) {
			clipModels[id]->Link( gameLocal.clip, self, id, current[id].origin, current[id].axis );
		}
	} else if ( id == -1 ) {
		for ( size_t i = 0; i < clipModels.Num(); i++ ) {
			current[i].origin *= rotation;
			current[i].axis *= rotation.ToMat3();

			if ( hasMaster ) {
				self->GetMasterPosition( masterOrigin, masterAxis );
				current[i].localAxis *= rotation.ToMat3();
				current[i].localOrigin = ( current[i].origin - masterOrigin ) * masterAxis.Transpose();
			} else {
				current[i].localAxis = current[i].axis;
				current[i].localOrigin = current[i].origin;
			}

			if ( clipModels[i] ) {
				clipModels[i]->Link( gameLocal.clip, self, i, current[i].origin, current[i].axis );
			}
		}
	}
}

/*
================
idPhysics_StaticMulti::GetOrigin
================
*/
const idVec3 &idPhysics_StaticMulti::GetOrigin( const index_t id ) const {
	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) ) {
		return current[id].origin;
	}
	if ( clipModels.Num() ) {
		return current[0].origin;
	} else {
		return vec3_origin;
	}
}

/*
================
idPhysics_StaticMulti::GetAxis
================
*/
const idMat3 &idPhysics_StaticMulti::GetAxis( const index_t id ) const {
	if ( id >= 0 && std::cmp_less(id, clipModels.Num()) ) {
		return current[id].axis;
	}
	if ( clipModels.Num() ) {
		return current[0].axis;
	} else {
		return mat3_identity;
	}
}

/*
================
idPhysics_StaticMulti::SetLinearVelocity
================
*/
void idPhysics_StaticMulti::SetLinearVelocity( const idVec3 &newLinearVelocity, const index_t id ) {
}

/*
================
idPhysics_StaticMulti::SetAngularVelocity
================
*/
void idPhysics_StaticMulti::SetAngularVelocity( const idVec3 &newAngularVelocity, const index_t id ) {
}

/*
================
idPhysics_StaticMulti::GetLinearVelocity
================
*/
const idVec3 &idPhysics_StaticMulti::GetLinearVelocity( const index_t id ) const {
	return vec3_origin;
}

/*
================
idPhysics_StaticMulti::GetAngularVelocity
================
*/
const idVec3 &idPhysics_StaticMulti::GetAngularVelocity( const index_t id ) const {
	return vec3_origin;
}

/*
================
idPhysics_StaticMulti::SetGravity
================
*/
void idPhysics_StaticMulti::SetGravity( const idVec3 &newGravity ) {
}

/*
================
idPhysics_StaticMulti::GetGravity
================
*/
const idVec3 &idPhysics_StaticMulti::GetGravity() const {
	static idVec3 gravity( 0, 0, -g_gravity.GetFloat() );
	return gravity;
}

/*
================
idPhysics_StaticMulti::GetGravityNormal
================
*/
const idVec3 &idPhysics_StaticMulti::GetGravityNormal() const {
	static idVec3 gravity( 0, 0, -1 );
	return gravity;
}

/*
================
idPhysics_StaticMulti::ClipTranslation
================
*/
void idPhysics_StaticMulti::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	memset( static_cast<void*>(&results), 0, sizeof( trace_t ) );
	gameLocal.Warning( "idPhysics_StaticMulti::ClipTranslation called" );
}

/*
================
idPhysics_StaticMulti::ClipRotation
================
*/
void idPhysics_StaticMulti::ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const {
	memset(static_cast<void*>(&results), 0, sizeof( trace_t ) );
	gameLocal.Warning( "idPhysics_StaticMulti::ClipRotation called" );
}

/*
================
idPhysics_StaticMulti::ClipContents
================
*/
int idPhysics_StaticMulti::ClipContents( const idClipModel *model ) const {
	int contents = 0;

	for ( size_t i = 0; i < clipModels.Num(); i++ ) {
		if ( clipModels[i] ) {
			if ( model ) {
				contents |= gameLocal.clip.ContentsModel( clipModels[i]->GetOrigin(), clipModels[i], clipModels[i]->GetAxis(), -1,
											model->Handle(), model->GetOrigin(), model->GetAxis() );
			} else {
				contents |= gameLocal.clip.Contents( clipModels[i]->GetOrigin(), clipModels[i], clipModels[i]->GetAxis(), -1, nullptr);
			}
		}
	}
	return contents;
}

/*
================
idPhysics_StaticMulti::DisableClip
================
*/
void idPhysics_StaticMulti::DisableClip() {
	for ( size_t i = 0; i < clipModels.Num(); i++ ) {
        if ( clipModels[i] ) {
			clipModels[i]->Disable();
		}
	}
}

/*
================
idPhysics_StaticMulti::EnableClip
================
*/
void idPhysics_StaticMulti::EnableClip() {
	for ( size_t i = 0; i < clipModels.Num(); i++ ) {
		if ( clipModels[i] ) {
			clipModels[i]->Enable();
		}
	}
}

/*
================
idPhysics_StaticMulti::UnlinkClip
================
*/
void idPhysics_StaticMulti::UnlinkClip() {
	for ( size_t i = 0; i < clipModels.Num(); i++ ) {
        if ( clipModels[i] ) {
			clipModels[i]->Unlink();
		}
	}
}

/*
================
idPhysics_StaticMulti::LinkClip
================
*/
void idPhysics_StaticMulti::LinkClip() {
	for ( size_t i = 0; i < clipModels.Num(); i++ ) {
		if ( clipModels[i] ) {
			clipModels[i]->Link( gameLocal.clip, self, i, current[i].origin, current[i].axis );
		}
	}
}

/*
================
idPhysics_StaticMulti::EvaluateContacts
================
*/
bool idPhysics_StaticMulti::EvaluateContacts() {
	return false;
}

/*
================
idPhysics_StaticMulti::GetNumContacts
================
*/
size_t idPhysics_StaticMulti::GetNumContacts() const {
	return 0;
}

/*
================
idPhysics_StaticMulti::GetContact
================
*/
const contactInfo_t &idPhysics_StaticMulti::GetContact( const index_t num ) const {
	static contactInfo_t info = {};
	memset( static_cast<void*>(&info), 0, sizeof( info ) );
	return info;
}

/*
================
idPhysics_StaticMulti::ClearContacts
================
*/
void idPhysics_StaticMulti::ClearContacts() {
}

/*
================
idPhysics_StaticMulti::AddContactEntity
================
*/
void idPhysics_StaticMulti::AddContactEntity( idEntity *e ) {
}

/*
================
idPhysics_StaticMulti::RemoveContactEntity
================
*/
void idPhysics_StaticMulti::RemoveContactEntity( idEntity *e ) {
}

/*
================
idPhysics_StaticMulti::HasGroundContacts
================
*/
bool idPhysics_StaticMulti::HasGroundContacts() const {
	return false;
}

/*
================
idPhysics_StaticMulti::IsGroundEntity
================
*/
bool idPhysics_StaticMulti::IsGroundEntity( const index_t entityNum ) const {
	return false;
}

/*
================
idPhysics_StaticMulti::IsGroundClipModel
================
*/
bool idPhysics_StaticMulti::IsGroundClipModel( const index_t entityNum, const index_t id ) const {
	return false;
}

/*
================
idPhysics_StaticMulti::SetPushed
================
*/
void idPhysics_StaticMulti::SetPushed( ID_TIME_T deltaTime ) {
}

/*
================
idPhysics_StaticMulti::GetPushedLinearVelocity
================
*/
const idVec3 &idPhysics_StaticMulti::GetPushedLinearVelocity( const index_t id ) const {
	return vec3_origin;
}

/*
================
idPhysics_StaticMulti::GetPushedAngularVelocity
================
*/
const idVec3 &idPhysics_StaticMulti::GetPushedAngularVelocity( const index_t id ) const {
	return vec3_origin;
}

/*
================
idPhysics_StaticMulti::SetMaster
================
*/
void idPhysics_StaticMulti::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin = {};
	idMat3 masterAxis = {};

	if ( master ) {
		if ( !hasMaster ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			for ( size_t i = 0; i < clipModels.Num(); i++ ) {
                current[i].localOrigin = ( current[i].origin - masterOrigin ) * masterAxis.Transpose();
				if ( orientated ) {
					current[i].localAxis = current[i].axis * masterAxis.Transpose();
				} else {
					current[i].localAxis = current[i].axis;
				}
			}
			hasMaster = true;
			isOrientated = orientated;
		}
	} else {
		if ( hasMaster ) {
			hasMaster = false;
		}
	}
}

/*
================
idPhysics_StaticMulti::GetBlockingInfo
================
*/
const trace_t *idPhysics_StaticMulti::GetBlockingInfo() const {
	return nullptr;
}

/*
================
idPhysics_StaticMulti::GetBlockingEntity
================
*/
idEntity *idPhysics_StaticMulti::GetBlockingEntity() const {
	return nullptr;
}

/*
================
idPhysics_StaticMulti::GetLinearEndTime
================
*/
index_t idPhysics_StaticMulti::GetLinearEndTime() const {
	return 0;
}

/*
================
idPhysics_StaticMulti::GetAngularEndTime
================
*/
index_t idPhysics_StaticMulti::GetAngularEndTime() const {
	return 0;
}

/*
================
idPhysics_StaticMulti::WriteToSnapshot
================
*/
void idPhysics_StaticMulti::WriteToSnapshot( idBitMsg &msg ) const {
	msg.WriteULongLong( current.Num() );

	for ( size_t i = 0; i < current.Num(); i++ ) {
		idCQuat quat = current[i].axis.ToCQuat();
		idCQuat localQuat = current[i].localAxis.ToCQuat();

		msg.WriteFloat( current[i].origin[0] );
		msg.WriteFloat( current[i].origin[1] );
		msg.WriteFloat( current[i].origin[2] );
		msg.WriteFloat( quat.x );
		msg.WriteFloat( quat.y );
		msg.WriteFloat( quat.z );
		msg.WriteDeltaFloat( current[i].origin[0], current[i].localOrigin[0] );
		msg.WriteDeltaFloat( current[i].origin[1], current[i].localOrigin[1] );
		msg.WriteDeltaFloat( current[i].origin[2], current[i].localOrigin[2] );
		msg.WriteDeltaFloat( quat.x, localQuat.x );
		msg.WriteDeltaFloat( quat.y, localQuat.y );
		msg.WriteDeltaFloat( quat.z, localQuat.z );
	}
}

/*
================
idPhysics_StaticMulti::ReadFromSnapshot
================
*/
void idPhysics_StaticMulti::ReadFromSnapshot( const idBitMsg &msg ) {
	size_t num = msg.ReadULongLong();
	assert( num == current.Num() );

	previous = next;

	next.SetNum( num );

	for ( size_t i = 0; i < current.Num(); i++ ) {
		next[i] = ReadStaticInterpolatePStateFromSnapshot( msg );
	}
}
