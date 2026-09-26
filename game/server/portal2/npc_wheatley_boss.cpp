#include "cbase.h"
#include "ai_baseactor.h"
#include "ai_basenpc.h"
#include "props.h"
#include "entityoutput.h"
#include "igamesystem.h"
#include "tier0/icommandline.h"
#include "portal_player.h"

#define	BOSS_MODEL "models/npcs/glados/glados_wheatley_boss.mdl"

class CNPCWheatleyBoss : public CAI_BaseNPC
{
public:
	DECLARE_CLASS(CNPCWheatleyBoss, CAI_BaseNPC);
	DECLARE_DATADESC();

	CNPCWheatleyBoss()
	{
	}

	bool CreateVPhysics(void)
	{
		VPhysicsInitNormal(SOLID_VPHYSICS, 0, false);
		return true;
	}

	void Spawn(void);
	void Precache(void);
	int OnTakeDamage(const CTakeDamageInfo &info);
	void InputHitByFutbol(inputdata_t &inputdata);
	int FutbolHits(void) const { return m_nFutbolHits; }
	bool FutbolDefeated(void) const { return m_bFutbolDefeated; }
private:
	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	COutputEvent m_OnPlayerPickup;
	COutputEvent m_OnPhysGunDrop;
	COutputEvent m_OnFutbolHit;
	int m_nFutbolHits;
	bool m_bFutbolDefeated;

	void RegisterFutbolHit(CBaseEntity *pActivator);
};

LINK_ENTITY_TO_CLASS(npc_wheatley_boss, CNPCWheatleyBoss);

BEGIN_DATADESC(CNPCWheatleyBoss)
	DEFINE_INPUTFUNC(FIELD_VOID, "HitByFutbol", InputHitByFutbol),
	DEFINE_OUTPUT(m_OnFutbolHit, "OnFutbolHit"),
	DEFINE_FIELD(m_nFutbolHits, FIELD_INTEGER),
	DEFINE_FIELD(m_bFutbolDefeated, FIELD_BOOLEAN),
END_DATADESC()

void CNPCWheatleyBoss::RegisterFutbolHit(CBaseEntity *pActivator)
{
	if (m_bFutbolDefeated)
		return;

	++m_nFutbolHits;
	if (CBaseEntity *pCounter = gEntList.FindEntityByName(NULL, "wheatley_hit_count"))
	{
		variant_t increment;
		increment.SetInt(1);
		pCounter->AcceptInput("Add", pActivator ? pActivator : this, this, increment, 0);
	}

	m_OnFutbolHit.FireOutput(pActivator ? pActivator : this, this);
	m_bFutbolDefeated = m_nFutbolHits >= 3;
}

void CNPCWheatleyBoss::Precache(void)
{
	PrecacheModel(BOSS_MODEL);

	BaseClass::Precache();
}

void CNPCWheatleyBoss::Spawn(void)
{
	Precache();
	SetModel(BOSS_MODEL);

	SetSolid(SOLID_VPHYSICS);
	CreateVPhysics();

	BaseClass::Spawn();
	NPCInit();
	SetMaxHealth(10000);
	SetHealth(10000);
	m_takedamage = DAMAGE_YES;
}

int CNPCWheatleyBoss::OnTakeDamage(const CTakeDamageInfo &info)
{
	if (info.GetInflictor() && FClassnameIs(info.GetInflictor(), "prop_exploding_futbol"))
	{
		RegisterFutbolHit(info.GetAttacker());
		return 1;
	}

	return 0;
}

void CNPCWheatleyBoss::InputHitByFutbol(inputdata_t &inputdata)
{
	RegisterFutbolHit(inputdata.pActivator);
}

CON_COMMAND_F(portal2_boss_hit, "Apply a test futbol hit to Wheatley.", FCVAR_CHEAT)
{
	CBaseEntity *pBoss = gEntList.FindEntityByName(NULL, "@sphere");
	if (!pBoss)
		pBoss = gEntList.FindEntityByClassname(NULL, "npc_wheatley_boss");
	if (!pBoss)
	{
		Warning("portal2_boss_hit: Wheatley is not present\n");
		return;
	}

	const int count = clamp(args.ArgC() > 1 ? atoi(args[1]) : 1, 1, 3);
	variant_t empty;
	for (int i = 0; i < count; ++i)
		pBoss->AcceptInput("HitByFutbol", UTIL_GetLocalPlayer(), UTIL_GetLocalPlayer(), empty, 0);
	Msg("PORTAL2_BOSS_TEST applied_hits=%d\n", count);
}

