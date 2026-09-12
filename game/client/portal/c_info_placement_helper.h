#ifndef C_INFO_PLACEMENT_HELPER_H
#define C_INFO_PLACEMENT_HELPER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"

class C_InfoPlacementHelper : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_InfoPlacementHelper, C_BaseEntity );

	C_InfoPlacementHelper( void ) {}

	// Stubs: the real class networks target angles and a use-helper-angles flag.
	const QAngle &GetTargetAngles( void ) const { return GetAbsAngles(); }
	bool ShouldUseHelperAngles( void ) const { return true; }
	float GetTargetRadius( void ) const { return 32.0f; }
};

inline C_InfoPlacementHelper *UTIL_FindPlacementHelper( const Vector &vPosition, CBasePlayer *pPlayer ) { return NULL; }

#endif // C_INFO_PLACEMENT_HELPER_H
