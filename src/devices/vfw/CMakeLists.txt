# Copyright: (C) 2009 RobotCub Consortium
# Author: Paul Fitzpatrick
# CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT

yarp_prepare_plugin(vfw_grabber
                    CATEGORY device
                    TYPE yarp::dev::VfwGrabber
                    INCLUDE VfwGrabber.h
                    EXTRA_CONFIG WRAPPER=grabber
                    DEPENDS "CREATE_DEVICE_LIBRARY_MODULES;WIN32")

if(NOT SKIP_vfw_grabber)
  find_library(vfw_library vfw32)
  if(vfw_library)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    yarp_add_plugin(yarp_vfw VfwGrabber.cpp
                             VfwGrabber.h)
    target_link_libraries(yarp_vfw YARP::YARP_OS
                                   YARP::YARP_sig
                                   YARP::YARP_dev
                                   ${vfw_library})
    yarp_install(TARGETS yarp_vfw
                 EXPORT YARP
                 COMPONENT runtime
                 LIBRARY DESTINATION ${YARP_DYNAMIC_PLUGINS_INSTALL_DIR}
                 ARCHIVE DESTINATION ${YARP_STATIC_PLUGINS_INSTALL_DIR})
    yarp_install(FILES vfw.ini
                 COMPONENT runtime
                 DESTINATION ${YARP_PLUGIN_MANIFESTS_INSTALL_DIR})

    set_property(TARGET vfw PROPERTY FOLDER "Unsupported")

    if(YARP_COMPILE_TESTS)
      add_executable(vfw_test vfw_test.cpp
                              VfwGrabber.cpp
                              VfwGrabber.h)
      target_link_libraries(vfw_test YARP::YARP_OS
                                     YARP::YARP_init
                                     YARP::YARP_sig
                                     YARP::YARP_dev
                                     ${vfw_library})
    endif()
  else()
    message(FATAL_ERROR "cannot find video for windows")
  endif()
endif()
