#ifndef __MD4_H__
#define __MD4_H__

#pragma once

/*
===============================================================================

	Calculates a checksum for a block of data
	using the MD4 message-digest algorithm.

===============================================================================
*/

uint32 MD4_BlockChecksum( const void *data, size_t length );

#endif /* !__MD4_H__ */
