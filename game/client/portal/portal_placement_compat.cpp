#include "cbase.h"
#include "portal_placement.h"

int AllEdictsAlongRay( CBaseEntity **, int, const Ray_t &, int ) { return 0; }
bool IsPortalIntersectingNoPortalVolume( const Vector &, const QAngle &, const Vector &, float, float ) { return false; }
PortalPlacementResult_t IsPortalOverlappingOtherPortals( const CProp_Portal *, const Vector &, const QAngle &, float, float, bool, bool ) { return PORTAL_PLACEMENT_SUCCESS; }
PortalPlacementResult_t VerifyPortalPlacementAndFizzleBlockingPortals( const CProp_Portal *, Vector &, QAngle &, float, float, PortalPlacedBy_t ) { return PORTAL_PLACEMENT_SUCCESS; }
bool PortalPlacementSucceeded( PortalPlacementResult_t result )
{
	return result == PORTAL_PLACEMENT_SUCCESS || result == PORTAL_PLACEMENT_USED_HELPER || result == PORTAL_PLACEMENT_BUMPED;
}
