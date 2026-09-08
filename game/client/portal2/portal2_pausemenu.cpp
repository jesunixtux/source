//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Minimal in-client pause menu for the experimental Portal 2 ARM64
// build. This is intentionally independent of Portal 2's original GameUI, so
// it still works while the full menu stack is incomplete.
//
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "tier0/icommandline.h"
#include "vgui_int.h"
#include "tier1/convar.h"
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static ConVar portal2_ui_language( "portal2_ui_language", "auto", FCVAR_ARCHIVE,
	"Language for the Portal 2 ARM64 compatibility menu (auto, english, spanish, brazilian, french, german, italian, russian, polish)." );

static const char *const s_ppszPortal2Languages[] =
{
	"english", "spanish", "brazilian", "french", "german", "italian", "russian", "polish"
};

static const char *Portal2NormalizeMenuLanguage( const char *pLanguage )
{
	if ( !pLanguage || !pLanguage[0] || !Q_stricmp( pLanguage, "auto" ) )
	{
		ConVarRef clLanguage( "cl_language" );
		pLanguage = clLanguage.IsValid() ? clLanguage.GetString() : "english";
	}

	for ( int i = 0; i < ARRAYSIZE( s_ppszPortal2Languages ); ++i )
	{
		if ( !Q_stricmp( pLanguage, s_ppszPortal2Languages[i] ) )
			return s_ppszPortal2Languages[i];
	}
	return "english";
}

static const char *Portal2CurrentMenuLanguage()
{
	return Portal2NormalizeMenuLanguage( portal2_ui_language.GetString() );
}

static void Portal2RefreshMenuLanguage()
{
	if ( !g_pVGuiLocalize )
		return;

	// Each complete catalog is loaded after English, so changing the language
	// cannot leave stale tokens from the previous selection in the VGUI table.
	g_pVGuiLocalize->AddFile( "resource/portal2_arm64_english.txt", "GAME", false );
	const char *pLanguage = Portal2CurrentMenuLanguage();
	if ( Q_stricmp( pLanguage, "english" ) )
	{
		char fileName[MAX_PATH];
		Q_snprintf( fileName, sizeof( fileName ), "resource/portal2_arm64_%s.txt", pLanguage );
		g_pVGuiLocalize->AddFile( fileName, "GAME", false );
	}
}

static void Portal2SetMenuLanguage( const char *pLanguage )
{
	portal2_ui_language.SetValue( Portal2NormalizeMenuLanguage( pLanguage ) );
	Portal2RefreshMenuLanguage();
}

class CPortal2FallbackPausePanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CPortal2FallbackPausePanel, vgui::Panel );

public:
	CPortal2FallbackPausePanel( vgui::Panel *pParent );

	void Toggle();
	void ShowMenu( bool bShow );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnMouseReleased( vgui::MouseCode code );
	virtual void Paint();
	virtual void PerformLayout();

private:
	enum PauseAction_t
	{
		ACTION_RESUME = 0,
		ACTION_RESTART,
		ACTION_DISCONNECT,
		ACTION_QUIT,
		ACTION_LANGUAGE,
		ACTION_COUNT
	};

	void RunAction( PauseAction_t action );
	void EnsureFonts();
	void DrawText( const wchar_t *pText, int x, int y, vgui::HFont font, Color color );
	void DrawToken( const char *pToken, int x, int y, vgui::HFont font, Color color );
	void CycleLanguage();
	int GetActionAtPos( int x, int y ) const;

	vgui::HFont m_hTitleFont;
	vgui::HFont m_hTextFont;
	int m_nMenuX;
	int m_nMenuY;
	int m_nMenuW;
	int m_nMenuH;
	int m_nButtonH;
	int m_nHoverAction;
};

static CPortal2FallbackPausePanel *g_pPortal2PausePanel = NULL;
static bool g_bOpenPauseMenuWhenReady = false;

CPortal2FallbackPausePanel::CPortal2FallbackPausePanel( vgui::Panel *pParent )
	: BaseClass( pParent, "Portal2FallbackPausePanel" )
{
	m_hTitleFont = vgui::INVALID_FONT;
	m_hTextFont = vgui::INVALID_FONT;
	m_nMenuX = 0;
	m_nMenuY = 0;
	m_nMenuW = 460;
	m_nMenuH = 368;
	m_nButtonH = 42;
	m_nHoverAction = -1;

	SetVisible( false );
	SetPaintEnabled( true );
	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );
	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );
	SetProportional( false );
}

void CPortal2FallbackPausePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	EnsureFonts();
}

