#include "cbase.h"
#include "props.h"
#include "triggers.h"
#include "info_placement_helper.h"
#include "paint_sprayer.h"

namespace
{
const char *const kFutbolModel = "models/props/futbol.mdl";

bool IsWheatleyCollision(CBaseEntity *pEntity)
{
	if (!pEntity)
		return false;

	return FClassnameIs(pEntity, "npc_wheatley_boss") ||
		FStrEq(STRING(pEntity->GetEntityName()), "wheatley_shield");
}

ConVar exploding_futbol_explosion_damage("exploding_futbol_explosion_damage", "100", FCVAR_REPLICATED | FCVAR_CHEAT);
ConVar r_flashlightbrightness("r_flashlightbrightness", "1", FCVAR_REPLICATED | FCVAR_CHEAT);

static void Portal2CompatNoopCommand(const CCommand &) {}
static ConCommand voicerecord_toggle("voicerecord_toggle", Portal2CompatNoopCommand,
	"Portal 2 compatibility command; voice capture is unavailable in this build.", FCVAR_CHEAT);
static ConCommand ss_force_primary_fullscreen("ss_force_primary_fullscreen", Portal2CompatNoopCommand,
	"Portal 2 compatibility command; splitscreen fullscreen is unavailable in this build.", FCVAR_CHEAT);
}

class CPropExplodingFutbol : public CPhysicsProp
{
public:
	DECLARE_CLASS(CPropExplodingFutbol, CPhysicsProp);
	DECLARE_DATADESC();

	CPropExplodingFutbol() : m_bExplodeOnTouch(true), m_bExploded(false) {}

	void Precache(void) override
	{
		if (GetModelName() == NULL_STRING)
			SetModel(kFutbolModel);
		PrecacheModel(kFutbolModel);
		BaseClass::Precache();
	}

	void Spawn(void) override
	{
		PrecacheModel(kFutbolModel);
		SetModel(kFutbolModel);
		Precache();
		BaseClass::Spawn();
		SetCollisionGroup(COLLISION_GROUP_DEBRIS);
		SetThink(&CPropExplodingFutbol::CheckBossProximity);
		SetNextThink(gpGlobals->curtime + 0.05f);
	}

	void VPhysicsCollision(int index, gamevcollisionevent_t *pEvent) override
	{
		const int otherIndex = !index;
		CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
		const bool bossHit = IsWheatleyCollision(pHitEntity);

		if (m_bExplodeOnTouch && (bossHit || pEvent->collisionSpeed > 150.0f))
		{
			Explode(pHitEntity);
			return;
		}

		BaseClass::VPhysicsCollision(index, pEvent);
	}

private:
	void CheckBossProximity(void)
	{
		CBaseEntity *pBoss = gEntList.FindEntityByName(NULL, "@sphere");
		if (pBoss && GetAbsOrigin().DistTo(pBoss->WorldSpaceCenter()) < 192.0f)
		{
			Explode(pBoss);
			return;
		}

		SetNextThink(gpGlobals->curtime + 0.05f);
	}

	void InputExplode(inputdata_t &inputdata)
	{
		Explode(inputdata.pActivator);
	}

	void Explode(CBaseEntity *pActivator)
	{
		if (m_bExploded)
			return;
		m_bExploded = true;

		CBaseEntity *pBoss = gEntList.FindEntityByName(NULL, "@sphere");
		if (!pBoss)
			pBoss = gEntList.FindEntityByClassname(NULL, "npc_wheatley_boss");

		if (pBoss && (pActivator == pBoss || IsWheatleyCollision(pActivator)))
		{
			CTakeDamageInfo damage(this, pActivator ? pActivator : this,
				exploding_futbol_explosion_damage.GetFloat(), DMG_BLAST);
			pBoss->TakeDamage(damage);
		}

		UTIL_Remove(this);
	}

	bool m_bExplodeOnTouch;
	bool m_bExploded;
};

LINK_ENTITY_TO_CLASS(prop_exploding_futbol, CPropExplodingFutbol);

BEGIN_DATADESC(CPropExplodingFutbol)
	DEFINE_KEYFIELD(m_bExplodeOnTouch, FIELD_BOOLEAN, "ExplodeOnTouch"),
	DEFINE_FIELD(m_bExploded, FIELD_BOOLEAN),
	DEFINE_INPUTFUNC(FIELD_VOID, "Explode", InputExplode),
	DEFINE_THINKFUNC(CheckBossProximity),
END_DATADESC()

class CPointFutbolShooter : public CPointEntity
{
public:
	DECLARE_CLASS(CPointFutbolShooter, CPointEntity);
	DECLARE_DATADESC();

	CPointFutbolShooter() : m_flLaunchSpeed(400.0f), m_bDisabled(false) {}

	void Spawn(void) override
	{
		BaseClass::Spawn();
		if (m_flLaunchSpeed <= 0.0f)
			m_flLaunchSpeed = 400.0f;
	}

private:
	void InputShoot(inputdata_t &)
	{
		if (m_bDisabled)
			return;

		Vector direction;
		AngleVectors(GetAbsAngles(), &direction);
		CBaseEntity *pFutbol = CBaseEntity::Create("prop_exploding_futbol", GetAbsOrigin(), GetAbsAngles(), this);
		if (pFutbol)
			pFutbol->SetAbsVelocity(direction * m_flLaunchSpeed);
	}

