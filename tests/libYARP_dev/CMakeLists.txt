# Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.


################################################################################
# dev

add_executable(harness_dev_catch2 ${harness_main}
                                  AnalogWrapperTest.cpp
                                  ControlBoardRemapperTest.cpp
                                  FrameTransformClientTest.cpp
                                  MapGrid2DTest.cpp
                                  MultipleAnalogSensorsInterfacesTest.cpp
                                  PolyDriverTest.cpp
                                  robotDescriptionTest.cpp
                                  TestFrameGrabberTest.cpp)

target_link_libraries(harness_dev_catch2 PRIVATE YARP::YARP_OS
                                                 YARP::YARP_init
                                                 YARP::YARP_serversql
                                                 YARP::YARP_sig
                                                 YARP::YARP_math
                                                 YARP::YARP_dev)

target_compile_definitions(harness_dev_catch2 PRIVATE $<IF:$<BOOL:${USE_SYSTEM_CATCH}>,USE_SYSTEM_CATCH,>)

ParseAndAddCatchTests(harness_dev_catch2)

