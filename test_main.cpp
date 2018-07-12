#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "logging.hpp"

int main( int argc, char* argv[] )
{
    Catch::Session session;
    bool debug_log = false;
    bool trace_log = false;
    using namespace Catch::clara;
    auto cli = session.cli()
    | Opt(debug_log)
          ["--show-debug"]
          ("Shoiw DEBUG level output on the console")
    | Opt(trace_log)
         ["--show-trace"]
         ("Show TRACE level output on the console");
    session.cli(cli);
    int returnCode = session.applyCommandLine( argc, argv );
    if( returnCode != 0 ) // Indicates a command line error
        return returnCode;

    if (debug_log)
    {
        fnc::enable_debug_logging();
    }
    if (trace_log)
    {
        fnc::enable_trace_logging();
    }
    return session.run();
}
