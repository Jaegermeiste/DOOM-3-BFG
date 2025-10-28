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
#include <algorithm>

#include "../../idlib/precompiled.h"
#include "../snd_local.h"
#include "../../../doomclassic/doom/i_sound.h"

using Microsoft::WRL::ComPtr;

idCVar s_showLevelMeter( "s_showLevelMeter", "0", CVAR_BOOL|CVAR_ARCHIVE, "Show VU meter" );
idCVar s_meterTopTime( "s_meterTopTime", "1000", CVAR_INTEGER|CVAR_ARCHIVE, "How long (in milliseconds) peaks are displayed on the VU meter" );
idCVar s_meterPosition( "s_meterPosition", "100 100 20 200", CVAR_ARCHIVE, "VU meter location (x y w h)" );
idCVar s_device( "s_device", "-1", CVAR_INTEGER|CVAR_ARCHIVE, "Which audio device to use (listDevices to list, -1 for default)" );
idCVar s_showPerfData( "s_showPerfData", "0", CVAR_BOOL, "Show XAudio2 Performance data" );
extern idCVar s_volume_dB;

/*
========================
idSoundHardware_XAudio2::idSoundHardware_XAudio2
========================
*/
idSoundHardware_XAudio2::idSoundHardware_XAudio2() {
	pXAudio2 = nullptr;
	pMasterVoice = nullptr;
	pSubmixVoice = nullptr;

	vuMeterRMS = nullptr;
	vuMeterPeak = nullptr;

	outputChannels = 0;
	channelMask = 0;

	voices.SetNum( 0 );
	zombieVoices.SetNum( 0 );
	freeVoices.SetNum( 0 );

	lastResetTime = 0;

	preferredAudioDeviceDetails = {};
}

