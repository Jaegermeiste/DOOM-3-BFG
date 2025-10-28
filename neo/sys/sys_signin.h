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
#ifndef __SYS_SIGNIN_H__
#define __SYS_SIGNIN_H__

#pragma once

/*
================================================
idSignInManagerBase 
================================================
*/
class idSignInManagerBase {
public:

	idSignInManagerBase() noexcept :
		  minDesiredLocalUsers( 0 ),
		  maxDesiredLocalUsers( 0 ),
		  defaultProfile(nullptr) {}
	  virtual							~idSignInManagerBase() {}

	  virtual void					Pump() = 0;
	[[nodiscard]] virtual size_t	GetNumLocalUsers() const = 0;
	  virtual idLocalUser *			GetLocalUserByIndex( index_t index ) = 0;
	[[nodiscard]] virtual const idLocalUser *	GetLocalUserByIndex( index_t index ) const = 0;
	  virtual void					RemoveLocalUserByIndex( index_t index ) = 0;
	  virtual void					RegisterLocalUser( index_t inputDevice ) = 0;								// Register a local controller user to the passed in input device
	  virtual idLocalUser *			GetRegisteringUser() { return nullptr; }									// This is a user that has started the registration process but is not yet a local user.
	  virtual idLocalUser *			GetRegisteringUserByInputDevice( index_t inputDevice ) { return nullptr; }
	  virtual void					SignIn() {}
	  virtual bool					IsDeviceBeingRegistered( index_t inputDevice ) { return false; }
	  virtual bool					IsAnyDeviceBeingRegistered() { return false; }
	  virtual void					Shutdown() {}

	  // Outputs all the local users and other debugging information from the sign in manager
	  virtual void					DebugOutputLocalUserInfo() {}

	  //================================================================================
	  // Common helper functions
	  //================================================================================

	  void 					SetDesiredLocalUsers(const size_t minDesiredLocalUsers, const size_t maxDesiredLocalUsers ) { this->minDesiredLocalUsers = minDesiredLocalUsers; this->maxDesiredLocalUsers = maxDesiredLocalUsers; }
	  bool 					ProcessInputEvent( const sysEvent_t * ev );
	  idPlayerProfile *		GetDefaultProfile();

	  // Master user always index 0
	  idLocalUser *			GetMasterLocalUser() { return ( GetNumLocalUsers() > 0 ) ? GetLocalUserByIndex( 0 ) : nullptr; }
	[[nodiscard]] const idLocalUser *	GetMasterLocalUser() const { return ( GetNumLocalUsers() > 0 ) ? GetLocalUserByIndex( 0 ) : nullptr; }

	[[nodiscard]] bool 					IsMasterLocalUserPersistent() const { return ( GetMasterLocalUser() != nullptr) ? GetMasterLocalUser()->IsPersistent() : false; }
	[[nodiscard]] bool 					IsMasterLocalUserOnline() const { return ( GetMasterLocalUser() != nullptr) ? GetMasterLocalUser()->IsOnline() : false; }
	[[nodiscard]] int					GetMasterInputDevice() const { return ( GetMasterLocalUser() != nullptr) ? GetMasterLocalUser()->GetInputDevice() : -1; }
	[[nodiscard]] localUserHandle_t		GetMasterLocalUserHandle() const { return ( GetMasterLocalUser() != nullptr) ? GetMasterLocalUser()->GetLocalUserHandle() : localUserHandle_t(); }
	  idLocalUser *			GetLocalUserByInputDevice( index_t index );
	  idLocalUser *			GetLocalUserByHandle( localUserHandle_t handle );
	  idPlayerProfile *		GetPlayerProfileByInputDevice( index_t index );
	  bool					RemoveLocalUserByInputDevice( index_t index );
	  bool					RemoveLocalUserByHandle( localUserHandle_t handle );
	  void					RemoveAllLocalUsers();
	  void					SaveUserProfiles();

	  // This will remove local players that are not signed into a profile.
	  // If requiredOnline: This removes the users who cannot play online
	  void					ValidateLocalUsers( bool requireOnline );

	  bool					RequirePersistentMaster();

	  localUserHandle_t		GetUniqueLocalUserHandle( const char * name );

protected:
	size_t				minDesiredLocalUsers;
	size_t				maxDesiredLocalUsers;
	idPlayerProfile *	defaultProfile;
};

#endif	// __SYS_SIGNIN_H__
