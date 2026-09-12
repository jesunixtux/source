//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SHARED_CLASSNAMES_H
#define SHARED_CLASSNAMES_H
#ifdef _WIN32
#pragma once
#endif

// Hacky macros to allow shared code to work without even worse macro-izing
#if defined( CLIENT_DLL )

#define CBaseEntity				C_BaseEntity
#define CBaseCombatCharacter	C_BaseCombatCharacter
#define CBaseAnimating			C_BaseAnimating
#define CBasePlayer				C_BasePlayer

// Portal 2 aliased entity factory macro. The P2 source names classes without
// the client prefix and relies on the factory macro to prepend it.
#define LINK_ENTITY_TO_CLASS_ALIASED( localName, className ) LINK_ENTITY_TO_CLASS( localName, C_##className )

#else

#define LINK_ENTITY_TO_CLASS_ALIASED( localName, className ) LINK_ENTITY_TO_CLASS( localName, C##className )

#endif


#endif // SHARED_CLASSNAMES_H
