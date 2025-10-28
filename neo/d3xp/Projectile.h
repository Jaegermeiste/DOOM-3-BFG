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

#ifndef __GAME_PROJECTILE_H__
#define __GAME_PROJECTILE_H__

/*
===============================================================================

  idProjectile
	
===============================================================================
*/

extern const idEventDef EV_Explode;

class idProjectile : public idEntity {
public :
	CLASS_PROTOTYPE( idProjectile );

							idProjectile();
	~idProjectile() override;

	void					Spawn();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Create( idEntity *owner, const idVec3 &start, const idVec3 &dir );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
	void			FreeLightDef() override;

	idEntity *				GetOwner() const;
	void					CatchProjectile( idEntity* o, const char* reflectName );
	int						GetProjectileState();
	void					Event_CreateProjectile( idEntity *owner, const idVec3 &start, const idVec3 &dir );
	void					Event_LaunchProjectile( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity );
	void					Event_SetGravity( float gravity );

	void			Think() override;
	void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;
	bool			Collide( const trace_t &collision, const idVec3 &velocity ) override;
	virtual void			Explode( const trace_t &collision, idEntity *ignore );
	void					Fizzle();

	static idVec3			GetVelocity( const idDict *projectile );
	static idVec3			GetGravity( const idDict *projectile );

	enum {
		EVENT_DAMAGE_EFFECT = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	void					SetLaunchedFromGrabber(const bool bl ) { launchedFromGrabber = bl; }
	bool					GetLaunchedFromGrabber() { return launchedFromGrabber; }

	static void				DefaultDamageEffect( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity );
	static bool				ClientPredictionCollide( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity, bool addDamageEffect );
	void			ClientPredictionThink() override;
	void			ClientThink( const int curTime, const double fraction, const bool predict ) override;
	void			WriteToSnapshot( idBitMsg &msg ) const override;
	void			ReadFromSnapshot( const idBitMsg &msg ) override;
	bool			ClientReceiveEvent( int event, const ID_TIME_T time, const idBitMsg &msg ) override;

	void					QueueToSimulate( ID_TIME_T startTime );
	virtual void			SimulateProjectileFrame( int msec, ID_TIME_T endTime );
	virtual void			PostSimulate( ID_TIME_T endTime );

	struct simulatedProjectile_t {
		simulatedProjectile_t(): projectile(nullptr), startTime( 0 ) {}
		idProjectile* projectile;
		int	startTime;
	};

	static constexpr int		MAX_SIMULATED_PROJECTILES = 64;

	// This list is used to "catch up" client projectiles to the current time on the server
	static idArray< simulatedProjectile_t, MAX_SIMULATED_PROJECTILES >	projectilesToSimulate;

protected:
	idEntityPtr<idEntity>	owner;

	struct projectileFlags_s {
		bool				detonate_on_world			: 1;
		bool				detonate_on_actor			: 1;
		bool				randomShaderSpin			: 1;
		bool				isTracer					: 1;
		bool				noSplashDamage				: 1;
	} projectileFlags;

	bool					launchedFromGrabber;

	float					thrust;
	int						thrust_end;
	float					damagePower;

	renderLight_t			renderLight;
	qhandle_t				lightDefHandle;				// handle to renderer light def
	idVec3					lightOffset;
	int						lightStartTime;
	int						lightEndTime;
	idVec3					lightColor;

	idForce_Constant		thruster;
	idPhysics_RigidBody		physicsObj;

	const idDeclParticle *	smokeFly;
	int						smokeFlyTime;
	bool					mNoExplodeDisappear;
	bool					mTouchTriggers;

	int						originalTimeGroup;

	typedef enum {
		// must update these in script/doom_defs.script if changed
		SPAWNED = 0,
		CREATED = 1,
		LAUNCHED = 2,
		FIZZLED = 3,
		EXPLODED = 4
	} projectileState_t;
	
	projectileState_t		state;

private:

	idVec3					launchOrigin;
	idMat3					launchAxis;