static void listDevices_f( const idCmdArgs & args ) {

	IXAudio2 * pXAudio2 = soundSystemLocal.hardware.GetIXAudio2();

	if ( pXAudio2 == nullptr) {
		idLib::Warning( "No xaudio object" );
		return;
	}

	auto renderDeviceList = idSoundHardware_XAudio2::EnumerateRenderDevices();
	if (renderDeviceList.Num() == 0 ) {
		idLib::Warning( "No audio devices found" );
		return;
	}

	for ( index_t i = 0; i < renderDeviceList.Num(); i++ ) {
		auto& deviceDetails = renderDeviceList[i];

		idStaticList< const char *, 7 > roles;
		if (deviceDetails.Role == NotDefaultDevice) {
			roles.Append( "Not Default Device" );
		}
		else if (deviceDetails.Role & InvalidDeviceRole) {
			roles.Append(" Invalid Device" );
		}
		else
		{
			if (deviceDetails.Role & DefaultConsoleDevice) {
				roles.Append("Default Console Device" );
			}
			if (deviceDetails.Role & DefaultMultimediaDevice) {
				roles.Append("Default Multimedia Device" );
			}
			if (deviceDetails.Role & DefaultCommunicationsDevice) {
				roles.Append("Default Communications Device" );
			}
			if (deviceDetails.Role & DefaultGameDevice) {
				roles.Append("Default Game Device" );
			}
			if (deviceDetails.Role & GlobalDefaultDevice) {
				roles.Append("Global Default Device" );
			}
		}

		idStaticList< const char *, 18 > channelNames;
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_FRONT_LEFT ) {
			channelNames.Append( "Front Left" );
		}
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_FRONT_RIGHT ) {
			channelNames.Append( "Front Right" );
		}
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_FRONT_CENTER ) {
			channelNames.Append( "Front Center" );
		}
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_LOW_FREQUENCY ) {
			channelNames.Append( "Low Frequency" );
		}
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_BACK_LEFT ) {
			channelNames.Append( "Back Left" );
		}
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_BACK_RIGHT ) {
			channelNames.Append( "Back Right" );
		}
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_FRONT_LEFT_OF_CENTER ) {
			channelNames.Append( "Front Left of Center" );
		}
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_FRONT_RIGHT_OF_CENTER ) {
			channelNames.Append( "Front Right of Center" );
		}
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_BACK_CENTER ) {
			channelNames.Append( "Back Center" );
		}
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_SIDE_LEFT ) {
			channelNames.Append( "Side Left" );
		}
		if ( deviceDetails.OutputFormat.dwChannelMask & SPEAKER_SIDE_RIGHT ) {
			channelNames.Append( "Side Right" );
		}
		if (deviceDetails.OutputFormat.dwChannelMask & SPEAKER_TOP_CENTER) {
			channelNames.Append( "Top Center" );
		}
		if (deviceDetails.OutputFormat.dwChannelMask & SPEAKER_TOP_FRONT_LEFT) {
			channelNames.Append( "Top Front Left" );
		}
		if (deviceDetails.OutputFormat.dwChannelMask & SPEAKER_TOP_FRONT_CENTER) {
			channelNames.Append( "Top Front Center" );
		}
		if (deviceDetails.OutputFormat.dwChannelMask & SPEAKER_TOP_FRONT_RIGHT) {
			channelNames.Append( "Top Front Right" );
		}
		if (deviceDetails.OutputFormat.dwChannelMask & SPEAKER_TOP_BACK_LEFT) {
			channelNames.Append( "Top Back Left" );
		}
		if (deviceDetails.OutputFormat.dwChannelMask & SPEAKER_TOP_BACK_CENTER) {
			channelNames.Append( "Top Back Center" );
		}
		if (deviceDetails.OutputFormat.dwChannelMask & SPEAKER_TOP_BACK_RIGHT) {
			channelNames.Append( "Top Back Right" );
		}

		idLib::Printf( "[%zd]: %s\n    ID: &s\n    State: 0x%08lX\n", 
			i, 
			deviceDetails.DisplayName[0] ? deviceDetails.DisplayName : "(no name)",
			deviceDetails.DeviceID[0] ? deviceDetails.DeviceID : "(no id)",
			deviceDetails.DeviceState);
		idLib::Printf( "     %d channels, %d Hz, %d bits per sample\n",
			deviceDetails.OutputFormat.Format.nChannels, 
			deviceDetails.OutputFormat.Format.nSamplesPerSec, 
			deviceDetails.OutputFormat.Format.wBitsPerSample);

		if ( channelNames.Num() != deviceDetails.OutputFormat.Format.nChannels ) {
			idLib::Printf( S_COLOR_YELLOW "WARNING: " S_COLOR_RED "Mismatch between # of channels and channel mask\n" );
		}
		if ( channelNames.Num() == 1 ) {
			idLib::Printf( "     %s\n", channelNames[0] );
		} else if ( channelNames.Num() == 2 ) {
			idLib::Printf( "     %s and %s\n", channelNames[0], channelNames[1] );
		} else if ( channelNames.Num() > 2 ) {
			idLib::Printf( "     %s", channelNames[0] );
			for ( size_t j = 1; j < channelNames.Num() - 1; j++ ) {
				idLib::Printf( ", %s", channelNames[j] );
			}
			idLib::Printf( ", and %s\n", channelNames[channelNames.Num() - 1] );
		}

		if ( roles.Num() == 1 ) {
			idLib::Printf( "     %s\n", roles[0] );
		} else if ( roles.Num() == 2 ) {
			idLib::Printf( "     %s and %s\n", roles[0], roles[1] );
		} else if ( roles.Num() > 2 ) {
			idLib::Printf( "     %s", roles[0] );
			for ( size_t j = 1; j < roles.Num() - 1; j++ ) {
				idLib::Printf( ", %s", roles[j] );
			}
			idLib::Printf( ", and %s\n", roles[roles.Num() - 1] );
		}
	}
}

