cmake_minimum_required(VERSION 3.24)
project(StreamForgeVertical VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

option(STREAMFORGE_BUILD_OBS_PLUGIN "Build the OBS Studio plugin" ON)
option(STREAMFORGE_BUILD_INSTALLER "Enable the Windows NSIS installer package" ON)

if(NOT STREAMFORGE_BUILD_OBS_PLUGIN)
  message(FATAL_ERROR "StreamForge Vertical is an OBS plugin; configure with STREAMFORGE_BUILD_OBS_PLUGIN=ON.")
endif()

find_package(libobs CONFIG REQUIRED)
find_package(obs-frontend-api CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Widgets)

add_library(streamforge-vertical MODULE
  src/vertical_canvas.cpp
)

target_include_directories(streamforge-vertical PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(streamforge-vertical PRIVATE
  OBS::libobs
  OBS::obs-frontend-api
  Qt6::Core
  Qt6::Widgets
)

set_target_properties(streamforge-vertical PROPERTIES
  PREFIX ""
  OUTPUT_NAME "streamforge-vertical"
  AUTOMOC ON
)

if(WIN32)
  set_target_properties(streamforge-vertical PROPERTIES SUFFIX ".dll")
  # Standard system-wide OBS plugin location, relative to the selected OBS root.
  install(TARGETS streamforge-vertical
    RUNTIME DESTINATION "obs-plugins/64bit"
    LIBRARY DESTINATION "obs-plugins/64bit"
  )
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    DESTINATION "."
  )

  if(STREAMFORGE_BUILD_INSTALLER)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "StreamForge Vertical")
    set(CPACK_PACKAGE_VENDOR "StreamForge")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Vertical canvas plugin for OBS Studio 30.0.2")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "obs-studio")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_DISPLAY_NAME "StreamForge Vertical for OBS Studio")
    set(CPACK_NSIS_PACKAGE_NAME "StreamForge Vertical")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "StreamForgeVertical")
    include(CPack)
  endif()
endif()
