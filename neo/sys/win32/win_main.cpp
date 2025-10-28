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

#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <mapi.h>
#include <ShellAPI.h>
#include <Shlobj.h>

#ifndef __MRC__
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "../sys_local.h"
#include "win_local.h"
#include "../../renderer/tr_local.h"

idCVar Win32Vars_t::sys_arch( "sys_arch", "", CVAR_SYSTEM | CVAR_INIT, "" );
idCVar Win32Vars_t::sys_cpustring( "sys_cpustring", "detect", CVAR_SYSTEM | CVAR_INIT, "" );
idCVar Win32Vars_t::in_mouse( "in_mouse", "1", CVAR_SYSTEM | CVAR_BOOL, "enable mouse input" );
idCVar Win32Vars_t::win_allowAltTab( "win_allowAltTab", "0", CVAR_SYSTEM | CVAR_BOOL, "allow Alt-Tab when fullscreen" );
idCVar Win32Vars_t::win_notaskkeys( "win_notaskkeys", "0", CVAR_SYSTEM | CVAR_INTEGER, "disable windows task keys" );
idCVar Win32Vars_t::win_username( "win_username", "", CVAR_SYSTEM | CVAR_INIT, "windows user name" );
idCVar Win32Vars_t::win_outputEditString( "win_outputEditString", "1", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar Win32Vars_t::win_viewlog( "win_viewlog", "0", CVAR_SYSTEM | CVAR_INTEGER, "" );
idCVar Win32Vars_t::win_timerUpdate( "win_timerUpdate", "0", CVAR_SYSTEM | CVAR_BOOL, "allows the game to be updated while dragging the window" );
idCVar Win32Vars_t::win_allowMultipleInstances( "win_allowMultipleInstances", "0", CVAR_SYSTEM | CVAR_BOOL, "allow multiple instances running concurrently" );

Win32Vars_t	win32;

static char		sys_cmdline[MAX_STRING_CHARS];

static sysMemoryStats_t exeLaunchMemoryStats;

static HANDLE hProcessMutex;

/*
================
Sys_GetExeLaunchMemoryStatus
================
*/
void Sys_GetExeLaunchMemoryStatus( sysMemoryStats_t &stats ) {
	stats = exeLaunchMemoryStats;
}

/*
==================
Sys_Sentry
==================
*/
static void Sys_Sentry() {
}


#pragma optimize( "", on )

#if defined(DEBUG) || defined (_DEBUG)


static size_t debug_total_alloc = 0;
static size_t debug_total_alloc_count = 0;
static size_t debug_current_alloc = 0;
static size_t debug_current_alloc_count = 0;
static size_t debug_frame_alloc = 0;
static size_t debug_frame_alloc_count = 0;

static idCVar sys_showMallocs( "sys_showMallocs", "0", CVAR_SYSTEM, "" );

// _HOOK_ALLOC, _HOOK_REALLOC, _HOOK_FREE

typedef struct CrtMemBlockHeader
{
	struct _CrtMemBlockHeader *pBlockHeaderNext;	// Pointer to the block allocated just before this one:
	struct _CrtMemBlockHeader *pBlockHeaderPrev;	// Pointer to the block allocated just after this one
   char *szFileName;    // File name
   int nLine;           // Line number
   size_t nDataSize;    // Size of user block
   int nBlockUse;       // Type of block
   long lRequest;       // Allocation number
	byte		gap[4];								// Buffer just before (lower than) the user's memory:
} CrtMemBlockHeader;

#include <crtdbg.h>

/*
==================
Sys_AllocHook

	called for every malloc/new/free/delete
==================
*/
static int Sys_AllocHook(const int nAllocType, void *pvData, const size_t nSize, const int nBlockUse, long lRequest, const unsigned char * szFileName, int nLine ) 
{
	CrtMemBlockHeader	*pHead = nullptr;
	byte				*temp = nullptr;

	if ( nBlockUse == _CRT_BLOCK )
	{
      return( TRUE );
	}

	// get a pointer to memory block header
	temp = static_cast<byte*>(pvData);
	temp -= 32;
	pHead = reinterpret_cast<CrtMemBlockHeader*>(temp);

	switch( nAllocType ) {
		case	_HOOK_ALLOC:
			debug_total_alloc += nSize;
			debug_current_alloc += nSize;
			debug_frame_alloc += nSize;
			debug_total_alloc_count++;
			debug_current_alloc_count++;
			debug_frame_alloc_count++;
			break;

		case	_HOOK_FREE:
			assert( pHead->gap[0] == 0xfd && pHead->gap[1] == 0xfd && pHead->gap[2] == 0xfd && pHead->gap[3] == 0xfd );

			debug_current_alloc -= pHead->nDataSize;
			debug_current_alloc_count--;
			debug_total_alloc_count++;
			debug_frame_alloc_count++;
			break;

		case	_HOOK_REALLOC:
			assert( pHead->gap[0] == 0xfd && pHead->gap[1] == 0xfd && pHead->gap[2] == 0xfd && pHead->gap[3] == 0xfd );

			debug_current_alloc -= pHead->nDataSize;
			debug_total_alloc += nSize;
			debug_current_alloc += nSize;
			debug_frame_alloc += nSize;
			debug_total_alloc_count++;
			debug_current_alloc_count--;
			debug_frame_alloc_count++;
			break;
	}
	return( TRUE );
}

/*
==================
Sys_DebugMemory_f
==================
*/
static void Sys_DebugMemory_f() {
  	common->Printf( "Total allocation %8dk in %d blocks\n", debug_total_alloc / 1024, debug_total_alloc_count );
  	common->Printf( "Current allocation %8dk in %d blocks\n", debug_current_alloc / 1024, debug_current_alloc_count );
}

/*
==================
Sys_MemFrame
==================
*/
static void Sys_MemFrame() {
	if( sys_showMallocs.GetInteger() ) {
		common->Printf("Frame: %8dk in %5d blocks\n", debug_frame_alloc / 1024, debug_frame_alloc_count );
	}

	debug_frame_alloc = 0;
	debug_frame_alloc_count = 0;
}

#endif

/*
==================
Sys_FlushCacheMemory

On windows, the vertex buffers are write combined, so they
don't need to be flushed from the cache
==================
*/
static void Sys_FlushCacheMemory( void *base, int bytes ) {
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void Sys_Error( const char *error, ... ) {
	va_list		argptr = nullptr;
	char		text[4096] = {};
    MSG        msg = {};

	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr);

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

	Win_SetErrorText( text );
	Sys_ShowConsole( 1, true );

	timeEndPeriod( 1 );

	Sys_ShutdownInput();

	GLimp_Shutdown();

	extern idCVar com_productionMode;
	if ( com_productionMode.GetInteger() == 0 ) {
		// wait for the user to quit
		while ( 1 ) {
			if ( !GetMessage( &msg, nullptr, 0, 0 ) ) {
				common->Quit();
			}
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}
	Sys_DestroyConsole();

	exit (1);
}

/*
========================
Sys_Launch
========================
*/
void Sys_Launch( const char * path, idCmdArgs & args,  void * data, size_t dataSize ) {

	TCHAR				szPathOrig[_MAX_PATH] = {};
	STARTUPINFO			si = {};
	PROCESS_INFORMATION	pi = {};

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);

	strcpy( szPathOrig, va( "\"%s\" %s", Sys_EXEPath(), static_cast<const char*>(data) ) );

	if ( !CreateProcess(nullptr, szPathOrig, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi ) ) {
		idLib::Error( "Could not start process: '%s' ", szPathOrig );
		return;
	}
	cmdSystem->AppendCommandText( "quit\n" );
}

/*
========================
Sys_GetCmdLine
========================
*/
const char * Sys_GetCmdLine() {
	return sys_cmdline;
}

/*
========================
Sys_ReLaunch
========================
*/
void Sys_ReLaunch( void * data, const size_t dataSize ) {
	TCHAR				szPathOrig[MAX_PRINT_MSG] = {};
	STARTUPINFO			si = {};
	PROCESS_INFORMATION	pi = {};

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);

	strcpy( szPathOrig, va( "\"%s\" %s", Sys_EXEPath(), static_cast<const char*>(data) ) );

	CloseHandle( hProcessMutex );

	if ( !CreateProcess(nullptr, szPathOrig, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi ) ) {
		idLib::Error( "Could not start process: '%s' ", szPathOrig );
		return;
	}
	cmdSystem->AppendCommandText( "quit\n" );
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit() {
	timeEndPeriod( 1 );
	Sys_ShutdownInput();
	Sys_DestroyConsole();
	ExitProcess( 0 );
}


/*
==============
Sys_Printf
==============
*/
constexpr size_t MAXPRINTMSG = 4096;
static void Sys_Printf( const char *fmt, ... ) {
	char		msg[MAXPRINTMSG] = {};

	va_list argptr = nullptr;
	va_start(argptr, fmt);
	idStr::vsnPrintf( msg, MAXPRINTMSG-1, fmt, argptr );
	va_end(argptr);
	msg[sizeof(msg)-1] = '\0';

	OutputDebugString( msg );

	if ( win32.win_outputEditString.GetBool() && idLib::IsMainThread() ) {
		Conbuf_AppendText( msg );
	}
}

/*
==============
Sys_DebugPrintf
==============
*/
static void Sys_DebugPrintf( const char *fmt, ... ) {
	char msg[MAXPRINTMSG] = {};

	va_list argptr = nullptr;
	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, MAXPRINTMSG-1, fmt, argptr );
	msg[ sizeof(msg)-1 ] = '\0';
	va_end( argptr );

	OutputDebugString( msg );
}

