#include "cbase.h"
#include "info_placement_helper.h"
#include "debugoverlay_shared.h"

#include "tier0/memdbgon.h"

ConVar portal2_debug_placement_helpers(
	"portal2_debug_placement_helpers", "0", FCVAR_CHEAT,
	"Portal 2 placement helpers: 0=off, 1=selected, 2=all candidates." );

namespace
{
const float kPlacementHelperTieEpsilonSqr = 0.01f;

enum PlacementHelperResult_t
{
	PLACEMENT_HELPER_DISABLED,
	PLACEMENT_HELPER_INVALID_RADIUS,
	PLACEMENT_HELPER_OUTSIDE_RADIUS,
	PLACEMENT_HELPER_NOT_SELECTED,
	PLACEMENT_HELPER_SELECTED,
};

struct PlacementHelperCandidate_t
{
	CInfoPlacementHelper *pHelper;
	PlacementHelperResult_t result;
	float flDistanceSqr;
};

const char *PlacementHelperResultName( PlacementHelperResult_t result )
{
	switch ( result )
	{
	case PLACEMENT_HELPER_DISABLED: return "DISABLED";
	case PLACEMENT_HELPER_INVALID_RADIUS: return "INVALID_RADIUS";
	case PLACEMENT_HELPER_OUTSIDE_RADIUS: return "OUTSIDE_RADIUS";
	case PLACEMENT_HELPER_SELECTED: return "SELECTED";
	default: return "NOT_SELECTED";
	}
}

void DebugPlacementHelpers( const CUtlVector< PlacementHelperCandidate_t > &candidates,
	CInfoPlacementHelper *pSelected )
{
	const int nDebug = portal2_debug_placement_helpers.GetInt();
	if ( nDebug <= 0 )
		return;

	for ( int i = 0; i < candidates.Count(); ++i )
	{
		const PlacementHelperCandidate_t &candidate = candidates[i];
		if ( nDebug == 1 && candidate.pHelper != pSelected )
			continue;

		CInfoPlacementHelper *pHelper = candidate.pHelper;
		const bool bSelected = pHelper == pSelected;
		const int r = bSelected ? 0 : 255;
		const int g = bSelected ? 255 : 96;
		const float flRadius = MAX( pHelper->GetTargetRadius(), 1.0f );
		const Vector vecCenter = pHelper->GetAbsOrigin();
		Vector vecForward;
		AngleVectors( pHelper->GetTargetAngles(), &vecForward );

		NDebugOverlay::Sphere( vecCenter, flRadius, r, g, 0, true, 1.0f );
		NDebugOverlay::Line( vecCenter, vecCenter + vecForward * 32.0f, r, g, 0, true, 1.0f );

		char szText[256];
		Q_snprintf( szText, sizeof( szText ),
			"info_placement_helper #%d center %.1f %.1f %.1f radius %.1f enabled %d angles %.1f %.1f %.1f %s",
			pHelper->entindex(), XYZ( vecCenter ), pHelper->GetTargetRadius(),
			pHelper->IsEnabled() ? 1 : 0, XYZ( pHelper->GetTargetAngles() ),
			PlacementHelperResultName( candidate.result ) );
		NDebugOverlay::Text( vecCenter + Vector( 0, 0, 8 ), szText, false, 1.0f );
	}

	static float s_flNextMessageTime = 0.0f;
	if ( gpGlobals && gpGlobals->curtime >= s_flNextMessageTime )
	{
		if ( pSelected )
		{
			Msg( "Portal2PlacementHelper: selected #%d center %.1f %.1f %.1f radius %.1f\n",
				pSelected->entindex(), XYZ( pSelected->GetAbsOrigin() ), pSelected->GetTargetRadius() );
		}
		else
		{
			Msg( "Portal2PlacementHelper: selected none\n" );
		}
		s_flNextMessageTime = gpGlobals->curtime + 0.25f;
	}
}
}

LINK_ENTITY_TO_CLASS( info_placement_helper, CInfoPlacementHelper );

