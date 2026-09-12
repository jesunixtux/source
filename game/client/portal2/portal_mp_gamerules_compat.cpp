#include "cbase.h"
#include "portal/portal_mp_gamerules.h"

C_PortalMPGameRules *g_pPortalMPGameRules = NULL;
bool C_PortalMPGameRules::Is2GunsCoOp() { return false; }
bool C_PortalMPGameRules::IsVS() { return false; }
bool C_PortalMPGameRules::IsCreditsMap() { return false; }
void C_PortalMPGameRules::LoadMapCompleteData() {}
