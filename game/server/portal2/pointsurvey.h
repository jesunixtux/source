//===== Copyright 1996-2006, Valve Corporation, All rights reserved. ======//
//
// Purpose: Point entity that receives "survey_done" client messages and
//          forwards completion to map logic.
//
// Reconstructed from private Portal 2 source; minimal API used by game code.
//
// $NoKeywords: $
//===========================================================================//

#ifndef POINTSURVEY_H
#define POINTSURVEY_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "entityoutput.h"

class CPointSurvey : public CBaseEntity
{
	DECLARE_CLASS( CPointSurvey, CBaseEntity );
	DECLARE_DATADESC();

public:
	virtual void Spawn( void );
	void OnSurveyCompleted( void );

private:
	COutputEvent m_OnSurveyCompleted;
};

#endif // POINTSURVEY_H