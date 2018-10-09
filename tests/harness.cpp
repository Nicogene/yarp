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

#include <yarp/os/Network.h>
#include <yarp/os/Property.h>
#include <yarp/os/NameStore.h>

#include <yarp/serversql/yarpserversql.h>

#include <YarpBuildLocation.h>

using namespace yarp::os;

int main(int argc, char *argv[])
{
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

    // To make sure that the dev test are able to find all the devices
    // compile by YARP, also the one compiled as dynamic plugins
    // we add the build directory to the YARP_DATA_DIRS enviromental variable
    // CMAKE_CURRENT_DIR is the define by the CMakeLists.txt tests file
    std::string dirs = CMAKE_BINARY_DIR +
                       yarp::os::Network::getDirectorySeparator() +
                       "share" +
                       yarp::os::Network::getDirectorySeparator() +
                       "yarp";

    // Add TEST_DATA_DIR to YARP_DATA_DIRS in order to find the contexts used
    // by the tests
    dirs += yarp::os::Network::getPathSeparator() +
            TEST_DATA_DIR;

    // If set, append user YARP_DATA_DIRS
    // FIXME check if this can be removed
    Network::getEnvironment("YARP_DATA_DIRS");
    if (!Network::getEnvironment("YARP_DATA_DIRS").empty()) {
        dirs += yarp::os::Network::getPathSeparator() +
                Network::getEnvironment("YARP_DATA_DIRS");
    }

    Network::setEnvironment("YARP_DATA_DIRS", dirs);
    printf("YARP_DATA_DIRS=\"%s\"\n", Network::getEnvironment("YARP_DATA_DIRS").c_str());


    returnCode = session.run();


    yarp.queryBypass(nullptr);
    delete store;

    return returnCode;
}
