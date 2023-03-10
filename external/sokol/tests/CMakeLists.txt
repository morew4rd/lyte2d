cmake_minimum_required(VERSION 3.10)
project(sokol-test)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)  # needed for UWP

# SOKOL_GLCORE33, SOKOL_GLES2, SOKOL_GLES3, SOKOL_D3D11, SOKOL_METAL, SOKOL_WGPU, SOKOL_DUMMY
set(SOKOL_BACKEND "SOKOL_DUMMY_BACKEND" CACHE STRING "Select 3D backend API")
set_property(CACHE SOKOL_BACKEND PROPERTY STRINGS SOKOL_GLCORE33 SOKOL_METAL SOKOL_D3D11 SOKOL_DUMMY_BACKEND)
option(SOKOL_FORCE_EGL "Force EGL with GLCORE33 backend" OFF)
option(USE_ARC "Enable/disable ARC" OFF)
option(USE_ANALYZER "Enable/disable clang analyzer" OFF)

if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(EMSCRIPTEN 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL iOS)
    set(OSX_IOS 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL Android)
    set(ANDROID 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(LINUX 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL Darwin)
    set(OSX_MACOS 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
    set(UWP 1)
elseif (CMAKE_SYSTEM_NAME STREQUAL Windows)
    set(WINDOWS 1)
else()
    message(FATAL_ERROR "Unrecognized CMAKE_SYSTEM_NAME")
endif()

message(">> CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(">> SOKOL_BACKEND: ${SOKOL_BACKEND}")
message(">> SOKOL_FORCE_EGL: ${SOKOL_FORCE_EGL}")
if (OSX_IOS OR OSX_MACOS)
    if (USE_ARC)
        message(">> ObjC ARC ENABLED")
    else()
        message(">> ObjC ARC DISABLED")
    endif()
endif()
message(">> BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(">> TOOLCHAIN: ${CMAKE_TOOLCHAIN_FILE}")

set(c_flags)
set(cxx_flags)
set(link_flags)
set(system_libs)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(c_flags ${c_flags} /W4 /WX /D_CRT_SECURE_NO_WARNINGS)
    set(cxx_flags ${cxx_flags} /W4 /WX /EHsc /D_CRT_SECURE_NO_WARNINGS)
else()
    set(c_flags ${c_flags} -Wall -Wextra -Werror -Wsign-conversion)
    set(cxx_flags ${cxx_flags} -Wall -Wextra -Werror -Wsign-conversion -fno-rtti -fno-exceptions)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(c_flags ${c_flags} -Wno-missing-field-initializers)
        set(cxx_flags ${cxx_flags} -Wno-missing-field-initializers)
    endif()
    if (USE_ANALYZER)
        # FIXME: consider using clang-tidy via CMAKE_CXX_CLANG_TIDY: https://ortogonal.github.io/cmake-clang-tidy/
        # with the default settings this spams the output with irrelevant C++ coding style warnings in 3rd party libs though
        message(">> Configuring for static code analysis")
        set(c_flags ${c_flags} --analyze -Xanalyzer -analyzer-opt-analyze-headers)
        set(cxx_flags ${cxx_flags} --analyze -Xanalyzer -analyzer-opt-analyze-headers)
        set(link_flags ${link_flags} --analyze -Wno-unused-command-line-argument)
    endif()
endif()

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(link_flags ${link-flags} -sNO_FILESYSTEM=1 -sASSERTIONS=0 -sMALLOC=emmalloc -sINITIAL_MEMORY=33554432 --closure=1)
elseif (OSX_IOS)
    set(exe_type MACOSX_BUNDLE)
    if (USE_ARC)
        set(c_flags ${c_flags} -fobjc-arc)
        set(cxx_flags ${cxx_flags} -fobjc-arc)
    endif()
    set(system_libs ${system_libs} "-framework Foundation" "-framework UIKit" "-framework AudioToolbox" "-framework AVFoundation")
    if (SOKOL_BACKEND STREQUAL SOKOL_METAL)
        set(system_libs ${system_libs} "-framework Metal" "-framework MetalKit")
    else()
        set(system_libs ${system_libs} "-framework OpenGLES" "-framework GLKit")
    endif()
elseif (ANDROID)
    set(system_libs ${system_libs} GLESv3 EGL OpenSLES log android)
    # FIXME
elseif (LINUX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    if ((SOKOL_BACKEND STREQUAL SOKOL_GLES3) OR SOKOL_FORCE_EGL)
        set(system_libs ${system_libs} X11 Xi Xcursor EGL GL asound dl m Threads::Threads)
    else()
        set(system_libs ${system_libs} X11 Xi Xcursor GL asound dl m Threads::Threads)
    endif()
elseif (OSX_MACOS)
    set(exe_type MACOSX_BUNDLE)
    if (USE_ARC)
        set(c_flags ${c_flags} -fobjc-arc)
        set(cxx_flags ${cxx_flags} -fobjc-arc)
    endif()
    set(system_libs ${system_libs} "-framework QuartzCore" "-framework Cocoa" "-framework AudioToolbox")
    if (SOKOL_BACKEND STREQUAL SOKOL_METAL)
        set(system_libs ${system_libs} "-framework MetalKit" "-framework Metal")
    else()
        set(system_libs ${system_libs} "-framework OpenGL")
    endif()
elseif (UWP)
    set(exe_type WIN32)
elseif (WINDOWS)
    set(exe_type WIN32)
endif()

macro(configure_common target)
    if (SOKOL_FORCE_EGL)
        target_compile_definitions(${target} PRIVATE SOKOL_FORCE_EGL)
    endif()
    target_compile_definitions(${target} PRIVATE ${SOKOL_BACKEND})
    target_link_options(${target} PRIVATE ${link_flags})
    target_link_libraries(${target} PRIVATE ${system_libs})
    target_include_directories(${target} PRIVATE ../.. ../../util)
    target_include_directories(${target} PRIVATE ../ext)
endmacro()

macro(configure_osx_properties target)
    if (OSX_IOS)
        target_compile_definitions(${target} PRIVATE GLES_SILENCE_DEPRECATION)
    endif()
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${target}")
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "${target}")
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_PRODUCT_NAME "${target}")
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "${target}")
endmacro()

macro(configure_c target)
    configure_common(${target})
    target_compile_options(${target} PRIVATE ${c_flags})
    if (OSX_MACOS OR OSX_IOS)
        target_compile_options(${target} PRIVATE -x objective-c)
        configure_osx_properties(${target})
    endif()
endmacro()

macro(configure_cxx target)
    configure_common(${target})
    target_compile_options(${target} PRIVATE ${cxx_flags})
    if (OSX_MACOS OR OSX_IOS)
        target_compile_options(${target} PRIVATE -x objective-c++)
        configure_osx_properties(${target})
    endif()
endmacro()

#--- cimgui
add_library(cimgui
    ext/fips-cimgui/cimgui/cimgui.cpp
    ext/fips-cimgui/cimgui/imgui/imgui.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_demo.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_draw.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_tables.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_widgets.cpp)
target_include_directories(cimgui SYSTEM PUBLIC ext/fips-cimgui)

add_library(imgui
    ext/fips-cimgui/cimgui/imgui/imgui.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_demo.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_draw.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_tables.cpp
    ext/fips-cimgui/cimgui/imgui/imgui_widgets.cpp)
target_include_directories(imgui SYSTEM PUBLIC ext/fips-cimgui/cimgui/imgui)

add_library(spine
    ext/spine-runtimes/spine-c/spine-c/src/spine/Animation.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/AnimationState.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/AnimationStateData.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Array.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Atlas.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/AtlasAttachmentLoader.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Attachment.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/AttachmentLoader.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Bone.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/BoneData.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/BoundingBoxAttachment.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/ClippingAttachment.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Color.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Debug.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Event.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/EventData.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/IkConstraint.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/IkConstraintData.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Json.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Json.h
    ext/spine-runtimes/spine-c/spine-c/src/spine/MeshAttachment.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/PathAttachment.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/PathConstraint.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/PathConstraintData.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/PointAttachment.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/RegionAttachment.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Sequence.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Skeleton.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/SkeletonBinary.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/SkeletonBounds.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/SkeletonClipping.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/SkeletonData.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/SkeletonJson.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Skin.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Slot.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/SlotData.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/TransformConstraint.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/TransformConstraintData.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/Triangulator.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/VertexAttachment.c
    ext/spine-runtimes/spine-c/spine-c/src/spine/extension.c)
target_include_directories(spine SYSTEM PUBLIC ext/spine-runtimes/spine-c/spine-c/include)
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(spine PRIVATE /wd4267 /wd4244)   # conversion from 'x' to 'y' possible loss of data
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(spine PRIVATE -Wno-shorten-64-to-32)
endif()


add_library(nuklear ext/nuklear.c)

add_subdirectory(compile)
add_subdirectory(functional)
