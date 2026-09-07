#include "../materialsystem/shaderapidx9/shadercomboindex.h"
#include <assert.h>
#include <stdio.h>

int main()
{
    const uint32_t bases[] = { 0, 576, 2265372288u };
    for (uint32_t base : bases) {
        const int32_t legacyBase = static_cast<int32_t>(base);
        assert(ShaderStaticRecordIndex(legacyBase, 288) == base / 288);
        for (uint32_t dynamic = 0; dynamic < 288; ++dynamic) {
            uint32_t result = 0;
            assert(DecodeShaderDynamicIndex(true, dynamic, legacyBase, 288, result));
            assert(result == dynamic);
            assert(DecodeShaderDynamicIndex(false, base + dynamic, legacyBase, 288, result));
            assert(result == dynamic);
        }
        uint32_t result = 0;
        assert(!DecodeShaderDynamicIndex(true, 288, legacyBase, 288, result));
        assert(!DecodeShaderDynamicIndex(false, base + 288, legacyBase, 288, result));
        assert(!DecodeShaderDynamicIndex(false, base - 1, legacyBase, 288, result));
        assert(!DecodeShaderDynamicIndex(true, 0, legacyBase, 0, result));
        assert(!DecodeShaderDynamicIndex(true, 0, legacyBase, -1, result));
    }
    puts("PASS: VCS 5/6, low/high-bit bases, dynamic indices and invalid bounds");
}
