// Native adapter for sp_a1_intro1's installed Squirrel scene tables. This is
// deliberately not a general VScript VM. Unimplemented calls are reported.
#include "cbase.h"
#include "eventqueue.h"
#include "sceneentity.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "saverestore_utlvector.h"
#include "igamesystem.h"
#include "tier0/icommandline.h"
#include "tier0/memdbgon.h"

static void Fire( const char *target, const char *input, const char *arg, float delay, CBaseEntity *caller )
{
    variant_t value;
    value.SetString( AllocPooledString( arg ) );
    g_EventQueue.AddEvent( target, input, value, delay, UTIL_GetLocalPlayer(), caller );
}

class CPortal2IntroRuntime : public CPointEntity
{
public:
    DECLARE_CLASS( CPortal2IntroRuntime, CPointEntity );
    DECLARE_DATADESC();
    CPortal2IntroRuntime() : m_data( NULL ), m_doorOpened( false ), m_vaultStarted( false ),
        m_gauntletSaid( false ), m_fizzlerSaid( false ), m_fizzlerTriggered( false ),
        m_fizzlerPassed( false ), m_transitionReady( false ),
        m_elevatorTransitionStarted( false ), m_levelChangeQueued( false ) {}
    ~CPortal2IntroRuntime() { if ( m_data ) m_data->deleteThis(); }
    void Spawn()
    {
        BaseClass::Spawn();
        SetName( AllocPooledString( "@portal2_intro_runtime" ) );
        LoadData();
        // This is an opt-in integration fixture. It drives the exact map I/O
        // used by the departure elevator; ordinary launches never enter here.
        if ( CommandLine()->FindParm( "-portal2_elevator_transition_test" ) )
            Fire( GetEntityName().ToCStr(), "TestElevatorTransition", "", 3.0f, this );
    }
    void OnRestore() { BaseClass::OnRestore(); LoadData(); }
    void LoadData()
    {
        if ( m_data ) return;
        m_data = new KeyValues( "Portal2IntroScenes" );
        m_data->UsesEscapeSequences( true );
        if ( !m_data->LoadFromFile( filesystem, "scripts/portal2_intro1_scenes.txt", "GAME" ) )
            Warning( "PORTAL2_INTRO missing scene data: run prepare-portal2-intro-script.py\n" );
    }
    KeyValues *Scene( const char *id )
    {
        if ( !id || !id[0] ) return NULL;
        LoadData();
        KeyValues *scenes = m_data->FindKey( "scenes" );
        return scenes ? scenes->FindKey( id ) : NULL;
    }
    void PlayID( int id )
    {
        LoadData();
        char number[32]; Q_snprintf( number, sizeof(number), "%d", id );
        KeyValues *lookup = m_data->FindKey( "lookup" );
        const char *key = lookup ? lookup->GetString( number, "" ) : "";
        KeyValues *scene = Scene( key );
        if ( !scene ) { Warning( "PORTAL2_INTRO missing scene id %d\n", id ); return; }
        Fire( GetEntityName().ToCStr(), "PlayScene", key, scene->GetFloat( "predelay" ), this );
    }
    void Cancel()
    {
        g_EventQueue.CancelEvents( this );
        variant_t empty;
        FOR_EACH_VEC( m_scenes, i )
        {
            CBaseEntity *scene = m_scenes[i].Get();
            if ( !scene ) continue;
            g_EventQueue.CancelEvents( scene );
            scene->AcceptInput( "Cancel", this, this, empty, 0 );
            UTIL_Remove( scene );
        }
        m_scenes.Purge();
    }
    void Fires( KeyValues *entry, bool atStart, CBaseEntity *caller )
    {
        KeyValues *fires = entry->FindKey( "fires" );
        for ( KeyValues *item = fires ? fires->GetFirstSubKey() : NULL; item; item = item->GetNextKey() )
        {
            bool starts = FStrEq( item->GetString( "fireatstart" ), "true" );
            if ( starts == atStart ) Fire( item->GetString("entity"), item->GetString("input"),
                item->GetString("parameter"), item->GetFloat("delay"), caller );
        }
    }
    void Next( KeyValues *entry, float delay, CBaseEntity *caller )
    {
        const char *next = entry->GetString( "next" );
        KeyValues *nextEntry = Scene( next );
        Msg("PORTAL2_INTRO next=%s available=%d delay=%.2f\n",next,nextEntry!=NULL,delay);
        if ( nextEntry ) Fire( GetEntityName().ToCStr(), "PlayScene", next,
            MAX( 0.0f, delay ) + nextEntry->GetFloat( "predelay" ), caller );
    }
    void InputPlay( inputdata_t &data )
    {
        const char *id = data.value.String();
        KeyValues *entry = Scene( id );
        if ( !entry ) return;
        const char *path = entry->GetString("vcd");
        const float duration = GetSceneDuration( path );
        if ( duration <= 0 )
        {
            Warning( "PORTAL2_INTRO cannot play scene %s (%s); progression not faked\n", id, path );
            return;
        }
        CBaseEntity *scene = CreateEntityByName( "logic_choreographed_scene" );
        if ( !scene ) return;
        scene->KeyValue( "SceneFile", path );
        scene->KeyValue( "target1", entry->GetString("settarget1") );
        char action[256];
        Q_snprintf( action, sizeof(action), "@portal2_intro_runtime,CompleteScene,%s,0,1", id );
        scene->KeyValue( "OnCompletion", action );
        DispatchSpawn( scene );
        scene->Activate();
        m_scenes.AddToTail( scene );
        variant_t empty;
        scene->AcceptInput( "Start", this, this, empty, 0 );
        Fires( entry, true, scene );
        const float post = entry->GetFloat("postdelay");
        if ( post < 0 ) Next( entry, duration + post, scene );
        Msg( "PORTAL2_INTRO scene started %s duration=%.2f\n", id, duration );
    }
    void InputComplete( inputdata_t &data )
    {
        KeyValues *entry = Scene( data.value.String() );
        if ( !entry ) return;
        Msg( "PORTAL2_INTRO scene completed %s\n", data.value.String() );
        Fires( entry, false, this );
        if ( entry->GetFloat("postdelay") >= 0 ) Next( entry, entry->GetFloat("postdelay"), this );
        // Keep nagging until the actual door-use trigger cancels this chain.
        if ( !m_doorOpened && !Q_strncmp( data.value.String(), "-601_", 5 ) )
        {
            int n = atoi( data.value.String() + 5 ) % 5 + 1;
            char key[32]; Q_snprintf( key, sizeof(key), "-601_%02d", n );
            Fire( GetEntityName().ToCStr(), "PlayScene", key, 2.0f, this );
        }
        if ( data.pCaller && data.pCaller != this )
        {
            m_scenes.FindAndRemove( data.pCaller );
            // The source event queue is still dispatching this caller. Delete
            // on a later tick, after the completion/next-scene events return.
            g_EventQueue.AddEvent(data.pCaller,"Kill",1.0f,this,this);
        }
    }
    void BeginElevatorTransition( CBaseEntity *caller, const char *reason )
    {
        if ( m_elevatorTransitionStarted )
            return;
        m_elevatorTransitionStarted = true;
        Msg( "PORTAL2_INTRO elevator transition started (%s)\n", reason );
        Fire( "@transition_from_map", "Trigger", "", 0, caller );
        Fire( "@transition_with_survey", "Trigger", "", 0, caller );

        // In the complete engine, @exit_teleport carries the player through
        // transition_trigger, which calls TransitionFromMap.  A few stripped
        // compatibility maps lack that final relay.  Keep the authored route
        // first, then guarantee the same destination if it did not fire.
        Fire( GetEntityName().ToCStr(), "CompleteElevatorTransition", "", 2.0f, caller );
    }
    void QueueIntro2ChangeLevel( CBaseEntity *caller, const char *reason )
    {
        if ( m_levelChangeQueued )
            return;
        m_levelChangeQueued = true;
        Msg( "PORTAL2_INTRO elevator changing level (%s): sp_a1_intro1 -> sp_a1_intro2\n", reason );
        Fire( "@changelevel", "ChangeLevel", "sp_a1_intro2", 0, caller );
    }
    void InputElevatorFailSafe( inputdata_t &data )
    {
        BeginElevatorTransition( data.pCaller ? data.pCaller : this, "failsafe" );
    }
    void InputCompleteElevatorTransition( inputdata_t &data )
    {
        QueueIntro2ChangeLevel( data.pCaller ? data.pCaller : this, "fallback" );
    }
    void InputTestElevatorTransition( inputdata_t &data )
    {
        m_transitionReady = true;
        CBaseEntity *elevator = gEntList.FindEntityByName( NULL, "departure_elevator-elevator_1" );
        if ( !elevator )
        {
            Warning( "PORTAL2_ELEVATOR_TEST FAIL: departure elevator missing\n" );
            return;
        }
        variant_t code;
        code.SetString( AllocPooledString( "StartMoving()" ) );
        elevator->AcceptInput( "RunScriptCode", this, this, code, 0 );
        Msg( "PORTAL2_ELEVATOR_TEST started authored elevator I/O\n" );
    }
    bool Run( CBaseEntity *host, const char *code )
    {
        char name[128]; int n = 0;
        while ( *code == ' ' ) ++code;
        while ( code[n] && code[n] != '(' && code[n] != ' ' && n < (int)sizeof(name)-1 ) { name[n] = code[n]; ++n; }
        name[n] = 0;
        const char *args = Q_strstr( code, "(" );
        if ( FStrEq(name,"GladosPlayVcd") && args ) { PlayID( atoi(args+1) ); return true; }
        struct Call { const char *name; int id; };
        static const Call calls[] = {
            {"sp_a1_intro1_open_door_sequence",-600}, {"sp_a1_intro1_say_apple_nag",-603},
            {"sp_a1_intro1_explain_brain_damage",-607}, {"sp_a1_intro1_prepare",-608},
            {"sp_a1_intro_cognitive_gauntlet",359}, {"sp_a1_intro_good_luck",361} };
        for ( unsigned i=0; i<ARRAYSIZE(calls); ++i ) if ( FStrEq(name,calls[i].name) ) { PlayID(calls[i].id); return true; }
        if ( FStrEq(name,"sp_a1_intro1_knew_someone_alive") ) { m_doorOpened=true; Cancel(); PlayID(-602); return true; }
        if ( FStrEq(name,"sp_a1_intro1_open_door_nags") ) { if (!m_doorOpened) PlayID(-601); return true; }
        if ( FStrEq(name,"sp_a1_intro1_jumping_close_enough") ) { Cancel(); PlayID(-604); return true; }
        if ( FStrEq(name,"sp_a1_intro_cognitive_gauntlet_over") ) { m_gauntletSaid=true; return true; }
        if ( FStrEq(name,"sp_a1_intro1_vault_start") ) { m_vaultStarted=true; return true; }
        if ( FStrEq(name,"sp_a1_intro_thats_the_spirit") ) { if(!m_vaultStarted && m_gauntletSaid) { Cancel(); PlayID(360); } return true; }
        if ( FStrEq(name,"sp_a1_intro1_fizzler_intro") )
        { m_fizzlerTriggered=true; if(!m_fizzlerSaid && m_scenes.Count()==0) { m_fizzlerSaid=true; PlayID(553); } return true; }
        if ( FStrEq(name,"sp_a1_intro1_fizzler_passed") )
        { m_fizzlerPassed=true; if(!m_fizzlerSaid && m_scenes.Count()==0) { m_fizzlerSaid=true; PlayID(630); } return true; }
        if ( FStrEq(name,"StartContainerAnimations") )
        { Fire("@container_stacks_1","SetAnimation","anim1",0,host); Fire("@container_stacks_2","SetAnimation","anim1",0,host); Fire("@container_stacks_2","DisableDraw","",0,host); return true; }
        if ( FStrEq(name,"ShowHiddenContainers") ) { Fire("@container_stacks_2","EnableDraw","",0,host); return true; }
        if ( FStrEq(name,"StartMoving") )
        {
            Fire(host->GetEntityName().ToCStr(),"SetSpeedReal","200",0,host);
            // The first authored path callback arrives about five seconds
            // into this ride.  If it is lost because a VScript callback is
            // unavailable, let the car move for its authored segment and
            // then use the same transition relays rather than trapping the
            // player in an endless shaft.
            Fire(GetEntityName().ToCStr(),"ElevatorFailSafe","",12.0f,host);
            return true;
        }
        if ( FStrEq(name,"ReadyForTransition") || FStrEq(name,"FailSafeTransition") )
        { if(m_transitionReady || FStrEq(name,"FailSafeTransition")) BeginElevatorTransition(host,name); return true; }
        if ( FStrEq(name,"TransitionFromMap") ) { QueueIntro2ChangeLevel(host,"authored trigger"); return true; }
        if ( FStrEq(name,"GladosRelaxationVaultPowerUp") ) { Fire("open_portal_relay","Trigger","",0,host); return true; }
        if ( FStrEq(name,"TransitionReady") ) { m_transitionReady=true; return true; }
        if ( FStrEq(name,"sp_a1_intro1_fizzler_test") )
        { if(m_fizzlerTriggered && !m_fizzlerSaid) { m_fizzlerSaid=true; PlayID(m_fizzlerPassed?630:553); } return true; }
        if ( FStrEq(name,"ExitStarted") ) return true; // no exitstarted entry for intro1
        if ( FStrEq(name,"DisplayChapterTitle") ) { UTIL_ClientPrintAll(HUD_PRINTCENTER,"Chapter 1: The Courtesy Call"); return true; }
        // Original functions that only prefetch PS3 files or have empty bodies.
        if ( FStrEq(name,"PrecacheContainerAnimations") || FStrEq(name,"SetupContainerAttachments") ||
             FStrEq(name,"sp_a1_intro1_first_wall_impact") || FStrEq(name,"sp_a1_intro1_container_start_moving") ) return true;
        return false;
    }
private:
    KeyValues *m_data;
    CUtlVector<EHANDLE> m_scenes;
    bool m_doorOpened, m_vaultStarted, m_gauntletSaid, m_fizzlerSaid;
    bool m_fizzlerTriggered, m_fizzlerPassed, m_transitionReady;
    bool m_elevatorTransitionStarted, m_levelChangeQueued;
};
LINK_ENTITY_TO_CLASS( portal2_intro_runtime, CPortal2IntroRuntime );
BEGIN_DATADESC( CPortal2IntroRuntime )
    DEFINE_INPUTFUNC( FIELD_STRING, "PlayScene", InputPlay ),
    DEFINE_INPUTFUNC( FIELD_STRING, "CompleteScene", InputComplete ),
    DEFINE_UTLVECTOR( m_scenes, FIELD_EHANDLE ),
    DEFINE_FIELD( m_doorOpened, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_vaultStarted, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_gauntletSaid, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_fizzlerSaid, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_fizzlerTriggered, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_fizzlerPassed, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_transitionReady, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_elevatorTransitionStarted, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_levelChangeQueued, FIELD_BOOLEAN ),
    DEFINE_INPUTFUNC( FIELD_VOID, "ElevatorFailSafe", InputElevatorFailSafe ),
    DEFINE_INPUTFUNC( FIELD_VOID, "CompleteElevatorTransition", InputCompleteElevatorTransition ),
    DEFINE_INPUTFUNC( FIELD_VOID, "TestElevatorTransition", InputTestElevatorTransition ),