	void					AddDefaultDamageEffect( const trace_t &collision, const idVec3 &velocity );
	void					AddParticlesAndLight();

	void					Event_Explode();
	void					Event_Fizzle();
	void					Event_RadiusDamage( idEntity *ignore );
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					Event_GetProjectileState();
};

class idGuidedProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idGuidedProjectile );

							idGuidedProjectile();
							~idGuidedProjectile() override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();
	void			Think() override;
	void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f ) override;
	void					SetEnemy( idEntity *ent );
	void					Event_SetEnemy(idEntity *ent);

protected:
	float					speed;
	idEntityPtr<idEntity>	enemy;
	virtual void			GetSeekPos( idVec3 &out );

private:
	idAngles				rndScale;
	idAngles				rndAng;
	idAngles				angles;
	int						rndUpdateTime;
	float					turn_max;
	float					clamp_dist;
	bool					burstMode;
	bool					unGuided;
	float					burstDist;
	float					burstVelocity;
};

class idSoulCubeMissile : public idGuidedProjectile {
public:
	CLASS_PROTOTYPE ( idSoulCubeMissile );
	~idSoulCubeMissile() override;
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();
	void			Think() override;
	void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float power = 1.0f, const float dmgPower = 1.0f ) override;

protected:
	void			GetSeekPos( idVec3 &out ) override;
	void					ReturnToOwner();
	void					KillTarget( const idVec3 &dir );

private:
	idVec3					startingVelocity;
	idVec3					endingVelocity;
	float					accelTime;
	int						launchTime;
	bool					killPhase;
	bool					returnPhase;
	idVec3					destOrg;
	idVec3					orbitOrg;
	int						orbitTime;
	int						smokeKillTime;
	const idDeclParticle *	smokeKill;
};

struct beamTarget_t {
	idEntityPtr<idEntity>	target;
	renderEntity_t			renderEntity;
	qhandle_t				modelDefHandle;
};

class idBFGProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idBFGProjectile );

							idBFGProjectile();
							~idBFGProjectile() override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();
	void			Think() override;
	void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f ) override;
	void			Explode( const trace_t &collision, idEntity *ignore ) override;

private:
	idList<beamTarget_t, TAG_PROJECTILE>	beamTargets;
	renderEntity_t			secondModel;
	qhandle_t				secondModelDefHandle;
	int						nextDamageTime;
	idStr					damageFreq;

	void					FreeBeams();
	void					Event_RemoveBeams();
	void					ApplyDamage();
};

class idHomingProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idHomingProjectile );

	idHomingProjectile();
	~idHomingProjectile() override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();
	void			Think() override;
	void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f ) override;
	void					SetEnemy( idEntity *ent );
	void					SetSeekPos( idVec3 pos );
	void					Event_SetEnemy(idEntity *ent);

protected:
	float					speed;
	idEntityPtr<idEntity>	enemy;
	idVec3					seekPos;

private:
	idAngles				rndScale;
	idAngles				rndAng;
	idAngles				angles;
	float					turn_max;
	float					clamp_dist;
	bool					burstMode;
	bool					unGuided;
	float					burstDist;
	float					burstVelocity;
};


/*
===============================================================================

  idDebris
	
===============================================================================
*/

class idDebris : public idEntity {
public :
	CLASS_PROTOTYPE( idDebris );

							idDebris();
							~idDebris() override;

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	void					Spawn();

	void					Create( idEntity *owner, const idVec3 &start, const idMat3 &axis );
	void					Launch();
	void					Think() override;
	void					Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;
	void					Explode();
	void					Fizzle();
	bool			Collide( const trace_t &collision, const idVec3 &velocity ) override;


private:
	idEntityPtr<idEntity>	owner;
	idPhysics_RigidBody		physicsObj;
	const idDeclParticle *	smokeFly;
	int						smokeFlyTime;
	const idSoundShader *	sndBounce;


	void					Event_Explode();
	void					Event_Fizzle();
};

#endif /* !__GAME_PROJECTILE_H__ */