	void InputEnable(inputdata_t &) { m_bDisabled = false; }
	void InputDisable(inputdata_t &) { m_bDisabled = true; }

	float m_flLaunchSpeed;
	bool m_bDisabled;
};

LINK_ENTITY_TO_CLASS(point_futbol_shooter, CPointFutbolShooter);

BEGIN_DATADESC(CPointFutbolShooter)
	DEFINE_KEYFIELD(m_flLaunchSpeed, FIELD_FLOAT, "launchSpeed"),
	DEFINE_FIELD(m_bDisabled, FIELD_BOOLEAN),
	DEFINE_INPUTFUNC(FIELD_VOID, "Shoot", InputShoot),
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
END_DATADESC()

class CCompatPaintSphere : public CPointEntity
{
public:
	DECLARE_CLASS(CCompatPaintSphere, CPointEntity);
	DECLARE_DATADESC();

	CCompatPaintSphere() : m_flRadius(0.0f), m_nPaintType(NO_POWER), m_flAlpha(1.0f) {}

private:
	float m_flRadius;
	int m_nPaintType;
	float m_flAlpha;
};

LINK_ENTITY_TO_CLASS(paint_sphere, CCompatPaintSphere);

BEGIN_DATADESC(CCompatPaintSphere)
	DEFINE_KEYFIELD(m_flRadius, FIELD_FLOAT, "radius"),
	DEFINE_KEYFIELD(m_nPaintType, FIELD_INTEGER, "paint_type"),
	DEFINE_KEYFIELD(m_flAlpha, FIELD_FLOAT, "alpha_percent"),
END_DATADESC()

class CCompatPaintBomb : public CPhysicsProp
{
public:
	DECLARE_CLASS(CCompatPaintBomb, CPhysicsProp);
	DECLARE_DATADESC();

	void Precache(void) override
	{
		if (GetModelName() == NULL_STRING)
			SetModel(kFutbolModel);
		PrecacheModel(kFutbolModel);
		BaseClass::Precache();
	}

	void Spawn(void) override
	{
		PrecacheModel(kFutbolModel);
		SetModel(kFutbolModel);
		Precache();
		BaseClass::Spawn();
	}

private:
	int m_nPaintType;
	bool m_bAllowFunnel;
};

LINK_ENTITY_TO_CLASS(prop_paint_bomb, CCompatPaintBomb);

BEGIN_DATADESC(CCompatPaintBomb)
	DEFINE_KEYFIELD(m_nPaintType, FIELD_INTEGER, "PaintType"),
	DEFINE_KEYFIELD(m_bAllowFunnel, FIELD_BOOLEAN, "allowfunnel"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(info_placement_helper, CInfoPlacementHelper);
LINK_ENTITY_TO_CLASS(info_paint_sprayer, CPaintSprayer);

class CCompatTriggerCatapult : public CBaseTrigger
{
public:
	DECLARE_CLASS(CCompatTriggerCatapult, CBaseTrigger);
	DECLARE_DATADESC();

	CCompatTriggerCatapult() : m_flPlayerSpeed(450.0f), m_flPhysicsSpeed(450.0f), m_bCompatDisabled(false) {}

	void Spawn(void) override
	{
		BaseClass::Spawn();
		InitTrigger();
		SetTouch(&CCompatTriggerCatapult::Touch);
	}

private:
	void Touch(CBaseEntity *pOther)
	{
		if (m_bCompatDisabled || m_bDisabled || !pOther)
			return;

		Vector direction = m_vecLaunchDirection;
		if (m_iszLaunchTarget != NULL_STRING)
		{
			CBaseEntity *pTarget = gEntList.FindEntityByName(NULL, m_iszLaunchTarget);
			if (pTarget)
				direction = pTarget->WorldSpaceCenter() - pOther->WorldSpaceCenter();
		}

		if (VectorNormalize(direction) <= 0.0f)
			return;

		const float speed = pOther->IsPlayer() ? m_flPlayerSpeed : m_flPhysicsSpeed;
		pOther->SetAbsVelocity(direction * speed);
	}

	void InputEnable(inputdata_t &) { m_bCompatDisabled = false; m_bDisabled = false; }
	void InputDisable(inputdata_t &) { m_bCompatDisabled = true; m_bDisabled = true; }

	string_t m_iszLaunchTarget;
	Vector m_vecLaunchDirection;
	float m_flPlayerSpeed;
	float m_flPhysicsSpeed;
	bool m_bCompatDisabled;
};

LINK_ENTITY_TO_CLASS(trigger_catapult, CCompatTriggerCatapult);

BEGIN_DATADESC(CCompatTriggerCatapult)
	DEFINE_KEYFIELD(m_iszLaunchTarget, FIELD_STRING, "launchTarget"),
	DEFINE_KEYFIELD(m_vecLaunchDirection, FIELD_VECTOR, "launchDirection"),
	DEFINE_KEYFIELD(m_flPlayerSpeed, FIELD_FLOAT, "playerSpeed"),
	DEFINE_KEYFIELD(m_flPhysicsSpeed, FIELD_FLOAT, "physicsSpeed"),
	DEFINE_FIELD(m_bCompatDisabled, FIELD_BOOLEAN),
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
END_DATADESC()
