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

#ifndef __SYS_PUBLIC__
#define __SYS_PUBLIC__

#pragma once

#include "../idlib/CmdArgs.h"

/*
===============================================================================

	Non-portable system services.

===============================================================================
*/

enum cpuid_t : uint64 {
	CPUID_NONE							= 0x00000,
	CPUID_UNSUPPORTED					= 0x00001,	 // unsupported (386/486)
	CPUID_GENERIC						= 0x00002,	 // unrecognized processor
	CPUID_INTEL							= 0x00004,	 // Intel
	CPUID_AMD							= 0x00008,	 // AMD
	CPUID_MMX							= 0x00010,	 // Multi Media Extensions
	CPUID_3DNOW							= 0x00020,	 // 3DNow!
	CPUID_SSE							= 0x00040,	 // Streaming SIMD Extensions
	CPUID_SSE2							= 0x00080,	 // Streaming SIMD Extensions 2
	CPUID_SSE3							= 0x00100,	 // Streaming SIMD Extensions 3 aka Prescott's New Instructions
	CPUID_ALTIVEC						= 0x00200,	 // AltiVec
	CPUID_SMT							= 0x01000,	 // Hyper-Threading Technology
	CPUID_CMOV							= 0x02000,	 // Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
	CPUID_FTZ							= 0x04000,	 // Flush-To-Zero mode (denormal results are flushed to zero)
	CPUID_DAZ							= 0x08000,	 // Denormals-Are-Zero mode (denormal source operands are set to zero)
	CPUID_XENON							= 0x10000,	 // Xbox 360
	CPUID_CELL							= 0x20000,	 // PS3

	CPUID_SSSE3                         = 0x40000,	 // Supplemental Streaming SIMD Extensions 3
	CPUID_SSE4_1                        = 0x80000,	 // Streaming SIMD Extensions 4.1
	CPUID_SSE4_2                        = 0x100000,	 // Streaming SIMD Extensions 4.2
	CPUID_AVX                           = 0x200000,	 // Advanced Vector Extensions
	CPUID_AVX2                          = 0x400000,	 // Advanced Vector Extensions 2
	CPUID_AVX512F                       = 0x800000,	 // Advanced Vector Extensions 512 Foundation
	CPUID_FMA3                          = 0x1000000, // Fused Multiply Add 3

	CPUID_32BIT                         = 0x4000000, // Any 32-bit processor
	CPUID_64BIT                         = 0x8000000, // Any 64-bit processor

	CPUID_ARM                           = 0x10000000, // Any ARM processor
	CPUID_NEON                          = 0x20000000, // ARM NEON
	CPUID_SVE                           = 0x40000000, // ARM SVE
	CPUID_SVE2                          = 0x80000000, // ARM SVE2
	CPUID_SVE2_1                        = 0x100000000 // ARM SVE2.1
};

typedef enum fpuExceptions_e : uint8 {
	FPU_EXCEPTION_INVALID_OPERATION		= 1,
	FPU_EXCEPTION_DENORMALIZED_OPERAND	= 2,
	FPU_EXCEPTION_DIVIDE_BY_ZERO		= 4,
	FPU_EXCEPTION_NUMERIC_OVERFLOW		= 8,
	FPU_EXCEPTION_NUMERIC_UNDERFLOW		= 16,
	FPU_EXCEPTION_INEXACT_RESULT		= 32
} fpuExceptions_t;

typedef enum fpuPrecision_e : uint8 {
	FPU_PRECISION_SINGLE				= 0,
	FPU_PRECISION_DOUBLE				= 1,
	FPU_PRECISION_DOUBLE_EXTENDED		= 2
} fpuPrecision_t;

typedef enum fpuRounding_e : uint8 {
	FPU_ROUNDING_TO_NEAREST				= 0,
	FPU_ROUNDING_DOWN					= 1,
	FPU_ROUNDING_UP						= 2,
	FPU_ROUNDING_TO_ZERO				= 3
} fpuRounding_t;

