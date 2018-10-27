# Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.


################################################################################
# sig

add_executable(harness_sig_catch2 ${harness_main}
                                  ImageTest.cpp
                                  MatrixTest.cpp
                                  PointCloudTest.cpp
                                  SoundTest.cpp
                                  VectorOfTest.cpp
                                  VectorTest.cpp)
target_link_libraries(harness_sig_catch2 PRIVATE YARP::YARP_OS
                                                 YARP::YARP_init
                                                 YARP::YARP_serversql
                                                 YARP::YARP_sig
                                                 YARP::YARP_gsl)

target_compile_definitions(harness_sig_catch2 PRIVATE $<IF:$<BOOL:${USE_SYSTEM_CATCH}>,USE_SYSTEM_CATCH,>)

ParseAndAddCatchTests(harness_sig_catch2)
