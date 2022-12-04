cmake_minimum_required(VERSION 3.0)

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

set(JAVA_HOME $ENV{JAVA_HOME})
if (NOT JAVA_HOME)
    message(FATAL_ERROR "Cannot find JAVA_HOME in the environment")
endif ()
include_directories(${JAVA_HOME}/include)
include_directories(${JAVA_HOME}/include/darwin)

set(CPP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpp)
set(SRC_LIST ${CPP_PATH}/Navi.cpp
    ${CPP_PATH}/NaviExport.cpp
)
set(SRC_LIST ${SRC_LIST}
    ${CPP_PATH}/Navi.h
)

if (APPLE)
    ADD_DEFINITIONS(-D_MACOSX)
    set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_64_BIT)")
    add_library(RecastJni SHARED ${SRC_LIST})
else ( )
    add_library(RecastJni SHARED ${SRC_LIST})
endif ( )

add_subdirectory(thirdparty/recastnavigation)

add_dependencies(RecastJni Detour DetourTileCache Recast)