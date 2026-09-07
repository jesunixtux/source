// Portal 2 map I/O adapters, compiled only by the Portal 2 game target.
// The instance proxy contract follows the matching local Portal-2 source:
// OnProxyRelay1..16 are both input and output names, with the proxy as caller
// and activator. This preserves instance outputs already compiled into BSPs.
#include "cbase.h"

#include "tier0/memdbgon.h"

#define PORTAL2_PROXY_CHANNELS( MACRO ) \
    MACRO( 1 ) MACRO( 2 ) MACRO( 3 ) MACRO( 4 ) \
    MACRO( 5 ) MACRO( 6 ) MACRO( 7 ) MACRO( 8 ) \
    MACRO( 9 ) MACRO( 10 ) MACRO( 11 ) MACRO( 12 ) \
    MACRO( 13 ) MACRO( 14 ) MACRO( 15 ) MACRO( 16 )

class CPortal2InstanceIOProxy : public CPointEntity
{
public:
    DECLARE_CLASS( CPortal2InstanceIOProxy, CPointEntity );
    DECLARE_DATADESC();

#define PORTAL2_DECLARE_PROXY( N ) \
    void InputProxyRelay##N( inputdata_t &inputdata ) \
    { m_OnProxyRelay[N - 1].FireOutput( this, this ); }
    PORTAL2_PROXY_CHANNELS( PORTAL2_DECLARE_PROXY )
#undef PORTAL2_DECLARE_PROXY

private:
    COutputEvent m_OnProxyRelay[16];
};

LINK_ENTITY_TO_CLASS( func_instance_io_proxy, CPortal2InstanceIOProxy );

BEGIN_DATADESC( CPortal2InstanceIOProxy )
#define PORTAL2_DESCRIBE_PROXY( N ) \
    DEFINE_INPUTFUNC( FIELD_STRING, "OnProxyRelay" #N, InputProxyRelay##N ), \
    DEFINE_OUTPUT( m_OnProxyRelay[N - 1], "OnProxyRelay" #N ),
    PORTAL2_PROXY_CHANNELS( PORTAL2_DESCRIBE_PROXY )
#undef PORTAL2_DESCRIBE_PROXY
END_DATADESC()

#undef PORTAL2_PROXY_CHANNELS

// Portal 2's transition script sends the destination as the ChangeLevel input
// parameter, unlike the old trigger_changelevel's map key. Use the engine's
// validated, queued level-change API. Landmark-relative campaign persistence
// remains a separate feature; this adapter starts the destination normally.
class CPortal2PointChangeLevel : public CPointEntity
{
public:
    DECLARE_CLASS( CPortal2PointChangeLevel, CPointEntity );
    DECLARE_DATADESC();

    CPortal2PointChangeLevel() : m_bChangeQueued( false ) {}

    void InputChangeLevel( inputdata_t &inputdata )
    {
        const char *pszMap = inputdata.value.String();
        if ( !pszMap || !pszMap[0] || !engine->IsMapValid( pszMap ) )
        {
            Warning( "point_changelevel %s: invalid destination '%s'\n",
                GetDebugName(), pszMap ? pszMap : "" );
            return;
        }
        if ( m_bChangeQueued )
            return;

        m_bChangeQueued = true;
        Msg( "Portal2 level transition: %s -> %s\n", STRING( gpGlobals->mapname ), pszMap );
        engine->ChangeLevel( pszMap, NULL );
    }

private:
    bool m_bChangeQueued;
};

LINK_ENTITY_TO_CLASS( point_changelevel, CPortal2PointChangeLevel );

BEGIN_DATADESC( CPortal2PointChangeLevel )
    DEFINE_INPUTFUNC( FIELD_STRING, "ChangeLevel", InputChangeLevel ),
    DEFINE_FIELD( m_bChangeQueued, FIELD_BOOLEAN ),
END_DATADESC()
