#include "cbase.h"
#include "prop_button_base.h"

//LINK_ENTITY_TO_CLASS(prop_button, CPropButton);

BEGIN_DATADESC(CPropButtonBase)

DEFINE_USEFUNC(Use),
//DEFINE_KEYFIELD(areaPortalName, FIELD_STRING, "AreaPortalWindow"),
DEFINE_INPUTFUNC(FIELD_VOID, "Press", InputPress),
DEFINE_INPUTFUNC(FIELD_VOID, "Lock", InputLock),
DEFINE_INPUTFUNC(FIELD_VOID, "Unlock", InputUnlock),
DEFINE_KEYFIELD(m_flDelay, FIELD_FLOAT, "Delay"),
DEFINE_KEYFIELD(m_bLocked, FIELD_BOOLEAN, "StartLocked"),
DEFINE_FIELD(m_bPressed, FIELD_BOOLEAN),
DEFINE_FIELD(m_hPressActivator, FIELD_EHANDLE),
DEFINE_FIELD(idleSequenceId, FIELD_INTEGER),
DEFINE_FIELD(downSequenceId, FIELD_INTEGER),
DEFINE_FIELD(upSequenceId, FIELD_INTEGER),
DEFINE_FIELD(idleDownSequenceId, FIELD_INTEGER),
DEFINE_THINKFUNC(Reset),

DEFINE_OUTPUT(m_OnPressed, "OnPressed"),
DEFINE_OUTPUT(m_OnButtonReset, "OnButtonReset"),
DEFINE_OUTPUT(m_OnPressedBlue, "OnPressedBlue"),
DEFINE_OUTPUT(m_OnPressedOrange, "OnPressedOrange"),

END_DATADESC()

void CPropButtonBase::CheckSequence(int id)
{
	bool sequenceStatus = PrefetchSequence(id);
	sequenceStatus;
	Assert(id != ACT_INVALID && sequenceStatus);
}

int CPropButtonBase::ObjectCaps()
{
	int caps = BaseClass::ObjectCaps();

	caps |= FCAP_IMPULSE_USE;

	return caps;
}

void CPropButtonBase::InputPress(inputdata_t& data)
{
	m_hPressActivator=data.pActivator;
	Press();
}

void CPropButtonBase::Press()
{
	BeginPress();
}

bool CPropButtonBase::BeginPress()
{
	if(m_bLocked || m_bPressed) return false;
	m_bPressed=true;
	// CDynamicProp does not call ReachedEndOfSequence. Use a separate
	// saved context think so resetting the button cannot replace AnimThink.
	SetContextThink(&CPropButtonBase::Reset, gpGlobals->curtime+MAX(0.1f,m_flDelay), "Portal2ButtonReset");
	m_OnPressed.FireOutput(m_hPressActivator.Get(),this);
	return true;
}

void CPropButtonBase::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if(useType==USE_OFF) return;
	m_hPressActivator=pActivator;
	Press();
}

void CPropButtonBase::Spawn(void)
{
	BaseClass::Spawn();

	SetSolid(SOLID_VPHYSICS);

	SetUse(&CPropButtonBase::Use);

	CreateVPhysics();
	SetPlaybackRate(1.0f);
}

void CPropButtonBase::Unpress(void)
{
}

void CPropButtonBase::Reset(void)
{
	if(!m_bPressed) return;
	m_bPressed=false;
	Unpress();
	m_OnButtonReset.FireOutput(m_hPressActivator.Get(), this);
}

void CPropButtonBase::ReachedEndOfSequence(void)
{
	if (GetSequence() == downSequenceId)
	{
		Unpress();
	}

	if (GetSequence() == upSequenceId)
	{
		Reset();
	}
}