class CPortal2BossGameplayTest : public CAutoGameSystemPerFrame
{
public:
	CPortal2BossGameplayTest() : CAutoGameSystemPerFrame("Portal2BossGameplayTest"), m_start(-1.0f), m_hits(0), m_reported(false) {}

	void LevelInitPreEntity(void) override
	{
		m_start = -1.0f;
		m_hits = 0;
		m_reported = false;
	}

	void FrameUpdatePostEntityThink(void) override
	{
		if (!CommandLine()->FindParm("-portal2_boss_test") || !FStrEq(STRING(gpGlobals->mapname), "sp_a4_finale4") || m_reported)
			return;

		if (m_start < 0.0f)
			m_start = gpGlobals->curtime;

		CNPCWheatleyBoss *pBoss = dynamic_cast<CNPCWheatleyBoss *>(gEntList.FindEntityByName(NULL, "@sphere"));
		if (!pBoss)
		{
			if (gpGlobals->curtime - m_start > 10.0f)
			{
				Warning("PORTAL2_BOSS_TEST FAIL: @sphere was not spawned\n");
				m_reported = true;
			}
			return;
		}

		if (gpGlobals->curtime - m_start < 1.0f + m_hits)
			return;
		if (m_hits >= 3)
		{
			if (pBoss->FutbolHits() == 3)
			{
				Msg("PORTAL2_BOSS_TEST hits=%d defeated=%d\n", pBoss->FutbolHits(), pBoss->FutbolDefeated());
				m_reported = true;
				return;
			}
			if (gpGlobals->curtime - m_start > 10.0f)
			{
				Warning("PORTAL2_BOSS_TEST FAIL: futbol hits=%d\n", pBoss->FutbolHits());
				m_reported = true;
			}
			return;
		}

		const Vector direction(1.0f, 0.0f, 0.0f);
		const Vector origin = pBoss->WorldSpaceCenter() - direction * 128.0f;
		CBaseEntity *pFutbol = CBaseEntity::Create("prop_exploding_futbol", origin, vec3_angle, pBoss);
		if (pFutbol)
			pFutbol->SetAbsVelocity(direction * 850.0f);
		else
			Warning("PORTAL2_BOSS_TEST FAIL: futbol %d was not created\n", m_hits + 1);
		Msg("PORTAL2_BOSS_TEST launch=%d boss_hits=%d\n", m_hits + 1, pBoss->FutbolHits());
		++m_hits;
		if (m_hits == 3 && pBoss->FutbolHits() == 3)
		{
			Msg("PORTAL2_BOSS_TEST hits=%d defeated=%d\n", pBoss->FutbolHits(), pBoss->FutbolDefeated());
			m_reported = true;
		}
		else if (m_hits == 3)
		{
			Msg("PORTAL2_BOSS_TEST waiting_for_physics boss_hits=%d\n", pBoss->FutbolHits());
			return;
		}
	}

private:
	float m_start;
	int m_hits;
	bool m_reported;
};

static CPortal2BossGameplayTest g_Portal2BossGameplayTest;

class CPortal2FinaleLoadout : public CAutoGameSystemPerFrame
{
public:
	CPortal2FinaleLoadout() : CAutoGameSystemPerFrame("Portal2FinaleLoadout"), m_applied(false) {}

	void LevelInitPreEntity(void) override { m_applied = false; }

	void FrameUpdatePostEntityThink(void) override
	{
		if (m_applied || !FStrEq(STRING(gpGlobals->mapname), "sp_a4_finale4"))
			return;

		CPortal_Player *pPlayer = ToPortalPlayer(UTIL_GetLocalPlayer());
		if (!pPlayer || !pPlayer->IsAlive())
			return;

		pPlayer->GivePlayerPortalGun(true, true);
		pPlayer->TurnOnPotatos();
		m_applied = true;
		Msg("PORTAL2_BOSS_LOADOUT portalgun=1 potatos=1\n");
	}

private:
	bool m_applied;
};

static CPortal2FinaleLoadout g_Portal2FinaleLoadout;
