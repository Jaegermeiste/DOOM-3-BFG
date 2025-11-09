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
#include "../../idlib/precompiled.h"

#include "win_local.h"
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#undef StrCmpN
#undef StrCmpNI
#undef StrCmpI
#include <atlbase.h>

#include <comdef.h>
#include <comutil.h>
#include <Wbemidl.h>

#include "sys/sys_lobby_backend.h"

#pragma comment (lib, "wbemuuid.lib")

#pragma warning(disable:4740)	// warning C4740: flow in or out of inline asm code suppresses global optimization

#if defined(ID_WIN64)

constexpr auto EPOCH_DIFF_100NS = 116444736000000000ULL;

// Convert FILETIME (100ns ticks since 1601-01-01) to Unix epoch milliseconds.
[[nodiscard]] static inline uint64 Sys_FiletimeToUnixMs(const uint64 filetime100ns) noexcept {
	// Guard against dates before 1970 if ever needed; clamp to 0.
	if (filetime100ns <= EPOCH_DIFF_100NS)
	{
		return 0ULL;
	}
	const uint64 unix100ns = filetime100ns - EPOCH_DIFF_100NS;
	return unix100ns / 10000ULL; // 100 ns -> ms
}

// High-precision, wall-clock UTC in milliseconds since Unix epoch.
// Uses GetSystemTimePreciseAsFileTime (Win8+). Falls back to GetSystemTimeAsFileTime if needed.
[[nodiscard]] static inline uint64 Sys_RealtimeMsUTC() noexcept {
	FILETIME ft = {};
	// On Windows 11 this is always available, but keep a tiny fallback for robustness.
	// If you prefer to avoid the (very small) indirect call below, call the API directly.
	static auto pGetPrecise = []() -> void (WINAPI*)(LPFILETIME) {
		const HMODULE k = GetModuleHandleW(L"kernel32.dll");
		if (!k)
		{
			return nullptr;
		}
		return reinterpret_cast<void (WINAPI*)(LPFILETIME)>(GetProcAddress(k, "GetSystemTimePreciseAsFileTime"));
	}();

	if (pGetPrecise) {
		pGetPrecise(&ft);
	}
	else {
		GetSystemTimeAsFileTime(&ft);
	}

	ULARGE_INTEGER uli = {};
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;

	return Sys_FiletimeToUnixMs(uli.QuadPart);
}

#endif // #if defined(ID_WIN64)

/*
================
Sys_Milliseconds
================
*/
ID_TIME_T Sys_Milliseconds() {
#if defined(ID_WIN64)
	static const double sys_perfFreqInv = [] {
		LARGE_INTEGER perfFreq = {};
		QueryPerformanceFrequency(&perfFreq);
		return 1000.0 / static_cast<double>(perfFreq.QuadPart);
		}();

	LARGE_INTEGER tickCount = {};
	QueryPerformanceCounter(&tickCount);

	return static_cast<ID_TIME_T>(static_cast<double>(tickCount.QuadPart) * sys_perfFreqInv);
#else
	static auto sys_timeBase = timeGetTime();
	return static_cast<ID_TIME_T>(timeGetTime()) - sys_timeBase;
#endif // #if defined(ID_WIN64)
}

/*
========================
Sys_Microseconds
========================
*/
ID_MICROSEC_T Sys_Microseconds() {
	static ID_MICROSEC_T ticksPerMicrosecondTimes1024 = 0;

	if ( ticksPerMicrosecondTimes1024 == 0 ) {
		ticksPerMicrosecondTimes1024 = ( numeric_cast<ID_MICROSEC_T>(Sys_ClockTicksPerSecond()) << 10 ) / 1000000;
		assert( ticksPerMicrosecondTimes1024 > 0 );
	}

	return (numeric_cast<ID_MICROSEC_T>(Sys_GetClockTicks()) << 10) / ticksPerMicrosecondTimes1024;
}

