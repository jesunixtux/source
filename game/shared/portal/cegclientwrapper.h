// Shared Portal compatibility entry point for Valve's unavailable CEG wrapper.
// Keep the implementation in portal2, where the imported constants live, but
// make quoted includes from shared Portal sources independent of target-specific
// VPC include paths. This preserves both Portal 1 and Portal 2 builds.
#ifndef PORTAL_SHARED_CEGCLIENTWRAPPER_H
#define PORTAL_SHARED_CEGCLIENTWRAPPER_H

#include "../portal2/cegclientwrapper.h"

#endif // PORTAL_SHARED_CEGCLIENTWRAPPER_H
