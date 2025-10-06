#ifndef __MD4_H__
#define __MD4_H__

#pragma once

/*
===============================================================================

	Calculates a checksum for a block of data
	using the MD4 message-digest algorithm.

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

unsigned long MD4_BlockChecksum( const void *data, size_t length );

#endif /* !__MD4_H__ */