/*
================
Sys_GetSystemRam

	returns amount of physical memory in MB
================
*/
size_t Sys_GetSystemRam() {
	MEMORYSTATUSEX statex = {};
	statex.dwLength = sizeof ( statex );
	GlobalMemoryStatusEx (&statex);
	auto physRam = statex.ullTotalPhys / ( 1024ULL * 1024ULL );
	// HACK: For some reason, ullTotalPhys is sometimes off by a meg or two, so we round up to the nearest 16 megs
	physRam = ( physRam + 8 ) & ~15;
	return numeric_cast<size_t>(physRam);
}


/*
================
Sys_GetDriveFreeSpace
returns in megabytes
================
*/
size_t Sys_GetDriveFreeSpace( const char *path ) {
	size_t ret = Sys_GetDriveFreeSpaceInBytes(path);
	ret = ret / (1024ULL * 1024ULL);
	return ret;
}

/*
========================
Sys_GetDriveFreeSpaceInBytes
========================
*/
size_t Sys_GetDriveFreeSpaceInBytes( const char * path ) {
	DWORDLONG lpFreeBytesAvailable = 0;
	DWORDLONG lpTotalNumberOfBytes = 0;
	DWORDLONG lpTotalNumberOfFreeBytes = 0;
	size_t ret = 1;
	//FIXME: see why this is failing on some machines
	if ( ::GetDiskFreeSpaceEx( path, reinterpret_cast<PULARGE_INTEGER>(&lpFreeBytesAvailable), reinterpret_cast<PULARGE_INTEGER>(&lpTotalNumberOfBytes), reinterpret_cast<PULARGE_INTEGER>(&lpTotalNumberOfFreeBytes) ) ) {
		ret = lpFreeBytesAvailable;
	}
	return ret;
}

/*
================
Sys_GetVideoRam
returns in megabytes
================
*/
size_t Sys_GetVideoRam() {
	size_t retSize = 64;

	CComPtr<IWbemLocator> spLoc = nullptr;
	HRESULT hr = CoCreateInstance( CLSID_WbemLocator, nullptr, CLSCTX_SERVER, IID_IWbemLocator, ( LPVOID * ) &spLoc );
	if ( hr != S_OK || spLoc == nullptr) {
		return retSize;
	}

	CComBSTR bstrNamespace( _T( "\\\\.\\root\\CIMV2" ) );
	CComPtr<IWbemServices> spServices;

	// Connect to CIM
	hr = spLoc->ConnectServer( bstrNamespace, nullptr, nullptr, nullptr, NULL, nullptr, nullptr, &spServices );
	if ( hr != WBEM_S_NO_ERROR ) {
		return retSize;
	}

	// Switch the security level to IMPERSONATE so that provider will grant access to system-level objects.  
	hr = CoSetProxyBlanket( spServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE );
	if ( hr != S_OK ) {
		return retSize;
	}

	// Get the vid controller
	CComPtr<IEnumWbemClassObject> spEnumInst = nullptr;
	hr = spServices->CreateInstanceEnum( CComBSTR( "Win32_VideoController" ), WBEM_FLAG_SHALLOW, nullptr, &spEnumInst ); 
	if ( hr != WBEM_S_NO_ERROR || spEnumInst == nullptr) {
		return retSize;
	}

	ULONG uNumOfInstances = 0;
	CComPtr<IWbemClassObject> spInstance = nullptr;
	hr = spEnumInst->Next( 10000, 1, &spInstance, &uNumOfInstances );

	if ( hr == S_OK && spInstance ) {
		// Get properties from the object
		CComVariant varSize;
		hr = spInstance->Get( CComBSTR( _T( "AdapterRAM" ) ), 0, &varSize, nullptr, nullptr );
		if ( hr == S_OK ) {
			retSize = varSize.intVal / ( 1024 * 1024 );
			if ( retSize == 0 ) {
				retSize = 64;
			}
		}
	}
	return retSize;
}

