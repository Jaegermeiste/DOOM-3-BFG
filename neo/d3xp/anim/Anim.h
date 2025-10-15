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
#ifndef __ANIM_H__
#define __ANIM_H__

//
// animation channels
// these can be changed by modmakers and licensees to be whatever they need.
constexpr size_t ANIM_NumAnimChannels		= 5;
constexpr size_t ANIM_MaxAnimsPerChannel	= 3;
constexpr size_t ANIM_MaxSyncedAnims		= 3;

//
// animation channels.  make sure to change script/doom_defs.script if you add any channels, or change their order
//
constexpr size_t ANIMCHANNEL_ALL			= 0;
constexpr size_t ANIMCHANNEL_TORSO			= 1;
constexpr size_t ANIMCHANNEL_LEGS			= 2;
constexpr size_t ANIMCHANNEL_HEAD			= 3;
constexpr size_t ANIMCHANNEL_EYELIDS		= 4;

// for converting from 24 frames per second to milliseconds
ID_INLINE ID_TIME_T FRAME2MS(size_t framenum ) {
	return idMath::integer_cast<ID_TIME_T>(( framenum * 1000 ) / 24);
}

class idRenderModel;
class idAnimator;
class idAnimBlend;
class function_t;
class idEntity;
class idSaveGame;
class idRestoreGame;

typedef struct {
	size_t	cycleCount;	// how many times the anim has wrapped to the beginning (0 for clamped anims)
	size_t	frame1;
	size_t	frame2;
	float	frontlerp;
	float	backlerp;
} frameBlend_t;

typedef struct {
	size_t					nameIndex;
	size_t					parentNum;
	int						animBits;
	int						firstComponent;
} jointAnimInfo_t;

typedef struct {
	jointHandle_t			num;
	jointHandle_t			parentNum;
	size_t					channel;
} jointInfo_t;

//
// joint modifier modes.  make sure to change script/doom_defs.script if you add any, or change their order.
//
typedef enum {
	JOINTMOD_NONE,				// no modification
	JOINTMOD_LOCAL,				// modifies the joint's position or orientation in joint local space
	JOINTMOD_LOCAL_OVERRIDE,	// sets the joint's position or orientation in joint local space
	JOINTMOD_WORLD,				// modifies joint's position or orientation in model space
	JOINTMOD_WORLD_OVERRIDE		// sets the joint's position or orientation in model space
} jointModTransform_t;

typedef struct {
	jointHandle_t			jointnum;
	idMat3					mat;
	idVec3					pos;
	jointModTransform_t		transform_pos;
	jointModTransform_t		transform_axis;
} jointMod_t;

enum animBit_e : uint8
{
	ANIM_BIT_TX = 0,
	ANIM_BIT_TY = 1,
	ANIM_BIT_TZ = 2,
	ANIM_BIT_QX = 3,
	ANIM_BIT_QY = 4,
	ANIM_BIT_QZ = 5
};

#define	ANIM_TX				BIT( ANIM_BIT_TX )
#define	ANIM_TY				BIT( ANIM_BIT_TY )
#define	ANIM_TZ				BIT( ANIM_BIT_TZ )
#define	ANIM_QX				BIT( ANIM_BIT_QX )
#define	ANIM_QY				BIT( ANIM_BIT_QY )
#define	ANIM_QZ				BIT( ANIM_BIT_QZ )

typedef enum {
	FC_SCRIPTFUNCTION,
	FC_SCRIPTFUNCTIONOBJECT,
	FC_EVENTFUNCTION,
	FC_SOUND,
	FC_SOUND_VOICE,
	FC_SOUND_VOICE2,
	FC_SOUND_BODY,
	FC_SOUND_BODY2,
	FC_SOUND_BODY3,
	FC_SOUND_WEAPON,
	FC_SOUND_ITEM,
	FC_SOUND_GLOBAL,
	FC_SOUND_CHATTER,
	FC_SKIN,
	FC_TRIGGER,
	FC_TRIGGER_SMOKE_PARTICLE,
	FC_MELEE,
	FC_DIRECTDAMAGE,
	FC_BEGINATTACK,
	FC_ENDATTACK,
	FC_MUZZLEFLASH,
	FC_CREATEMISSILE,
	FC_LAUNCHMISSILE,
	FC_FIREMISSILEATTARGET,
	FC_FOOTSTEP,
	FC_LEFTFOOT,
	FC_RIGHTFOOT,
	FC_ENABLE_EYE_FOCUS,
	FC_DISABLE_EYE_FOCUS,
	FC_FX,
	FC_DISABLE_GRAVITY,
	FC_ENABLE_GRAVITY,
	FC_JUMP,
	FC_ENABLE_CLIP,
	FC_DISABLE_CLIP,
	FC_ENABLE_WALK_IK,
	FC_DISABLE_WALK_IK,
	FC_ENABLE_LEG_IK,
	FC_DISABLE_LEG_IK,
	FC_RECORDDEMO,
	FC_AVIGAME
	, FC_LAUNCH_PROJECTILE, 
	FC_TRIGGER_FX,
	FC_START_EMITTER,
	FC_STOP_EMITTER,
} frameCommandType_t;

