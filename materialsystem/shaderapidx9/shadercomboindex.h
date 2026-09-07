#ifndef SHADERCOMBOINDEX_H
#define SHADERCOMBOINDEX_H
#include <stdint.h>

// VCS IDs are unsigned even though the legacy runtime stores bases in int32.
inline uint32_t ShaderStaticRecordIndex( int32_t base, uint32_t dynamicCount )
{
    // Caller must validate the cache's dynamic count before allocation/use.
    return static_cast<uint32_t>( base ) / dynamicCount;
}

inline bool DecodeShaderDynamicIndex( bool version6, uint32_t storedID,
    int32_t base, int32_t count, uint32_t &index )
{
    index = version6 ? storedID : storedID - static_cast<uint32_t>( base );
    return count > 0 && index < static_cast<uint32_t>( count );
}
#endif