END_DATADESC()

// logic_script dispatches to the same map-scoped handler as scripted actors.
class CPortal2ScriptHost : public CPointEntity {};
LINK_ENTITY_TO_CLASS( logic_script, CPortal2ScriptHost );

bool Portal2RunIntroScript( CBaseEntity *host, const char *code )
{
    if ( !FStrEq( STRING(gpGlobals->mapname), "sp_a1_intro1" ) ) return false;
    CPortal2IntroRuntime *runtime = dynamic_cast<CPortal2IntroRuntime *>(
        gEntList.FindEntityByClassname( NULL, "portal2_intro_runtime" ) );
    if ( !runtime ) runtime = dynamic_cast<CPortal2IntroRuntime *>(
        CBaseEntity::Create( "portal2_intro_runtime", vec3_origin, vec3_angle ) );
    if ( runtime && runtime->Run( host, code ) ) return true;
    Warning( "PORTAL2_INTRO unsupported script on %s: %s\n", host->GetDebugName(), code );
    return false;
}

// Explicit playground entry, separate from both the campaign and automated
// tests. It skips the hotel cinematic but keeps the original chamber entities.
CON_COMMAND_F(portal2_intro_playground,"Enter intro1's first chamber with the dual gun (skips the hotel cinematic).",FCVAR_CHEAT)
{
    CBasePlayer *player=UTIL_GetLocalPlayer();
    if(!player || !player->IsAlive() || !FStrEq(STRING(gpGlobals->mapname),"sp_a1_intro1"))
    { Warning("Load sp_a1_intro1 before entering the playground.\n"); return; }
    CPortal2IntroRuntime *runtime=dynamic_cast<CPortal2IntroRuntime *>(gEntList.FindEntityByClassname(NULL,"portal2_intro_runtime"));
    if(runtime) runtime->Cancel();
    variant_t empty;
    const char *classes[]={"point_viewcontrol","point_viewproxy","logic_choreographed_scene"};
    for(unsigned i=0;i<ARRAYSIZE(classes);++i)
    {
        CBaseEntity *entity=NULL;
        while((entity=gEntList.FindEntityByClassname(entity,classes[i]))!=NULL)
        {
            g_EventQueue.CancelEvents(entity);
            entity->AcceptInput(i==2?"Cancel":"Disable",player,player,empty,0);
        }
    }
    const Vector origin(-688,4200,2680); const QAngle angles(0,90,0);
    player->Teleport(&origin,&angles,&vec3_origin);
    player->EnableControl(true); player->SetViewEntity(player);
    player->RemoveSolidFlags(FSOLID_NOT_SOLID); player->RemoveFlag(FL_FROZEN);
    engine->ServerCommand("portal2_equip_portalgun\n");
    Fire("drop_box_rl","Trigger","",0.5f,player);
    UTIL_ClientPrintAll(HUD_PRINTTALK,"Portal 2 playground: mouse 1/2 portals, E pick up cube, F6 equip gun.");
    Msg("PORTAL2_PLAYGROUND ready: original intro1 chamber, dual gun, cube puzzle\n");
}

class CPortal2PlaygroundLaunch : public CAutoGameSystemPerFrame
{
public:
    CPortal2PlaygroundLaunch() : CAutoGameSystemPerFrame("Portal2PlaygroundLaunch"),m_start(-1),m_done(false) {}
    void LevelInitPreEntity() { m_start=-1; m_done=false; }
    void FrameUpdatePostEntityThink()
    {
        if(m_done || !CommandLine()->FindParm("-portal2_intro_playground") ||
           !FStrEq(STRING(gpGlobals->mapname),"sp_a1_intro1") || !UTIL_GetLocalPlayer()) return;
        if(m_start<0) m_start=gpGlobals->curtime;
        // Allow the stock map's wake-up camera to release before skipping.
        if(gpGlobals->curtime-m_start>15)
        { engine->ServerCommand("portal2_intro_playground\n"); m_done=true; }
    }
private:
    float m_start; bool m_done;
};
static CPortal2PlaygroundLaunch g_Portal2PlaygroundLaunch;
