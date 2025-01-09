# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.30)

if (DEFINED ENV{HOMEKIT_PATH})
    set(HOMEKIT_PATH ${ENV_HOMEKIT_PATH})
else ()
    set(HOMEKIT_PATH ${CMAKE_CURRENT_LIST_DIR})
endif(DEFINED ENV{HOMEKIT_PATH})

set(EXTRA_COMPONENT_DIRS ${HOMEKIT_PATH}/components ${HOMEKIT_PATH}/components/homekit src)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(homekit_fan)
