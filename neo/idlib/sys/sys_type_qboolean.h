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

#ifndef __SYS_TYPE_QBOOLEAN__
#define __SYS_TYPE_QBOOLEAN__

#pragma once

#include <concepts>

class qboolean {
public:
	constexpr qboolean()
	{
		value = QBOOLEAN_INVALID;
	}

	constexpr qboolean( const bool b )
	{
		if (b == false)
		{
			value = QBOOLEAN_FALSE;
		}
		else if (b == true)
		{
			value = QBOOLEAN_TRUE;
		}
		else
		{
			value = QBOOLEAN_INVALID;
		}
	}

	constexpr qboolean( const std::integral auto& i ) {
		if (i == 0)
		{
			value = QBOOLEAN_FALSE;
		}
		else if (i == 1)
		{
			value = QBOOLEAN_TRUE;
		}
		else
		{
			value = QBOOLEAN_INVALID;
		}
	}

	constexpr qboolean( const std::floating_point auto& f )
	{
		if (f == 0.0f)
		{
			value = QBOOLEAN_FALSE;
		}
		else if (f == 1.0f)
		{
			value = QBOOLEAN_TRUE;
		}
		else
		{
			value = QBOOLEAN_INVALID;
		}
	}

	constexpr bool is_true() const noexcept { return ( value == QBOOLEAN_TRUE ); }
	constexpr bool is_false() const noexcept { return ( value == QBOOLEAN_FALSE ); }
	constexpr bool is_invalid() const noexcept { return (( value != QBOOLEAN_TRUE ) && ( value != QBOOLEAN_FALSE )); }
	constexpr bool value_or( bool fallback ) const noexcept
	{
		if (is_invalid())
		{
			return fallback;
		}
		else if (is_true())
		{
			return true;
		}
		else if (is_false())
		{
			return false;
		}

		return false;
	}

	constexpr explicit operator bool() const noexcept { return value_or(false); }

	const char* c_str() const {
		if (is_true())
		{
			return "true";
		}
		else if (is_false())
		{
			return "false";
		}

		return "<invalid>";
	}

	operator const char* () {
		return c_str();
	}

	operator const char* () const {
		return c_str();
	}

private:
	enum qbooleanValue_e : int8
	{
		QBOOLEAN_INVALID = -1,
		QBOOLEAN_FALSE = 0,
		QBOOLEAN_TRUE = 1,
		QBOOLEAN_MAX
	};

	qbooleanValue_e value = QBOOLEAN_INVALID;
};

#endif // __SYS_TYPE_QBOOLEAN__