/*
==============
Sys_DebugVPrintf
==============
*/
static void Sys_DebugVPrintf( const char *fmt, const va_list arg ) {
	char msg[MAXPRINTMSG] = {};

	idStr::vsnPrintf( msg, MAXPRINTMSG-1, fmt, arg );
	msg[ sizeof(msg)-1 ] = '\0';

	OutputDebugString( msg );
}

/*
==============
Sys_Sleep
==============
*/
static void Sys_Sleep(const uint32 msec ) {
	Sleep( msec );
}

/*
==============
Sys_ShowWindow
==============
*/
static void Sys_ShowWindow(const bool show ) {
	::ShowWindow( win32.hWnd, show ? SW_SHOW : SW_HIDE );
}

/*
==============
Sys_IsWindowVisible
==============
*/
static bool Sys_IsWindowVisible() {
	return ( ::IsWindowVisible( win32.hWnd ) != 0 );
}

/*
==============
Sys_Mkdir
==============
*/
void Sys_Mkdir( const char *path ) {
	_mkdir (path);
}

/*
=================
Sys_FileTimeStamp
=================
*/
ID_TIME_T Sys_FileTimeStamp(const idFileHandle fp ) {
	FILETIME writeTime = {};
	GetFileTime( fp, nullptr, nullptr, &writeTime );

	/*
		FILETIME = number of 100-nanosecond ticks since midnight 
		1 Jan 1601 UTC. time_t = number of 1-second ticks since 
		midnight 1 Jan 1970 UTC. To translate, we subtract a
		FILETIME representation of midnight, 1 Jan 1970 from the
		time in question and divide by the number of 100-ns ticks
		in one second.
	*/

	SYSTEMTIME base_st = {
		1970,   // wYear
		1,      // wMonth
		0,      // wDayOfWeek
		1,      // wDay
		0,      // wHour
		0,      // wMinute
		0,      // wSecond
		0       // wMilliseconds
	};

	FILETIME base_ft = {};
	SystemTimeToFileTime( &base_st, &base_ft );

	LARGE_INTEGER itime = {};
	itime.QuadPart = reinterpret_cast<LARGE_INTEGER&>( writeTime ).QuadPart;
	itime.QuadPart -= reinterpret_cast<LARGE_INTEGER&>( base_ft ).QuadPart;
	itime.QuadPart /= 10000000LL;
	return static_cast<ID_TIME_T>(itime.QuadPart);
}

/*
========================
Sys_Rmdir
========================
*/
bool Sys_Rmdir( const char *path ) {
	return _rmdir( path ) == 0;
}

/*
========================
Sys_IsFileWritable
========================
*/
bool Sys_IsFileWritable( const char *path ) {
	struct _stat st;
	if ( _stat( path, &st ) == -1 ) {
		return true;
	}
	return ( st.st_mode & S_IWRITE ) != 0;
}

/*
========================
Sys_IsFolder
========================
*/
sysFolder_t Sys_IsFolder( const char *path ) {
	struct _stat buffer;
	if ( _stat( path, &buffer ) < 0 ) {
		return FOLDER_ERROR;
	}
	return ( buffer.st_mode & _S_IFDIR ) != 0 ? FOLDER_YES : FOLDER_NO;
}

/*
==============
Sys_Cwd
==============
*/
static const char *Sys_Cwd() {
	static char cwd[MAX_OSPATH] = {};

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

/*
==============
Sys_DefaultBasePath
==============
*/
const char *Sys_DefaultBasePath() {
	return Sys_Cwd();
}

// Vista shit
typedef HRESULT (WINAPI * SHGetKnownFolderPath_t)( const GUID & rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath );
// NOTE: FOLIDERID_SavedGames is already exported from in shell32.dll in Windows 7.  We can only detect
// the compiler version, but that doesn't doesn't tell us which version of the OS we're linking against.
// This GUID value should never change, so we name it something other than FOLDERID_SavedGames to get
// around this problem.
constexpr GUID FOLDERID_SavedGames_IdTech5 = { 0x4c5c32ff, 0xbb9d, 0x43b0, { 0xb5, 0xb4, 0x2d, 0x72, 0xe5, 0x4e, 0xaa, 0xa4 } };

/*
==============
Sys_DefaultSavePath
==============
*/
const char *Sys_DefaultSavePath() {
	static char savePath[ MAX_PATH ] = {};
	memset( savePath, 0, MAX_PATH );

	HMODULE hShell = LoadLibrary( "shell32.dll" );
	if ( hShell ) {
		SHGetKnownFolderPath_t SHGetKnownFolderPath = reinterpret_cast<SHGetKnownFolderPath_t>(GetProcAddress(hShell, "SHGetKnownFolderPath"));
		if ( SHGetKnownFolderPath ) {
			wchar_t * path;
			if ( SUCCEEDED( SHGetKnownFolderPath( FOLDERID_SavedGames_IdTech5, CSIDL_FLAG_CREATE | CSIDL_FLAG_PER_USER_INIT, nullptr, &path ) ) ) {
				if ( wcstombs( savePath, path, MAX_PATH ) > MAX_PATH ) {
					savePath[0] = 0;
				}
				CoTaskMemFree( path );
			}
		}
		FreeLibrary( hShell );
	}

	if ( savePath[0] == 0 ) {
		SHGetFolderPath(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, savePath );
		strcat( savePath, "\\My Games" );
	}

	strcat( savePath, SAVE_PATH );

	return savePath;
}

/*
==============
Sys_EXEPath
==============
*/
const char *Sys_EXEPath() {
	static char exe[ MAX_OSPATH ] = {};
	GetModuleFileName(nullptr, exe, sizeof( exe ) - 1 );
	return exe;
}

/*
==============
Sys_ListFiles
==============
*/
static int64 Sys_ListFiles( const char *directory, const char *extension, idStrList &list ) {
	idStr		search;
	struct _finddata_t findinfo = {};
	intptr_t			findhandle = 0;
	int			flag = 0;

	if ( !extension) {
		extension = "";
	}

	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}

	sprintf( search, "%s\\*%s", directory, extension );

	// search
	list.Clear();

	findhandle = _findfirst( search, &findinfo );
	if ( findhandle == -1 ) {
		return -1;
	}

	do {
		if ( flag ^ ( findinfo.attrib & _A_SUBDIR ) ) {
			list.Append( findinfo.name );
		}
	} while ( _findnext( findhandle, &findinfo ) != -1 );

	_findclose( findhandle );

	return numeric_cast<int64>(list.Num());
}


