//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Send-prop priority helpers (Portal 2 multiplayer). Reconstructed
//          from private Portal 2 source; not used by this port, provided for
//          include compatibility.
//
//=============================================================================//

#ifndef SENDPROP_PRIORITIES_H
#define SENDPROP_PRIORITIES_H
#ifdef _WIN32
#pragma once
#endif

// Origin send priorities used by CPortal_Player's high/low-res origin tables.
#define SENDPROP_LOCALPLAYER_ORIGINXY_PRIORITY		((byte)28)
#define SENDPROP_LOCALPLAYER_ORIGINZ_PRIORITY		((byte)28)
#define SENDPROP_NONLOCALPLAYER_ORIGINXY_PRIORITY	((byte)4)
#define SENDPROP_NONLOCALPLAYER_ORIGINZ_PRIORITY	((byte)4)

#endif // SENDPROP_PRIORITIES_H