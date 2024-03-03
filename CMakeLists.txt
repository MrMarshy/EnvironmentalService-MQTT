# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(MQTTTest)

target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/credentials/client.crt" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/credentials/client.key" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "main/credentials/thingspeak.crt" TEXT)
