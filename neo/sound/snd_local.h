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

#ifndef __SND_LOCAL_H__
#define __SND_LOCAL_H__

#pragma once

#include "WaveFile.h"

// Maximum number of voices we can have allocated
constexpr size_t MAX_HARDWARE_VOICES = 48;

// A single voice can play multiple channels (up to 5.1, but most commonly stereo)
// This is the maximum number of channels which can play simultaneously
// This is limited primarily by seeking on the optical drive, secondarily by memory consumption, and tertiarily by CPU time spent mixing
constexpr size_t MAX_HARDWARE_CHANNELS = 64;

// We may need up to 3 buffers for each hardware voice if they are all long sounds
constexpr size_t MAX_SOUND_BUFFERS = (MAX_HARDWARE_VOICES * 3);

// Maximum number of channels in a sound sample
constexpr size_t MAX_CHANNELS_PER_VOICE = 8;

/*
========================
MsecToSamples
SamplesToMsec
========================
*/
ID_INLINE_EXTERN size_t MsecToSamples( ID_TIME_T msec, uint32 sampleRate ) { return ( msec * ( sampleRate / 100 ) ) / 10; }
ID_INLINE_EXTERN ID_TIME_T SamplesToMsec( size_t samples, uint32 sampleRate ) { return sampleRate < 100 ? 0 : ( samples * 10 ) / ( sampleRate / 100 ); }

/*
========================
DBtoLinear
LinearToDB
========================
*/
ID_INLINE_EXTERN float DBtoLinear( float db ) { return idMath::Pow( 2.0f, db * ( 1.0f / 6.0f ) ); }
ID_INLINE_EXTERN float LinearToDB( float linear ) { return ( linear > 0.0f ) ? ( idMath::Log( linear ) * ( 6.0f / 0.693147181f ) ) : -999.0f; }

// demo sound commands
typedef enum soundDemoCommand_e : uint8 {
	SCMD_STATE,				// followed by a load game state
	SCMD_PLACE_LISTENER,
	SCMD_ALLOC_EMITTER,

	SCMD_FREE,
	SCMD_UPDATE,
	SCMD_START,
	SCMD_MODIFY,
	SCMD_STOP,
	SCMD_FADE
} soundDemoCommand_t;

#include "SoundVoice.h"


#define OPERATION_SET 1

//#include <dxsdkver.h>

#include <xaudio2.h>
#include <xaudio2fx.h>
#include <X3DAudio.h>
//#include <xma2defs.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <audioclient.h>
#include <propvarutil.h>
#include <spatialaudioclient.h>
#include <spatialaudiometadata.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include "XAudio2/XA2_SoundSample.h"
#include "XAudio2/XA2_SoundVoice.h"
#include "XAudio2/XA2_SoundHardware.h"



//------------------------
// Listener data
//------------------------
struct listener_t {
	idMat3	axis;		// orientation of the listener
	idVec3	pos;		// position in meters
	size_t	id;			// the entity number, used to detect when a sound is local
	size_t	area;		// area number the listener is in
};

class idSoundFade {
public:
	ID_TIME_T	fadeStartTime;
	ID_TIME_T	fadeEndTime;
	float	fadeStartVolume;
	float	fadeEndVolume;


public:
	idSoundFade() { Clear(); }

	void	Clear();
	void	SetVolume( float to );
	void	Fade( float to, ID_TIME_T length, ID_TIME_T soundTime );

	float	GetVolume( ID_TIME_T soundTime ) const;
};

/*
================================================
idSoundChannel
================================================
*/
class idSoundChannel {
public:
	bool	CanMute() const;

	void	Mute();
	bool	CheckForCompletion( ID_TIME_T currentTime ) const;

	void	UpdateVolume( ID_TIME_T currentTime );
	void	UpdateHardware( float volumeAdd, ID_TIME_T currentTime );

	// returns true if this channel is marked as looping
	bool	IsLooping() const;

	class idSoundEmitterLocal *	emitter;

	ID_TIME_T				startTime;
	ID_TIME_T				endTime;
	size_t					logicalChannel;
	bool					allowSlow;

	soundShaderParms_t		parms;				// combines shader parms and per-channel overrides
	const idSoundShader *	soundShader;
	idSoundSample *			leadinSample;
	idSoundSample *			loopingSample;
	idSoundFade				volumeFade;

	float					volumeDB;			// last volume at which this channel will play (calculated in UpdateVolume)
	float					currentAmplitude;	// current amplitude on the hardware voice

	// hardwareVoice will be freed and NULL'd when a sound is out of range, 
	// and reallocated when it comes back in range
	idSoundVoice *			hardwareVoice;

	// only allocated by the soundWorld block allocator
	idSoundChannel();
	~idSoundChannel();
};

// Maximum number of SoundChannels for a single SoundEmitter.
// This is probably excessive...
constexpr size_t MAX_CHANNELS_PER_EMITTER = 16;

/*
===================================================================================

idSoundWorldLocal

===================================================================================
*/

