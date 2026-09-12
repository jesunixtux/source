//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Paint gun weapon. Reconstructed from private Portal 2 source; the
//          paint streaming subsystem it depends on is not part of this port,
//          so the weapon exposes the same gameplay API with local state.
//
//=============================================================================//

#ifndef WEAPON_PAINTGUN_H
#define WEAPON_PAINTGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_portalbasecombatweapon.h"
#include "paint_color_manager.h"
#include "paint_stream.h"

class CWeaponPaintGun : public CBasePortalCombatWeapon
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CWeaponPaintGun, CBasePortalCombatWeapon );

	CWeaponPaintGun( void );
	virtual void ItemPostFrame( void );
	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );
	virtual void StartShootingSound( void ) {}
	virtual void StopShootingSound( void ) {}

	virtual void Precache( void );
	virtual void Spawn( void );

	// Paint power management
	void ActivatePaint( PaintPowerType nPowerType );
	bool HasPaintPower( PaintPowerType nIndex ) const;
	bool HasAnyPaintPower( void ) const;
	PaintPowerType GetCurrentPaint( void ) const;
	int GetPaintCount( void ) const;
	bool HasPaintAmmo( unsigned paintType ) const;
	void DecrementPaintAmmo( unsigned paintType );
	void ResetAmmo( void );
	void SprayPaint( float flDeltaTime, int paintType );

	void SetSubType( int iType ) OVERRIDE;

	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL ) OVERRIDE;
	virtual bool Deploy( void ) OVERRIDE;
	virtual void Drop( const Vector &vecVelocity ) OVERRIDE;
	virtual void WeaponIdle( void ) OVERRIDE;

	void OnRestore( void );

	PortalWeaponID GetWeaponID( void ) const { return WEAPON_PAINTGUN; }

private:
	DECLARE_ACTTABLE();
	bool	m_bHasPaint[PAINT_POWER_TYPE_COUNT_PLUS_NO_POWER];
	int		m_nCurrentColor;
	int		m_nPaintAmmo;
	CUtlVector<int> m_PaintAmmoPerType;
	CHandle<CPaintStream> m_hPaintStream[PAINT_POWER_TYPE_COUNT];
	bool m_bFiringPaint;
	bool m_bFiringErase;
	float m_flAccumulatedTime;
	int m_nBlobRandomSeed;
};

#endif // WEAPON_PAINTGUN_H