typedef enum joystickAxis_e : uint8 {
	AXIS_LEFT_X,
	AXIS_LEFT_Y,
	AXIS_RIGHT_X,
	AXIS_RIGHT_Y,
	AXIS_LEFT_TRIG,
	AXIS_RIGHT_TRIG,
	MAX_JOYSTICK_AXIS
} joystickAxis_t;

typedef enum sysEventType_e : uint8 {
	SE_NONE,				// evTime is still valid
	SE_KEY,					// evValue is a key code, evValue2 is the down flag
	SE_CHAR,				// evValue is an ascii char
	SE_MOUSE,				// evValue and evValue2 are reletive signed x / y moves
	SE_MOUSE_ABSOLUTE,		// evValue and evValue2 are absolute coordinates in the window's client area.
	SE_MOUSE_LEAVE,			// evValue and evValue2 are meaninless, this indicates the mouse has left the client area.
	SE_JOYSTICK,		// evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE				// evPtr is a char*, from typing something at a non-game console
} sysEventType_t;

enum sys_mEvents_e : uint8 {
	M_ACTION1,
	M_ACTION2,
	M_ACTION3,
	M_ACTION4,
	M_ACTION5,
	M_ACTION6,
	M_ACTION7,
	M_ACTION8,
	M_DELTAX,
	M_DELTAY,
	M_DELTAZ,
	M_INVALID
};

typedef enum usercmdButton_e : uint8 {
	UB_NONE,

	UB_MOVEUP,
	UB_MOVEDOWN,
	UB_LOOKLEFT,
	UB_LOOKRIGHT,
	UB_MOVEFORWARD,
	UB_MOVEBACK,
	UB_LOOKUP,
	UB_LOOKDOWN,
	UB_MOVELEFT,
	UB_MOVERIGHT,

	UB_ATTACK,
	UB_SPEED,
	UB_ZOOM,
	UB_SHOWSCORES,
	UB_USE,

	UB_IMPULSE0,
	UB_IMPULSE1,
	UB_IMPULSE2,
	UB_IMPULSE3,
	UB_IMPULSE4,
	UB_IMPULSE5,
	UB_IMPULSE6,
	UB_IMPULSE7,
	UB_IMPULSE8,
	UB_IMPULSE9,
	UB_IMPULSE10,
	UB_IMPULSE11,
	UB_IMPULSE12,
	UB_IMPULSE13,
	UB_IMPULSE14,
	UB_IMPULSE15,
	UB_IMPULSE16,
	UB_IMPULSE17,
	UB_IMPULSE18,
	UB_IMPULSE19,
	UB_IMPULSE20,
	UB_IMPULSE21,
	UB_IMPULSE22,
	UB_IMPULSE23,
	UB_IMPULSE24,
	UB_IMPULSE25,
	UB_IMPULSE26,
	UB_IMPULSE27,
	UB_IMPULSE28,
	UB_IMPULSE29,
	UB_IMPULSE30,
	UB_IMPULSE31,

	UB_MAX_BUTTONS
} usercmdButton_t;

enum sys_jEvents_e : uint8 {
	J_EVENT_NONE = 0,
	J_ACTION1,
	J_ACTION2,
	J_ACTION3,
	J_ACTION4,
	J_ACTION5,
	J_ACTION6,
	J_ACTION7,
	J_ACTION8,
	J_ACTION9,
	J_ACTION10,
	J_ACTION11,
	J_ACTION12,
	J_ACTION13,
	J_ACTION14,
	J_ACTION15,
	J_ACTION16,
	J_ACTION17,
	J_ACTION18,
	J_ACTION19,
	J_ACTION20,
	J_ACTION21,
	J_ACTION22,
	J_ACTION23,
	J_ACTION24,
	J_ACTION25,
	J_ACTION26,
	J_ACTION27,
	J_ACTION28,
	J_ACTION29,
	J_ACTION30,
	J_ACTION31,
	J_ACTION32,
	J_ACTION_MAX = J_ACTION32,

