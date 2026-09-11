//===== Copyright 1996-2006, Valve Corporation, All rights reserved. ======//
//
// Purpose: Point entity that receives "survey_done" client messages and
//          forwards completion to map logic.
//
// Reconstructed from private Portal 2 source; minimal API used by game code.
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "pointsurvey.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( point_survey, CPointSurvey );

BEGIN_DATADESC( CPointSurvey )

	DEFINE_OUTPUT( m_OnSurveyCompleted, "OnSurveyCompleted" ),

END_DATADESC()

void CPointSurvey::Spawn( void )
{
	BaseClass::Spawn();
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	AddEffects( EF_NODRAW );
}

void CPointSurvey::OnSurveyCompleted( void )
{
	m_OnSurveyCompleted.FireOutput( NULL, this );
}