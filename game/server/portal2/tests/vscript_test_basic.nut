IncludeScript( "vscript_test_include" );

scopeValue <- 41;

function VScriptBasicCheck()
{
	if ( scopeValue + 1 != 42 )
		throw "variables/functions failed";
	if ( IncludedValue() != 7 )
		throw "IncludeScript failed";

	local vector = Vector( 1, 2, 3 );
	if ( vector.x != 1 || vector.y != 2 || vector.z != 3 )
		throw "Vector failed";
	if ( Time() < 0 || FrameTime() < 0 )
		throw "time bindings failed";
	if ( thisEntity == null )
		throw "thisEntity scope failed";

	local target = Entities.FindByName( null, "@vscript_basic_target" );
	if ( target == null )
		throw "Entities.FindByName failed";
	if ( Entities.FindByClassname( null, "portal2_vscript_test_target" ) == null )
		throw "Entities.FindByClassname failed";
	local first = Entities.First();
	if ( first == null )
		throw "Entities.First failed";
	Entities.Next( first );

	EntFireByHandle( target, "Mark", "", 0.0, null, thisEntity );
	EntFire( "@vscript_basic_target", "Mark", "", 0.0, thisEntity );
}
