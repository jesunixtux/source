#ifndef GAME_TIMESCALE_SHARED_H
#define GAME_TIMESCALE_SHARED_H

// Portal 2 uses a managed timescale simulation group (private engine system).
// This port defines the minimal API surface the game code references.

#include "mathlib/mathlib.h"

class CTimeScaleGroup
{
public:
	CTimeScaleGroup() : m_flCurrentTimescale( 1.0f ), m_flDesiredTimescale( 1.0f ) {}

	void SetCurrentTimescale( float flTimescale ) { m_flCurrentTimescale = flTimescale; }
	void SetDesiredTimescale( float flTimescale ) { m_flDesiredTimescale = flTimescale; m_flCurrentTimescale = flTimescale; }
	float GetCurrentTimescale( void ) const { return m_flCurrentTimescale; }

private:
	float m_flCurrentTimescale;
	float m_flDesiredTimescale;
};

CTimeScaleGroup & GameTimescale( void )
{
	static CTimeScaleGroup s_TimescaleGroup;
	return s_TimescaleGroup;
}

#endif // GAME_TIMESCALE_SHARED_H