/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData() {
	char *data = nullptr;
	char *cliptext = nullptr;

	if ( OpenClipboard(nullptr) != 0 ) {
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != nullptr ) {
			if ( ( cliptext = static_cast<char*>(GlobalLock(hClipboardData)) ) != nullptr ) {
				data = static_cast<char*>(Mem_Alloc(GlobalSize(hClipboardData) + 1, TAG_CRAP));
				strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );
				
				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
================
Sys_SetClipboardData
================
*/
void Sys_SetClipboardData( const char *string ) {
	HGLOBAL HMem;
	char *PMem = nullptr;

	// allocate memory block
	HMem = static_cast<char*>(::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, strlen(string) + 1));
	if ( HMem == nullptr) {
		return;
	}
	// lock allocated memory and obtain a pointer
	PMem = static_cast<char*>(::GlobalLock(HMem));
	if ( PMem == nullptr) {
		return;
	}
	// copy text into allocated memory block
	lstrcpy( PMem, string );
	// unlock allocated memory
	::GlobalUnlock( HMem );
	// open Clipboard
	if ( !OpenClipboard( nullptr ) ) {
		::GlobalFree( HMem );
		return;
	}
	// remove current Clipboard contents
	EmptyClipboard();
	// supply the memory handle to the Clipboard
	SetClipboardData( CF_TEXT, HMem );
	HMem = nullptr;
	// close Clipboard
	CloseClipboard();
}

/*
========================
ExecOutputFn
========================
*/
static void ExecOutputFn( const char * text ) {
	idLib::Printf( text );
}


/*
========================
Sys_Exec

if waitMsec is INFINITE, completely block until the process exits
If waitMsec is -1, don't wait for the process to exit
Other waitMsec values will allow the workFn to be called at those intervals.
========================
*/
bool Sys_Exec(	const char * appPath, const char * workingPath, const char * args,
	const execProcessWorkFunction_t workFn, execOutputFunction_t outputFn, const uint32 waitMS,
	unsigned int & exitCode ) {
		exitCode = 0;
		SECURITY_ATTRIBUTES secAttr = {};
		secAttr.nLength = sizeof( SECURITY_ATTRIBUTES );
		secAttr.bInheritHandle = TRUE;
		secAttr.lpSecurityDescriptor = nullptr;

		HANDLE hStdOutRead;
		HANDLE hStdOutWrite;
		CreatePipe( &hStdOutRead, &hStdOutWrite, &secAttr, 0 );
		SetHandleInformation( hStdOutRead, HANDLE_FLAG_INHERIT, 0 );

		HANDLE hStdInRead;
		HANDLE hStdInWrite;
		CreatePipe( &hStdInRead, &hStdInWrite, &secAttr, 0 );
		SetHandleInformation( hStdInWrite, HANDLE_FLAG_INHERIT, 0 );										

		STARTUPINFO si = {};
		memset( &si, 0, sizeof( si ) );
		si.cb = sizeof( si );
		si.hStdError = hStdOutWrite;
		si.hStdOutput = hStdOutWrite;
		si.hStdInput = hStdInRead;
		si.wShowWindow = FALSE;
		si.dwFlags |= STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

		PROCESS_INFORMATION pi = {};
		memset ( &pi, 0, sizeof( pi ) );

		if ( outputFn != nullptr) {
			outputFn( va( "^2Executing Process: ^7%s\n^2working path: ^7%s\n^2args: ^7%s\n", appPath, workingPath, args ) );
		} else {
			outputFn = ExecOutputFn;
		}

		// we duplicate args here so we can concatenate the exe name and args into a single command line
		const char * imageName = appPath;
		char * cmdLine = nullptr;
		{
			// if we have any args, we need to copy them to a new buffer because CreateProcess modifies
			// the command line buffer.
			if ( args != nullptr) {
				if ( appPath != nullptr) {
					size_t len = idStr::Length( args ) + idStr::Length( appPath ) + 1 /* for space */ + 1 /* for NULL terminator */ + 2 /* app quotes */;
					cmdLine = static_cast<char*>(Mem_Alloc(len, TAG_TEMP));
					// note that we're putting quotes around the appPath here because when AAS2.exe gets an app path with spaces
					// in the path "w:/zion/build/win32/Debug with Inlines/AAS2.exe" it gets more than one arg for the app name,
					// which it most certainly should not, so I am assuming this is a side effect of using CreateProcess.
					idStr::snPrintf( cmdLine, len, "\"%s\" %s", appPath, args );
				} else {
					size_t len = idStr::Length( args ) + 1;
					cmdLine = static_cast<char*>(Mem_Alloc(len, TAG_TEMP));
					idStr::Copynz( cmdLine, args, len );
				}
				// the image name should always be NULL if we have command line arguments because it is already
				// prefixed to the command line.
				imageName = nullptr;
			}
		}

		BOOL result = CreateProcess( imageName, (LPSTR)cmdLine, nullptr, nullptr, TRUE, 0, nullptr, workingPath, &si, &pi );

		if ( result == FALSE ) {
			TCHAR szBuf[1024] = {};
			LPVOID lpMsgBuf = nullptr;
			DWORD dw = GetLastError(); 

			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM,
				nullptr,
				dw,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPTSTR>(&lpMsgBuf),
				0, nullptr);

			wsprintf( szBuf, "%d: %s", dw, lpMsgBuf );
			if ( outputFn != nullptr) {
				outputFn( szBuf );
			}
			LocalFree( lpMsgBuf );
			if ( cmdLine != nullptr) {
				Mem_Free( cmdLine );
			}
			return false;
		} else if ( waitMS >= 0 ) {	// if waitMS == -1, don't wait for process to exit
			DWORD ec = 0;
			char buffer[ 4096 ] = {};
			for ( ; ; ) {
				DWORD wait = WaitForSingleObject(pi.hProcess, waitMS);
				GetExitCodeProcess( pi.hProcess, &ec );

				DWORD bytesRead = 0;
				DWORD bytesAvail = 0;
				DWORD bytesLeft = 0;
				BOOL ok = PeekNamedPipe( hStdOutRead, nullptr, 0, nullptr, &bytesAvail, &bytesLeft );
				if ( ok && bytesAvail != 0 ) {
					ok = ReadFile( hStdOutRead, buffer, sizeof( buffer ) - 3, &bytesRead, nullptr);
					if ( ok && bytesRead > 0 ) {
						buffer[ bytesRead ] = '\0';
						if ( outputFn != nullptr) {
							size_t length = 0;
							for ( size_t i = 0; buffer[i] != '\0'; i++ ) {
								if ( buffer[i] != '\r' ) {
									buffer[length++] = buffer[i];
								}
							}
							buffer[length++] = '\0';
							outputFn( buffer );
						}
					}
				}

				if ( ec != STILL_ACTIVE ) {
					exitCode = ec;
					break;
				}

				if ( workFn != nullptr) {
					if ( !workFn() ) {
						TerminateProcess( pi.hProcess, 0 );
						break;
					}
				}
			}
		}

		// this assumes that windows duplicates the command line string into the created process's
		// environment space.
		if ( cmdLine != nullptr) {
			Mem_Free( cmdLine );
		}

		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
		CloseHandle( hStdOutRead );
		CloseHandle( hStdOutWrite );
		CloseHandle( hStdInRead );
		CloseHandle( hStdInWrite );
		return true;
}

/*
========================================================================

DLL Loading

========================================================================
*/

/*
=====================
Sys_DLL_Load
=====================
*/
dllHandle_t Sys_DLL_Load( const char *dllName ) {
	HINSTANCE libHandle = LoadLibrary( dllName );
	return libHandle;
}

/*
=====================
Sys_DLL_GetProcAddress
=====================
*/
address_t Sys_DLL_GetProcAddress(const dllHandle_t dllHandle, const char *procName ) {
	return reinterpret_cast<address_t>(GetProcAddress(dllHandle, procName)); 
}

/*
=====================
Sys_DLL_Unload
=====================
*/
void Sys_DLL_Unload(const dllHandle_t dllHandle ) {
	if (!dllHandle) {
		return;
	}
	if (FreeLibrary(dllHandle) == 0) {
		const auto lastError = GetLastError();
		LPVOID lpMsgBuf = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER,
			nullptr,
			lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			reinterpret_cast<LPTSTR>(&lpMsgBuf),
			0,
			nullptr
		);
		Sys_Error("Sys_DLL_Unload: FreeLibrary failed - %s (%d)", lpMsgBuf, lastError);
	}
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

constexpr size_t MAX_QUEUED_EVENTS = 256;
constexpr auto	 MASK_QUEUED_EVENTS = (MAX_QUEUED_EVENTS - 1);

static sysEvent_t	eventQueue[MAX_QUEUED_EVENTS];
static index_t		eventHead = 0;
static index_t		eventTail = 0;

/*
================
Sys_QueueEvent

Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueueEvent(const sysEventType_t type, const int value, const int value2, const size_t ptrLength, void *ptr, const index_t inputDeviceNum ) {
	sysEvent_t * ev = &eventQueue[ eventHead & MASK_QUEUED_EVENTS ];

	if ( eventHead - eventTail >= MAX_QUEUED_EVENTS ) {
		common->Printf("Sys_QueueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr ) {
			Mem_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
	ev->inputDevice = inputDeviceNum;
}

/*
=============
Sys_PumpEvents

This allows windows to be moved during renderbump
=============
*/
static void Sys_PumpEvents() {
    MSG msg = {};

	// pump the message loop
	while( PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE ) ) {
		if ( !GetMessage( &msg, nullptr, 0, 0 ) ) {
			common->Quit();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		if ( win32.sysMsgTime && win32.sysMsgTime > msg.time ) {
			// don't ever let the event times run backwards	
//			common->Printf( "Sys_PumpEvents: win32.sysMsgTime (%i) > msg.time (%i)\n", win32.sysMsgTime, msg.time );
		} else {
			win32.sysMsgTime = msg.time;
		}
 
		TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}
}

/*
================
Sys_GenerateEvents
================
*/
void Sys_GenerateEvents() {
	static int entered = false;
	char *s = nullptr;

	if ( entered ) {
		return;
	}
	entered = true;

	// pump the message loop
	Sys_PumpEvents();

	// grab or release the mouse cursor if necessary
	IN_Frame();

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char *b = nullptr;
		size_t	len = 0;

		len = strlen( s ) + 1;
		b = static_cast<char*>(Mem_Alloc(len, TAG_EVENTS));
		strcpy( b, s );
		Sys_QueueEvent( SE_CONSOLE, 0, 0, len, b, 0 );
	}

	entered = false;
}

/*
================
Sys_ClearEvents
================
*/
void Sys_ClearEvents() {
	eventHead = eventTail = 0;
}

/*
================
Sys_GetEvent
================
*/
sysEvent_t Sys_GetEvent() {
	sysEvent_t	ev;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQueue[ ( eventTail - 1 ) & MASK_QUEUED_EVENTS ];
	}

	// return the empty event 
	memset( &ev, 0, sizeof( ev ) );

	return ev;
}

//================================================================

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
static void Sys_In_Restart_f( const idCmdArgs &args ) {
	Sys_ShutdownInput();
	Sys_InitInput();
}

/*
================
Sys_AlreadyRunning

returns true if there is a copy of D3 running already
================
*/
bool Sys_AlreadyRunning() {
#ifndef DEBUG
	if ( !win32.win_allowMultipleInstances.GetBool() ) {
		hProcessMutex = ::CreateMutex(nullptr, FALSE, "DOOM3" );
		if ( ::GetLastError() == ERROR_ALREADY_EXISTS || ::GetLastError() == ERROR_ACCESS_DENIED ) {
			return true;
		}
	}
#endif
	return false;
}

/*
================
Sys_Init

The cvar system must already be setup
================
*/
constexpr auto OSR2_BUILD_NUMBER = 1111;
constexpr auto WIN98_BUILD_NUMBER = 1998;

