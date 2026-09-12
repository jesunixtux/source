//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Portal floor turret. Moved from the .cpp so the class is visible
//          to the shared tractor beam code; provides the entity surface used
//          by this port.
//
//=============================================================================//

#ifndef NPC_PORTAL_TURRET_FLOOR_H
#define NPC_PORTAL_TURRET_FLOOR_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "npc_turret_floor.h"
#include "rope.h"

#define SF_FLOOR_TURRET_AUTOACTIVATE		0x00000020
#define SF_FLOOR_TURRET_STARTINACTIVE		0x00000040
#define SF_FLOOR_TURRET_OUT_OF_AMMO			0x00000100

#define	FLOOR_TURRET_PORTAL_MODEL	"models/props/Turret_01.mdl"
#define FLOOR_TURRET_GLOW_SPRITE	"sprites/glow1.vmt"
#define FLOOR_TURRET_BC_YAW			"aim_yaw"
#define FLOOR_TURRET_BC_PITCH		"aim_pitch"
#define	PORTAL_FLOOR_TURRET_RANGE	1500
#define	PORTAL_FLOOR_TURRET_MAX_SHOT_DELAY	2.5f
#define	FLOOR_TURRET_MAX_WAIT		5
#define FLOOR_TURRET_SHORT_WAIT		2.0f		// Used for FAST_RETIRE spawnflag

#define SF_FLOOR_TURRET_FASTRETIRE			0x00000080

#define TURRET_FLOOR_DAMAGE_MULTIPLIER 3.0f
#define TURRET_FLOOR_BULLET_FORCE_MULTIPLIER 0.4f
#define TURRET_FLOOR_PHYSICAL_FORCE_MULTIPLIER 135.0f

#define PORTAL_FLOOR_TURRET_NUM_ROPES 4

//Turret states
enum portalTurretState_e
{
	PORTAL_TURRET_DISABLED = TURRET_STATE_TOTAL,
	PORTAL_TURRET_COLLIDE,
	PORTAL_TURRET_PICKUP,
	PORTAL_TURRET_SHOTAT,
	PORTAL_TURRET_DISSOLVED,

	PORTAL_TURRET_STATE_TOTAL
};

class CNPC_Portal_FloorTurret : public CNPC_FloorTurret
{
	DECLARE_CLASS( CNPC_Portal_FloorTurret, CNPC_FloorTurret );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

public:

	CNPC_Portal_FloorTurret( void );

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	Activate( void );
	virtual void	UpdateOnRemove( void );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	virtual bool	ShouldAttractAutoAim( CBaseEntity *pAimingEnt );
	virtual float	GetAutoAimRadius();
	virtual Vector	GetAutoAimCenter();

	virtual void	OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );

	virtual void	NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params );

	virtual bool	PreThink( turretState_e state );
	virtual void	Shoot( const Vector &vecSrc, const Vector &vecDirToEnemy, bool bStrict = false );
	virtual void	SetEyeState( eyeState_t state );

	virtual bool	OnSide( void );

	virtual float	GetAttackDamageScale( CBaseEntity *pVictim );
	virtual Vector	GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget );

	// Think functions
	virtual void	Retire( void );
	virtual void	Deploy( void );
	virtual void	ActiveThink( void );
	virtual void	SearchThink( void );
	virtual void	AutoSearchThink( void );
	virtual void	TippedThink( void );
	virtual void	HeldThink( void );
	virtual void	InactiveThink( void );
	virtual void	SuppressThink( void );
	virtual void	DisabledThink( void );
	virtual void	HackFindEnemy( void );

	virtual void	StartTouch( CBaseEntity *pOther );

	bool	IsLaserOn( void ) { return m_bLaserOn; }
	void	LaserOff( void );
	void	LaserOn( void );
	void	RopesOn();
	void	RopesOff();

	void	FireBullet( const char *pTargetName );

	// Inputs
	void	InputFireBullet( inputdata_t &inputdata );

	void	OnExitedTractorBeam( void );

	bool	IsProjectedWallBlockingTurretFromPlayer( CBasePlayer *pPlayer ) { return false; }

private:

	CHandle<CRopeKeyframe>	m_hRopes[ PORTAL_FLOOR_TURRET_NUM_ROPES ];

	CNetworkVar( bool, m_bOutOfAmmo );
	CNetworkVar( bool, m_bLaserOn );
	CNetworkVar( int, m_sLaserHaloSprite );

	int		m_iBarrelAttachments[ 4 ];
	bool	m_bShootWithBottomBarrels;
	bool	m_bDamageForce;

	float	m_fSearchSpeed;
	float	m_fMovingTargetThreashold;
	float	m_flDistToEnemy;

	turretState_e	m_iLastState;
	float			m_fNextTalk;
	bool			m_bDelayTippedTalk;

};

#endif // NPC_PORTAL_TURRET_FLOOR_H