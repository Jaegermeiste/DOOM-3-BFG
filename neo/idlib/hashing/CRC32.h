#ifndef __CRC32_H__
#define __CRC32_H__

#pragma once

/*
===============================================================================

	Calculates a checksum for a block of data
	using the CRC-32.

===============================================================================
*/

void CRC32_InitChecksum( uint32 &crcvalue );
void CRC32_UpdateChecksum( uint32 &crcvalue, const void *data, size_t length);
void CRC32_FinishChecksum( uint32 &crcvalue );
uint32 CRC32_BlockChecksum( const void *data, size_t length);

#endif /* !__CRC32_H__ */
