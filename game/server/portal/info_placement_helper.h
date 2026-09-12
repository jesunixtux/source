//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: Server-side stub for the placement helper entity used by the
//          portal gun. Full helper logic will be imported later; for now this
//          lets shared weapon code compile and link.
//
//=============================================================================//

#ifndef INFO_PLACEMENT_HELPER_H
#define INFO_PLACEMENT_HELPER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

class CInfoPlacementHelper : public CBaseAnimating
{
	DECLARE_CLASS( CInfoPlacementHelper, CBaseAnimating );
public:
	float GetTargetRadius() const { return 0.0f; }
};

inline CInfoPlacementHelper *UTIL_FindPlacementHelper( const Vector &vecPosition, CBasePlayer *pPlayer )
{
	return NULL;
}

#endif // INFO_PLACEMENT_HELPER_H