/*
================
Sys_GetCurrentMemoryStatus

	returns OS mem info
	all values are in kB except the memoryload
================
*/
void Sys_GetCurrentMemoryStatus( sysMemoryStats_t &stats ) {
	MEMORYSTATUSEX statex = {};

	statex.dwLength = sizeof( statex );
	GlobalMemoryStatusEx( &statex );

	memset( &stats, 0, sizeof( stats ) );

	stats.memoryLoad = statex.dwMemoryLoad;

	stats.totalPhysical = statex.ullTotalPhys >> 20;
	stats.availPhysical = statex.ullAvailPhys >> 20;
	stats.availPageFile = statex.ullAvailPageFile >> 20;
	stats.totalPageFile = statex.ullTotalPageFile >> 20;
	stats.totalVirtual = statex.ullTotalVirtual >> 20;
	stats.availVirtual = statex.ullAvailVirtual >> 20;
	stats.availExtendedVirtual = statex.ullAvailExtendedVirtual >> 20;
}

/*
================
Sys_LockMemory
================
*/
bool Sys_LockMemory( void *ptr, const size_t bytes ) {
	return ( VirtualLock( ptr, static_cast<SIZE_T>(bytes) ) != FALSE );
}

/*
================
Sys_UnlockMemory
================
*/
bool Sys_UnlockMemory( void *ptr, const size_t bytes ) {
	return ( VirtualUnlock( ptr, static_cast<SIZE_T>(bytes) ) != FALSE );
}

/*
================
Sys_SetPhysicalWorkMemory
================
*/
void Sys_SetPhysicalWorkMemory(const size_t minBytes, const size_t maxBytes ) {
	::SetProcessWorkingSetSize( GetCurrentProcess(), minBytes, maxBytes );
}

/*
================
Sys_GetCurrentUser
================
*/
char *Sys_GetCurrentUser() {
	static char s_userName[1024] = {};
	DWORD size = numeric_cast<DWORD>(sizeof( s_userName ));


	if ( !GetUserName( s_userName, &size ) ) {
		strcpy( s_userName, "player" );
	}

	if ( !s_userName[0] ) {
		strcpy( s_userName, "player" );
	}

	return s_userName;
}	


/*
===============================================================================

	Call stack

===============================================================================
*/


constexpr auto PROLOGUE_SIGNATURE = 0x00EC8B55;

#include <dbghelp.h>

constexpr int UNDECORATE_FLAGS =	UNDNAME_NO_MS_KEYWORDS |
								UNDNAME_NO_ACCESS_SPECIFIERS |
								UNDNAME_NO_FUNCTION_RETURNS |
								UNDNAME_NO_ALLOCATION_MODEL |
								UNDNAME_NO_ALLOCATION_LANGUAGE |
								UNDNAME_NO_MEMBER_TYPE;

#if (defined(_DEBUG) || defined(DEBUG)) && 1

typedef struct symbol_s {
	address_t           address;
	char *				name;
	struct symbol_s *	next;
} symbol_t;

typedef struct module_s {
	address_t           address;
	char *				name;
	symbol_t *			symbols;
	struct module_s *	next;
} module_t;

static module_t *modules;

/*
==================
SkipRestOfLine
==================
*/
static void SkipRestOfLine( const char **ptr ) {
	while( (**ptr) != '\0' && (**ptr) != '\n' && (**ptr) != '\r' ) {
		(*ptr)++;
	}
	while( (**ptr) == '\n' || (**ptr) == '\r' ) {
		(*ptr)++;
	}
}

/*
==================
SkipWhiteSpace
==================
*/
static void SkipWhiteSpace( const char **ptr ) {
	while( (**ptr) == ' ' ) {
		(*ptr)++;
	}
}

