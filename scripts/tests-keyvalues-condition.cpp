#include "KeyValues.h"
#include "tier0/dbg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <string>

extern bool EvaluateConditional( const char *expression );
static int errors = 0;

static SpewRetval_t CaptureSpew( SpewType_t, const char *message )
{
	if ( strstr( message, "KeyValues Error" ) )
		++errors;
	fputs( message, stderr );
	return SPEW_CONTINUE;
}

static void Check( bool value, const char *description )
{
	if ( !value )
	{
		fprintf( stderr, "FAIL: %s\n", description );
		exit( 1 );
	}
}

int main( int argc, char **argv )
{
	SpewOutputFunc( CaptureSpew );
	Check( EvaluateConditional( "[$WIN32]" ), "legacy WIN32 means desktop PC" );
	Check( EvaluateConditional( "[$POSIX]" ), "legacy POSIX selector" );
	Check( !EvaluateConditional( "[$WINDOWS]" ), "macOS is not Windows" );
	Check( !EvaluateConditional( "[$OSX]" ), "preserve the port's legacy OSX resource selection" );
	Check( !EvaluateConditional( "[$WIN32_UNKNOWN]" ), "symbol names must match exactly" );
	Check( EvaluateConditional( "[!$GAMECONSOLE]" ), "desktop is not GAMECONSOLE" );
	Check( !EvaluateConditional( "[0 && $WIN32]" ), "constant false and desktop" );
	Check( EvaluateConditional( "[0 || $WIN32 && !$GAMECONSOLE]" ), "AND precedence and OR" );
	Check( !EvaluateConditional( "[(0 || $WIN32) && $GAMECONSOLE]" ), "parentheses" );
	Check( EvaluateConditional( "[!($GAMECONSOLEWIDE && $GAMECONSOLEHIDEF)]" ), "console display flags" );
	Check( !EvaluateConditional( "[$WIN32 &&]" ), "incomplete expression rejected" );
	Check( !EvaluateConditional( "[$WIN32] trailing" ), "trailing text rejected" );
	Check( !EvaluateConditional( "[$WIN32 || )]" ), "short circuit must not hide invalid syntax" );
	std::string deep = "[" + std::string( 100, '!' ) + "$WIN32]";
	Check( !EvaluateConditional( deep.c_str() ), "nesting is bounded" );
	errors = 0; // Invalid expressions above intentionally report diagnostics.

	KeyValues *kv = new KeyValues( "test" );
	kv->LoadFromBuffer( "conditional-fixture", R"KV("test" {
		"desktop" [!$GAMECONSOLE] { "value" "yes" }
		"console" [$GAMECONSOLE] { "value" "no" }
		"disabled" [0 && $GAMECONSOLE] { "value" "no" }
		"tail" "yes" [ $WIN32 && !$GAMECONSOLE ]
		"quoted" "[0 && $GAMECONSOLE]"
		"after" "intact"
	})KV" );
	Check( errors == 0, "spaced conditional tokens parse without errors" );
	Check( kv->FindKey( "desktop" ) && !kv->FindKey( "console" ) && !kv->FindKey( "disabled" ), "conditional blocks selected" );
	Check( !strcmp( kv->GetString( "tail" ), "yes" ), "value conditional selected" );
	Check( !strcmp( kv->GetString( "quoted" ), "[0 && $GAMECONSOLE]" ), "quoted brackets unchanged" );
	Check( !strcmp( kv->GetString( "after" ), "intact" ), "token after conditional preserved" );
	kv->deleteThis();
	kv = new KeyValues( "test" );
	kv->LoadFromBuffer( "unterminated-fixture", "\"test\" { \"value\" \"yes\" [$WIN32" );
	Check( errors > 0, "unterminated bracket reports a parse error" );
	kv->deleteThis();

	for ( int i = 1; i < argc; ++i )
	{
		std::ifstream stream( argv[i], std::ios::binary );
		Check( stream.good(), argv[i] );
		std::ostringstream buffer;
		buffer << stream.rdbuf();
		std::string text = buffer.str();
		kv = new KeyValues( "Scheme" );
		errors = 0;
		kv->LoadFromBuffer( argv[i], text.c_str() );
		Check( errors == 0 && kv->GetFirstSubKey(), argv[i] );
		printf( "RESOURCE PASS: %s\n", argv[i] );
		kv->deleteThis();
	}
	puts( "PASS: real KeyValues parser, boolean conditions and resource loading" );
}
