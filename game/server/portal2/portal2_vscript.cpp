// Minimal, real Squirrel-backed VScript runtime for the Portal 2 target.
// Squirrel is the public MIT-licensed implementation pinned as a submodule at
// external/squirrel.  This layer intentionally exposes only bindings whose
// behavior is understood and tested; unsupported calls remain visible.
#include "cbase.h"
#include "portal2_vscript.h"
#include "eventqueue.h"
#include "filesystem.h"
#include "igamesystem.h"
#include "tier0/icommandline.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlvector.h"

#include <squirrel.h>
#include <sqstdaux.h>
#include <ctype.h>
#include <stdarg.h>

#include "tier0/memdbgon.h"

ConVar vscript_debug( "vscript_debug", "0", FCVAR_CHEAT,
	"Log Portal 2 VScript VM, scope, file, call and entity I/O activity." );

namespace
{
struct EntityScope_t
{
	CBaseHandle m_Entity;
	HSQOBJECT m_Table;
};

static void VScriptPrint( HSQUIRRELVM, const SQChar *pFormat, ... )
{
	char buffer[2048];
	va_list args;
	va_start( args, pFormat );
	V_vsnprintf( buffer, sizeof( buffer ), pFormat, args );
	va_end( args );
	Msg( "[VSCRIPT] %s", buffer );
}

static void VScriptError( HSQUIRRELVM, const SQChar *pFormat, ... )
{
	char buffer[2048];
	va_list args;
	va_start( args, pFormat );
	V_vsnprintf( buffer, sizeof( buffer ), pFormat, args );
	va_end( args );
	Warning( "[VSCRIPT] %s", buffer );
}

class CPortal2VScriptVM : public CAutoGameSystemPerFrame
{
public:
	CPortal2VScriptVM() : CAutoGameSystemPerFrame( "Portal2VScriptVM" ), m_pVM( NULL ) {}

	bool Init()
	{
		m_pVM = sq_open( 1024 );
		if ( !m_pVM )
		{
			Warning( "[VSCRIPT] Failed to initialize Squirrel VM\n" );
			return false;
		}

		sq_setforeignptr( m_pVM, this );
		sq_setprintfunc( m_pVM, VScriptPrint, VScriptError );
		sqstd_seterrorhandlers( m_pVM );
		RegisterBindings();
		if ( vscript_debug.GetBool() )
			Msg( "[VSCRIPT] VM initialized (%s)\n", SQUIRREL_VERSION );
		return true;
	}

	void Shutdown()
	{
		ReleaseScopes();
		if ( m_pVM )
		{
			sq_close( m_pVM );
			m_pVM = NULL;
		}
	}

	void LevelShutdownPostEntity()
	{
		ReleaseScopes();
	}

	bool RunCode( CBaseEntity *pEntity, const char *pCode, const char *pDebugName )
	{
		if ( !m_pVM || !pCode || !pCode[0] )
			return false;

		HSQOBJECT scope = GetScope( pEntity );
		const SQInteger oldTop = sq_gettop( m_pVM );
		const char *pName = ( pDebugName && pDebugName[0] ) ? pDebugName : "RunScriptCode";
		if ( SQ_FAILED( sq_compilebuffer( m_pVM, pCode, V_strlen( pCode ), pName, SQTrue ) ) )
		{
			sq_settop( m_pVM, oldTop );
			return false;
		}

		sq_pushobject( m_pVM, scope );
		if ( SQ_FAILED( sq_setclosureroot( m_pVM, -2 ) ) )
		{
			sq_settop( m_pVM, oldTop );
			return false;
		}

		const EHANDLE previousExecutingEntity = m_hExecutingEntity;
		m_hExecutingEntity = pEntity;
		sq_pushobject( m_pVM, scope );
		const bool ok = SQ_SUCCEEDED( sq_call( m_pVM, 1, SQFalse, SQTrue ) );
		m_hExecutingEntity = previousExecutingEntity;
		if ( !ok )
			LogMissingNativeFunction( scope, pCode );
		sq_settop( m_pVM, oldTop );
		return ok;
	}

