#include "server_pch.h"
#include "paint_cpu.h"
#include "common.h"
#include "gl_model_private.h"
#include "../game/shared/portal2/paint_enum.h"

namespace
{
struct PaintStamp
{
	const model_t *model;
	int surface;
	Vector center;
	float radius;
	unsigned char color;
};

CUtlVector<PaintStamp> stamps;
const model_t *paintWorld = NULL;
int nextStamp = 0;
unsigned char allSurfacesColor = NO_POWER;

void CheckWorld()
{
	if ( paintWorld == host_state.worldmodel )
		return;
	stamps.RemoveAll();
	paintWorld = host_state.worldmodel;
	nextStamp = 0;
	allSurfacesColor = NO_POWER;
}

Vector ClosestPointOnTriangle( const Vector &point, const Vector &a, const Vector &b, const Vector &c )
{
	const Vector ab = b - a, ac = c - a, ap = point - a;
	const float d1 = DotProduct( ab, ap ), d2 = DotProduct( ac, ap );
	if ( d1 <= 0 && d2 <= 0 ) return a;
	const Vector bp = point - b;
	const float d3 = DotProduct( ab, bp ), d4 = DotProduct( ac, bp );
	if ( d3 >= 0 && d4 <= d3 ) return b;
	const float vc = d1 * d4 - d3 * d2;
	if ( vc <= 0 && d1 >= 0 && d3 <= 0 ) return a + ab * ( d1 / ( d1 - d3 ) );
	const Vector cp = point - c;
	const float d5 = DotProduct( ab, cp ), d6 = DotProduct( ac, cp );
	if ( d6 >= 0 && d5 <= d6 ) return c;
	const float vb = d5 * d2 - d1 * d6;
	if ( vb <= 0 && d2 >= 0 && d6 <= 0 ) return a + ac * ( d2 / ( d2 - d6 ) );
	const float va = d3 * d6 - d5 * d4;
	if ( va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0 )
		return b + ( c - b ) * ( ( d4 - d3 ) / ( d4 - d3 + d5 - d6 ) );
	const float sum = va + vb + vc;
	if ( sum <= 0 ) return a;
	return a + ab * ( vb / sum ) + ac * ( vc / sum );
}

bool SurfaceTouchesSphere( worldbrushdata_t *brush, int surfaceIndex, const Vector &center, float radius )
{
	SurfaceHandle_t surface = SurfaceHandleFromIndex( surfaceIndex, brush );
	if ( MSurf_Flags( surface ) & ( SURFDRAW_NODRAW | SURFDRAW_SKY | SURFDRAW_TRANS | SURFDRAW_NOLIGHT ) )
		return false;
	if ( SurfaceHasDispInfo( surface ) || MSurf_VertCount( surface ) < 3 )
		return false;
	const VPlane plane = MSurf_GetForwardFacingPlane( surface, brush );
	const float distance = plane.DistTo( center );
	if ( distance < -0.5f || distance > radius )
		return false;
	const int first = MSurf_FirstVertIndex( surface );
	const Vector &a = brush->vertexes[brush->vertindices[first]].position;
	for ( int i = 1; i + 1 < MSurf_VertCount( surface ); ++i )
	{
		const Vector &b = brush->vertexes[brush->vertindices[first + i]].position;
		const Vector &c = brush->vertexes[brush->vertindices[first + i + 1]].position;
		if ( ( center - ClosestPointOnTriangle( center, a, b, c ) ).LengthSqr() <= radius * radius )
			return true;
	}
	return false;
}
}

bool PaintCPU_HasPaintmap()
{
	CheckWorld();
	return !Q_stricmp( COM_GetModDirectory(), "portal2" ) && paintWorld &&
		paintWorld->type == mod_brush && paintWorld->brush.pShared;
}

bool PaintCPU_SpherePaintSurface( const model_t *model, const Vector &position, unsigned char color, float radius )
{
	if ( !PaintCPU_HasPaintmap() || !model || model->type != mod_brush ||
		!model->brush.pShared || color > NO_POWER || radius <= 0 )
		return false;
	worldbrushdata_t *brush = model->brush.pShared;
	bool painted = false;
	for ( int i = model->brush.firstmodelsurface; i < model->brush.firstmodelsurface + model->brush.nummodelsurfaces; ++i )
	{
		if ( i < 0 || i >= brush->numsurfaces || !SurfaceTouchesSphere( brush, i, position, radius ) )
			continue;
		PaintStamp stamp = { model, i, position, radius, color };
		// shortcut: recent splats replace nearby ones; use paint luxels for pixel-accurate coating and rendering.
		bool replaced = false;
		for ( int j = stamps.Count() - 1; j >= 0 && j > stamps.Count() - 64; --j )
		{
			if ( stamps[j].model == model && stamps[j].surface == i &&
				( stamps[j].center - position ).LengthSqr() < 64.0f )
			{
				stamps[j] = stamp;
				replaced = true;
				break;
			}
		}
		if ( !replaced )
		{
			if ( stamps.Count() < 65536 ) stamps.AddToTail( stamp );
			else { stamps[nextStamp] = stamp; nextStamp = ( nextStamp + 1 ) % stamps.Count(); }
		}
		painted = true;
	}
	return painted;
}

void PaintCPU_SphereTracePaintSurface( const model_t *model, const Vector &position, const Vector &normal, float radius, CUtlVector<unsigned char> &colors )
{
	colors.RemoveAll();
	if ( !PaintCPU_HasPaintmap() || !model || model->type != mod_brush || !model->brush.pShared || radius <= 0 )
		return;
	worldbrushdata_t *brush = model->brush.pShared;
	for ( int i = stamps.Count() - 1; i >= 0; --i )
	{
		const PaintStamp &stamp = stamps[i];
		if ( stamp.model != model || ( stamp.center - position ).LengthSqr() > ( stamp.radius + radius ) * ( stamp.radius + radius ) )
			continue;
		SurfaceHandle_t surface = SurfaceHandleFromIndex( stamp.surface, brush );
		if ( DotProduct( normal, MSurf_GetForwardFacingPlane( surface, brush ).m_Normal ) < 0.9f ||
			!SurfaceTouchesSphere( brush, stamp.surface, position + normal * 0.1f, radius ) )
			continue;
		colors.AddToTail( stamp.color );
		return;
	}
	if ( allSurfacesColor != NO_POWER ) colors.AddToTail( allSurfacesColor );
}

void PaintCPU_RemoveAllPaint()
{
	stamps.RemoveAll();
	nextStamp = 0;
	allSurfacesColor = NO_POWER;
}

void PaintCPU_PaintAllSurfaces( unsigned char color )
{
	PaintCPU_RemoveAllPaint();
	if ( color <= NO_POWER ) allSurfacesColor = color;
}

void PaintCPU_RemovePaint( const model_t *model )
{
	for ( int i = stamps.Count() - 1; i >= 0; --i )
		if ( stamps[i].model == model ) stamps.FastRemove( i );
	nextStamp = 0;
}
