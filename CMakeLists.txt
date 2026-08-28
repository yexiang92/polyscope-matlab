cmake_minimum_required(VERSION 3.15.0)
project(polyscope_matlab LANGUAGES CXX)

# Default to release build
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)

## Build options
option(BUILD_MATLAB_BINDINGS "Build MATLAB MEX interface" ON)

## Gather dependencies

# == Eigen
set(EIGEN3_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/deps/eigen/")

# == polyscope
add_subdirectory(deps/polyscope)

# == MATLAB MEX interface
if (BUILD_MATLAB_BINDINGS)
  find_package(Matlab REQUIRED)
  if(Matlab_FOUND)
    message(STATUS "MATLAB found at ${Matlab_ROOT_DIR}; building polyscope_mex")

    set(MEX_SOURCES
      src/matlab/cpp/polyscope_mex.cpp
      src/matlab/cpp/command_router.cpp
      src/matlab/cpp/matlab_data_utils.cpp
      src/matlab/cpp/bind_core.cpp
      src/matlab/cpp/bind_point_cloud.cpp
      src/matlab/cpp/bind_surface_mesh.cpp
      src/matlab/cpp/bind_curve_network.cpp
      src/matlab/cpp/bind_volume_mesh.cpp
      src/matlab/cpp/bind_volume_grid.cpp
      src/matlab/cpp/bind_sparse_volume_grid.cpp
      src/matlab/cpp/bind_camera_view.cpp
      src/matlab/cpp/bind_transformation_gizmo.cpp
      src/matlab/cpp/bind_slice_plane.cpp
      src/matlab/cpp/bind_imgui.cpp
      src/matlab/cpp/bind_imgui_constant_lookup.cpp
      src/matlab/cpp/bind_implot.cpp
      src/matlab/cpp/bind_implot_constant_lookup.cpp
    )

    matlab_add_mex(
      NAME polyscope_mex
      SRC ${MEX_SOURCES}
      R2018a
      LINK_TO polyscope Matlab::MatlabDataArray
      OUTPUT_NAME polyscope_mex
    )

    target_include_directories(polyscope_mex PRIVATE "${EIGEN3_INCLUDE_DIR}")
    target_include_directories(polyscope_mex PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/matlab/cpp")
    target_include_directories(polyscope_mex PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/polyscope/deps/glad/include")
    target_include_directories(polyscope_mex PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/polyscope/deps/stb")
    set_target_properties(polyscope_mex PROPERTIES CXX_VISIBILITY_PRESET "default")

    if(APPLE)
      message(STATUS "Building MATLAB MEX for macOS architecture: ${CMAKE_OSX_ARCHITECTURES}")
    endif()

    # Copy the produced MEX binary next to the MATLAB package
    add_custom_command(
      TARGET polyscope_mex POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:polyscope_mex>
          ${CMAKE_CURRENT_SOURCE_DIR}/src/matlab/+polyscope/private/$<TARGET_FILE_NAME:polyscope_mex>
      VERBATIM
    )
  endif()
endif() # BUILD_MATLAB_BINDINGS
