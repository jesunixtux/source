//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ENGINE_IENGINETRACE_H
#define ENGINE_IENGINETRACE_H
#ifdef _WIN32
#pragma once
#endif

#include "basehandle.h"
#include "utlvector.h" //need CUtlVector for IEngineTrace::GetBrushesIn*()
#include "mathlib/vector4d.h"
#include "vphysics/virtualmesh.h"

class Vector;
class IHandleEntity;
struct Ray_t;
class CGameTrace;
typedef CGameTrace trace_t;
class ICollideable;
class QAngle;
class CTraceListData;
class CPhysCollide;
struct cplane_t;

//-----------------------------------------------------------------------------
// The standard trace filter... NOTE: Most normal traces inherit from CTraceFilter!!!
//-----------------------------------------------------------------------------
enum TraceType_t
{
	TRACE_EVERYTHING = 0,
	TRACE_WORLD_ONLY,				// NOTE: This does *not* test static props!!!
	TRACE_ENTITIES_ONLY,			// NOTE: This version will *not* test static props
	TRACE_EVERYTHING_FILTER_PROPS,	// NOTE: This version will pass the IHandleEntity for props through the filter, unlike all other filters
};

abstract_class ITraceFilter
{
public:
	virtual bool ShouldHitEntity( IHandleEntity *pEntity, int contentsMask ) = 0;
	virtual TraceType_t	GetTraceType() const = 0;
};


//-----------------------------------------------------------------------------
// Classes are expected to inherit these + implement the ShouldHitEntity method
//-----------------------------------------------------------------------------

// This is the one most normal traces will inherit from
class CTraceFilter : public ITraceFilter
{
public:
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_EVERYTHING;
	}
};

class CTraceFilterEntitiesOnly : public ITraceFilter
{
public:
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}
};


//-----------------------------------------------------------------------------
// Classes need not inherit from these
//-----------------------------------------------------------------------------
class CTraceFilterWorldOnly : public ITraceFilter
{
public:
	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		return false;
	}
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_WORLD_ONLY;
	}
};

class CTraceFilterWorldAndPropsOnly : public ITraceFilter
{
public:
	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		return false;
	}
	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_EVERYTHING;
	}
};

class CTraceFilterHitAll : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{ 
		return true; 
	}
};


//-----------------------------------------------------------------------------
// Enumeration interface for EnumerateLinkEntities
//-----------------------------------------------------------------------------
abstract_class IEntityEnumerator
{
public:
	// This gets called with each handle
	virtual bool EnumEntity( IHandleEntity *pHandleEntity ) = 0; 
};


//-----------------------------------------------------------------------------
struct BrushSideInfo_t
{
	cplane_t		plane;		// The plane of the brush side
	unsigned short	bevel;		// Bevel plane?
	unsigned short	thin;		// Thin?
};

class CBrushQuery
{
public:
	CBrushQuery( void )
	{
		m_iCount = 0;
		m_pBrushes = NULL;
		m_iMaxBrushSides = 0;
		m_pReleaseFunc = NULL;
		m_pData = NULL;
	}
	~CBrushQuery( void )
	{
		ReleasePrivateData();
	}
	void ReleasePrivateData( void )
	{
		if ( m_pReleaseFunc )
		{
			m_pReleaseFunc( this );
		}

		m_iCount = 0;
		m_pBrushes = NULL;
		m_iMaxBrushSides = 0;
		m_pReleaseFunc = NULL;
		m_pData = NULL;
	}

	inline int Count( void ) const { return m_iCount; }
	inline uint32 *Base( void ) { return m_pBrushes; }
	inline uint32 operator[]( int iIndex ) const { return m_pBrushes[iIndex]; }
	inline uint32 GetBrushNumber( int iIndex ) const { return m_pBrushes[iIndex]; }

	inline int MaxBrushSides( void ) const { return m_iMaxBrushSides; }

	// Adopt ownership of a brush index array allocated with new[].
	void AdoptBrushes( uint32 *pBrushes, int count, int maxSides )
	{
		ReleasePrivateData();
		m_iCount = count;
		m_pBrushes = pBrushes;
		m_iMaxBrushSides = maxSides;
		m_pReleaseFunc = &DefaultRelease;
	}

	static void DefaultRelease( CBrushQuery *pQuery )
	{
		delete[] pQuery->m_pBrushes;
		pQuery->m_pBrushes = NULL;
	}

protected:
	int		m_iCount;
	uint32	*m_pBrushes;
	int		m_iMaxBrushSides;
	void	(*m_pReleaseFunc)( CBrushQuery * );
	void	*m_pData;
};