// Input:  DWORD dwProductType  (returned by GetProductInfo)
// Output: const char* edition  (friendly edition text)
static const char* GetWindowsEditionFromProductType( const DWORD dwProductType )
{
	const char* edition = "Unknown";

	switch (dwProductType)
	{
	case PRODUCT_UNDEFINED:                               edition = "Undefined"; break;

	case PRODUCT_ULTIMATE:                                edition = "Ultimate"; break;
	case PRODUCT_HOME_BASIC:                              edition = "Home Basic"; break;
	case PRODUCT_HOME_PREMIUM:                            edition = "Home Premium"; break;
	case PRODUCT_ENTERPRISE:                              edition = "Enterprise"; break;
	case PRODUCT_HOME_BASIC_N:                            edition = "Home Basic N"; break;
	case PRODUCT_BUSINESS:                                edition = "Business"; break;
	case PRODUCT_STANDARD_SERVER:                         edition = "Server Standard (Full)"; break;
	case PRODUCT_DATACENTER_SERVER:                       edition = "Server Datacenter (Full)"; break;
	case PRODUCT_SMALLBUSINESS_SERVER:                    edition = "Small Business Server"; break;
	case PRODUCT_ENTERPRISE_SERVER:                       edition = "Server Enterprise (Full)"; break;
	case PRODUCT_STARTER:                                 edition = "Starter"; break;
	case PRODUCT_DATACENTER_SERVER_CORE:                  edition = "Server Datacenter (Core)"; break;
	case PRODUCT_STANDARD_SERVER_CORE:                    edition = "Server Standard (Core)"; break;
	case PRODUCT_ENTERPRISE_SERVER_CORE:                  edition = "Server Enterprise (Core)"; break;
	case PRODUCT_ENTERPRISE_SERVER_IA64:                  edition = "Server Enterprise (IA64)"; break;
	case PRODUCT_BUSINESS_N:                              edition = "Business N"; break;
	case PRODUCT_WEB_SERVER:                              edition = "Web Server (Full)"; break;
	case PRODUCT_CLUSTER_SERVER:                          edition = "Cluster Server"; break;
	case PRODUCT_HOME_SERVER:                             edition = "Home Server"; break;
	case PRODUCT_STORAGE_EXPRESS_SERVER:                  edition = "Storage Server Express (Full)"; break;
	case PRODUCT_STORAGE_STANDARD_SERVER:                 edition = "Storage Server Standard (Full)"; break;
	case PRODUCT_STORAGE_WORKGROUP_SERVER:                edition = "Storage Server Workgroup (Full)"; break;
	case PRODUCT_STORAGE_ENTERPRISE_SERVER:               edition = "Storage Server Enterprise (Full)"; break;
	case PRODUCT_SERVER_FOR_SMALLBUSINESS:                edition = "Server for Small Business"; break;
	case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:            edition = "Small Business Server Premium"; break;
	case PRODUCT_HOME_PREMIUM_N:                          edition = "Home Premium N"; break;
	case PRODUCT_ENTERPRISE_N:                            edition = "Enterprise N"; break;
	case PRODUCT_ULTIMATE_N:                              edition = "Ultimate N"; break;
	case PRODUCT_WEB_SERVER_CORE:                         edition = "Web Server (Core)"; break;
	case PRODUCT_MEDIUMBUSINESS_SERVER_MANAGEMENT:        edition = "Essential Business Server Management"; break;
	case PRODUCT_MEDIUMBUSINESS_SERVER_SECURITY:          edition = "Essential Business Server Security"; break;
	case PRODUCT_MEDIUMBUSINESS_SERVER_MESSAGING:         edition = "Essential Business Server Messaging"; break;
	case PRODUCT_SERVER_FOUNDATION:                       edition = "Server Foundation"; break;
	case PRODUCT_HOME_PREMIUM_SERVER:                     edition = "Home Premium Server"; break;
	case PRODUCT_SERVER_FOR_SMALLBUSINESS_V:              edition = "Server for Small Business (No Hyper-V)"; break;
	case PRODUCT_STANDARD_SERVER_V:                       edition = "Server Standard (No Hyper-V)"; break;
	case PRODUCT_DATACENTER_SERVER_V:                     edition = "Server Datacenter (No Hyper-V)"; break;
	case PRODUCT_ENTERPRISE_SERVER_V:                     edition = "Server Enterprise (No Hyper-V)"; break;
	case PRODUCT_DATACENTER_SERVER_CORE_V:                edition = "Server Datacenter Core (No Hyper-V)"; break;
	case PRODUCT_STANDARD_SERVER_CORE_V:                  edition = "Server Standard Core (No Hyper-V)"; break;
	case PRODUCT_ENTERPRISE_SERVER_CORE_V:                edition = "Server Enterprise Core (No Hyper-V)"; break;
	case PRODUCT_HYPERV:                                  edition = "Microsoft Hyper-V Server"; break;
	case PRODUCT_STORAGE_EXPRESS_SERVER_CORE:             edition = "Storage Server Express (Core)"; break;
	case PRODUCT_STORAGE_STANDARD_SERVER_CORE:            edition = "Storage Server Standard (Core)"; break;
	case PRODUCT_STORAGE_WORKGROUP_SERVER_CORE:           edition = "Storage Server Workgroup (Core)"; break;
	case PRODUCT_STORAGE_ENTERPRISE_SERVER_CORE:          edition = "Storage Server Enterprise (Core)"; break;
	case PRODUCT_STARTER_N:                               edition = "Starter N"; break;
	case PRODUCT_PROFESSIONAL:                            edition = "Professional"; break;
	case PRODUCT_PROFESSIONAL_N:                          edition = "Professional N"; break;
	case PRODUCT_SB_SOLUTION_SERVER:                      edition = "SB Solution Server"; break;
	case PRODUCT_SERVER_FOR_SB_SOLUTIONS:                 edition = "Server for SB Solutions"; break;
	case PRODUCT_STANDARD_SERVER_SOLUTIONS:               edition = "Standard Server Solutions"; break;
	case PRODUCT_STANDARD_SERVER_SOLUTIONS_CORE:          edition = "Standard Server Solutions (Core)"; break;
	case PRODUCT_SB_SOLUTION_SERVER_EM:                   edition = "SB Solution Server (Embedded)"; break;
	case PRODUCT_SERVER_FOR_SB_SOLUTIONS_EM:              edition = "Server for SB Solutions (Embedded)"; break;
	case PRODUCT_SOLUTION_EMBEDDEDSERVER:                 edition = "Solution Embedded Server"; break;
	case PRODUCT_SOLUTION_EMBEDDEDSERVER_CORE:            edition = "Solution Embedded Server (Core)"; break;
	case PRODUCT_PROFESSIONAL_EMBEDDED:                   edition = "Professional (Embedded)"; break;
	case PRODUCT_ESSENTIALBUSINESS_SERVER_MGMT:           edition = "Essential Business Server Management"; break;
	case PRODUCT_ESSENTIALBUSINESS_SERVER_ADDL:           edition = "Essential Business Server Additional"; break;
	case PRODUCT_ESSENTIALBUSINESS_SERVER_MGMTSVC:        edition = "Essential Business Server Mgmt Service"; break;
	case PRODUCT_ESSENTIALBUSINESS_SERVER_ADDLSVC:        edition = "Essential Business Server Addl Service"; break;
	case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM_CORE:       edition = "Small Business Server Premium (Core)"; break;
	case PRODUCT_CLUSTER_SERVER_V:                        edition = "Cluster Server (No Hyper-V)"; break;
	case PRODUCT_EMBEDDED:                                edition = "Embedded"; break;
	case PRODUCT_STARTER_E:                               edition = "Starter E"; break;
	case PRODUCT_HOME_BASIC_E:                            edition = "Home Basic E"; break;
	case PRODUCT_HOME_PREMIUM_E:                          edition = "Home Premium E"; break;
	case PRODUCT_PROFESSIONAL_E:                          edition = "Professional E"; break;
	case PRODUCT_ENTERPRISE_E:                            edition = "Enterprise E"; break;
	case PRODUCT_ULTIMATE_E:                              edition = "Ultimate E"; break;
	case PRODUCT_ENTERPRISE_EVALUATION:                   edition = "Enterprise Evaluation"; break;
	case PRODUCT_MULTIPOINT_STANDARD_SERVER:              edition = "MultiPoint Server Standard"; break;
	case PRODUCT_MULTIPOINT_PREMIUM_SERVER:               edition = "MultiPoint Server Premium"; break;
	case PRODUCT_STANDARD_EVALUATION_SERVER:              edition = "Server Standard Evaluation"; break;
	case PRODUCT_DATACENTER_EVALUATION_SERVER:            edition = "Server Datacenter Evaluation"; break;
	case PRODUCT_ENTERPRISE_N_EVALUATION:                 edition = "Enterprise N Evaluation"; break;
	case PRODUCT_EMBEDDED_AUTOMOTIVE:                     edition = "Embedded Automotive"; break;
	case PRODUCT_EMBEDDED_INDUSTRY_A:                     edition = "Embedded Industry A"; break;
	case PRODUCT_THINPC:                                  edition = "Thin PC"; break;
	case PRODUCT_EMBEDDED_A:                              edition = "Embedded A"; break;
	case PRODUCT_EMBEDDED_INDUSTRY:                       edition = "Embedded Industry"; break;
	case PRODUCT_EMBEDDED_E:                              edition = "Embedded E"; break;
	case PRODUCT_EMBEDDED_INDUSTRY_E:                     edition = "Embedded Industry E"; break;
	case PRODUCT_EMBEDDED_INDUSTRY_A_E:                   edition = "Embedded Industry A E"; break;
	case PRODUCT_STORAGE_WORKGROUP_EVALUATION_SERVER:     edition = "Storage Server Workgroup Evaluation"; break;
	case PRODUCT_STORAGE_STANDARD_EVALUATION_SERVER:      edition = "Storage Server Standard Evaluation"; break;
	case PRODUCT_CORE_ARM:                                edition = "Core (ARM)"; break;
	case PRODUCT_CORE_N:                                  edition = "Core N"; break;
	case PRODUCT_CORE_COUNTRYSPECIFIC:                    edition = "Core (Country Specific)"; break;
	case PRODUCT_CORE_SINGLELANGUAGE:                     edition = "Core Single Language"; break;
	case PRODUCT_CORE:                                    edition = "Core"; break;
	case PRODUCT_PROFESSIONAL_WMC:                        edition = "Professional with Media Center"; break;
	case PRODUCT_EMBEDDED_INDUSTRY_EVAL:                  edition = "Embedded Industry (Eval)"; break;
	case PRODUCT_EMBEDDED_INDUSTRY_E_EVAL:                edition = "Embedded Industry E (Eval)"; break;
	case PRODUCT_EMBEDDED_EVAL:                           edition = "Embedded (Eval)"; break;
	case PRODUCT_EMBEDDED_E_EVAL:                         edition = "Embedded E (Eval)"; break;
	case PRODUCT_NANO_SERVER:                             edition = "Nano Server"; break;
	case PRODUCT_CLOUD_STORAGE_SERVER:                    edition = "Cloud Storage Server"; break;
	case PRODUCT_CORE_CONNECTED:                          edition = "Core Connected"; break;
	case PRODUCT_PROFESSIONAL_STUDENT:                    edition = "Professional Student"; break;
	case PRODUCT_CORE_CONNECTED_N:                        edition = "Core Connected N"; break;
	case PRODUCT_PROFESSIONAL_STUDENT_N:                  edition = "Professional Student N"; break;
	case PRODUCT_CORE_CONNECTED_SINGLELANGUAGE:           edition = "Core Connected Single Language"; break;
	case PRODUCT_CORE_CONNECTED_COUNTRYSPECIFIC:          edition = "Core Connected Country Specific"; break;
	case PRODUCT_CONNECTED_CAR:                           edition = "Connected Car"; break;
	case PRODUCT_INDUSTRY_HANDHELD:                       edition = "Industry Handheld"; break;
	case PRODUCT_PPI_PRO:                                 edition = "Surface Hub (PPI Pro)"; break;
	case PRODUCT_ARM64_SERVER:                            edition = "ARM64 Server"; break;
	case PRODUCT_EDUCATION:                               edition = "Education"; break;
	case PRODUCT_EDUCATION_N:                             edition = "Education N"; break;
	case PRODUCT_IOTUAP:                                  edition = "IoT Core (UAP)"; break;
	case PRODUCT_CLOUD_HOST_INFRASTRUCTURE_SERVER:        edition = "Cloud Host Infrastructure Server"; break;
	case PRODUCT_ENTERPRISE_S:                            edition = "Enterprise S"; break;
	case PRODUCT_ENTERPRISE_S_N:                          edition = "Enterprise S N"; break;
	case PRODUCT_PROFESSIONAL_S:                          edition = "Professional S"; break;
	case PRODUCT_PROFESSIONAL_S_N:                        edition = "Professional S N"; break;
	case PRODUCT_ENTERPRISE_S_EVALUATION:                 edition = "Enterprise S Evaluation"; break;
	case PRODUCT_ENTERPRISE_S_N_EVALUATION:               edition = "Enterprise S N Evaluation"; break;
	case PRODUCT_HOLOGRAPHIC:                             edition = "Holographic"; break;
	case PRODUCT_HOLOGRAPHIC_BUSINESS:                    edition = "Holographic Business"; break;
	case PRODUCT_PRO_SINGLE_LANGUAGE:                     edition = "Pro Single Language"; break;
	case PRODUCT_PRO_CHINA:                               edition = "Pro China Only"; break;
	case PRODUCT_ENTERPRISE_SUBSCRIPTION:                 edition = "Enterprise Subscription"; break;
	case PRODUCT_ENTERPRISE_SUBSCRIPTION_N:               edition = "Enterprise Subscription N"; break;
	case PRODUCT_DATACENTER_NANO_SERVER:                  edition = "Datacenter Nano Server"; break;
	case PRODUCT_STANDARD_NANO_SERVER:                    edition = "Standard Nano Server"; break;
	case PRODUCT_DATACENTER_A_SERVER_CORE:                edition = "Datacenter Server Azure (Core)"; break;
	case PRODUCT_STANDARD_A_SERVER_CORE:                  edition = "Standard Server Azure (Core)"; break;
	case PRODUCT_DATACENTER_WS_SERVER_CORE:               edition = "Datacenter WS Server (Core)"; break;
	case PRODUCT_STANDARD_WS_SERVER_CORE:                 edition = "Standard WS Server (Core)"; break;
	case PRODUCT_UTILITY_VM:                              edition = "Utility VM"; break;
	case PRODUCT_DATACENTER_EVALUATION_SERVER_CORE:       edition = "Datacenter Evaluation (Core)"; break;
	case PRODUCT_STANDARD_EVALUATION_SERVER_CORE:         edition = "Standard Evaluation (Core)"; break;
	case PRODUCT_PRO_WORKSTATION:                         edition = "Pro for Workstations"; break;
	case PRODUCT_PRO_WORKSTATION_N:                       edition = "Pro for Workstations N"; break;
	case PRODUCT_PRO_FOR_EDUCATION:                       edition = "Pro Education"; break;
	case PRODUCT_PRO_FOR_EDUCATION_N:                     edition = "Pro Education N"; break;
	case PRODUCT_AZURE_SERVER_CORE:                       edition = "Azure Server (Core)"; break;
	case PRODUCT_AZURE_NANO_SERVER:                       edition = "Azure Nano Server"; break;
	case PRODUCT_ENTERPRISEG:                             edition = "Enterprise G (China Gov)"; break;
	case PRODUCT_ENTERPRISEGN:                            edition = "Enterprise GN (China Gov N)"; break;
	case PRODUCT_SERVERRDSH:                              edition = "Windows Server RDSH"; break;
	case PRODUCT_CLOUD:                                   edition = "Cloud"; break;
	case PRODUCT_CLOUDN:                                  edition = "Cloud N"; break;
	case PRODUCT_HUBOS:                                   edition = "Hub OS"; break;
	case PRODUCT_ONECOREUPDATEOS:                         edition = "OneCore Update OS"; break;
	case PRODUCT_CLOUDE:                                  edition = "CloudE"; break;
	case PRODUCT_IOTOS:                                   edition = "IoT OS"; break;
	case PRODUCT_CLOUDEN:                                 edition = "CloudE N"; break;
	case PRODUCT_IOTEDGEOS:                               edition = "IoT Edge OS"; break;
	case PRODUCT_IOTENTERPRISE:                           edition = "IoT Enterprise"; break;
	case PRODUCT_LITE:                                    edition = "Lite / 10X"; break;
	case PRODUCT_IOTENTERPRISES:                          edition = "IoT Enterprise S"; break;
	case PRODUCT_XBOX_SYSTEMOS:                           edition = "Xbox System OS"; break;
	case PRODUCT_XBOX_GAMEOS:                             edition = "Xbox Game OS"; break;
	case PRODUCT_XBOX_ERAOS:                              edition = "Xbox ERA OS"; break;
	case PRODUCT_XBOX_DURANGOHOSTOS:                      edition = "Xbox Durango Host OS"; break;
	case PRODUCT_XBOX_SCARLETTHOSTOS:                     edition = "Xbox Scarlett Host OS"; break;
	case PRODUCT_XBOX_KEYSTONE:                           edition = "Xbox Keystone"; break;
	case PRODUCT_AZURE_SERVER_CLOUDHOST:                  edition = "Azure Server CloudHost"; break;
	case PRODUCT_AZURE_SERVER_CLOUDMOS:                   edition = "Azure Server CloudMOS"; break;
	case PRODUCT_CLOUDEDITIONN:                           edition = "Cloud Edition N"; break;
	case PRODUCT_CLOUDEDITION:                            edition = "Cloud Edition"; break;
	case PRODUCT_VALIDATION:                              edition = "Validation SKU"; break;
	case PRODUCT_IOTENTERPRISESK:                         edition = "IoT Enterprise SK"; break;
	case PRODUCT_IOTENTERPRISEK:                          edition = "IoT Enterprise K"; break;
	case PRODUCT_IOTENTERPRISESEVAL:                      edition = "IoT Enterprise S (Eval)"; break;
	case PRODUCT_AZURE_SERVER_AGENTBRIDGE:                edition = "Azure Server AgentBridge"; break;
	case PRODUCT_AZURE_SERVER_NANOHOST:                   edition = "Azure Server NanoHost"; break;
	case PRODUCT_WNC:                                     edition = "Windows Core (WNC)"; break;

	case PRODUCT_AZURESTACKHCI_SERVER_CORE:               edition = "Azure Stack HCI (Core)"; break;
	case PRODUCT_DATACENTER_SERVER_AZURE_EDITION:         edition = "Server Datacenter Azure Edition (Full)"; break;
	case PRODUCT_DATACENTER_SERVER_CORE_AZURE_EDITION:    edition = "Server Datacenter Azure Edition (Core)"; break;
	case PRODUCT_DATACENTER_WS_SERVER_CORE_AZURE_EDITION: edition = "Datacenter WS Server (Core, Azure Edition)"; break;

	case PRODUCT_UNLICENSED:                              edition = "Unlicensed"; break;

	default:                                              /* keep "Unknown" */ break;
	}

	return edition;
}

