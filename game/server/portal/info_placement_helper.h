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

#if defined( PORTAL2 )
class CInfoPlacementHelper : public CBaseAnimating
{
	DECLARE_CLASS( CInfoPlacementHelper, CBaseAnimating );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

public:
	CInfoPlacementHelper();

	void Spawn( void ) override;
	void UpdateOnRemove( void ) override;

	float GetTargetRadius() const { return m_flTargetRadius; }
	const QAngle &GetTargetAngles() const;
	bool ShouldUseHelperAngles() const { return m_bSnapToHelperAngles; }
	bool IsEnabled() const { return m_bEnabled; }
	bool ForcePlacement() const { return m_bForcePlacement; }
	bool HideUntilPlaced() const { return m_bHideUntilPlaced; }
	float GetTargetSize() const { return m_flTargetSize; }
	bool UsesSizeLimit() const { return m_bUsesSizeLimit; }

	static CUtlVector< CInfoPlacementHelper * > &Helpers();

private:
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	float m_flTargetRadius;
	QAngle m_angTargetAngles;
	float m_flTargetSize;
	bool m_bStartDisabled;
	bool m_bEnabled;
	bool m_bSnapToHelperAngles;
	bool m_bForcePlacement;
	bool m_bHideUntilPlaced;
	bool m_bUsesSizeLimit;
};

CInfoPlacementHelper *UTIL_FindPlacementHelper( const Vector &vecPosition, CBasePlayer *pPlayer );
#else
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
#endif

#endif // INFO_PLACEMENT_HELPER_H
