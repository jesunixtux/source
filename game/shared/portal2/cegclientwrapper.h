//========= Copyright (c) 2010, Valve Corporation, All rights reserved. ============//
//
// Purpose: Placeholder for the licensed CEG library wrapper. The real wrapper is
//			shipped in a separate (not public) SDK component; this stub keeps the
//			compile-time constants the code actually needs and no-ops the rest.
//
//=============================================================================//

#ifndef CEGCLIENTWRAPPER_H
#define CEGCLIENTWRAPPER_H

#ifdef _WIN32
#pragma once
#endif

#include "paint_enum.h"
#include "bspflags.h"

namespace CEG
{
	// These are the compile-time constants the P2 code was built against.
	// They are resolved from the paint enum and bsp flags:
	//   - PaintSpeedPower     -> SPEED_POWER
	//   - PaintBouncePower    -> BOUNCE_POWER
	//   - PaintCleanserPower  -> CLEANSER_POWER
	//   - PaintReflectPower   -> REFLECT_POWER
	//   - PaintPortalPower    -> PORTAL_POWER
	//   - SurfNoPortalFlag    -> SURF_NOPORTAL
	//   - SurfNoPaintFlag     -> SURF_NODECALS (P2 SDK: #define SURF_NOPAINT SURF_NODECALS)
	//   - SurfNoportalFlag    -> SURF_NOPORTAL
	static const int PaintSpeedPower	= SPEED_POWER;
	static const int PaintBouncePower	= BOUNCE_POWER;
	static const int PaintCleanserPower = CLEANSER_POWER;
	static const int PaintReflectPower	= REFLECT_POWER;
	static const int PaintPortalPower	= PORTAL_POWER;
	static const int SurfNoPortalFlag	= SURF_NOPORTAL;
	static const int SurfNoPaintFlag	= SURF_NODECALS;
	static const int SurfNoportalFlag	= SURF_NOPORTAL;
}

// The CEG library is a licensed component that is not shipped in the public SDK.
// Every macro it supplies is resolved to a safe no-op here.
#define CEG_NOINLINE
#define CEG_GCV_PRE()			((void)0)
#define CEG_GCV_POST()			((void)0)
#define CEG_GCV_NAME(_func)		((void)0)
#define CEG_GCV_START()			
#define CEG_GCV_FINISH()		

// The port's call sites omit the trailing ';' (the licensed CEG library emits a
// full statement), so these must expand to nothing rather than to a declaration.
#define CEG_PROTECT_FUNCTION( _func )
#define CEG_PROTECT_MEMBER_FUNCTION( _class_method )
#define CEG_PROTECT_STATIC_MEMBER_FUNCTION( _name, _func )
#define CEG_PROTECT_VIRTUAL_FUNCTION( _name )

#define CEG_GET_CONSTANT_VALUE( _constant )	( CEG::_constant )

#define RANDOM_CEG_TEST_SECRET()			((void)0)
#define RANDOM_CEG_TEST_SECRET_PERIOD( _first, _period )

// SteamWorks is not linked into this build.
#define STEAMWORKS_TESTSECRETALWAYS()

#endif // CEGCLIENTWRAPPER_H