void Sys_Init() {

	CoInitialize(nullptr);

	// get WM_TIMER messages pumped every millisecond
//	SetTimer( NULL, 0, 100, NULL );

	cmdSystem->AddCommand( "in_restart", Sys_In_Restart_f, CMD_FL_SYSTEM, "restarts the input system" );

	//
	// Windows user name
	//
	win32.win_username.SetString( Sys_GetCurrentUser() );

	//
	// Windows version
	//
	win32.osversion.dwOSVersionInfoSize = sizeof( win32.osversion );

	if ( !GetVersionEx( reinterpret_cast<LPOSVERSIONINFO>(&win32.osversion) ) )
		Sys_Error( "Couldn't get OS info" );

	if ( win32.osversion.dwMajorVersion < 4 ) {
		Sys_Error( "%s requires Windows version 4 (NT) or greater", GAME_NAME);
	}
	if ( win32.osversion.dwPlatformId == VER_PLATFORM_WIN32s ) {
		Sys_Error( "%s doesn't run on Win32s", GAME_NAME);
	}

	DWORD dwProductType = 0;
	GetProductInfo(win32.osversion.dwMajorVersion, win32.osversion.dwMinorVersion, 0, 0, &dwProductType);

	win32.win_edition.SetString(GetWindowsEditionFromProductType(dwProductType));

	if( win32.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT ) {
		if (win32.osversion.dwMajorVersion <= 3) {
			win32.sys_arch.SetString("NT3");
		}
		else if (win32.osversion.dwMajorVersion <= 4) {
			win32.sys_arch.SetString("NT4");
		}
		else if (win32.osversion.dwMajorVersion == 5 && win32.osversion.dwMinorVersion == 0) {
			if (IsWindowsServer())
			{
				win32.sys_arch.SetString("2000 Server");
			}
			else
			{
				win32.sys_arch.SetString("2000 Pro");
			}
		}
		else if (win32.osversion.dwMajorVersion == 5 && win32.osversion.dwMinorVersion == 1) {
			win32.sys_arch.SetString("XP");
		}
		else if (win32.osversion.dwMajorVersion == 5 && win32.osversion.dwMinorVersion == 2) {
			if (IsWindowsServer())
			{
				if (win32.osversion.wSuiteMask & VER_SUITE_WH_SERVER)
				{
					win32.sys_arch.SetString("Home Server");
				}
				if (GetSystemMetrics(SM_SERVERR2) == 0)
				{
					win32.sys_arch.SetString("Server 2003");
				}
				else if (GetSystemMetrics(SM_SERVERR2) == 1)
				{
					win32.sys_arch.SetString("Server 2003 R2");
				}
			}
			else
			{
				win32.sys_arch.SetString("XP Pro x64");
			}
		}
		else if (win32.osversion.dwMajorVersion == 6 && win32.osversion.dwMinorVersion == 0) {
			if (IsWindowsServer())
			{
				win32.sys_arch.SetString("Server 2008");
			}
			else
			{
				win32.sys_arch.SetString("Vista");
			}
		}
		else if (win32.osversion.dwMajorVersion == 6 && win32.osversion.dwMinorVersion == 1) {
			if (IsWindowsServer())
			{
				win32.sys_arch.SetString("Server 2008 R2");
			}
			else
			{
				win32.sys_arch.SetString("7");
			}
		}
		else if (win32.osversion.dwMajorVersion == 6 && win32.osversion.dwMinorVersion == 2) {
			if (IsWindowsServer())
			{
				win32.sys_arch.SetString("Server 2012");
			}
			else
			{
				win32.sys_arch.SetString("8");
			}
		}
		else if (win32.osversion.dwMajorVersion == 6 && win32.osversion.dwMinorVersion == 3) {
			if (IsWindowsServer())
			{
				win32.sys_arch.SetString("Server 2012 R2");
			}
			else
			{
				win32.sys_arch.SetString("8.1");
			}
		}
		else if (win32.osversion.dwMajorVersion == 10 && win32.osversion.dwMinorVersion == 0 && win32.osversion.dwBuildNumber < 22000) {
			if (IsWindowsServer())
			{
				if (win32.osversion.dwBuildNumber == 14393)
				{
					win32.sys_arch.SetString("Server 2016");
				}
				else if (win32.osversion.dwBuildNumber == 17763)
				{
					win32.sys_arch.SetString("Server 2019");
				}
				else if (win32.osversion.dwBuildNumber == 20348)
				{
					win32.sys_arch.SetString("Server 2022");
				}
				else
				{
					win32.sys_arch.SetString("Server 2016/2019/2022");
				}
			}
			else
			{
				win32.sys_arch.SetString("10");
			}
		}
		else if (win32.osversion.dwMajorVersion == 10 && win32.osversion.dwMinorVersion == 0 && win32.osversion.dwBuildNumber >= 22000) {
			if (IsWindowsServer())
			{
				win32.sys_arch.SetString("Server 2025");
			}
			else
			{
				win32.sys_arch.SetString("11");
			}
		}
		else {
			win32.sys_arch.SetString("Unknown NT variant");
		}
	} else if( win32.osversion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
		if( win32.osversion.dwMajorVersion == 4 && win32.osversion.dwMinorVersion == 0 ) {
			// Win95
			if( win32.osversion.szCSDVersion[1] == 'C' ) {
				win32.sys_arch.SetString( "Win95 OSR2 (95)" );
			} else {
				win32.sys_arch.SetString( "Win95 (95)" );
			}
		} else if( win32.osversion.dwMajorVersion == 4 && win32.osversion.dwMinorVersion == 10 ) {
			// Win98
			if( win32.osversion.szCSDVersion[1] == 'A' ) {
				win32.sys_arch.SetString( "Win98SE (95)" );
			} else {
				win32.sys_arch.SetString( "Win98 (95)" );
			}
		} else if( win32.osversion.dwMajorVersion == 4 && win32.osversion.dwMinorVersion == 90 ) {
			// WinMe
		  	win32.sys_arch.SetString( "WinMe (95)" );
		} else {
		  	win32.sys_arch.SetString( "Unknown 95 variant" );
		}
	} else {
		win32.sys_arch.SetString( "unknown Windows variant" );
	}

	//
	// CPU type
	//
	if ( !idStr::Icmp( win32.sys_cpustring.GetString(), "detect" ) ) {
		idStr string;

		common->Printf( "%1.0f MHz ", Sys_ClockTicksPerSecond() / 1000000.0f );

		win32.cpuid = Sys_GetCPUCapabilities();

		string.Clear();

		// Vendor
		if ( win32.cpuid & CPUID_AMD ) {
			string += "AMD CPU";
		} else if ( win32.cpuid & CPUID_INTEL ) {
			string += "Intel CPU";
		} else if ( win32.cpuid & CPUID_ARM ) {
			string += "ARM CPU";
		} else if ( win32.cpuid & CPUID_UNSUPPORTED ) {
			string += "unsupported CPU";
		} else {
			string += "generic CPU";
		}

		// Architecture
		if ( (win32.cpuid & CPUID_64BIT) && ((win32.cpuid & CPUID_INTEL) || (win32.cpuid & CPUID_AMD)) ) {
			string += " (x86_64)";
		} else if ((win32.cpuid & CPUID_32BIT) && ((win32.cpuid & CPUID_INTEL) || (win32.cpuid & CPUID_AMD))) {
			string += " (x86)";
		} else if ((win32.cpuid & CPUID_64BIT) && (win32.cpuid & CPUID_ARM)) {
			string += " (ARM64)";
		} else if ((win32.cpuid & CPUID_32BIT) && (win32.cpuid & CPUID_ARM)) {
			string += " (ARM)";
		} else {
			string += " (unknown architecture)";
		}

		// Features
		string += " with ";
		if ( win32.cpuid & CPUID_MMX ) {
			string += "MMX & ";
		}
		if ( win32.cpuid & CPUID_3DNOW ) {
			string += "3DNow! & ";
		}
		if ( win32.cpuid & CPUID_SSE ) {
			string += "SSE & ";
		}
		if ( win32.cpuid & CPUID_SSE2 ) {
            string += "SSE2 & ";
		}
		if ( win32.cpuid & CPUID_SSE3 ) {
			string += "SSE3 & ";
		}
		if ( win32.cpuid & CPUID_SSSE3 ) {
			string += "SSSE3 & ";
		}
		if ( win32.cpuid & CPUID_SSE4_1 ) {
			string += "SSE4.1 & ";
		}
		if ( win32.cpuid & CPUID_SSE4_2 ) {
			string += "SSE4.2 & ";
		}
		if ( win32.cpuid & CPUID_AVX ) {
			string += "AVX & ";
		}
		if ( win32.cpuid & CPUID_AVX2 ) {
			string += "AVX2 & ";
		}
		if ( win32.cpuid & CPUID_AVX512F ) {
			string += "AVX512F & ";
		}
		if ( win32.cpuid & CPUID_FMA3 ) {
			string += "FMA3 & ";
		}
		if ( win32.cpuid & CPUID_NEON ) {
			string += "NEON & ";
		}
		if ( win32.cpuid & CPUID_SVE ) {
			string += "SVE & ";
		}
		if ( win32.cpuid & CPUID_SVE2 ) {
			string += "SVE2 & ";
		}
		if (win32.cpuid & CPUID_SVE2_1) {
			string += "SVE2.1 & ";
		}
		if ( win32.cpuid & CPUID_SMT ) {
			string += "HTT & ";
		}
		string.StripTrailing( " & " );
		string.StripTrailing( " with " );
		win32.sys_cpustring.SetString( string );
	} else {
		common->Printf( "forcing CPU type to " );
		idLexer src( win32.sys_cpustring.GetString(), idStr::Length( win32.sys_cpustring.GetString() ), "sys_cpustring" );
		idToken token;

		int id = CPUID_NONE;
		while( src.ReadToken( &token ) ) {
			if ( token.Icmp( "generic" ) == 0 ) {
				id |= CPUID_GENERIC;
			} else if ( token.Icmp( "intel" ) == 0 ) {
				id |= CPUID_INTEL;
			} else if ( token.Icmp( "amd" ) == 0 ) {
				id |= CPUID_AMD;
			} else if ( token.Icmp( "mmx" ) == 0 ) {
				id |= CPUID_MMX;
			} else if ( token.Icmp( "3dnow" ) == 0 ) {
				id |= CPUID_3DNOW;
			} else if ( token.Icmp( "sse" ) == 0 ) {
				id |= CPUID_SSE;
			} else if ( token.Icmp( "sse2" ) == 0 ) {
				id |= CPUID_SSE2;
			} else if ( token.Icmp( "sse3" ) == 0 ) {
				id |= CPUID_SSE3;
			} else if ( token.Icmp( "ssse3" ) == 0) {
				id |= CPUID_SSSE3;
			} else if ( token.Icmp( "sse4.1" ) == 0 ) {
				id |= CPUID_SSE4_1;
			} else if ( token.Icmp( "sse4.2" ) == 0 ) {
				id |= CPUID_SSE4_2;
			} else if ( token.Icmp( "avx" ) == 0 ) {
				id |= CPUID_AVX;
			} else if ( token.Icmp( "avx2" ) == 0 ) {
				id |= CPUID_AVX2;
			} else if ( token.Icmp( "avx512f" ) == 0 ) {
				id |= CPUID_AVX512F;
			} else if ( token.Icmp( "fma3" ) == 0 ) {
				id |= CPUID_FMA3;
			} else if ( token.Icmp( "htt" ) == 0 ) {
				id |= CPUID_SMT;
			}
		}
		if ( id == CPUID_NONE ) {
			common->Printf( "WARNING: unknown sys_cpustring '%s'\n", win32.sys_cpustring.GetString() );
			id = CPUID_GENERIC;
		}
		win32.cpuid = static_cast<cpuid_t>(id);
	}

	common->Printf( "%s\n", win32.sys_cpustring.GetString() );
	common->Printf( "%d MB System Memory\n", Sys_GetSystemRam() );
	common->Printf( "%d MB Video Memory\n", Sys_GetVideoRam() );
	if ( ( win32.cpuid & CPUID_SSE2 ) == 0 ) {
		common->Error( "SSE2 not supported!" );
	}

	win32.g_Joystick.Init();
}

