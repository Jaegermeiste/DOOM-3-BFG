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

#ifndef __AAS_LOCAL_H__
#define __AAS_LOCAL_H__

#pragma once

#include "AAS.h"
#include "../Pvs.h"


class idRoutingCache {
	friend class idAASLocal;

public:
								idRoutingCache( size_t size );
								~idRoutingCache();

	int							Size() const;

private:
	int							type;					// portal or area cache
	size_t						size;					// size of cache
	int							cluster;				// cluster of the cache
	int							areaNum;				// area of the cache
	int							travelFlags;			// combinations of the travel flags
	idRoutingCache *			next;					// next in list
	idRoutingCache *			prev;					// previous in list
	idRoutingCache *			time_next;				// next in time based list
	idRoutingCache *			time_prev;				// previous in time based list
	unsigned short				startTravelTime;		// travel time to start with
	unsigned char *				reachabilities;			// reachabilities used for routing
	unsigned short *			travelTimes;			// travel time for every area
};


class idRoutingUpdate {
	friend class idAASLocal;

private:
	int							cluster;				// cluster number of this update
	int							areaNum;				// area number of this update
	ID_TIME_T    				tmpTravelTime;			// temporary travel time
	ID_TIME_T *     			areaTravelTimes;		// travel times within the area
	idVec3						start;					// start point into area
	idRoutingUpdate *			next;					// next in list
	idRoutingUpdate *			prev;					// prev in list
	bool						isInList;				// true if the update is in the list
};


class idRoutingObstacle {
	friend class idAASLocal;
								idRoutingObstacle() { }

private:
	idBounds					bounds;					// obstacle bounds
	idList<int, TAG_AAS>		areas;					// areas the bounds are in
};


class idAASLocal : public idAAS {
public:
								idAASLocal();
								~idAASLocal() override;
								bool				Init( const idStr &mapName, unsigned int mapFileCRC ) override;
	virtual void				Shutdown();
								void				Stats() const override;
								void				Test( const idVec3 &origin ) override;
								const idAASSettings *GetSettings() const override;
								int					PointAreaNum( const idVec3 &origin ) const override;
								int					PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags ) const override;
								int					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags ) const override;
								void				PushPointIntoAreaNum( index_t areaNum, idVec3 &origin ) const override;
								idVec3				AreaCenter( index_t areaNum ) const override;
								int					AreaFlags( index_t areaNum ) const override;
								int					AreaTravelFlags( index_t areaNum ) const override;
								bool				Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const override;
								const idPlane &		GetPlane( index_t planeNum ) const override;
								int					GetWallEdges( index_t areaNum, const idBounds &bounds, int travelFlags, int *edges, size_t maxEdges ) const override;
								void				SortWallEdges( int *edges, size_t numEdges ) const override;
								void				GetEdgeVertexNumbers( index_t edgeNum, int verts[2] ) const override;
								void				GetEdge( index_t edgeNum, idVec3 &start, idVec3 &end ) const override;
								bool				SetAreaState( const idBounds &bounds, const int areaContents, bool disabled ) override;
								aasHandle_t			AddObstacle( const idBounds &bounds ) override;
								void				RemoveObstacle( const aasHandle_t handle ) override;
								void				RemoveAllObstacles() override;
								ID_TIME_T			TravelTimeToGoalArea( index_t areaNum, const idVec3 &origin, index_t goalAreaNum, int travelFlags ) const override;
								bool				RouteToGoalArea( index_t areaNum, const idVec3 origin, index_t goalAreaNum, int travelFlags, int &travelTime, idReachability **reach ) const override;
								bool				WalkPathToGoal( aasPath_t &path, index_t areaNum, const idVec3 &origin, index_t goalAreaNum, const idVec3 &goalOrigin, int travelFlags ) const override;
								bool				WalkPathValid( index_t areaNum, const idVec3 &origin, index_t goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idVec3 &endPos, index_t &endAreaNum ) const override;
								bool				FlyPathToGoal( aasPath_t &path, index_t areaNum, const idVec3 &origin, index_t goalAreaNum, const idVec3 &goalOrigin, int travelFlags ) const override;
								bool				FlyPathValid( index_t areaNum, const idVec3 &origin, index_t goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idVec3 &endPos, index_t &endAreaNum ) const override;
								void				ShowWalkPath( const idVec3 &origin, index_t goalAreaNum, const idVec3 &goalOrigin ) const override;
								void				ShowFlyPath( const idVec3 &origin, index_t goalAreaNum, const idVec3 &goalOrigin ) const override;
								bool				FindNearestGoal( aasGoal_t &goal, index_t areaNum, const idVec3 origin, const idVec3 &target, int travelFlags, aasObstacle_t *obstacles, size_t numObstacles, idAASCallback &callback ) const override;

