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

#ifndef __PHYSICS_PARAMETRIC_H__
#define __PHYSICS_PARAMETRIC_H__

#pragma once

/*
===================================================================================

	Parametric physics

	Used for predefined or scripted motion. The motion of an object is completely
	parametrized. By adjusting the parameters an object is forced to follow a
	predefined path. The parametric physics is typically used for doors, bridges,
	rotating fans etc.

===================================================================================
*/

typedef struct parametricPState_s {
	ID_TIME_T								time;					// physics time
	ID_TIME_T								atRest;					// set when simulation is suspended
	idVec3									origin;					// world origin
	idAngles								angles;					// world angles
	idMat3									axis;					// world axis
	idVec3									localOrigin;			// local origin
	idAngles								localAngles;			// local angles
	idExtrapolate<idVec3>					linearExtrapolation;	// extrapolation based description of the position over time
	idExtrapolate<idAngles>					angularExtrapolation;	// extrapolation based description of the orientation over time
	idInterpolateAccelDecelLinear<idVec3>	linearInterpolation;	// interpolation based description of the position over time
	idInterpolateAccelDecelLinear<idAngles>	angularInterpolation;	// interpolation based description of the orientation over time
	idCurve_Spline<idVec3> *				spline;					// spline based description of the position over time
	idInterpolateAccelDecelLinear<float>	splineInterpolate;		// position along the spline over time
	bool									useSplineAngles;		// set the orientation using the spline
} parametricPState_t;

class idPhysics_Parametric : public idPhysics_Base {

public:
	CLASS_PROTOTYPE( idPhysics_Parametric );

							idPhysics_Parametric();
							~idPhysics_Parametric() override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					SetPusher( int flags );
	bool					IsPusher() const;

	void					SetLinearExtrapolation( extrapolation_t type, ID_TIME_T time, ID_TIME_T duration, const idVec3 &base, const idVec3 &speed, const idVec3 &baseSpeed );
	void					SetAngularExtrapolation( extrapolation_t type, ID_TIME_T time, ID_TIME_T duration, const idAngles &base, const idAngles &speed, const idAngles &baseSpeed );
	extrapolation_t			GetLinearExtrapolationType() const;
	extrapolation_t			GetAngularExtrapolationType() const;

	void					SetLinearInterpolation( ID_TIME_T time, ID_TIME_T accelTime, ID_TIME_T decelTime, ID_TIME_T duration, const idVec3 &startPos, const idVec3 &endPos );
	void					SetAngularInterpolation( ID_TIME_T time, ID_TIME_T accelTime, ID_TIME_T decelTime, ID_TIME_T duration, const idAngles &startAng, const idAngles &endAng );

	void					SetSpline( idCurve_Spline<idVec3> *spline, ID_TIME_T accelTime, ID_TIME_T decelTime, bool useSplineAngles );
	idCurve_Spline<idVec3> *GetSpline() const;
	int64					GetSplineAcceleration() const;
	int64					GetSplineDeceleration() const;
	bool					UsingSplineAngles() const;

	void					GetLocalOrigin( idVec3 &curOrigin ) const;
	void					GetLocalAngles( idAngles &curAngles ) const;

	void					GetAngles( idAngles &curAngles ) const;

public:	// common physics interface
	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true ) override;
	idClipModel *			GetClipModel( int id = 0 ) const override;
	size_t					GetNumClipModels() const override;

	void					SetMass( float mass, int id = -1 ) override;
	float					GetMass( int id = -1 ) const override;

	void					SetContents( int contents, int id = -1 ) override;
	int						GetContents( int id = -1 ) const override;

	const idBounds &		GetBounds( int id = -1 ) const override;
	const idBounds &		GetAbsBounds( int id = -1 ) const override;

	bool					Evaluate( ID_TIME_T timeStepMSec, ID_TIME_T endTimeMSec ) override;
	bool					Interpolate( const float fraction ) override;
	void					UpdateTime( ID_TIME_T endTimeMSec ) override;
	ID_TIME_T				GetTime() const override;

	void					Activate() override;
	bool					IsAtRest() const override;
	ID_TIME_T				GetRestStartTime() const override;
	bool					IsPushable() const override;

	void					SaveState() override;
	void					RestoreState() override;

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 ) override;
	void					SetAxis( const idMat3 &newAxis, int id = -1 ) override;

	void					Translate( const idVec3 &translation, int id = -1 ) override;
	void					Rotate( const idRotation &rotation, int id = -1 ) override;

	const idVec3 &			GetOrigin( int id = 0 ) const override;
	const idMat3 &			GetAxis( int id = 0 ) const override;

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 ) override;
	void					SetAngularVelocity( const idVec3 &newAngularVelocity, int id = 0 ) override;

	const idVec3 &			GetLinearVelocity( int id = 0 ) const override;
	const idVec3 &			GetAngularVelocity( int id = 0 ) const override;

	void					DisableClip() override;
	void					EnableClip() override;

	void					UnlinkClip() override;
	void					LinkClip() override;

	void					SetMaster( idEntity *master, const bool orientated = true ) override;

	const trace_t *			GetBlockingInfo() const override;
	idEntity *				GetBlockingEntity() const override;

	ID_TIME_T				GetLinearEndTime() const override;
	ID_TIME_T				GetAngularEndTime() const override;

	void					WriteToSnapshot( idBitMsg &msg ) const override;
	void					ReadFromSnapshot( const idBitMsg &msg ) override;

private:
	// parametric physics state
	parametricPState_t		current;
	parametricPState_t		saved;

	physicsInterpolationState_t		previous;
	physicsInterpolationState_t		next;

	// pusher
	bool					isPusher;
	idClipModel *			clipModel;
	int						pushFlags;

	// results of last evaluate
	trace_t					pushResults;
	bool					isBlocked;

	// master
	bool					hasMaster;
	bool					isOrientated;

private:
	bool					TestIfAtRest() const;
	void					Rest();
};

#endif /* !__PHYSICS_PARAMETRIC_H__ */
