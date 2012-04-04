#include <sim/system.hh>
#include <sim/process.hh>
#include <sst/core/params.h>
#include <sst/core/component.h>
#include <params/LiveProcess.hh>
#include <m5.h>
#include <util.h>

#include <debug.h>
#include <paramHelp.h>

static inline Process* newProcess( const std::string name, 
     const SST::Params& params, System* system, SST::Component* comp = NULL )
{
    LiveProcessParams& process  = * new LiveProcessParams;

    process.name = name;

    //params.print_all_params(std::cout);

    // ProcessParams
    process.system = system;
    INIT_STR( process, params, errout );
    if ( process.errout.empty() ) {
        process.errout = "cerr";
    }
    INIT_STR( process, params, input );
    if ( process.input.empty() ) {
        process.input = "cin";
    }
    INIT_STR( process, params, output );
    if ( process.output.empty() ) {
        process.output = "cout";
    }

    INIT_HEX( process, params, max_stack_size );

    // LiveProcessParams
    INIT_INT( process, params, egid );
    INIT_INT( process, params, euid );
    INIT_INT( process, params, gid );
    INIT_INT( process, params, pid );
    INIT_INT( process, params, ppid );
    INIT_INT( process, params, uid );
    INIT_STR( process, params, cwd );
    INIT_STR( process, params, executable );

    std::string tmpName = resolveString( process.executable );
    if ( tmpName.empty() ) {
        fprintf(stderr,"fatal: bad executable name `%s`\n", 
                                            process.executable.c_str() );
        exit(-1);
    }
    process.executable = tmpName;
	
    std::string str = params.find_string( "registerExit" );
    
    if( comp && ! str.compare("yes") ) {
        INFO("registering exit `%s`\n",name.c_str());
        static_cast< M5* >( static_cast< void* >( comp ) )->registerExit();
    }

    process.cmd.clear();
    SST::Params tmp = params.find_prefix_params( "cmd." ); 
    if ( tmp.size() ) {
        SST::Params::iterator iter = tmp.begin();
        for ( iter = tmp.begin(); iter!= tmp.end(); iter++ ) { 
            process.cmd.push_back(resolveString((*iter).second));
        }
    }

    assert(process.cmd.size() != 0 &&
           " fatal: bad executable name ");

    tmp = params.find_prefix_params( "env." ); 
    if ( tmp.size() ) {
        process.env.resize( tmp.size() );
        SST::Params::iterator iter = tmp.begin();
        for ( int i = 0; i < process.env.size(); i++ ) { 
            process.env[i] = (*iter).second;
            ++iter;
            F_STR( process, env[i] );
        }
    }

    INIT_INT( process, params, simpoint );

    return process.create();
}

static inline SST::Params mergeParams( std::string name, SST::Params p1, SST::Params p2 )
{
    SST::Params tmp = p2.find_prefix_params(name);
    int num = p1.find_prefix_params(name).size();

    SST::Params::iterator iter;
    iter = tmp.begin(); 
    while ( iter != tmp.end() ) {
        std::stringstream numStr;
        numStr << num; 
        p1[ name + numStr.str() ] = (*iter).second;
        ++iter;
        ++num;
    }
    return p1;
}

static inline SST::Params mergeParams( SST::Params p1, SST::Params p2 )
{
    SST:: Params params = mergeParams( "cmd.", p1, p2 ); 
    return mergeParams( "env.", params, p2 ); 
}

