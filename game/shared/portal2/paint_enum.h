//========= Copyright © 1996-2010, Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the paint power types.
//
//=============================================================================//

#ifndef PAINT_ENUM_H
#define PAINT_ENUM_H

enum PaintPowerType
{
	BOUNCE_POWER = 0,
	REFLECT_POWER,
	SPEED_POWER,
	PORTAL_POWER,
	NO_POWER,
	PAINT_POWER_TYPE_COUNT = NO_POWER,
	PAINT_POWER_TYPE_COUNT_PLUS_NO_POWER,
	INVALID_PAINT_POWER = -1
};

#endif // ifndef PAINT_ENUM_H
