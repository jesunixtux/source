//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Trigger tractor beam (server). Reconstructed surface for this
//          port; trigger_tractorbeam_shared.cpp is not registered, so only
//          the members consumed by compiled TUs are provided.
//
//=============================================================================//

#ifndef TRIGGER_TRACTORBEAM_H
#define TRIGGER_TRACTORBEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "util_shared.h"

abstract_class ITriggerTractorBeamAutoList
{
public:
	static const CUtlVector< ITriggerTractorBeamAutoList* >& AutoList( void )
	{
		static CUtlVector< ITriggerTractorBeamAutoList* > s_AutoList;
		return s_AutoList;
	}
};

class CTrigger_TractorBeam : public CBaseEntity, public ITriggerTractorBeamAutoList
{
	DECLARE_CLASS( CTrigger_TractorBeam, CBaseEntity );

public:
	virtual Vector GetForceDirection() const { return vec3_origin; }
	int GetLastUpdateFrame() const { return 0; }

	void ForceAttachEntity( CBaseEntity *pOther ) {}
	void UpdateBeam( const Vector& vStartPoint, const Vector& vEndPoint, float flLinearForce ) {}
	void SetDirection( const Vector &vStart, const Vector &vEnd ) {}
	void RemoveDeadBlobs() {}
	void RemoveChangedBeamBlobs() {}
	void RemoveAllBlobsFromBeam() {}
	float GetLinearLimit() { return 0.0f; }
	void EndTouch( CBaseEntity *pOther ) {}
};

#endif // TRIGGER_TRACTORBEAM_H