/*
 * Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
 * Copyright (C) 2006-2010 RobotCub Consortium
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#define CATCH_CONFIG_RUNNER
#if defined(USE_SYSTEM_CATCH)
#include <catch.hpp>
#else
#include "catch.hpp"
#endif

#ifdef YARP2_LINUX
# define CHECK_FOR_LEAKS
#endif

#ifdef CHECK_FOR_LEAKS
# include <mcheck.h>
#endif


#include <yarp/os/Network.h>
#include <yarp/os/Property.h>
#include <yarp/os/NameStore.h>

#include <yarp/serversql/yarpserversql.h>

using namespace yarp::os;

int main(int argc, char *argv[])
{
#ifdef CHECK_FOR_LEAKS
    mtrace();
#endif

    Catch::Session session;

    bool verbose = 0;

#if (CATCH_VERSION_MAJOR>=2)
    using namespace Catch::clara;
    auto cli = session.cli() | Opt(verbose )["-y"]["--yarp-verbose"]("Enable verbose mode");
    session.cli( cli );
#endif

    int returnCode = session.applyCommandLine( argc, argv );
    if( returnCode != 0 ) {
        return returnCode;
    }

    yarp::os::Network yarp;

    yarp::os::Property opts;
    opts.put("portdb",":memory:");
    opts.put("subdb",":memory:");
    opts.put("local",1);
    yarp::os::NameStore *store = yarpserver_create(opts);
    yarp.queryBypass(store);

    if (verbose) {
        Network::setVerbosity(1);
    }

    returnCode = session.run();

    yarp.queryBypass(nullptr);
    delete store;

    return returnCode;
}
