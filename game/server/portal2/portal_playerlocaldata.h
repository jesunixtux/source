//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: Portal 2 player local data - server side definition of the local
//			player data structure that is sent only to the owning player.
//
//=============================================================================//

#ifndef PORTAL_PLAYERLOCALDATA_H
#define PORTAL_PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "playerlocaldata.h"
#include "networkvar.h"
#include "mathlib/vector.h"
#include "simtimer.h"
#include "util_shared.h"
#include "portal_player_shared.h"
#include "paint_enum.h"

class CTrigger_TractorBeam;

//-----------------------------------------------------------------------------
// Purpose: Player specific data ( sent only to local player, too )
//-----------------------------------------------------------------------------
class CPortalPlayerLocalData : public CPlayerLocalData
{
public:
	DECLARE_CLASS( CPortalPlayerLocalData, CPlayerLocalData );
	DECLARE_SIMPLE_DATADESC();
	DECLARE_EMBEDDED_NETWORKVAR();

	Vector					m_vEyeOffset;				// Eye offset when we are in the world (the difference between the base eye offset and the eye is a function of ducking, etc.)
	bool					m_bSlowingTime;				// Whether the player is in the process of slowing down time
	bool					m_bShowingViewFinder;		// Whether the player is in the viewfinder (looking through a camera)
	bool					m_bZoomedIn;				// Whether the player is zoomed in (in a camera or telescope)
	float					m_flAirControlSupressionTime;	// How much longer air control is suppressed
	Vector					m_vPreUpdateVelocity;		// Velocity before the update
	CHandle<CTrigger_TractorBeam> m_hTractorBeam;		// The tractor beam we're affected by
	PaintPowerType			m_PaintedPowerType;			// The type of paint we're currently affected by
	CountdownTimer			m_PaintedPowerTimer;		// How long we've been affected by paint

	Vector					m_Up;						// "Up" for the player - affected by portals and camera modes
	Vector					m_StickNormal;				// Surface normal of what we're stuck to
	Vector					m_OldStickNormal;			// Surface normal we stuck to before the current one
	Vector					m_vLocalUp;					// Up locally (absolute world up)
	Vector					m_vStickRotationAxis;		// Rotation axis for the current stick
	InAirState				m_InAirState;				// Current state of being in the air
	StickCameraState		m_nStickCameraState;		// Current stick camera state
	int						m_nTractorBeamCount;		// How many tractor beams we're currently in
	Quaternion				m_qQuaternionPunch;			// View punch applied as a quaternion
	float					m_flAirInputScale;			// Scaling of air movement input
	bool					m_bDuckedInAir;				// Whether we're ducking while in the air
	bool					m_bPreventedCrouchJumpThisFrame;	// Whether a jump while ducked was prevented this frame
	bool					m_bJumpedThisFrame;			// Whether we jumped this frame
	bool					m_bAttemptHullResize;		// Whether we're attempting to resize the player hull
	bool					m_bBouncedThisFrame;		// Whether we bounced off something this frame
	bool					m_bDoneCorrectPitch;		// Whether the correct pitch was applied this frame
	bool					m_bDoneStickInterp;			// Whether stick interpolation was done this frame
	float					m_fBouncedTime;				// When the player last bounced
	Vector					m_StandHullMax;				// Current stand hull max
	Vector					m_StandHullMin;				// Current stand hull min
	Vector					m_DuckHullMax;				// Current duck hull max
	Vector					m_DuckHullMin;				// Current duck hull min
	Vector					m_CachedStandHullMaxAttempt;	// Cached stand hull max for the current attempt
	Vector					m_CachedStandHullMinAttempt;	// Cached stand hull min for the current attempt
	Vector					m_CachedDuckHullMaxAttempt;		// Cached duck hull max for the current attempt
	Vector					m_CachedDuckHullMinAttempt;		// Cached duck hull min for the current attempt
	CachedPaintPowerChoiceResultArray m_CachedPaintPowerChoiceResults;	// Cached results for the paint power choice

	float					m_flSlowTimeMaximum;		// Maximum time we can slow down time
	float					m_flSlowTimeRemaining;		// How much slow time is left
};

EXTERN_SEND_TABLE(DT_PortalLocal);

#endif // PORTAL_PLAYERLOCALDATA_H