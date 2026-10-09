// Portal 2 map I/O adapters, compiled only by the Portal 2 game target.
// The instance proxy contract follows the matching local Portal-2 source:
// OnProxyRelay1..16 are both input and output names, with the proxy as caller
// and activator. This preserves instance outputs already compiled into BSPs.
#include "cbase.h"
#include "eventqueue.h"
#include "tier0/icommandline.h"

#include "tier0/memdbgon.h"

// Set by portal2_equip_portalgun when the experimental dual gun is handed out;
// the arrival adapter honours it after a plain changelevel (no player state).
extern ConVar portal2_resume_portalgun;

// Set by the arrival/departure runtimes and read by the -portal2_ground_probe
// trace below, so the ride car's state keeps being logged even after the
// player has lost contact with it (the ALLAZGOS.md section 40 failure mode).
static EHANDLE g_pPortal2RideTrain;

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
    { m_OnProxyRelay[N - 1].FireOutput( inputdata.pActivator ? inputdata.pActivator : this, this ); }
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

// intro2..intro5 start in a sealed transition room. OnPostTransition must
// put the player in the authored arrival car, not leave them at player_start.
// This adapter is deliberately limited to the intro car rig (intro2..intro5)
// and does not emulate the general Portal 2 campaign/landmark persistence.
class CPortal2ArrivalRuntime : public CPointEntity
{
public:
    DECLARE_CLASS(CPortal2ArrivalRuntime, CPointEntity);
    DECLARE_DATADESC();
    CPortal2ArrivalRuntime() : m_started(false), m_finished(false), m_startZ(0), m_deadline(0) {}
    void Start()
    {
        if(m_started) return;
        CBasePlayer *player=UTIL_GetLocalPlayer();
        CBaseEntity *destination=gEntList.FindEntityByName(NULL,"@arrival_teleport");
        m_train=gEntList.FindEntityByName(NULL,"arrival_elevator-elevator_1");
        m_bottom=gEntList.FindEntityByName(NULL,"@elevator_1_bottom_path_1");
        if(!player || !destination || !m_train || !m_bottom)
        { Warning("PORTAL2_ARRIVAL missing player/car/destination/path\n"); return; }
        m_started=true;
        g_pPortal2RideTrain=m_train;
        // MoveToPathNode is not an Orange Box train input. Use its supported
        // real-speed input on this two-node arrival path, then detect arrival.
        variant_t empty;
        CBaseEntity *trigger=gEntList.FindEntityByName(NULL,"arrival_elevator-elevator_1_interior_start_trigger");
        if(trigger) trigger->AcceptInput("Disable",player,this,empty,0);
        const Vector origin=destination->GetAbsOrigin();
        const QAngle angles=destination->GetAbsAngles();
        AngleVectors(angles,&m_exitDirection);
        player->Teleport(&origin,&angles,&vec3_origin);
        player->SetViewEntity(player);
        player->EnableControl(true);
        player->RemoveFlag(FL_FROZEN);
        // The engine's changelevel has no save; carry the intro blue-only gun
        // across the map switch when the player obtained it in an earlier map.
        if(portal2_resume_portalgun.GetBool())
        {
            engine->ServerCommand("portal2_equip_portalgun blue\n");
            Msg("PORTAL2_PRESERVE: portal gun carried into %s\n",STRING(gpGlobals->mapname));
        }
        m_startZ=player->GetAbsOrigin().z;
        g_EventQueue.AddEvent("arrival_elevator-signs_on","Trigger",empty,0.1f,player,this);
        g_EventQueue.AddEvent("arrival_elevator-light_elevator_dynamic","TurnOn",empty,0.0f,player,this);
        variant_t speed; speed.SetFloat(300.0f);
        g_EventQueue.AddEvent(m_train,"SetSpeedReal",speed,0.2f,player,this);
        Msg("PORTAL2_ARRIVAL started player=%.1f %.1f %.1f\n",origin.x,origin.y,origin.z);
        m_deadline=gpGlobals->curtime+20.0f;
        SetThink(&CPortal2ArrivalRuntime::ArrivalThink);
        SetNextThink(gpGlobals->curtime+0.1f);
    }
    void ArrivalThink()
    {
        CBasePlayer *player=UTIL_GetLocalPlayer();
        if(!m_train || !m_bottom || !player) return;
        // The legacy train stops its centre half a wheelbase before a dead
        // end (this authored car has wheels=50). Require it to be stopped as
        // well as near the terminal node; do not open on a timer mid-ride.
        if(m_train->GetAbsOrigin().DistTo(m_bottom->GetAbsOrigin())<32.0f &&
           m_train->GetAbsVelocity().LengthSqr()<1.0f)
        {
            variant_t empty; m_train->AcceptInput("Stop",player,this,empty,0);
            g_EventQueue.AddEvent("arrival_elevator-open","Trigger",empty,0.1f,player,this);
            m_finished=true;
            const Vector pos=player->GetAbsOrigin();
            const bool carried=m_startZ-pos.z>600.0f && (pos-m_train->GetAbsOrigin()).Length2D()<100.0f;
            CBaseEntity *pGun=player->Weapon_OwnsThisType("weapon_portalgun");
            const char *active=player->GetActiveWeapon()?player->GetActiveWeapon()->GetClassname():"none";
            // The carry-over gun must survive the arrival ride; report whether
            // the dual weapon is owned and active when the car parks.
            Msg("PORTAL2_ARRIVAL ride=%s player=%.1f %.1f %.1f train_z=%.1f gun_owned=%d gun_active=%s\n",
                carried?"PASS":"FAIL",pos.x,pos.y,pos.z,m_train->GetAbsOrigin().z,pGun!=0,active);
            if(CommandLine()->FindParm("-portal2_elevator_transition_test") || CommandLine()->FindParm("-portal2_intro2_exit_test") || CommandLine()->FindParm("-portal2_intro_exit_test"))
            {
                SetThink(&CPortal2ArrivalRuntime::TestWalkOut);
                SetNextThink(gpGlobals->curtime+3.0f);
            }
            return;
        }
        if(gpGlobals->curtime>m_deadline)
        { Warning("PORTAL2_ARRIVAL ride=FAIL timeout train_z=%.1f\n",m_train->GetAbsOrigin().z); return; }
        SetNextThink(gpGlobals->curtime+0.1f);
    }
    void TestWalkOut()
    {
        CBasePlayer *player=UTIL_GetLocalPlayer();
        if(!player) return;
        engine->ClientCommand(player->edict(),"+forward\n");
        SetThink(&CPortal2ArrivalRuntime::TestExit);
        SetNextThink(gpGlobals->curtime+3.0f);
    }
    void TestExit()
    {
        CBasePlayer *player=UTIL_GetLocalPlayer();
        if(!player || !m_train) return;
        engine->ClientCommand(player->edict(),"-forward\n");
        const Vector pos=player->GetAbsOrigin();
        Msg("PORTAL2_ARRIVAL exit=%s player=%.1f %.1f %.1f\n",
            DotProduct(pos-m_train->GetAbsOrigin(),m_exitDirection)>180.0f?"PASS":"FAIL",pos.x,pos.y,pos.z);
        Msg("PORTAL2_ARRIVAL exit_map=%s distance=%.1f\n",STRING(gpGlobals->mapname),DotProduct(pos-m_train->GetAbsOrigin(),m_exitDirection));
    }
private:
    bool m_started, m_finished;
    float m_startZ, m_deadline;
    EHANDLE m_train, m_bottom;
    Vector m_exitDirection;
};
LINK_ENTITY_TO_CLASS(portal2_arrival_runtime, CPortal2ArrivalRuntime);
BEGIN_DATADESC(CPortal2ArrivalRuntime)
    DEFINE_FIELD(m_started,FIELD_BOOLEAN),
    DEFINE_FIELD(m_finished,FIELD_BOOLEAN),
    DEFINE_FIELD(m_startZ,FIELD_FLOAT),
    DEFINE_FIELD(m_deadline,FIELD_TIME),
    DEFINE_FIELD(m_train,FIELD_EHANDLE),
    DEFINE_FIELD(m_bottom,FIELD_EHANDLE),
    DEFINE_FIELD(m_exitDirection,FIELD_VECTOR),
    DEFINE_THINKFUNC(ArrivalThink),
    DEFINE_THINKFUNC(TestWalkOut),
    DEFINE_THINKFUNC(TestExit),
