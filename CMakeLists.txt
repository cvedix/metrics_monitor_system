cmake_minimum_required(VERSION 3.15)
project(metrics_monitor_system VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Add hwinfo as submodule
set(HWINFO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/hwinfo")
if(EXISTS "${HWINFO_DIR}/CMakeLists.txt")
    add_subdirectory(${HWINFO_DIR})
else()
    message(FATAL_ERROR "hwinfo not found. Please run: ./setup_dependencies.sh")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib)

# Source files
set(SOURCES
    src/main.cpp
    src/system_info.cpp
    src/system_status.cpp
    src/device_config.cpp
    src/config.cpp
    src/json_utils.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    lfreist-hwinfo::hwinfo
    pthread
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

# Copy JSON config files to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.json"
        "${CMAKE_BINARY_DIR}/config.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.json.example"
        "${CMAKE_BINARY_DIR}/config.json.example"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/device_config.example.json"
        "${CMAKE_BINARY_DIR}/device_config.example.json"
    COMMENT "Copying JSON config files to build directory"
)

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES 
    config.json.example
    device_config.example.json
    DESTINATION share/${PROJECT_NAME}
)