	bool RunFile( CBaseEntity *pEntity, const char *pScriptName )
	{
		char path[MAX_PATH];
		if ( !BuildScriptPath( pScriptName, path, sizeof( path ) ) )
			return false;

		CUtlBuffer source( 0, 0, CUtlBuffer::TEXT_BUFFER );
		if ( !filesystem->ReadFile( path, "GAME", source ) )
		{
			Warning( "[VSCRIPT] Script not found: %s\n", path );
			return false;
		}
		source.PutChar( 0 );
		if ( vscript_debug.GetBool() )
			Msg( "[VSCRIPT] Loading script: %s\n", path );
		return RunCode( pEntity, static_cast<const char *>( source.Base() ), path );
	}

	bool CallFunction( CBaseEntity *pEntity, const char *pFunctionName )
	{
		if ( !m_pVM || !pFunctionName || !pFunctionName[0] )
			return false;

		HSQOBJECT scope = GetScope( pEntity );
		const SQInteger oldTop = sq_gettop( m_pVM );
		sq_pushobject( m_pVM, scope );
		sq_pushstring( m_pVM, pFunctionName, -1 );
		if ( SQ_FAILED( sq_get( m_pVM, -2 ) ) )
		{
			sq_settop( m_pVM, oldTop );
			Warning( "[VSCRIPT] Missing script function: %s\n", pFunctionName );
			return false;
		}

		if ( vscript_debug.GetBool() )
			Msg( "[VSCRIPT] Calling %s()\n", pFunctionName );
		const EHANDLE previousExecutingEntity = m_hExecutingEntity;
		m_hExecutingEntity = pEntity;
		sq_pushobject( m_pVM, scope );
		const bool ok = SQ_SUCCEEDED( sq_call( m_pVM, 1, SQFalse, SQTrue ) );
		m_hExecutingEntity = previousExecutingEntity;
		sq_settop( m_pVM, oldTop );
		return ok;
	}

	CBaseEntity *EntityFromArg( SQInteger index ) const
	{
		if ( sq_gettype( m_pVM, index ) == OT_NULL )
			return NULL;
		SQInteger packed = 0;
		if ( SQ_FAILED( sq_getinteger( m_pVM, index, &packed ) ) )
			return NULL;
		return CBaseEntity::Instance( CBaseHandle( static_cast<uintp>( packed ) ) );
	}

	void PushEntity( CBaseEntity *pEntity )
	{
		if ( pEntity )
			sq_pushinteger( m_pVM, static_cast<SQInteger>( pEntity->GetRefEHandle().ToInt() ) );
		else
			sq_pushnull( m_pVM );
	}

	CBaseEntity *ExecutingEntity() const { return m_hExecutingEntity.Get(); }

private:
	void LogMissingNativeFunction( const HSQOBJECT &scope, const char *pCode )
	{
		// Portal 2 map outputs commonly contain a single global function call.
		// Diagnose that safe subset without guessing the behavior of the missing API.
		while ( *pCode == ' ' || *pCode == '\t' || *pCode == '\r' || *pCode == '\n' )
			++pCode;
		if ( !( isalpha( static_cast<unsigned char>( *pCode ) ) || *pCode == '_' ) )
			return;

		char functionName[128];
		int length = 0;
		while ( ( isalnum( static_cast<unsigned char>( *pCode ) ) || *pCode == '_' ) &&
				length < static_cast<int>( sizeof( functionName ) ) - 1 )
			functionName[length++] = *pCode++;
		functionName[length] = '\0';
		while ( *pCode == ' ' || *pCode == '\t' )
			++pCode;
		if ( *pCode != '(' )
			return;

		const SQInteger oldTop = sq_gettop( m_pVM );
		sq_pushobject( m_pVM, scope );
		sq_pushstring( m_pVM, functionName, -1 );
		const bool missing = SQ_FAILED( sq_get( m_pVM, -2 ) );
		sq_settop( m_pVM, oldTop );
		if ( missing )
			Warning( "[VSCRIPT] Missing native function: %s\n", functionName );
	}

