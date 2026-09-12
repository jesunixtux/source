#ifndef PROJECTEDWALLENTITY_H
#define PROJECTEDWALLENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "paintable_entity.h"
#include "util_shared.h"

DECLARE_AUTO_LIST( IProjectedWallEntityAutoList );

class CProjectedWallEntity : public CBaseEntity, public IPaintableEntity, public IProjectedWallEntityAutoList
{
	DECLARE_CLASS( CProjectedWallEntity, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

public:
	CProjectedWallEntity();
	virtual ~CProjectedWallEntity() {}
	virtual void Touch( CBaseEntity *pOther );
	virtual PaintPowerType GetPaintPowerAtPoint( const Vector &worldContactPt ) const;
	virtual void Paint( PaintPowerType type, const Vector &worldContactPt );
	virtual void CleansePaint();
	virtual PaintPowerType GetPaintedPower() const { return NO_POWER; }

	int ComputeSegmentIndex( const Vector &vWorldPositionOnWall ) const;
	void DisplaceObstructingEntities( void );
	void DisplaceObstructingEntity( CBaseEntity *pEntity, bool bIgnoreStuck );
	void DisplaceObstructingEntity( CBaseEntity *pEntity, const Vector &vOrigin,
		const Vector &vWallUp, const Vector &vWallRight, bool bIgnoreStuck );
	void GetExtents( Vector &outMins, Vector &outMaxs, float flWidthScale = 1.0f );
	Vector GetLengthVector() const { return m_vecEndPoint - m_vecStartPoint; }
	Vector Up() const;

	int m_nNumSegments;
	CUtlVector< PaintPowerType > m_PaintPowers;
	Vector m_vecStartPoint;
	Vector m_vecEndPoint;
	float m_flLength;
	float m_flSegmentLength;
	Vector m_vWorldSpace_WallMins;
	Vector m_vWorldSpace_WallMaxs;
	float m_flWidth;
	float m_flHeight;
	bool m_bIsHorizontal;
};

#endif // PROJECTEDWALLENTITY_H