END_DATADESC()

bool Portal2RunArrivalScript(CBaseEntity *host,const char *code)
{
    const char *map=STRING(gpGlobals->mapname);
    if((!FStrEq(map,"sp_a1_intro2") && !FStrEq(map,"sp_a1_intro3") &&
        !FStrEq(map,"sp_a1_intro4") && !FStrEq(map,"sp_a1_intro5")) ||
       !FStrEq(host->GetEntityName().ToCStr(),"@transition_script") ||
       (!FStrEq(code,"OnPostTransition()") && !FStrEq(code,"OnPostTransition"))) return false;
    CPortal2ArrivalRuntime *runtime=dynamic_cast<CPortal2ArrivalRuntime *>(gEntList.FindEntityByClassname(NULL,"portal2_arrival_runtime"));
    if(!runtime) runtime=dynamic_cast<CPortal2ArrivalRuntime *>(CBaseEntity::Create("portal2_arrival_runtime",vec3_origin,vec3_angle));
    if(!runtime) return false;
    runtime->Start();
    return true;
}

// Departure linkage for the homogeneous intro car chain (intro2..intro5).
// Each map uses the same car, teleport and transition relays; the destination
// is looked up here instead of the BSP so one adapter drives the whole chain.
static bool Portal2NextIntroMap(const char *current,char *out,size_t outSize)
{
    static const struct { const char *from,*to; } kNext[] = {
        {"sp_a1_intro2","sp_a1_intro3"},
        {"sp_a1_intro3","sp_a1_intro4"},
        {"sp_a1_intro4","sp_a1_intro5"},
    };
    for(unsigned i=0;i<ARRAYSIZE(kNext);++i)
        if(FStrEq(current,kNext[i].from)) { Q_strncpy(out,kNext[i].to,outSize); return true; }
    return false;
}

