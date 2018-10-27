# Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.


################################################################################
# serversql

add_executable(harness_serversql_catch2 ${harness_main}
                                        ServerTest.cpp)

target_include_directories(harness_serversql_catch2 PRIVATE ${hmac_INCLUDE_DIRS})

target_link_libraries(harness_serversql_catch2 PRIVATE YARP::YARP_OS
                                                       YARP::YARP_init
                                                       YARP::YARP_serversql)

target_compile_definitions(harness_serversql_catch2 PRIVATE $<IF:$<BOOL:${USE_SYSTEM_CATCH}>,USE_SYSTEM_CATCH,>)

ParseAndAddCatchTests(harness_serversql_catch2)
