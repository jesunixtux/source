//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef OPTIONSSUBOTHERS_H
#define OPTIONSSUBOTHERS_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyPage.h"

class CCvarToggleCheckButton;

class COptionsSubOthers : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( COptionsSubOthers, vgui::PropertyPage );

public:
	COptionsSubOthers( vgui::Panel *parent );
	~COptionsSubOthers();

	virtual void OnResetData();
	virtual void OnApplyChanges();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

protected:
	virtual void OnControlModified();

private:
	CCvarToggleCheckButton *m_pConsoleButtonCheckBox;
};

#endif // OPTIONSSUBOTHERS_H