	J_AXIS_MIN = J_ACTION_MAX + 1,
	J_AXIS_LEFT_X = J_AXIS_MIN + static_cast<int>(AXIS_LEFT_X),
	J_AXIS_LEFT_Y = J_AXIS_MIN + static_cast<int>(AXIS_LEFT_Y),
	J_AXIS_RIGHT_X = J_AXIS_MIN + static_cast<int>(AXIS_RIGHT_X),
	J_AXIS_RIGHT_Y = J_AXIS_MIN + static_cast<int>(AXIS_RIGHT_Y),
	J_AXIS_LEFT_TRIG = J_AXIS_MIN + static_cast<int>(AXIS_LEFT_TRIG),
	J_AXIS_RIGHT_TRIG = J_AXIS_MIN + static_cast<int>(AXIS_RIGHT_TRIG),

	J_AXIS_MAX = J_AXIS_MIN + static_cast<int>(MAX_JOYSTICK_AXIS) - 1,

	J_DPAD_UP,
	J_DPAD_DOWN,
	J_DPAD_LEFT,
	J_DPAD_RIGHT,

	MAX_JOY_EVENT
};

/*
================================================
The first part of this table maps directly to Direct Input scan codes (DIK_* from dinput.h)
But they are duplicated here for console portability
================================================
*/
typedef enum keyNum_e : int16 {
	K_INVALID = -1,
	K_NONE,

	K_ESCAPE,
	K_1,
	K_2,
	K_3,
	K_4,
	K_5,
	K_6,
	K_7,
	K_8,
	K_9,
	K_0,
	K_MINUS,
	K_EQUALS,
	K_BACKSPACE,
	K_TAB,
	K_Q,
	K_W,
	K_E,
	K_R,
	K_T,
	K_Y,
	K_U,
	K_I,
	K_O,
	K_P,
	K_LBRACKET,
	K_RBRACKET,
	K_ENTER,
	K_LCTRL,
	K_A,
	K_S,
	K_D,
	K_F,
	K_G,
	K_H,
	K_J,
	K_K,
	K_L,
	K_SEMICOLON,
	K_APOSTROPHE,
	K_GRAVE,
	K_LSHIFT,
	K_BACKSLASH,
	K_Z,
	K_X,
	K_C,
	K_V,
	K_B,
	K_N,
	K_M,
	K_COMMA,
	K_PERIOD,
	K_SLASH,
	K_RSHIFT,
	K_KP_STAR,
	K_LALT,
	K_SPACE,
	K_CAPSLOCK,
	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_NUMLOCK,
	K_SCROLL,
	K_KP_7,
	K_KP_8,
	K_KP_9,
	K_KP_MINUS,
	K_KP_4,
	K_KP_5,
	K_KP_6,
	K_KP_PLUS,
	K_KP_1,
	K_KP_2,
	K_KP_3,
	K_KP_0,
	K_KP_DOT,
	K_F11			= 0x57,
	K_F12			= 0x58,
	K_F13			= 0x64,
	K_F14			= 0x65,
	K_F15			= 0x66,
	K_KANA			= 0x70,
	K_CONVERT		= 0x79,
	K_NOCONVERT		= 0x7B,
	K_YEN			= 0x7D,
	K_KP_EQUALS		= 0x8D,
	K_CIRCUMFLEX	= 0x90,
	K_AT			= 0x91,
	K_COLON			= 0x92,
	K_UNDERLINE		= 0x93,
	K_KANJI			= 0x94,
	K_STOP			= 0x95,
	K_AX			= 0x96,
	K_UNLABELED		= 0x97,
	K_KP_ENTER		= 0x9C,
	K_RCTRL			= 0x9D,
	K_KP_COMMA		= 0xB3,
	K_KP_SLASH		= 0xB5,
	K_PRINTSCREEN	= 0xB7,
	K_RALT			= 0xB8,
	K_PAUSE			= 0xC5,
	K_HOME			= 0xC7,
	K_UPARROW		= 0xC8,
	K_PGUP			= 0xC9,
	K_LEFTARROW		= 0xCB,
	K_RIGHTARROW	= 0xCD,
	K_END			= 0xCF,
	K_DOWNARROW		= 0xD0,
	K_PGDN			= 0xD1,
	K_INS			= 0xD2,
	K_DEL			= 0xD3,
	K_LWIN			= 0xDB,
	K_RWIN			= 0xDC,
	K_APPS			= 0xDD,
	K_POWER			= 0xDE,
	K_SLEEP			= 0xDF,

	//------------------------
	// K_JOY codes must be contiguous, too
	//------------------------

	K_JOY1 = 256,
	K_JOY2,
	K_JOY3,
	K_JOY4,
	K_JOY5,
	K_JOY6,
	K_JOY7,
	K_JOY8,
	K_JOY9,
	K_JOY10,
	K_JOY11,
	K_JOY12,
	K_JOY13,
	K_JOY14,
	K_JOY15,
	K_JOY16,

	K_JOY_STICK1_UP,
	K_JOY_STICK1_DOWN,
	K_JOY_STICK1_LEFT,
	K_JOY_STICK1_RIGHT,

	K_JOY_STICK2_UP,
	K_JOY_STICK2_DOWN,
	K_JOY_STICK2_LEFT,
	K_JOY_STICK2_RIGHT,

	K_JOY_TRIGGER1,
	K_JOY_TRIGGER2,

	K_JOY_DPAD_UP,
	K_JOY_DPAD_DOWN,
	K_JOY_DPAD_LEFT,
	K_JOY_DPAD_RIGHT,

	//------------------------
	// K_MOUSE enums must be contiguous (no char codes in the middle)
	//------------------------

	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,

	K_MWHEELDOWN,
	K_MWHEELUP,

	K_LAST_KEY
} keyNum_t;

