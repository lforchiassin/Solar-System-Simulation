cmake_minimum_required(VERSION 3.16)
project("orbitalsim")
set(CMAKE_CXX_STANDARD 11)

# Set vcpkg toolchain file
set(CMAKE_TOOLCHAIN_FILE "/home/dylan/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# From "Working with CMake" documentation:
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # AddressSanitizer (ASan)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # UndefinedBehaviorSanitizer (UBSan)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

# Find packages
find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# Find OpenGL and other system dependencies
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# Find X11 for Linux
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(X11 REQUIRED)
endif()

add_executable(orbitalsim main.cpp orbitalSim.cpp view.cpp)

# Platform-specific libraries and linking
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(orbitalsim PRIVATE 
        raylib 
        glfw 
        "-framework IOKit" 
        "-framework Cocoa" 
        "-framework OpenGL"
    )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(orbitalsim PRIVATE 
        raylib 
        glfw 
        OpenGL::GL 
        ${CMAKE_DL_LIBS} 
        Threads::Threads
        ${X11_LIBRARIES}
        m
        rt
    )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(orbitalsim PRIVATE 
        raylib 
        glfw3
        opengl32
        winmm
        gdi32
    )
endif()

# Print debug info
message(STATUS "raylib found: ${raylib_FOUND}")
message(STATUS "glfw3 found: ${glfw3_FOUND}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")