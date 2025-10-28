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
#include "../../precompiled.h"

/*
================================================================================================
================================================================================================
*/

constexpr auto MS_VC_EXCEPTION = 0x406D1388;

typedef struct tagTHREADNAME_INFO {
	DWORD dwType;		// Must be 0x1000.
	LPCSTR szName;		// Pointer to name (in user addr space).
	DWORD dwThreadID;	// Thread ID (-1=caller thread).
	DWORD dwFlags;		// Reserved for future use, must be zero.
} THREADNAME_INFO;
/*
========================
Sys_SetThreadName
========================
*/
static void Sys_SetThreadName(const DWORD threadID, const char * name ) {
	THREADNAME_INFO info = {};
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = threadID;
	info.dwFlags = 0;

	__try {
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(DWORD), reinterpret_cast<const ULONG_PTR*>(&info) );
	}
	// this much is just to keep /analyze quiet
	__except( GetExceptionCode() == MS_VC_EXCEPTION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {
		info.dwFlags = 0;
	}
}

/*
========================
Sys_SetCurrentThreadName
========================
*/
void Sys_SetCurrentThreadName( const char * name ) {
	Sys_SetThreadName( GetCurrentThreadId(), name );
}

/*
========================
Sys_CreateThread
========================
*/
uintptr_t Sys_CreateThread(const xthread_t function, void *parms, const xthreadPriority priority, const char *name, core_t core, const size_t stackSize, const bool suspended ) {

	DWORD flags = ( suspended ? CREATE_SUSPENDED : 0 );
	// Without this flag the 'dwStackSize' parameter to CreateThread specifies the "Stack Commit Size"
	// and the "Stack Reserve Size" is set to the value specified at link-time.
	// With this flag the 'dwStackSize' parameter to CreateThread specifies the "Stack Reserve Size"
	// and the �Stack Commit Size� is set to the value specified at link-time.
	// For various reasons (some of which historic) we reserve a large amount of stack space in the
	// project settings. By setting this flag and by specifying 64 kB for the "Stack Commit Size" in
	// the project settings we can create new threads with a much smaller reserved (and committed)
	// stack space. It is very important that the "Stack Commit Size" is set to a small value in
	// the project settings. If it is set to a large value we may be both reserving and committing
	// a lot of memory by setting the STACK_SIZE_PARAM_IS_A_RESERVATION flag. There are some
	// 50 threads allocated for normal game play. If, for instance, the commit size is set to 16 MB
	// then by adding this flag we would be reserving and committing 50 x 16 = 800 MB of memory.
	// On the other hand, if this flag is not set and the "Stack Reserve Size" is set to 16 MB in the
	// project settings, then we would still be reserving 50 x 16 = 800 MB of virtual address space.
	flags |= STACK_SIZE_PARAM_IS_A_RESERVATION;

	DWORD threadId;
	HANDLE handle = CreateThread(nullptr,	// LPSECURITY_ATTRIBUTES lpsa, //-V513
	                             stackSize,
	                             reinterpret_cast<LPTHREAD_START_ROUTINE>(function),
	                             parms,
	                             flags,
	                             &threadId);
	if ( handle == nullptr ) {
		idLib::common->FatalError( "CreateThread error: %i", GetLastError() );
		return static_cast<uintptr_t>(0);
	}
	Sys_SetThreadName( threadId, name );
	if ( priority == THREAD_HIGHEST ) {
		SetThreadPriority( handle, THREAD_PRIORITY_HIGHEST );		//  we better sleep enough to do this
	} else if ( priority == THREAD_ABOVE_NORMAL ) {
		SetThreadPriority( handle, THREAD_PRIORITY_ABOVE_NORMAL );
	} else if ( priority == THREAD_BELOW_NORMAL ) {
		SetThreadPriority( handle, THREAD_PRIORITY_BELOW_NORMAL );
	} else if ( priority == THREAD_LOWEST ) {
		SetThreadPriority( handle, THREAD_PRIORITY_LOWEST );
	}

	// Under Windows, we don't set the thread affinity and let the OS deal with scheduling

	return reinterpret_cast<uintptr_t>(handle);
}


/*
========================
Sys_GetCurrentThreadID
========================
*/
uintptr_t Sys_GetCurrentThreadID() {
	return GetCurrentThreadId();
}

/*
========================
Sys_WaitForThread
========================
*/
void Sys_WaitForThread(const uintptr_t threadHandle ) {
	WaitForSingleObject( reinterpret_cast<HANDLE>(threadHandle), INFINITE );
}

/*
========================
Sys_DestroyThread
========================
*/
void Sys_DestroyThread(const uintptr_t threadHandle ) {
	if ( threadHandle == 0 ) {
		return;
	}
	WaitForSingleObject( reinterpret_cast<HANDLE>(threadHandle), INFINITE );
	CloseHandle( reinterpret_cast<HANDLE>(threadHandle) );
}

/*
========================
Sys_Yield
========================
*/
void Sys_Yield() {
	SwitchToThread();
}

/*
================================================================================================

	Signal

================================================================================================
*/

/*
========================
Sys_SignalCreate
========================
*/
void Sys_SignalCreate( signalHandle_t & handle, const bool manualReset ) {
	handle = CreateEvent(nullptr, manualReset, FALSE, nullptr);
}

/*
========================
Sys_SignalDestroy
========================
*/
void Sys_SignalDestroy( signalHandle_t &handle ) {
	CloseHandle( handle );
}

