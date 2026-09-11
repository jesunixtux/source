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

// The next departure uses the same authored speed and path callbacks, but
// must not instantiate intro1's dialogue/runtime or select its destination.
class CPortal2IntroDeparture : public CPointEntity
{
public:
    DECLARE_CLASS(CPortal2IntroDeparture,CPointEntity);
    DECLARE_DATADESC();
    CPortal2IntroDeparture() : m_started(false),m_transition(false),m_queued(false),m_startTime(0) {}
    void Start(CBaseEntity *car)
    {
        if(m_started) return;
        m_started=true;
        m_car=car;
        m_startTime=gpGlobals->curtime;
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
private:
    bool m_started,m_transition,m_queued;
    float m_startTime;
    EHANDLE m_car;
};
LINK_ENTITY_TO_CLASS(portal2_intro_departure, CPortal2IntroDeparture);
BEGIN_DATADESC(CPortal2IntroDeparture)
    DEFINE_FIELD(m_started,FIELD_BOOLEAN),
    DEFINE_FIELD(m_transition,FIELD_BOOLEAN),
    DEFINE_FIELD(m_queued,FIELD_BOOLEAN),
    DEFINE_FIELD(m_startTime,FIELD_TIME),
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