void CPortal2FallbackPausePanel::EnsureFonts()
{
	if ( m_hTitleFont == vgui::INVALID_FONT )
	{
		// CFontManager's first dynamically-created font has handle 0 on this
		// POSIX renderer, while MatSystemSurface treats 0 as INVALID_FONT.
		// Reserve that handle locally; this is deliberately not a global font
		// manager change, because it would affect every Source game.
		vgui::surface()->CreateFont();
		m_hTitleFont = vgui::surface()->CreateFont();
		if ( !vgui::surface()->SetFontGlyphSet( m_hTitleFont, "Helvetica", 30, 700, 0, 0,
			vgui::ISurface::FONTFLAG_ANTIALIAS | vgui::ISurface::FONTFLAG_DROPSHADOW ) )
		{
			vgui::surface()->SetFontGlyphSet( m_hTitleFont, "Arial", 30, 700, 0, 0,
				vgui::ISurface::FONTFLAG_ANTIALIAS | vgui::ISurface::FONTFLAG_DROPSHADOW );
		}
	}

	if ( m_hTextFont == vgui::INVALID_FONT )
	{
		m_hTextFont = vgui::surface()->CreateFont();
		if ( !vgui::surface()->SetFontGlyphSet( m_hTextFont, "Helvetica", 22, 500, 0, 0,
			vgui::ISurface::FONTFLAG_ANTIALIAS | vgui::ISurface::FONTFLAG_DROPSHADOW ) )
		{
			vgui::surface()->SetFontGlyphSet( m_hTextFont, "Arial", 22, 500, 0, 0,
				vgui::ISurface::FONTFLAG_ANTIALIAS | vgui::ISurface::FONTFLAG_DROPSHADOW );
		}
	}
}

void CPortal2FallbackPausePanel::Toggle()
{
	ShowMenu( !IsVisible() );
}

void CPortal2FallbackPausePanel::ShowMenu( bool bShow )
{
	SetVisible( bShow );

	if ( bShow )
	{
		MoveToFront();
		RequestFocus();
		vgui::input()->SetMouseFocus( GetVPanel() );
		vgui::surface()->SetCursorAlwaysVisible( true );
		engine->ClientCmd_Unrestricted( "setpause\n" );
	}
	else
	{
		vgui::surface()->SetCursorAlwaysVisible( false );
		engine->ClientCmd_Unrestricted( "unpause\n" );
	}
}

void CPortal2FallbackPausePanel::RunAction( PauseAction_t action )
{
	switch ( action )
	{
	case ACTION_RESUME:
		ShowMenu( false );
		break;
	case ACTION_RESTART:
		engine->ClientCmd_Unrestricted( "restart\n" );
		ShowMenu( false );
		break;
	case ACTION_DISCONNECT:
		engine->ClientCmd_Unrestricted( "disconnect\n" );
		ShowMenu( false );
		break;
	case ACTION_QUIT:
		engine->ClientCmd_Unrestricted( "quit\n" );
		break;
	case ACTION_LANGUAGE:
		CycleLanguage();
		break;
	default:
		break;
	}
}

void CPortal2FallbackPausePanel::CycleLanguage()
{
	const char *pCurrent = Portal2CurrentMenuLanguage();
	for ( int i = 0; i < ARRAYSIZE( s_ppszPortal2Languages ); ++i )
	{
		if ( !Q_stricmp( pCurrent, s_ppszPortal2Languages[i] ) )
		{
			Portal2SetMenuLanguage( s_ppszPortal2Languages[( i + 1 ) % ARRAYSIZE( s_ppszPortal2Languages )] );
			return;
		}
	}
	Portal2SetMenuLanguage( "english" );
}

void CPortal2FallbackPausePanel::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( code == KEY_ESCAPE || code == KEY_F10 )
	{
		ShowMenu( false );
		return;
	}

	if ( code >= KEY_1 && code < KEY_1 + ACTION_COUNT )
	{
		RunAction( (PauseAction_t)( code - KEY_1 ) );
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}

void CPortal2FallbackPausePanel::OnMousePressed( vgui::MouseCode code )
{
	if ( code == MOUSE_LEFT )
	{
		RequestFocus();
	}

	BaseClass::OnMousePressed( code );
}

void CPortal2FallbackPausePanel::OnMouseReleased( vgui::MouseCode code )
{
	if ( code == MOUSE_LEFT )
	{
		int x, y;
		vgui::input()->GetCursorPosition( x, y );
		ScreenToLocal( x, y );

		const int nAction = GetActionAtPos( x, y );
		if ( nAction >= 0 )
		{
			RunAction( (PauseAction_t)nAction );
			return;
		}
	}

	BaseClass::OnMouseReleased( code );
}