IMPLEMENT_SERVERCLASS_ST( CInfoPlacementHelper, DT_InfoPlacementHelper )
	SendPropFloat( SENDINFO( m_flTargetRadius ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flTargetSize ), 0, SPROP_NOSCALE ),
	SendPropBool( SENDINFO( m_bEnabled ) ),
	SendPropBool( SENDINFO( m_bSnapToHelperAngles ) ),
	SendPropBool( SENDINFO( m_bForcePlacement ) ),
	SendPropBool( SENDINFO( m_bHideUntilPlaced ) ),
	SendPropBool( SENDINFO( m_bUsesSizeLimit ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CInfoPlacementHelper )
	DEFINE_KEYFIELD( m_flTargetRadius, FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_flTargetSize, FIELD_FLOAT, "target_size" ),
	DEFINE_KEYFIELD( m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bSnapToHelperAngles, FIELD_BOOLEAN, "snap_to_helper_angles" ),
	DEFINE_KEYFIELD( m_bForcePlacement, FIELD_BOOLEAN, "force_placement" ),
	DEFINE_KEYFIELD( m_bHideUntilPlaced, FIELD_BOOLEAN, "hide_until_placed" ),
	DEFINE_KEYFIELD( m_bUsesSizeLimit, FIELD_BOOLEAN, "usesizelimit" ),
	DEFINE_KEYFIELD( m_iszProxyName, FIELD_STRING, "proxy_name" ),
	DEFINE_KEYFIELD( m_iszAttachTargetName, FIELD_STRING, "attach_target_name" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_OUTPUT( m_OnObjectPlaced, "OnObjectPlaced" ),
	DEFINE_OUTPUT( m_OnObjectPlacedSize, "OnObjectPlacedSize" ),
END_DATADESC()

CInfoPlacementHelper::CInfoPlacementHelper()
	: m_bStartDisabled( false ),
	  m_iszProxyName( NULL_STRING ),
	  m_iszAttachTargetName( NULL_STRING )
{
	m_flTargetRadius = 16.0f;
	m_flTargetSize = 0.0f;
	m_bEnabled = true;
	m_bSnapToHelperAngles = false;
	m_bForcePlacement = false;
	m_bHideUntilPlaced = false;
	m_bUsesSizeLimit = false;
}

void CInfoPlacementHelper::Spawn( void )
{
	BaseClass::Spawn();
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	m_bEnabled = !m_bStartDisabled;
}

int CInfoPlacementHelper::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

const QAngle &CInfoPlacementHelper::GetTargetAngles() const
{
	return GetAbsAngles();
}

void CInfoPlacementHelper::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
}

void CInfoPlacementHelper::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
}

CInfoPlacementHelper *UTIL_FindPlacementHelper( const Vector &vecPosition, CBasePlayer *pPlayer )
{
	(void)pPlayer;
	CInfoPlacementHelper *pBest = NULL;
	float flBestDistanceSqr = FLT_MAX;
	CUtlVector< PlacementHelperCandidate_t > candidates;

	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "info_placement_helper" ) ) != NULL )
	{
		CInfoPlacementHelper *pHelper = dynamic_cast< CInfoPlacementHelper * >( pEntity );
		if ( !pHelper )
			continue;

		PlacementHelperCandidate_t candidate;
		candidate.pHelper = pHelper;
		candidate.flDistanceSqr = vecPosition.DistToSqr( pHelper->GetAbsOrigin() );
		candidate.result = PLACEMENT_HELPER_NOT_SELECTED;

		const float flRadius = pHelper->GetTargetRadius();
		if ( !pHelper->IsEnabled() )
			candidate.result = PLACEMENT_HELPER_DISABLED;
		else if ( !( flRadius > 0.0f ) )
			candidate.result = PLACEMENT_HELPER_INVALID_RADIUS;
		else if ( candidate.flDistanceSqr > flRadius * flRadius )
			candidate.result = PLACEMENT_HELPER_OUTSIDE_RADIUS;
		else if ( !pBest || candidate.flDistanceSqr < flBestDistanceSqr - kPlacementHelperTieEpsilonSqr ||
			( fabsf( candidate.flDistanceSqr - flBestDistanceSqr ) <= kPlacementHelperTieEpsilonSqr &&
			  pHelper->entindex() < pBest->entindex() ) )
		{
			pBest = pHelper;
			flBestDistanceSqr = candidate.flDistanceSqr;
		}

		candidates.AddToTail( candidate );
	}

	for ( int i = 0; i < candidates.Count(); ++i )
	{
		if ( candidates[i].result == PLACEMENT_HELPER_NOT_SELECTED && candidates[i].pHelper == pBest )
			candidates[i].result = PLACEMENT_HELPER_SELECTED;
	}

	DebugPlacementHelpers( candidates, pBest );
	return pBest;
}
