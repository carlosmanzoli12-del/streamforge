cmake_minimum_required(VERSION 3.24)
project(StreamForgeVertical VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

option(STREAMFORGE_BUILD_OBS_PLUGIN "Build the OBS Studio plugin" ON)

if(STREAMFORGE_BUILD_OBS_PLUGIN)
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
  endif()
endif()
