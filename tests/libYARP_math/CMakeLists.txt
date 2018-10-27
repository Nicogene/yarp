# Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.


################################################################################
# dev

add_executable(harness_math_catch2 ${harness_main}
                                   MathTest.cpp
                                   Vec2DTest.cpp
                                   svdTest.cpp
                                   RandTest.cpp)

target_link_libraries(harness_math_catch2 PRIVATE YARP::YARP_OS
                                                  YARP::YARP_init
                                                  YARP::YARP_serversql
                                                  YARP::YARP_sig
                                                  YARP::YARP_math)

target_compile_definitions(harness_math_catch2 PRIVATE $<IF:$<BOOL:${USE_SYSTEM_CATCH}>,USE_SYSTEM_CATCH,>)

ParseAndAddCatchTests(harness_math_catch2)