typedef struct {
	int						num;
	int						firstCommand;
} frameLookup_t;

typedef struct {
	frameCommandType_t		type;
	idStr					*string;

	union {
		const idSoundShader	*soundShader;
		const function_t	*function;
		const idDeclSkin	*skin;
		size_t				index;
	};
} frameCommand_t;

typedef struct {
	bool					prevent_idle_override		: 1;
	bool					random_cycle_start			: 1;
	bool					ai_no_turn					: 1;
	bool					anim_turn					: 1;
} animFlags_t;

/*
==============================================================================================

	idMD5Anim

==============================================================================================
*/

class idMD5Anim {
private:
	size_t					numFrames;
	size_t					frameRate;
	ID_TIME_T				animLength;
	size_t					numJoints;
	size_t					numAnimatedComponents;
	idList<idBounds, TAG_MD5_ANIM>		bounds;
	idList<jointAnimInfo_t, TAG_MD5_ANIM>	jointInfo;
	idList<idJointQuat, TAG_MD5_ANIM>		baseFrame;
	idList<float, TAG_MD5_ANIM>			componentFrames;
	idStr					name;
	idVec3					totaldelta;
	mutable size_t			ref_count;

public:
							idMD5Anim();
							~idMD5Anim();

	void					Free();
	bool					Reload();
	size_t					Allocated() const;
	size_t					Size() const { return sizeof( *this ) + Allocated(); };
	bool					LoadAnim( const char *filename );
	bool					LoadBinary( idFile * file, ID_TIME_T sourceTimeStamp );
	void					WriteBinary( idFile * file, ID_TIME_T sourceTimeStamp );

	void					IncreaseRefs() const;
	void					DecreaseRefs() const;
	size_t					NumRefs() const;
	
	void					CheckModelHierarchy( const idRenderModel *model ) const;
	void					GetInterpolatedFrame( frameBlend_t &frame, idJointQuat *joints, const size_t *index, const size_t numIndexes ) const;
	
	void					GetSingleFrame(Ordinal auto framenum, idJointQuat *joints, const size_t *index, const size_t numIndexes ) const;
	ID_TIME_T				Length() const;
	size_t					NumFrames() const;
	size_t					NumJoints() const;
	const idVec3			&TotalMovementDelta() const;
	const char				*Name() const;

	
	void					GetFrameBlend(Ordinal auto framenum, frameBlend_t &frame ) const;	// frame 1 is first frame
	void					ConvertTimeToFrame(ID_TIME_T time, size_t cyclecount, frameBlend_t &frame ) const;

	void					GetOrigin( idVec3 &offset, ID_TIME_T currentTime, size_t cyclecount ) const;
	void					GetOriginRotation( idQuat &rotation, ID_TIME_T time, size_t cyclecount ) const;
	void					GetBounds( idBounds &bounds, ID_TIME_T currentTime, size_t cyclecount ) const;
};

/*
==============================================================================================

	idAnim

==============================================================================================
*/

class idAnim {
private:
	const class idDeclModelDef	*modelDef;
	const idMD5Anim				*anims[ ANIM_MaxSyncedAnims ];
	size_t						numAnims;
	idStr						name;
	idStr						realname;
	idList<frameLookup_t, TAG_ANIM>		frameLookup;
	idList<frameCommand_t, TAG_ANIM>		frameCommands;
	animFlags_t					flags;

public:
								idAnim();
								idAnim( const idDeclModelDef *modelDef, const idAnim *anim );
								~idAnim();
	
	void						SetAnim( const idDeclModelDef *modelDef, const char *sourcename, const char *animname, Ordinal auto num, const idMD5Anim *md5anims[ ANIM_MaxSyncedAnims ] );
	const char					*Name() const;
	const char					*FullName() const;
	
	const idMD5Anim				*MD5Anim(Ordinal auto num ) const;
	const idDeclModelDef		*ModelDef() const;
	ID_TIME_T					Length() const;
	size_t						NumFrames() const;
	size_t						NumAnims() const;
	const idVec3				&TotalMovementDelta() const;
	
