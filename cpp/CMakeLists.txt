cmake_minimum_required (VERSION 3.16)
project (cjdemo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE "Release")

set(${PROJECT_NAME}_VERSION_MAJOR 1)
set(${PROJECT_NAME}_VERSION_MINOR 0)

include_directories( ${CMAKE_SOURCE_DIR}/include/ )

# CGAL
find_package( CGAL QUIET COMPONENTS )
if ( CGAL_FOUND )
  message(STATUS "CGAL found")
  message(STATUS ${CGAL_LIBRARIES})
  message(STATUS ${CGAL_3RD_PARTY_LIBRARIES})
else()
  message(SEND_ERROR "CGAL library is required")
  return()  
endif()

find_package(Eigen3 REQUIRED)
include(CGAL_Eigen3_support)

add_executable(cjdemo src/main.cpp
        src/main.h)

target_compile_options(cjdemo PRIVATE -Wall)

target_link_libraries(${PROJECT_NAME} CGAL::CGAL CGAL::Eigen3_support)
