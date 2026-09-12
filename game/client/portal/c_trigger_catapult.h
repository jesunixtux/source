//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Trigger catapult (client). Reconstructed surface for this port;
//          mirror of trigger_catapult.h with C_ naming.
//
//=============================================================================//

#ifndef C_TRIGGER_CATAPULT_H
#define C_TRIGGER_CATAPULT_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
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

class C_TriggerCatapult : public C_BaseEntity, public ITriggerCatapultAutoList
{
	DECLARE_CLASS( C_TriggerCatapult, C_BaseEntity );

public:
	static const CUtlVector< ITriggerCatapultAutoList* >& AutoList( void )
	{
		return ITriggerCatapultAutoList::AutoList();
	}

	void EndTouch( CBaseEntity *pOther ) {}
};

#endif // C_TRIGGER_CATAPULT_H