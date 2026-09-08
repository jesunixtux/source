// Portal 2-only compatibility entities for maps authored after the Orange Box
// entity set.  These deliberately preserve map I/O before visual subsystems
// (Bink movie panels and the instructor HUD) are brought over to the client.
#include "cbase.h"

#include "tier0/memdbgon.h"

class CPortal2LogicPlayMovie : public CPointEntity
{
public:
	DECLARE_CLASS( CPortal2LogicPlayMovie, CPointEntity );
	DECLARE_DATADESC();

	CPortal2LogicPlayMovie() : m_bFinished( false ) {}

	void Spawn()
	{
		BaseClass::Spawn();
		// The original entity begins its movie when the map becomes active. Bink
		// playback is not available in this branch yet, so finish on the next
		// tick rather than leave the map's OnPlaybackFinished chain blocked.
		SetThink( &CPortal2LogicPlayMovie::FinishMovie );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	void InputPlayMovie( inputdata_t & )
	{
		SetThink( &CPortal2LogicPlayMovie::FinishMovie );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	void FinishMovie()
	{
		if ( m_bFinished )
			return;
		m_bFinished = true;
		Msg( "PORTAL2_COMPAT movie '%s' unavailable; continuing map I/O\n",
			STRING( m_iszMovieFilename ) );
		m_OnPlaybackFinished.FireOutput( this, this );
	}

private:
	string_t m_iszMovieFilename;
	bool m_bFinished;
	COutputEvent m_OnPlaybackFinished;
};

LINK_ENTITY_TO_CLASS( logic_playmovie, CPortal2LogicPlayMovie );

BEGIN_DATADESC( CPortal2LogicPlayMovie )
	DEFINE_KEYFIELD( m_iszMovieFilename, FIELD_STRING, "MovieFilename" ),
	DEFINE_FIELD( m_bFinished, FIELD_BOOLEAN ),
	DEFINE_OUTPUT( m_OnPlaybackFinished, "OnPlaybackFinished" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "PlayMovie", InputPlayMovie ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Start", InputPlayMovie ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Skip", InputPlayMovie ),
END_DATADESC()

class CPortal2InstructorHint : public CPointEntity
{
public:
	DECLARE_CLASS( CPortal2InstructorHint, CPointEntity );
	DECLARE_DATADESC();

	CPortal2InstructorHint() : m_bEnabled( true ) {}

	void InputShowHint( inputdata_t & )
	{
		if ( m_bEnabled && m_iszCaption != NULL_STRING )
			UTIL_ClientPrintAll( HUD_PRINTTALK, STRING( m_iszCaption ) );
	}
	void InputEndHint( inputdata_t & ) {}
	void InputEnable( inputdata_t & ) { m_bEnabled = true; }
	void InputDisable( inputdata_t & ) { m_bEnabled = false; }

private:
	string_t m_iszCaption;
	bool m_bEnabled;
};

LINK_ENTITY_TO_CLASS( env_instructor_hint, CPortal2InstructorHint );

BEGIN_DATADESC( CPortal2InstructorHint )
	DEFINE_KEYFIELD( m_iszCaption, FIELD_STRING, "hint_caption" ),
	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ShowHint", InputShowHint ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EndHint", InputEndHint ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

// These are server-side anchors for purely client-facing media/event systems.
// Keeping their names and inputs valid allows Portal 2 map I/O to continue;
// a later client implementation can replace their presentation.
class CPortal2MovieDisplay : public CPointEntity {};
class CPortal2GameEventProxy : public CPointEntity {};
class CPortal2LandmarkExit : public CPointEntity {};

LINK_ENTITY_TO_CLASS( vgui_movie_display, CPortal2MovieDisplay );
LINK_ENTITY_TO_CLASS( info_game_event_proxy, CPortal2GameEventProxy );
LINK_ENTITY_TO_CLASS( info_landmark_exit, CPortal2LandmarkExit );