/*
==================
ParseHexNumber
==================
*/
static int ParseHexNumber( const char **ptr ) {
	int n = 0;
	while( (**ptr) >= '0' && (**ptr) <= '9' || (**ptr) >= 'a' && (**ptr) <= 'f' ) {
		n <<= 4;
		if ( **ptr >= '0' && **ptr <= '9' ) {
			n |= ( (**ptr) - '0' );
		} else {
			n |= 10 + ( (**ptr) - 'a' );
		}
		(*ptr)++;
	}
	return n;
}

static int64 ParseHexNumber64(const char** ptr) {
	int64 n = 0;
	while ((**ptr) >= '0' && (**ptr) <= '9' || (**ptr) >= 'a' && (**ptr) <= 'f') {
		n <<= 4;
		if (**ptr >= '0' && **ptr <= '9') {
			n |= numeric_cast<int64>((**ptr) - '0');
		}
		else {
			n |= 10LL + numeric_cast<int64>((**ptr) - 'a');
		}
		(*ptr)++;
	}
	return n;
}

/*
==================
Sym_Init
==================
*/
static void Sym_Init(const address_t addr ) {
	TCHAR moduleName[MAX_STRING_CHARS] = {};
	MEMORY_BASIC_INFORMATION mbi = {};

	VirtualQuery( reinterpret_cast<LPVOID>(addr), &mbi, sizeof(mbi) );

	GetModuleFileName( static_cast<HMODULE>(mbi.AllocationBase), moduleName, sizeof( moduleName ) );

	char *ext = moduleName + strlen( moduleName );
	while( ext > moduleName && *ext != '.' ) {
		ext--;
	}
	if ( ext == moduleName ) {
		strcat( moduleName, ".map" );
	} else {
		strcpy( ext, ".map" );
	}

	module_t *module = static_cast<module_t*>(malloc(sizeof(module_t)));
	module->name = static_cast<char*>(malloc(strlen(moduleName) + 1));
	strcpy( module->name, moduleName );
	module->address = reinterpret_cast<address_t>(mbi.AllocationBase);
	module->symbols = nullptr;
	module->next = modules;
	modules = module;

	FILE * fp = fopen( moduleName, "rb" );
	if ( fp == nullptr) {
		return;
	}

	auto pos = ftell( fp );
	fseek( fp, 0, SEEK_END );
	auto length = numeric_cast<size_t>(ftell( fp ));
	fseek( fp, pos, SEEK_SET );

	char *text = static_cast<char*>(malloc(length + 1));
	fread( text, 1, length, fp );
	text[length] = '\0';
	fclose( fp );

	const char *ptr = text;

	// skip up to " Address" on a new line
	while( *ptr != '\0' ) {
		SkipWhiteSpace( &ptr );
		if ( idStr::Cmpn( ptr, "Address", 7 ) == 0 ) {
			SkipRestOfLine( &ptr );
			break;
		}
		SkipRestOfLine( &ptr );
	}

	address_t symbolAddress = 0;

	char symbolName[MAX_STRING_CHARS] = {};
	symbol_t *symbol = nullptr;

	// parse symbols
	while( *ptr != '\0' ) {

		SkipWhiteSpace( &ptr );

		ParseHexNumber( &ptr );
		if ( *ptr == ':' ) {
			ptr++;
		} else {
			break;
		}
		ParseHexNumber( &ptr );

		SkipWhiteSpace( &ptr );

		// parse symbol name
		size_t symbolLength = 0;
		while( *ptr != '\0' && *ptr != ' ' ) {
			symbolName[symbolLength++] = *ptr++;
			if ( symbolLength >= sizeof( symbolName ) - 1 ) {
				break;
			}
		}
		symbolName[symbolLength++] = '\0';

		SkipWhiteSpace( &ptr );

		// parse symbol address
#if defined (ID_WIN64)
		symbolAddress = ParseHexNumber64(&ptr);
#else
		symbolAddress = ParseHexNumber( &ptr );
#endif // #if defined(ID_WIN64)

		SkipRestOfLine( &ptr );

		symbol = static_cast<symbol_t*>(malloc(sizeof(symbol_t)));
		symbol->name = static_cast<char*>(malloc(symbolLength));
		strcpy( symbol->name, symbolName );
		symbol->address = symbolAddress;
		symbol->next = module->symbols;
		module->symbols = symbol;
	}

	free( text );
}

