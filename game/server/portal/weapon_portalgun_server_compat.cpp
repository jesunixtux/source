#include "cbase.h"
#include "weapon_portalgun.h"
#include "env_portal_laser.h"
#ifdef NO_STEAM
CSteamAPIContext *steamapicontext = NULL;
#endif
CWeaponPortalgun::CWeaponPortalgun() {}
void CWeaponPortalgun::Precache(){}
void CWeaponPortalgun::OnRestore(){}
void CWeaponPortalgun::UpdateOnRemove(){BaseClass::UpdateOnRemove();}
void CWeaponPortalgun::PrimaryAttack()
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !m_bCanFirePortal1 || !pPlayer )
		return;
	FirePortal( false, NULL, false );
	m_iLastFiredPortal = 1;
	WeaponSound( SINGLE );
	m_OnFiredPortal1.FireOutput( pPlayer, this );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
}
void CWeaponPortalgun::SecondaryAttack()
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !m_bCanFirePortal2 || !pPlayer )
		return;
	FirePortal( true, NULL, false );
	m_iLastFiredPortal = 2;
	WeaponSound( WPN_DOUBLE );
	m_OnFiredPortal2.FireOutput( pPlayer, this );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
}
void CWeaponPortalgun::ItemPreFrame(){}
void CWeaponPortalgun::ItemHolsterFrame(){}
void CWeaponPortalgun::WeaponIdle(){}
bool CWeaponPortalgun::Reload(){return false;}
bool CWeaponPortalgun::Deploy(){return true;}
bool CWeaponPortalgun::Holster(CBaseCombatWeapon*){return true;}
bool CWeaponPortalgun::ShouldDrawCrosshair(){return true;}
void CWeaponPortalgun::StopEffects(bool){}
void CWeaponPortalgun::DoEffect(int,Vector*){}
void CWeaponPortalgun::PlayPickupSound(){}
void CWeaponPortalgun::DelayAttack(float){}
void CWeaponPortalgun::UseDeny(){}
Activity CWeaponPortalgun::GetPrimaryAttackActivity(){return ACT_VM_IDLE;}
acttable_t CWeaponPortalgun::m_acttable[] = {};
IMPLEMENT_ACTTABLE(CWeaponPortalgun)
void CWeaponPortalgun::SetCanFirePortal1(bool v){m_bCanFirePortal1=v;}
void CWeaponPortalgun::SetCanFirePortal2(bool v){m_bCanFirePortal2=v;}

CPortalLaser::CPortalLaser() {}
CPortalLaser::~CPortalLaser() {}
Vector CPortalLaser::ClosestPointOnLineSegment(const Vector &point){return GetAbsOrigin();}

void WallPainted(int,int,CBaseEntity*){}
void PaintPowerPickup(int,CBasePlayer*){}
CBaseEntity *GetCoopSpawnLocation(int){return NULL;}

PortalPlacementResult_t CWeaponPortalgun::FirePortal(bool bPortal2,Vector *pDirection)
{
	const float result = FirePortal( bPortal2, pDirection, false );
	return static_cast<PortalPlacementResult_t>( static_cast<int>( result ) );
}

BEGIN_SIMPLE_DATADESC(PortalPlayerStatistics_t)
END_DATADESC()
