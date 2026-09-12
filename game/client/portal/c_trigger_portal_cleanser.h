#ifndef C_TRIGGER_PORTAL_CLEANSER_H
#define C_TRIGGER_PORTAL_CLEANSER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"

class C_TriggerPortalCleanser : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TriggerPortalCleanser, C_BaseEntity );

	C_TriggerPortalCleanser( void ) {}

	// Stub: treat all fizzler triggers as enabled on the client.
	// The real class would network m_bDisabled from the server.
	bool IsEnabled( void ) const { return true; }
};

#define CTriggerPortalCleanser C_TriggerPortalCleanser

#endif // C_TRIGGER_PORTAL_CLEANSER_H