/*
================
Sys_Shutdown
================
*/
void Sys_Shutdown() {
	CoUninitialize();
}

/*
================
Sys_GetProcessorId
================
*/
cpuid_t Sys_GetProcessorId() {
    return win32.cpuid;
}

/*
================
Sys_GetProcessorString
================
*/
const char *Sys_GetProcessorString() {
	return win32.sys_cpustring.GetString();
}

//=======================================================================

//#define SET_THREAD_AFFINITY


/*
====================
Win_Frame
====================
*/
static void Win_Frame() {
	// if "viewlog" has been modified, show or hide the log console
	if ( win32.win_viewlog.IsModified() ) {
		win32.win_viewlog.ClearModified();
	}
}

extern "C" { void _chkstk( int size ); };

static void clrstk();

/*
====================
TestChkStk
====================
*/
static void TestChkStk() {
	int		buffer[0x1000] = {};

	buffer[0] = 1;
}

/*
====================
HackChkStk
====================
*/
static void HackChkStk() {
	DWORD	old;
	VirtualProtect( _chkstk, 6, PAGE_EXECUTE_READWRITE, &old );
	*(byte *)_chkstk = 0xe9;
	*(int *)((int)_chkstk+1) = (int)clrstk - (int)_chkstk - 5;

	TestChkStk();
}

