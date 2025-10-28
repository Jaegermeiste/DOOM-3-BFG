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

#ifndef __MATH_SIMD_SSE_H__
#define __MATH_SIMD_SSE_H__

#pragma once

/*
===============================================================================

	SSE implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_SSE : public idSIMD_Generic {
public:
	[[nodiscard]] const char * VPCALL GetName() const override;

	void VPCALL BlendJoints( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const jointHandle_t* index, const size_t numJoints ) override;
	void VPCALL BlendJointsFast( idJointQuat *joints, const idJointQuat *blendJoints, const float lerp, const jointHandle_t* index, const size_t numJoints ) override;
	void VPCALL ConvertJointQuatsToJointMats( idJointMat *jointMats, const idJointQuat *jointQuats, const size_t numJoints ) override;
	void VPCALL ConvertJointMatsToJointQuats( idJointQuat *jointQuats, const idJointMat *jointMats, const size_t numJoints ) override;
	void VPCALL TransformJoints( idJointMat *jointMats, const jointHandle_t*parents, const jointHandle_t firstJoint, const jointHandle_t lastJoint ) override;
	void VPCALL UntransformJoints( idJointMat *jointMats, const jointHandle_t*parents, const jointHandle_t firstJoint, const jointHandle_t lastJoint ) override;
};

#endif /* !__MATH_SIMD_SSE_H__ */