/*
========================
idSoundHardware_XAudio2::IsDefaultRole
========================
*/
bool idSoundHardware_XAudio2::IsDefaultRole( IMMDeviceEnumerator* en, const wchar_t* deviceId, const ERole role )
{
	ComPtr<IMMDevice> def = nullptr;

	if (FAILED(en->GetDefaultAudioEndpoint(eRender, role, def.GetAddressOf())) || !def)
	{
		return false;
	}

	LPWSTR defId = nullptr;

	if (FAILED(def->GetId(&defId)) || !defId)
	{
		return false;
	}

	const bool same = (wcscmp(defId, deviceId) == 0);
	CoTaskMemFree(defId);

	return same;
}

/*
========================
idSoundHardware_XAudio2::FillMixFormat
========================
*/
void idSoundHardware_XAudio2::FillMixFormat( IAudioClient* ac, WAVEFORMATEXTENSIBLE& fmt )
{
	ZeroMemory(&fmt, sizeof(fmt));

	WAVEFORMATEX* pwfx = nullptr;

	if (FAILED(ac->GetMixFormat(&pwfx)) || !pwfx)
	{
		return;
	}

	if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && pwfx->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
	{
		fmt = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
	}
	else
	{
		fmt.Format = *pwfx;
		fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		fmt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		fmt.Samples.wValidBitsPerSample = pwfx->wBitsPerSample;
		fmt.dwChannelMask = 0;
		fmt.SubFormat = (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || pwfx->wBitsPerSample == 32)
			? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
			: KSDATAFORMAT_SUBTYPE_PCM;
	}

	CoTaskMemFree(pwfx);
}

/*
========================
idSoundHardware_XAudio2::GetRenderDeviceInfo
========================
*/

