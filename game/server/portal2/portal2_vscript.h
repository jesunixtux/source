#ifndef PORTAL2_VSCRIPT_H
#define PORTAL2_VSCRIPT_H

class CBaseEntity;

// Portal 2-only Squirrel bridge.  The public entry points deliberately avoid
// exposing Squirrel types to the rest of the game DLL.
bool Portal2VScriptRunCode( CBaseEntity *pEntity, const char *pCode, const char *pDebugName = NULL );
bool Portal2VScriptRunFile( CBaseEntity *pEntity, const char *pScriptName );
bool Portal2VScriptCallFunction( CBaseEntity *pEntity, const char *pFunctionName );
bool Portal2VScriptHandleInput( CBaseEntity *pEntity, const char *pInputName, const char *pValue );

#endif // PORTAL2_VSCRIPT_H
