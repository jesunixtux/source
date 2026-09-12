//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Laser beam entity for the reflective-cube puzzle mechanic.
//          Reconstructed from private Portal 2 source; provides the entity
//          surface used by this port's code.
//
//=============================================================================//

#ifndef ENV_PORTAL_LASER_H
#define ENV_PORTAL_LASER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "util_shared.h"
#include "entitylist.h"

DECLARE_AUTO_LIST( IPortalLaserAutoList );

class CPortalLaser : public CBaseEntity, public IPortalLaserAutoList
{
public:
	DECLARE_CLASS( CPortalLaser, CBaseEntity );
	DECLARE_DATADESC();

	CPortalLaser( void );
	virtual ~CPortalLaser( void );

	// Returns the closest point on the beam line to the given point
	Vector ClosestPointOnLineSegment( const Vector &vPoint );
};

#endif // ENV_PORTAL_LASER_H