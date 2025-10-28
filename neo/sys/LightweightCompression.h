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
#ifndef __LIGHTWEIGHT_COMPRESSION_H__
#define __LIGHTWEIGHT_COMPRESSION_H__

		
struct lzwCompressionData_t {
	static constexpr size_t	LZW_DICT_BITS	= 12;
	static constexpr size_t	LZW_DICT_SIZE	= 1 << LZW_DICT_BITS;
	
	uint8					dictionaryK[LZW_DICT_SIZE];
	uint16					dictionaryW[LZW_DICT_SIZE];

	int						nextCode;
	int						codeBits;

	int						codeWord;
	
	uint64					tempValue;
	int						tempBits;
	int						bytesWritten;
};

/*
========================
idLZWCompressor
Simple lzw based encoder/decoder
========================
*/
class idLZWCompressor {
public:
	idLZWCompressor( lzwCompressionData_t * lzwData_ ) : lzwData( lzwData_ ) {}

	static constexpr int	LZW_BLOCK_SIZE	= ( 1 << 15 );
	static constexpr int	LZW_START_BITS	= 9;
	static constexpr int	LZW_FIRST_CODE	= ( 1 << ( LZW_START_BITS - 1 ) );

	void	Start( uint8 * data_, size_t maxSize, bool append = false );
	int		ReadBits( int bits );
	int		WriteChain( int code );
	void	DecompressBlock();
	void	WriteBits( uint32 value, int bits );
	byte	ReadByte( bool ignoreOverflow = false );
	void	WriteByte( byte value );
	[[nodiscard]] int		Lookup( int w, int k ) const;
	int		AddToDict( int w, int k );
	bool	BumpBits();
	int		End();

	[[nodiscard]] size_t		Length() const { return lzwData->bytesWritten; }
	[[nodiscard]] size_t		GetReadCount() const { return bytesRead; }

	void	Save();
	void	Restore() const;

	[[nodiscard]] bool	IsOverflowed() const { return overflowed; }
	
	size_t	Write( const void * data, const size_t length ) {
		const auto src = static_cast<const uint8*>(data);
		
		for ( size_t i = 0; i < length && !IsOverflowed(); i++ ) {
			WriteByte( src[i] );
		}
		
		return length;
	}

	size_t	Read( void * data, const size_t length, const bool ignoreOverflow = false ) {
		uint8 * src = static_cast<uint8*>(data);
		
		for ( size_t i = 0; i < length; i++ ) {
			const auto byte = ReadByte( ignoreOverflow );
			
			if ( byte == -1 ) {
				return i;
			}
			
			src[i] = static_cast<uint8>(byte);
		}
		
		return length;
	}

	size_t	WriteR( const void * data, const size_t length ) {
		const auto src = static_cast<const uint8*>(data);
		
		for ( size_t i = 0; i < length && !IsOverflowed(); i++ ) {
			WriteByte( src[length - i - 1] );
		}
		
		return length;
	}

	size_t	ReadR( void * data, const size_t length, const bool ignoreOverflow = false ) {
		uint8 * src = static_cast<uint8*>(data);
		
		for ( size_t i = 0; i < length; i++ ) {
			const auto byte = ReadByte( ignoreOverflow );
			
			if ( byte == -1 ) {
				return i;
			}
			
			src[length - i - 1] = static_cast<uint8>(byte);
		}
		
		return length;
	}

	template<class type> ID_INLINE size_t WriteAgnostic( const type & c ) {
		return Write( &c, sizeof( c ) );
	}

	template<class type> ID_INLINE size_t ReadAgnostic( type & c, const bool ignoreOverflow = false ) {
		size_t r = Read( &c, sizeof( c ), ignoreOverflow );
		return r;
	}

	static constexpr int DICTIONARY_HASH_BITS	= 10;
	static constexpr int MAX_DICTIONARY_HASH	= 1 << DICTIONARY_HASH_BITS;
	static constexpr int HASH_MASK				= MAX_DICTIONARY_HASH - 1;
	
private:
	void ClearHash();
		
	lzwCompressionData_t *	lzwData;
	uint16					hash[MAX_DICTIONARY_HASH];
	uint16					nextHash[lzwCompressionData_t::LZW_DICT_SIZE];
	
	// Used by DecompressBlock
	int					oldCode;
		
	uint8 *				data;		// Read/write
	int					maxSize;
	bool				overflowed;

	// For reading
	int					bytesRead;
	uint8				block[LZW_BLOCK_SIZE];
	size_t				blockSize;
	index_t				blockIndex;
	
	// saving/restoring when overflow (when writing). 
	// Must call End directly after restoring (dictionary is bad so can't keep writing)
	size_t				savedBytesWritten;
	size_t				savedCodeWord;
	size_t				saveCodeBits;
	uint64				savedTempValue;
	int					savedTempBits;
};

/*
========================
idZeroRunLengthCompressor
Simple zero based run length encoder/decoder
========================
*/
class idZeroRunLengthCompressor {
public:
	idZeroRunLengthCompressor() noexcept : zeroCount( 0 ), destStart(nullptr) {
	}
	
	void Start( uint8 * dest_, idLZWCompressor * comp_, size_t maxSize_ );
	bool WriteRun();
	bool WriteByte( uint8 value );
	byte ReadByte();
	void ReadBytes( byte * dest, int count );
	void WriteBytes( uint8 * src, int count );
	int End();

	[[nodiscard]] int CompressedSize() const { return compressed; }

private:
	int ReadInternal();

	int					zeroCount;		// Number of pending zeroes
	idLZWCompressor *	comp;
	uint8 *				destStart;
	uint8 *				dest;
	int					compressed;		// Compressed size
	int					maxSize;
};

#endif // __LIGHTWEIGHT_COMPRESSION_H__