/*
==================
Sym_Shutdown
==================
*/
static void Sym_Shutdown() {
	module_t *m = nullptr;
	symbol_t *s = nullptr;

	for ( m = modules; m != nullptr; m = modules ) {
		modules = m->next;
		for ( s = m->symbols; s != nullptr; s = m->symbols ) {
			m->symbols = s->next;
			free( s->name );
			free( s );
		}
		free( m->name );
		free( m );
	}
	modules = nullptr;
}

/*
==================
Sym_GetFuncInfo
==================
*/
static void Sym_GetFuncInfo(const address_t addr, idStr &module, idStr &funcName ) {
	MEMORY_BASIC_INFORMATION mbi = {};
	const module_t *m = nullptr;
	const symbol_t *s = nullptr;

	VirtualQuery( reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi) );

	for ( m = modules; m != nullptr; m = m->next ) {
		if ( m->address == reinterpret_cast<address_t>(mbi.AllocationBase) ) {
			break;
		}
	}
	if ( !m ) {
		Sym_Init( addr );
		m = modules;
	}

	for ( s = m->symbols; s != nullptr; s = s->next )
	{
		if ( s->address == addr ) 
		{
			char undName[MAX_STRING_CHARS] = {};
			if ( UnDecorateSymbolName( s->name, undName, sizeof(undName), UNDECORATE_FLAGS ) ) {
				funcName = undName;
			} else {
				funcName = s->name;
			}
			for ( size_t i = 0; i < funcName.Length(); i++ ) {
				if ( funcName[i] == '(' ) {
					funcName.CapLength( i );
					break;
				}
			}
			module = m->name;
			return;
		}
	}

	sprintf( funcName, "0x%08lld", addr );
	module = "";
}

#elif defined(_DEBUG) || defined(DEBUG)

DWORD lastAllocationBase = -1;
HANDLE processHandle;
idStr lastModule;

/*
==================
Sym_Init
==================
*/
static void Sym_Init( address_t addr ) {
	TCHAR moduleName[MAX_STRING_CHARS] = {};
	TCHAR modShortNameBuf[MAX_STRING_CHARS] = {};
	MEMORY_BASIC_INFORMATION mbi = {};

	if ( lastAllocationBase != -1 ) {
		Sym_Shutdown();
	}

	VirtualQuery( (void*)addr, &mbi, sizeof(mbi) );

	GetModuleFileName( (HMODULE)mbi.AllocationBase, moduleName, sizeof( moduleName ) );
	_splitpath( moduleName, NULL, NULL, modShortNameBuf, NULL );
	lastModule = modShortNameBuf;

	processHandle = GetCurrentProcess();
	if ( !SymInitialize( processHandle, NULL, FALSE ) ) {
		return;
	}
	if ( !SymLoadModule( processHandle, NULL, moduleName, NULL, (DWORD)mbi.AllocationBase, 0 ) ) {
		SymCleanup( processHandle );
		return;
	}

	SymSetOptions( SymGetOptions() & ~SYMOPT_UNDNAME );

	lastAllocationBase = (DWORD) mbi.AllocationBase;
}

/*
==================
Sym_Shutdown
==================
*/
static void Sym_Shutdown() {
	SymUnloadModule( GetCurrentProcess(), lastAllocationBase );
	SymCleanup( GetCurrentProcess() );
	lastAllocationBase = -1;
}