class idSoundWorldLocal : public idSoundWorld {
public:
							idSoundWorldLocal();
							~idSoundWorldLocal() override;

	//------------------------
	// Functions from idSoundWorld, implemented in SoundWorld.cpp
	//------------------------

	// Called at map start
							void			ClearAllSoundEmitters() override;

	// stop all playing sounds
							void			StopAllSounds() override;

	// get a new emitter that can play sounds in this world
							idSoundEmitter *AllocSoundEmitter() override;

	// for load games
							idSoundEmitter *EmitterForIndex( const index_t index ) override;

	// query data from all emitters in the world
							float			CurrentShakeAmplitude() override;

	// where is the camera
							void			PlaceListener( const idVec3 &origin, const idMat3 &axis, const index_t listenerId ) override;

	// fade all sounds in the world with a given shader soundClass
	// to is in Db, over is in seconds
							void			FadeSoundClasses( const int soundClass, const float to, const float over ) override;

	// dumps the current state and begins archiving commands
							void			StartWritingDemo( idDemoFile *demo ) override;
							void			StopWritingDemo() override;

	// read a sound command from a demo file
							void			ProcessDemoCommand( idDemoFile *readDemo ) override;

	// menu sounds
							ID_TIME_T		PlayShaderDirectly( const char *name, const s_channelType channel = -1 ) override;

							void			Skip( ID_TIME_T time ) override;

							void			Pause() override;
							void			UnPause() override;
							bool			IsPaused() override { return isPaused; }

	virtual ID_TIME_T		GetSoundTime();

	// avidump
							void			AVIOpen( const char *path, const char *name ) override;
							void			AVIClose() override;

	// SaveGame Support
							void			WriteToSaveGame( idFile *savefile ) override;
							void			ReadFromSaveGame( idFile *savefile ) override;

							void			SetSlowmoSpeed( float speed ) override;
							void			SetEnviroSuit( bool active ) override;

	//=======================================

	//------------------------
	// Random stuff that's not exposed outside the sound system
	//------------------------
	void			Update();
	void			OnReloadSound( const idDecl *decl );

	idSoundChannel *	AllocSoundChannel();
	void				FreeSoundChannel( idSoundChannel * );

public:
	// even though all these variables are public, nobody outside the sound system includes SoundWorld_local.h
	// so this is equivalent to making it private and friending all the other classes in the sound system

	idSoundFade			volumeFade;						// master volume knob for the entire world
	idSoundFade			soundClassFade[SOUND_MAX_CLASSES];

	idRenderWorld *		renderWorld;	// for debug visualization and light amplitude sampling
	idDemoFile *		writeDemo;		// if not NULL, archive commands here

	float				currentCushionDB;	// channels at or below this level will be faded to 0
	float				shakeAmp;			// last calculated shake amplitude

	listener_t			listener;
	idList<idSoundEmitterLocal *, TAG_AUDIO>	emitters;

	idSoundEmitter *	localSound;			// for PlayShaderDirectly()

	idBlockAlloc<idSoundEmitterLocal, 16>	emitterAllocator;
	idBlockAlloc<idSoundChannel, 16>		channelAllocator;

	idSoundFade				pauseFade;
	ID_TIME_T				pausedTime;
	ID_TIME_T				accumulatedPauseTime;
	bool					isPaused;

	float					slowmoSpeed;
	bool					enviroSuitActive;

public: 
	struct soundPortalTrace_t {
		int		portalArea;
		const soundPortalTrace_t * prevStack;
	};

	void			ResolveOrigin( const int stackDepth, const soundPortalTrace_t * prevStack, const int soundArea, const float dist, const idVec3 & soundOrigin, idSoundEmitterLocal * def );
};


/*
================================================
idSoundEmitterLocal 
================================================
*/
class idSoundEmitterLocal : public idSoundEmitter {
public:
	void	Free( bool immediate ) override;

	virtual void	Reset();

	void	UpdateEmitter( const idVec3 &origin, int listenerId, const soundShaderParms_t *parms ) override;

	[[nodiscard]] ID_TIME_T StartSound( const idSoundShader *shader, const s_channelType channel, float diversity = 0, int shaderFlags = 0, bool allowSlow = true ) override;

	void	ModifySound( const s_channelType channel, const soundShaderParms_t *parms ) override;
	void	StopSound( const s_channelType channel ) override;

	void	FadeSound( const s_channelType channel, float to, float over ) override;

	[[nodiscard]] bool	CurrentlyPlaying( const s_channelType channel = SCHANNEL_ANY ) const override;

	float	CurrentAmplitude() override;

	[[nodiscard]] size_t	Index() const override;

	//----------------------------------------------

	void			Init( index_t i, idSoundWorldLocal * sw );

	// Returns true if the emitter should be freed.
	bool			CheckForCompletion( ID_TIME_T currentTime );

	void			OverrideParms( const soundShaderParms_t * base, const soundShaderParms_t * over, soundShaderParms_t * out );

	void			Update( ID_TIME_T currentTime );
	void			OnReloadSound( const idDecl *decl );