bool idSoundHardware_XAudio2::GetRenderDeviceInfo(ComPtr<IMMDeviceEnumerator> deviceEnumerator, ComPtr<IMMDevice> device, XAudio2DeviceInfo_s& outInfo)
{
	// Clear output first
	ZeroMemory(&outInfo, sizeof(outInfo));
	outInfo.Role = InvalidDeviceRole;

	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	const bool doUninit = (hr == S_OK || hr == S_FALSE);
	if (FAILED(hr))
	{
		return false;
	}

	bool ok = false;

	if (deviceEnumerator)
	{
		if (device)
		{
			// State
			if (FAILED(device->GetState(&outInfo.DeviceState)))
			{
				idLib::Warning("Failed to get audio device state");
			}

			// IDs
			LPWSTR idW = nullptr;
			if (SUCCEEDED(device->GetId(&idW)) && idW)
			{
				// Preserve wide and UTF-8
				idStr::WideCopy(idW, outInfo.DeviceID_W, std::size(outInfo.DeviceID_W));
				idStr::WideToUtf8(idW, outInfo.DeviceID, std::size(outInfo.DeviceID));

				// Friendly name
				ComPtr<IPropertyStore> propertyStore;
				if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, propertyStore.GetAddressOf())) && propertyStore) {
					PROPVARIANT propVariant = {};
					PropVariantInit(&propVariant);

					if (SUCCEEDED(propertyStore->GetValue(PKEY_Device_FriendlyName, &propVariant))) {
						if (propVariant.vt == VT_LPWSTR && propVariant.pwszVal) {
							idStr::WideToUtf8(propVariant.pwszVal, outInfo.DisplayName, std::size(outInfo.DisplayName));
						}

						if (FAILED(PropVariantClear(&propVariant)))
						{
							idLib::Warning("Failed to clear propVariant");
						}
					}
				}

				// Role bitmask: set all roles the endpoint is default for
				XAudio2DeviceRole_e mask = NotDefaultDevice;
				/*if (IsDefaultRole(deviceEnumerator.Get(), idW, eGame))
				{
					mask = static_cast<XAudio2DeviceRole_e>(mask | DefaultGameDevice);
				}*/
				if (IsDefaultRole(deviceEnumerator.Get(), idW, eConsole))
				{
					mask = static_cast<XAudio2DeviceRole_e>(mask | DefaultConsoleDevice);
				}
				if (IsDefaultRole(deviceEnumerator.Get(), idW, eMultimedia))
				{
					mask = static_cast<XAudio2DeviceRole_e>(mask | DefaultMultimediaDevice);
				}
				if (IsDefaultRole(deviceEnumerator.Get(), idW, eCommunications))
				{
					mask = static_cast<XAudio2DeviceRole_e>(mask | DefaultCommunicationsDevice);
				}
				outInfo.Role = mask; // (Legacy Game bit is never set by Core Audio)

				// Mix format
				ComPtr<IAudioClient> audioClient = nullptr;
				if (SUCCEEDED(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
					reinterpret_cast<void**>(audioClient.GetAddressOf()))) && audioClient)
				{
					FillMixFormat(audioClient.Get(), outInfo.OutputFormat);
				}

				// Spatial Audio
				ComPtr<ISpatialAudioClient> spatialAudioClient = nullptr;
				hr = device->Activate(__uuidof(ISpatialAudioClient), CLSCTX_ALL, nullptr,
					reinterpret_cast<void**>(spatialAudioClient.GetAddressOf()));
				if (SUCCEEDED(hr) && spatialAudioClient) {
					// Check object-based (non-metadata) support
					outInfo.SupportsSpatialObjects =
						SUCCEEDED(spatialAudioClient->IsSpatialAudioStreamAvailable(__uuidof(ISpatialAudioObjectRenderStream), nullptr));

					// Check metadata-aware (object + metadata) support
					outInfo.SupportsSpatialMetadata =
						SUCCEEDED(spatialAudioClient->IsSpatialAudioStreamAvailable(__uuidof(ISpatialAudioObjectRenderStreamForMetadata), nullptr));

					// Query max dynamic objects
					UINT32 maxObjects = 0;
					if (SUCCEEDED(spatialAudioClient->GetMaxDynamicObjectCount(&maxObjects))) {
						outInfo.MaxDynamicObjects = maxObjects;
					}

					// Query max frame count
					UINT32 maxFrame = 0;
					if (SUCCEEDED(spatialAudioClient->GetMaxFrameCount(&outInfo.OutputFormat.Format, &maxFrame))) {
						outInfo.MaxDynamicObjects = maxObjects;
					}
				}

				ok = true;
				CoTaskMemFree(idW);
			}
		}
	}

	if (doUninit)
	{
		CoUninitialize();
	}
	return ok;
}

/*
========================
idSoundHardware_XAudio2::EnumerateRenderDevices
========================
*/
idList<XAudio2DeviceInfo_s> idSoundHardware_XAudio2::EnumerateRenderDevices()
{
	idList<XAudio2DeviceInfo_s> result = {};

	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	const bool doUninit = (hr == S_OK || hr == S_FALSE);
	if (FAILED(hr))
	{
		return result;
	}

	ComPtr<IMMDeviceEnumerator> deviceEnumerator = nullptr;
	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(deviceEnumerator.GetAddressOf()))))
	{
		if (doUninit)
		{
			CoUninitialize();
		}
		return result;
	}

	ComPtr<IMMDeviceCollection> deviceCollection = nullptr;
	if (FAILED(deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATEMASK_ALL, deviceCollection.GetAddressOf())))
	{
		if (doUninit)
		{
			CoUninitialize();
		}
		return result;
	}

	UINT count = 0;
	hr = deviceCollection->GetCount(&count);
	if (FAILED(hr))
	{
		return result;
	}

	result.Resize(count);

	for (size_t i = 0; i < count; ++i)
	{
		ComPtr<IMMDevice> device = nullptr;
		if (FAILED(deviceCollection->Item(i, device.GetAddressOf())) || !device)
		{
			continue;
		}

		XAudio2DeviceInfo_s info{};

		GetRenderDeviceInfo(deviceEnumerator, device, info);
		
		result.AddUnique(info);
	}

	if (doUninit)
	{
		CoUninitialize();
	}
	return result;
}

