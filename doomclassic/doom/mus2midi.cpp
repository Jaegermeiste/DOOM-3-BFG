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

#include "Precompiled.h"
#include "globaldata.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include "idlib/sys/sys_defines.h"
// mus header


// reads a variable length integer
static uint64 ReadVarLen( const byte* buffer ) {
	uint64 value = 0;

	if ((value = *buffer++) & 0x80) {
		byte c = 0;
		value &= 0x7f;
		do  {
			value = (value << 7) + ((c = *buffer++) & 0x7f);
		}  while (c & 0x80);
	}

	return value;
}

// Writes a variable length integer to a buffer, and returns bytes written
static size_t WriteVarLen( uint64 value, byte* out ) 
{
	size_t count = 0;

	uint64 buffer = value & 0x7f;
	while ((value >>= 7) > 0) {
		buffer <<= 8;
		buffer += 0x80;
		buffer += (value & 0x7f);
	}

	while (true) {
		++count;
		*out = static_cast<byte>(buffer);
		++out;
		if (buffer & 0x80)
		{
			buffer >>= 8;
		}
		else
		{
			break;
		}
	}

	return count;
}

// writes a byte, and returns the buffer
static inline byte* WriteByte( byte* buf, const byte b )
{
	byte* bufferPtr = buf;
	++*bufferPtr = b;
	return bufferPtr;
}

static inline byte* WriteUInt16( void* b, const uint16 i16 )
{
	byte* bufferPtr = static_cast<byte*>(b);
	++*bufferPtr = (i16 >> 8);
	++*bufferPtr = (i16 & 0x00FF);
	return bufferPtr;
}

static inline byte* WriteUInt32( void* b, const uint32 i32 )
{
	byte* bufferPtr = static_cast<byte*>(b);
	++*bufferPtr = (i32 & 0xff000000) >> 24;
	++*bufferPtr = (i32 & 0x00ff0000) >> 16;
	++*bufferPtr = (i32 & 0x0000ff00) >> 8;
	++*bufferPtr = (i32 & 0x000000ff);
	return bufferPtr;
}

static inline byte* WriteUInt64( void* b, const uint64 i64 )
{
	byte* bufferPtr = static_cast<byte*>(b);
	++*bufferPtr = (i64 & 0xff00000000000000) >> 56;
	++*bufferPtr = (i64 & 0x00ff000000000000) >> 48;
	++*bufferPtr = (i64 & 0x0000ff0000000000) >> 40;
	++*bufferPtr = (i64 & 0x000000ff00000000) >> 32;
	++*bufferPtr = (i64 & 0x00000000ff000000) >> 24;
	++*bufferPtr = (i64 & 0x0000000000ff0000) >> 16;
	++*bufferPtr = (i64 & 0x000000000000ff00) >> 8;
	++*bufferPtr = (i64 & 0x00000000000000ff);
	return bufferPtr;
}

// Format - 0(1 track only), 1(1 or more tracks, each play same time), 2(1 or more, each play seperatly)
static void Midi_CreateHeader(MidiHeaderChunk_t* header, const short format, const short track_count, const short division)
{
	WriteUInt32( header->name, static_cast<uint32>('MThd'));
	WriteUInt32( &header->length, 6);
	WriteUInt16( &header->format, format);
	WriteUInt16( &header->ntracks, track_count);
	WriteUInt16( &header->division, division);
}

static unsigned char* Midi_WriteTempo( byte* buffer, const int32 tempo)
{
	buffer = WriteByte(buffer, 0x00);	// delta time
	buffer = WriteByte(buffer, 0xff);	// sys command
	buffer = WriteUInt16(buffer, 0x5103); // command - set tempo
	
	buffer = WriteByte(buffer, tempo & 0x000000ff);
	buffer = WriteByte(buffer, (tempo & 0x0000ff00) >> 8);
	buffer = WriteByte(buffer, (tempo & 0x00ff0000) >> 16);

	return buffer;
}

