cmake_minimum_required(VERSION 3.22.1)
project(FlowX)

# Set default build type to Debug if not specified
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)")


################
##HEADER FILES##
################
include_directories(include delay_modeling design_space_exp routing_strategies ~/mosek/10.2/tools/platform/linux64x86/h ../external/pugixml/ ../external/)
#link_directories(../external/mosek/10.0/tools/platform/linux64x86/bin)

###############
##SOURCE FILES##
################
file(GLOB SOURCES main.cpp util.cpp ../external/pugixml/pugixml.cpp "aperiodic_flows/*" "delay_modeling/*" "design_space_exp/*" "modules/*" "routing_strategies/*")
add_executable(FlowX ${SOURCES})

################
##LIBRARY     ##
################
# Import the library into the CMake build system
ADD_LIBRARY(mosek SHARED IMPORTED)
ADD_LIBRARY(fusion SHARED IMPORTED)

# Specify the location of the library 
SET_TARGET_PROPERTIES(FlowX mosek PROPERTIES IMPORTED_LOCATION "~/mosek/10.2/tools/platform/linux64x86/bin/libmosek64.so")
SET_TARGET_PROPERTIES(FlowX fusion PROPERTIES IMPORTED_LOCATION "~/mosek/10.2/tools/platform/linux64x86/bin/libfusion64.so")

target_link_libraries(FlowX mosek m fusion)

