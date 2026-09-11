//===== Copyright 1996-2006, Valve Corporation, All rights reserved. ======//
//
// Purpose: Entity that forwards notifications of teleports (through portals
//          touching its volume) to map logic entities.
//
// Reconstructed from private Portal 2 source; minimal API used by CProp_Portal.
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "func_portalled.h"
#include "prop_portal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( func_portalled, CFunc_Portalled );

BEGIN_DATADESC( CFunc_Portalled )

	DEFINE_OUTPUT( m_OnPrePortalled, "OnPrePortalled" ),
	DEFINE_OUTPUT( m_OnPostPortalled, "OnPostPortalled" ),
	DEFINE_OUTPUT( m_OnPortalNoLongerTouching, "OnPortalNoLongerTouching" ),

END_DATADESC()

CFunc_Portalled::CFunc_Portalled( void )
{
}

void CFunc_Portalled::Spawn( void )
{
	BaseClass::Spawn();
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	AddEffects( EF_NODRAW );
}

void CFunc_Portalled::Activate( void )
{
	BaseClass::Activate();
}

bool CFunc_Portalled::IsPortalTouchingDetector( CBaseEntity *pPortal )
{
	// Trigger volumes defined by the map carry their own touch logic.
	return pPortal != NULL && pPortal->GetAbsOrigin().IsValid();
}

void CFunc_Portalled::OnPrePortalled( CBaseEntity *pEntity, bool bRestorePortal )
{
	if ( pEntity )
	{
		m_OnPrePortalled.FireOutput( pEntity, pEntity );
	}
}

void CFunc_Portalled::OnPostPortalled( CBaseEntity *pEntity, bool bRestorePortal )
{
	if ( pEntity )
	{
		m_OnPostPortalled.FireOutput( pEntity, pEntity );
	}
}