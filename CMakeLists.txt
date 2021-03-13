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
    src/Common/DDSTextureLoader.cpp
    src/Common/DDSTextureLoader.hpp

    src/Chapter8/Exercises/6/LitWaves/FrameResource.hpp
    src/Chapter8/Exercises/6/LitWaves/FrameResource.cpp
    src/Chapter8/Exercises/6/LitWaves/Waves.hpp
    src/Chapter8/Exercises/6/LitWaves/Waves.cpp
    src/Chapter8/Exercises/6/LitWaves/LitWavesApp.cpp

    # src/Chapter8/Exercises/3/FrameResource.hpp
    # src/Chapter8/Exercises/3/FrameResource.cpp
    # src/Chapter8/Exercises/3/ShapesApp.hpp
    # src/Chapter8/Exercises/3/ShapesApp.cpp
    # src/Chapter8/Exercises/5/FrameResource.hpp
    # src/Chapter8/Exercises/5/FrameResource.cpp
    # src/Chapter8/Exercises/5/ShapesApp.hpp
    # src/Chapter8/Exercises/5/ShapesApp.cpp
    # src/Chapter8/Exercises/1/FrameResource.hpp
    # src/Chapter8/Exercises/1/FrameResource.cpp
    # src/Chapter8/Exercises/1/Waves.hpp
    # src/Chapter8/Exercises/1/Waves.cpp
    # src/Chapter8/Exercises/1/LitWavesApp.cpp
    # src/Chapter8/LitWaves/FrameResource.hpp
    # src/Chapter8/LitWaves/FrameResource.cpp
    # src/Chapter8/LitWaves/Waves.hpp
    # src/Chapter8/LitWaves/Waves.cpp
    # src/Chapter8/LitWaves/LitWavesApp.cpp
    # src/Chapter7/Skull/FrameResource.hpp
    # src/Chapter7/Skull/FrameResource.cpp
    # src/Chapter7/Skull/SkullApp.cpp
    # src/Chapter7/Skull/SkullApp.hpp

    # src/Chapter7/FrameResource.hpp
    # src/Chapter7/FrameResource.cpp
    # src/Chapter7/ShapesApp.cpp
    # src/Chapter7/ShapesApp.hpp

    src/io/FileUtil.hpp
    src/io/FileUtil.cpp
    src/io/StringUtil.hpp
    src/io/StringUtil.cpp
)

get_cmake_property(_varNames VARIABLES)
list (REMOVE_DUPLICATES _varNames)
list (SORT _varNames)
foreach (_varName ${_varNames})
    if (_varName MATCHES "_INIT$")
        message(STATUS "${_varName}=${${_varName}}")
    endif()
endforeach()


include_directories(src/)

if(MSVC)

    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_DEBUG)
    add_definitions(-DDEBUG)

    if(NOT CMAKE_C_EXTENSIONS)
        set(CMAKE_C_EXTENSIONS OFF)
    endif()
    target_compile_options(${proj} PRIVATE /W4)
    # target_compile_options(${proj} PRIVATE /EHs-c- /GR-)


endif()

target_compile_definitions(${proj} PRIVATE "UNICODE" "_UNICODE")
target_link_libraries(${proj} PRIVATE "d3d12.lib" "d3dcompiler.lib" "dxgi.lib")