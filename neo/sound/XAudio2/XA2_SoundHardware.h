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
#ifndef __XA_SOUNDHARDWARE_H__
#define __XA_SOUNDHARDWARE_H__

#pragma once

class idSoundSample_XAudio2;
class idSoundVoice_XAudio2;

enum XAudio2DeviceRole_e : int8 {
	NotDefaultDevice            = 0x0,
	DefaultConsoleDevice        = 0x1,
	DefaultMultimediaDevice     = 0x2,
	DefaultCommunicationsDevice = 0x4,
	DefaultGameDevice           = 0x8,   // legacy only; never set on modern Windows
	GlobalDefaultDevice         = 0xF,
	InvalidDeviceRole           = ~GlobalDefaultDevice
};

struct XAudio2DeviceInfo_s {
	char                 DeviceID[256]{};                      // IMMDevice endpoint ID
	wchar_t              DeviceID_W[256]{};                    // WIDE IMMDevice ID (pass to CreateMasteringVoice szDeviceId)
	char                 DisplayName[256]{};                   // Friendly name (e.g., "Speakers (Realtek High Definition Audio)")
	DWORD                DeviceState{};                        // DEVICE_STATE_ACTIVE, _DISABLED, _NOTPRESENT, _UNPLUGGED
	XAudio2DeviceRole_e  Role{ InvalidDeviceRole };            // bitmask; “Game” bit will be 0 on modern Windows
	WAVEFORMATEXTENSIBLE OutputFormat{};                       // Basic (tag/channels/rate/avgBps/blockAlign/bits)

	// Spatial Audio
	bool                 SupportsSpatialObjects{};             // ISpatialAudioObjectRenderStream
	bool                 SupportsSpatialMetadata{};            // ISpatialAudioObjectRenderStreamForMetadata
	UINT32               MaxDynamicObjects{};                  // number of simultaneous objects
	UINT32               MaxFrameCount{};                      // maximum possible frame count per processing pass
};

/*
================================================
idSoundEngineCallback
================================================
*/
class idSoundEngineCallback : public IXAudio2EngineCallback {
public:
	idSoundHardware_XAudio2 * hardware;

private:
	    // Called by XAudio2 just before an audio processing pass begins.
    STDMETHOD_( void, OnProcessingPassStart ) ( THIS ) {}

    // Called just after an audio processing pass ends.
    STDMETHOD_( void, OnProcessingPassEnd ) ( THIS ) {}

    // Called in the event of a critical system error which requires XAudio2
    // to be closed down and restarted.  The error code is given in Error.
    STDMETHOD_( void, OnCriticalError ) ( THIS_ HRESULT Error );
};

/*
================================================
idSoundHardware_XAudio2
================================================
*/

class idSoundHardware_XAudio2 {
public:
					idSoundHardware_XAudio2();

	void			Init();
	void			Shutdown();

	void 			Update();

	idSoundVoice *	AllocateVoice( const idSoundSample * leadinSample, const idSoundSample * loopingSample );
	void			FreeVoice( idSoundVoice * voice );

	// video playback needs this
	[[nodiscard]] IXAudio2 *		GetIXAudio2() const { return pXAudio2; };

	[[nodiscard]] size_t			GetNumZombieVoices() const { return zombieVoices.Num(); }
	[[nodiscard]] size_t			GetNumFreeVoices() const { return freeVoices.Num(); }

	// Enumerate all available XAudio2 render (output) devices.
    // Returns a list of fully populated XAudio2DeviceInfo_s records.
	ID_INLINE static idList<XAudio2DeviceInfo_s> EnumerateRenderDevices();

protected:
	friend class idSoundSample_XAudio2;
	friend class idSoundVoice_XAudio2;

private:
	IXAudio2 * pXAudio2;
	IXAudio2MasteringVoice * pMasterVoice;
	IXAudio2SubmixVoice * pSubmixVoice;

	idSoundEngineCallback	soundEngineCallback;
	ID_TIME_T			lastResetTime;

	size_t				outputChannels;
	DWORD				channelMask;

	idDebugGraph *		vuMeterRMS;
	idDebugGraph *		vuMeterPeak;
	ID_TIME_T			vuMeterPeakTimes[ 8 ];

	// Can't stop and start a voice on the same frame, so we have to double this to handle the worst case scenario of stopping all voices and starting a full new set
	idStaticList<idSoundVoice_XAudio2, MAX_HARDWARE_VOICES * 2 > voices;
	idStaticList<idSoundVoice_XAudio2 *, MAX_HARDWARE_VOICES * 2 > zombieVoices;
	idStaticList<idSoundVoice_XAudio2 *, MAX_HARDWARE_VOICES * 2 > freeVoices;

	XAudio2DeviceInfo_s preferredAudioDeviceDetails;

	// Helpers
	// Check whether the given device ID corresponds to the current default endpoint for a role.
	ID_INLINE static bool                        IsDefaultRole( IMMDeviceEnumerator* en, const wchar_t* deviceId, ERole role );

	// Query the mix/output format from an IAudioClient and fill a WAVEFORMATEXTENSIBLE structure.
	ID_INLINE static void                        FillMixFormat( IAudioClient* ac, WAVEFORMATEXTENSIBLE& fmt );

	// Retrieve the audio render device details
	// - role: eConsole / eMultimedia / eCommunications
	// - outDeviceID: UTF-8 string buffer (256 bytes)
	// Returns true on success.
	ID_INLINE static bool                        GetRenderDeviceInfo(Microsoft::WRL::ComPtr<IMMDeviceEnumerator> deviceEnumerator, Microsoft::WRL::ComPtr<IMMDevice> device, XAudio2DeviceInfo_s& outInfo);

	// Retrieve the default render device ID for a given Core Audio role.
	// Returns true on success.
	ID_INLINE static bool                        GetDefaultRenderDeviceInfo( ERole role, XAudio2DeviceInfo_s & outInfo);
};

/*
================================================
idSoundHardware
================================================
*/
class idSoundHardware : public idSoundHardware_XAudio2 {
};

#endif
