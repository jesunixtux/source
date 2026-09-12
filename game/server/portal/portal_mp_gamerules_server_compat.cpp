#include "cbase.h"
#include "portal_mp_gamerules.h"
CPortalMPGameRules *g_pPortalMPGameRules=NULL;
void CPortalMPGameRules::SaveMPStats(){}
void CPortalMPGameRules::PlayerWinRPS(CBasePlayer*){}
bool CPortalMPGameRules::IsCommunityCoopHub(){return false;}
void CPortalMPGameRules::SetAllMapsComplete(bool,int){}
void CPortalMPGameRules::SetMapCompleteData(int){}
bool CPortalMPGameRules::SupressSpawnPortalgun(int){return false;}
bool CPortalMPGameRules::IsVS(){return false;}