void CPortal2FallbackPausePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	vgui::surface()->GetScreenSize( wide, tall );
	SetBounds( 0, 0, wide, tall );

	m_nMenuW = MIN( 460, wide - 80 );
	m_nMenuH = 368;
	m_nMenuX = ( wide - m_nMenuW ) / 2;
	m_nMenuY = ( tall - m_nMenuH ) / 2;
	m_nButtonH = 42;
}

void CPortal2FallbackPausePanel::DrawText( const wchar_t *pText, int x, int y, vgui::HFont font, Color color )
{
	if ( font == vgui::INVALID_FONT || !pText )
		return;

	vgui::surface()->DrawSetTextFont( font );
	vgui::surface()->DrawSetTextColor( color );
	vgui::surface()->DrawSetTextPos( x, y );
	vgui::surface()->DrawPrintText( pText, V_wcslen( pText ) );
}

void CPortal2FallbackPausePanel::DrawToken( const char *pToken, int x, int y, vgui::HFont font, Color color )
{
	const wchar_t *pText = g_pVGuiLocalize ? g_pVGuiLocalize->Find( pToken ) : NULL;
	DrawText( pText, x, y, font, color );
}

int CPortal2FallbackPausePanel::GetActionAtPos( int x, int y ) const
{
	const int nButtonX = m_nMenuX + 36;
	const int nButtonW = m_nMenuW - 72;
	const int nFirstButtonY = m_nMenuY + 104;
	const int nGap = 12;

	if ( x < nButtonX || x > nButtonX + nButtonW )
		return -1;

	for ( int i = 0; i < ACTION_COUNT; ++i )
	{
		const int nButtonY = nFirstButtonY + i * ( m_nButtonH + nGap );
		if ( y >= nButtonY && y <= nButtonY + m_nButtonH )
			return i;
	}

	return -1;
}

void CPortal2FallbackPausePanel::Paint()
{
	EnsureFonts();
	PerformLayout();
	Portal2RefreshMenuLanguage();

	int wide, tall;
	GetSize( wide, tall );

	vgui::surface()->DrawSetColor( 0, 0, 0, 180 );
	vgui::surface()->DrawFilledRect( 0, 0, wide, tall );

	vgui::surface()->DrawSetColor( 16, 22, 24, 242 );
	vgui::surface()->DrawFilledRect( m_nMenuX, m_nMenuY, m_nMenuX + m_nMenuW, m_nMenuY + m_nMenuH );
	vgui::surface()->DrawSetColor( 255, 255, 255, 32 );
	vgui::surface()->DrawOutlinedRect( m_nMenuX, m_nMenuY, m_nMenuX + m_nMenuW, m_nMenuY + m_nMenuH );
	vgui::surface()->DrawSetColor( 65, 196, 229, 230 );
	vgui::surface()->DrawFilledRect( m_nMenuX, m_nMenuY, m_nMenuX + m_nMenuW, m_nMenuY + 5 );

	DrawToken( "#P2ARM64_TITLE", m_nMenuX + 34, m_nMenuY + 26, m_hTitleFont, Color( 235, 242, 245, 255 ) );
	DrawToken( "#P2ARM64_SUBTITLE", m_nMenuX + 36, m_nMenuY + 68, m_hTextFont, Color( 150, 198, 216, 255 ) );

	int cx, cy;
	vgui::input()->GetCursorPosition( cx, cy );
	ScreenToLocal( cx, cy );
	m_nHoverAction = GetActionAtPos( cx, cy );

	static const char *s_ppszActions[ACTION_COUNT] =
	{
		"#P2ARM64_RESUME",
		"#P2ARM64_RESTART",
		"#P2ARM64_DISCONNECT",
		"#P2ARM64_QUIT",
		"#P2ARM64_LANGUAGE_ACTION"
	};

	const int nButtonX = m_nMenuX + 36;
	const int nButtonW = m_nMenuW - 72;
	const int nFirstButtonY = m_nMenuY + 104;
	const int nGap = 12;
	for ( int i = 0; i < ACTION_COUNT; ++i )
	{
		const int nButtonY = nFirstButtonY + i * ( m_nButtonH + nGap );
		const bool bHover = ( i == m_nHoverAction );

		vgui::surface()->DrawSetColor( bHover ? 62 : 35, bHover ? 86 : 46, bHover ? 92 : 50, 230 );
		vgui::surface()->DrawFilledRect( nButtonX, nButtonY, nButtonX + nButtonW, nButtonY + m_nButtonH );
		vgui::surface()->DrawSetColor( bHover ? 130 : 76, bHover ? 220 : 112, bHover ? 240 : 120, 180 );
		vgui::surface()->DrawOutlinedRect( nButtonX, nButtonY, nButtonX + nButtonW, nButtonY + m_nButtonH );

		if ( i == ACTION_LANGUAGE && g_pVGuiLocalize )
		{
			char token[64];
			char language[32];
			Q_strncpy( language, Portal2CurrentMenuLanguage(), sizeof( language ) );
			V_strupr( language );
			Q_snprintf( token, sizeof( token ), "#P2ARM64_LANGUAGE_%s", language );
			wchar_t line[256];
			g_pVGuiLocalize->ConstructString( line, sizeof( line ), g_pVGuiLocalize->Find( s_ppszActions[i] ), 1,
				g_pVGuiLocalize->Find( token ) );
			DrawText( line, nButtonX + 18, nButtonY + 10, m_hTextFont, Color( 235, 242, 245, 255 ) );
		}
		else
		{
			DrawToken( s_ppszActions[i], nButtonX + 18, nButtonY + 10, m_hTextFont, Color( 235, 242, 245, 255 ) );
		}
	}
}

