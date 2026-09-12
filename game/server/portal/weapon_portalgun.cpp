//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

static const char *s_pDelayedPlacementContext = "PortalDelayedPlacement";

#include "BasePropDoor.h"
#include "portal_player.h"
#include "te_effect_dispatch.h"
#include "gameinterface.h"
#include "prop_combine_ball.h"
#include "portal_shareddefs.h"
#include "triggers.h"
#include "collisionutils.h"
#include "cbaseanimatingprojectile.h"
#include "weapon_physcannon.h"
#include "prop_portal_shared.h"
#include "portal_placement.h"
#include "weapon_portalgun_shared.h"
#include "physicsshadowclone.h"
#include "particle_parse.h"


#define BLAST_SPEED_NON_PLAYER 1000.0f
#define BLAST_SPEED 3000.0f

// Portal 2 caps how far a portal can be placed. The stock Portal 1 code traces
// to MAX_TRACE_LENGTH (~56756 units), which lets a gun place portals across the
// whole map. Clamp the placement trace so the gun behaves like its P2 counterpart.
ConVar sv_portal_placement_max_range( "sv_portal_placement_max_range", "5000", 0,
	"Maximum distance (in units) at which the portal gun can place a portal." );

static float clamp_range( float fMax )
{
	return MIN( fMax, sv_portal_placement_max_range.GetFloat() );
}


IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPortalgun, DT_WeaponPortalgun )

BEGIN_NETWORK_TABLE( CWeaponPortalgun, DT_WeaponPortalgun )
	SendPropBool( SENDINFO( m_bCanFirePortal1 ) ),
	SendPropBool( SENDINFO( m_bCanFirePortal2 ) ),
	SendPropInt( SENDINFO( m_iLastFiredPortal ) ),
	SendPropBool( SENDINFO( m_bOpenProngs ) ),
	SendPropFloat( SENDINFO( m_fCanPlacePortal1OnThisSurface ) ),
	SendPropFloat( SENDINFO( m_fCanPlacePortal2OnThisSurface ) ),
	SendPropFloat( SENDINFO( m_fEffectsMaxSize1 ) ), // HACK HACK! Used to make the gun visually change when going through a cleanser!
	SendPropFloat( SENDINFO( m_fEffectsMaxSize2 ) ),
	SendPropInt( SENDINFO( m_EffectState ) ),
END_NETWORK_TABLE()