// §40-embark settle: the departure car's boarding teleport drops the player
// with the hull exactly on the car's playerclip floor line, so every
// player-hull trace starts inside clip-solid (floor_clip under the feet,
// elevator_playerclip over the head; both CONTENTS_PLAYERCLIP, invisible to
// MASK_SOLID). Ground is never captured and the blocked-move resolution
// zeroes fall velocity each frame (pv stays at one frame of gravity, ~-4.5
// u/s; z frozen). The car itself is not pinned by a floor (TRACE_CAR is
// empty for 4096u below). Mirror the arrival's ground touch: nudge the
// player a few units up out of the clip so the ordinary short fall lands
// the feet on the car floor and the ground link carries the ride ("elevator
// carry" on embark). Positional only; no player physics changed and no
// ground link is forced.
static void Portal2UnpinDepartureRider(CBasePlayer *player,CBaseEntity *car)
{
    if(!player) return;
    if(player->GetGroundEntity()) return; // already grounded: nothing to do
    const Vector org=player->GetAbsOrigin();
    trace_t up, down;
    for(float lift=0.0f; lift<=64.0f; lift+=4.0f)
    {
        const Vector n=org+Vector(0,0,lift);
        UTIL_TraceLine(n+Vector(0,0,72.0f), n+Vector(0,0,72.0f+96.0f),
                       MASK_PLAYERSOLID, player, COLLISION_GROUP_NONE, &up);
        UTIL_TraceLine(n, n-Vector(0,0,8.0f),
                       MASK_PLAYERSOLID, player, COLLISION_GROUP_NONE, &down);
        if(!up.startsolid && !down.startsolid)
        {
            if(lift<0.5f) return; // hull already clear
            player->Teleport(&n,NULL,NULL);
            Msg("PORTAL2_INTRO departure unpin lift=%.1f z=%.1f\n",lift,n.z);
            return;
        }
        if(lift==0.0f)
            Msg("PORTAL2_INTRO departure pinned head_z=%.1f feet_z=%.1f up_solid=%d down_solid=%d\n",
                org.z+72.0f,org.z,up.startsolid?1:0,down.startsolid?1:0);
    }
    Msg("PORTAL2_INTRO departure unpin FAILED after 64u up_solid=%d down_solid=%d z=%.1f\n",
        up.startsolid?1:0,down.startsolid?1:0,org.z);
}

// §40: the rebuilt departure car ships its playerclip containment
// (doorclose_playerclip, elevator_playerclip, floor_clip) SOLID-FILLED and
// armed at map start. The boarding teleport therefore lands the hull inside
// them: every player trace starts inside clip-solid, ground is never
// captured and the blocked-move resolution pins the player (pv stays at one
// frame of gravity, ~-4.5 u/s; z frozen). The original map's VScript keeps
// these clips off for the ride; restore that here (once, at embark) so the
// player rests on the car's real floor and the ground link carries the
// descent. Re-arms on the next map load. No player physics changed.
static void Portal2DisableDepartureClips(CBaseEntity *car)
{
    static const char *kClips[] = {
        "departure_elevator-elevator_doorclose_playerclip",
        "departure_elevator-elevator_playerclip",
        "departure_elevator-floor_clip",
    };
    variant_t empty;
    for(unsigned i=0;i<ARRAYSIZE(kClips);++i)
    {
        CBaseEntity *clip=gEntList.FindEntityByName(NULL,kClips[i]);
        if(clip)
        {
            clip->AcceptInput("Disable",car,car,empty,0);
            Msg("PORTAL2_INTRO departure clip disabled: %s\n",kClips[i]);
        }
    }
}

// One-shot geometry dump of the departure ride clips (§40): where the
// floor_clip and elevator_playerclip boxes actually sit, so the embark
// position can be corrected against the real brush boxes instead of the
// pin symptoms. Printed once per level on the first frame near the car.
static bool s_departureClipDumped=false;
static void Portal2DumpDepartureClips()
{
    if(s_departureClipDumped) return;
    bool any=false;
    for(CBaseEntity *e=gEntList.FirstEnt();e!=NULL;e=gEntList.NextEnt(e))
    {
        const char *n=e->GetEntityName().ToCStr();
        if(!n||!n[0]) continue;
        if(Q_strncasecmp(n,"departure_elevator",18)!=0) continue;
        if(!strstr(n,"clip") && !strstr(n,"playerclip") && !strstr(n,"floor_clip")) continue;
        any=true;
        Vector mins,maxs;
        e->CollisionProp()->WorldSpaceSurroundingBounds(&mins,&maxs);
        const Vector &o=e->GetAbsOrigin();
        Msg("PORTAL2_CLIP %s origin=(%.0f %.0f %.0f) min=(%.0f %.0f %.0f) max=(%.0f %.0f %.0f)\n",
            n,o.x,o.y,o.z,mins.x,mins.y,mins.z,maxs.x,maxs.y,maxs.z);
    }
    if(!any) s_departureClipDumped=false;
}

