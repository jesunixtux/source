#include "cbase.h"
#include "portal_placement.h"
#include "portal2/portal_grabcontroller_shared.h"
#include "weapon_paintgun.h"

ConVar player_held_object_use_view_model("player_held_object_use_view_model", "0", FCVAR_REPLICATED | FCVAR_CHEAT);
ConVar sv_portal_placement_never_fail("sv_portal_placement_never_fail", "0", FCVAR_REPLICATED | FCVAR_CHEAT);

CBasePlayer *GetPlayerHoldingEntity(const CBaseEntity *pEntity){return GetPlayerHoldingEntity(const_cast<CBaseEntity*>(pEntity));}
void RotatePlayerHeldObject(CBasePlayer *,float,float,bool){}
void UpdateGrabControllerTargetPosition(CBasePlayer *p,Vector *v,QAngle *a,bool){UpdateGrabControllerTargetPosition(p,v,a);}

bool PortalPlacementSucceeded(PortalPlacementResult_t r){return r == PORTAL_PLACEMENT_SUCCESS;}
bool IsPortalIntersectingNoPortalVolume(const Vector&,const QAngle&,const Vector&,float,float){return false;}
PortalPlacementResult_t IsPortalOverlappingOtherPortals(const CProp_Portal*,const Vector&,const QAngle&,float,float,bool,bool){return PORTAL_PLACEMENT_SUCCESS;}
PortalPlacementResult_t VerifyPortalPlacementAndFizzleBlockingPortals(const CProp_Portal*,Vector&,QAngle&,float,float,PortalPlacedBy_t){return PORTAL_PLACEMENT_SUCCESS;}

void CWeaponPaintGun::ItemPostFrame(){}
void CWeaponPaintGun::PrimaryAttack(){}
void CWeaponPaintGun::SecondaryAttack(){}
void CWeaponPaintGun::ResetAmmo(){}
acttable_t CWeaponPaintGun::m_acttable[] = {};
IMPLEMENT_ACTTABLE(CWeaponPaintGun)
