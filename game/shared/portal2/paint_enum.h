//========= Copyright © 1996-2010, Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the paint power types.
//
//=============================================================================//

#ifndef PAINT_ENUM_H
#define PAINT_ENUM_H

enum PaintPowerType
{
	INVALID_PAINT_POWER = -1,
	SPEED_POWER = 0,
	BOUNCE_POWER,
	CLEANSER_POWER,
	REFLECT_POWER,
	PORTAL_POWER,
	VISUALIZER_POWER,
	NO_POWER,
	PAINT_POWER_TYPE_COUNT,

	// Force an extra slot so we can always allocate one past the count, even
	// when NO_POWER is the enum value being used.
	PAINT_POWER_TYPE_COUNT_PLUS_NO_POWER
};

#endif // ifndef PAINT_ENUM_H