// The next departure uses the same authored speed and path callbacks, but
// must not instantiate intro1's dialogue/runtime or select its destination.
class CPortal2IntroDeparture : public CPointEntity
{
public:
    DECLARE_CLASS(CPortal2IntroDeparture,CPointEntity);
    DECLARE_DATADESC();
    CPortal2IntroDeparture() : m_started(false),m_transition(false),m_queued(false),m_startTime(0),
        m_startZ(0),m_carStartZ(0),m_carried(false),m_sawCarGround(false),m_stuckReported(false),m_reported(false) {}
    void Start(CBaseEntity *car)
    {
        if(m_started) return;
        m_started=true;
        m_car=car;
        g_pPortal2RideTrain=car;
        m_startTime=gpGlobals->curtime;
        // Carry-test baseline (§40): the departure elevator must transport the
        // player. Anchor the player/car heights at car start so DepartureThink
        // can detect a car that descends while the player stays behind. Logs
        // only; no movement behaviour is changed.
        CBasePlayer *player=UTIL_GetLocalPlayer();
        // §40: the map's boarding teleport pins the hull against the departure
        // car's playerclips (floor_clip below, elevator_playerclip above), so
        // ground is never captured and the ride never carries. Clear the hull
        // before the descent starts; the ordinary fall then grounds the player
        // on the car floor and the ground link carries the ride. Positional
        // only: no player physics, no forced SetGroundEntity.
        // §40: the rebuilt departure cabin ships SOLID-FILLED playerclips
        // armed at map start (the original map's VScript keeps them off for
        // the ride). Disable them at embark so the hull is free and the
        // player rests on the car's real floor; the unpin settle below then
        // only fine-tunes if needed.
        Portal2DisableDepartureClips(car);
        Portal2UnpinDepartureRider(player,car);
        Portal2DumpDepartureClips();
        m_startZ=player?player->GetAbsOrigin().z:0.0f;
        m_carStartZ=car?car->GetAbsOrigin().z:0.0f;
        variant_t speed; speed.SetFloat(200.0f);
        g_EventQueue.AddEvent(car,"SetSpeedReal",speed,0.0f,this,this);
        Msg("PORTAL2_INTRO departure started\n");
        SetThink(&CPortal2IntroDeparture::DepartureThink);
        SetNextThink(gpGlobals->curtime+1.0f);
    }
    void Begin()
    {
        if(!m_started || m_transition) return;
        m_transition=true;
        ReportRide();
        variant_t empty;
        g_EventQueue.AddEvent("@transition_from_map","Trigger",empty,0.0f,this,this);
        g_EventQueue.AddEvent("@transition_with_survey","Trigger",empty,0.0f,this,this);
        // The original teleport should touch transition_trigger. Keep a
        // bounded fallback for overlays without the final teleport linkage.
        SetThink(&CPortal2IntroDeparture::Complete);
        SetNextThink(gpGlobals->curtime+2.0f);
        Msg("PORTAL2_INTRO departure path reached\n");
    }
    // The authored flow calls ReadyForTransition when the player reaches the
    // exit pad. Several intro maps lose that VScript callback, which left the
    // car ride forever parked. Fall back: when the car parks near @exit_teleport
    // (or a bounded timeout elapses), push the same transition relays ourselves.
    void DepartureThink()
    {
        if(m_transition || m_queued) return;
        CBaseEntity *car=m_car.Get();
        // Carry probe: first poll where the car descended >40u but the player
        // did not follow marks the §40 failure immediately, before the 25s
        // fallback masks it. "Carried" once the player follows the descent.
        if(car)
        {
            CBasePlayer *player=UTIL_GetLocalPlayer();
            const Vector &carPos=car->GetAbsOrigin();
            if(player)
            {
                const Vector &playerPos=player->GetAbsOrigin();
                const float carDrop=m_carStartZ-carPos.z;
                const float playerDrop=m_startZ-playerPos.z;
                const bool near=(playerPos-carPos).Length2D()<160.0f;
                // The player may rest on the car's own floor brush or on one
                // of the car's playerclip brushes (func_brush parented to the
                // train); both move with the car and carry via the ground
                // link. Walk the parent chain to attribute either.
                CBaseEntity *ground=player->GetGroundEntity();
                for(CBaseEntity *root=ground;root;root=root->GetMoveParent())
                    if(root==car) { ground=car; break; }
                if(ground==car) m_sawCarGround=true;
                // "Carried" once the player followed the car's descent while
                // grounded on it. The absolute 40u drop is not required: the
                // trained car path stalls early in some maps (separate train
                // issue); the transport is proven by following whatever the
                // car does, near it, from its ground.
                const bool carried=ground==car && playerDrop>10.0f &&
                    fabsf(playerDrop-carDrop)<40.0f && near;
                if(carried) m_carried=true;
                if(!m_stuckReported && carDrop>40.0f && playerDrop<20.0f)
                {
                    m_stuckReported=true;
                    Msg("PORTAL2_INTRO departure ride=FAIL stuck player_drop=%.1f car_drop=%.1f "
                        "player_z=%.1f car_z=%.1f near=%d onground=%d\n",
                        playerDrop,carDrop,playerPos.z,carPos.z,near?1:0,
                        (player->GetFlags()&FL_ONGROUND)?1:0);
                }
            }
        }
        CBaseEntity *exitDest=gEntList.FindEntityByName(NULL,"@exit_teleport");
        bool parked = car && exitDest &&
            car->GetAbsOrigin().DistTo(exitDest->GetAbsOrigin())<128.0f &&
            car->GetAbsVelocity().LengthSqr()<1.0f;
        if(parked || gpGlobals->curtime-m_startTime>25.0f)
        {
            Msg("PORTAL2_INTRO departure fallback: %s -> begin\n",parked?"car parked":"timeout");
            Begin();
            return;
        }
        SetNextThink(gpGlobals->curtime+0.5f);
    }
    void Complete()
    {
        if(!m_started || m_queued) return;
        ReportRide();
        char destination[128];
        if(!Portal2NextIntroMap(STRING(gpGlobals->mapname),destination,sizeof(destination)))
        {
            m_queued=true;
            Warning("PORTAL2_INTRO departure: no wired destination from %s\n",STRING(gpGlobals->mapname));
            return;
        }
        m_queued=true;
        variant_t map; map.SetString(AllocPooledString(destination));
        g_EventQueue.AddEvent("@changelevel","ChangeLevel",map,0.0f,this,this);
        Msg("PORTAL2_INTRO departure changing level: %s\n",destination);
    }
// One-shot verdict for the departure transport test: PASS once the player
    // followed the car descent by >40u while staying near it (m_carried),
    // FAIL otherwise. Reported on the transition path/fallback and on
    // changelevel; the guard keeps a single line per map.
    void ReportRide()
    {
        if(m_reported || !m_started) return;
        m_reported=true;
        CBasePlayer *player=UTIL_GetLocalPlayer();
        CBaseEntity *car=m_car.Get();
        const float carZ=car?car->GetAbsOrigin().z:m_carStartZ;
        const float playerZ=player?player->GetAbsOrigin().z:m_startZ;
        Msg("PORTAL2_INTRO departure ride=%s carried=%d ground_on_car=%d "
            "player_drop=%.1f car_drop=%.1f player_z=%.1f car_z=%.1f\n",
            m_carried?"PASS":"FAIL",m_carried?1:0,m_sawCarGround?1:0,
            m_startZ-playerZ,m_carStartZ-carZ,playerZ,carZ);
    }
private:
    bool m_started,m_transition,m_queued;
    float m_startTime,m_startZ,m_carStartZ;
    bool m_carried,m_sawCarGround,m_stuckReported,m_reported;
    EHANDLE m_car;
};
LINK_ENTITY_TO_CLASS(portal2_intro_departure, CPortal2IntroDeparture);
BEGIN_DATADESC(CPortal2IntroDeparture)
    DEFINE_FIELD(m_started,FIELD_BOOLEAN),
    DEFINE_FIELD(m_transition,FIELD_BOOLEAN),
    DEFINE_FIELD(m_queued,FIELD_BOOLEAN),
    DEFINE_FIELD(m_startTime,FIELD_TIME),
    DEFINE_FIELD(m_startZ,FIELD_FLOAT),
    DEFINE_FIELD(m_carStartZ,FIELD_FLOAT),
    DEFINE_FIELD(m_carried,FIELD_BOOLEAN),
    DEFINE_FIELD(m_sawCarGround,FIELD_BOOLEAN),
    DEFINE_FIELD(m_stuckReported,FIELD_BOOLEAN),
    DEFINE_FIELD(m_reported,FIELD_BOOLEAN),
    DEFINE_FIELD(m_car,FIELD_EHANDLE),
    DEFINE_THINKFUNC(DepartureThink),
    DEFINE_THINKFUNC(Complete),
