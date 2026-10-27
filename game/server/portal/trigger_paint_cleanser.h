#ifndef TRIGGER_PAINT_CLEANSER_H
#define TRIGGER_PAINT_CLEANSER_H

#include "triggers.h"

class CTriggerPaintCleanser : public CBaseTrigger
{
    DECLARE_CLASS( CTriggerPaintCleanser, CBaseTrigger );

public:
    void Spawn() override
    {
        BaseClass::Spawn();
        InitTrigger();
    }

    bool IsEnabled() const { return !m_bDisabled; }
};

#endif
