#ifndef __MD5_H__
#define __MD5_H__

#pragma once

/*
===============================================================================

	Calculates a checksum for a block of data
	using the MD5 message-digest algorithm.

===============================================================================
*/

struct MD5_CTX {
	uint32	state[4];
	uint32	bits[2];
	byte	in[64];
};

void MD5_Init( MD5_CTX *ctx );
void MD5_Update( MD5_CTX *context, byte const *input, size_t inputLen );
void MD5_Final( MD5_CTX *context,byte digest[16] );

uint32 MD5_BlockChecksum( const void *data, size_t length );

#endif /* !__MD5_H__ */
