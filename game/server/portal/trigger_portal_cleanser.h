#ifndef TRIGGER_PORTAL_CLEANSER_H
#define TRIGGER_PORTAL_CLEANSER_H

#include "triggers.h"

class CTriggerPortalCleanser : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerPortalCleanser, CBaseTrigger );
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	bool IsEnabled() const { return true; }
	DECLARE_DATADESC();
	COutputEvent m_OnDissolve, m_OnFizzle, m_OnDissolveBox;
};

#endif
