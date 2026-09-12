#ifndef C_BASEPROJECTEDENTITY_H
#define C_BASEPROJECTEDENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"

class C_BaseProjectedEntity : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_BaseProjectedEntity, C_BaseEntity );

	C_BaseProjectedEntity( void ) {}

	// Stub: the real implementation checks all projected entities for projection changes.
	static void TestAllForProjectionChanges( void ) {}
};

#define CBaseProjectedEntity C_BaseProjectedEntity

#endif // C_BASEPROJECTEDENTITY_H