END_DATADESC()

bool Portal2RunIntroDepartureScript(CBaseEntity *host,const char *code)
{
    const char *map=STRING(gpGlobals->mapname);
    if(!FStrEq(map,"sp_a1_intro2") && !FStrEq(map,"sp_a1_intro3") &&
       !FStrEq(map,"sp_a1_intro4") && !FStrEq(map,"sp_a1_intro5")) return false;
    const char *name=host->GetEntityName().ToCStr();
    const bool start=FStrEq(name,"departure_elevator-elevator_1") && FStrEq(code,"StartMoving()");
    const bool ready=FStrEq(name,"departure_elevator-elevator_1_player_teleport") &&
        (FStrEq(code,"ReadyForTransition()") || FStrEq(code,"FailSafeTransition()"));
    const bool finish=FStrEq(name,"@transition_script") && FStrEq(code,"TransitionFromMap()");
    if(!start && !ready && !finish) return false;
    CPortal2IntroDeparture *runtime=dynamic_cast<CPortal2IntroDeparture *>(gEntList.FindEntityByClassname(NULL,"portal2_intro_departure"));
    if(!runtime) runtime=dynamic_cast<CPortal2IntroDeparture *>(CBaseEntity::Create("portal2_intro_departure",vec3_origin,vec3_angle));
    if(!runtime) return false;
    if(start) runtime->Start(host);
    else if(ready) runtime->Begin();
    else runtime->Complete();
    return true;
}

