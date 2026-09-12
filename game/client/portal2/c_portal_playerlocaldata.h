//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: Portal 2 player local data - client side definition of the local
//			player data structure (only visible to the owning player).
//
//=============================================================================//

#ifndef C_PORTAL_PLAYERLOCALDATA_H
#define C_PORTAL_PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "c_playerlocaldata.h"
#include "networkvar.h"
#include "mathlib/vector.h"
#include "simtimer.h"
#include "util_shared.h"
#include "portal_player_shared.h"
#include "paint_enum.h"
#include "predictable_entity.h"

class C_Trigger_TractorBeam;

//-----------------------------------------------------------------------------
// Purpose: Player specific data (sent only to local player, too)
//-----------------------------------------------------------------------------
class C_PortalPlayerLocalData : public CPlayerLocalData
{
public:
	DECLARE_CLASS( C_PortalPlayerLocalData, CPlayerLocalData );
	DECLARE_PREDICTABLE();

	Vector					m_vEyeOffset;
	bool					m_bSlowingTime;
	bool					m_bShowingViewFinder;
	bool					m_bZoomedIn;
	float					m_flAirControlSupressionTime;
	Vector					m_vPreUpdateVelocity;
	CHandle<C_Trigger_TractorBeam> m_hTractorBeam;
	PaintPowerType			m_PaintedPowerType;
	CountdownTimer			m_PaintedPowerTimer;

	Vector					m_Up;
	Vector					m_StickNormal;
	Vector					m_OldStickNormal;
	Vector					m_vLocalUp;
	Vector					m_vStickRotationAxis;
	InAirState				m_InAirState;
	StickCameraState		m_nStickCameraState;
	int						m_nTractorBeamCount;
	Quaternion				m_qQuaternionPunch;
	float					m_flAirInputScale;
	bool					m_bDuckedInAir;
	bool					m_bPreventedCrouchJumpThisFrame;
	bool					m_bJumpedThisFrame;
	bool					m_bAttemptHullResize;
	bool					m_bBouncedThisFrame;
	bool					m_bDoneCorrectPitch;
	bool					m_bDoneStickInterp;
	float					m_fBouncedTime;
	Vector					m_StandHullMax;
	Vector					m_StandHullMin;
	Vector					m_DuckHullMax;
	Vector					m_DuckHullMin;
	Vector					m_CachedStandHullMaxAttempt;
	Vector					m_CachedStandHullMinAttempt;
	Vector					m_CachedDuckHullMaxAttempt;
	Vector					m_CachedDuckHullMinAttempt;
	CachedPaintPowerChoiceResultArray m_CachedPaintPowerChoiceResults;

	float					m_flSlowTimeMaximum;
	float					m_flSlowTimeRemaining;

	inline C_PortalPlayerLocalData()
	{
		// non-networked
	}
};

EXTERN_RECV_TABLE(DT_PortalLocal);

#endif // C_PORTAL_PLAYERLOCALDATA_H