// Opt-in integration test: uses the actual weapon trace, attack, linkage and
// movement code in sp_a1_intro1. Never runs in an ordinary campaign launch.
#include "cbase.h"
#include "igamesystem.h"
#include "tier0/icommandline.h"
#include "portal_player.h"
#include "weapon_portalgun.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "player_pickup.h"
#include "weapon_physcannon.h"
#include "tier0/memdbgon.h"

static bool CheckOutputFormats()
{
    CEventAction old("door,Open,,0.25,1");
    CEventAction modern("@glados\x1b" "RunScriptCode\x1b" "StopVideo(23,2)\x1b" "0.5\x1b" "-1");
    bool ok=FStrEq(STRING(old.m_iTarget),"door") && FStrEq(STRING(old.m_iTargetInput),"Open") &&
        old.m_iParameter==NULL_STRING && old.m_flDelay==0.25f && old.m_nTimesToFire==1 &&
        FStrEq(STRING(modern.m_iTarget),"@glados") && FStrEq(STRING(modern.m_iTargetInput),"RunScriptCode") &&
        FStrEq(STRING(modern.m_iParameter),"StopVideo(23,2)") && modern.m_flDelay==0.5f && modern.m_nTimesToFire==-1;
    Msg("PORTAL2_GAMEPLAY_TEST output_formats=%s\n",ok?"PASS":"FAIL");
    CBaseEntity *clip=CreateEntityByName("func_clip_vphysics");
    CBaseEntity *pending=CreateEntityByName("info_target");
    bool physicsSafe=false;
    if(clip && pending)
    {
        pending->SetMoveType(MOVETYPE_VPHYSICS);
        physicsSafe=!clip->ForceVPhysicsCollide(NULL) && !clip->ForceVPhysicsCollide(pending);
    }
    Msg("PORTAL2_GAMEPLAY_TEST physics_filter=%s\n",physicsSafe?"PASS":"FAIL");
    if(clip) UTIL_Remove(clip);
    if(pending) UTIL_Remove(pending);
    return ok;
}

static CWeaponPortalgun *Gun(CPortal_Player *p)
{ return dynamic_cast<CWeaponPortalgun *>(p->Weapon_OwnsThisType("weapon_portalgun")); }

bool Portal2GameplayTestOwnsInput()
{ return (CommandLine()->FindParm("-portal2_gameplay_test") || CommandLine()->FindParm("-portal2_intro_scene_test")) &&
    FStrEq(STRING(gpGlobals->mapname),"sp_a1_intro1"); }

class CPortal2EventQueueTest : public CPointEntity
{
public:
    DECLARE_CLASS(CPortal2EventQueueTest,CPointEntity);
    DECLARE_DATADESC();
    CPortal2EventQueueTest() : m_cancelled(false),m_unexpected(false) {}
    void Spawn()
    {
        BaseClass::Spawn();
        g_EventQueue.AddEvent(this,"CancelSelf",0.0f,this,this);
        g_EventQueue.AddEvent(this,"Unexpected",0.1f,this,this);
        g_EventQueue.AddEvent(this,"Verify",0.2f,NULL,NULL);
    }
    void CancelSelf(inputdata_t &) { g_EventQueue.CancelEvents(this); m_cancelled=true; }
    void Unexpected(inputdata_t &) { m_unexpected=true; }
    void InputVerify(inputdata_t &)
    { Msg("PORTAL2_GAMEPLAY_TEST reentrant_cancel=%s\n",m_cancelled&&!m_unexpected?"PASS":"FAIL"); UTIL_Remove(this); }
private:
    bool m_cancelled,m_unexpected;
};
LINK_ENTITY_TO_CLASS(portal2_eventqueue_test,CPortal2EventQueueTest);
BEGIN_DATADESC(CPortal2EventQueueTest)
    DEFINE_INPUTFUNC(FIELD_VOID,"CancelSelf",CancelSelf),
    DEFINE_INPUTFUNC(FIELD_VOID,"Unexpected",Unexpected),
    DEFINE_INPUTFUNC(FIELD_VOID,"Verify",InputVerify),
END_DATADESC()

