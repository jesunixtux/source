// Physical moving-frame adapter for the opening container ride. Cosmetic
// acceleration/skew from Portal 2's renderer is not simulated here.
#include "cbase.h"
#include "tier0/memdbgon.h"

class CPortal2ViewProxy : public CPointEntity
{
public:
    DECLARE_CLASS( CPortal2ViewProxy, CPointEntity );
    DECLARE_DATADESC();
    CPortal2ViewProxy() : m_bEnabled( false ) {}
    void InputEnable( inputdata_t &data )
    {
        m_hPlayer = ToBasePlayer( data.pActivator );
        if ( !m_hPlayer ) m_hPlayer = UTIL_GetLocalPlayer();
        m_hProxy = gEntList.FindEntityByName( NULL, m_iszProxy );
        if ( !m_hPlayer || !m_hProxy )
        {
            Warning( "Portal2 viewproxy %s: missing player/proxy\n", GetDebugName() );
            return;
        }
        m_previous = m_hProxy->EntityToWorldTransform();
        m_bEnabled = true;
        SetThink( &CPortal2ViewProxy::Follow );
        SetNextThink( gpGlobals->curtime );
    }
    void InputDisable( inputdata_t &data )
    {
        m_bEnabled = false;
        SetThink( NULL );
    }
    void InputTeleport( inputdata_t &data )
    {
        CBasePlayer *player = ToBasePlayer( m_hPlayer.Get() );
        if ( !player ) player = UTIL_GetLocalPlayer();
        CBaseEntity *proxy = m_hProxy.Get();
        if ( !proxy ) proxy = gEntList.FindEntityByName( NULL, m_iszProxy );
        if ( !player || !proxy ) return;
        const Vector origin = proxy->GetAbsOrigin() - player->GetViewOffset();
        const QAngle angles = proxy->GetAbsAngles();
        player->Teleport( &origin, &angles, &vec3_origin );
        InputDisable( data );
    }
    void Follow()
    {
        CBasePlayer *player = ToBasePlayer( m_hPlayer.Get() );
        if ( !m_bEnabled || !player || !m_hProxy ) return;
        const matrix3x4_t current = m_hProxy->EntityToWorldTransform();
        matrix3x4_t inverse, delta;
        MatrixInvert( m_previous, inverse );
        ConcatTransforms( current, inverse, delta );
        Vector origin;
        VectorTransform( player->GetAbsOrigin(), delta, origin );
        player->SetAbsOrigin( origin );
        // Keep manual look input, adding only the platform's orientation delta.
        QAngle angles = TransformAnglesToWorldSpace( player->EyeAngles(), delta );
        player->SnapEyeAngles( angles );
        m_previous = current;
        SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
    }
private:
    string_t m_iszProxy;
    EHANDLE m_hPlayer, m_hProxy;
    matrix3x4_t m_previous;
    bool m_bEnabled;
};
LINK_ENTITY_TO_CLASS( point_viewproxy, CPortal2ViewProxy );
BEGIN_DATADESC( CPortal2ViewProxy )
    DEFINE_KEYFIELD( m_iszProxy, FIELD_STRING, "proxy" ),
    DEFINE_FIELD( m_hPlayer, FIELD_EHANDLE ),
    DEFINE_FIELD( m_hProxy, FIELD_EHANDLE ),
    DEFINE_FIELD( m_previous, FIELD_MATRIX3X4_WORLDSPACE ),
    DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),
    DEFINE_THINKFUNC( Follow ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "TeleportPlayerToProxy", InputTeleport ),
END_DATADESC()
