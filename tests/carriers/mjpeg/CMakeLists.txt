# Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.

if(YARP_HAS_JPEG)
  if(MJPEG_AUTOCOMPRESS)
    add_definitions(-DMJPEG_AUTOCOMPRESS)
  endif()

  include_directories(SYSTEM ${JPEG_INCLUDE_DIR})
  include_directories("${CMAKE_SOURCE_DIR}/src/carriers/mjpeg_carrier/")

  add_executable(harness_mjpeg_catch2 ${harness_main}
                                      test_mjpeg.cpp
                                      ${CMAKE_SOURCE_DIR}/src/carriers/mjpeg_carrier/MjpegDecompression.h
                                      ${CMAKE_SOURCE_DIR}/src/carriers/mjpeg_carrier/MjpegDecompression.cpp)

  target_link_libraries(harness_mjpeg_catch2 PRIVATE YARP::YARP_OS
                                                     YARP::YARP_init
                                                     YARP::YARP_serversql
                                                     YARP::YARP_dev
                                                     YARP::YARP_sig
                                                     ${JPEG_LIBRARY})

  target_compile_definitions(harness_mjpeg_catch2 PRIVATE $<IF:$<BOOL:${USE_SYSTEM_CATCH}>,USE_SYSTEM_CATCH,>)

  ParseAndAddCatchTests(harness_mjpeg_catch2)
endif()