	static CPortal2VScriptVM *FromVM( HSQUIRRELVM vm )
	{
		return static_cast<CPortal2VScriptVM *>( sq_getforeignptr( vm ) );
	}

	static const char *StringArg( HSQUIRRELVM vm, SQInteger index, const char *pFallback = "" )
	{
		const SQChar *value = NULL;
		return SQ_SUCCEEDED( sq_getstring( vm, index, &value ) ) ? value : pFallback;
	}

	static float FloatArg( HSQUIRRELVM vm, SQInteger index, float fallback = 0.0f )
	{
		SQFloat value = fallback;
		if ( SQ_SUCCEEDED( sq_getfloat( vm, index, &value ) ) )
			return value;
		SQInteger integer = 0;
		return SQ_SUCCEEDED( sq_getinteger( vm, index, &integer ) ) ? static_cast<float>( integer ) : fallback;
	}

	static SQInteger NativeTime( HSQUIRRELVM vm )
	{
		sq_pushfloat( vm, gpGlobals ? gpGlobals->curtime : 0.0f );
		return 1;
	}

	static SQInteger NativeFrameTime( HSQUIRRELVM vm )
	{
		sq_pushfloat( vm, gpGlobals ? gpGlobals->frametime : 0.0f );
		return 1;
	}

	static SQInteger NativeVector( HSQUIRRELVM vm )
	{
		const float x = FloatArg( vm, 2 );
		const float y = FloatArg( vm, 3 );
		const float z = FloatArg( vm, 4 );
		sq_newtable( vm );
		const char *keys[] = { "x", "y", "z" };
		const float values[] = { x, y, z };
		for ( int i = 0; i < 3; ++i )
		{
			sq_pushstring( vm, keys[i], -1 );
			sq_pushfloat( vm, values[i] );
			sq_newslot( vm, -3, SQFalse );
		}
		return 1;
	}

	static SQInteger NativeEntFire( HSQUIRRELVM vm )
	{
		CPortal2VScriptVM *self = FromVM( vm );
		const char *target = StringArg( vm, 2 );
		const char *input = StringArg( vm, 3 );
		const char *value = StringArg( vm, 4 );
		const float delay = FloatArg( vm, 5 );
		CBaseEntity *activator = sq_gettop( vm ) >= 6 ? self->EntityFromArg( 6 ) : NULL;
		variant_t variant;
		variant.SetString( AllocPooledString( value ) );
		g_EventQueue.AddEvent( target, input, variant, delay, activator, self->ExecutingEntity() );
		if ( vscript_debug.GetBool() )
			Msg( "[VSCRIPT] EntFire target=%s input=%s\n", target, input );
		return 0;
	}

	static SQInteger NativeEntFireByHandle( HSQUIRRELVM vm )
	{
		CPortal2VScriptVM *self = FromVM( vm );
		CBaseEntity *target = self->EntityFromArg( 2 );
		if ( !target )
			return sq_throwerror( vm, "EntFireByHandle received an invalid entity handle" );
		const char *input = StringArg( vm, 3 );
		const char *value = StringArg( vm, 4 );
		const float delay = FloatArg( vm, 5 );
		CBaseEntity *activator = sq_gettop( vm ) >= 6 ? self->EntityFromArg( 6 ) : NULL;
		CBaseEntity *caller = sq_gettop( vm ) >= 7 ? self->EntityFromArg( 7 ) : self->ExecutingEntity();
		variant_t variant;
		variant.SetString( AllocPooledString( value ) );
		g_EventQueue.AddEvent( target, input, variant, delay, activator, caller );
		if ( vscript_debug.GetBool() )
			Msg( "[VSCRIPT] EntFireByHandle target=%s input=%s\n", target->GetDebugName(), input );
		return 0;
	}

