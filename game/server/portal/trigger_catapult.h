//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Trigger catapult (server). Reconstructed surface for this port;
//          no entity .cpp is registered, so only the interface used by
//          portal_player.cpp / portal_player_shared.cpp is kept.
//
//=============================================================================//

#ifndef TRIGGER_CATAPULT_H
#define TRIGGER_CATAPULT_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "util_shared.h"

abstract_class ITriggerCatapultAutoList
{
public:
	static const CUtlVector< ITriggerCatapultAutoList* >& AutoList( void )
	{
		static CUtlVector< ITriggerCatapultAutoList* > s_AutoList;
		return s_AutoList;
	}
};

class CTriggerCatapult : public CBaseEntity, public ITriggerCatapultAutoList
{
	DECLARE_CLASS( CTriggerCatapult, CBaseEntity );

public:
	static const CUtlVector< ITriggerCatapultAutoList* >& AutoList( void )
	{
		return ITriggerCatapultAutoList::AutoList();
	}

	void EndTouch( CBaseEntity *pOther ) {}
};

#endif // TRIGGER_CATAPULT_H