BEGIN_DATADESC( CWeaponPortalgun )

	DEFINE_KEYFIELD( m_bCanFirePortal1, FIELD_BOOLEAN, "CanFirePortal1" ),
	DEFINE_KEYFIELD( m_bCanFirePortal2, FIELD_BOOLEAN, "CanFirePortal2" ),
	DEFINE_FIELD( m_iLastFiredPortal, FIELD_INTEGER ),
	DEFINE_FIELD( m_bOpenProngs, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fCanPlacePortal1OnThisSurface, FIELD_FLOAT ),
	DEFINE_FIELD( m_fCanPlacePortal2OnThisSurface, FIELD_FLOAT ),
	DEFINE_FIELD( m_fEffectsMaxSize1, FIELD_FLOAT ),
	DEFINE_FIELD( m_fEffectsMaxSize2, FIELD_FLOAT ),
	DEFINE_FIELD( m_EffectState, FIELD_INTEGER ),
	DEFINE_FIELD( m_iPortalLinkageGroupID, FIELD_CHARACTER	),

	DEFINE_INPUTFUNC( FIELD_VOID, "ChargePortal1", InputChargePortal1 ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ChargePortal2", InputChargePortal2 ),
	DEFINE_INPUTFUNC( FIELD_VOID, "FirePortal1", FirePortal1 ),
	DEFINE_INPUTFUNC( FIELD_VOID, "FirePortal2", FirePortal2 ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "FirePortalDirection1", FirePortalDirection1 ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "FirePortalDirection2", FirePortalDirection2 ),

	DEFINE_SOUNDPATCH( m_pMiniGravHoldSound ),

	DEFINE_OUTPUT ( m_OnFiredPortal1, "OnFiredPortal1" ),
	DEFINE_OUTPUT ( m_OnFiredPortal2, "OnFiredPortal2" ),

	DEFINE_FUNCTION( Think ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( weapon_portalgun, CWeaponPortalgun );
PRECACHE_WEAPON_REGISTER(weapon_portalgun);


extern ConVar sv_portal_placement_debug;
extern ConVar sv_portal_placement_never_fail;

#ifdef PORTAL2
static ConVar portal2_portalgun_debug( "portal2_portalgun_debug", "0", FCVAR_CHEAT,
	"Report actual portal-gun traces and placement results in the experimental port." );

// The opening hotel map deliberately starts without a weapon; the dual gun is
// granted through impulse 101 and the F6/experiment binds. This flag is the
// single cross-map persistence this experiment honours: the engine's plain
// changelevel restores no player state, so post-transition arrival re-equips
// The intro chain hands out the single-portal (blue-only) gun: the continuity
// flag carries that blue gun across map switches by re-equipping it at arrival.
// It is not archived; it only lives for the current engine session. Default is
// on so a fresh session with the experimental build keeps the gun across maps;
// set it to 0 to disable the carry-over.
ConVar portal2_resume_portalgun( "portal2_resume_portalgun", "1", 0,
	"Portal 2 experiment: carry the intro blue-only portal gun across intro map arrivals." );
#endif


void CWeaponPortalgun::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	SetThink( &CWeaponPortalgun::Think );
	SetNextThink( gpGlobals->curtime + 0.1 );

	if( GameRules()->IsMultiplayer() )
	{
		CBaseEntity *pOwner = GetOwner();
		if( pOwner && pOwner->IsPlayer() )
			m_iPortalLinkageGroupID = pOwner->entindex();

		Assert( (m_iPortalLinkageGroupID >= 0) && (m_iPortalLinkageGroupID < 256) );
	}	
}

void CWeaponPortalgun::Activate()
{
	BaseClass::Activate();

	CreateSounds();

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer )
	{
		CBaseEntity *pHeldObject = GetPlayerHeldEntity( pPlayer );
		OpenProngs( ( pHeldObject ) ? ( false ) : ( true ) );
		OpenProngs( ( pHeldObject ) ? ( true ) : ( false ) );

		if( GameRules()->IsMultiplayer() )
			m_iPortalLinkageGroupID = pPlayer->entindex();

		Assert( (m_iPortalLinkageGroupID >= 0) && (m_iPortalLinkageGroupID < 256) );
	}

	// HACK HACK! Used to make the gun visually change when going through a cleanser!
	m_fEffectsMaxSize1 = 4.0f;
	m_fEffectsMaxSize2 = 4.0f;
}

void CWeaponPortalgun::OnPickedUp( CBaseCombatCharacter *pNewOwner )
{
	if( GameRules()->IsMultiplayer() )
	{
		if( pNewOwner && pNewOwner->IsPlayer() )
			m_iPortalLinkageGroupID = pNewOwner->entindex();

		Assert( (m_iPortalLinkageGroupID >= 0) && (m_iPortalLinkageGroupID < 256) );
	}

	BaseClass::OnPickedUp( pNewOwner );

#ifdef PORTAL2
	// The intro chain hands over the single-portal (blue-only) gun. A dropped
	// gun that is picked back up must restore a usable blue setup regardless of
	// the restrictive keyfields maps keep on their placed portal guns.
	// SetCanFirePortal1 already plays the pickup sound once.
	if ( portal2_resume_portalgun.GetBool() )
	{
		SetCanFirePortal1();
		return;
	}
#endif

	PlayPickupSound();
}

void CWeaponPortalgun::CreateSounds()
{
	if (!m_pMiniGravHoldSound)
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		CPASAttenuationFilter filter( this );

		m_pMiniGravHoldSound = controller.SoundCreate( filter, entindex(), "Weapon_Portalgun.HoldSound" );
		controller.Play( m_pMiniGravHoldSound, 0, 100 );
	}
}

void CWeaponPortalgun::StopLoopingSounds()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	controller.SoundDestroy( m_pMiniGravHoldSound );
	m_pMiniGravHoldSound = NULL;

	BaseClass::StopLoopingSounds();
}