/*
====================
GetExceptionCodeInfo
====================
*/
static const char *GetExceptionCodeInfo(const UINT code ) {
	switch( code ) {
		case EXCEPTION_ACCESS_VIOLATION: return "The thread tried to read from or write to a virtual address for which it does not have the appropriate access.";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.";
		case EXCEPTION_BREAKPOINT: return "A breakpoint was encountered.";
		case EXCEPTION_DATATYPE_MISALIGNMENT: return "The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.";
		case EXCEPTION_FLT_DENORMAL_OPERAND: return "One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "The thread tried to divide a floating-point value by a floating-point divisor of zero.";
		case EXCEPTION_FLT_INEXACT_RESULT: return "The result of a floating-point operation cannot be represented exactly as a decimal fraction.";
		case EXCEPTION_FLT_INVALID_OPERATION: return "This exception represents any floating-point exception not included in this list.";
		case EXCEPTION_FLT_OVERFLOW: return "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.";
		case EXCEPTION_FLT_STACK_CHECK: return "The stack overflowed or underflowed as the result of a floating-point operation.";
		case EXCEPTION_FLT_UNDERFLOW: return "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.";
		case EXCEPTION_ILLEGAL_INSTRUCTION: return "The thread tried to execute an invalid instruction.";
		case EXCEPTION_IN_PAGE_ERROR: return "The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.";
		case EXCEPTION_INT_DIVIDE_BY_ZERO: return "The thread tried to divide an integer value by an integer divisor of zero.";
		case EXCEPTION_INT_OVERFLOW: return "The result of an integer operation caused a carry out of the most significant bit of the result.";
		case EXCEPTION_INVALID_DISPOSITION: return "An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "The thread tried to continue execution after a noncontinuable exception occurred.";
		case EXCEPTION_PRIV_INSTRUCTION: return "The thread tried to execute an instruction whose operation is not allowed in the current machine mode.";
		case EXCEPTION_SINGLE_STEP: return "A trace trap or other single-instruction mechanism signaled that one instruction has been executed.";
		case EXCEPTION_STACK_OVERFLOW: return "The thread used up its stack.";
		default: return "Unknown exception";
	}
}

/*
====================
EmailCrashReport

  emailer originally from Raven/Quake 4
====================
*/
static void EmailCrashReport(const LPSTR messageText ) {
	static ID_TIME_T lastEmailTime = 0;

	if ( Sys_Milliseconds() < lastEmailTime + 10000 ) {
		return;
	}

	lastEmailTime = Sys_Milliseconds();

	HINSTANCE mapi = LoadLibrary( "MAPI32.DLL" ); 
	if( mapi ) {
		const LPMAPISENDMAIL	MAPISendMail = reinterpret_cast<LPMAPISENDMAIL>(GetProcAddress(mapi, "MAPISendMail"));
		if( MAPISendMail ) {
			MapiRecipDesc toProgrammers =
			{
				0,										// ulReserved
					MAPI_TO,							// ulRecipClass
					const_cast<LPSTR>("DOOM 3 BFG Crash"),						// lpszName
					const_cast<LPSTR>("SMTP:programmers@idsoftware.com"),	// lpszAddress
					0,									// ulEIDSize
					nullptr									// lpEntry
			};

			MapiMessage		message = {};
			message.lpszSubject = const_cast<LPSTR>("DOOM 3 BFG Fatal Error");
			message.lpszNoteText = messageText;
			message.nRecipCount = 1;
			message.lpRecips = &toProgrammers;

			MAPISendMail(
				0,									// LHANDLE lhSession
				0,									// ULONG ulUIParam
				&message,							// lpMapiMessage lpMessage
				MAPI_DIALOG,						// FLAGS flFlags
				0									// ULONG ulReserved
				);
		}
		FreeLibrary( mapi );
	}
}

size_t Sys_FPU_PrintStateFlags( char *ptr, int ctrl, int stat, int tags, DWORD inof, int inse, DWORD opof, int opse );