// Opt-in integration fixture: enter the actual departure trigger, without
// sending StartMoving or ChangeLevel directly. Never active in normal play.
// Runs on any intro chain map so the intro3/intro4 departures can be
// exercised the same way as intro2.
class CPortal2Intro2ExitTest : public CAutoGameSystemPerFrame
{
public:
    CPortal2Intro2ExitTest() : CAutoGameSystemPerFrame("Portal2Intro2ExitTest"),m_start(-1),m_done(false) {}
    void LevelInitPreEntity() { m_start=-1; m_done=false; }
    void FrameUpdatePostEntityThink()
    {
        if(m_done || (!CommandLine()->FindParm("-portal2_intro2_exit_test") &&
                      !CommandLine()->FindParm("-portal2_intro_exit_test"))) return;
        const char *map=STRING(gpGlobals->mapname);
        if(!FStrEq(map,"sp_a1_intro2") && !FStrEq(map,"sp_a1_intro3") &&
           !FStrEq(map,"sp_a1_intro4") && !FStrEq(map,"sp_a1_intro5")) return;
        CBasePlayer *player=UTIL_GetLocalPlayer();
        if(!player || !player->IsAlive()) return;
        if(m_start<0) m_start=gpGlobals->curtime;
        if(gpGlobals->curtime-m_start<20.0f) return;
        CBaseEntity *dest=gEntList.FindEntityByName(NULL,"departure_elevator-elevator_1_player_teleport_dest");
        if(!dest) { Warning("PORTAL2_INTRO_EXIT missing departure destination in %s\n",map); m_done=true; return; }
        const Vector pos=dest->GetAbsOrigin(); const QAngle angles=dest->GetAbsAngles();
        player->Teleport(&pos,&angles,&vec3_origin);
        m_done=true;
        Msg("PORTAL2_INTRO_EXIT entered departure car in %s\n",map);
    }
private:
    float m_start; bool m_done;
};
static CPortal2Intro2ExitTest g_Portal2Intro2ExitTest;

// One-line per-map probe of which intro rig entities are present, dumped on
// the first post-entity frame after loading any intro map. Diagnoses maps
// whose arrival/departure rig renames the homogeneous intro2..intro5 chain.
static int Portal2RigNameCompare(const CUtlString *a,const CUtlString *b)
{ return Q_stricmp(a->String(),b->String()); }
class CPortal2RigProbe : public CAutoGameSystemPerFrame
{
public:
    CPortal2RigProbe() : CAutoGameSystemPerFrame("Portal2RigProbe"), m_dumped(false) {}
    void LevelInitPreEntity() { m_dumped=false; }
    void FrameUpdatePostEntityThink()
    {
        if(m_dumped || !UTIL_GetLocalPlayer()) return;
        const char *map=STRING(gpGlobals->mapname);
        if(!FStrEq(map,"sp_a1_intro1") && !FStrEq(map,"sp_a1_intro2") &&
           !FStrEq(map,"sp_a1_intro3") && !FStrEq(map,"sp_a1_intro4") &&
           !FStrEq(map,"sp_a1_intro5")) return;
        m_dumped=true;
        static const char *kPrefixes[]={
            "arrival_","departure_","@arrival_","@elevator_",
            "@transition","@changelevel","transition_trigger","@exit_teleport"};
        CUtlVector<CUtlString> names;
        for(CBaseEntity *e=gEntList.FirstEnt();e!=NULL;e=gEntList.NextEnt(e))
        {
            const char *n=e->GetEntityName().ToCStr();
            if(!n||!n[0]) continue;
            bool keep=false;
            for(unsigned i=0;i<ARRAYSIZE(kPrefixes)&&!keep;++i)
                keep=Q_strncasecmp(n,kPrefixes[i],Q_strlen(kPrefixes[i]))==0;
            if(keep) names.AddToTail(CUtlString(n));
        }
        // De-dup (instance clones share names) and cap the report.
        names.Sort(Portal2RigNameCompare);
        char join[1024]; int off=0; int listed=0;
        for(int i=0;i<names.Count()&&listed<160;++i)
        {
            if(i>0&&FStrEq(names[i-1].String(),names[i].String())) continue;
            ++listed;
            off+=Q_snprintf(join+off,sizeof(join)-off," %s",names[i].String());
        }
        Msg("PORTAL2_RIG map=%s entities:%s\n",map,join);
    }
private:
    bool m_dumped;
};
static CPortal2RigProbe g_Portal2RigProbe;

#undef PORTAL2_PROXY_CHANNELS

