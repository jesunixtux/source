#include "cbase.h"
#include "cpaintblob.h"
#include "paintable_entity.h"
#include "paint_database.h"
#include "portal_util_shared.h"
#include "tier0/icommandline.h"

CBasePaintBlob::CBasePaintBlob() : m_flDestVortexRadius(0), m_flCurrentVortexRadius(0), m_flCurrentVortexSpeed(0), m_flVortexDirection(1), m_vecTempEndPosition(vec3_origin), m_vecTempEndVelocity(vec3_origin), m_vecPosition(vec3_origin), m_vecPrevPosition(vec3_origin), m_vecVelocity(vec3_origin), m_vContactNormal(vec3_origin), m_paintType(NO_POWER), m_hOwner(NULL), m_MoveState(PAINT_BLOB_AIR_MOVE), m_flLifeTime(0), m_vecStreakDir(vec3_origin), m_bStreakDirChanged(false), m_flStreakTimer(0), m_flStreakSpeedDampenRate(0), m_bDeleteFlag(false), m_flAccumulatedTime(0), m_flLastUpdateTime(0), m_flRadiusScale(1), m_bShouldPlayEffect(true), m_bInTractorBeam(false), m_bSilent(false), m_hPortal(NULL), m_vCollisionBoxCenter(vec3_origin), m_bCollisionBoxHitSolid(false), m_bShouldPlaySound(true), m_bDrawOnly(false), m_bTeleportedThisFrame(false), m_nTeleportationCount(0) {}
CBasePaintBlob::~CBasePaintBlob() {}
void CBasePaintBlob::Init(const Vector &o,const Vector &v,int t,float life,float damp,CBaseEntity *owner,bool silent,bool drawOnly){m_vecPosition=o;m_vecPrevPosition=o;m_vecVelocity=v;m_paintType=(PaintPowerType)t;m_flStreakTimer=life;m_flStreakSpeedDampenRate=damp;m_hOwner=owner;m_bSilent=silent;m_bDrawOnly=drawOnly;m_bShouldPlayEffect=false;m_bDeleteFlag=false;m_flLifeTime=0;m_flLastUpdateTime=gpGlobals->curtime;}
const Vector& CBasePaintBlob::GetPosition() const{return m_vecPosition;}
void CBasePaintBlob::SetPosition(const Vector &v){m_vecPosition=v;}
const Vector& CBasePaintBlob::GetVelocity() const{return m_vecVelocity;}
void CBasePaintBlob::SetVelocity(const Vector &v){m_vecVelocity=v;}
const Vector& CBasePaintBlob::GetContactNormal() const{return m_vContactNormal;}
bool CBasePaintBlob::ShouldDeleteThis() const{return m_bDeleteFlag;}
bool CBasePaintBlob::ShouldPlayEffect() const{return m_bShouldPlayEffect;}
bool CBasePaintBlob::ShouldPlaySound() const{return m_bShouldPlaySound;}
void CBasePaintBlob::SetShouldPlaySound(bool v){m_bShouldPlaySound=v;}
void CBasePaintBlob::SetDeletionFlag(bool v){m_bDeleteFlag=v;}
void CBasePaintBlob::UpdateBlobCollision(float,const Vector& end,Vector& endVelocity)
{
	trace_t trace;
	UTIL_TraceLine( m_vecPosition, end, MASK_SOLID, m_hOwner.Get(), COLLISION_GROUP_NONE, &trace );
	m_vecPrevPosition = m_vecPosition;
	if ( trace.DidHit() && trace.m_pEnt )
	{
		m_vecPosition = trace.endpos;
		m_vContactNormal = trace.plane.normal;
		PaintBlobPaint( trace );
		m_bShouldPlayEffect = !m_bDrawOnly;
		m_bDeleteFlag = true;
		return;
	}
	m_vecPosition = end;
	m_vecVelocity = endVelocity;
}
void CBasePaintBlob::UpdateBlobPostCollision(float dt)
{
	m_flAccumulatedTime += dt;
}
void PaintBlobUpdate(const PaintBlobVector_t &blobs)
{
	for ( int i = 0; i < blobs.Count(); ++i )
	{
		CBasePaintBlob *blob = blobs[i];
		if ( !blob || blob->ShouldDeleteThis() ) continue;
		const float dt = clamp( gpGlobals->curtime - blob->GetLastUpdateTime(), 0.0f, 0.05f );
		blob->SetLastUpdateTime( gpGlobals->curtime );
		if ( dt <= 0 ) continue;
		blob->UpdateLifeTime( dt );
		// shortcut: straight ballistic blobs; integrate portal and tractor-beam paths when those maps are tested.
		if ( blob->GetLifeTime() > 5.0f ) { blob->SetDeletionFlag( true ); continue; }
		Vector velocity = blob->GetVelocity();
		velocity.z -= 600.0f * dt;
		const Vector end = blob->GetPosition() + velocity * dt;
		blob->UpdateBlobCollision( dt, end, velocity );
		if ( !blob->ShouldDeleteThis() ) blob->UpdateBlobPostCollision( dt );
	}
}
float CBasePaintBlob::GetLifeTime() const{return m_flLifeTime;}
void CBasePaintBlob::UpdateLifeTime(float dt){m_flLifeTime+=dt;}

#ifdef PORTAL2
void CPaintBlob::PaintBlobPaint( const trace_t &trace )
{
	if ( m_bDrawOnly )
		return;

	CBaseEntity *hit = trace.m_pEnt;
	const bool painted = PaintDatabase.AddPaint( hit, trace.endpos, trace.plane.normal,
		m_paintType, 60.0f, 1.0f );
	if ( painted && hit->IsBSPModel() && CommandLine()->FindParm( "-portal2_paint_probe" ) )
	{
		static bool checked = false;
		if ( !checked )
		{
			const PaintPowerType observed = UTIL_Paint_TracePower( hit,
				trace.endpos + trace.plane.normal * 0.1f, trace.plane.normal );
			Msg( "PORTAL2_PAINT_ROUNDTRIP expected=%d observed=%d %s\n",
				m_paintType, observed, observed == m_paintType ? "PASS" : "FAIL" );
			checked = true;
		}
	}

	static bool reported = false;
	if ( !reported && CommandLine()->FindParm( "-portal2_paint_probe" ) )
	{
		Msg( "PORTAL2_PAINT_APPLY target=%s type=%d result=%s\n",
			hit ? hit->GetClassname() : "<none>", m_paintType,
			painted ? ( hit && hit->IsBSPModel() ? "world-painted" : "entity-painted" ) : "not-painted" );
		reported = true;
	}
}
#endif
