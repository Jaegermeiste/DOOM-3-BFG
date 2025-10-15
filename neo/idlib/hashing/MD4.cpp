
#pragma hdrstop
#include "../precompiled.h"

/*
   RSA Data Security, Inc., MD4 message-digest algorithm. (RFC1320)
*/

/*

Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD4 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD4 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.

*/

/* MD4 context. */
typedef struct {
	UINT4 state[4];				/* state (ABCD) */
	UINT4 count[2];				/* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64]; 	/* input buffer */
} MD4_CTX;

/* Constants for MD4Transform routine.  */
enum
{
	S11 = 3,
	S12 = 7,
	S13 = 11,
	S14 = 19,
	S21 = 3,
	S22 = 5,
	S23 = 9,
	S24 = 13,
	S31 = 3,
	S32 = 9,
	S33 = 11,
	S34 = 15
};

#ifndef _MD_PADDING
#define _MD_PADDING
static unsigned char PADDING[64] = {
0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
#endif // _MD_PADDING

/* F, G and H are basic MD4 functions. */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG and HH are transformations for rounds 1, 2 and 3 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s) {(a) += F ((b), (c), (d)) + (x); (a) = ROTATE_LEFT ((a), (s));}

#define GG(a, b, c, d, x, s) {(a) += G ((b), (c), (d)) + (x) + (UINT4)0x5a827999; (a) = ROTATE_LEFT ((a), (s));}

#define HH(a, b, c, d, x, s) {(a) += H ((b), (c), (d)) + (x) + (UINT4)0x6ed9eba1; (a) = ROTATE_LEFT ((a), (s));}

/* Encodes input (UINT4) into output (unsigned char). Assumes len is a multiple of 4. */
static void MD4_Encode( unsigned char *output, const UINT4 *input, const size_t len ) {
	unsigned int i, j;

	for ( i = 0, j = 0; j < len; i++, j += 4 ) {
 		output[j] = static_cast<unsigned char>(input[i] & 0xff);
 		output[j+1] = static_cast<unsigned char>((input[i] >> 8) & 0xff);
 		output[j+2] = static_cast<unsigned char>((input[i] >> 16) & 0xff);
 		output[j+3] = static_cast<unsigned char>((input[i] >> 24) & 0xff);
	}
}

/* Decodes input (unsigned char) into output (UINT4). Assumes len is a multiple of 4. */
static void MD4_Decode( UINT4 *output, const unsigned char *input, const size_t len ) {
	unsigned int i, j;

	for ( i = 0, j = 0; j < len; i++, j += 4 ) {
 		output[i] = static_cast<UINT4>(input[j]) | (static_cast<UINT4>(input[j + 1]) << 8) | (static_cast<UINT4>(input[j + 2]) << 16) | (static_cast<UINT4>(input[j + 3]) << 24);
	}
}

