#ifndef TRIGGER_PAINT_CLEANSER_H
#define TRIGGER_PAINT_CLEANSER_H

#include "baseentity.h"

// Compatibility surface for the paint blob tracer.  Maps that do not ship
// the original cleanser entity simply behave as if no cleanser was enabled.
class CTriggerPaintCleanser : public CBaseEntity
{
public:
	bool IsEnabled() const { return false; }
};

#endif