void CWeaponPortalgun::DoEffectBlast( bool bPortal2, int iPlacedBy, const Vector &ptStart, const Vector &ptFinalPos, const QAngle &qStartAngles, float fDelay )
{
	CEffectData	fxData;
	fxData.m_vOrigin = ptStart;
	fxData.m_vStart = ptFinalPos;
	fxData.m_flScale = gpGlobals->curtime + fDelay;
	fxData.m_vAngles = qStartAngles;
	fxData.m_nColor = ( ( bPortal2 ) ? ( 2 ) : ( 1 ) );
	fxData.m_nDamageType = iPlacedBy;
	DispatchEffect( "PortalBlast", fxData );
}

//-----------------------------------------------------------------------------
// Purpose: Allows a generic think function before the others are called
// Input  : state - which state the turret is currently in
//-----------------------------------------------------------------------------
bool CWeaponPortalgun::PreThink( void )
{
	//Animate
	StudioFrameAdvance();

	//Do not interrupt current think function
	return false;
}

void CWeaponPortalgun::Think( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink() )
		return;

	SetNextThink( gpGlobals->curtime + 0.1f );

	CPortal_Player *pPlayer = ToPortalPlayer( GetOwner() );

	if ( !pPlayer || pPlayer->GetActiveWeapon() != this )
	{
		m_fCanPlacePortal1OnThisSurface = 1.0f;
		m_fCanPlacePortal2OnThisSurface = 1.0f;
		return;
	}

	// Test portal placement
	m_fCanPlacePortal1OnThisSurface = ( ( m_bCanFirePortal1 ) ? ( FirePortal( false, 0, 1 ) ) : ( 0.0f ) );
	m_fCanPlacePortal2OnThisSurface = ( ( m_bCanFirePortal2 ) ? ( FirePortal( true, 0, 2 ) ) : ( 0.0f ) );

	// Draw obtained portal color chips
	int iSlot1State = ( ( m_bCanFirePortal1 ) ? ( 0 ) : ( 1 ) ); // FIXME: Portal gun might have only red but not blue;
	int iSlot2State = ( ( m_bCanFirePortal2 ) ? ( 0 ) : ( 1 ) );

#ifdef PORTAL2
	// Studio 49's group 1 is PotatOS, not a portal-color chip; group 2
	// does not exist. Leave those bodygroups under the map's control.
	if ( GetModelPtr() && GetModelPtr()->GetRenderHdr()->version < 49 )
#endif
	{
		SetBodygroup( 1, iSlot1State );
		SetBodygroup( 2, iSlot2State );
	}

	if ( pPlayer->GetViewModel() )
	{
#ifdef PORTAL2
		CStudioHdr *pViewHdr = pPlayer->GetViewModel()->GetModelPtr();
		if ( pViewHdr && pViewHdr->GetRenderHdr()->version < 49 )
#endif
		{
			pPlayer->GetViewModel()->SetBodygroup( 1, iSlot1State );
			pPlayer->GetViewModel()->SetBodygroup( 2, iSlot2State );
		}
	}

	// HACK HACK! Used to make the gun visually change when going through a cleanser!
	if ( m_fEffectsMaxSize1 > 4.0f )
	{
		m_fEffectsMaxSize1 -= gpGlobals->frametime * 400.0f;
		if ( m_fEffectsMaxSize1 < 4.0f )
			m_fEffectsMaxSize1 = 4.0f;
	}

	if ( m_fEffectsMaxSize2 > 4.0f )
	{
		m_fEffectsMaxSize2 -= gpGlobals->frametime * 400.0f;
		if ( m_fEffectsMaxSize2 < 4.0f )
			m_fEffectsMaxSize2 = 4.0f;
	}
}

void CWeaponPortalgun::OpenProngs( bool bOpenProngs )
{
	if ( m_bOpenProngs == bOpenProngs )
	{
		return;
	}

	m_bOpenProngs = bOpenProngs;

	DoEffect( ( m_bOpenProngs ) ? ( EFFECT_HOLDING ) : ( EFFECT_READY ) );

	SendWeaponAnim( ( m_bOpenProngs ) ? ( ACT_VM_PICKUP ) : ( ACT_VM_RELEASE ) );
}

void CWeaponPortalgun::InputChargePortal1( inputdata_t &inputdata )
{
	DispatchParticleEffect( "portal_1_charge", PATTACH_POINT_FOLLOW, this, "muzzle" );
}

