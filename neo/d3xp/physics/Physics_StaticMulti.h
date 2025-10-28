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

#ifndef __PHYSICS_STATICMULTI_H__
#define __PHYSICS_STATICMULTI_H__

#pragma once

/*
===============================================================================

	Physics for a non moving object using no or multiple collision models.

===============================================================================
*/

class idPhysics_StaticMulti : public idPhysics {

public:
	CLASS_PROTOTYPE( idPhysics_StaticMulti );

							idPhysics_StaticMulti();
							~idPhysics_StaticMulti() override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					RemoveIndex( index_t id = 0, bool freeClipModel = true );

public:	// common physics interface

	void					SetSelf( idEntity *e ) override;

	void					SetClipModel( idClipModel *model, float density, const index_t id = 0, bool freeOld = true ) override;
	[[nodiscard]] idClipModel *			GetClipModel( const index_t id = 0 ) const;
	[[nodiscard]] size_t					GetNumClipModels() const override;

	void					SetMass( float mass, const index_t id = -1 );
	[[nodiscard]] float					GetMass( const index_t id = -1 ) const;

	void					SetContents( int contents, const index_t id = -1 );
	[[nodiscard]] int						GetContents( const index_t id = -1 ) const;

	void					SetClipMask( int mask, const index_t id = -1 );
	[[nodiscard]] int						GetClipMask( const index_t id = -1 ) const;

	[[nodiscard]] const idBounds &		GetBounds( const index_t id = -1 ) const;
	[[nodiscard]] const idBounds &		GetAbsBounds( const index_t id = -1 ) const;

	bool					Evaluate( const ID_TIME_T timeStepMSec, ID_TIME_T endTimeMSec ) override;
	bool					Interpolate( const double fraction ) override;
	void					ResetInterpolationState( const idVec3 & origin, const idMat3 & axis ) override {}
	void					UpdateTime( ID_TIME_T endTimeMSec ) override;
	ID_TIME_T				GetTime() const override;

	void					GetImpactInfo( const index_t id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const index_t id, const idVec3 &point, const idVec3 &impulse );
	void					AddForce( const index_t id, const idVec3 &point, const idVec3 &force );
	void					Activate() override;
	void					PutToRest() override;
	[[nodiscard]] bool					IsAtRest() const override;
	ID_TIME_T				GetRestStartTime() const override;
	[[nodiscard]] bool					IsPushable() const override;

	void					SaveState() override;
	void					RestoreState() override;

	void					SetOrigin( const idVec3 &newOrigin, index_t id = -1 );
	void					SetAxis( const idMat3 &newAxis, index_t id = -1 );

	void					Translate( const idVec3 &translation, index_t id = -1 );
	void					Rotate( const idRotation &rotation, index_t id = -1 );

	[[nodiscard]] const idVec3 &			GetOrigin( const index_t id = 0 ) const;
	[[nodiscard]] const idMat3 &			GetAxis( const index_t id = 0 ) const;

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, index_t id = 0 );
	void					SetAngularVelocity( const idVec3 &newAngularVelocity, index_t id = 0 );

	[[nodiscard]] const idVec3 &			GetLinearVelocity( const index_t id = 0 ) const;
	[[nodiscard]] const idVec3 &			GetAngularVelocity( const index_t id = 0 ) const;

	void					SetGravity( const idVec3 &newGravity ) override;
	[[nodiscard]] const idVec3 &			GetGravity() const override;
	[[nodiscard]] const idVec3 &			GetGravityNormal() const override;

	void					ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const override;
	void					ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const override;
	int						ClipContents( const idClipModel *model ) const override;

	void					DisableClip() override;
	void					EnableClip() override;

	void					UnlinkClip() override;
	void					LinkClip() override;

	bool					EvaluateContacts() override;
	[[nodiscard]] size_t					GetNumContacts() const override;
	[[nodiscard]] const contactInfo_t &	GetContact( const index_t num ) const;
	void					ClearContacts() override;
	void					AddContactEntity( idEntity *e ) override;
	void					RemoveContactEntity( idEntity *e ) override;

	[[nodiscard]] bool					HasGroundContacts() const override;
	[[nodiscard]] bool					IsGroundEntity( const index_t entityNum ) const;
	[[nodiscard]] bool					IsGroundClipModel( const index_t entityNum, const index_t id ) const;

	void					SetPushed( ID_TIME_T deltaTime );
	[[nodiscard]] const idVec3 &			GetPushedLinearVelocity( const index_t id = 0 ) const;
	[[nodiscard]] const idVec3 &			GetPushedAngularVelocity( const index_t id = 0 ) const;

	void					SetMaster( idEntity *master, const bool orientated = true ) override;

	[[nodiscard]] const trace_t *			GetBlockingInfo() const override;
	[[nodiscard]] idEntity *				GetBlockingEntity() const override;

	ID_TIME_T				GetLinearEndTime() const override;
	ID_TIME_T				GetAngularEndTime() const override;

	void					WriteToSnapshot( idBitMsg &msg ) const override;
	void					ReadFromSnapshot( const idBitMsg &msg ) override;

protected:
	idEntity *				self;					// entity using this physics object
	idList<staticPState_t, TAG_IDLIB_LIST_PHYSICS>	current;				// physics state
	idList<idClipModel *, TAG_IDLIB_LIST_PHYSICS>	clipModels;				// collision model

	// States used in client-side interpolation
	idList<staticInterpolatePState_t, TAG_IDLIB_LIST_PHYSICS> previous;
	idList<staticInterpolatePState_t, TAG_IDLIB_LIST_PHYSICS> next;

	// master
	bool					hasMaster;
	bool					isOrientated;
};

#endif /* !__PHYSICS_STATICMULTI_H__ */