// Interface the engine exposes to the game DLL
//-----------------------------------------------------------------------------
#define INTERFACEVERSION_ENGINETRACE_SERVER	"EngineTraceServer004"
#define INTERFACEVERSION_ENGINETRACE_CLIENT	"EngineTraceClient004"
abstract_class IEngineTrace
{
public:
	// Returns the contents mask + entity at a particular world-space position
	virtual int		GetPointContents( const Vector &vecAbsPosition, IHandleEntity** ppEntity = NULL ) = 0;
	
	// Get the point contents, but only test the specific entity. This works
	// on static props and brush models.
	//
	// If the entity isn't a static prop or a brush model, it returns CONTENTS_EMPTY and sets
	// bFailed to true if bFailed is non-null.
	virtual int		GetPointContents_Collideable( ICollideable *pCollide, const Vector &vecAbsPosition ) = 0;

	// Traces a ray against a particular entity
	virtual void	ClipRayToEntity( const Ray_t &ray, unsigned int fMask, IHandleEntity *pEnt, trace_t *pTrace ) = 0;

	// Traces a ray against a particular entity
	virtual void	ClipRayToCollideable( const Ray_t &ray, unsigned int fMask, ICollideable *pCollide, trace_t *pTrace ) = 0;

	// A version that simply accepts a ray (can work as a traceline or tracehull)
	virtual void	TraceRay( const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace ) = 0;

	// A version that sets up the leaf and entity lists and allows you to pass those in for collision.
	virtual void	SetupLeafAndEntityListRay( const Ray_t &ray, CTraceListData &traceData ) = 0;
	virtual void    SetupLeafAndEntityListBox( const Vector &vecBoxMin, const Vector &vecBoxMax, CTraceListData &traceData ) = 0;
	virtual void	TraceRayAgainstLeafAndEntityList( const Ray_t &ray, CTraceListData &traceData, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace ) = 0;

	// A version that sweeps a collideable through the world
	// abs start + abs end represents the collision origins you want to sweep the collideable through
	// vecAngles represents the collision angles of the collideable during the sweep
	virtual void	SweepCollideable( ICollideable *pCollide, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
		const QAngle &vecAngles, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace ) = 0;

	// Enumerates over all entities along a ray
	// If triggers == true, it enumerates all triggers along a ray
	virtual void	EnumerateEntities( const Ray_t &ray, bool triggers, IEntityEnumerator *pEnumerator ) = 0;

	// Same thing, but enumerate entitys within a box
	virtual void	EnumerateEntities( const Vector &vecAbsMins, const Vector &vecAbsMaxs, IEntityEnumerator *pEnumerator ) = 0;

	// Convert a handle entity to a collideable.  Useful inside enumer
	virtual ICollideable *GetCollideable( IHandleEntity *pEntity ) = 0;

	// HACKHACK: Temp for performance measurments
	virtual int GetStatByIndex( int index, bool bClear ) = 0;


	//finds brushes in an AABB, prone to some false positives
	virtual void GetBrushesInAABB( const Vector &vMins, const Vector &vMaxs, CUtlVector<int> *pOutput, int iContentsMask = 0xFFFFFFFF ) = 0;
	virtual void GetBrushesInAABB( const Vector &vMins, const Vector &vMaxs, CBrushQuery &BrushQuery, int iContentsMask = 0xFFFFFFFF, int cmodelIndex = 0 ) = 0;

	// Portal 2: finds brushes within a collideable's bounding box.
	virtual void GetBrushesInCollideable( ICollideable *pCollideable, CBrushQuery &BrushQuery ) = 0;

	//Creates a CPhysCollide out of all displacements wholly or partially contained in the specified AABB
	virtual CPhysCollide* GetCollidableFromDisplacementsInAABB( const Vector& vMins, const Vector& vMaxs ) = 0;

	//retrieve brush planes and contents, returns zero if the brush doesn't exist,
	//returns positive number of sides filled out if the array can hold them all, negative number of slots needed to hold info if the array is too small
	virtual int GetBrushInfo( int iBrush, int &ContentsOut, BrushSideInfo_t *pBrushSideInfoOut, int iBrushSideInfoArraySize ) = 0;

	virtual bool PointOutsideWorld( const Vector &ptTest ) = 0; //Tests a point to see if it's outside any playable area

	// Walks bsp to find the leaf containing the specified point
	virtual int GetLeafContainingPoint( const Vector &ptTest ) = 0;

	// Portal 2: collect displacement mesh lists overlapping an AABB.
	virtual int GetMeshesFromDisplacementsInAABB( const Vector &vMins, const Vector &vMaxs, virtualmeshlist_t *pMeshes, int nMaxMeshes ) = 0;
};



#endif // ENGINE_IENGINETRACE_H
