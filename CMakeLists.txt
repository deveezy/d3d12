cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(proj d3d12)
project(${proj} CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${proj} WIN32
    src/Common/d3dApp.cpp 
    src/Common/d3dApp.hpp 
    src/Common/d3dUtil.cpp 
    src/Common/d3dUtil.hpp 
    src/Common/d3dx12.h 
    src/Common/defines.hpp 
    src/Common/GameTimer.hpp 
    src/Common/GameTimer.cpp 
    src/Common/GeometryGenerator.hpp 
    src/Common/GeometryGenerator.cpp 
    src/Common/MathHelper.hpp 
    src/Common/MathHelper.cpp 
    src/Common/UploadBuffer.hpp 
    src/Chapter8/LitWaves/FrameResource.hpp
    src/Chapter8/LitWaves/FrameResource.cpp
    src/Chapter8/LitWaves/Waves.hpp
    src/Chapter8/LitWaves/Waves.cpp
    src/Chapter8/LitWaves/LitWavesApp.cpp
)


include_directories(src/)

if(MSVC)

    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_DEBUG)
    add_definitions(-DDEBUG)

    if(NOT CMAKE_C_EXTENSIONS)
        set(CMAKE_C_EXTENSIONS OFF)
    endif()

endif()

target_compile_definitions(${proj} PRIVATE "UNICODE" "_UNICODE")
target_link_libraries(${proj} PRIVATE "d3d12.lib" "d3dcompiler.lib" "dxgi.lib")