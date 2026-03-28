cmake_minimum_required(VERSION 3.20)
project(tcping VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files (excluding main.cpp for library)
set(LIB_SOURCES
    src/statistics.cpp
    src/error.cpp
    src/args.cpp
    src/tcping.cpp
)

# Header files
set(HEADERS
    src/include/common.h
    src/include/config.h
    src/include/statistics.h
    src/include/error.h
    src/include/args.h
    src/include/tcping.h
    src/include/version.h
)

# Create library for testing
add_library(tcping_lib STATIC ${LIB_SOURCES} ${HEADERS})
target_include_directories(tcping_lib PUBLIC ${CMAKE_SOURCE_DIR}/src/include)

# Main executable
add_executable(tcping src/main.cpp)
target_link_libraries(tcping PRIVATE tcping_lib)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(tcping_lib PRIVATE _WIN32)
    target_link_libraries(tcping_lib PUBLIC ws2_32)
    # Use GCC-style warnings for MinGW, MSVC warnings for MSVC
    if(MINGW)
        target_compile_options(tcping_lib PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    else()
        target_compile_options(tcping_lib PRIVATE /W4)
    endif()
else()
    target_compile_options(tcping_lib PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

install(TARGETS tcping
    RUNTIME DESTINATION bin
)

# Testing setup
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    # Create test executable with amalgamated Catch2
    add_executable(tests
        external/catch_amalgamated.cpp
        src/tests/test_args.cpp
        src/tests/test_statistics.cpp
        src/tests/test_error.cpp
        src/tests/test_tcping.cpp
    )

    target_include_directories(tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_SOURCE_DIR}/external
    )
    target_link_libraries(tests PRIVATE tcping_lib)

    # Enable CTest
    enable_testing()
    add_test(NAME tests COMMAND tests)
endif()