	static SQInteger NativeIncludeScript( HSQUIRRELVM vm )
	{
		CPortal2VScriptVM *self = FromVM( vm );
		const char *script = StringArg( vm, 2 );
		if ( !self->RunFile( self->ExecutingEntity(), script ) )
			return sq_throwerror( vm, "IncludeScript failed" );
		sq_pushbool( vm, SQTrue );
		return 1;
	}

	static SQInteger NativeFindByName( HSQUIRRELVM vm )
	{
		CPortal2VScriptVM *self = FromVM( vm );
		CBaseEntity *start = self->EntityFromArg( 2 );
		self->PushEntity( gEntList.FindEntityByName( start, StringArg( vm, 3 ), self->ExecutingEntity() ) );
		return 1;
	}

	static SQInteger NativeFindByClassname( HSQUIRRELVM vm )
	{
		CPortal2VScriptVM *self = FromVM( vm );
		self->PushEntity( gEntList.FindEntityByClassname( self->EntityFromArg( 2 ), StringArg( vm, 3 ) ) );
		return 1;
	}

	static SQInteger NativeFirst( HSQUIRRELVM vm )
	{
		CPortal2VScriptVM *self = FromVM( vm );
		self->PushEntity( gEntList.FirstEnt() );
		return 1;
	}

	static SQInteger NativeNext( HSQUIRRELVM vm )
	{
		CPortal2VScriptVM *self = FromVM( vm );
		CBaseEntity *entity = self->EntityFromArg( 2 );
		self->PushEntity( entity ? gEntList.NextEnt( entity ) : NULL );
		return 1;
	}

	void RegisterGlobal( const char *pName, SQFUNCTION function )
	{
		sq_pushroottable( m_pVM );
		sq_pushstring( m_pVM, pName, -1 );
		sq_newclosure( m_pVM, function, 0 );
		sq_setnativeclosurename( m_pVM, -1, pName );
		sq_newslot( m_pVM, -3, SQFalse );
		sq_pop( m_pVM, 1 );
	}

	void RegisterTableFunction( const char *pName, SQFUNCTION function )
	{
		sq_pushstring( m_pVM, pName, -1 );
		sq_newclosure( m_pVM, function, 0 );
		sq_setnativeclosurename( m_pVM, -1, pName );
		sq_newslot( m_pVM, -3, SQFalse );
	}

	void RegisterBindings()
	{
		RegisterGlobal( "Time", NativeTime );
		RegisterGlobal( "FrameTime", NativeFrameTime );
		RegisterGlobal( "Vector", NativeVector );
		RegisterGlobal( "EntFire", NativeEntFire );
		RegisterGlobal( "EntFireByHandle", NativeEntFireByHandle );
		RegisterGlobal( "IncludeScript", NativeIncludeScript );

		sq_pushroottable( m_pVM );
		sq_pushstring( m_pVM, "Entities", -1 );
		sq_newtable( m_pVM );
		RegisterTableFunction( "FindByName", NativeFindByName );
		RegisterTableFunction( "FindByClassname", NativeFindByClassname );
		RegisterTableFunction( "First", NativeFirst );
		RegisterTableFunction( "Next", NativeNext );
		sq_newslot( m_pVM, -3, SQFalse );
		sq_pop( m_pVM, 1 );
	}

	bool BuildScriptPath( const char *pScriptName, char *pPath, size_t pathSize ) const
	{
		if ( !pScriptName || !pScriptName[0] || Q_strstr( pScriptName, ".." ) ||
			Q_strstr( pScriptName, ":" ) || pScriptName[0] == '/' || pScriptName[0] == '\\' )
		{
			Warning( "[VSCRIPT] Rejected unsafe script path: %s\n", pScriptName ? pScriptName : "<null>" );
			return false;
		}
		const bool hasExtension = V_strlen( pScriptName ) >= 4 &&
			!Q_stricmp( pScriptName + V_strlen( pScriptName ) - 4, ".nut" );
		Q_snprintf( pPath, pathSize, "scripts/vscripts/%s%s", pScriptName, hasExtension ? "" : ".nut" );
		V_FixSlashes( pPath, '/' );
		return true;
	}

