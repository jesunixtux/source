#include "cbase.h"
#include "cpaintblob.h"

CBasePaintBlob::CBasePaintBlob() : m_flDestVortexRadius(0), m_flCurrentVortexRadius(0), m_flCurrentVortexSpeed(0), m_flVortexDirection(1), m_vecTempEndPosition(vec3_origin), m_vecTempEndVelocity(vec3_origin), m_vecPosition(vec3_origin), m_vecPrevPosition(vec3_origin), m_vecVelocity(vec3_origin), m_vContactNormal(vec3_origin), m_paintType(NO_POWER), m_hOwner(NULL), m_MoveState(PAINT_BLOB_AIR_MOVE), m_flLifeTime(0), m_vecStreakDir(vec3_origin), m_bStreakDirChanged(false), m_flStreakTimer(0), m_flStreakSpeedDampenRate(0), m_bDeleteFlag(false), m_flAccumulatedTime(0), m_flLastUpdateTime(0), m_flRadiusScale(1), m_bShouldPlayEffect(true), m_bInTractorBeam(false), m_bSilent(false), m_hPortal(NULL), m_vCollisionBoxCenter(vec3_origin), m_bCollisionBoxHitSolid(false), m_bShouldPlaySound(true), m_bDrawOnly(false), m_bTeleportedThisFrame(false), m_nTeleportationCount(0) {}
CBasePaintBlob::~CBasePaintBlob() {}
void CBasePaintBlob::Init(const Vector &o,const Vector &v,int t,float life,float damp,CBaseEntity *owner,bool silent,bool drawOnly){m_vecPosition=o;m_vecPrevPosition=o;m_vecVelocity=v;m_paintType=(PaintPowerType)t;m_flStreakTimer=life;m_flStreakSpeedDampenRate=damp;m_hOwner=owner;m_bSilent=silent;m_bDrawOnly=drawOnly;m_bShouldPlayEffect=!drawOnly;}
const Vector& CBasePaintBlob::GetPosition() const{return m_vecPosition;}
const Vector& CBasePaintBlob::GetContactNormal() const{return m_vContactNormal;}
bool CBasePaintBlob::ShouldDeleteThis() const{return m_bDeleteFlag;}
bool CBasePaintBlob::ShouldPlayEffect() const{return m_bShouldPlayEffect;}
bool CBasePaintBlob::ShouldPlaySound() const{return m_bShouldPlaySound;}
void CBasePaintBlob::SetShouldPlaySound(bool v){m_bShouldPlaySound=v;}
void CBasePaintBlob::SetDeletionFlag(bool v){m_bDeleteFlag=v;}
void CBasePaintBlob::UpdateBlobCollision(float,const Vector&,Vector&){}
void CBasePaintBlob::UpdateBlobPostCollision(float){}
void PaintBlobUpdate(const PaintBlobVector_t&){}