	bool						GetOrigin( idVec3 &offset, Ordinal auto animNum, ID_TIME_T time, size_t cyclecount ) const;
	
	bool						GetOriginRotation( idQuat &rotation, Ordinal auto animNum, ID_TIME_T currentTime, size_t cyclecount ) const;
	
	bool						GetBounds( idBounds &bounds, Ordinal auto animNum, ID_TIME_T time, size_t cyclecount ) const;
	const char					*AddFrameCommand( const class idDeclModelDef *modelDef, Ordinal auto framenum, idLexer &src, const idDict *def );
	void						CallFrameCommands( idEntity *ent, Ordinal auto from, Ordinal auto to ) const;
	bool						HasFrameCommands() const;

								// returns first frame (zero based) that command occurs.  returns -1 if not found.
	Ordinal auto				FindFrameForFrameCommand( frameCommandType_t framecommand, const frameCommand_t **command ) const;
	void						SetAnimFlags( const animFlags_t &animflags );
	const animFlags_t			&GetAnimFlags() const;
};

/*
==============================================================================================

	idDeclModelDef

==============================================================================================
*/

class idDeclModelDef : public idDecl {
public:
								idDeclModelDef();
								~idDeclModelDef();

	virtual size_t				Size() const;
	virtual const char *		DefaultDefinition() const;
	virtual bool				Parse( const char *text, const size_t textLength, bool allowBinaryVersion );
	virtual void				FreeData();

	void						Touch() const;

	const idDeclSkin *			GetDefaultSkin() const;
	const idJointQuat *			GetDefaultPose() const;
	void						SetupJoints( int *numJoints, idJointMat **jointList, idBounds &frameBounds, bool removeOriginOffset ) const;
	idRenderModel *				ModelHandle() const;
	void						GetJointList( const char *jointnames, idList<jointHandle_t> &jointList ) const;
	const jointInfo_t *			FindJoint( const char *name ) const;

	size_t						NumAnims() const;
	
	const idAnim *				GetAnim( Ordinal auto index ) const;
	size_t						GetSpecificAnim( const char *name ) const;
	size_t						GetAnim( const char *name ) const;
	bool						HasAnim( const char *name ) const;
	const idDeclSkin *			GetSkin() const;
	const char *				GetModelName() const;
	const idList<jointInfo_t> &	Joints() const;
	const size_t *				JointParents() const;
	size_t						NumJoints() const;
	
	const jointInfo_t *			GetJoint(jointHandle_t jointHandle ) const;
	
	const char *				GetJointName(jointHandle_t jointHandle ) const;
	
	size_t						NumJointsOnChannel(Ordinal auto channel ) const;
	
	const auto *				GetChannelJoints(Ordinal auto channel ) const;

	const idVec3 &				GetVisualOffset() const;

private:
	void						CopyDecl( const idDeclModelDef *decl );
	bool						ParseAnim( idLexer &src, int numDefaultAnims );

private:
	idVec3						offset;
	idList<jointInfo_t, TAG_ANIM>			joints;
	idList<size_t, TAG_ANIM>				jointParents;
	idList<size_t, TAG_ANIM>				channelJoints[ ANIM_NumAnimChannels ];
	idRenderModel *				modelHandle;
	idList<idAnim *, TAG_ANIM>			anims;
	const idDeclSkin *			skin;
};

/*
==============================================================================================

	idAnimBlend

==============================================================================================
*/

class idAnimBlend {
private:
	const class idDeclModelDef	*modelDef;
	ID_TIME_T					starttime;
	ID_TIME_T					endtime;
	ID_TIME_T					timeOffset;
	float						rate;

	ID_TIME_T					blendStartTime;
	ID_TIME_T					blendDuration;
	float						blendStartValue;
	float						blendEndValue;

	float						animWeights[ ANIM_MaxSyncedAnims ];
	int64					    cycle;
	size_t						frame;
	size_t						animNum;
	bool						allowMove;
	bool						allowFrameCommands;

	friend class				idAnimator;

	void						Reset( const idDeclModelDef *_modelDef );
	void						CallFrameCommands( idEntity *ent, ID_TIME_T fromtime, ID_TIME_T totime ) const;
	
	void						SetFrame( const idDeclModelDef *modelDef, Ordinal auto animnum, size_t frame, ID_TIME_T currenttime, ID_TIME_T blendtime );
	
	void						CycleAnim( const idDeclModelDef *modelDef, Ordinal auto animnum, ID_TIME_T currenttime, ID_TIME_T blendtime );
	
