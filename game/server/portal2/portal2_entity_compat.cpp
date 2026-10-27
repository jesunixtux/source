// Portal 2-only compatibility entities for maps authored after the Orange Box
// entity set.  These deliberately preserve map I/O before visual subsystems
// (Bink movie panels and the instructor HUD) are brought over to the client.
#include "cbase.h"
#include "paint_sprayer.h"
#include "paint_stream.h"
#include "paint_stream_manager.h"
#include "trigger_paint_cleanser.h"
#include "tier0/icommandline.h"

#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( info_paint_sprayer, CPaintSprayer );
LINK_ENTITY_TO_CLASS( paint_stream, CPaintStream );
LINK_ENTITY_TO_CLASS( trigger_paint_cleanser, CTriggerPaintCleanser );

BEGIN_DATADESC( CPaintStream )
    DEFINE_FIELD( m_nPaintType, FIELD_INTEGER ),
    DEFINE_FIELD( m_nRenderMode, FIELD_INTEGER ),
    DEFINE_FIELD( m_nMaxBlobCount, FIELD_INTEGER ),
END_DATADESC()

void CPaintStream::Spawn()
{
    BaseClass::Spawn();
    SetMoveType( MOVETYPE_NONE );
    SetSolid( SOLID_NONE );
}

int CPaintStream::UpdateTransmitState()
{
	// shortcut: keep streams server-only until the matching client stream table exists.
    return SetTransmitState( FL_EDICT_DONTSEND );
}

BEGIN_DATADESC( CPaintSprayer )
    DEFINE_FIELD( m_hPaintStream, FIELD_EHANDLE ),
    DEFINE_KEYFIELD( m_PaintPowerType, FIELD_INTEGER, "PaintType" ),
    DEFINE_KEYFIELD( m_nAmbientSound, FIELD_INTEGER, "AmbientSound" ),
    DEFINE_KEYFIELD( m_nBlobRandomSeed, FIELD_INTEGER, "blob_random_seed" ),
    DEFINE_KEYFIELD( m_nMaxBlobCount, FIELD_INTEGER, "maxblobcount" ),
    DEFINE_KEYFIELD( m_flBlobsPerSecond, FIELD_FLOAT, "blobs_per_second" ),
    DEFINE_KEYFIELD( m_flBlobSpreadRadius, FIELD_FLOAT, "blob_spread_radius" ),
    DEFINE_KEYFIELD( m_flBlobSpreadAngle, FIELD_FLOAT, "blob_spread_angle" ),
    DEFINE_KEYFIELD( m_flMinSpeed, FIELD_FLOAT, "min_speed" ),
    DEFINE_KEYFIELD( m_flMaxSpeed, FIELD_FLOAT, "max_speed" ),
    DEFINE_KEYFIELD( m_flStreakPercentage, FIELD_FLOAT, "blob_streak_percentage" ),
    DEFINE_KEYFIELD( m_flMinStreakTime, FIELD_FLOAT, "min_streak_time" ),
    DEFINE_KEYFIELD( m_flMaxStreakTime, FIELD_FLOAT, "max_streak_time" ),
    DEFINE_KEYFIELD( m_flMinStreakSpeedDampen, FIELD_FLOAT, "min_streak_speed_dampen" ),
    DEFINE_KEYFIELD( m_flMaxStreakSpeedDampen, FIELD_FLOAT, "max_streak_speed_dampen" ),
    DEFINE_KEYFIELD( m_flNoisyBlobPercentage, FIELD_FLOAT, "noisy_blob_percentage" ),
    DEFINE_KEYFIELD( m_bSilent, FIELD_BOOLEAN, "silent" ),
    DEFINE_KEYFIELD( m_bDrawOnly, FIELD_BOOLEAN, "draw_only" ),
    DEFINE_KEYFIELD( m_bStartActive, FIELD_BOOLEAN, "start_active" ),
    DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_flAccumulatedTime, FIELD_FLOAT ),
    DEFINE_FIELD( m_flPercentageSinceLastNoisyBlob, FIELD_FLOAT ),
    DEFINE_THINKFUNC( PaintSprayerThink ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Start", InputStart ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Stop", InputStop ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "ChangePaintType", InputChangePaintType ),
END_DATADESC()