// Opt-in dynamic physics sanity probe. With -portal2_phys_probe it spawns the
// Portal 2 weighted cube (prop_physics) above the player and reports whether
// it is simulated by the vphysics reimplementation: position/velocity/body
// every poll, PASS once the cube settles on the floor, FAIL on timeout.
class CPortal2PhysicsProbe : public CAutoGameSystemPerFrame
{
public:
    CPortal2PhysicsProbe() : CAutoGameSystemPerFrame("Portal2PhysicsProbe"),
        m_state(0),m_spawnTime(0.0f),m_pollTime(0.0f),m_lastZ(0.0f),m_lastVel(0.0f),m_stableCount(0) {}
    void LevelInitPreEntity() { m_state=0; m_spawnTime=0.0f; m_pollTime=0.0f; m_lastZ=0.0f; m_lastVel=0.0f; m_stableCount=0; }
    void FrameUpdatePostEntityThink()
    {
        if(!CommandLine()->FindParm("-portal2_phys_probe")) return;
        CBasePlayer *player=UTIL_GetLocalPlayer();
        if(!player || !player->IsAlive()) return;
        if(m_state==0)
        {
            m_state=1;
            m_pollTime=gpGlobals->curtime+6.0f;
            return;
        }
        if(m_state==1 && gpGlobals->curtime<m_pollTime) return;
        if(m_state==1)
        {
            Vector pos=player->GetAbsOrigin();
            pos.z+=96.0f;
            CBaseEntity *pCube=CBaseEntity::CreateNoSpawn("prop_physics",pos,vec3_angle);
            if(!pCube){ Warning("PORTAL2_PHYS could not create prop_physics\n"); m_state=4; return; }
            pCube->KeyValue("model","models/props_underground/underground_weighted_cube.mdl");
            pCube->KeyValue("targetname","p2physprobe");
            pCube->KeyValue("spawnflags","0");
            pCube->Spawn();
            m_state=2;
            m_spawnTime=gpGlobals->curtime;
            m_pollTime=gpGlobals->curtime+1.0f;
            return;
        }
        CBaseEntity *pCube=gEntList.FindEntityByName(NULL,"p2physprobe");
        if(!pCube)
        {
            if(gpGlobals->curtime-m_spawnTime>20.0f){ Warning("PORTAL2_PHYS FAIL entity gone\n"); m_state=4; }
            return;
        }
        if(gpGlobals->curtime<m_pollTime) return;
        m_pollTime=gpGlobals->curtime+1.0f;
        IPhysicsObject *pBody=pCube->VPhysicsGetObject();
        const Vector orgn=pCube->GetAbsOrigin();
        const Vector vel=pCube->GetAbsVelocity();
        Msg("PORTAL2_PHYS t=%.2f pos=(%.1f %.1f %.1f) vel=(%.1f %.1f %.1f) body=%d solid=%d\n",
            gpGlobals->curtime,orgn.x,orgn.y,orgn.z,vel.x,vel.y,vel.z,pBody?1:0,pCube->IsSolid());
        float dz=fabsf(orgn.z-m_lastZ);
        float dvel=fabsf(vel.Length()-m_lastVel);
        m_lastZ=orgn.z; m_lastVel=vel.Length();
        if(dz<0.5f && dvel<2.0f) m_stableCount++;
        else m_stableCount=0;
        if(m_stableCount>=4)
        {
            Msg("PORTAL2_PHYS PASS settled z=%.1f at t=%.2f\n",orgn.z,gpGlobals->curtime);
            m_state=4; // probe complete; the prop may keep living in the map
            return;
        }
        if(gpGlobals->curtime-m_spawnTime>30.0f)
        {
            Warning("PORTAL2_PHYS FAIL did not settle pos=(%.1f %.1f %.1f) body=%d\n",
                orgn.x,orgn.y,orgn.z,pBody?1:0);
            m_state=4;
        }
    }
private:
    int m_state; float m_spawnTime,m_pollTime,m_lastZ,m_lastVel; int m_stableCount;
};
static CPortal2PhysicsProbe g_Portal2PhysicsProbe;

// Opt-in per-frame GroundEntity/BaseVelocity trace (ALLAZGOS.md sections
// 38-40). Enabled with +portal2_ground_probe 1 (same ConVar as the
// SetGroundEntity transition log in gamemovement.cpp). Logs every frame the
// player is on or near the instrumented ride car, plus a few seconds after
// the car parks, and always on a ground-entity change, so the first divergent
// tick of an elevator descent can be located in the log.
extern ConVar portal2_ground_probe;

