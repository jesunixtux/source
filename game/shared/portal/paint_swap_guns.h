//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Paint gun swapping (multiplayer). Reconstructed from private Portal 2
//          source; the gun-swap flow is not part of this port, so the header is
//          provided for include compatibility.
//
//=============================================================================//

#ifndef PAINT_SWAP_GUNS_H
#define PAINT_SWAP_GUNS_H
#ifdef _WIN32
#pragma once
#endif

class CPortal_Player;

// Stand-ins for the private Portal 2 gun-swap helpers. The cooperative
// swap flow is not ported, so these are provided to keep call sites compiling.
inline bool CheckSwapProximity( CPortal_Player *pPlayer, CPortal_Player *pOtherPlayer )
{
	return true;
}

inline void SwapPaintAndPortalGuns( CPortal_Player *pPlayer, CPortal_Player *pOtherPlayer )
{
}

#endif // PAINT_SWAP_GUNS_H