void CWeaponPortalgun::InputChargePortal2( inputdata_t &inputdata )
{
	DispatchParticleEffect( "portal_2_charge", PATTACH_POINT_FOLLOW, this, "muzzle" );
}

void CWeaponPortalgun::FirePortal1( inputdata_t &inputdata )
{
	FirePortal( false );
	m_iLastFiredPortal = 1;

	CBaseCombatCharacter *pOwner = GetOwner();

	if( pOwner && pOwner->IsPlayer() )
	{
		WeaponSound( SINGLE );
	}
	else
	{
		WeaponSound( SINGLE_NPC );
	}
}

void CWeaponPortalgun::FirePortal2( inputdata_t &inputdata )
{
	FirePortal( true );
	m_iLastFiredPortal = 2;

	CBaseCombatCharacter *pOwner = GetOwner();

	if( pOwner && pOwner->IsPlayer() )
	{
		WeaponSound( WPN_DOUBLE );
	}
	else
	{
		WeaponSound( DOUBLE_NPC );
	}
}

void CWeaponPortalgun::FirePortalDirection1( inputdata_t &inputdata )
{
	Vector vDirection;
	inputdata.value.Vector3D( vDirection );
	FirePortal( false, &vDirection );
	m_iLastFiredPortal = 1;
	
	CBaseCombatCharacter *pOwner = GetOwner();

	if( pOwner && pOwner->IsPlayer() )
	{
		WeaponSound( SINGLE );
	}
	else
	{
		WeaponSound( SINGLE_NPC );
	}
}

void CWeaponPortalgun::FirePortalDirection2( inputdata_t &inputdata )
{
	Vector vDirection;
	inputdata.value.Vector3D( vDirection );
	FirePortal( true, &vDirection );
	m_iLastFiredPortal = 2;
	
	CBaseCombatCharacter *pOwner = GetOwner();

	if( pOwner && pOwner->IsPlayer() )
	{
		WeaponSound( WPN_DOUBLE );
	}
	else
	{
		WeaponSound( DOUBLE_NPC );
	}
}