/*
====================
_except_handler
====================
*/
static EXCEPTION_DISPOSITION __cdecl _except_handler(const struct _EXCEPTION_RECORD *ExceptionRecord, void * EstablisherFrame,
                                                     struct _CONTEXT *ContextRecord, void * DispatcherContext ) {

	static char msg[ 8192 ] = {};
	char FPUFlags[2048] = {};

#if defined (ID_WIN64)
	Sys_FPU_PrintStateFlags(FPUFlags, ContextRecord->FltSave.ControlWord,
										ContextRecord->FltSave.StatusWord,
										ContextRecord->FltSave.TagWord,
										ContextRecord->FltSave.ErrorOffset,
										ContextRecord->FltSave.ErrorSelector,
										ContextRecord->FltSave.DataOffset,
										ContextRecord->FltSave.DataSelector);
#else
	Sys_FPU_PrintStateFlags( FPUFlags, ContextRecord->FloatSave.ControlWord,
										ContextRecord->FloatSave.StatusWord,
										ContextRecord->FloatSave.TagWord,
										ContextRecord->FloatSave.ErrorOffset,
										ContextRecord->FloatSave.ErrorSelector,
										ContextRecord->FloatSave.DataOffset,
										ContextRecord->FloatSave.DataSelector );
#endif

	sprintf( msg, 
		"Please describe what you were doing when DOOM 3 BFG crashed!\n"
		"If this text did not pop into your email client please copy and email it to programmers@idsoftware.com\n"
			"\n"
			"-= FATAL EXCEPTION =-\n"
			"\n"
			"%s\n"
			"\n"
			"0x%lx at address 0x%08p\n"
			"\n"
			"%s\n"
			"\n"
#if defined (ID_WIN64)
			"RAX = 0x%08llu RBX = 0x%08llu\n"
			"RCX = 0x%08llu RDX = 0x%08llu\n"
			"RSI = 0x%08llu RDI = 0x%08llu\n"
			"RBP = 0x%08llu RSP = 0x%08llu\n"
			"R8  = 0x%08llu R9  = 0x%08llu\n"
			"R10 = 0x%08llu R11 = 0x%08llu\n"
			"R12 = 0x%08llu R13 = 0x%08llu\n"
			"R14 = 0x%08llu R15 = 0x%08llu\n"
			"RIP = 0x%08llu EFL = 0x%08lx\n"
#else
			"EAX = 0x%08x EBX = 0x%08x\n"
			"ECX = 0x%08x EDX = 0x%08x\n"
			"ESI = 0x%08x EDI = 0x%08x\n"
			"EIP = 0x%08x ESP = 0x%08x\n"
			"EBP = 0x%08x EFL = 0x%08x\n"
#endif
			"\n"
			"CS = 0x%04x\n"
			"SS = 0x%04x\n"
			"DS = 0x%04x\n"
			"ES = 0x%04x\n"
			"FS = 0x%04x\n"
			"GS = 0x%04x\n"
			"\n"
			"%s\n",
			com_version.GetString(),
			ExceptionRecord->ExceptionCode,
			ExceptionRecord->ExceptionAddress,
			GetExceptionCodeInfo( ExceptionRecord->ExceptionCode ),
#if defined (ID_WIN64)
			ContextRecord->Rax, ContextRecord->Rbx,
			ContextRecord->Rcx, ContextRecord->Rdx,
			ContextRecord->Rsi, ContextRecord->Rdi,
			ContextRecord->Rbp, ContextRecord->Rsp,
			ContextRecord->R8,  ContextRecord->R9,
			ContextRecord->R10, ContextRecord->R11,
			ContextRecord->R12, ContextRecord->R13,
			ContextRecord->R14, ContextRecord->R15,
			ContextRecord->Rip,
#else
			ContextRecord->Eax, ContextRecord->Ebx,
			ContextRecord->Ecx, ContextRecord->Edx,
			ContextRecord->Esi, ContextRecord->Edi,
			ContextRecord->Eip, ContextRecord->Esp,
			ContextRecord->Ebp,
#endif
			ContextRecord->EFlags,
			ContextRecord->SegCs,
			ContextRecord->SegSs,
			ContextRecord->SegDs,
			ContextRecord->SegEs,
			ContextRecord->SegFs,
			ContextRecord->SegGs,
			FPUFlags
		);

	EmailCrashReport( msg );
	common->FatalError( msg );

    // Tell the OS to restart the faulting instruction
    return ExceptionContinueExecution;
}

#define TEST_FPU_EXCEPTIONS	/*	FPU_EXCEPTION_INVALID_OPERATION |		*/	\
							/*	FPU_EXCEPTION_DENORMALIZED_OPERAND |	*/	\
							/*	FPU_EXCEPTION_DIVIDE_BY_ZERO |			*/	\
							/*	FPU_EXCEPTION_NUMERIC_OVERFLOW |		*/	\
							/*	FPU_EXCEPTION_NUMERIC_UNDERFLOW |		*/	\
							/*	FPU_EXCEPTION_INEXACT_RESULT |			*/	\
								0

/*
==================
WinMain
==================
*/
int WINAPI WinMain(const HINSTANCE hInstance, HINSTANCE hPrevInstance, const LPSTR lpCmdLine, int nCmdShow ) {

	const HCURSOR hcurSave = ::SetCursor( LoadCursor( nullptr, IDC_WAIT ) );

	Sys_SetPhysicalWorkMemory( 192ULL << 20, 1024ULL << 20 );

	Sys_GetCurrentMemoryStatus( exeLaunchMemoryStats );

#if 0
    DWORD handler = (DWORD)_except_handler;
    __asm
    {                           // Build EXCEPTION_REGISTRATION record:
        push    handler         // Address of handler function
        push    FS:[0]          // Address of previous handler
        mov     FS:[0],ESP      // Install new EXCEPTION_REGISTRATION
    }
#endif

	win32.hInstance = hInstance;
	idStr::Copynz( sys_cmdline, lpCmdLine, sizeof( sys_cmdline ) );

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

#ifndef USE_STL_MUTEX
	for (auto& criticalSection : win32.criticalSections)
	{
		InitializeCriticalSection( &criticalSection);
	}
#endif

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	std::ignore = timeBeginPeriod( 1 );

	// get the initial time base
	Sys_Milliseconds();

#ifdef DEBUG
	// disable the painfully slow MS heap check every 1024 allocs
	_CrtSetDbgFlag( 0 );
#endif

//	Sys_FPU_EnableExceptions( TEST_FPU_EXCEPTIONS );
	Sys_FPU_SetPrecision( FPU_PRECISION_DOUBLE_EXTENDED );

	common->Init( 0, nullptr, lpCmdLine );

#if TEST_FPU_EXCEPTIONS != 0
	common->Printf( Sys_FPU_GetState() );
#endif

	if ( win32.win_notaskkeys.GetInteger() ) {
		DisableTaskKeys( TRUE, FALSE, /*( win32.win_notaskkeys.GetInteger() == 2 )*/ FALSE );
	}

	// hide or show the early console as necessary
	if ( win32.win_viewlog.GetInteger() ) {
		Sys_ShowConsole( 1, true );
	} else {
		Sys_ShowConsole( 0, false );
	}

#ifdef SET_THREAD_AFFINITY 
	// give the main thread an affinity for the first cpu
	SetThreadAffinityMask( GetCurrentThread(), 1 );
#endif

	::SetCursor( hcurSave );

	::SetFocus( win32.hWnd );

    // main game loop
	while( 1 ) {

		Win_Frame();

#if defined(DEBUG) || defined (_DEBUG)
		Sys_MemFrame();
#endif

		// set exceptions, even if some crappy syscall changes them!
		Sys_FPU_EnableExceptions( TEST_FPU_EXCEPTIONS );

		// run the game
		common->Frame();
	}

	// never gets here
	return 0;
}

/*
====================
clrstk

I tried to get the run time to call this at every function entry, but
====================
*/
static int	parmBytes;
__declspec( naked ) void clrstk() {
	// eax = bytes to add to stack
	__asm {
		mov		[parmBytes],eax
        neg     eax                     ; compute new stack pointer in eax
        add     eax,esp
        add     eax,4
        xchg    eax,esp
        mov     eax,dword ptr [eax]		; copy the return address
        push    eax
        
        ; clear to zero
        push	edi
        push	ecx
        mov		edi,esp
        add		edi,12
        mov		ecx,[parmBytes]
		shr		ecx,2
        xor		eax,eax
		cld
        rep	stosd
        pop		ecx
        pop		edi
        
        ret
	}
}

/*
==================
idSysLocal::OpenURL
==================
*/
void idSysLocal::OpenURL( const char *url, const bool doexit ) {
	static bool doexit_spamguard = false;
	HWND wnd;

	if (doexit_spamguard) {
		common->DPrintf( "OpenURL: already in an exit sequence, ignoring %s\n", url );
		return;
	}

	common->Printf("Open URL: %s\n", url);

	if ( !ShellExecute(nullptr, "open", url, nullptr, nullptr, SW_RESTORE ) ) {
		common->Error( "Could not open url: '%s' ", url );
		return;
	}

	wnd = GetForegroundWindow();
	if ( wnd ) {
		ShowWindow( wnd, SW_MAXIMIZE );
	}

	if ( doexit ) {
		doexit_spamguard = true;
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}

/*
==================
idSysLocal::StartProcess
==================
*/
void idSysLocal::StartProcess( const char *exePath, const bool doexit ) {
	TCHAR				szPathOrig[_MAX_PATH];
	STARTUPINFO			si;
	PROCESS_INFORMATION	pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);

	strncpy( szPathOrig, exePath, _MAX_PATH );

	if( !CreateProcess(nullptr, szPathOrig, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi ) ) {
        common->Error( "Could not start process: '%s' ", szPathOrig );
	    return;
	}

	if ( doexit ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}

/*
==================
Sys_SetFatalError
==================
*/
void Sys_SetFatalError( const char *error ) {
}

/*
================
Sys_SetLanguageFromSystem
================
*/
extern idCVar sys_lang;
void Sys_SetLanguageFromSystem() {
	sys_lang.SetString( Sys_DefaultLanguage() );
}
