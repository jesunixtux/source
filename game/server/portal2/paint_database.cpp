//========= Copyright (c) Valve Corporation, All rights reserved. ============//
//
// Purpose: Global paint database implementation (reconstructed).
//
//=============================================================================//

#include "cbase.h"
#include "paint_database.h"
#include "paintable_entity.h"
#include "portal_util_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CPaintDatabase PaintDatabase;

bool CPaintDatabase::AddPaint( CBaseEntity *entity, const Vector &position, const Vector &normal,
	PaintPowerType type, float radius, float alpha )
{
	if ( !entity || type < 0 || type > NO_POWER )
		return false;
	if ( entity->IsBSPModel() )
		return UTIL_PaintBrushEntity( entity, position + normal * 0.1f, type, radius, alpha ) > 0;
	IPaintableEntity *paintable = dynamic_cast<IPaintableEntity *>( entity );
	if ( !paintable )
		return false;
	if ( m_paintedEntities.Find( entity->GetRefEHandle() ) == m_paintedEntities.InvalidIndex() )
		m_paintedEntities.AddToTail( entity->GetRefEHandle() );
	paintable->Paint( type, position );
	return true;
}

void CPaintDatabase::RemoveAllPaint()
{
	for ( int i = 0; i < m_paintedEntities.Count(); ++i )
	{
		CBaseEntity *entity = gEntList.GetBaseEntity( m_paintedEntities[i] );
		IPaintableEntity *paintable = entity ? dynamic_cast<IPaintableEntity *>( entity ) : NULL;
		if ( paintable ) paintable->CleansePaint();
	}
	m_paintedEntities.RemoveAll();
	engine->RemoveAllPaint();
}