/*
========================
idSoundHardware_XAudio2::GetDefaultRenderDeviceId
========================
*/
bool idSoundHardware_XAudio2::GetDefaultRenderDeviceInfo(ERole role, XAudio2DeviceInfo_s& outInfo)
{
	// Clear output first
	ZeroMemory(&outInfo, sizeof(outInfo));
	outInfo.Role = InvalidDeviceRole;

	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	const bool doUninit = (hr == S_OK || hr == S_FALSE);
	if (FAILED(hr))
	{
		return false;
	}

	bool ok = false;

	ComPtr<IMMDeviceEnumerator> deviceEnumerator = nullptr;
	if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(deviceEnumerator.GetAddressOf()))))
	{
		ComPtr<IMMDevice> device = nullptr;
		if (SUCCEEDED(deviceEnumerator->GetDefaultAudioEndpoint(eRender, role, device.GetAddressOf())) && device)
		{
			GetRenderDeviceInfo(deviceEnumerator, device, outInfo);
		}
	}

	if (doUninit)
	{
		CoUninitialize();
	}
	return ok;
}

/*
========================
idSoundHardware_XAudio2::Init
========================
*/
void idSoundHardware_XAudio2::Init() {

	cmdSystem->AddCommand( "listDevices", listDevices_f, 0, "Lists the connected sound devices", nullptr);

	DWORD xAudioCreateFlags = 0;
#ifdef _DEBUG
	xAudioCreateFlags |= XAUDIO2_DEBUG_ENGINE;
#endif

	XAUDIO2_PROCESSOR xAudioProcessor = XAUDIO2_USE_DEFAULT_PROCESSOR;

	if ( FAILED( XAudio2Create( &pXAudio2, xAudioCreateFlags, xAudioProcessor ) ) ) {
		if ( xAudioCreateFlags & XAUDIO2_DEBUG_ENGINE ) {
			// in case the debug engine isn't installed
			xAudioCreateFlags &= ~XAUDIO2_DEBUG_ENGINE;
			if ( FAILED( XAudio2Create( &pXAudio2, xAudioCreateFlags, xAudioProcessor ) ) ) {		
				idLib::FatalError( "Failed to create XAudio2 engine.  Try installing the latest DirectX." );
				return;
			}
		} else {
			idLib::FatalError( "Failed to create XAudio2 engine.  Try installing the latest DirectX." );
			return;
		}
	}
#ifdef _DEBUG
	XAUDIO2_DEBUG_CONFIGURATION debugConfiguration = { };
	debugConfiguration.TraceMask = XAUDIO2_LOG_WARNINGS;
	debugConfiguration.BreakMask = XAUDIO2_LOG_ERRORS;
	pXAudio2->SetDebugConfiguration( &debugConfiguration );
#endif

	// Register the sound engine callback
	pXAudio2->RegisterForCallbacks( &soundEngineCallback );
	soundEngineCallback.hardware = this;

	auto renderDeviceList = EnumerateRenderDevices();
	if (renderDeviceList.Num() == 0 ) {
		idLib::Warning( "No audio devices found" );
		pXAudio2->Release();
		pXAudio2 = nullptr;
		return;
	}

	idCmdArgs args = {};
	listDevices_f( args );

	index_t preferredDeviceIndex = s_device.GetInteger();
	if (preferredDeviceIndex < 0 || preferredDeviceIndex >= renderDeviceList.Num() ) {
		GetDefaultRenderDeviceInfo(eConsole, preferredAudioDeviceDetails);
	}
	else
	{
		preferredAudioDeviceDetails = renderDeviceList[preferredDeviceIndex];
	}

	idLib::Printf( "Using device %s\n", preferredAudioDeviceDetails.DisplayName );

	const DWORD outputSampleRate = Max( XAUDIO2FX_REVERB_MIN_FRAMERATE, Min( XAUDIO2FX_REVERB_MAX_FRAMERATE, preferredAudioDeviceDetails.OutputFormat.Format.nSamplesPerSec ) ); // 44100;

	if ( FAILED( pXAudio2->CreateMasteringVoice( &pMasterVoice, XAUDIO2_DEFAULT_CHANNELS, outputSampleRate, 0, preferredAudioDeviceDetails.DeviceID_W, NULL ) ) ) {
		idLib::Warning( "Failed to create master voice" );
		pXAudio2->Release();
		pXAudio2 = nullptr;
		return;
	}
	pMasterVoice->SetVolume( DBtoLinear( s_volume_dB.GetFloat() ) );

	outputChannels = preferredAudioDeviceDetails.OutputFormat.Format.nChannels;
	channelMask = preferredAudioDeviceDetails.OutputFormat.dwChannelMask;

	idSoundVoice::InitSurround( outputChannels, channelMask );

	// ---------------------
	// Initialize the Doom classic sound system.
	// ---------------------
	I_InitSoundHardware( outputChannels, channelMask );

	// ---------------------
	// Create VU Meter Effect
	// ---------------------
	IUnknown * vuMeter = nullptr;
	XAudio2CreateVolumeMeter( &vuMeter, 0 );

	XAUDIO2_EFFECT_DESCRIPTOR descriptor = {};
	descriptor.InitialState = true;
	descriptor.OutputChannels = outputChannels;
	descriptor.pEffect = vuMeter;

	XAUDIO2_EFFECT_CHAIN chain = {};
	chain.EffectCount = 1;
	chain.pEffectDescriptors = &descriptor;

	pMasterVoice->SetEffectChain( &chain );

	vuMeter->Release();

	// ---------------------
	// Create VU Meter Graph
	// ---------------------

	vuMeterRMS = console->CreateGraph( outputChannels );
	vuMeterPeak = console->CreateGraph( outputChannels );
	vuMeterRMS->Enable( false );
	vuMeterPeak->Enable( false );

	memset( vuMeterPeakTimes, 0, sizeof( vuMeterPeakTimes ) );

	vuMeterPeak->SetFillMode( idDebugGraph::GRAPH_LINE );
	vuMeterPeak->SetBackgroundColor( idVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );

	vuMeterRMS->AddGridLine( 0.500f, idVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	vuMeterRMS->AddGridLine( 0.250f, idVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	vuMeterRMS->AddGridLine( 0.125f, idVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );

	const char * channelNames[] = { "L", "R", "C", "S", "Lb", "Rb", "Lf", "Rf", "Cb", "Ls", "Rs" };
	for ( size_t i = 0, ci = 0; ci < sizeof(channelNames); ci++ ) {
		if ( ( channelMask & BIT( ci ) ) == 0 ) {
			continue;
		}
		vuMeterRMS->SetLabel( numeric_cast<index_t>(i), channelNames[ ci ] );
		i++;
	}

	// ---------------------
	// Create submix buffer
	// ---------------------
	if ( FAILED( pXAudio2->CreateSubmixVoice( &pSubmixVoice, 1, outputSampleRate, 0, 0, NULL, NULL ) ) ) {
		idLib::FatalError( "Failed to create submix voice" );
	}

	// XAudio doesn't really impose a maximum number of voices
	voices.SetNum( voices.Max() );
	freeVoices.SetNum( voices.Max() );
	zombieVoices.SetNum( 0 );
	for ( size_t i = 0; i < voices.Num(); i++ ) {
		freeVoices[i] = &voices[i];
	}
}

/*
========================
idSoundHardware_XAudio2::Shutdown
========================
*/
void idSoundHardware_XAudio2::Shutdown() {
	for ( size_t i = 0; i < voices.Num(); i++ ) {
		voices[ i ].DestroyInternal();
	}
	voices.Clear();
	freeVoices.Clear();
	zombieVoices.Clear();

	// ---------------------
	// Shutdown the Doom classic sound system.
	// ---------------------
	I_ShutdownSoundHardware();

	if ( pXAudio2 != nullptr) {
		// Unregister the sound engine callback
		pXAudio2->UnregisterForCallbacks( &soundEngineCallback );
	}

	if ( pSubmixVoice != nullptr) {
		pSubmixVoice->DestroyVoice();
		pSubmixVoice = nullptr;
	}
	if ( pMasterVoice != nullptr) {
		// release the vu meter effect
		pMasterVoice->SetEffectChain(nullptr);
		pMasterVoice->DestroyVoice();
		pMasterVoice = nullptr;
	}
	if ( pXAudio2 != nullptr) {
		XAUDIO2_PERFORMANCE_DATA perfData = {};
		pXAudio2->GetPerformanceData( &perfData );
		idLib::Printf( "Final pXAudio2 performanceData: Voices: %d/%d CPU: %.2f%% Mem: %dkb\n", perfData.ActiveSourceVoiceCount, perfData.TotalSourceVoiceCount, numeric_cast<double>(perfData.AudioCyclesSinceLastQuery) / numeric_cast<double>(perfData.TotalCyclesSinceLastQuery), perfData.MemoryUsageInBytes / 1024 );
		pXAudio2->Release();
		pXAudio2 = nullptr;
	}
	if ( vuMeterRMS != nullptr) {
		console->DestroyGraph( vuMeterRMS );
		vuMeterRMS = nullptr;
	}
	if ( vuMeterPeak != nullptr) {
		console->DestroyGraph( vuMeterPeak );
		vuMeterPeak = nullptr;
	}
}

/*
========================
idSoundHardware_XAudio2::AllocateVoice
========================
*/
idSoundVoice * idSoundHardware_XAudio2::AllocateVoice( const idSoundSample * leadinSample, const idSoundSample * loopingSample ) {
	if ( leadinSample == nullptr) {
		return nullptr;
	}
	if ( loopingSample != nullptr) {
		if ( ( leadinSample->format.basic.formatTag != loopingSample->format.basic.formatTag ) || ( leadinSample->format.basic.numChannels != loopingSample->format.basic.numChannels ) ) {
			idLib::Warning( "Leadin/looping format mismatch: %s & %s", leadinSample->GetName(), loopingSample->GetName() );
			loopingSample = nullptr;
		}
	}

	// Try to find a free voice that matches the format
	// But fallback to the last free voice if none match the format
	idSoundVoice * voice = nullptr;
	for ( size_t i = 0; i < freeVoices.Num(); i++ ) {
		if ( freeVoices[i]->IsPlaying() ) {
			continue;
		}
		voice = static_cast<idSoundVoice*>(freeVoices[i]);
		if ( voice->CompatibleFormat( dynamic_cast<idSoundSample_XAudio2*>(const_cast<idSoundSample*>(leadinSample)) ) ) {
			break;
		}
	}
	if ( voice != nullptr) {
		voice->Create( leadinSample, loopingSample );
		freeVoices.Remove( voice );
		return voice;
	}
	
	return nullptr;
}

/*
========================
idSoundHardware_XAudio2::FreeVoice
========================
*/
void idSoundHardware_XAudio2::FreeVoice( idSoundVoice * voice ) {
	voice->Stop();

	// Stop() is asynchronous, so we won't flush buffers until the
	// voice on the zombie channel actually returns !IsPlaying() 
	zombieVoices.Append( voice );
}

/*
========================
idSoundHardware_XAudio2::Update
========================
*/
void idSoundHardware_XAudio2::Update() {
	if ( pXAudio2 == nullptr) {
		ID_TIME_T nowTime = Sys_Milliseconds();
		if ( lastResetTime + 1000 < nowTime ) {
			lastResetTime = nowTime;
			Init();
		}
		return;
	}
	if ( soundSystem->IsMuted() ) {
		pMasterVoice->SetVolume( 0.0f, OPERATION_SET );
	} else {
		pMasterVoice->SetVolume( DBtoLinear( s_volume_dB.GetFloat() ), OPERATION_SET );
	}

	pXAudio2->CommitChanges( XAUDIO2_COMMIT_ALL );

	// IXAudio2SourceVoice::Stop() has been called for every sound on the
	// zombie list, but it is documented as asynchronous, so we have to wait
	// until it actually reports that it is no longer playing.
	for ( size_t i = 0; i < zombieVoices.Num(); i++ ) {
		zombieVoices[i]->FlushSourceBuffers();
		if ( !zombieVoices[i]->IsPlaying() ) {
			freeVoices.Append( zombieVoices[i] );
			zombieVoices.RemoveIndexFast( i );
			i--;
		} else {
			static int playingZombies;
			playingZombies++;
		}
	}

	if ( s_showPerfData.GetBool() ) {
		XAUDIO2_PERFORMANCE_DATA perfData;
		pXAudio2->GetPerformanceData( &perfData );
		idLib::Printf( "Voices: %d/%d CPU: %.2f%% Mem: %dkb\n", perfData.ActiveSourceVoiceCount, perfData.TotalSourceVoiceCount, numeric_cast<double>(perfData.AudioCyclesSinceLastQuery) / numeric_cast<double>(perfData.TotalCyclesSinceLastQuery), perfData.MemoryUsageInBytes / 1024 );
	}

	if ( vuMeterRMS == nullptr) {
		// Init probably hasn't been called yet
		return;
	}

	vuMeterRMS->Enable( s_showLevelMeter.GetBool() );
	vuMeterPeak->Enable( s_showLevelMeter.GetBool() );

	if ( !s_showLevelMeter.GetBool() ) {
		pMasterVoice->DisableEffect( 0 );
		return;
	} else {
		pMasterVoice->EnableEffect( 0 );
	}

	float peakLevels[ 8 ];
	float rmsLevels[ 8 ];

	XAUDIO2FX_VOLUMEMETER_LEVELS levels = {};
	levels.ChannelCount = outputChannels;
	levels.pPeakLevels = peakLevels;
	levels.pRMSLevels = rmsLevels;

	levels.ChannelCount = Min(levels.ChannelCount, 8);

	pMasterVoice->GetEffectParameters( 0, &levels, sizeof( levels ) );

	ID_TIME_T currentTime = Sys_Milliseconds();
	for ( size_t i = 0; i < outputChannels; i++ ) {
		if ( vuMeterPeakTimes[i] < currentTime ) {
			vuMeterPeak->SetValue( numeric_cast<index_t>(i), vuMeterPeak->GetValue( i ) * 0.9f, colorRed );
		}
	}

	float width = 20.0f;
	float height = 200.0f;
	float left = 100.0f;
	float top = 100.0f;

	sscanf( s_meterPosition.GetString(), "%f %f %f %f", &left, &top, &width, &height );

	float channelCountf = numeric_cast<float>(levels.ChannelCount);

	vuMeterRMS->SetPosition( left, top, width * channelCountf, height );
	vuMeterPeak->SetPosition( left, top, width * channelCountf, height );

	for ( uint32 i = 0; i < levels.ChannelCount; i++ ) {
		vuMeterRMS->SetValue( i, rmsLevels[ i ], idVec4( 0.5f, 1.0f, 0.0f, 1.00f ) );
		if ( peakLevels[ i ] >= vuMeterPeak->GetValue( i ) ) {
			vuMeterPeak->SetValue( i, peakLevels[ i ], colorRed );
			vuMeterPeakTimes[i] = currentTime + s_meterTopTime.GetInteger();
		}
	}
}


/*
================================================
idSoundEngineCallback
================================================
*/

/*
========================
idSoundEngineCallback::OnCriticalError
========================
*/
void idSoundEngineCallback::OnCriticalError( HRESULT Error ) {
	soundSystemLocal.SetNeedsRestart();
}
