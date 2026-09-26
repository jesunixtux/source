#ifndef C_INFO_PLACEMENT_HELPER_H
#define C_INFO_PLACEMENT_HELPER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"

#if defined( PORTAL2 )
class C_InfoPlacementHelper : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_InfoPlacementHelper, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	const QAngle &GetTargetAngles( void ) const;
	bool ShouldUseHelperAngles( void ) const { return m_bSnapToHelperAngles; }
	float GetTargetRadius( void ) const { return m_flTargetRadius; }
	bool IsEnabled( void ) const { return m_bEnabled; }
	bool ForcePlacement( void ) const { return m_bForcePlacement; }
	bool HideUntilPlaced( void ) const { return m_bHideUntilPlaced; }
	float GetTargetSize( void ) const { return m_flTargetSize; }
	bool UsesSizeLimit( void ) const { return m_bUsesSizeLimit; }

	static CUtlVector< C_InfoPlacementHelper * > &Helpers();

private:
	float m_flTargetRadius;
	QAngle m_angTargetAngles;
	float m_flTargetSize;
	bool m_bEnabled;
	bool m_bSnapToHelperAngles;
	bool m_bForcePlacement;
	bool m_bHideUntilPlaced;
	bool m_bUsesSizeLimit;
};

C_InfoPlacementHelper *UTIL_FindPlacementHelper( const Vector &vPosition, CBasePlayer *pPlayer );
#else
class C_InfoPlacementHelper : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_InfoPlacementHelper, C_BaseEntity );

	C_InfoPlacementHelper( void ) {}

	const QAngle &GetTargetAngles( void ) const { return GetAbsAngles(); }
	bool ShouldUseHelperAngles( void ) const { return true; }
	float GetTargetRadius( void ) const { return 32.0f; }
};

inline C_InfoPlacementHelper *UTIL_FindPlacementHelper( const Vector &vPosition, CBasePlayer *pPlayer ) { return NULL; }
#endif

#endif // C_INFO_PLACEMENT_HELPER_H
