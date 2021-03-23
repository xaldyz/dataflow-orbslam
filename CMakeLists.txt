cmake_minimum_required(VERSION 2.8)
project(ORB_SLAM2)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

SET(USE_CUSTOM_VX FALSE CACHE BOOL "Use Custom OpenVX")
SET(USE_CUSTOM_VX_GPU TRUE CACHE BOOL "In custom OpenVX, use GPU")
SET(USE_PIPELINE FALSE CACHE BOOL "Pipelined feature extraction")
SET(WAIT_PIPELINE TRUE CACHE BOOL "In pipeline mode, wait previous frame fully elaborated")
SET(SYNC_TYPE 0 CACHE INT "Check sync type between Tracking and Local Mapping. 0 = standard, 1 = list, 2 = FULL sync")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wall  -O3 -march=native ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall   -O3 -march=native")

# Check C++11 or C++0x support
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
   add_definitions(-DCOMPILEDWITHC11)
   message(STATUS "Using flag -std=c++11.")
elseif(COMPILER_SUPPORTS_CXX0X)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
   add_definitions(-DCOMPILEDWITHC0X)
   message(STATUS "Using flag -std=c++0x.")
else()
   message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules)

find_package(OpenCV 3.4.3 REQUIRED HINTS /usr/local/opencv-3.4.3)
if(NOT OpenCV_FOUND)
   find_package(OpenCV 2.4.3 QUIET)
   if(NOT OpenCV_FOUND)
      message(FATAL_ERROR "OpenCV > 2.4.3 not found.")
   endif()
endif()

IF(USE_CUSTOM_VX)
    add_definitions(-DCUSTOM_VX)
    add_definitions(-DUSE_NVTX)
    find_library( NVTX_LIBRARY nvToolsExt PATHS ENV LD_LIBRARY_PATH /usr/local/cuda/lib64)
    set(CUSTOM "_custom")
    MESSAGE("Custom OpenVX build!")
ELSE()
	unset(CUSTOM)
	unset(NVTX_LIBRARY)
    remove_definitions(-DCUSTOM_VX)
    remove_definitions(-DUSE_NVTX)
    MESSAGE("Standard ORB-SLAM2!")
ENDIF()

IF(USE_CUSTOM_VX_GPU)
    add_definitions(-DCUSTOM_VX_GPU)
    set(EXECUTOR "_gpu")
    MESSAGE("Custom OpenVX, run on GPU")
ELSE()
	set(EXECUTOR "_cpu")
    remove_definitions(-DCUSTOM_VX_GPU)
    MESSAGE("Custom OpenVX, run on CPU")
ENDIF()

IF(NOT USE_CUSTOM_VX)
	unset(EXECUTOR)
	MESSAGE("Custom OpenVX not set, undoing GPU message")
ENDIF()


IF(USE_PIPELINE)
    add_definitions(-DPIPELINE_FEATURE)
    set(PIPELINE "_pipeline")
    MESSAGE("Pipelined feature extraction")
ELSE()
	unset(PIPELINE)
	
    remove_definitions(-DPIPELINE_FEATURE)
    MESSAGE("NOT Pipelined feature extraction")
ENDIF()

IF(WAIT_PIPELINE)
    add_definitions(-DWAIT_PIPELINE)
    set(WAIT "_wait")
    MESSAGE("Wait for pipeline consumption")
ELSE()
	unset(WAIT)
	
    remove_definitions(-DWAIT_PIPELINE)
    MESSAGE("DON'T wait for pipeline consumption")
ENDIF()

find_package(Eigen3 3.1.0 REQUIRED)
find_package(Pangolin REQUIRED)

find_package(CUDA QUIET REQUIRED)

include_directories(
${PROJECT_SOURCE_DIR}
${PROJECT_SOURCE_DIR}/include
${PROJECT_SOURCE_DIR}/custom_openvx
${EIGEN3_INCLUDE_DIR}
${Pangolin_INCLUDE_DIRS}
${CUDA_INCLUDE_DIRS}
${OpenCV_INCLUDE_DIRS}
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/${CUSTOM}${EXECUTOR}${PIPELINE}${WAIT}_${SYNC_TYPE})

#IF(USE_CUSTOM_VX)
set( SOURCE_CUSTOM_OPENVX

custom_openvx/functions.cpp

custom_openvx/fast.cpp
custom_openvx/fast.cu

custom_openvx/scale.cc
custom_openvx/scale.cu

custom_openvx/gaussian.cc
custom_openvx/gaussian.cu

custom_openvx/compute_grid.cc

custom_openvx/compute_quadtree.cc

custom_openvx/compute_angle.cc

custom_openvx/compute_orb.cc

custom_openvx/compute_scalevector.cc)
#ELSE()
#unset( SOURCE_CUSTOM_OPENVX )
#ENDIF()

cuda_add_library(${PROJECT_NAME} SHARED
src/System.cc
src/Tracking.cc
src/LocalMapping.cc
src/LoopClosing.cc
src/ORBextractor.cc
src/ORBmatcher.cc
src/FrameDrawer.cc
src/Converter.cc
src/MapPoint.cc
src/KeyFrame.cc
src/Map.cc
src/MapDrawer.cc
src/Optimizer.cc
src/PnPsolver.cc
src/Frame.cc
src/KeyFrameDatabase.cc
src/Sim3Solver.cc
src/Initializer.cc
src/Viewer.cc

${SOURCE_CUSTOM_OPENVX}
)

target_link_libraries(${PROJECT_NAME}
${OpenCV_LIBS}
${EIGEN3_LIBS}
${Pangolin_LIBRARIES}
${PROJECT_SOURCE_DIR}/Thirdparty/DBoW2/lib/libDBoW2.so
${PROJECT_SOURCE_DIR}/Thirdparty/g2o/lib/libg2o.so
${NVTX_LIBRARY})

# Build examples

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/build${CUSTOM}${EXECUTOR}${PIPELINE}${WAIT}_${SYNC_TYPE}/RGB-D)

#cuda_add_executable(rgbd_tum
#Examples/RGB-D/rgbd_tum.cc)
#target_link_libraries(rgbd_tum ${PROJECT_NAME})

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/build${CUSTOM}${EXECUTOR}${PIPELINE}${WAIT}_${SYNC_TYPE}/Stereo)

#cuda_add_executable(stereo_kitti
#Examples/Stereo/stereo_kitti.cc)
#target_link_libraries(stereo_kitti ${PROJECT_NAME})

#cuda_add_executable(stereo_euroc
#Examples/Stereo/stereo_euroc.cc)
#target_link_libraries(stereo_euroc ${PROJECT_NAME})


#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/build${CUSTOM}${EXECUTOR}${PIPELINE}${WAIT}_${SYNC_TYPE}/Monocular)

#cuda_add_executable(mono_tum
#Examples/Monocular/mono_tum.cc)
#target_link_libraries(mono_tum ${PROJECT_NAME})

cuda_add_executable(mono_kitti
Examples/Monocular/mono_kitti.cc)
target_link_libraries(mono_kitti ${PROJECT_NAME})

#cuda_add_executable(mono_euroc
#Examples/Monocular/mono_euroc.cc)
#target_link_libraries(mono_euroc ${PROJECT_NAME})

#cuda_add_executable(test_kitti
#Examples/Test/mono_kitti.cc)
#target_link_libraries(test_kitti ${PROJECT_NAME})