float CWeaponPortalgun::TraceFirePortal( bool bPortal2, const Vector &vTraceStart, const Vector &vDirection, trace_t &tr, Vector &vFinalPosition, QAngle &qFinalAngles, int iPlacedBy, bool bTest /*= false*/ )
{
	CTraceFilterSimpleClassnameList baseFilter( this, COLLISION_GROUP_NONE );
	UTIL_Portal_Trace_Filter( &baseFilter );
	CTraceFilterTranslateClones traceFilterPortalShot( &baseFilter );

	Ray_t rayEyeArea;
	rayEyeArea.Init( vTraceStart + vDirection * 24.0f, vTraceStart + vDirection * -24.0f );

	float fMustBeCloserThan = 2.0f;

	CProp_Portal *pNearPortal = static_cast<CProp_Portal *>( UTIL_Portal_FirstAlongRay( rayEyeArea, fMustBeCloserThan ) );

	if ( !pNearPortal )
	{
		// Check for portal near and infront of you
		rayEyeArea.Init( vTraceStart + vDirection * -24.0f, vTraceStart + vDirection * 48.0f );

		fMustBeCloserThan = 2.0f;

		pNearPortal = static_cast<CProp_Portal *>( UTIL_Portal_FirstAlongRay( rayEyeArea, fMustBeCloserThan ) );
	}

	if ( pNearPortal && pNearPortal->IsActivedAndLinked() )
	{
		iPlacedBy = PORTAL_PLACED_BY_PEDESTAL;

		Vector vPortalForward;
		pNearPortal->GetVectors( &vPortalForward, 0, 0 );

		if ( vDirection.Dot( vPortalForward ) < 0.01f )
		{
			// If shooting out of the world, fizzle
			if ( !bTest )
			{
				CProp_Portal *pPortal = CProp_Portal::FindPortal( m_iPortalLinkageGroupID, bPortal2, true );

				pPortal->m_iDelayedFailure = ( ( pNearPortal->m_bIsPortal2 ) ? ( PORTAL_FIZZLE_NEAR_RED ) : ( PORTAL_FIZZLE_NEAR_BLUE ) );
				VectorAngles( vPortalForward, pPortal->m_qDelayedAngles );
				pPortal->m_vDelayedPosition = pNearPortal->GetAbsOrigin();

				vFinalPosition = pPortal->m_vDelayedPosition;
				qFinalAngles = pPortal->m_qDelayedAngles;

				UTIL_TraceLine( vTraceStart - vDirection * 16.0f, vTraceStart + (vDirection * clamp_range( m_fMaxRange1 )), MASK_SHOT_PORTAL, &traceFilterPortalShot, &tr );

				return PORTAL_ANALOG_SUCCESS_NEAR;
			}

			UTIL_TraceLine( vTraceStart - vDirection * 16.0f, vTraceStart + (vDirection * clamp_range( m_fMaxRange1 )), MASK_SHOT_PORTAL, &traceFilterPortalShot, &tr );

			return PORTAL_ANALOG_SUCCESS_OVERLAP_LINKED;
		}
	}

	// Trace to see where the portal hit
	UTIL_TraceLine( vTraceStart, vTraceStart + (vDirection * clamp_range( m_fMaxRange1 )), MASK_SHOT_PORTAL, &traceFilterPortalShot, &tr );

	if ( !tr.DidHit() || tr.startsolid )
	{
		// If it didn't hit anything, fizzle
		if ( !bTest )
		{
			CProp_Portal *pPortal = CProp_Portal::FindPortal( m_iPortalLinkageGroupID, bPortal2, true );

			pPortal->m_iDelayedFailure = PORTAL_FIZZLE_NONE;
			VectorAngles( -vDirection, pPortal->m_qDelayedAngles );
			pPortal->m_vDelayedPosition = tr.endpos;

			vFinalPosition = pPortal->m_vDelayedPosition;
			qFinalAngles = pPortal->m_qDelayedAngles;
		}

		return PORTAL_ANALOG_SUCCESS_PASSTHROUGH_SURFACE;
	}

	// Trace to the surface to see if there's a rotating door in the way
	CBaseEntity *list[1024];

	Ray_t ray;
	ray.Init( vTraceStart, tr.endpos );

	int nCount = UTIL_EntitiesAlongRay( list, 1024, ray, 0 );

	// Loop through all entities along the ray between the gun and the surface
	for ( int i = 0; i < nCount; i++ )
	{
		// If the entity is a rotating door
		if( FClassnameIs( list[i], "prop_door_rotating" ) )
		{
			// Check more precise door collision
			CBasePropDoor *pRotatingDoor = static_cast<CBasePropDoor *>( list[i] );

			Ray_t rayDoor;
			rayDoor.Init( vTraceStart, vTraceStart + (vDirection * clamp_range( m_fMaxRange1 )) );

			trace_t trDoor;
			pRotatingDoor->TestCollision( rayDoor, 0, trDoor );

			if ( trDoor.DidHit() )
			{
				// There's a door in the way
				tr = trDoor;

				if ( sv_portal_placement_debug.GetBool() )
				{
					Vector vMin;
					Vector vMax;
					Vector vZero = Vector( 0.0f, 0.0f, 0.0f );
					list[ i ]->GetCollideable()->WorldSpaceSurroundingBounds( &vMin, &vMax );
					NDebugOverlay::Box( vZero, vMin, vMax, 0, 255, 0, 128, 0.5f );
				}

				if ( !bTest )
				{
					CProp_Portal *pPortal = CProp_Portal::FindPortal( m_iPortalLinkageGroupID, bPortal2, true );

					pPortal->m_iDelayedFailure = PORTAL_FIZZLE_CANT_FIT;
					VectorAngles( tr.plane.normal, pPortal->m_qDelayedAngles );
					pPortal->m_vDelayedPosition = trDoor.endpos;

					vFinalPosition = pPortal->m_vDelayedPosition;
					qFinalAngles = pPortal->m_qDelayedAngles;
				}

				return PORTAL_ANALOG_SUCCESS_CANT_FIT;
			}
		}
		else if ( FClassnameIs( list[i], "trigger_portal_cleanser" ) )
		{
			CBaseTrigger *pTrigger = static_cast<CBaseTrigger*>( list[i] );

			if ( pTrigger && !pTrigger->m_bDisabled )
			{
				Vector vMin;
				Vector vMax;
				pTrigger->GetCollideable()->WorldSpaceSurroundingBounds( &vMin, &vMax );

				IntersectRayWithBox( ray.m_Start, ray.m_Delta, vMin, vMax, 0.0f, &tr );

				tr.plane.normal = -vDirection;

				if ( !bTest )
				{
					CProp_Portal *pPortal = CProp_Portal::FindPortal( m_iPortalLinkageGroupID, bPortal2, true );

					pPortal->m_iDelayedFailure = PORTAL_FIZZLE_CLEANSER;
					VectorAngles( tr.plane.normal, pPortal->m_qDelayedAngles );
					pPortal->m_vDelayedPosition = tr.endpos;

					vFinalPosition = pPortal->m_vDelayedPosition;
					qFinalAngles = pPortal->m_qDelayedAngles;
				}

				return PORTAL_ANALOG_SUCCESS_CLEANSER;
			}
		}
	}

	Vector vUp( 0.0f, 0.0f, 1.0f );
	if( ( tr.plane.normal.x > -0.001f && tr.plane.normal.x < 0.001f ) && ( tr.plane.normal.y > -0.001f && tr.plane.normal.y < 0.001f ) )
	{
		//plane is a level floor/ceiling
		vUp = vDirection;
	}

	// Check that the placement succeed
	VectorAngles( tr.plane.normal, vUp, qFinalAngles );

	vFinalPosition = tr.endpos;
	return VerifyPortalPlacement( CProp_Portal::FindPortal( m_iPortalLinkageGroupID, bPortal2 ), vFinalPosition, qFinalAngles, iPlacedBy, bTest );
}