/*
==================
Sym_GetFuncInfo
==================
*/
static void Sym_GetFuncInfo( long addr, idStr &module, idStr &funcName ) {
	MEMORY_BASIC_INFORMATION mbi;

	VirtualQuery( (void*)addr, &mbi, sizeof(mbi) );

	if ( (DWORD) mbi.AllocationBase != lastAllocationBase ) {
		Sym_Init( addr );
	}

	BYTE symbolBuffer[ sizeof(IMAGEHLP_SYMBOL) + MAX_STRING_CHARS ];
	PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)&symbolBuffer[0];
	pSymbol->SizeOfStruct = sizeof(symbolBuffer);
	pSymbol->MaxNameLength = 1023;
	pSymbol->Address = 0;
	pSymbol->Flags = 0;
	pSymbol->Size =0;

	DWORD symDisplacement = 0;
	if ( SymGetSymFromAddr( processHandle, addr, &symDisplacement, pSymbol ) ) {
		// clean up name, throwing away decorations that don't affect uniqueness
	    char undName[MAX_STRING_CHARS];
		if ( UnDecorateSymbolName( pSymbol->Name, undName, sizeof(undName), UNDECORATE_FLAGS ) ) {
			funcName = undName;
		} else {
			funcName = pSymbol->Name;
		}
		module = lastModule;
	}
	else {
		LPVOID lpMsgBuf;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL 
						);
		LocalFree( lpMsgBuf );

		// Couldn't retrieve symbol (no debug info?, can't load dbghelp.dll?)
		sprintf( funcName, "0x%08x", addr );
		module = "";
    }
}

#else

/*
==================
Sym_Init
==================
*/
static void Sym_Init( long addr ) {
}

/*
==================
Sym_Shutdown
==================
*/
static void Sym_Shutdown() {
}

/*
==================
Sym_GetFuncInfo
==================
*/
static void Sym_GetFuncInfo( long addr, idStr &module, idStr &funcName ) {
	module = "";
	sprintf( funcName, "0x%08x", addr );
}

#endif

/*
==================
GetFuncAddr
==================
*/
static address_t GetFuncAddr( address_t midPtPtr ) {
	do {
		long temp = (*reinterpret_cast<long*>(midPtPtr));
		if ( (temp&0x00FFFFFF) == PROLOGUE_SIGNATURE ) {
			break;
		}
		midPtPtr--;
	} while(true);

	return midPtPtr;
}

/*
==================
GetCallerAddr
==================
*/
static address_t GetCallerAddr( long _ebp ) {
	long midPtPtr = 0;
	address_t res = 0;

	__asm {
		mov		eax, _ebp
		mov		ecx, [eax]		// check for end of stack frames list
		test	ecx, ecx		// check for zero stack frame
		jz		label
		mov		eax, [eax+4]	// get the ret address
		test	eax, eax		// check for zero return address
		jz		label
		mov		midPtPtr, eax
	}
	res = GetFuncAddr( midPtPtr );
label:
	return res;
}