	HSQOBJECT GetScope( CBaseEntity *pEntity )
	{
		if ( !pEntity )
		{
			HSQOBJECT root;
			sq_pushroottable( m_pVM );
			sq_getstackobj( m_pVM, -1, &root );
			sq_pop( m_pVM, 1 );
			return root;
		}

		const CBaseHandle handle = pEntity->GetRefEHandle();
		FOR_EACH_VEC_BACK( m_Scopes, i )
		{
			if ( !CBaseEntity::Instance( m_Scopes[i].m_Entity ) )
			{
				sq_release( m_pVM, &m_Scopes[i].m_Table );
				m_Scopes.FastRemove( i );
				continue;
			}
			if ( m_Scopes[i].m_Entity == handle )
				return m_Scopes[i].m_Table;
		}

		sq_newtable( m_pVM );
		sq_pushroottable( m_pVM );
		sq_setdelegate( m_pVM, -2 );
		sq_pushstring( m_pVM, "thisEntity", -1 );
		PushEntity( pEntity );
		sq_newslot( m_pVM, -3, SQFalse );

		EntityScope_t scope;
		scope.m_Entity = handle;
		sq_getstackobj( m_pVM, -1, &scope.m_Table );
		sq_addref( m_pVM, &scope.m_Table );
		sq_pop( m_pVM, 1 );
		m_Scopes.AddToTail( scope );
		if ( vscript_debug.GetBool() )
			Msg( "[VSCRIPT] Created scope for %s\n", pEntity->GetDebugName() );
		return scope.m_Table;
	}

	void ReleaseScopes()
	{
		if ( m_pVM )
		{
			FOR_EACH_VEC( m_Scopes, i )
				sq_release( m_pVM, &m_Scopes[i].m_Table );
		}
		m_Scopes.Purge();
		m_hExecutingEntity = NULL;
	}

	HSQUIRRELVM m_pVM;
	CUtlVector<EntityScope_t> m_Scopes;
	EHANDLE m_hExecutingEntity;
};

CPortal2VScriptVM g_Portal2VScriptVM;
}

class CPortal2LogicScript : public CPointEntity
{
public:
	DECLARE_CLASS( CPortal2LogicScript, CPointEntity );
	DECLARE_DATADESC();

	CPortal2LogicScript() : m_iszScripts( NULL_STRING ) {}

	void Spawn()
	{
		BaseClass::Spawn();
		SetContextThink( &CPortal2LogicScript::RunInitialScripts, gpGlobals->curtime,
			"Portal2VScriptInitialScripts" );
	}

	void RunInitialScripts()
	{
		if ( m_iszScripts == NULL_STRING )
			return;
		char scripts[1024];
		V_strncpy( scripts, STRING( m_iszScripts ), sizeof( scripts ) );
		for ( char *script = strtok( scripts, " ;\t\r\n" ); script; script = strtok( NULL, " ;\t\r\n" ) )
			Portal2VScriptRunFile( this, script );
	}

	void InputRunScriptCode( inputdata_t &data ) { Portal2VScriptRunCode( this, data.value.String() ); }
	void InputRunScriptFile( inputdata_t &data ) { Portal2VScriptRunFile( this, data.value.String() ); }
	void InputCallScriptFunction( inputdata_t &data ) { Portal2VScriptCallFunction( this, data.value.String() ); }

private:
	string_t m_iszScripts;
};

LINK_ENTITY_TO_CLASS( logic_script, CPortal2LogicScript );

BEGIN_DATADESC( CPortal2LogicScript )
	DEFINE_KEYFIELD( m_iszScripts, FIELD_STRING, "vscripts" ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RunScriptCode", InputRunScriptCode ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RunScriptFile", InputRunScriptFile ),
	DEFINE_INPUTFUNC( FIELD_STRING, "CallScriptFunction", InputCallScriptFunction ),