class CPortal2GameplayTest : public CAutoGameSystemPerFrame
{
public:
    CPortal2GameplayTest() : CAutoGameSystemPerFrame("Portal2GameplayTest"), m_step(0),m_start(-1),m_crossed(false) {}
    void LevelInitPreEntity() { m_step=0; m_start=-1; m_crossed=false; }
    bool Shoot(CPortal_Player *p,bool orange)
    {
        CWeaponPortalgun *gun=Gun(p);
        if(!gun) return false;
        CProp_Portal *blue=CProp_Portal::FindPortal(gun->m_iPortalLinkageGroupID,false);
        const int pitches[]={0,30,-30,60,-60,90};
        for(unsigned pi=0;pi<ARRAYSIZE(pitches);++pi) for(int yaw=0;yaw<360;yaw+=15)
        {
            const int pitch=pitches[pi];
            Vector dir,hit; QAngle final; trace_t tr; const QAngle aim(pitch,yaw,0);
            AngleVectors(aim,&dir);
            const float result=gun->TraceFirePortal(orange,p->EyePosition(),dir,tr,hit,final,PORTAL_PLACED_BY_PLAYER,true);
            if(pitch==0 && yaw%90==0)
                Msg("PORTAL2_GAMEPLAY_TEST trace yaw=%d result=%.3f fraction=%.4f solid=%d surface=%s\n",
                    yaw,result,tr.fraction,tr.startsolid,tr.surface.name?tr.surface.name:"<none>");
            if(result<0.3f || (orange && blue && hit.DistTo(blue->GetAbsOrigin())<160)) continue;
            p->SnapEyeAngles(aim);
            if(orange) gun->SecondaryAttack(); else gun->PrimaryAttack();
            Msg("PORTAL2_GAMEPLAY_TEST attack=%s aim=%d %d result=%.3f\n",orange?"orange":"blue",pitch,yaw,result);
            return true;
        }
        Warning("PORTAL2_GAMEPLAY_TEST no valid %s surface at %.0f %.0f %.0f\n",orange?"orange":"blue",
            p->GetAbsOrigin().x,p->GetAbsOrigin().y,p->GetAbsOrigin().z);
        return false;
    }
    void FrameUpdatePostEntityThink()
    {
        if(!Portal2GameplayTestOwnsInput()) return;
        CPortal_Player *p=ToPortalPlayer(UTIL_GetLocalPlayer());
        if(!p || !p->IsAlive()) return;
        if(m_start<0)
        { m_start=gpGlobals->curtime; CheckOutputFormats(); CBaseEntity::Create("portal2_eventqueue_test",vec3_origin,vec3_angle); }
        const float elapsed=gpGlobals->curtime-m_start;
        const bool sceneTest=CommandLine()->FindParm("-portal2_intro_scene_test")!=0;
        if(m_step==0 && elapsed>15)
        {
            // Placement test starts in the map's real first test chamber,
            // beyond the glass relaxation vault, on unmodified white panels.
            CBaseEntity *camera=NULL; variant_t empty;
            while((camera=gEntList.FindEntityByClassname(camera,"point_viewcontrol"))!=NULL)
                camera->AcceptInput("Disable",p,p,empty,0);
            const Vector origin=sceneTest?Vector(-1280,4352,2680):Vector(-688,4200,2680);
            const QAngle angles(0,90,0);
            p->Teleport(&origin,&angles,&vec3_origin);
            p->EnableControl(true); p->SetViewEntity(p); p->RemoveSolidFlags(FSOLID_NOT_SOLID);
            if(!sceneTest) engine->ServerCommand("portal2_equip_portalgun\nportal2_portalgun_debug 1\n");
            ++m_step;
        }
        if(sceneTest) return; // The unmodified map trigger starts the scene chain.
        if(m_step==1 && elapsed>18) { Shoot(p,false); ++m_step; }
        if(m_step==2 && elapsed>21) { Shoot(p,true); ++m_step; }
        CWeaponPortalgun *gun=Gun(p);
        if(!gun) return;
        CProp_Portal *blue=CProp_Portal::FindPortal(gun->m_iPortalLinkageGroupID,false);
        CProp_Portal *orange=CProp_Portal::FindPortal(gun->m_iPortalLinkageGroupID,true);
        if(m_step==3 && elapsed>24)
        {
            const bool linked=blue && orange && blue->IsActivedAndLinked() && orange->IsActivedAndLinked() &&
                blue->m_hLinkedPortal.Get()==orange && orange->m_hLinkedPortal.Get()==blue;
            Msg("PORTAL2_GAMEPLAY_TEST placement=%s weapon_active=%d\n",linked?"PASS":"FAIL",p->GetActiveWeapon()==gun);
            if(linked)
            {
                blue->GetVectors(&m_direction,NULL,NULL);
                m_exit=orange->GetAbsOrigin();
                const Vector centerOffset=p->WorldSpaceCenter()-p->GetAbsOrigin();
                const Vector origin=blue->GetAbsOrigin()+m_direction*48-centerOffset;
                const Vector velocity=-m_direction*200;
                QAngle aim; VectorAngles(-m_direction,aim);
                p->Teleport(&origin,&aim,&velocity);
                p->SetGroundEntity(NULL);
                m_entry=p->GetAbsOrigin();
                m_step=4;
            }
            else m_step=6;
        }
        if(m_step==4)
        {
            if(!m_crossed) p->SetAbsVelocity(-m_direction*200);
            if(elapsed>27)
            {
                Msg("PORTAL2_GAMEPLAY_TEST crossing=%s\n",m_crossed?"PASS":"FAIL");
                Msg("PORTAL2_GAMEPLAY_TEST crossing_position=%.1f %.1f %.1f expected_exit=%.1f %.1f %.1f\n",
                    p->GetAbsOrigin().x,p->GetAbsOrigin().y,p->GetAbsOrigin().z,m_exit.x,m_exit.y,m_exit.z);
                p->SetAbsVelocity(vec3_origin); m_step=5;
            }
        }
        if(m_step==5 && elapsed>29)
        {
            CBaseEntity *cube=gEntList.FindEntityByClassname(NULL,"prop_weighted_cube");
            CBaseEntity *door=gEntList.FindEntityByName(NULL,"exit_door_left_a00");
            if(!cube || !door)
            { Warning("PORTAL2_GAMEPLAY_TEST cube_puzzle=FAIL missing cube/door\n"); m_step=9; return; }
            m_cube=cube; m_door=door; m_closedDoor=door->GetAbsOrigin();
            const Vector playerOrigin(-688,4304,2680), nearby(-644,4330,2710), onButton(-624,4432,2710);
            const QAngle aim(30,30,0);
            p->Teleport(&playerOrigin,&aim,&vec3_origin);
            cube->Teleport(&nearby,&vec3_angle,&vec3_origin);
            cube->Use(p,p,USE_TOGGLE,0);
            Msg("PORTAL2_GAMEPLAY_TEST cube_pickup=%s\n",GetPlayerHeldEntity(p)==cube?"PASS":"FAIL");
            p->ForceDropOfCarriedPhysObjects(cube);
            cube->Teleport(&onButton,&vec3_angle,&vec3_origin);
            if(cube->VPhysicsGetObject()) cube->VPhysicsGetObject()->Wake();
            m_step=7;
        }
        if(m_step==7 && elapsed>34)
        {
            const bool opened=m_door && m_door->GetAbsOrigin().DistTo(m_closedDoor)>10;
            Msg("PORTAL2_GAMEPLAY_TEST cube_button_opens_door=%s\n",opened?"PASS":"FAIL");
            if(m_cube)
            {
                const Vector away(-560,4304,2710);
                m_cube->Teleport(&away,&vec3_angle,&vec3_origin);
                Msg("PORTAL2_GAMEPLAY_TEST cube_position=%.1f %.1f %.1f\n",
                    m_cube->GetAbsOrigin().x,m_cube->GetAbsOrigin().y,m_cube->GetAbsOrigin().z);
            }
            m_step=8;
        }
        if(m_step==8 && elapsed>39)
        {
            const bool closed=m_door && m_door->GetAbsOrigin().DistTo(m_closedDoor)<1;
            Msg("PORTAL2_GAMEPLAY_TEST cube_button_closes_door=%s\n",closed?"PASS":"FAIL");
            p->SnapEyeAngles(QAngle(0,90,0)); m_step=9;
        }
    }
    void PlayerTeleported(CBaseEntity *player,CProp_Portal *entry)
    {
        if(!Portal2GameplayTestOwnsInput() || m_step!=4 || entry->m_bIsPortal2 || player!=UTIL_GetLocalPlayer()) return;
        m_crossed=player->WorldSpaceCenter().DistTo(m_exit)<160;
        Msg("PORTAL2_GAMEPLAY_TEST observed_real_portal_teleport=%d\n",m_crossed);
    }
private:
    int m_step; float m_start; bool m_crossed; Vector m_direction,m_exit,m_entry,m_closedDoor;
    EHANDLE m_cube,m_door;
};
static CPortal2GameplayTest g_Portal2GameplayTest;

void Portal2GameplayTestPlayerTeleported(CBaseEntity *player,CProp_Portal *entry)
{ g_Portal2GameplayTest.PlayerTeleported(player,entry); }
