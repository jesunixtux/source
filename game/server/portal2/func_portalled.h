//===== Copyright 1996-2006, Valve Corporation, All rights reserved. ======//
//
// Purpose: Entity that forwards notifications of teleports (through portals
//          touching its volume) to map logic entities.
//
// Reconstructed from private Portal 2 source; minimal API used by CProp_Portal.
//
// $NoKeywords: $
//===========================================================================//

#ifndef FUNC_PORTALLED_H
#define FUNC_PORTALLED_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "entityoutput.h"

class CProp_Portal;

class CFunc_Portalled : public CBaseEntity
{
	DECLARE_CLASS( CFunc_Portalled, CBaseEntity );
	DECLARE_DATADESC();

public:
	CFunc_Portalled( void );

	virtual void Spawn( void );
	virtual void Activate( void );

	// Used by CProp_Portal to decide whether this detector should be notified.
	virtual bool IsPortalTouchingDetector( CBaseEntity *pPortal );

	// Teleport notification callbacks issued by CProp_Portal.
	virtual void OnPrePortalled( CBaseEntity *pEntity, bool bRestorePortal );
	virtual void OnPostPortalled( CBaseEntity *pEntity, bool bRestorePortal );

private:
	COutputEvent m_OnPrePortalled;
	COutputEvent m_OnPostPortalled;
	COutputInt m_OnPortalNoLongerTouching;
};

#endif // FUNC_PORTALLED_H