#ifndef SPLITSCREEN_H
#define SPLITSCREEN_H

//===========================================================================
//
// Purpose: Splitscreen helpers.
//
// This port is single player only, so all splitscreen machinery collapses to
// the lone local player (slot 0). These helpers exist so Portal 2 code that
// references splitscreen concepts compiles and behaves as single player.
//
//===========================================================================

#define MAX_SPLITSCREEN_PLAYERS 1

// Returns the factory function that returns the index of the current
// splitscreen player.  On the engine side this is the parameter to
// CBasePlayer::GetLocalPlayer().  Single player: always slot 0.
#define GET_ACTIVE_SPLITSCREEN_SLOT() 0

// Entries to this macro assert that the given entity is the active
// splitscreen player.  Single player: no assertion necessary.
#define ACTIVE_SPLITSCREEN_PLAYER_GUARD_ENT( ent )

// The splitscreen slot of the local player.
inline int GetSplitScreenPlayerSlot()
{
	return 0;
}

// VGUI has no splitscreen support on this port.
inline bool VGui_IsSplitScreen()
{
	return false;
}

// Portal 2 queries whether the current view is a splitscreen picture-in-picture
// pass. This macOS ARM64 port is single-player only, so that can never happen.
inline bool VGui_IsSplitScreenPIP()
{
	return false;
}

#ifdef CLIENT_DLL
// The player whose view is currently being rendered (single player: the local
// player). A macro so it expands at the call site where C_BasePlayer is complete.
#define GetSplitScreenViewPlayer(...) (C_BasePlayer::GetLocalPlayer( __VA_ARGS__ ))
#endif // CLIENT_DLL

// Single player ports never have remote splitscreen view players.
#define AddRemoteSplitScreenViewPlayer( entity )
#define RemoveRemoteSplitScreenViewPlayer( entity )

// This port supports only one local player.
#define FOR_EACH_VALID_SPLITSCREEN_PLAYER( slot ) for ( int slot = 0; slot < 1; ++slot )

#endif // SPLITSCREEN_H
