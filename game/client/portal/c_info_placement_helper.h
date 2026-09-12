//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side stub for the placement helper entity used by the
//          portal gun. The full entity logic lives server-side; the client
//          only needs the type and a no-op finder for shared code to compile.
//
//=============================================================================//

#ifndef C_INFO_PLACEMENT_HELPER_H
#define C_INFO_PLACEMENT_HELPER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

class C_InfoPlacementHelper;

inline C_InfoPlacementHelper *UTIL_FindPlacementHelper( const Vector &vecPosition, C_BasePlayer *pPlayer )
{
	return NULL;
}

#endif // C_INFO_PLACEMENT_HELPER_H
