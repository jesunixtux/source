//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		NO_STEAM stand-in for the Portal achievements helper.
//
//				UTIL_RecordAchievementEvent is a Portal-specific helper that
//				routes achievement fire events into the Steam achievement
//				system. This port is built with NO_STEAM=1, so the calls are
//				compiled out instead of shipped to Steam.
//
//=============================================================================//

#ifndef ACHIEVEMENT_STUBS_H
#define ACHIEVEMENT_STUBS_H
#pragma once

#include "baseentity.h"

inline void UTIL_RecordAchievementEvent( const char *szAchievementName, CBaseEntity *pActivator )
{
	// Achievements are disabled in the NO_STEAM build.
}

#endif // ACHIEVEMENT_STUBS_H