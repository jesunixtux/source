//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: Portal 2 player local data - server
//
//=============================================================================//

#include "cbase.h"
#include "portal_playerlocaldata.h"
#include "utlvector.h"

BEGIN_SEND_TABLE_NOBASE( CPortalPlayerLocalData, DT_PortalLocal )
	SendPropVector( SENDINFO_NOCHECK( m_vEyeOffset ), -1, SPROP_COORD ),
	SendPropBool( SENDINFO_NOCHECK( m_bSlowingTime ) ),
	SendPropBool( SENDINFO_NOCHECK( m_bShowingViewFinder ) ),
	SendPropBool( SENDINFO_NOCHECK( m_bZoomedIn ) ),
	SendPropFloat( SENDINFO_NOCHECK( m_flAirControlSupressionTime ), 32, SPROP_NOSCALE ),
	SendPropVector( SENDINFO_NOCHECK( m_vPreUpdateVelocity ), -1, SPROP_COORD ),
	SendPropEHandle( SENDINFO_NOCHECK( m_hTractorBeam ) ),
	SendPropInt( SENDINFO_NOCHECK( m_PaintedPowerType ), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

BEGIN_SIMPLE_DATADESC_( CPortalPlayerLocalData, CPlayerLocalData )
	DEFINE_FIELD( m_vEyeOffset, FIELD_VECTOR ),
	DEFINE_FIELD( m_bSlowingTime, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bShowingViewFinder, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bZoomedIn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flAirControlSupressionTime, FIELD_TIME ),
	DEFINE_FIELD( m_vPreUpdateVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( m_hTractorBeam, FIELD_EHANDLE ),
	DEFINE_FIELD( m_PaintedPowerType, FIELD_INTEGER ),
	DEFINE_FIELD( m_Up, FIELD_VECTOR ),
	DEFINE_FIELD( m_StickNormal, FIELD_VECTOR ),
	DEFINE_FIELD( m_OldStickNormal, FIELD_VECTOR ),
	DEFINE_FIELD( m_vLocalUp, FIELD_VECTOR ),
	DEFINE_FIELD( m_vStickRotationAxis, FIELD_VECTOR ),
	DEFINE_FIELD( m_InAirState, FIELD_INTEGER ),
	DEFINE_FIELD( m_nStickCameraState, FIELD_INTEGER ),
	DEFINE_FIELD( m_nTractorBeamCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_qQuaternionPunch, FIELD_QUATERNION ),
	DEFINE_FIELD( m_flAirInputScale, FIELD_FLOAT ),
	DEFINE_FIELD( m_bDuckedInAir, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPreventedCrouchJumpThisFrame, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bJumpedThisFrame, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bAttemptHullResize, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBouncedThisFrame, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDoneCorrectPitch, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDoneStickInterp, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fBouncedTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_StandHullMax, FIELD_VECTOR ),
	DEFINE_FIELD( m_StandHullMin, FIELD_VECTOR ),
	DEFINE_FIELD( m_DuckHullMax, FIELD_VECTOR ),
	DEFINE_FIELD( m_DuckHullMin, FIELD_VECTOR ),
	DEFINE_FIELD( m_CachedStandHullMaxAttempt, FIELD_VECTOR ),
	DEFINE_FIELD( m_CachedStandHullMinAttempt, FIELD_VECTOR ),
	DEFINE_FIELD( m_CachedDuckHullMaxAttempt, FIELD_VECTOR ),
	DEFINE_FIELD( m_CachedDuckHullMinAttempt, FIELD_VECTOR ),
END_DATADESC()
