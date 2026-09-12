//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Trigger tractor beam (client). Reconstructed surface for this
//          port; mirror of trigger_tractorbeam.h with C_ naming.
//
//=============================================================================//

#ifndef C_TRIGGER_TRACTORBEAM_H
#define C_TRIGGER_TRACTORBEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
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

class C_Trigger_TractorBeam : public C_BaseEntity, public ITriggerTractorBeamAutoList
{
	DECLARE_CLASS( C_Trigger_TractorBeam, C_BaseEntity );

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

	// Portal 2: compute linear/angular force applied to an entity this frame.
	// Stub implementation leaves outputs zero.
	virtual void CalculateFrameMovement( CBaseEntity *pEntity, CBaseEntity *pPlayer, float flDeltaTime, Vector &vLinear, AngularImpulse &angAngular ) { vLinear.Init(); angAngular.Init(); }
};

#define CTrigger_TractorBeam C_Trigger_TractorBeam

#endif // C_TRIGGER_TRACTORBEAM_H
