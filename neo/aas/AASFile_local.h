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

#ifndef __AASFILELOCAL_H__
#define __AASFILELOCAL_H__

#pragma once

/*
===============================================================================

	AAS File Local

===============================================================================
*/

class idAASFileLocal : public idAASFile {
	friend class idAASBuild;
	friend class idAASReach;
	friend class idAASCluster;
public:
								idAASFileLocal();
	virtual 					~idAASFileLocal();

public:
	[[nodiscard]] idVec3				EdgeCenter( Ordinal auto edgeNum ) const;
	[[nodiscard]] idVec3				FaceCenter( Ordinal auto faceNum ) const;
	[[nodiscard]] idVec3				AreaCenter( Ordinal auto areaNum ) const;

	[[nodiscard]] idBounds			EdgeBounds( Ordinal auto edgeNum ) const;
	[[nodiscard]] idBounds			FaceBounds( Ordinal auto faceNum ) const;
	[[nodiscard]] idBounds			AreaBounds( Ordinal auto areaNum ) const;

	[[nodiscard]] virtual int64					PointAreaNum( const idVec3 &origin ) const;
	[[nodiscard]] virtual int64					PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags, const int excludeTravelFlags ) const;
	[[nodiscard]] virtual int64					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const;
	        void				PushPointIntoAreaNum( Ordinal auto areaNum, idVec3 &point ) const;
	virtual bool				Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const;
	virtual void				PrintInfo() const;

public:
	bool						Load( const idStr &fileName, unsigned int mapFileCRC );
	bool						Write( const idStr &fileName, unsigned int mapFileCRC );

	[[nodiscard]] size_t		MemorySize() const;
	void						ReportRoutingEfficiency() const;
	void						Optimize();
	void						LinkReversedReachability();
	void						FinishAreas();

	void						Clear();
	void						DeleteReachabilities();
	void						DeleteClusters();

private:
	bool						ParseIndex( idLexer &src, idList<aasIndex_t> &indexes );
	bool						ParsePlanes( idLexer &src );
	bool						ParseVertices( idLexer &src );
	bool						ParseEdges( idLexer &src );
	bool						ParseFaces( idLexer &src );
	bool						ParseReachabilities( idLexer &src, int areaNum );
	bool						ParseAreas( idLexer &src );
	bool						ParseNodes( idLexer &src );
	bool						ParsePortals( idLexer &src );
	bool						ParseClusters( idLexer &src );

private:
	[[nodiscard]] int							BoundsReachableAreaNum_r( int nodeNum, const idBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const;
	              void					    	MaxTreeDepth_r( Ordinal auto nodeNum, int &depth, size_t &maxDepth ) const;
	[[nodiscard]] int							MaxTreeDepth() const;
	[[nodiscard]] int							AreaContentsTravelFlags( Ordinal auto areaNum ) const;
	[[nodiscard]] idVec3						AreaReachableGoal( Ordinal auto areaNum ) const;
	[[nodiscard]] int							NumReachabilities() const;
};

#endif /* !__AASFILELOCAL_H__ */
