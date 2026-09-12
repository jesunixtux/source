//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Paint gun weapon. Reconstructed from private Portal 2 source; the
//          paint streaming subsystem is not part of this port, so paint powers
//          are tracked client-side as gameplay state.
//
//=============================================================================//

#include "cbase.h"

#include "weapon_paintgun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( weapon_paintgun, CWeaponPaintGun );
PRECACHE_WEAPON_REGISTER(weapon_paintgun);

BEGIN_DATADESC( CWeaponPaintGun )

	DEFINE_FIELD( m_nCurrentColor, FIELD_INTEGER ),
	DEFINE_FIELD( m_nPaintAmmo, FIELD_INTEGER ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponPaintGun::CWeaponPaintGun( void )
{
	ResetAmmo();

	for( int i = 0; i < PAINT_POWER_TYPE_COUNT_PLUS_NO_POWER; ++i )
	{
		m_bHasPaint[i] = false;
	}

	m_nCurrentColor = SPEED_POWER;
}


//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CWeaponPaintGun::Precache( void )
{
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CWeaponPaintGun::Spawn( void )
{
	Precache();

	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: Gives the weapon a paint power
//-----------------------------------------------------------------------------
void CWeaponPaintGun::ActivatePaint( PaintPowerType nPowerType )
{
	if( nPowerType < 0 || nPowerType >= PAINT_POWER_TYPE_COUNT )
		return;

	m_bHasPaint[nPowerType] = true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponPaintGun::HasPaintPower( PaintPowerType nIndex ) const
{
	if( nIndex < 0 || nIndex >= PAINT_POWER_TYPE_COUNT )
		return false;

	return m_bHasPaint[nIndex];
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponPaintGun::HasAnyPaintPower( void ) const
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
PaintPowerType CWeaponPaintGun::GetCurrentPaint( void ) const
{
	return (PaintPowerType)m_nCurrentColor;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CWeaponPaintGun::GetPaintCount( void ) const
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
void CWeaponPaintGun::SetSubType( int iType )
{
	m_nCurrentColor = iType;
	BaseClass::SetSubType( iType );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponPaintGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	return BaseClass::Holster( pSwitchingTo );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponPaintGun::Deploy( void )
{
	SetModel( GetWorldModel() );
	return BaseClass::Deploy();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPaintGun::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPaintGun::WeaponIdle( void )
{
	BaseClass::WeaponIdle();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPaintGun::OnRestore( void )
{
	BaseClass::OnRestore();
}