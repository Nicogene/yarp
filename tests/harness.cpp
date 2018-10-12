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

namespace {

static yarp::os::Network* net = nullptr;
static yarp::os::NameStore* store = nullptr;
static bool verbose = false;

static void setup_Environment()
{
    // To make sure that the dev test are able to find all the devices
    // compile by YARP, also the one compiled as dynamic plugins
    // we add the build directory to the YARP_DATA_DIRS enviromental variable
    // CMAKE_CURRENT_DIR is the define by the CMakeLists.txt tests file
    std::string dirs = CMAKE_BINARY_DIR +
                       yarp::os::NetworkBase::getDirectorySeparator() +
                       "share" +
                       yarp::os::NetworkBase::getDirectorySeparator() +
                       "yarp";

    // Add TEST_DATA_DIR to YARP_DATA_DIRS in order to find the contexts used
    // by the tests
    dirs += yarp::os::NetworkBase::getPathSeparator() +
            TEST_DATA_DIR;

    // If set, append user YARP_DATA_DIRS
    // FIXME check if this can be removed
    yarp::os::NetworkBase::getEnvironment("YARP_DATA_DIRS");
    if (!yarp::os::NetworkBase::getEnvironment("YARP_DATA_DIRS").empty()) {
        dirs += yarp::os::NetworkBase::getPathSeparator() +
                yarp::os::NetworkBase::getEnvironment("YARP_DATA_DIRS");
    }

    yarp::os::NetworkBase::setEnvironment("YARP_DATA_DIRS", dirs);

    if (verbose) {
        printf("YARP_DATA_DIRS=\"%s\"\n", yarp::os::NetworkBase::getEnvironment("YARP_DATA_DIRS").c_str());
    }
}

static void init_Network()
{
    net = new yarp::os::Network;
    if (verbose) {
        yarp::os::NetworkBase::setVerbosity(1);
    }
}

static void fini_Network()
{
    delete net;
    net = nullptr;
}

static void init_NameStore()
{
    assert(net != nullptr);

    yarp::os::Property opts;
    opts.put("portdb",":memory:");
    opts.put("subdb",":memory:");
    opts.put("local",1);
    if (verbose) {
        opts.put("verbose", 1);
    }
    store = yarpserver_create(opts);
    net->queryBypass(store);
}

static void fini_NameStore()
{
    net->queryBypass(nullptr);
    delete store;
    store = nullptr;
}

} // namespace


int main(int argc, char *argv[])
{
    Catch::Session session;

#if defined(CATCH_VERSION_MAJOR) && (CATCH_VERSION_MAJOR>=2)
    using namespace Catch::clara;
    auto cli = session.cli() | Opt(verbose)["--yarp-verbose"]("Enable verbose mode");
    session.cli( cli );
#endif

    int returnCode = session.applyCommandLine( argc, argv );
    if( returnCode != 0 ) {
        return returnCode;
    }

    setup_Environment();

    init_Network();
    init_NameStore();

    returnCode = session.run();

    fini_NameStore();
    fini_Network();

    return returnCode;
}
