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
#ifndef __SYS_THREADING_H__
#define __SYS_THREADING_H__

#pragma once
#include "idlib/precompiled.h"

#ifndef __TYPEINFOGEN__

#define USE_STL_MUTEX

#ifdef USE_STL_MUTEX
#include <mutex>
#endif
#include <idlib/precompiled.h>

/*
================================================================================================

	Platform specific mutex, signal, atomic integer and memory barrier.

================================================================================================
*/

#ifdef USE_STL_MUTEX
	typedef std::recursive_mutex	mutexHandle_t;
#else
	typedef CRITICAL_SECTION		mutexHandle_t;
#endif
	typedef HANDLE					signalHandle_t;
#if defined( ID_WIN32 )
	typedef LONG					interlockedInt_t;
#elif defined ( ID_WIN64 )
	typedef LONG64					interlockedInt_t;
#endif

	// _ReadWriteBarrier() does not translate to any instructions but keeps the compiler
	// from reordering read and write instructions across the barrier.
	// MemoryBarrier() inserts and CPU instruction that keeps the CPU from reordering reads and writes.
	#pragma intrinsic(_ReadWriteBarrier)
	#define SYS_MEMORYBARRIER		_ReadWriteBarrier(); MemoryBarrier()





/*
================================================================================================

	Platform specific thread local storage.
	Can be used to store either a pointer or an integer.

================================================================================================
*/


	class idSysThreadLocalStorage {
	public:
		DWORD	tlsIndex;

		idSysThreadLocalStorage() noexcept {
			tlsIndex = TlsAlloc();
		}

		explicit idSysThreadLocalStorage( const ptrdiff_t &val ) {
			tlsIndex = TlsAlloc();
			TlsSetValue( tlsIndex, reinterpret_cast<LPVOID>(val) );
		}
		~idSysThreadLocalStorage() {
			TlsFree( tlsIndex );
		}
		operator ptrdiff_t() const
		{
			return reinterpret_cast<ptrdiff_t>(TlsGetValue(tlsIndex));
		}
		const ptrdiff_t & operator = ( const ptrdiff_t &val ) const
		{
			TlsSetValue( tlsIndex, reinterpret_cast<LPVOID>(val) );
			return val;
		}

		template <std::integral T>
		constexpr const T& operator=(const T& val)
		{
			AssignInteger(val);
			return val;
		}

		template <std::integral T>
		constexpr idSysThreadLocalStorage(const T val)
		{
			AssignInteger(val);
		}
		bool operator==(const idSysThreadLocalStorage& other) const
		{
			return tlsIndex == other.tlsIndex;
		}

		template <std::integral T>
		constexpr bool operator==(const T val) const noexcept
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (val < 0)
				{
					return false;
				}
			}

			const auto stored_value = reinterpret_cast<unsigned long long>(TlsGetValue(tlsIndex));
			const auto val_wide = static_cast<unsigned long long>(static_cast<std::make_unsigned_t<T>>(val));

			// If T is narrower than a pointer (e.g., 32-bit T on 64-bit),
			// assert that no high bits would be lost (faithful to reinterpret_cast equality).
			if constexpr (sizeof(T) < sizeof(LPVOID)) {
				constexpr unsigned bit_diff = static_cast<unsigned>(sizeof(LPVOID) * 8 - sizeof(T) * 8);
				assert((stored_value >> bit_diff) == 0 &&
					"Pointer doesn't fit in T (would truncate on 64-bit)");
			}

			return stored_value == val_wide;
		}

		template <std::integral T>
		friend constexpr bool operator==(const T val, const idSysThreadLocalStorage& rhs) noexcept
		{
			return rhs == val;
		}

	private:
		template <std::integral T>
		constexpr void AssignInteger(const T& val)
		{
			if constexpr (std::is_signed_v<T>) {
				assert(val >= 0); // Value must be non-negative
			}

			const auto val_wide = static_cast<unsigned long long>(val);

#if defined(ID_WIN32)
			assert(val_wide <= static_cast<unsigned long long>((std::numeric_limits<DWORD>::max)())); // Value exceeds DWORD max
#endif

			tlsIndex = TlsAlloc();
			TlsSetValue(tlsIndex, reinterpret_cast<LPVOID>(val_wide));
		}
	};

#define ID_TLS idSysThreadLocalStorage


#endif // __TYPEINFOGEN__

/*
================================================================================================

	Platform independent threading functions.

================================================================================================
*/

enum core_t {
	CORE_ANY = -1,
	CORE_0A,
	CORE_0B,
	CORE_1A,
	CORE_1B,
	CORE_2A,
	CORE_2B
};

typedef unsigned int (*xthread_t)( void * );

enum xthreadPriority {
	THREAD_LOWEST,
	THREAD_BELOW_NORMAL,
	THREAD_NORMAL,
	THREAD_ABOVE_NORMAL,
	THREAD_HIGHEST
};

constexpr size_t DEFAULT_THREAD_STACK_SIZE = ( 256ULL * 1024ULL );

// on win32, the threadID is NOT the same as the threadHandle
uintptr_t			Sys_GetCurrentThreadID();

// returns a threadHandle
uintptr_t			Sys_CreateThread( xthread_t function, void *parms, xthreadPriority priority, 
									  const char *name, core_t core, int stackSize = DEFAULT_THREAD_STACK_SIZE, 
									  bool suspended = false );

void				Sys_WaitForThread( uintptr_t threadHandle );
void				Sys_DestroyThread( uintptr_t threadHandle );
void				Sys_SetCurrentThreadName( const char *name );

void				Sys_SignalCreate( signalHandle_t & handle, bool manualReset );
void				Sys_SignalDestroy( signalHandle_t & handle );
void				Sys_SignalRaise( signalHandle_t & handle );
void				Sys_SignalClear( signalHandle_t & handle );
bool				Sys_SignalWait( signalHandle_t & handle, /*ID_TIME_T*/ int64 timeout );

void				Sys_MutexCreate( mutexHandle_t & handle );
void				Sys_MutexDestroy( mutexHandle_t & handle );
bool				Sys_MutexLock( mutexHandle_t & handle, bool blocking );
void				Sys_MutexUnlock( mutexHandle_t & handle );

interlockedInt_t	Sys_InterlockedIncrement( interlockedInt_t & value );
interlockedInt_t	Sys_InterlockedDecrement( interlockedInt_t & value );

interlockedInt_t	Sys_InterlockedAdd( interlockedInt_t & value, interlockedInt_t i );
interlockedInt_t	Sys_InterlockedSub( interlockedInt_t & value, interlockedInt_t i );

interlockedInt_t	Sys_InterlockedExchange( interlockedInt_t & value, interlockedInt_t exchange );
interlockedInt_t	Sys_InterlockedCompareExchange( interlockedInt_t & value, interlockedInt_t comparand, interlockedInt_t exchange );

void *				Sys_InterlockedExchangePointer( void * & ptr, void * exchange );
void *				Sys_InterlockedCompareExchangePointer( void * & ptr, void * comparand, void * exchange );

void				Sys_Yield();

#ifndef USE_STL_MUTEX
enum criticalSections_e{
	CRITICAL_SECTION_ZERO = 0,
	CRITICAL_SECTION_ONE,
	CRITICAL_SECTION_TWO,
	CRITICAL_SECTION_THREE,
	MAX_CRITICAL_SECTIONS
};
#endif

#endif	// !__SYS_THREADING_H__
