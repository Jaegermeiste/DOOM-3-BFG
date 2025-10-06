#ifndef __MD5_H__
#define __MD5_H__

/*
===============================================================================

	Calculates a checksum for a block of data
	using the MD5 message-digest algorithm.

===============================================================================
*/

#ifndef _MD_TYPES
#define _MD_TYPES
/* POINTER defines a generic pointer type */
typedef unsigned char* POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;
#endif // _MD_TYPES

struct MD5_CTX {
	UINT4	state[4];
	UINT4	bits[2];
	unsigned char	in[64];
};

void MD5_Init( MD5_CTX *ctx );
void MD5_Update( MD5_CTX *context, unsigned char const *input, size_t inputLen );
void MD5_Final( MD5_CTX *context, unsigned char digest[16] );

unsigned int MD5_BlockChecksum( const void *data, size_t length );

#endif /* !__MD5_H__ */