	//----------------------------------------------

	idSoundWorldLocal *		soundWorld;						// the world that holds this emitter

	size_t		index;							// in world emitter list
	bool		canFree;						// if true, this emitter can be canFree (once channels.Num() == 0)

	// a single soundEmitter can have many channels playing from the same point
	idStaticList<idSoundChannel *, MAX_CHANNELS_PER_EMITTER> channels;

	//----- set by UpdateEmitter -----
	idVec3				origin;
	soundShaderParms_t	parms;
	size_t				emitterId;						// sounds will be full volume when emitterId == listenerId

	//----- set by Update -----
	int			lastValidPortalArea;
	float		directDistance;
	float		spatializedDistance;
	idVec3		spatializedOrigin;

	// sound emitters are only allocated by the soundWorld block allocator
					idSoundEmitterLocal();
	~idSoundEmitterLocal() override;
};


/*
===================================================================================

idSoundSystemLocal

===================================================================================
*/
class idSoundSystemLocal : public idSoundSystem {
public:
	// all non-hardware initialization
	void			Init() override;

	// shutdown routine
	void			Shutdown() override;

	idSoundWorld *	AllocSoundWorld( idRenderWorld *rw ) override;
	void			FreeSoundWorld( idSoundWorld *sw ) override;

	// specifying NULL will cause silence to be played
	void			SetPlayingSoundWorld( idSoundWorld *soundWorld ) override;

	// some tools, like the sound dialog, may be used in both the game and the editor
	// This can return NULL, so check!
	idSoundWorld *	GetPlayingSoundWorld() override;

	// sends the current playing sound world information to the sound hardware
	void			Render() override;

	// Mutes the SSG_MUSIC group
	void			MuteBackgroundMusic( bool mute ) override { musicMuted = mute; }

	// sets the final output volume to 0
	// This should only be used when the app is deactivated
	// Since otherwise there will be problems with different subsystems muting and unmuting at different times
	void			SetMute( bool mute ) override { muted = mute; }
	bool			IsMuted() override { return muted; }

	void			OnReloadSound( const idDecl * sound ) override;

	void			StopAllSounds() override;

	void			InitStreamBuffers() override;
	void			FreeStreamBuffers() override;

	void *			GetIXAudio2() const override;

	// for the sound level meter window
	cinData_t		ImageForTime( const ID_TIME_T milliseconds, const bool waveform ) override;

	// Free all sounds loaded during the last map load
	void			BeginLevelLoad() override;

	// We might want to defer the loading of new sounds to this point
	void			EndLevelLoad() override;

	// prints memory info
	void			PrintMemInfo( MemInfo_t *mi ) override;

	//-------------------------

	// Before a sound is reloaded, any active voices using it must
	// be stopped.  Returns true if any were playing, and should be
	// restarted after the sound is reloaded.
	void					StopVoicesWithSample( const idSoundSample * const sample );

	void					Restart();
	void					SetNeedsRestart() { needsRestart = true; }

	ID_TIME_T				SoundTime() const;

	// may return NULL if there are no more voices left
	idSoundVoice *			AllocateVoice( const idSoundSample * leadinSample, const idSoundSample * loopingSample );
	void					FreeVoice( idSoundVoice * );

	idSoundSample *			LoadSample( const char * name );

	void			Preload( idPreloadManifest & preload ) override;

	struct bufferContext_t {
		bufferContext_t() :
			voice(nullptr),
			sample(nullptr),
			bufferNumber( 0 )
		{ }
		idSoundVoice_XAudio2 *	voice;
		idSoundSample_XAudio2 * sample;
		size_t bufferNumber;
	};

	// Get a stream buffer from the free pool, returns NULL if none are available
	bufferContext_t *			ObtainStreamBufferContext();
	void						ReleaseStreamBufferContext( bufferContext_t * p );

	idSysMutex					streamBufferMutex;
	idStaticList< bufferContext_t *, MAX_SOUND_BUFFERS > freeStreamBufferContexts;
	idStaticList< bufferContext_t *, MAX_SOUND_BUFFERS > activeStreamBufferContexts;
	idStaticList< bufferContext_t, MAX_SOUND_BUFFERS > bufferContexts;

	idSoundWorldLocal *			currentSoundWorld;
	idStaticList<idSoundWorldLocal *, 32>	soundWorlds;

	idList<idSoundSample *, TAG_AUDIO>		samples;
	idHashIndex					sampleHash;

	idSoundHardware				hardware;

	idRandom2					random;
	
	ID_TIME_T					soundTime;
	bool						muted;
	bool						musicMuted;
	bool						needsRestart;

	bool						insideLevelLoad;

	//-------------------------

	idSoundSystemLocal() :
		soundTime( 0 ),
		currentSoundWorld( nullptr ),
		muted( false ),
		musicMuted( false ),
		needsRestart( false )
		{}
};

extern	idSoundSystemLocal	soundSystemLocal;

#endif /* !__SND_LOCAL_H__ */
