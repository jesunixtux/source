#ifndef ENGINE_PAINT_CPU_H
#define ENGINE_PAINT_CPU_H

#include "mathlib/vector.h"
#include "tier1/utlvector.h"

struct model_t;

bool PaintCPU_HasPaintmap();
bool PaintCPU_SpherePaintSurface( const model_t *model, const Vector &position, unsigned char color, float radius );
void PaintCPU_SphereTracePaintSurface( const model_t *model, const Vector &position, const Vector &normal, float radius, CUtlVector<unsigned char> &colors );
void PaintCPU_RemoveAllPaint();
void PaintCPU_PaintAllSurfaces( unsigned char color );
void PaintCPU_RemovePaint( const model_t *model );

#endif
