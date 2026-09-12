//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Paint gun weapon (client). Reconstructed from private Portal 2
//          source; the paint streaming subsystem is not part of this port, so
//          paint powers are tracked as gameplay state.
//
//=============================================================================//

#ifndef C_WEAPON_PAINTGUN_H
#define C_WEAPON_PAINTGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_portalbasecombatweapon.h"
#include "paint_color_manager.h"

class C_WeaponPaintGun : public CBasePortalCombatWeapon
{
public:
	DECLARE_CLASS( C_WeaponPaintGun, CBasePortalCombatWeapon );

	C_WeaponPaintGun( void );

	virtual void Precache( void );
	virtual void Spawn( void );

	// Paint power management
	bool HasPaintPower( PaintPowerType nIndex ) const;
	bool HasAnyPaintPower( void ) const;
	PaintPowerType GetCurrentPaint( void ) const;
	int GetPaintCount( void ) const;

	void SetSubType( int iType ) OVERRIDE;
	void ChangeRenderColor( void );

	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL ) OVERRIDE;
	virtual bool Deploy( void ) OVERRIDE;
	virtual void Drop( const Vector &vecVelocity ) OVERRIDE;
	virtual void WeaponIdle( void ) OVERRIDE;

	PortalWeaponID GetWeaponID( void ) const { return WEAPON_PAINTGUN; }

private:
	bool	m_bHasPaint[PAINT_POWER_TYPE_COUNT_PLUS_NO_POWER];
	int		m_nCurrentColor;
};

#endif // C_WEAPON_PAINTGUN_H