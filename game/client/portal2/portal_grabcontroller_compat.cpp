#include "cbase.h"
#include "portal2/portal_grabcontroller_shared.h"

CGrabController::CGrabController() {}
CGrabController::~CGrabController() {}

C_PlayerHeldObjectClone::~C_PlayerHeldObjectClone() {}
bool C_PlayerHeldObjectClone::InitClone( C_BaseEntity *, C_BasePlayer *, bool, C_PlayerHeldObjectClone * ) { return false; }
void C_PlayerHeldObjectClone::ClientThink() {}
bool C_PlayerHeldObjectClone::OnInternalDrawModel( ClientModelRenderInfo_t * ) { return false; }
int C_PlayerHeldObjectClone::DrawModel( int, const RenderableInstance_t & ) { return 0; }
bool C_PlayerHeldObjectClone::HasPreferredCarryAnglesForPlayer( C_BasePlayer * ) { return false; }
QAngle C_PlayerHeldObjectClone::PreferredCarryAngles() { return vec3_angle; }
