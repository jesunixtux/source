//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: Global paint database for the server - tracks paintable entities.
//			Reconstructed minimal implementation for the open-source port.
//
//=============================================================================//

#ifndef PAINT_DATABASE_H
#define PAINT_DATABASE_H

#ifdef _WIN32
#pragma once
#endif

#include "paint_color_manager.h"
#include "basehandle.h"
#include "tier1/utlvector.h"

class CBaseEntity;

class CPaintDatabase
{
public:
	CPaintDatabase() {}
	bool AddPaint( CBaseEntity *entity, const Vector &position, const Vector &normal,
		PaintPowerType type, float radius, float alpha );

	// Removes all paint from every surface
	void RemoveAllPaint();

private:
	CUtlVector<CBaseHandle> m_paintedEntities;
};

// Global database instance (defined in server code)
extern CPaintDatabase PaintDatabase;

#endif // PAINT_DATABASE_H