/* MD4 basic transformation. Transforms state based on block. */
static void MD4_Transform( UINT4 state[4], const unsigned char block[64] ) {
	UINT4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	MD4_Decode (x, block, 64);

	/* Round 1 */
	FF (a, b, c, d, x[ 0], S11) /* 1 */
	FF (d, a, b, c, x[ 1], S12) /* 2 */
	FF (c, d, a, b, x[ 2], S13) /* 3 */
	FF (b, c, d, a, x[ 3], S14) /* 4 */
	FF (a, b, c, d, x[ 4], S11) /* 5 */
	FF (d, a, b, c, x[ 5], S12) /* 6 */
	FF (c, d, a, b, x[ 6], S13) /* 7 */
	FF (b, c, d, a, x[ 7], S14) /* 8 */
	FF (a, b, c, d, x[ 8], S11) /* 9 */
	FF (d, a, b, c, x[ 9], S12) /* 10 */
	FF (c, d, a, b, x[10], S13) /* 11 */
	FF (b, c, d, a, x[11], S14) /* 12 */
	FF (a, b, c, d, x[12], S11) /* 13 */
	FF (d, a, b, c, x[13], S12) /* 14 */
	FF (c, d, a, b, x[14], S13) /* 15 */
	FF (b, c, d, a, x[15], S14) /* 16 */

	/* Round 2 */
	GG (a, b, c, d, x[ 0], S21) /* 17 */
	GG (d, a, b, c, x[ 4], S22) /* 18 */
	GG (c, d, a, b, x[ 8], S23) /* 19 */
	GG (b, c, d, a, x[12], S24) /* 20 */
	GG (a, b, c, d, x[ 1], S21) /* 21 */
	GG (d, a, b, c, x[ 5], S22) /* 22 */
	GG (c, d, a, b, x[ 9], S23) /* 23 */
	GG (b, c, d, a, x[13], S24) /* 24 */
	GG (a, b, c, d, x[ 2], S21) /* 25 */
	GG (d, a, b, c, x[ 6], S22) /* 26 */
	GG (c, d, a, b, x[10], S23) /* 27 */
	GG (b, c, d, a, x[14], S24) /* 28 */
	GG (a, b, c, d, x[ 3], S21) /* 29 */
	GG (d, a, b, c, x[ 7], S22) /* 30 */
	GG (c, d, a, b, x[11], S23) /* 31 */
	GG (b, c, d, a, x[15], S24) /* 32 */

	/* Round 3 */
	HH (a, b, c, d, x[ 0], S31) /* 33 */
	HH (d, a, b, c, x[ 8], S32) /* 34 */
	HH (c, d, a, b, x[ 4], S33) /* 35 */
	HH (b, c, d, a, x[12], S34) /* 36 */
	HH (a, b, c, d, x[ 2], S31) /* 37 */
	HH (d, a, b, c, x[10], S32) /* 38 */
	HH (c, d, a, b, x[ 6], S33) /* 39 */
	HH (b, c, d, a, x[14], S34) /* 40 */
	HH (a, b, c, d, x[ 1], S31) /* 41 */
	HH (d, a, b, c, x[ 9], S32) /* 42 */
	HH (c, d, a, b, x[ 5], S33) /* 43 */
	HH (b, c, d, a, x[13], S34) /* 44 */
	HH (a, b, c, d, x[ 3], S31) /* 45 */
	HH (d, a, b, c, x[11], S32) /* 46 */
	HH (c, d, a, b, x[ 7], S33) /* 47 */
	HH (b, c, d, a, x[15], S34) /* 48 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	/* Zeroize sensitive information.*/
	memset (x, 0, sizeof (x));
}

/* MD4 initialization. Begins an MD4 operation, writing a new context. */
static void MD4_Init( MD4_CTX *context ) noexcept {
	context->count[0] = context->count[1] = 0;

	/* Load magic initialization constants.*/
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

/* MD4 block update operation. Continues an MD4 message-digest operation, processing another message block, and updating the context. */
static void MD4_Update( MD4_CTX *context, const unsigned char *input, const size_t inputLen ) {
	size_t i;

	/* Compute number of bytes mod 64 */
	unsigned int index = static_cast<unsigned int>((context->count[0] >> 3) & 0x3F);

	/* Update number of bits */
	if ((context->count[0] += (static_cast<UINT4>(inputLen) << 3))< (static_cast<UINT4>(inputLen) << 3)) {
		context->count[1]++;
	}

	context->count[1] += (static_cast<UINT4>(inputLen) >> 29);

	const size_t partLen = 64 - static_cast<size_t>(index);

	/* Transform as many times as possible.*/
	if ( inputLen >= partLen ) {
 		memcpy(&context->buffer[index], input, partLen);
 		MD4_Transform (context->state, context->buffer);

		for ( i = partLen; i + 63 < inputLen; i += 64 ) {
 			MD4_Transform (context->state, &input[i]);
		}

 		index = 0;
	} else {
 		i = 0;
	}

	/* Buffer remaining input */
	memcpy (&context->buffer[index], &input[i], inputLen-i);
}

/* MD4 finalization. Ends an MD4 message-digest operation, writing the message digest and zeroizing the context. */
static void MD4_Final( MD4_CTX *context, unsigned char digest[16] ) {
	unsigned char bits[8];

	/* Save number of bits */
	MD4_Encode( bits, context->count, 8 );

	/* Pad out to 56 mod 64.*/
	const unsigned int index = static_cast<unsigned int>((context->count[0] >> 3) & 0x3f);
	const unsigned int padLen = (index < 56) ? (56 - index) : (120 - index);
	MD4_Update (context, PADDING, padLen);

	/* Append length (before padding) */
	MD4_Update( context, bits, 8 );
	
	/* Store state in digest */
	MD4_Encode( digest, context->state, 16 );

	/* Zero sensitive information.*/
	memset (context, 0, sizeof (*context));
}

/*
===============
MD4_BlockChecksum
===============
*/
unsigned long MD4_BlockChecksum( const void *data, const size_t length ) {
	unsigned long	digest[4] = {};
	MD4_CTX			ctx = {};

	MD4_Init( &ctx );
	MD4_Update( &ctx, static_cast<const unsigned char*>(data), length );
	MD4_Final( &ctx, reinterpret_cast<unsigned char*>(digest) );

	const unsigned long val = digest[0] ^ digest[1] ^ digest[2] ^ digest[3];

	return val;
}
