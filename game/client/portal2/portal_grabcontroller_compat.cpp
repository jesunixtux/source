#include "cbase.h"
#include "portal2/portal_grabcontroller_shared.h"

ConVar sv_portal_placement_never_fail( "sv_portal_placement_never_fail", "0", FCVAR_REPLICATED | FCVAR_CHEAT );

ConVar player_held_object_use_view_model( "player_held_object_use_view_model", "-1", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar locator_background_style( "locator_background_style", "0" );
ConVar locator_background_color( "locator_background_color", "0 0 0 128" );
ConVar locator_background_thickness_x( "locator_background_thickness_x", "12" );
ConVar locator_background_thickness_y( "locator_background_thickness_y", "12" );
ConVar locator_background_shift_x( "locator_background_shift_x", "0" );
ConVar locator_background_shift_y( "locator_background_shift_y", "0" );
ConVar locator_background_border_color( "locator_background_border_color", "32 32 32 64" );
ConVar locator_icon_max_size_non_ss( "locator_icon_max_size_non_ss", "64" );
ConVar locator_icon_min_size_non_ss( "locator_icon_min_size_non_ss", "16" );
ConVar locator_lerp_rest( "locator_lerp_rest", "0.5" );
ConVar locator_start_at_crosshair( "locator_start_at_crosshair", "0" );
ConVar locator_target_offset_x( "locator_target_offset_x", "0" );
ConVar locator_target_offset_y( "locator_target_offset_y", "0" );
ConVar locator_topdown_style( "locator_topdown_style", "0" );
ConVar sv_portal_placement_debug( "sv_portal_placement_debug", "0" );

void PlayerPickupObject( C_BasePlayer *, C_BaseEntity * ) {}
CBasePlayer *GetPlayerHoldingEntity( const C_BaseEntity * ) { return NULL; }
Vector Pickup_DefaultPhysGunLaunchVelocity( const Vector &, float ) { return vec3_origin; }

CGrabController::CGrabController() {}
CGrabController::~CGrabController() {}
void CGrabController::AttachEntity( CBasePlayer *, CBaseEntity *, IPhysicsObject *, bool, const Vector &, bool ) {}
void CGrabController::AttachEntityVM( CBasePlayer *, CBaseEntity *, IPhysicsObject *, bool, const Vector &, bool ) {}
bool CGrabController::DetachEntity( bool ) { return false; }
bool CGrabController::DetachEntityVM( bool ) { return false; }
void CGrabController::DetachUnknownEntity() {}
bool CGrabController::UpdateObject( CBasePlayer *, float, bool ) { return false; }
bool CGrabController::UpdateObjectVM( CBasePlayer *, float ) { return false; }
IMotionEvent::simresult_e CGrabController::Simulate( IPhysicsMotionController *, IPhysicsObject *, float, Vector &, AngularImpulse & ) { return SIM_NOTHING; }
void CGrabController::ClientApproachTarget( CBasePlayer * ) {}

C_PlayerHeldObjectClone::~C_PlayerHeldObjectClone() {}
bool C_PlayerHeldObjectClone::InitClone( C_BaseEntity *, C_BasePlayer *, bool, C_PlayerHeldObjectClone * ) { return false; }
void C_PlayerHeldObjectClone::ClientThink() {}
bool C_PlayerHeldObjectClone::OnInternalDrawModel( ClientModelRenderInfo_t * ) { return false; }
int C_PlayerHeldObjectClone::DrawModel( int, const RenderableInstance_t & ) { return 0; }
bool C_PlayerHeldObjectClone::HasPreferredCarryAnglesForPlayer( C_BasePlayer * ) { return false; }
QAngle C_PlayerHeldObjectClone::PreferredCarryAngles() { return vec3_angle; }