float CWeaponPortalgun::FirePortal( bool bPortal2, Vector *pVector /*= 0*/, bool bTest /*= false*/ )
{
	bool bPlayer = false;
	Vector vEye;
	Vector vDirection;
	Vector vTracerOrigin;

	CBaseEntity *pOwner = GetOwner();

	if ( pOwner && pOwner->IsPlayer() )
	{
		bPlayer = true;
	}

	if( bPlayer )
	{
		CPortal_Player *pPlayer = (CPortal_Player *)pOwner;

		if ( !bTest && pPlayer )
		{
			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY, 0 );
		}

		Vector forward, right, up;
		AngleVectors( pPlayer->EyeAngles(), &forward, &right, &up );
		pPlayer->EyeVectors( &vDirection, NULL, NULL );
		vEye = pPlayer->EyePosition();

		// Check if the players eye is behind the portal they're in and translate it
		VMatrix matThisToLinked;
		CProp_Portal *pPlayerPortal = static_cast<CProp_Portal *>( pPlayer->m_hPortalEnvironment.Get() );

		if ( pPlayerPortal )
		{
			Vector ptPortalCenter;
			Vector vPortalForward;

			ptPortalCenter = pPlayerPortal->GetAbsOrigin();
			pPlayerPortal->GetVectors( &vPortalForward, NULL, NULL );

			Vector vEyeToPortalCenter = ptPortalCenter - vEye;

			float fPortalDist = vPortalForward.Dot( vEyeToPortalCenter );
			if( fPortalDist > 0.0f )
			{
				// Eye is behind the portal
				matThisToLinked = pPlayerPortal->MatrixThisToLinked();
			}
			else
			{
				pPlayerPortal = NULL;
			}
		}

		if ( pPlayerPortal )
		{
			UTIL_Portal_VectorTransform( matThisToLinked, forward, forward );
			UTIL_Portal_VectorTransform( matThisToLinked, right, right );
			UTIL_Portal_VectorTransform( matThisToLinked, up, up );
			UTIL_Portal_VectorTransform( matThisToLinked, vDirection, vDirection );
			UTIL_Portal_PointTransform( matThisToLinked, vEye, vEye );

			if ( pVector )
			{
				UTIL_Portal_VectorTransform( matThisToLinked, *pVector, *pVector );
			}
		}

		vTracerOrigin = vEye
			+ forward * 30.0f
			+ right * 4.0f
			+ up * (-5.0f);
	}
	else
	{
		// This portalgun is not held by the player-- Fire using the muzzle attachment
		Vector vecShootOrigin;
		QAngle angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		vEye = vecShootOrigin;
		vTracerOrigin = vecShootOrigin;
		AngleVectors( angShootDir, &vDirection, NULL, NULL );
	}

	if ( !bTest )
	{
		SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	}

	if ( pVector )
	{
		vDirection = *pVector;
	}

	Vector vTraceStart = vEye + (vDirection * m_fMinRange1);

	Vector vFinalPosition;
	QAngle qFinalAngles;

	PortalPlacedBy_t ePlacedBy = ( bPlayer ) ? ( PORTAL_PLACED_BY_PLAYER ) : ( PORTAL_PLACED_BY_PEDESTAL );

	trace_t tr;
	float fPlacementSuccess = TraceFirePortal( bPortal2, vTraceStart, vDirection, tr, vFinalPosition, qFinalAngles, ePlacedBy, bTest );

	if ( sv_portal_placement_never_fail.GetBool() )
	{
		fPlacementSuccess = 1.0f;
	}

	if ( !bTest )
	{
		CProp_Portal *pPortal = CProp_Portal::FindPortal( m_iPortalLinkageGroupID, bPortal2, true );

		// If it was a failure, put the effect at exactly where the player shot instead of where the portal bumped to
		if ( fPlacementSuccess < 0.5f )
			vFinalPosition = tr.endpos;

		pPortal->PlacePortal( vFinalPosition, qFinalAngles, static_cast<PortalPlacementResult_t>( static_cast<int>( fPlacementSuccess ) ), true );

		float fDelay = vTracerOrigin.DistTo( tr.endpos ) / ( ( bPlayer ) ? ( BLAST_SPEED ) : ( BLAST_SPEED_NON_PLAYER ) );

		QAngle qFireAngles;
		VectorAngles( vDirection, qFireAngles );
		DoEffectBlast( pPortal->m_bIsPortal2, ePlacedBy, vTracerOrigin, vFinalPosition, qFireAngles, fDelay );

		pPortal->SetContextThink( &CProp_Portal::DelayedPlacementThink, gpGlobals->curtime + fDelay, s_pDelayedPlacementContext ); 
		pPortal->m_vDelayedPosition = vFinalPosition;
		pPortal->m_hPlacedBy = this;
#ifdef PORTAL2
		if ( portal2_portalgun_debug.GetBool() )
		{
			Msg( "PORTAL2_PORTALGUN shot=%s result=%.3f position=%.1f %.1f %.1f linkage=%u\n",
				bPortal2 ? "orange" : "blue", fPlacementSuccess,
				vFinalPosition.x, vFinalPosition.y, vFinalPosition.z,
				(unsigned int)m_iPortalLinkageGroupID );
		}
#endif
	}

	return fPlacementSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPortalgun::StartEffects( void )
{
}