	void						PlayAnim( const idDeclModelDef *modelDef, Ordinal auto animnum, ID_TIME_T currenttime, ID_TIME_T blendtime );
	
	bool						BlendAnim(ID_TIME_T currentTime, Ordinal auto channel, size_t numJoints, idJointQuat *blendFrame, float &blendWeight, bool removeOrigin, bool overrideBlend, bool printInfo ) const;
	void						BlendOrigin( int currentTime, idVec3 &blendPos, float &blendWeight, bool removeOriginOffset ) const;
	void						BlendDelta( int fromtime, int totime, idVec3 &blendDelta, float &blendWeight ) const;
	void						BlendDeltaRotation( int fromtime, int totime, idQuat &blendDelta, float &blendWeight ) const;
	bool						AddBounds( int currentTime, idBounds &bounds, bool removeOriginOffset ) const;

public:
								idAnimBlend();
	void						Save( idSaveGame *savefile ) const;
	void						Restore( idRestoreGame *savefile, const idDeclModelDef *modelDef );
	const char					*AnimName() const;
	const char					*AnimFullName() const;
	float						GetWeight(ID_TIME_T currenttime ) const;
	float						GetFinalWeight() const;
	void						SetWeight( float newweight, ID_TIME_T currenttime, ID_TIME_T blendtime );
	size_t						NumSyncedAnims() const;
	
	bool						SetSyncedAnimWeight( Ordinal auto num, float weight );
	void						Clear(ID_TIME_T currentTime, ID_TIME_T clearTime );
	bool						IsDone(ID_TIME_T currentTime ) const;
	bool						FrameHasChanged(ID_TIME_T currentTime ) const;
	int64						GetCycleCount() const;
	void						SetCycleCount( size_t count );
	void						SetPlaybackRate(ID_TIME_T currentTime, float newRate );
	float						GetPlaybackRate() const;
	void						SetStartTime(ID_TIME_T startTime );
	ID_TIME_T					GetStartTime() const;
	ID_TIME_T					GetEndTime() const;
	size_t						GetFrameNumber(ID_TIME_T currenttime ) const;
	ID_TIME_T					AnimTime(ID_TIME_T currenttime ) const;
	size_t						NumFrames() const;
	ID_TIME_T					Length() const;
	ID_TIME_T					PlayLength() const;
	void						AllowMovement( bool allow );
	void						AllowFrameCommands( bool allow );
	const idAnim				*Anim() const;
	size_t						AnimNum() const;
};

/*
==============================================================================================

	idAFPoseJointMod

==============================================================================================
*/

typedef enum {
	AF_JOINTMOD_AXIS,
	AF_JOINTMOD_ORIGIN,
	AF_JOINTMOD_BOTH
} AFJointModType_t;

class idAFPoseJointMod {
public:
								idAFPoseJointMod();

	AFJointModType_t			mod;
	idMat3						axis;
	idVec3						origin;
};

ID_INLINE idAFPoseJointMod::idAFPoseJointMod() {
	mod = AF_JOINTMOD_AXIS;
	axis.Identity();
	origin.Zero();
}

/*
==============================================================================================

	idAnimator

==============================================================================================
*/

class idAnimator {
public:
								idAnimator();
								~idAnimator();

	size_t						Allocated() const;
	size_t						Size() const;

	void						Save( idSaveGame *savefile ) const;					// archives object for save game file
	void						Restore( idRestoreGame *savefile );					// unarchives object from save game file

	void						SetEntity( idEntity *ent );
	idEntity					*GetEntity() const ;
	void						RemoveOriginOffset( bool remove );
	bool						RemoveOrigin() const;

	void						GetJointList( const char *jointnames, idList<jointHandle_t> &jointList ) const;

	int							NumAnims() const;
	const idAnim				*GetAnim( int index ) const;
	int							GetAnim( const char *name ) const;
	bool						HasAnim( const char *name ) const;

	void						ServiceAnims( int fromtime, int totime );
	bool						IsAnimating( int currentTime ) const;

	void						GetJoints( int *numJoints, idJointMat **jointsPtr );
	int							NumJoints() const;
	jointHandle_t				GetFirstChild( jointHandle_t jointnum ) const;
	jointHandle_t				GetFirstChild( const char *name ) const;

	idRenderModel				*SetModel( const char *modelname );
	idRenderModel				*ModelHandle() const;
	const idDeclModelDef		*ModelDef() const;