struct sysEvent_t {
	sysEventType_t	evType;
	int				evValue;
	int				evValue2;
	size_t			evPtrLength;		// bytes of data pointed to by evPtr, for journaling
	void *			evPtr;				// this must be manually freed if not NULL

	index_t			inputDevice;
	[[nodiscard]] bool		IsKeyEvent() const { return evType == SE_KEY; }
	[[nodiscard]] bool		IsMouseEvent() const { return evType == SE_MOUSE; }
	[[nodiscard]] bool		IsCharEvent() const { return evType == SE_CHAR; }
	[[nodiscard]] bool		IsJoystickEvent() const { return evType == SE_JOYSTICK; }
	[[nodiscard]] bool		IsKeyDown() const { return evValue2 != 0; }
	[[nodiscard]] keyNum_t	GetKey() const { return static_cast< keyNum_t >( evValue ); }
	[[nodiscard]] int		GetXCoord() const { return evValue; }
	[[nodiscard]] int		GetYCoord() const { return evValue2; }
};

struct sysMemoryStats_t {
	uint32 memoryLoad;
	size_t totalPhysical;
	size_t availPhysical;
	size_t totalPageFile;
	size_t availPageFile;
	size_t totalVirtual;
	size_t availVirtual;
	size_t availExtendedVirtual;
};

#if defined (ID_WIN64) || defined (ID_WIN32)
typedef INT_PTR address_t;
typedef HINSTANCE dllHandle_t;
#else
typedef uintptr_t address_t;
typedef int dllHandle_t;
#endif // defined (ID_WIN64) || defined (ID_WIN32)


void			Sys_Init();
void			Sys_Shutdown();
void			Sys_Error( const char *error, ...);
const char *	Sys_GetCmdLine();
void			Sys_ReLaunch( void * launchData, size_t launchDataSize );
void			Sys_Launch( const char * path, idCmdArgs & args,  void * launchData, size_t launchDataSize );
void			Sys_SetLanguageFromSystem();
const char *	Sys_DefaultLanguage();
void			Sys_Quit();