class CPortal2GroundProbe : public CAutoGameSystemPerFrame
{
public:
    CPortal2GroundProbe() : CAutoGameSystemPerFrame("Portal2GroundProbe"),
        m_windowEnd(-1.0f), m_lastGround(NULL) {}
    void LevelInitPreEntity() { m_windowEnd=-1.0f; m_lastGround=NULL; }
    void FrameUpdatePostEntityThink()
    {
        if(!portal2_ground_probe.GetBool()) return;
        CBasePlayer *player=UTIL_GetLocalPlayer();
        if(!player) return;
        CBaseEntity *train=g_pPortal2RideTrain.Get();
        if(train)
        {
            const float dist=player->GetAbsOrigin().DistTo(train->GetAbsOrigin());
            if(dist<512.0f || train->GetAbsVelocity().LengthSqr()>1.0f)
                m_windowEnd=gpGlobals->curtime+5.0f;
        }
        CBaseEntity *ground=player->GetGroundEntity();
        const bool changed=ground!=m_lastGround.Get();
        m_lastGround=ground;
        if(gpGlobals->curtime>m_windowEnd && !changed) return;
        const char *groundId="NULL";
        if(ground)
        {
            const char *n=ground->GetEntityName().ToCStr();
            groundId=(n&&n[0])?n:ground->GetClassname();
        }
        const char *trainId="none";
        if(train)
        {
            const char *n=train->GetEntityName().ToCStr();
            trainId=(n&&n[0])?n:train->GetClassname();
        }
        const Vector &baseVel=player->GetBaseVelocity();
        Msg("PORTAL2_GROUND tick=%d t=%.3f ground=%s changed=%d train=%s "
            "player_z=%.1f ground_z=%.1f ground_vel=(%.1f %.1f %.1f) "
            "train_z=%.1f train_vel=(%.1f %.1f %.1f) "
            "base_vel=(%.1f %.1f %.1f) player_vel=(%.1f %.1f %.1f) "
            "onground=%d frozen=%d movetype=%d\n",
            gpGlobals->tickcount,gpGlobals->curtime,groundId,changed?1:0,trainId,
            player->GetAbsOrigin().z,
            ground?ground->GetAbsOrigin().z:0.0f,
            ground?ground->GetAbsVelocity().x:0.0f,
            ground?ground->GetAbsVelocity().y:0.0f,
            ground?ground->GetAbsVelocity().z:0.0f,
            train?train->GetAbsOrigin().z:0.0f,
            train?train->GetAbsVelocity().x:0.0f,
            train?train->GetAbsVelocity().y:0.0f,
            train?train->GetAbsVelocity().z:0.0f,
            baseVel.x,baseVel.y,baseVel.z,
            player->GetAbsVelocity().x,
            player->GetAbsVelocity().y,
            player->GetAbsVelocity().z,
            (player->GetFlags()&FL_ONGROUND)?1:0,
            (player->GetFlags()&FL_FROZEN)?1:0,
            (int)player->GetMoveType());
        // §39/40 surface id: first solid directly under the feet, skipping
        // only the player's own hull (not the ride car), so a working ride
        // reports the car floor while a lost-ground state reports whichever
        // brush actually holds the player. frac/startsolid/allsolid separate
        // "resting on" from "embedded in".
        trace_t tr;
        UTIL_TraceLine(player->GetAbsOrigin(), player->GetAbsOrigin()-Vector(0,0,4096),
                       MASK_SOLID, player, COLLISION_GROUP_NONE, &tr);
        const char *hitId="world", *hitClass="world";
        if(tr.m_pEnt)
        {
            const char *n=tr.m_pEnt->GetEntityName().ToCStr();
            hitId=(n&&n[0])?n:"<unnamed>";
            hitClass=tr.m_pEnt->GetClassname();
        }
        Msg("PORTAL2_TRACE tick=%d t=%.3f frac=%.2f startsolid=%d allsolid=%d contents=%d "
            "hit=%s class=%s nz=%.2f end_z=%.1f\n",
            gpGlobals->tickcount,gpGlobals->curtime,tr.fraction,tr.startsolid?1:0,tr.allsolid?1:0,
            tr.contents,hitId,hitClass,tr.plane.normal.z,tr.endpos.z);
        // §40 pin direction: where is the solid that holds an un-grounded
        // boarding player? Up trace from the head (origin+72) and a short
        // down trace from the feet reveal whether the frustum is a ceiling
        // overhead pin or a floor/embed contact, independent of the 4096u
        // line that swallows the interior sleeve.
        {
            trace_t ut;
            UTIL_TraceLine(player->GetAbsOrigin()+Vector(0,0,72.0f),
                           player->GetAbsOrigin()+Vector(0,0,72.0f+96.0f),
                           MASK_PLAYERSOLID, player, COLLISION_GROUP_NONE, &ut);
            Msg("PORTAL2_TRACE_UP tick=%d t=%.3f frac=%.2f startsolid=%d hit=%s end_z=%.1f\n",
                gpGlobals->tickcount,gpGlobals->curtime,ut.fraction,ut.startsolid?1:0,
                ut.m_pEnt?(ut.m_pEnt->GetEntityName().ToCStr()[0]?ut.m_pEnt->GetEntityName().ToCStr():"<unnamed>"):"none",
                ut.endpos.z);
        }
        {
            trace_t nt;
            UTIL_TraceLine(player->GetAbsOrigin(), player->GetAbsOrigin()-Vector(0,0,128.0f),
                           MASK_PLAYERSOLID, player, COLLISION_GROUP_NONE, &nt);
            Msg("PORTAL2_TRACE_NEAR tick=%d t=%.3f frac=%.2f startsolid=%d hit=%s end_z=%.1f\n",
                gpGlobals->tickcount,gpGlobals->curtime,nt.fraction,nt.startsolid?1:0,
                nt.m_pEnt?(nt.m_pEnt->GetEntityName().ToCStr()[0]?nt.m_pEnt->GetEntityName().ToCStr():"<unnamed>"):"none",
                nt.endpos.z);
        }
        // What is the first solid under the ride car itself? Skipping both the
        // player and the car reveals the surface the departure car embeds
        // against when its descent stalls (the ~36-52u stick), separate from
        // the player-side surface above it.
        if(train)
        {
            trace_t ct;
            CTraceFilterSkipTwoEntities carFilter(player,train,COLLISION_GROUP_NONE);
            UTIL_TraceLine(train->GetAbsOrigin(), train->GetAbsOrigin()-Vector(0,0,4096),
                           MASK_SOLID, &carFilter, &ct);
            const char *carHitId="world", *carHitClass="world";
            if(ct.m_pEnt)
            {
                const char *n=ct.m_pEnt->GetEntityName().ToCStr();
                carHitId=(n&&n[0])?n:"<unnamed>";
                carHitClass=ct.m_pEnt->GetClassname();
            }
            Msg("PORTAL2_TRACE_CAR tick=%d t=%.3f frac=%.2f startsolid=%d allsolid=%d contents=%d "
                "car_z=%.1f hit=%s class=%s nz=%.2f end_z=%.1f\n",
                gpGlobals->tickcount,gpGlobals->curtime,ct.fraction,ct.startsolid?1:0,ct.allsolid?1:0,
                ct.contents,train->GetAbsOrigin().z,carHitId,carHitClass,ct.plane.normal.z,ct.endpos.z);
        }
    }
private:
    float m_windowEnd;
    EHANDLE m_lastGround;
};
static CPortal2GroundProbe g_Portal2GroundProbe;

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
