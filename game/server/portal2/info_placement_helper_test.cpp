#include "cbase.h"
#include "igamesystem.h"
#include "tier0/icommandline.h"
#include "info_placement_helper.h"

#include "tier0/memdbgon.h"

namespace
{
CInfoPlacementHelper *CreateTestPlacementHelper( const Vector &vecOrigin, float flRadius,
	const QAngle &angTarget, bool bSnapToAngles, bool bStartDisabled )
{
	CBaseEntity *pEntity = CreateEntityByName( "info_placement_helper" );
	if ( !pEntity )
		return NULL;

	char szRadius[32];
	Q_snprintf( szRadius, sizeof( szRadius ), "%g", flRadius );
	pEntity->KeyValue( "radius", szRadius );
	pEntity->KeyValue( "snap_to_helper_angles", bSnapToAngles ? "1" : "0" );
	pEntity->KeyValue( "StartDisabled", bStartDisabled ? "1" : "0" );
	pEntity->SetAbsOrigin( vecOrigin );
	pEntity->SetAbsAngles( angTarget );
	DispatchSpawn( pEntity );
	return dynamic_cast< CInfoPlacementHelper * >( pEntity );
}

bool AnglesEqual( const QAngle &a, const QAngle &b )
{
	return fabsf( a.x - b.x ) < 0.01f && fabsf( a.y - b.y ) < 0.01f &&
		fabsf( a.z - b.z ) < 0.01f;
}

void PrintPlacementHelperResult( const char *pName, bool bPassed )
{
	Msg( "PORTAL2_PLACEMENT_HELPER %s %s\n", pName, bPassed ? "PASS" : "FAIL" );
}

void RunPlacementHelperTests()
{
	const Vector vecBase( 100000.0f, 100000.0f, 100000.0f );
	CUtlVector< CInfoPlacementHelper * > helpers;

	PrintPlacementHelperResult( "absent", UTIL_FindPlacementHelper( vecBase, NULL ) == NULL );

	const QAngle angTarget( 10.0f, 20.0f, 30.0f );
	CInfoPlacementHelper *pPrimary = CreateTestPlacementHelper( vecBase, 64.0f, angTarget, true, false );
	if ( pPrimary )
		helpers.AddToTail( pPrimary );
	PrintPlacementHelperResult( "inside", pPrimary &&
		UTIL_FindPlacementHelper( vecBase + Vector( 10, 0, 0 ), NULL ) == pPrimary );
	PrintPlacementHelperResult( "outside", pPrimary &&
		UTIL_FindPlacementHelper( vecBase + Vector( 128, 0, 0 ), NULL ) == NULL );
	PrintPlacementHelperResult( "orientation", pPrimary && pPrimary->ShouldUseHelperAngles() &&
		AnglesEqual( pPrimary->GetTargetAngles(), angTarget ) );

	variant_t emptyValue;
	if ( pPrimary )
		pPrimary->AcceptInput( "Disable", NULL, NULL, emptyValue, 0 );
	PrintPlacementHelperResult( "disabled", pPrimary && !pPrimary->IsEnabled() &&
		UTIL_FindPlacementHelper( vecBase, NULL ) == NULL );

	CInfoPlacementHelper *pInvalid = CreateTestPlacementHelper(
		vecBase + Vector( 500, 0, 0 ), 0.0f, vec3_angle, false, false );
	if ( pInvalid )
		helpers.AddToTail( pInvalid );
	PrintPlacementHelperResult( "invalid_radius", pInvalid &&
		UTIL_FindPlacementHelper( pInvalid->GetAbsOrigin(), NULL ) == NULL );

	const Vector vecTieCenter = vecBase + Vector( 1000, 0, 0 );
	CInfoPlacementHelper *pFirst = CreateTestPlacementHelper(
		vecTieCenter + Vector( -10, 0, 0 ), 64.0f, vec3_angle, false, false );
	CInfoPlacementHelper *pSecond = CreateTestPlacementHelper(
		vecTieCenter + Vector( 10, 0, 0 ), 64.0f, vec3_angle, false, false );
	if ( pFirst ) helpers.AddToTail( pFirst );
	if ( pSecond ) helpers.AddToTail( pSecond );
	CInfoPlacementHelper *pTieWinner = UTIL_FindPlacementHelper( vecTieCenter, NULL );
	const bool bDeterministic = pFirst && pSecond && pFirst->entindex() < pSecond->entindex() &&
		pTieWinner == pFirst && UTIL_FindPlacementHelper( vecTieCenter, NULL ) == pFirst;
	PrintPlacementHelperResult( "deterministic", bDeterministic );
	PrintPlacementHelperResult( "tie_break", bDeterministic );

	for ( int i = 0; i < helpers.Count(); ++i )
		UTIL_Remove( helpers[i] );
}

class CPortal2PlacementHelperTestSystem : public CAutoGameSystem
{
public:
	CPortal2PlacementHelperTestSystem()
		: CAutoGameSystem( "Portal2PlacementHelperTestSystem" ), m_bRan( false )
	{
	}

	void LevelInitPostEntity() override
	{
		if ( !m_bRan && CommandLine()->FindParm( "-portal2_placement_helper_test" ) )
		{
			m_bRan = true;
			RunPlacementHelperTests();
		}
	}

private:
	bool m_bRan;
};

CPortal2PlacementHelperTestSystem g_Portal2PlacementHelperTestSystem;
}