void CPaintSprayer::Spawn()
{
    BaseClass::Spawn();
    m_nMaxBlobCount = MAX( 1, m_nMaxBlobCount );
    m_bPaintProbeLoggedThink = false;

    CBaseEntity *entity = CBaseEntity::Create( "paint_stream", GetAbsOrigin(), GetAbsAngles(), this );
    m_hPaintStream = dynamic_cast<CPaintStream *>( entity );
    if ( !m_hPaintStream )
    {
        Warning( "Portal 2 paint sprayer '%s' could not create paint_stream\n", GetEntityName().ToCStr() );
        if ( entity )
            UTIL_Remove( entity );
        return;
    }

    m_hPaintStream->m_nPaintType = m_PaintPowerType;
    m_hPaintStream->m_nMaxBlobCount = m_nMaxBlobCount;
    if ( m_bStartActive )
    {
        m_bActive = true;
        SetContextThink( &CPaintSprayer::PaintSprayerThink, gpGlobals->curtime + 0.1f,
            "PaintSprayerThink" );
    }
    else
        SetContextThink( NULL, TICK_NEVER_THINK, "PaintSprayerThink" );
}

void CPaintSprayer::PaintSprayerThink()
{
    if ( !m_bPaintProbeLoggedThink && CommandLine()->FindParm( "-portal2_paint_probe" ) )
    {
        Msg( "PORTAL2_PAINT_THINK sprayer=%s active=%d rate=%.1f dt=%.4f stream=%d\n",
            GetEntityName().ToCStr(), m_bActive, m_flBlobsPerSecond, gpGlobals->frametime,
            m_hPaintStream.Get() != NULL );
        m_bPaintProbeLoggedThink = true;
    }
    if ( m_bActive )
        SprayPaint( gpGlobals->frametime );
    SetNextThink( gpGlobals->curtime + 0.01f, "PaintSprayerThink" );
}

void CPaintSprayer::InputStart( inputdata_t & )
{
    if ( CommandLine()->FindParm( "-portal2_paint_probe" ) )
        Msg( "PORTAL2_PAINT_INPUT sprayer=%s input=Start stream=%d\n",
            GetEntityName().ToCStr(), m_hPaintStream.Get() != NULL );
    if ( !m_hPaintStream )
        return;
    m_bActive = true;
    SetContextThink( &CPaintSprayer::PaintSprayerThink, gpGlobals->curtime + 0.1f,
        "PaintSprayerThink" );
}

void CPaintSprayer::InputStop( inputdata_t & )
{
    if ( CommandLine()->FindParm( "-portal2_paint_probe" ) )
        Msg( "PORTAL2_PAINT_INPUT sprayer=%s input=Stop stream=%d seed=%d\n",
            GetEntityName().ToCStr(), m_hPaintStream.Get() != NULL, m_nBlobRandomSeed );
    m_bActive = false;
    SetContextThink( NULL, TICK_NEVER_THINK, "PaintSprayerThink" );
}

void CPaintSprayer::InputChangePaintType( inputdata_t &inputdata )
{
    const int paintType = inputdata.value.Int();
    if ( paintType < 0 || paintType >= PAINT_POWER_TYPE_COUNT )
        return;
    m_PaintPowerType = paintType;
    if ( m_hPaintStream )
        m_hPaintStream->m_nPaintType = paintType;
}

class CPortal2PaintBlobPoolInit : public CAutoGameSystem
{
public:
    CPortal2PaintBlobPoolInit() : CAutoGameSystem( "Portal2PaintBlobPoolInit" ) {}

    void LevelInitPostEntity() override
    {
        int capacity = 0;
        CUtlVector<IPaintStreamAutoList *> &streams = IPaintStreamAutoList::AutoList();
        for ( int i = 0; i < streams.Count(); ++i )
        {
            CPaintStream *stream = static_cast<CPaintStream *>( streams[i] );
            capacity += MAX( 0, stream->m_nMaxBlobCount );
        }
        // shortcut: capacity covers map-authored streams; resize when runtime streams are supported.
        if ( capacity > 0 )
            PaintStreamManager.AllocatePaintBlobPool( capacity );
    }
};
static CPortal2PaintBlobPoolInit g_Portal2PaintBlobPoolInit;

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
LINK_ENTITY_TO_CLASS( info_landmark_entry, CPortal2LandmarkExit );
