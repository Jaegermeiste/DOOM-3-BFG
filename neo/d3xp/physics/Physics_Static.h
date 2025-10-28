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

#ifndef __PHYSICS_STATIC_H__
#define __PHYSICS_STATIC_H__

#pragma once

/*
===============================================================================

	Physics for a non moving object using at most one collision model.

===============================================================================
*/

class idBitMsg;

typedef struct staticPState_s {
	idVec3					origin;
	idMat3					axis;
	idVec3					localOrigin;
	idMat3					localAxis;
} staticPState_t;

// Storing the state used for interpolation with quaternions
// means I don't have to do a bunch of conversions between
// idMat3s and idQuats every frame.
struct staticInterpolatePState_t {
	idVec3					origin;
	idQuat					axis;
	idVec3					localOrigin;
	idQuat					localAxis;
};

/*
================
ReadStaticInterpolatePStateFromSnapshot
================
*/
staticInterpolatePState_t ReadStaticInterpolatePStateFromSnapshot( const idBitMsg & msg );
staticPState_s	ConvertInterpolateStateToPState( const staticInterpolatePState_t & interpolateState  );
staticInterpolatePState_t ConvertPStateToInterpolateState( const staticPState_t & state );

class idPhysics_Static : public idPhysics {

public:
	CLASS_PROTOTYPE( idPhysics_Static );

							idPhysics_Static();
							~idPhysics_Static() override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

public:	// common physics interface
	void					SetSelf( idEntity *e ) override;

	void					SetClipModel( idClipModel *model, float density, const index_t id = 0, bool freeOld = true ) override;
	idClipModel *			GetClipModel( const index_t id = 0 ) const override;
	size_t					GetNumClipModels() const override;

	void					SetMass( float mass, const index_t id = -1 ) override;
	float					GetMass( const index_t id = -1 ) const override;

	void					SetContents( int contents, const index_t id = -1 ) override;
	int						GetContents( const index_t id = -1 ) const override;

	void					SetClipMask( int mask, const index_t id = -1 ) override;
	int						GetClipMask( const index_t id = -1 ) const override;

	const idBounds &		GetBounds( const index_t id = -1 ) const override;
	const idBounds &		GetAbsBounds( const index_t id = -1 ) const override;

	bool					Evaluate( const ID_TIME_T timeStepMSec, ID_TIME_T endTimeMSec );
	bool					Interpolate( const double fraction ) override;
	void					ResetInterpolationState( const idVec3 & origin, const idMat3 & axis ) override {}
	void					UpdateTime( ID_TIME_T endTimeMSec );
	ID_TIME_T				GetTime() const override;

	void					GetImpactInfo( const index_t id, const idVec3 &point, impactInfo_t *info ) const override;
	void					ApplyImpulse( const index_t id, const idVec3 &point, const idVec3 &impulse ) override;
	void					AddForce( const index_t id, const idVec3 &point, const idVec3 &force ) override;
	void					Activate() override;
	void					PutToRest() override;
	bool					IsAtRest() const override;
	ID_TIME_T				GetRestStartTime() const override;
	bool					IsPushable() const override;

	void					SaveState() override;
	void					RestoreState() override;

	void					SetOrigin( const idVec3 &newOrigin, const index_t id = -1 ) override;
	void					SetAxis( const idMat3 &newAxis, const index_t id = -1 ) override;

	void					Translate( const idVec3 &translation, const index_t id = -1 ) override;
	void					Rotate( const idRotation &rotation, const index_t id = -1 ) override;

	const idVec3 &			GetOrigin( const index_t id = 0 ) const override;
	const idMat3 &			GetAxis( const index_t id = 0 ) const override;

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, const index_t id = 0 ) override;
	void					SetAngularVelocity( const idVec3 &newAngularVelocity, const index_t id = 0 ) override;

	const idVec3 &			GetLinearVelocity( const index_t id = 0 ) const override;
	const idVec3 &			GetAngularVelocity( const index_t id = 0 ) const override;

	void					SetGravity( const idVec3 &newGravity ) override;
	const idVec3 &			GetGravity() const override;
	const idVec3 &			GetGravityNormal() const override;

	void					ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const override;
	void					ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const override;
	int						ClipContents( const idClipModel *model ) const override;

	void					DisableClip() override;
	void					EnableClip() override;

	void					UnlinkClip() override;
	void					LinkClip() override;

	bool					EvaluateContacts() override;
	size_t					GetNumContacts() const override;
	const contactInfo_t &	GetContact( size_t num ) const;
	void					ClearContacts() override;
	void					AddContactEntity( idEntity *e ) override;
	void					RemoveContactEntity( idEntity *e ) override;

	bool					HasGroundContacts() const override;
	bool					IsGroundEntity( const index_t entityNum ) const;
	bool					IsGroundClipModel( const index_t entityNum, const index_t id ) const;

	void					SetPushed( ID_TIME_T deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const index_t id = 0 ) const override;
	const idVec3 &			GetPushedAngularVelocity( const index_t id = 0 ) const override;

	void					SetMaster( idEntity *master, const bool orientated = true ) override;

	const trace_t *			GetBlockingInfo() const override;
	idEntity *				GetBlockingEntity() const override;

	ID_TIME_T				GetLinearEndTime() const override;
	ID_TIME_T				GetAngularEndTime() const override;

	void					WriteToSnapshot( idBitMsg &msg ) const override;
	void					ReadFromSnapshot( const idBitMsg &msg ) override;

protected:
	idEntity *				self;					// entity using this physics object
	staticPState_t			current;				// physics state
	idClipModel *			clipModel;				// collision model
	
	// Used for client-side interpolation
	staticInterpolatePState_t	previous;
	staticInterpolatePState_t	next;

	// master
	bool					hasMaster;
	bool					isOrientated;
};

staticPState_t InterpolateStaticPState( const staticInterpolatePState_t & previous,
										const staticInterpolatePState_t & next,
										double fraction );

#endif /* !__PHYSICS_STATIC_H__ */