bool			Sys_AlreadyRunning();

// note that this isn't journaled...
char *			Sys_GetClipboardData();
void			Sys_SetClipboardData( const char *string );

// will go to the various text consoles
// NOT thread safe - never use in the async paths
void			Sys_Printf( VERIFY_FORMAT_STRING const char *msg, ... );

// guaranteed to be thread-safe
void			Sys_DebugPrintf( VERIFY_FORMAT_STRING const char *fmt, ... );
void			Sys_DebugVPrintf( const char *fmt, va_list arg );

// a decent minimum sleep time to avoid going below the process scheduler speeds
constexpr ID_TIME_T SYS_MINSLEEP = 20;

// allow game to yield CPU time
// NOTE: due to SYS_MINSLEEP this is very bad portability karma, and should be completely removed
void			Sys_Sleep( const ID_TIME_T msec );

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
ID_TIME_T		Sys_Milliseconds();
uint64  		Sys_Microseconds();

// for accurate performance testing
double			Sys_GetClockTicks();
double			Sys_ClockTicksPerSecond();

// returns a selection of the CPUID_* flags
cpuid_t			Sys_GetProcessorId();
const char *	Sys_GetProcessorString();

// returns true if the FPU stack is empty
bool			Sys_FPU_StackIsEmpty();

// empties the FPU stack
void			Sys_FPU_ClearStack();

// returns the FPU state as a string
const char *	Sys_FPU_GetState();

// enables the given FPU exceptions
void			Sys_FPU_EnableExceptions( int exceptions );

// sets the FPU precision
void			Sys_FPU_SetPrecision( int precision );

// sets the FPU rounding mode
void			Sys_FPU_SetRounding( uint8 rounding );

// sets Flush-To-Zero mode (only available when CPUID_FTZ is set)
void			Sys_FPU_SetFTZ( bool enable );

// sets Denormals-Are-Zero mode (only available when CPUID_DAZ is set)
void			Sys_FPU_SetDAZ( bool enable );

// returns amount of system ram
size_t			Sys_GetSystemRam();

// returns amount of video ram
size_t			Sys_GetVideoRam();

// returns amount of drive space in path
size_t			Sys_GetDriveFreeSpace( const char *path );

// returns amount of drive space in path in bytes
size_t			Sys_GetDriveFreeSpaceInBytes( const char * path );

// returns memory stats
void			Sys_GetCurrentMemoryStatus( sysMemoryStats_t &stats );
void			Sys_GetExeLaunchMemoryStatus( sysMemoryStats_t &stats );

// lock and unlock memory
bool			Sys_LockMemory( void *ptr, size_t bytes );
bool			Sys_UnlockMemory( void *ptr, size_t bytes );

// set amount of physical work memory
void			Sys_SetPhysicalWorkMemory(size_t minBytes, size_t maxBytes );

// allows retrieving the call stack at execution points
size_t			Sys_GetCallStack( address_t *callStack, const size_t callStackSize, const size_t skipFrames );
const char *	Sys_GetCallStackStr( const address_t *callStack, const size_t callStackSize );
const char *	Sys_GetCallStackCurStr( const size_t depth );
const char *	Sys_GetCallStackCurAddressStr( const size_t depth );
void			Sys_ShutdownSymbols();

// DLL loading, the path should be a fully qualified OS path to the DLL file to be loaded
dllHandle_t		Sys_DLL_Load( const char* dllName );
address_t		Sys_DLL_GetProcAddress( dllHandle_t dllHandle, const char *procName );
void			Sys_DLL_Unload( dllHandle_t dllHandle );

// event generation
void			Sys_GenerateEvents();
sysEvent_t		Sys_GetEvent();
void			Sys_ClearEvents();

// input is tied to windows, so it needs to be started up and shut down whenever 
// the main window is recreated
void			Sys_InitInput();
void			Sys_ShutdownInput();

