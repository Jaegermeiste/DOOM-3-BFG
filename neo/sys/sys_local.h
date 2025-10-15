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

#ifndef __SYS_LOCAL__
#define __SYS_LOCAL__

/*
==============================================================

	idSysLocal

==============================================================
*/

class idSysLocal : public idSys {
public:
	void			DebugPrintf( VERIFY_FORMAT_STRING const char *fmt, ... ) override;
	void			DebugVPrintf( const char *fmt, va_list arg ) override;

	double			GetClockTicks() override;
	double			ClockTicksPerSecond() override;
	cpuid_t			GetProcessorId() override;
	const char *	GetProcessorString() override;
	const char *	FPU_GetState() override;
	bool			FPU_StackIsEmpty() override;
	void			FPU_SetFTZ( bool enable ) override;
	void			FPU_SetDAZ( bool enable ) override;

	void			FPU_EnableExceptions( int exceptions ) override;

	virtual void			GetCallStack( address_t *callStack, const size_t callStackSize );
	virtual const char *	GetCallStackStr( const address_t *callStack, const size_t callStackSize );
	virtual const char *	GetCallStackCurStr(size_t depth );
	void			ShutdownSymbols() override;

	virtual bool			LockMemory( void *ptr, size_t bytes );
	virtual bool			UnlockMemory( void *ptr, size_t bytes );

	dllHandle_t 	DLL_Load( const char *dllName ) override;
	address_t		DLL_GetProcAddress( dllHandle_t dllHandle, const char *procName ) override;
	void			DLL_Unload( dllHandle_t dllHandle ) override;
	virtual void			DLL_GetFileName( const char *baseName, char *dllName, size_t maxLength );

	sysEvent_t		GenerateMouseButtonEvent( int button, bool down ) override;
	sysEvent_t		GenerateMouseMoveEvent( int deltax, int deltay ) override;

	void			OpenURL( const char *url, bool quit ) override;
	void			StartProcess( const char *exeName, bool quit ) override;
};

#endif /* !__SYS_LOCAL__ */