static CPortal2FallbackPausePanel *Portal2PauseMenu_GetPanel()
{
	if ( g_pPortal2PausePanel )
		return g_pPortal2PausePanel;

	vgui::VPANEL parent = VGui_GetClientDLLRootPanel();
	if ( !parent )
	{
		return NULL;
	}

	g_pPortal2PausePanel = new CPortal2FallbackPausePanel( NULL );
	g_pPortal2PausePanel->SetParent( parent );
	return g_pPortal2PausePanel;
}

CON_COMMAND_F( portal2_pausemenu, "Shows the Portal 1-compatible game menu in the Portal 2 ARM64 target.", FCVAR_CLIENTDLL )
{
	engine->ClientCmd_Unrestricted( "gameui_activate\n" );
}

bool Portal2PauseMenu_HandleKeyInput( int down, ButtonCode_t keynum )
{
	if ( !down )
		return false;

	// Let the engine process Escape/F10 through the standard GameUI route.
	// The launchers bind both keys to gameui_activate.
	return false;
}

void Portal2PauseMenu_LevelInit()
{
	if ( CommandLine()->FindParm( "-portal2_pausemenu" ) )
	{
		g_bOpenPauseMenuWhenReady = true;
	}

	if ( !g_bOpenPauseMenuWhenReady )
		return;

	// CPortal2PauseMenuSystem::Update activates GameUI once level loading ends.
}

class CPortal2PauseMenuSystem : public CAutoGameSystemPerFrame
{
public:
	CPortal2PauseMenuSystem() : CAutoGameSystemPerFrame( "Portal2PauseMenuSystem" )
	{
	}

	virtual void LevelInitPostEntity()
	{
		if ( CommandLine()->FindParm( "-portal2_pausemenu" ) )
		{
			g_bOpenPauseMenuWhenReady = true;
		}

		if ( !g_bOpenPauseMenuWhenReady )
			return;

		// CPortal2PauseMenuSystem::Update activates GameUI once level loading ends.
	}

	virtual void LevelShutdownPreEntity()
	{
		if ( g_pPortal2PausePanel )
		{
			g_pPortal2PausePanel->ShowMenu( false );
		}
		g_bOpenPauseMenuWhenReady = false;
	}

	virtual void Update( float /*frametime*/ )
	{
		// A command-line request arrives during level load; defer the standard
		// GameUI activation until the map is actually running, otherwise the
		// loading transition immediately hides it again.
		if ( g_bOpenPauseMenuWhenReady && engine->IsInGame() )
		{
			engine->ClientCmd_Unrestricted( "gameui_activate\n" );
			g_bOpenPauseMenuWhenReady = false;
		}
	}
};

static CPortal2PauseMenuSystem g_Portal2PauseMenuSystem;

CON_COMMAND_F( portal2_language, "Set Portal 2 ARM64 compatibility-menu language.", FCVAR_CLIENTDLL )
{
	if ( args.ArgC() != 2 )
	{
		Msg( "portal2_language is %s (auto, english, spanish, brazilian, french, german, italian, russian, polish)\n", portal2_ui_language.GetString() );
		return;
	}
	Portal2SetMenuLanguage( args[1] );
}
