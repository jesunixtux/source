//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: Portal 2 player local data - client
//
//=============================================================================//

#include "cbase.h"
#include "c_portal_playerlocaldata.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_RECV_TABLE_NOBASE( C_PortalPlayerLocalData, DT_PortalLocal )
	RecvPropVector( RECVINFO( m_vEyeOffset ) ),
	RecvPropBool( RECVINFO( m_bSlowingTime ) ),
	RecvPropBool( RECVINFO( m_bShowingViewFinder ) ),
	RecvPropBool( RECVINFO( m_bZoomedIn ) ),
	RecvPropFloat( RECVINFO( m_flAirControlSupressionTime ), SPROP_NOSCALE ),
	RecvPropVector( RECVINFO( m_vPreUpdateVelocity ) ),
	RecvPropEHandle( RECVINFO( m_hTractorBeam ) ),
	RecvPropInt( RECVINFO( m_PaintedPowerType ), SPROP_UNSIGNED ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Prediction block, mirrors CPortalPlayerLocalData
//			FTYPEDESC_INSENDTABLE only for the fields that are actually sent,
//			to avoid prediction mismatch error spam.
//-----------------------------------------------------------------------------
BEGIN_PREDICTION_DATA( C_PortalPlayerLocalData )

	DEFINE_PRED_FIELD( m_vEyeOffset, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bSlowingTime, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bShowingViewFinder, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bZoomedIn, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flAirControlSupressionTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_vPreUpdateVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hTractorBeam, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_PaintedPowerType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD( m_Up, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_StickNormal, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_OldStickNormal, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_vLocalUp, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_vStickRotationAxis, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_InAirState, FIELD_INTEGER, 0 ),
	DEFINE_PRED_FIELD( m_nStickCameraState, FIELD_INTEGER, 0 ),
	DEFINE_PRED_FIELD( m_nTractorBeamCount, FIELD_INTEGER, 0 ),
	DEFINE_PRED_FIELD( m_qQuaternionPunch, FIELD_QUATERNION, 0 ),
	DEFINE_PRED_FIELD( m_flAirInputScale, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_bDuckedInAir, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bPreventedCrouchJumpThisFrame, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bJumpedThisFrame, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bAttemptHullResize, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bBouncedThisFrame, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bDoneCorrectPitch, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_bDoneStickInterp, FIELD_BOOLEAN, 0 ),
	DEFINE_PRED_FIELD( m_fBouncedTime, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_StandHullMax, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_StandHullMin, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_DuckHullMax, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_DuckHullMin, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_CachedStandHullMaxAttempt, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_CachedStandHullMinAttempt, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_CachedDuckHullMaxAttempt, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_CachedDuckHullMinAttempt, FIELD_VECTOR, 0 ),
	DEFINE_PRED_FIELD( m_flSlowTimeMaximum, FIELD_FLOAT, 0 ),
	DEFINE_PRED_FIELD( m_flSlowTimeRemaining, FIELD_FLOAT, 0 ),

END_PREDICTION_DATA()