// keyboard input polling
size_t			Sys_PollKeyboardInputEvents();
keyNum_t		Sys_ReturnKeyboardInputEvent( const Ordinal auto n, keyNum_t &ch, bool &state );
void			Sys_EndKeyboardInputEvents();

// mouse input polling
static constexpr size_t MAX_MOUSE_EVENTS = 256;
size_t			Sys_PollMouseInputEvents( int mouseEvents[MAX_MOUSE_EVENTS][2] );

// joystick input polling
void			Sys_SetRumble( index_t device, int low, int hi );
size_t			Sys_PollJoystickInputEvents( index_t deviceNum );
int				Sys_ReturnJoystickInputEvent( const sys_jEvents_e n, int &action, int &value );
void			Sys_EndJoystickInputEvents();

// when the console is down, or the game is about to perform a lengthy
// operation like map loading, the system can release the mouse cursor
// when in windowed mode
void			Sys_GrabMouseCursor( bool grabIt );

void			Sys_ShowWindow( bool show );
bool			Sys_IsWindowVisible();
void			Sys_ShowConsole( int visLevel, bool quitOnClose );

// This really isn't the right place to have this, but since this is the 'top level' include
// and has a function signature with 'FILE' in it, it kinda needs to be here =/
typedef HANDLE idFileHandle;


ID_TIME_T		Sys_FileTimeStamp( idFileHandle fp );
// NOTE: do we need to guarantee the same output on all platforms?
const char *	Sys_TimeStampToStr( ID_TIME_T timeStamp );
const char *	Sys_SecToStr( int sec );

const char *	Sys_DefaultBasePath();
const char *	Sys_DefaultSavePath();

// know early if we are performing a fatal error shutdown so the error message doesn't get lost
void			Sys_SetFatalError( const char *error );

// Execute the specified process and wait until it's done, calling workFn every waitMS milliseconds.
// If showOutput == true, std IO from the executed process will be output to the console.
// Note that the return value is not an indication of the exit code of the process, but is false
// only if the process could not be created at all. If you wish to check the exit code of the 
// spawned process, check the value returned in exitCode.
typedef bool ( *execProcessWorkFunction_t )();
typedef void ( *execOutputFunction_t)( const char * text );
bool Sys_Exec(	const char * appPath, const char * workingPath, const char * args, 
	execProcessWorkFunction_t workFn, execOutputFunction_t outputFn, const uint32 waitMS,
	unsigned int & exitCode );

// localization

constexpr auto ID_LANG_ENGLISH  = "english";
constexpr auto ID_LANG_FRENCH   = "french";
constexpr auto ID_LANG_ITALIAN  = "italian";
constexpr auto ID_LANG_GERMAN   = "german";
constexpr auto ID_LANG_SPANISH  = "spanish";
constexpr auto ID_LANG_JAPANESE = "japanese";
size_t Sys_NumLangs();
const char * Sys_Lang( int idx );

/*
==============================================================

	Networking

==============================================================
*/

typedef enum netadrtype_e : uint8 {
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IPv4,
	NA_IPv6
} netadrtype_t;

enum netadrflags_e : uint16
{
	NETADR_F_LOOPBACK     = 0x00000001, // 127.0.0.1 or ::1
	NETADR_F_MULTICAST    = 0x00000002, // 224/4 or ff00::/8
	NETADR_F_LINKLOCAL    = 0x00000004, // 169.254/16 or fe80::/10
	NETADR_F_PRIVATE_V4   = 0x00000008, // RFC1918 (10/8, 172.16/12, 192.168/16)
	NETADR_F_CGNAT_V4     = 0x00000010, // 100.64/10 (carrier-grade NAT)
	NETADR_F_ULA_V6       = 0x00000020, // fc00::/7 (unique local)
	NETADR_F_DOC          = 0x00000040,	// documentation ranges (v4+v6; IPv4: 192.0.2.0/24, 198.51.100.0/24, 203.0.113.0/24; IPv6: 2001 : db8:: / 32)
	NETADR_F_UNSPECIFIED  = 0x00000080, // 0.0.0.0 or ::
	NETADR_F_ANYCAST      = 0x00000100, // only when source says so (cannot infer from bits)
	NETADR_F_V4MAPPED_V6  = 0x00000200, // ::ffff:w.x.y.z (if you ever store v4-mapped)
	NETADR_F_GLOBAL       = 0x00000400, // public unicast (derived: not any of the above)
	NETADR_F_SITELOCAL_V6 = 0x00000800, // fec0::/10 (deprecated; still seen sometimes)
	NETADR_F_MULTICAST_S  = 0x00001000, // “has scope nibble” (use aux field below)
	NETADR_F_BROADCAST_V4 = 0x00002000  // 255.255.255.255 (limited broadcast)
};

