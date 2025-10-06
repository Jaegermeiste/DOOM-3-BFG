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
#include "../idlib/precompiled.h"

#include "tr_local.h"

idRenderEntityLocal::idRenderEntityLocal() {
	memset( &parms, 0, sizeof( parms ) );
	memset( modelMatrix, 0, sizeof( modelMatrix ) );

	world					= nullptr;
	index					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	dynamicModel			= nullptr;
	dynamicModelFrameCount	= 0;
	cachedDynamicModel		= nullptr;
	localReferenceBounds	= bounds_zero;
	globalReferenceBounds	= bounds_zero;
	viewCount				= 0;
	viewEntity				= nullptr;
	decals					= nullptr;
	overlays				= nullptr;
	entityRefs				= nullptr;
	firstInteraction		= nullptr;
	lastInteraction			= nullptr;
	needsPortalSky			= false;
}

void idRenderEntityLocal::FreeRenderEntity() {
}

void idRenderEntityLocal::UpdateRenderEntity( const renderEntity_t *re, bool forceUpdate ) {
}

void idRenderEntityLocal::GetRenderEntity( renderEntity_t *re ) {
}

void idRenderEntityLocal::ForceUpdate() {
}

int idRenderEntityLocal::GetIndex() {
	return index;
}

void idRenderEntityLocal::ProjectOverlay( const idPlane localTextureAxis[2], const idMaterial *material ) {
}
void idRenderEntityLocal::RemoveDecals() {
}

//======================================================================

idRenderLightLocal::idRenderLightLocal() {
	memset( &parms, 0, sizeof( parms ) );
	memset( lightProject, 0, sizeof( lightProject ) );

	lightHasMoved			= false;
	world					= nullptr;
	index					= 0;
	areaNum					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	lightShader				= nullptr;
	falloffImage			= nullptr;
	globalLightOrigin		= vec3_zero;
	viewCount				= 0;
	viewLight				= nullptr;
	references				= nullptr;
	foggedPortals			= nullptr;
	firstInteraction		= nullptr;
	lastInteraction			= nullptr;

	baseLightProject.Zero();
	inverseBaseLightProject.Zero();
}

void idRenderLightLocal::FreeRenderLight() {
}
void idRenderLightLocal::UpdateRenderLight( const renderLight_t *re, bool forceUpdate ) {
}
void idRenderLightLocal::GetRenderLight( renderLight_t *re ) {
}
void idRenderLightLocal::ForceUpdate() {
}
int idRenderLightLocal::GetIndex() {
	return index;
}
