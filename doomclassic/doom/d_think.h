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

#ifndef __D_THINK__
#define __D_THINK__

#pragma once

#ifdef __GNUG__
#pragma interface
#endif

//
// Experimental stuff.
// To compile this as "ANSI C with classes"
//  we will need to handle the various
//  action functions cleanly.
//

// One “universal” action type: pass N args as an array of void*
using actionf_t = void (*)(void* const* args, size_t nargs);

// ---- Generic adapter: deduce param types from the function pointer ----
template<auto F>
struct action_adapter;

// Partial specialization for plain function pointers with any parameter pack
template<typename... Ps, void (*F)(Ps...)>
struct action_adapter<F> {
	static void thunk(void* const* args, size_t nargs) noexcept {
		// Arity check (debug-time); remove if you prefer purely UB-free runtime
		if (nargs != sizeof...(Ps)) [[unlikely]] return;
		call(std::make_index_sequence<sizeof...(Ps)>{}, args);
	}

	static constexpr actionf_t ptr = &thunk;

private:
	template<size_t... I>
	static void call(std::index_sequence<I...>, void* const* args) noexcept {
		// For each parameter P:
		//  - if P is a pointer type, we expect args[I] is that pointer (void* upcast is OK)
		//  - if P is a non-pointer type, we expect args[I] points to that object (P*),
		//    and we dereference to pass by value.
		(F(arg_cast<Ps>(args[I])...), void());
	}

	template<typename P>
	static auto arg_cast(void* p) noexcept {
		if constexpr (std::is_pointer_v<P>) {
			// object/function pointer upcast back to P
			return static_cast<P>(p);
		}
		else {
			// treat arg slot as pointer-to-P and pass by value
			return *static_cast<std::add_pointer_t<std::remove_reference_t<P>>>(p);
		}
	}
};

// Convenience macro for deducing and creating erased action adapters automatically.
// Usage:  ACTION_ADAPTER(FunctionName)
// Expands to:  action_adapter<&FunctionName>::ptr
#define ACTIONF_T(fn) (action_adapter<&(fn)>::ptr)





// Historically, "think_t" is yet another
// function pointer to a routine to handle
// an actor.
typedef actionf_t  think_t;


// Doubly linked list of actors.
typedef struct thinker_s
{
    struct thinker_s*	prev;
    struct thinker_s*	next;
    think_t		function;
    
} thinker_t;



#endif