// Legacy IPv4-only layout (what old clients expect)
#pragma pack(push, 1)
struct netadr_legacy_t {
	int32         type;   // cast of netadrtype_t; original → int32 on wire assume same numeric values for NA_IPv4/NA_BROADCAST/NA_LOOPBACK
	uint8         ip[4];  // IPv4 only
	uint16        port;   // stored in network byte order
};
#pragma pack(pop)

constexpr size_t LEGACY_NETADR_WIRE_SIZE = sizeof(netadr_legacy_t); // should be 10

// "NAD2" (0x4E414402) big-endian; beware host endianness.
// We'll write bytes explicitly to avoid UB.
static const uint8 NADR_MAGIC10[LEGACY_NETADR_WIRE_SIZE] = {
	0x4E, 0x41, 0x44, 0x02, // 'N','A','D','2'
	0x00, 0x00, 0x00, 0x00, // invalid IPv4 address
	0xFF, 0xFF              // port 0xFFFF (USHRT_MAX)
};

struct netadr_t {
	netadrtype_t type;        // address type (IPv4, IPv6, etc.)
	uint32       flags;

	union addr_u {
		uint8           ip4[4];  // raw IPv4 address bytes
		uint8           ip6[16]; // raw IPv6 address bytes
		struct in_addr  in4;
		struct in6_addr in6;
	} addr;
	uint16       port;        // network byte order (use htons / ntohs)

	// For IPv6 only (ignored for IPv4): interface scope (zone index)
	uint32       v6_scope_id;   // corresponds to sockaddr_in6.sin6_scope_id, required for link-local addresses (fe80::/10) to be usable.
	uint8        v6_mcast_scope;  // 0 if not v6 multicast; else 0x1=if, 0x2=link, 0x5=site, 0xE=global
};

constexpr auto PORT_ANY = -1;

/*
================================================
idUDP
================================================
*/
class idUDP {
public:
	// this just zeros netSocket and port
				idUDP();
	virtual		~idUDP();

	// if the InitForPort fails, the idUDP.port field will remain 0
	bool		InitForPort( uint16 portNumber );

	[[nodiscard]] netadrtype_t GetType() const { return bound_to.type; }
	[[nodiscard]] uint16	   GetPort() const { return bound_to.port; }
	[[nodiscard]] netadr_t	   GetAdr() const { return bound_to; }
	[[nodiscard]] uint32	   GetUIntIPV4Adr() const { return ( bound_to.addr.ip4[0] | bound_to.addr.ip4[1] << 8 | bound_to.addr.ip4[2] << 16 | bound_to.addr.ip4[3] << 24 ); }

	void		Close();

	bool		GetPacket( netadr_t &from, void *data, size_t &size, const size_t maxSize );
	
	bool		GetPacketBlocking( netadr_t &from, void *data, size_t &size, const size_t maxSize, const ID_TIME_T timeout );

	void		SendPacket( const netadr_t& to, const void *data, const size_t size );

	void		SetSilent( const bool silent ) { this->silent = silent; }
	[[nodiscard]] bool		GetSilent() const { return silent; }

	size_t		packetsRead;
	size_t		bytesRead;

	size_t		packetsWritten;
	size_t		bytesWritten;