/*
========================
Sys_SignalRaise
========================
*/
void Sys_SignalRaise( signalHandle_t & handle ) {
	SetEvent( handle );
}

/*
========================
Sys_SignalClear
========================
*/
void Sys_SignalClear( signalHandle_t & handle ) {
	// events are created as auto-reset so this should never be needed
	ResetEvent( handle );
}

/*
========================
Sys_SignalWait
========================
*/
bool Sys_SignalWait( signalHandle_t & handle, const ID_TIME_T timeout ) {
	const DWORD result = WaitForSingleObject( handle, timeout == idSysSignal::WAIT_INFINITE ? INFINITE : numeric_cast<DWORD>(timeout) );
	assert( result == WAIT_OBJECT_0 || ( timeout != idSysSignal::WAIT_INFINITE && result == WAIT_TIMEOUT ) );
	return ( result == WAIT_OBJECT_0 );
}

/*
================================================================================================

	Mutex

================================================================================================
*/

/*
========================
Sys_MutexCreate
========================
*/
void Sys_MutexCreate( mutexHandle_t & handle ) {
#ifndef USE_STL_MUTEX
	InitializeCriticalSection( &handle );
#endif
}

/*
========================
Sys_MutexDestroy
========================
*/
void Sys_MutexDestroy( mutexHandle_t & handle ) {
#ifndef USE_STL_MUTEX
	DeleteCriticalSection( &handle );
#endif
}

/*
========================
Sys_MutexLock
========================
*/
_Success_(return != 0)
bool Sys_MutexLock( mutexHandle_t & handle, const bool blocking ) {
#ifdef USE_STL_MUTEX
	//_Acquires_lock_(handle)
	auto result = handle.try_lock();
	if (result == 0) {
		if (!blocking) {
			return false;
		}
		//_Acquires_lock_(handle)
		handle.lock();
	}
	return true;
#else
	_Acquires_lock_(handle)
	auto result = TryEnterCriticalSection(&handle);
	if ( result == 0 ) {
		if ( !blocking ) {
			return false;
		}
		_Acquires_lock_(handle)
		EnterCriticalSection( &handle );
	}
	return true;
#endif
}

/*
========================
Sys_MutexUnlock
========================
*/
void Sys_MutexUnlock( mutexHandle_t & handle ) {
#ifdef USE_STL_MUTEX
	//_Releases_lock_(handle)
	handle.unlock();
#else
	_Releases_lock_(handle)
	LeaveCriticalSection( &handle );
#endif
}

/*
================================================================================================

	Interlocked Integer

================================================================================================
*/

/*
========================
Sys_InterlockedIncrement
========================
*/
interlockedInt_t Sys_InterlockedIncrement( interlockedInt_t & value ) {
#if defined(ID_WIN32)
	return InterlockedIncrementAcquire( & value );
#elif defined (ID_WIN64)
	return InterlockedIncrementAcquire64(&value);
#endif
}

/*
========================
Sys_InterlockedDecrement
========================
*/
interlockedInt_t Sys_InterlockedDecrement( interlockedInt_t & value ) {
#if defined(ID_WIN32)
	return InterlockedDecrementRelease(&value);
#elif defined (ID_WIN64)
	return InterlockedDecrementRelease64(&value);
#endif
}

/*
========================
Sys_InterlockedAdd
========================
*/
interlockedInt_t Sys_InterlockedAdd( interlockedInt_t & value, const interlockedInt_t i ) {
#if defined(ID_WIN32)
	return InterlockedExchangeAdd( & value, i ) + i;
#elif defined (ID_WIN64)
	return InterlockedExchangeAdd64(&value, i) + i;
#endif
}

/*
========================
Sys_InterlockedSub
========================
*/
interlockedInt_t Sys_InterlockedSub( interlockedInt_t & value, const interlockedInt_t i ) {
#if defined(ID_WIN32)
	return InterlockedExchangeAdd( & value, - i ) - i;
#elif defined (ID_WIN64)
	return InterlockedExchangeAdd64(&value, -i) - i;
#endif
}

/*
========================
Sys_InterlockedExchange
========================
*/
interlockedInt_t Sys_InterlockedExchange( interlockedInt_t & value, const interlockedInt_t exchange ) {
#if defined(ID_WIN32)
	return InterlockedExchange( & value, exchange );
#elif defined (ID_WIN64)
	return InterlockedExchange64(&value, exchange);
#endif
}

/*
========================
Sys_InterlockedCompareExchange
========================
*/
interlockedInt_t Sys_InterlockedCompareExchange( interlockedInt_t & value, const interlockedInt_t comparand, const interlockedInt_t exchange ) {
#if defined(ID_WIN32)
	return InterlockedCompareExchange( & value, exchange, comparand );
#elif defined (ID_WIN64)
	return InterlockedCompareExchange64(&value, exchange, comparand);
#endif
}

/*
================================================================================================

	Interlocked Pointer

================================================================================================
*/

/*
========================
Sys_InterlockedExchangePointer
========================
*/
void *Sys_InterlockedExchangePointer( void *& ptr, void * exchange ) {
	return InterlockedExchangePointer( & ptr, exchange );
}

/*
========================
Sys_InterlockedCompareExchangePointer
========================
*/
void * Sys_InterlockedCompareExchangePointer( void * & ptr, void * comparand, void * exchange ) {
	return InterlockedCompareExchangePointer( & ptr, exchange, comparand );
}
