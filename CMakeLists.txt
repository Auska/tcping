cmake_minimum_required(VERSION 3.20)
project(tcping VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tcping main.cpp)

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