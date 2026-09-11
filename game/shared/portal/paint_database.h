//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: Global paint database for the server - tracks paintable entities.
//			Reconstructed minimal implementation for the open-source port.
//
//=============================================================================//

#ifndef PAINT_DATABASE_H
#define PAINT_DATABASE_H

#ifdef _WIN32
#pragma once
#endif

#include "paint_color_manager.h"

class CPaintDatabase
{
public:
	CPaintDatabase() {}

	// Removes all paint from every surface
	void RemoveAllPaint();
};

// Global database instance (defined in server code)
extern CPaintDatabase PaintDatabase;

#endif // PAINT_DATABASE_H