END_DATADESC()

class CPortal2VScriptTestTarget : public CPointEntity
{
public:
	DECLARE_CLASS( CPortal2VScriptTestTarget, CPointEntity );
	DECLARE_DATADESC();
	CPortal2VScriptTestTarget() : m_nMarks( 0 ) {}
	void InputMark( inputdata_t & )
	{
		if ( ++m_nMarks == 2 )
			Msg( "VSCRIPT_BASIC PASS\n" );
	}
private:
	int m_nMarks;
};

LINK_ENTITY_TO_CLASS( portal2_vscript_test_target, CPortal2VScriptTestTarget );
BEGIN_DATADESC( CPortal2VScriptTestTarget )
	DEFINE_FIELD( m_nMarks, FIELD_INTEGER ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Mark", InputMark ),
END_DATADESC()

bool Portal2VScriptRunCode( CBaseEntity *pEntity, const char *pCode, const char *pDebugName )
{
	return g_Portal2VScriptVM.RunCode( pEntity, pCode, pDebugName );
}

bool Portal2VScriptRunFile( CBaseEntity *pEntity, const char *pScriptName )
{
	return g_Portal2VScriptVM.RunFile( pEntity, pScriptName );
}

bool Portal2VScriptCallFunction( CBaseEntity *pEntity, const char *pFunctionName )
{
	return g_Portal2VScriptVM.CallFunction( pEntity, pFunctionName );
}

bool Portal2VScriptHandleInput( CBaseEntity *pEntity, const char *pInputName, const char *pValue )
{
	if ( !Q_stricmp( pInputName, "RunScriptCode" ) )
		return Portal2VScriptRunCode( pEntity, pValue );
	if ( !Q_stricmp( pInputName, "RunScriptFile" ) )
		return Portal2VScriptRunFile( pEntity, pValue );
	if ( !Q_stricmp( pInputName, "CallScriptFunction" ) )
		return Portal2VScriptCallFunction( pEntity, pValue );
	return false;
}

static void RunPortal2VScriptBasicTest()
{
	CBaseEntity *target = gEntList.FindEntityByName( NULL, "@vscript_basic_target" );
	if ( !target )
	{
		target = CBaseEntity::Create( "portal2_vscript_test_target", vec3_origin, vec3_angle );
		if ( target )
			target->SetName( AllocPooledString( "@vscript_basic_target" ) );
	}
	CBaseEntity *host = CBaseEntity::Create( "logic_script", vec3_origin, vec3_angle );
	if ( !target || !host )
	{
		Warning( "VSCRIPT_BASIC FAIL: could not create test entities\n" );
		return;
	}
	host->SetName( AllocPooledString( "@vscript_basic_script" ) );
	if ( !Portal2VScriptRunFile( host, "vscript_test_basic" ) ||
		 !Portal2VScriptCallFunction( host, "VScriptBasicCheck" ) )
	{
		Warning( "VSCRIPT_BASIC FAIL: script execution\n" );
	}
}

class CPortal2VScriptBasicTestSystem : public CAutoGameSystemPerFrame
{
public:
	CPortal2VScriptBasicTestSystem()
		: CAutoGameSystemPerFrame( "Portal2VScriptBasicTestSystem" ), m_bRan( false ) {}

	void LevelInitPreEntity() { m_bRan = false; }

	void FrameUpdatePostEntityThink()
	{
		if ( m_bRan || !CommandLine()->FindParm( "-portal2_vscript_test" ) ||
			 !gpGlobals || gpGlobals->curtime < 1.0f )
			return;
		m_bRan = true;
		RunPortal2VScriptBasicTest();
	}

private:
	bool m_bRan;
};

CPortal2VScriptBasicTestSystem g_Portal2VScriptBasicTestSystem;

CON_COMMAND_F( portal2_vscript_test_basic, "Run the minimal Portal 2 Squirrel/VScript regression.", FCVAR_CHEAT )
{
	RunPortal2VScriptBasicTest();
}
