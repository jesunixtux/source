//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Portal 2 multiplayer statistics. Ablated from the port: only the
//          surface consumed by portal_player.cpp is kept (no-op hooks).
//
//=============================================================================//

#ifndef PORTAL_MP_STATS_H
#define PORTAL_MP_STATS_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "portal_mp_gamerules.h"

class CBasePlayer;

class CPortalMPStats
{
public:
	static CPortalMPStats *GetPortalMPStats( void )
	{
		static CPortalMPStats instance;
		return &instance;
	}

	void IncrementPlayerTauntsUsedMap( CBasePlayer *pPlayer, Coop_Taunts taunt ) {}
	void SetStats( int a, int b, int c, int d ) {}
	void IncrementPlayerDeathsMap( CBasePlayer *pPlayer ) {}
	void IncrementPlayerPortalsTraveled( CBasePlayer *pPlayer ) {}
	void IncrementPlayerPortals( CBasePlayer *pPlayer ) {}
	void IncrementPlayerSteps( CBasePlayer *pPlayer ) {}
	void SaveStats( CBasePlayer *pPlayer ) {}
	void SavePerMapStats( CBasePlayer *pPlayer, const char *pchName ) {}
	void TeamTauntSuccess( const char *szTaunt ) {}
};

inline CPortalMPStats *GetPortalMPStats( void )
{
	return CPortalMPStats::GetPortalMPStats();
}

#endif // PORTAL_MP_STATS_H
