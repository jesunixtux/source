//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Portal 2 research data tracker. Ablated from this port; kept as
//          a no-op stub so the telemetry hook surface in CPortal_Player
//          compiles without the private tracker implementation.
//
//=============================================================================//

#ifndef PORTAL2_RESEARCH_DATA_TRACKER_H
#define PORTAL2_RESEARCH_DATA_TRACKER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

class CPortal2ResearchDataTracker
{
public:
	void SetPlayerName( CBasePlayer *pPlayer ) {}
	void Event_PlayerGaveUp( void ) {}
	void IncrementDeath( CBasePlayer *pPlayer ) {}
	void IncrementPortalFired( CBasePlayer *pPlayer ) {}
	void IncrementStepsTaken( CBasePlayer *pPlayer ) {}
};

static CPortal2ResearchDataTracker g_Portal2ResearchDataTracker;

#endif // PORTAL2_RESEARCH_DATA_TRACKER_H