# Copyright (C) 2010 RobotCub Consortium
# Authors: Paul Fitzpatrick
# CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT

yarp_prepare_plugin(human
                    CATEGORY carrier
                    TYPE HumanCarrier
                    INCLUDE HumanCarrier.h
                    EXTRA_CONFIG CODE="HUMANITY"
                    DEPENDS CREATE_OPTIONAL_CARRIERS)

if(NOT SKIP_human)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  yarp_add_plugin(yarp_human HumanCarrier.h
                             HumanStream.h
                             HumanCarrier.cpp
                             HumanStream.cpp)
  target_link_libraries(yarp_human YARP::YARP_OS)
  yarp_install(TARGETS yarp_human
               EXPORT YARP
               COMPONENT runtime
               LIBRARY DESTINATION ${YARP_DYNAMIC_PLUGINS_INSTALL_DIR}
               ARCHIVE DESTINATION ${YARP_STATIC_PLUGINS_INSTALL_DIR})
  yarp_install(FILES human.ini
               COMPONENT runtime
               DESTINATION ${YARP_PLUGIN_MANIFESTS_INSTALL_DIR})

  set_property(TARGET yarp_human PROPERTY FOLDER "Plugins/Carrier")
endif()