	[[nodiscard]] bool		IsOpen() const { return netSocket > 0; }

private:
	netadr_t	bound_to;		// interface and port
#if defined(ID_WIN64) || defined (ID_WIN32)
	SOCKET      netSocket;
#else
	int			netSocket;		// OS specific socket
#endif
	bool		silent;			// don't emit anything ( black hole )
};



				// parses the port number
				// can also do DNS resolve if you ask for it.
				// NOTE: DNS resolve is a slow/blocking call, think before you use
				// ( could be exploited for server DoS )
bool			Sys_StringToNetAdr( const char *s, netadr_t *adr, const bool doDNSResolve );
const char *	Sys_NetAdrToString( const netadr_t& adr );
bool			Sys_IsLANAddress( const netadr_t& adr );
bool			Sys_CompareNetAdrBase( const netadr_t& a, const netadr_t& b );

size_t			Sys_GetLocalIPCount();
const char *	Sys_GetLocalIP( index_t i );

void			Sys_InitNetworking();
void			Sys_ShutdownNetworking();



/*
================================================
idJoystick is managed by each platform's local Sys implementation, and 
provides full *Joy Pad* support (the most common device, these days).
================================================
*/
class idJoystick {
public:
	virtual			~idJoystick() { }

	virtual bool	Init() { return false; }
	virtual void	Shutdown() { }
	virtual void	Deactivate() { }
	virtual void	SetRumble( index_t deviceNum, int rumbleLow, int rumbleHigh ) { }
	virtual size_t	PollInputEvents( index_t inputDeviceNum ) { return 0; }
	virtual int		ReturnInputEvent( const int n, int& action, int& value ) { return 0; }
	virtual void	EndInputEvents() { }
};



/*
==============================================================

	idSys

==============================================================
*/

class idSys {
public:
	virtual void			DebugPrintf( VERIFY_FORMAT_STRING const char *fmt, ... ) = 0;
	virtual void			DebugVPrintf( const char *fmt, va_list arg ) = 0;

	virtual double			GetClockTicks() = 0;
	virtual double			ClockTicksPerSecond() = 0;
	virtual cpuid_t			GetProcessorId() = 0;
	virtual const char *	GetProcessorString() = 0;
	virtual const char *	FPU_GetState() = 0;
	virtual bool			FPU_StackIsEmpty() = 0;
	virtual void			FPU_SetFTZ( bool enable ) = 0;
	virtual void			FPU_SetDAZ( bool enable ) = 0;

	virtual void			FPU_EnableExceptions( int exceptions ) = 0;

	virtual bool			LockMemory( void *ptr, int bytes ) = 0;
	virtual bool			UnlockMemory( void *ptr, int bytes ) = 0;

	virtual void			GetCallStack( address_t *callStack, const int callStackSize ) = 0;
	virtual const char *	GetCallStackStr( const address_t *callStack, const int callStackSize ) = 0;
	virtual const char *	GetCallStackCurStr( int depth ) = 0;
	virtual void			ShutdownSymbols() = 0;

	virtual dllHandle_t		DLL_Load( const char *dllName ) = 0;
	virtual address_t		DLL_GetProcAddress( dllHandle_t dllHandle, const char *procName ) = 0;
	virtual void			DLL_Unload( dllHandle_t dllHandle ) = 0;
	virtual void			DLL_GetFileName( const char *baseName, char *dllName, size_t maxLength ) = 0;

	virtual sysEvent_t		GenerateMouseButtonEvent( int button, bool down ) = 0;
	virtual sysEvent_t		GenerateMouseMoveEvent( int deltax, int deltay ) = 0;

	virtual void			OpenURL( const char *url, bool quit ) = 0;
	virtual void			StartProcess( const char *exePath, bool quit ) = 0;
};

extern idSys *				sys;

bool Sys_LoadOpenAL();
void Sys_FreeOpenAL();


#endif /* !__SYS_PUBLIC__ */
