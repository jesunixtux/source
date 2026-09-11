#include "cbase.h"
#include "props.h"

class CPropButtonBase : public CDynamicProp
{
public:
	DECLARE_CLASS(CPropButtonBase, CDynamicProp);
	DECLARE_DATADESC();

	CPropButtonBase()
		: idleSequenceId(-1), downSequenceId(-1), upSequenceId(-1), idleDownSequenceId(-1),
		  m_bPressed(false), m_bLocked(false), m_flDelay(1.0f)
	{
	}

	bool CreateVPhysics()
	{
		VPhysicsInitStatic();
		return true;
	}

	void InputPress(inputdata_t& data);

	virtual void Press(void);
	virtual void Unpress(void);
	void Reset(void);
	bool BeginPress(void);
	bool IsPressed() const { return m_bPressed; }
	bool IsLocked() const { return m_bLocked; }
	void InputLock(inputdata_t&) { m_bLocked=true; }
	void InputUnlock(inputdata_t&) { m_bLocked=false; }

	void CheckSequence(int id);

	void Spawn(void);

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps();

	void ReachedEndOfSequence(void);
protected:
	int idleSequenceId;
	int downSequenceId;
	int upSequenceId;
	int idleDownSequenceId;

	COutputEvent m_OnPressed;
	COutputEvent m_OnButtonReset;
	COutputEvent m_OnPressedBlue;
	COutputEvent m_OnPressedOrange;
	bool m_bPressed, m_bLocked;
	float m_flDelay;
	EHANDLE m_hPressActivator;
};