	void						ForceUpdate();
	void						ClearForceUpdate();
	bool						CreateFrame( int animtime, bool force );
	bool						FrameHasChanged( int animtime ) const;
	void						GetDelta( int fromtime, int totime, idVec3 &delta ) const;
	bool						GetDeltaRotation( int fromtime, int totime, idMat3 &delta ) const;
	void						GetOrigin( int currentTime, idVec3 &pos ) const;
	bool						GetBounds( int currentTime, idBounds &bounds );

	idAnimBlend					*CurrentAnim( int channelNum );
	void						Clear( int channelNum, int currentTime, int cleartime );
	void						SetFrame( int channelNum, int animnum, int frame, int currenttime, int blendtime );
	void						CycleAnim( int channelNum, int animnum, int currenttime, int blendtime );
	void						PlayAnim( int channelNum, int animnum, int currenttime, int blendTime );

								// copies the current anim from fromChannelNum to channelNum.
								// the copied anim will have frame commands disabled to avoid executing them twice.
	void						SyncAnimChannels( Ordinal auto channelNum, Ordinal auto fromChannelNum, ID_TIME_T currenttime, ID_TIME_T blendTime );

	void						SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos );
	void						SetJointAxis( jointHandle_t jointnum, jointModTransform_t transform_type, const idMat3 &mat );
	void						ClearJoint( jointHandle_t jointnum );
	void						ClearAllJoints();

	void						InitAFPose();
	void						SetAFPoseJointMod( const jointHandle_t jointNum, const AFJointModType_t mod, const idMat3 &axis, const idVec3 &origin );
	void						FinishAFPose( int animnum, const idBounds &bounds, const ID_TIME_T time );
	void						SetAFPoseBlendWeight( float blendWeight );
	bool						BlendAFPose( idJointQuat *blendFrame ) const;
	void						ClearAFPose();

	void						ClearAllAnims( ID_TIME_T currentTime, ID_TIME_T cleartime );

	jointHandle_t				GetJointHandle( const char *name ) const;
	const char *				GetJointName( jointHandle_t handle ) const;
	int							GetChannelForJoint( jointHandle_t joint ) const;
	bool						GetJointTransform( jointHandle_t jointHandle, ID_TIME_T currenttime, idVec3 &offset, idMat3 &axis );
	bool						GetJointLocalTransform( jointHandle_t jointHandle, ID_TIME_T currentTime, idVec3 &offset, idMat3 &axis );

	const animFlags_t			GetAnimFlags(Ordinal auto animnum ) const;
	int							NumFrames(Ordinal auto animnum ) const;
	int							NumSyncedAnims(Ordinal auto animnum ) const;
	const char					*AnimName(Ordinal auto animnum ) const;
	const char					*AnimFullName(Ordinal auto animnum ) const;
	int							AnimLength(Ordinal auto animnum ) const;
	const idVec3				&TotalMovementDelta(Ordinal auto animnum ) const;

private:
	void						FreeData();
	void						PushAnims(Ordinal auto channel, ID_TIME_T currentTime, ID_TIME_T blendTime );

private:
	const idDeclModelDef *		modelDef;
	idEntity *					entity;

	idAnimBlend					channels[ ANIM_NumAnimChannels ][ ANIM_MaxAnimsPerChannel ];
	idList<jointMod_t *, TAG_ANIM>		jointMods;
	int							numJoints;
	idJointMat *				joints;

	mutable int					lastTransformTime;		// mutable because the value is updated in CreateFrame
	mutable bool				stoppedAnimatingUpdate;
	bool						removeOriginOffset;
	bool						forceUpdate;

	idBounds					frameBounds;

	float						AFPoseBlendWeight;
	idList<jointHandle_t, TAG_ANIM>	AFPoseJoints;
	idList<idAFPoseJointMod, TAG_ANIM>	AFPoseJointMods;
	idList<idJointQuat, TAG_ANIM>	AFPoseJointFrame;
	idBounds					AFPoseBounds;
	int							AFPoseTime;
};

/*
==============================================================================================

	idAnimManager

==============================================================================================
*/

class idAnimManager {
public:
								idAnimManager();
								~idAnimManager();

	static bool					forceExport;

	void						Shutdown();
	idMD5Anim *					GetAnim( const char *name );
	void						Preload( const idPreloadManifest &manifest );
	void						ReloadAnims();
	void						ListAnims() const;
	int							JointIndex( const char *name );
	const char *				JointName( int index ) const;

	void						ClearAnimsInUse();
	void						FlushUnusedAnims();

private:
	idHashTable<idMD5Anim *>	animations;
	idStrList					jointnames;
	idHashIndex					jointnamesHash;
};

#endif /* !__ANIM_H__ */
