//========= Copyright 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: Weighted cube entity. Reconstructed from private Portal 2 source;
//          the cube exposes a subset of the private gameplay API used by this
//          port's code.
//
//=============================================================================//

#ifndef PROP_WEIGHTEDCUBE_H
#define PROP_WEIGHTEDCUBE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "props.h"

enum CubeType
{
	CUBE_STANDARD = 0,
	CUBE_COMPANION = 1,
	CUBE_REFLECTIVE,
	CUBE_SPHERE,
	CUBE_ANTIQUE,
	CUBE_SCHRODINGER,
};

class CPropWeightedCube : public CPhysicsProp
{
public:
	DECLARE_CLASS(CPropWeightedCube, CPhysicsProp);
	DECLARE_DATADESC();

	CPropWeightedCube()
	{
	}

	void Spawn(void);
	void Precache(void);

	//Use
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps();

	bool Dissolve(const char* materialName, float flStartTime, bool bNPCOnly, int nDissolveType, Vector vDissolverOrigin, int magnitude);

	void InputDissolve(inputdata_t &data);
	void InputSilentDissolve(inputdata_t &data);
	void InputPreDissolveJoke(inputdata_t &data);
	void InputExitDisabledState(inputdata_t &data);

	//Pickup
	void OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason);
	void OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t reason);

	// Reflective cube behavior
	void ExitDisabledState( void );
	void OnExitedTractorBeam( void );
	int GetCubeType( void ) const { return m_cubeType; }
	bool HasLaser( void ) const { return false; }

private:
	int	m_cubeType;
	int m_skinType;
	int m_paintPower;
	bool m_useNewSkins;
	bool m_allowFunnel;

	CHandle<CBasePlayer> m_hPhysicsAttacker;

	COutputEvent m_OnOrangePickup;
	COutputEvent m_OnBluePickup;
	COutputEvent m_OnPlayerPickup;

	COutputEvent m_OnPainted;

	COutputEvent m_OnPhysGunDrop;

	COutputEvent m_OnFizzled;
};

bool UTIL_IsReflectiveCube( CBaseEntity *pEntity );
bool UTIL_IsSchrodinger( CBaseEntity *pEntity );

#endif // PROP_WEIGHTEDCUBE_H