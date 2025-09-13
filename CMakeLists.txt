cmake_minimum_required(VERSION 3.5)

if (WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT" CACHE STRING "")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd  /Od /ZI" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd  /Od /ZI" CACHE STRING "")
endif ()

project(RecastNavigationJNI)

if (NOT CXX_STD)
    set(CXX_STD 14)
endif()
if (NOT WIN32 OR CYGWIN)
    if (CXX_STD EQUAL 17)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
    elseif(CXX_STD EQUAL 14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
else()
    if(CXX_STD EQUAL 17)
        # target_compile_features(yasio PUBLIC cxx_std_17) works
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
    elseif(CXX_STD EQUAL 14)
        # target_compile_features(yasio PUBLIC cxx_std_14) not works
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14")
    endif()
endif()

if (UNIX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

set(JAVA_HOME $ENV{JAVA_HOME})
if (NOT JAVA_HOME)
    message(FATAL_ERROR "Cannot find JAVA_HOME in the environment")
endif ()
include_directories(${JAVA_HOME}/include)
if (APPLE)
    include_directories(${JAVA_HOME}/include/darwin)
elseif (WIN32)
    include_directories(${JAVA_HOME}/include/win32)
else()
    include_directories(${JAVA_HOME}/include/linux)
endif()

set(RECAST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/recastnavigation)
set(JSON_DIR ${RECAST_DIR}/ThirdParty/json)
add_definitions(-DDT_POLYREF64=1)

# include recast library
include(GNUInstallDirs)
add_subdirectory(${RECAST_DIR}/DebugUtils)
add_subdirectory(${RECAST_DIR}/Detour)
add_subdirectory(${RECAST_DIR}/DetourTileCache)
add_subdirectory(${RECAST_DIR}/DetourCrowd)
add_subdirectory(${RECAST_DIR}/Recast)
if (MSVC)
    add_subdirectory(${RECAST_DIR}/RecastDemo)
endif()


include_directories(${RECAST_DIR}/QuadTree/Include)
include_directories(${JSON_DIR}/include)
include_directories(${RECAST_DIR}/Detour/Include)
include_directories(${RECAST_DIR}/DetourTileCache/Include)
include_directories(${RECAST_DIR}/Recast/Include)
include_directories(${RECAST_DIR}/RecastDemo/Contrib/fastlz)
include_directories(${RECAST_DIR}/RecastDemo/Include)

set(CPP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpp)
set(SRC_LIST ${CPP_PATH}/Navi.cpp
    ${CPP_PATH}/NaviExport.cpp
    ${CPP_PATH}/Util.cpp
    ${RECAST_DIR}/RecastDemo/Contrib/fastlz/fastlz.c
    ${RECAST_DIR}/RecastDemo/Source/Filelist.cpp
)
set(SRC_LIST ${SRC_LIST}
    ${CPP_PATH}/Navi.h
    ${CPP_PATH}/Util.h
    ${RECAST_DIR}/RecastDemo/Include/Filelist.h
    ${RECAST_DIR}/QuadTree/Include/QuadTree.h
    ${RECAST_DIR}/QuadTree/Include/AABB.h
    ${RECAST_DIR}/QuadTree/Include/Circle.h
    ${JSON_DIR}/include/nlohmann/json.hpp
)

if (APPLE)
    ADD_DEFINITIONS(-D_MACOSX)
    set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_64_BIT)")
    add_library(RecastJni SHARED ${SRC_LIST})
else ( )
    add_library(RecastJni SHARED ${SRC_LIST})
    target_compile_definitions(RecastJni PRIVATE EXPORT_DLL)
endif ( )

add_dependencies(RecastJni Detour DetourTileCache Recast)
target_link_libraries(RecastJni Detour DetourTileCache Recast)

add_executable(RecastJniTest ${CPP_PATH}/Main.cpp)
add_dependencies(RecastJniTest RecastJni)
target_link_libraries(RecastJniTest RecastJni)

set(RECAST_BIN ${RECAST_DIR}/RecastDemo/Bin)
target_compile_definitions(RecastJniTest PRIVATE -DRECAST_BIN="${RECAST_BIN}")