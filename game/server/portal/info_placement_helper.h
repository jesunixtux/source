//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: Server-authoritative Portal 2 placement helper contract.
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
	int UpdateTransmitState( void ) override;

	float GetTargetRadius() const { return m_flTargetRadius; }
	const QAngle &GetTargetAngles() const;
	bool ShouldUseHelperAngles() const { return m_bSnapToHelperAngles; }
	bool IsEnabled() const { return m_bEnabled; }
	bool ForcePlacement() const { return m_bForcePlacement; }
	bool HideUntilPlaced() const { return m_bHideUntilPlaced; }
	float GetTargetSize() const { return m_flTargetSize; }
	bool UsesSizeLimit() const { return m_bUsesSizeLimit; }

private:
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	CNetworkVar( float, m_flTargetRadius );
	CNetworkVar( float, m_flTargetSize );
	CNetworkVar( bool, m_bEnabled );
	CNetworkVar( bool, m_bSnapToHelperAngles );
	CNetworkVar( bool, m_bForcePlacement );
	CNetworkVar( bool, m_bHideUntilPlaced );
	CNetworkVar( bool, m_bUsesSizeLimit );

	bool m_bStartDisabled;
	string_t m_iszProxyName;
	string_t m_iszAttachTargetName;
	COutputEvent m_OnObjectPlaced;
	COutputInt m_OnObjectPlacedSize;
};

CInfoPlacementHelper *UTIL_FindPlacementHelper( const Vector &vecPosition, CBasePlayer *pPlayer );
#else
class CInfoPlacementHelper : public CBaseAnimating
{
	DECLARE_CLASS( CInfoPlacementHelper, CBaseAnimating );
public:
	float GetTargetRadius() const { return 0.0f; }
	const QAngle &GetTargetAngles() const { return GetAbsAngles(); }
	bool ShouldUseHelperAngles() const { return false; }
};

inline CInfoPlacementHelper *UTIL_FindPlacementHelper( const Vector &vecPosition, CBasePlayer *pPlayer )
{
	return NULL;
}
#endif

#endif // INFO_PLACEMENT_HELPER_H
