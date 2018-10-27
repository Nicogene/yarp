# Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.


################################################################################
# OS

add_executable(harness_os_catch2 ${harness_main}
                                 BinPortableTest.cpp
                                 BottleTest.cpp
                                 ContactTest.cpp
                                 ElectionTest.cpp
                                 EventTest.cpp
                                 LogStreamTest.cpp
                                 LogTest.cpp
                                 MessageStackTest.cpp
                                 NetTypeTest.cpp
                                 NetworkTest.cpp
                                 NodeTest.cpp
                                 PeriodicThreadTest.cpp
                                 PortablePairTest.cpp
                                 PortReaderBufferTest.cpp
                                 PortTest.cpp
                                 PropertyTest.cpp
                                 PublisherTest.cpp
                                 ResourceFinderTest.cpp
                                 RFModuleTest.cpp
                                 RouteTest.cpp
                                 SemaphoreTest.cpp
                                 StampTest.cpp
                                 StringInputStreamTest.cpp
                                 StringOutputStreamTest.cpp
                                 SystemInfoTest.cpp
                                 TerminatorTest.cpp
                                 ThreadTest.cpp
                                 TimerTest.cpp
                                 TimeTest.cpp
                                 ValueTest.cpp
                                 VocabTest.cpp)

target_include_directories(harness_os_catch2 PRIVATE ${hmac_INCLUDE_DIRS})

target_link_libraries(harness_os_catch2 PRIVATE YARP::YARP_OS
                                                YARP::YARP_init
                                                YARP::YARP_sig
                                                YARP::YARP_dev
                                                YARP::YARP_serversql
                                                YARP::YARP_companion)

target_compile_definitions(harness_os_catch2 PRIVATE $<IF:$<BOOL:${USE_SYSTEM_CATCH}>,USE_SYSTEM_CATCH,>)

ParseAndAddCatchTests(harness_os_catch2)


################################################################################
# OS/impl

add_executable(harness_os_impl_catch2 ${harness_main}
                                      BufferedConnectionWriterTest.cpp
                                      DgramTwoWayStreamTest.cpp
                                      NameConfigTest.cpp
                                      NameServerTest.cpp
                                      PortCommandTest.cpp
                                      PortCoreTest.cpp
                                      ProtocolTest.cpp
                                      StreamConnectionReaderTest.cpp)

target_include_directories(harness_os_impl_catch2 PRIVATE ${ACE_INCLUDE_DIRS})

target_link_libraries(harness_os_impl_catch2 PRIVATE YARP::YARP_OS
                                                     YARP::YARP_init
                                                     YARP::YARP_sig
                                                     YARP::YARP_serversql
                                                     ${ACE_LIBRARIES})

target_compile_definitions(harness_os_impl_catch2 PRIVATE $<IF:$<BOOL:${USE_SYSTEM_CATCH}>,USE_SYSTEM_CATCH,>)

ParseAndAddCatchTests(harness_os_impl_catch2)

