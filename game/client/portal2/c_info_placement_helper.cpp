#include "cbase.h"
#include "c_info_placement_helper.h"
#include "cliententitylist.h"
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
	C_InfoPlacementHelper *pHelper;
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
	C_InfoPlacementHelper *pSelected )
{
	const int nDebug = portal2_debug_placement_helpers.GetInt();
	if ( nDebug <= 0 )
		return;

	for ( int i = 0; i < candidates.Count(); ++i )
	{
		const PlacementHelperCandidate_t &candidate = candidates[i];
		if ( nDebug == 1 && candidate.pHelper != pSelected )
			continue;

		C_InfoPlacementHelper *pHelper = candidate.pHelper;
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
			Msg( "Portal2PlacementHelper(client): selected #%d center %.1f %.1f %.1f radius %.1f\n",
				pSelected->entindex(), XYZ( pSelected->GetAbsOrigin() ), pSelected->GetTargetRadius() );
		}
		else
		{
			Msg( "Portal2PlacementHelper(client): selected none\n" );
		}
		s_flNextMessageTime = gpGlobals->curtime + 0.25f;
	}
}
}

IMPLEMENT_CLIENTCLASS_DT( C_InfoPlacementHelper, DT_InfoPlacementHelper, CInfoPlacementHelper )
	RecvPropFloat( RECVINFO( m_flTargetRadius ) ),
	RecvPropFloat( RECVINFO( m_flTargetSize ) ),
	RecvPropBool( RECVINFO( m_bEnabled ) ),
	RecvPropBool( RECVINFO( m_bSnapToHelperAngles ) ),
	RecvPropBool( RECVINFO( m_bForcePlacement ) ),
	RecvPropBool( RECVINFO( m_bHideUntilPlaced ) ),
	RecvPropBool( RECVINFO( m_bUsesSizeLimit ) ),
END_RECV_TABLE()

C_InfoPlacementHelper::C_InfoPlacementHelper()
	: m_flTargetRadius( 16.0f ),
	  m_flTargetSize( 0.0f ),
	  m_bEnabled( true ),
	  m_bSnapToHelperAngles( false ),
	  m_bForcePlacement( false ),
	  m_bHideUntilPlaced( false ),
	  m_bUsesSizeLimit( false )
{
}

const QAngle &C_InfoPlacementHelper::GetTargetAngles( void ) const
{
	return GetAbsAngles();
}

C_InfoPlacementHelper *UTIL_FindPlacementHelper( const Vector &vPosition, CBasePlayer *pPlayer )
{
	(void)pPlayer;
	C_InfoPlacementHelper *pBest = NULL;
	float flBestDistanceSqr = FLT_MAX;
	CUtlVector< PlacementHelperCandidate_t > candidates;

	const int nHighestEntityIndex = ClientEntityList().GetHighestEntityIndex();
	for ( int i = 0; i <= nHighestEntityIndex; ++i )
	{
		C_InfoPlacementHelper *pHelper = dynamic_cast< C_InfoPlacementHelper * >(
			ClientEntityList().GetBaseEntity( i ) );
		if ( !pHelper )
			continue;

		PlacementHelperCandidate_t candidate;
		candidate.pHelper = pHelper;
		candidate.flDistanceSqr = vPosition.DistToSqr( pHelper->GetAbsOrigin() );
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
