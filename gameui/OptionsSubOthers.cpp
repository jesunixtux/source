//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Optional/miscellaneous settings page. Built without a layout file
//          so it works on any mod without touching the game resources.
//
// $NoKeywords: $
//
//=============================================================================//

#include "OptionsSubOthers.h"
#include "CvarToggleCheckButton.h"

#include "vgui/IScheme.h"
#include "vgui_controls/CheckButton.h"

#include <tier0/memdbgon.h>

using namespace vgui;

COptionsSubOthers::COptionsSubOthers( vgui::Panel *parent )
	: PropertyPage( parent, NULL )
{
	m_pConsoleButtonCheckBox = new CCvarToggleCheckButton(
		this, "ConsoleButton", "Enable Console Button", "ui_console_button" );
	m_pConsoleButtonCheckBox->AddActionSignalTarget( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubOthers::~COptionsSubOthers()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubOthers::OnResetData()
{
	m_pConsoleButtonCheckBox->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubOthers::OnApplyChanges()
{
	m_pConsoleButtonCheckBox->ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubOthers::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	int x = scheme()->GetProportionalScaledValueEx( GetScheme(), 24 );
	int y = scheme()->GetProportionalScaledValueEx( GetScheme(), 24 );
	int wide = scheme()->GetProportionalScaledValueEx( GetScheme(), 200 );
	int tall = scheme()->GetProportionalScaledValueEx( GetScheme(), 22 );

	m_pConsoleButtonCheckBox->SetPos( x, y );
	m_pConsoleButtonCheckBox->SetSize( wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubOthers::OnControlModified()
{
	PostActionSignal( new KeyValues( "ApplyButtonEnable" ) );
}