static bool Midi_UpdateBytesWritten( size_t* bytes_written, const size_t to_add, const size_t max)
{
	*bytes_written += to_add;

	if (max && *bytes_written > max)
	{
		assert(0);
		return false;
	}
	return true;
}

static constexpr auto MidiMap[] = 
{
	0,				// prog change
	0,				// bank sel
	1,	    //2		// mod pot
	0x07,	//3		// volume
	0x0A,	//4		// pan pot
	0x0B,	//5		// expression pot
	0x5B,	//6		// reverb depth
	0x5D,	//7		// chorus depth
	0x40,	//8		// sustain pedal
	0x43,	//9		// soft pedal
	0x78,	//10	// all sounds off
	0x7B,	//11	// all notes off
	0x7E,	//12	// mono(use numchannels + 1)
	0x7F,	//13	// poly
	0x79,	//14	// reset all controllers
};

// The MUS data is stored in little-endian.
namespace {
	uint16 LittleToNative( const uint16 value ) {
		return value;
	}
}

static bool Mus2Midi(byte* bytes, byte* out, size_t* len)
{
	if (bytes && out && len)
	{
		// mus header and instruments
		MUSheader_t header = {};

		// current position in read buffer
		byte* cur = bytes, * end = nullptr;

		// Midi header(format 0)
		MidiHeaderChunk_t midiHeader = {};
		// Midi track header, only 1 needed(format 0)
		MidiTrackChunk_t midiTrackHeader = {};
		// Stores the position of the midi track header(to change the size)
		byte* midiTrackHeaderOut = nullptr;

		// Delta time for midi event
		ID_TIME_T delta_time = 0;
		size_t i = 0;
		int8 channel_volume[MIDI_MAXCHANNELS] = {};
		size_t bytes_written = 0;
		int8 channelMap[MIDI_MAXCHANNELS] = {};
		index_t currentChannel = 0;

		// read the mus header
		memcpy(&header, cur, sizeof(header));
		cur += sizeof(header);

		header.scoreLen = LittleToNative(header.scoreLen);
		header.scoreStart = LittleToNative(header.scoreStart);
		header.channels = LittleToNative(header.channels);
		header.sec_channels = LittleToNative(header.sec_channels);
		header.instrCnt = LittleToNative(header.instrCnt);
		header.dummy = LittleToNative(header.dummy);

		// only 15 supported
		if (header.channels > MIDI_MAXCHANNELS - 1)
		{
			return false;
		}

		// Map channel 15 to 9(percussions)
		for (i = 0; i < MIDI_MAXCHANNELS; ++i) {
			channelMap[i] = -1;
			channel_volume[i] = 0x40;
		}
		channelMap[15] = 9;

		// Get current position, and end of position
		cur = bytes + header.scoreStart;
		end = cur + header.scoreLen;

		// Write out midi header
		Midi_CreateHeader(&midiHeader, 0, 1, 0x0059);
		Midi_UpdateBytesWritten(&bytes_written, MIDIHEADERSIZE, *len);
		memcpy(out, &midiHeader, MIDIHEADERSIZE);	// cannot use sizeof(packs it to 16 bytes)
		out += MIDIHEADERSIZE;

		// Store this position, for later filling in the midiTrackHeader
		Midi_UpdateBytesWritten(&bytes_written, sizeof(midiTrackHeader), *len);
		midiTrackHeaderOut = out;
		out += sizeof(midiTrackHeader);

		// microseconds per quarter note (yikes)
		Midi_UpdateBytesWritten(&bytes_written, 7, *len);
		out = Midi_WriteTempo(out, 0x001aa309);

		// Percussion channel starts out at full volume
		Midi_UpdateBytesWritten(&bytes_written, 4, *len);
		out = WriteByte(out, 0x00);
		out = WriteByte(out, 0xB9);
		out = WriteByte(out, 0x07);
		out = WriteByte(out, 127);

		// Main Loop
		while (cur < end) {
			index_t channel = 0;
			byte event = 0;
			byte temp_buffer[32] = {};	// temp buffer for current iterator
			byte* out_local = temp_buffer;
			byte status = 0, bit1 = 0, bit2 = 0, bitc = 2;

			// Read in current bit
			event = *cur++;
			channel = (event & 15);		// current channel

			// Write variable length delta time
			out_local += WriteVarLen(numeric_cast<uint32>(delta_time), out_local);

			if (channelMap[channel] < 0) {
				// Set all channels to 127 volume
				out_local = WriteByte(out_local, 0xB0 + numeric_cast<byte>(currentChannel));
				out_local = WriteByte(out_local, 0x07);
				out_local = WriteByte(out_local, 127);
				out_local = WriteByte(out_local, 0x00);

				channelMap[channel] = numeric_cast<int8>(++currentChannel);
				if (currentChannel == 9)
				{
					++currentChannel;
				}
			}

			status = channelMap[channel];

			// Handle ::g->events
			switch ((event & 122) >> 4)
			{
			default:
				assert(0);
				break;
			case MUSEVENT_KEYOFF:
				status |= 0x80;
				bit1 = *cur++;
				bit2 = 0x40;
				break;
			case MUSEVENT_KEYON:
				status |= 0x90;
				bit1 = *cur & 127;
				if (*cur++ & 128)	// volume bit?
				{
					channel_volume[channelMap[channel]] = numeric_cast<int8>(++*cur);
				}
				bit2 = channel_volume[channelMap[channel]];
				break;
			case MUSEVENT_PITCHWHEEL:
				status |= 0xE0;
				bit1 = (*cur & 1) >> 6;
				bit2 = (*cur++ >> 1) & 127;
				break;
			case MUSEVENT_CHANNELMODE:
				status |= 0xB0;
				assert(*cur < std::size(MidiMap));
				bit1 = MidiMap[*cur++];
				bit2 = (*cur++ == 12) ? numeric_cast<byte>(header.channels + 1) : 0x00;
				break;
			case MUSEVENT_CONTROLLERCHANGE:
				if (*cur == 0) {
					cur++;
					status |= 0xC0;
					bit1 = *cur++;
					bitc = 1;
				}
				else {
					status |= 0xB0;
					assert(*cur < std::size(MidiMap));
					bit1 = MidiMap[*cur++];
					bit2 = *cur++;
				}
				break;
			case 5:	// Unknown
				assert(0);
				break;
			case MUSEVENT_END:	// End
				status = 0xff;
				bit1 = 0x2f;
				bit2 = 0x00;
				assert(cur == end);
				break;
			case 7:	// Unknown
				assert(0);
				break;
			}

			// Write it out
			out_local = WriteByte(out_local, status);
			out_local = WriteByte(out_local, bit1);
			if (bitc == 2)
			{
				out_local = WriteByte(out_local, bit2);
			}


			// Write out temp stuff
			if (out_local != temp_buffer)
			{
				Midi_UpdateBytesWritten(&bytes_written, out_local - temp_buffer, *len);
				memcpy(out, temp_buffer, out_local - temp_buffer);
				out += out_local - temp_buffer;
			}

			if (event & 128) {
				delta_time = 0;
				do {
					delta_time = delta_time * 128 + (*cur & 127);
				} while ((*cur++ & 128));
			}
			else {
				delta_time = 0;
			}
		}

		// Write out track header
		WriteUInt32(midiTrackHeader.name, static_cast<uint32>('MTrk'));
		WriteUInt32(&midiTrackHeader.length, out - midiTrackHeaderOut - sizeof(midiTrackHeader));
		memcpy(midiTrackHeaderOut, &midiTrackHeader, sizeof(midiTrackHeader));

		// Store length written
		*len = bytes_written;
		/*{
			FILE* file = f o pen("d:\\test.midi", "wb");
			fwrite(midiTrackHeaderOut - sizeof(MidiHeaderChunk_t), bytes_written, 1, file);
			fclose(file);
		}*/
		return true;
	}

	return false;
}