void CWeaponPortalgun::DestroyEffects( void )
{
	// Stop everything
	StopEffects();
}

//-----------------------------------------------------------------------------
// Purpose: Ready effects
//-----------------------------------------------------------------------------
void CWeaponPortalgun::DoEffectReady( void )
{
	if ( m_pMiniGravHoldSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		controller.SoundChangeVolume( m_pMiniGravHoldSound, 0.0, 0.1 );
	}
}


//-----------------------------------------------------------------------------
// Holding effects
//-----------------------------------------------------------------------------
void CWeaponPortalgun::DoEffectHolding( void )
{
	if ( m_pMiniGravHoldSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		controller.SoundChangeVolume( m_pMiniGravHoldSound, 1.0, 0.1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown for the weapon when it's holstered
//-----------------------------------------------------------------------------
void CWeaponPortalgun::DoEffectNone( void )
{
	if ( m_pMiniGravHoldSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		controller.SoundChangeVolume( m_pMiniGravHoldSound, 0.0, 0.1 );
	}
}

void CC_UpgradePortalGun( void )
{
	CPortal_Player *pPlayer = ToPortalPlayer( UTIL_GetCommandClient() );

	CWeaponPortalgun *pPortalGun = static_cast<CWeaponPortalgun*>( pPlayer->Weapon_OwnsThisType( "weapon_portalgun" ) );
	if ( pPortalGun )
	{
		pPortalGun->SetCanFirePortal1();
		pPortalGun->SetCanFirePortal2();
	}
}

static ConCommand upgrade_portal("upgrade_portalgun", CC_UpgradePortalGun, "Equips the player with a single portal portalgun. Use twice for a dual portal portalgun.\n\tArguments:   	none ", FCVAR_CHEAT);




#ifdef PORTAL2
static void CC_Portal2EquipPortalgun( const CCommand &args )
{
	CBaseEntity *pOwnerEntity = UTIL_GetCommandClient();
	if ( !pOwnerEntity && GameRules() && !GameRules()->IsMultiplayer() )
		pOwnerEntity = UTIL_GetLocalPlayer();
	CPortal_Player *pPlayer = ToPortalPlayer( pOwnerEntity );
	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		Warning( "PORTAL2_PORTALGUN: load a map and wait for the player before equipping.\n" );
		return;
	}

	CWeaponPortalgun *pPortalGun = dynamic_cast<CWeaponPortalgun *>( pPlayer->Weapon_OwnsThisType( "weapon_portalgun" ) );
	if ( !pPortalGun )
		pPortalGun = dynamic_cast<CWeaponPortalgun *>( pPlayer->GiveNamedItem( "weapon_portalgun" ) );
	// Explicit experimental equip also works during a map's pickup lock. Keep
	// the map's normal pickup policy intact; equip only the item just requested.
	if ( pPortalGun && !pPortalGun->GetOwner() && !pPortalGun->IsMarkedForDeletion() )
		pPlayer->Weapon_Equip( pPortalGun );
	if ( !pPortalGun || pPortalGun->GetOwner() != pPlayer )
	{
		Warning( "PORTAL2_PORTALGUN: could not give the portal gun to the player.\n" );
		return;
	}

	// Optional "blue" argument restores the intro single-portal gun; the
	// default form stays the experimental dual setup for F6/playground use.
	const bool bBlueOnly = args.ArgC() >= 2 && FStrEq( args[1], "blue" );
	pPortalGun->SetCanFirePortal1();
	if ( !bBlueOnly )
		pPortalGun->SetCanFirePortal2();
	const bool bEquipped = pPlayer->GetActiveWeapon() == pPortalGun || pPlayer->Weapon_Switch( pPortalGun );
	portal2_resume_portalgun.SetValue( 1 );
	Msg( "PORTAL2_PORTALGUN equipped=%d blue=%d orange=%d linkage=%u\n",
		bEquipped ? 1 : 0, pPortalGun->CanFirePortal1() != 0, pPortalGun->CanFirePortal2() != 0,
		(unsigned int)pPortalGun->m_iPortalLinkageGroupID );
}

static ConCommand portal2_equip_portalgun( "portal2_equip_portalgun", CC_Portal2EquipPortalgun,
	"Equip the dual portal gun in an already loaded experimental Portal 2 map. Optional 'blue' restores the intro single-portal (blue-only) gun.", FCVAR_CHEAT );
#endif

static void change_portalgun_linkage_id_f( const CCommand &args )
{
	if( sv_cheats->GetBool() == false ) //heavy handed version since setting the concommand with FCVAR_CHEATS isn't working like I thought
		return;

	if( args.ArgC() < 2 )
		return;

	unsigned char iNewID = (unsigned char)atoi( args[1] );

	CPortal_Player *pPlayer = (CPortal_Player *)UTIL_GetCommandClient();

	int iWeaponCount = pPlayer->WeaponCount();
	for( int i = 0; i != iWeaponCount; ++i )
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if( pWeapon == NULL )
			continue;

		if( dynamic_cast<CWeaponPortalgun *>(pWeapon) != NULL )
		{
			CWeaponPortalgun *pPortalGun = (CWeaponPortalgun *)pWeapon;
			pPortalGun->m_iPortalLinkageGroupID = iNewID;
			break;
		}
	}
}

ConCommand change_portalgun_linkage_id( "change_portalgun_linkage_id", change_portalgun_linkage_id_f, "Changes the portal linkage ID for the portal gun held by the commanding player.", FCVAR_CHEAT );