/*
==================
Sys_GetCallStack

 use /Oy option
==================
*/
size_t Sys_GetCallStack( address_t *callStack, const size_t callStackSize, const size_t skipFrames = 0) noexcept {
	size_t i = 0, count = 0;
#if 1 //def _DEBUG
#if defined (ID_WIN64)
	if (!callStack || callStackSize == 0)
	{
		return 0;
	}

	// Capture current register context
	CONTEXT ctx = {};
	RtlCaptureContext(&ctx);

	// Skip this function + user-requested frames
	auto unwind_one = [](CONTEXT& c) -> bool {
		DWORD64 imageBase = 0;
		// history table not necessary for single-step skipping
		PRUNTIME_FUNCTION rf = RtlLookupFunctionEntry(c.Rip, &imageBase, nullptr);
		if (!rf) {
			// Leaf function: emulate "ret"
			const DWORD64* sp = reinterpret_cast<DWORD64*>(c.Rsp);
			if (!sp)
			{
				return false;
			}
			c.Rip = *sp;
			c.Rsp += 8;
		}
		else {
			void* handlerData = nullptr;
			DWORD64 establisherFrame = 0;
			CONTEXT newCtx = c;
			RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, c.Rip, rf,
				&newCtx, &handlerData, &establisherFrame, nullptr);
			c = newCtx;
		}
		return c.Rip != 0;
		};

	for (i = 0; i < skipFrames + 1; ++i) {
		if (!unwind_one(ctx))
		{
			return 0;
		}
	}

	// Walk until we fill the array or reach the top of the stack
	UNWIND_HISTORY_TABLE history = {};

	while (count < callStackSize && ctx.Rip != 0) {
		callStack[count++] = static_cast<address_t>(ctx.Rip);

		DWORD64 imageBase = 0;
		PRUNTIME_FUNCTION rf = ::RtlLookupFunctionEntry(ctx.Rip, &imageBase, &history);
		if (!rf) {
			// Leaf function: emulate "ret"
			const DWORD64* sp = reinterpret_cast<DWORD64*>(ctx.Rsp);
			if (!sp)
			{
				break;
			}
			ctx.Rip = *sp;
			ctx.Rsp += 8;
		}
		else {
			PVOID handlerData = nullptr;
			DWORD64 establisherFrame = 0;
			CONTEXT newCtx = ctx;
			RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, ctx.Rip, rf,
				&newCtx, &handlerData, &establisherFrame, nullptr);
			ctx = newCtx;
		}
	}

	return count;
#elif defined(ID_WIN32)
	long m_ebp;

	__asm {
		mov eax, ebp
		mov m_ebp, eax
	}
	// skip last two functions
	m_ebp = *((long*)m_ebp);
	m_ebp = *((long*)m_ebp);
	// list functions
	for ( i = 0; i < callStackSize; i++ ) {
		callStack[i] = GetCallerAddr( m_ebp );
		if ( callStack[i] == 0 ) {
			break;
		}
		m_ebp = *((long*)m_ebp);
	}
	count = i;
	// clear the rest of the stack
	while (i < callStackSize) {
		callStack[i++] = 0;
	}

	return count;
#endif // #if defined (ID_WIN64)
#else
	int i = 0;
	while (i < callStackSize) {
		callStack[i++] = 0;
	}

	return 0;
#endif
}

/*
==================
Sys_GetCallStackStr
==================
*/
const char *Sys_GetCallStackStr( const address_t *callStack, const size_t callStackSize ) {
	static char string[MAX_STRING_CHARS * 2] = {};
	idStr module = {}, funcName = {};

	size_t index = 0;
	for ( int64 i = numeric_cast<int64>(callStackSize) - 1; i >= 0; i-- ) {
		Sym_GetFuncInfo( callStack[i], module, funcName );
		index += sprintf( string+index, " -> %s", funcName.c_str() );
	}
	return string;
}

/*
==================
Sys_GetCallStackCurStr
==================
*/
const char *Sys_GetCallStackCurStr( const size_t depth ) {
	address_t* callStack = static_cast<address_t*>(_alloca(depth * sizeof(address_t)));
	auto totalDepth = Sys_GetCallStack( callStack, depth );
	return Sys_GetCallStackStr( callStack, totalDepth );
}

/*
==================
Sys_GetCallStackCurAddressStr
==================
*/
const char *Sys_GetCallStackCurAddressStr( const size_t depth ) {
	static char string[MAX_STRING_CHARS * 2] = {};

	address_t* callStack = static_cast<address_t*>(_alloca(depth * sizeof(address_t)));
	auto totalDepth = Sys_GetCallStack( callStack, depth );

	size_t index = 0;
	for ( int64 i = numeric_cast<int64>(totalDepth) - 1; i >= 0; i-- ) {
		index += sprintf( string+index, " -> 0x%08lld", callStack[i] );
	}
	return string;
}

/*
==================
Sys_ShutdownSymbols
==================
*/
void Sys_ShutdownSymbols() {
	Sym_Shutdown();
}
