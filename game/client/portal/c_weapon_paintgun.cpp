//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Paint gun weapon (client). Reconstructed from private Portal 2
//          source; the paint streaming subsystem is not part of this port, so
//          paint powers are tracked as gameplay state.
//
//=============================================================================//

#include "cbase.h"

#include "c_weapon_paintgun.h"

#include "c_portal_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( weapon_paintgun, C_WeaponPaintGun );
PRECACHE_WEAPON_REGISTER(weapon_paintgun);


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_WeaponPaintGun::C_WeaponPaintGun( void )
{
	for( int i = 0; i < PAINT_POWER_TYPE_COUNT_PLUS_NO_POWER; ++i )
	{
		m_bHasPaint[i] = false;
	}

	m_nCurrentColor = SPEED_POWER;
}


//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void C_WeaponPaintGun::Precache( void )
{
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void C_WeaponPaintGun::Spawn( void )
{
	Precache();

	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_WeaponPaintGun::HasPaintPower( PaintPowerType nIndex ) const
{
	if( nIndex < 0 || nIndex >= PAINT_POWER_TYPE_COUNT )
		return false;

	return m_bHasPaint[nIndex];
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_WeaponPaintGun::HasAnyPaintPower( void ) const
{
	for( int i = 0; i < PAINT_POWER_TYPE_COUNT; ++i )
	{
		if( HasPaintPower( (PaintPowerType)i ) )
		{
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
PaintPowerType C_WeaponPaintGun::GetCurrentPaint( void ) const
{
	return (PaintPowerType)m_nCurrentColor;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_WeaponPaintGun::GetPaintCount( void ) const
{
	int nCount = 0;
	for( int i = 0; i < PAINT_POWER_TYPE_COUNT; ++i )
	{
		if( HasPaintPower( (PaintPowerType)i ) )
		{
			++nCount;
		}
	}

	return nCount;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WeaponPaintGun::SetSubType( int iType )
{
	m_nCurrentColor = iType;
	BaseClass::SetSubType( iType );
}


//-----------------------------------------------------------------------------
// Purpose: Tint the view model to the currently selected paint color
//-----------------------------------------------------------------------------
void C_WeaponPaintGun::ChangeRenderColor( void )
{
	Color color = MapPowerToVisualColor( m_nCurrentColor );

	// Tint the weapon and the view model
	SetRenderColor( color.r(), color.g(), color.b() );

	C_BasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if( pOwner )
	{
		C_BaseViewModel *pViewModel = pOwner->GetViewModel( 0 );
		if( pViewModel )
		{
			C_BaseCombatWeapon *pActive = pOwner->GetActiveWeapon();
			if( pActive == this )
			{
				pViewModel->SetRenderColor( color.r(), color.g(), color.b() );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_WeaponPaintGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	return BaseClass::Holster( pSwitchingTo );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_WeaponPaintGun::Deploy( void )
{
	SetModel( GetWorldModel() );
	return BaseClass::Deploy();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WeaponPaintGun::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_WeaponPaintGun::WeaponIdle( void )
{
	BaseClass::WeaponIdle();
}