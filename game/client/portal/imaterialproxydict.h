// Compatibility header for Portal 2 material proxy sources.
#ifndef PORTAL_IMATERIAL_PROXY_DICT_H
#define PORTAL_IMATERIAL_PROXY_DICT_H

#include "materialsystem/IMaterialProxy.h"

#ifndef MATERIAL_VAR_ALPHA_MODIFIED_BY_PROXY
#define MATERIAL_VAR_ALPHA_MODIFIED_BY_PROXY MATERIAL_VAR_VERTEXALPHA
#endif

#ifndef EXPOSE_MATERIAL_PROXY
#define PORTAL_PROXY_STRINGIZE_INNER(value) #value
#define PORTAL_PROXY_STRINGIZE(value) PORTAL_PROXY_STRINGIZE_INNER(value)
#define EXPOSE_MATERIAL_PROXY( className, proxyName ) \
	EXPOSE_INTERFACE( className, IMaterialProxy, PORTAL_PROXY_STRINGIZE(proxyName) IMATERIAL_PROXY_INTERFACE_VERSION )
#endif

#endif
