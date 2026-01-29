cmake_minimum_required(VERSION 3.20)
project(tcping VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
    src/main.cpp
    src/statistics/statistics.cpp
    src/error/error.cpp
    src/args/args.cpp
    src/tcping.cpp
)

# Header files
set(HEADERS
    src/include/config.h
    src/include/statistics.h
    src/include/error.h
    src/include/args.h
    src/include/tcping.h
    src/include/version.h
)

add_executable(tcping ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(tcping PRIVATE ${CMAKE_SOURCE_DIR}/src/include)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(tcping PRIVATE _WIN32)
    target_link_libraries(tcping ws2_32)
    # Use GCC-style warnings for MinGW, MSVC warnings for MSVC
    if(MINGW)
        target_compile_options(tcping PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
    else()
        target_compile_options(tcping PRIVATE /W4)
    endif()
else()
    target_compile_options(tcping PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

install(TARGETS tcping
    RUNTIME DESTINATION bin
)