private:
	idAASFile *					file;
	idStr						name;

private:	// routing data
	idRoutingCache ***			areaCacheIndex;			// for each area in each cluster the travel times to all other areas in the cluster
	int							areaCacheIndexSize;		// number of area cache entries
	idRoutingCache **			portalCacheIndex;		// for each area in the world the travel times from each portal
	int							portalCacheIndexSize;	// number of portal cache entries
	idRoutingUpdate *			areaUpdate;				// memory used to update the area routing cache
	idRoutingUpdate *			portalUpdate;			// memory used to update the portal routing cache
	unsigned short *			goalAreaTravelTimes;	// travel times to goal areas
	unsigned short *			areaTravelTimes;		// travel times through the areas
	int							numAreaTravelTimes;		// number of area travel times
	mutable idRoutingCache *	cacheListStart;			// start of list with cache sorted from oldest to newest
	mutable idRoutingCache *	cacheListEnd;			// end of list with cache sorted from oldest to newest
	mutable int					totalCacheMemory;		// total cache memory used
	idList<idRoutingObstacle *, TAG_AAS>	obstacleList;			// list with obstacles

private:	// routing
	bool						SetupRouting();
	void						ShutdownRouting();
	unsigned short				AreaTravelTime( index_t areaNum, const idVec3 &start, const idVec3 &end ) const;
	void						CalculateAreaTravelTimes();
	void						DeleteAreaTravelTimes();
	void						SetupRoutingCache();
	void						DeleteClusterCache( int clusterNum );
	void						DeletePortalCache();
	void						ShutdownRoutingCache();
	void						RoutingStats() const;
	void						LinkCache( idRoutingCache *cache ) const;
	void						UnlinkCache( idRoutingCache *cache ) const;
	void						DeleteOldestCache() const;
	idReachability *			GetAreaReachability( index_t areaNum, int reachabilityNum ) const;
	int							ClusterAreaNum( int clusterNum, index_t areaNum ) const;
	void						UpdateAreaRoutingCache( idRoutingCache *areaCache ) const;
	idRoutingCache *			GetAreaRoutingCache( int clusterNum, index_t areaNum, int travelFlags ) const;
	void						UpdatePortalRoutingCache( idRoutingCache *portalCache ) const;
	idRoutingCache *			GetPortalRoutingCache( int clusterNum, index_t areaNum, int travelFlags ) const;
	void						RemoveRoutingCacheUsingArea( index_t areaNum );
	void						DisableArea( index_t areaNum );
	void						EnableArea( index_t areaNum );
	bool						SetAreaState_r( index_t nodeNum, const idBounds &bounds, const int areaContents, bool disabled );
	void						GetBoundsAreas_r( index_t nodeNum, const idBounds &bounds, idList<int> &areas ) const;
	void						SetObstacleState( const idRoutingObstacle *obstacle, bool enable );

private:	// pathing
	bool						EdgeSplitPoint( idVec3 &split, index_t edgeNum, const idPlane &plane ) const;
	bool						FloorEdgeSplitPoint( idVec3 &split, index_t areaNum, const idPlane &splitPlane, const idPlane &frontPlane, bool closest ) const;
	idVec3						SubSampleWalkPath( index_t areaNum, const idVec3 &origin, const idVec3 &start, const idVec3 &end, int travelFlags, index_t &endAreaNum ) const;
	idVec3						SubSampleFlyPath( index_t areaNum, const idVec3 &origin, const idVec3 &start, const idVec3 &end, int travelFlags, index_t &endAreaNum ) const;

private:	// debug
	const idBounds &			DefaultSearchBounds() const;
	void						DrawCone( const idVec3 &origin, const idVec3 &dir, float radius, const idVec4 &color ) const;
	void						DrawArea( index_t areaNum ) const;
	void						DrawFace( index_t faceNum, bool side ) const;
	void						DrawEdge( index_t edgeNum, bool arrow ) const;
	void						DrawReachability( const idReachability *reach ) const;
	void						ShowArea( const idVec3 &origin ) const;
	void						ShowWallEdges( const idVec3 &origin ) const;
	void						ShowHideArea( const idVec3 &origin, int targerAreaNum ) const;
	bool						PullPlayer( const idVec3 &origin, int toAreaNum ) const;
	void						RandomPullPlayer( const idVec3 &origin ) const;
	void						ShowPushIntoArea( const idVec3 &origin ) const;
};

#endif /* !__AAS_LOCAL_H__ */
