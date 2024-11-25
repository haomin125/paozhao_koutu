# cmakelists: lvsuPz_xj_server

cmake_minimum_required (VERSION 3.5)

project (lvsuPz_xj_server VERSION 1.0.0)

set (CMAKE_BUILD_TYPE,"")

enable_testing()

# version
configure_file (version.h.in version.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package (OpenCV REQUIRED)
include_directories (${OpenCV_INCLUDE_DIRS})

# tensorrt
include_directories(/opt/TensorRT/targets/x86_64-linux-gnu/include)
link_directories(/opt/TensorRT/targets/x86_64-linux-gnu/lib)
include_directories(/usr/local/TensorRT-8.2.2.1/targets/x86_64-linux-gnu/include)
link_directories(/usr/local/TensorRT-8.2.2.1/targets/x86_64-linux-gnu/lib)

if(UNIX)
    include_directories ("/usr/local/include/opencv" "/usr/local/include/opencv2")
    link_directories("/usr/local/lib")
    link_directories("/usr/local/cuda/lib64")

endif()

if(UNIX)
	set(BOOST_LIBRARYDIR "/usr/local/lib")
    find_package(Boost 1.68.0 EXACT REQUIRED COMPONENTS system filesystem thread)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})

else()
    set(Boost_USE_STATIC_LIBS ON)
    find_package(Boost 1.68.0 REQUIRED COMPONENTS system filesystem thread)
    include_directories ("$ENV{BOOST_ROOT}")
    link_directories ("$ENV{BOOST_ROOT}/lib64-msvc-14.1")
endif()

if(UNIX)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread")
    set (INSTALL_CONFIG_DIR /opt/config)
    # the sequence is really IMPORTANT
    # depency libs should be put after
	set (XJ_SERVER_LIBS product_line board view target_object bound_box camera camera_manager config  classifier_result detector io_card_manager io_mxe5500_manager io_manager plctag logger thread_pool tracker shared_utils apb-web-server web-server database workflow customized_json_config opencv_utils)
else()
    set (INSTALL_CONFIG_DIR C:\\opt\\config)
    set (XJ_SERVER_LIBS product_line board view target_object bound_box camera camera_manager config  classifier_result detector io_manager plctag logger thread_pool tracker shared_utils apb-web-server web-server database workflow customized_json_config opencv_utils utility)
endif()

if(UNIX)
	set(PYLON_LIBS pylonutility pylonbase GCBase_gcc_v3_1_Basler_pylon_v5_1)

	set(PYLON_INCLUDE "/opt/pylon5/include")
	set(PYLON_LIBS_DIR "/opt/pylon5/lib64")
else()
	set(PYLON_LIBS PylonUtility_v5_1 PylonBase_v5_1 GCBase_MD_VC141_v3_1_Basler_pylon_v5_1)

	set(PYLON_INCLUDE "$ENV{PYLON_ROOT}/Development/include")
	set(PYLON_LIBS_DIR "$ENV{PYLON_ROOT}/Development/lib/x64")
endif()
include_directories(${PYLON_INCLUDE})
link_directories(${PYLON_LIBS_DIR})

set (XJ_SERVER_LIBS ${XJ_SERVER_LIBS} camera_basler config_basler)






if(UNIX)
	set(BIODAQ_LIBS biodaq biodaqutil)

	set(IOCARD_LIBS_DIR "/opt/advantech/libs")
	set(IOCARD_INCLUDE "/opt/advantech/inc")

	include_directories(${IOCARD_INCLUDE})
    link_directories(${IOCARD_LIBS_DIR})
endif()


if(UNIX)
	# eigen lib header
	include_directories("/usr/local/include/eigen3")

	# tf lib header
	include_directories("/usr/local/include/absl/")
	include_directories("/usr/local/include/tf/")
	include_directories("/usr/local/include/tf/bazel-genfiles/")
	include_directories("/usr/local/include/tf/tensorflow/")
	include_directories("/usr/local/include/tf/third-party/")

    set(TENSORFLOW_LIBS tensorflow_cc tensorflow_framework)
	message(STATUS "===============" ${CMAKE_CXX_FLAGS} "===============")
endif()

if(UNIX)
	include_directories ("/opt/include")
	link_directories ("/opt/lib")
else()
	include_directories ("C:\\opt\\include")
	link_directories ("C:\\opt\\lib")
endif()

set(SRC_FILES main.cpp xj_app_server.cpp xj_app_config.cpp xj_app_tracker.cpp xj_app_detector.cpp xj_app_workflow.cpp xj_app_io_manager.cpp xj_app_web_server.cpp tensorrt_classifier.cpp)


add_executable (xjserver ${SRC_FILES})
target_link_libraries (xjserver ${XJ_SERVER_LIBS} ${Boost_LIBRARIES} ${OpenCV_LIBS} nvinfer cudart )


#######################################################################################################################
# if(UNIX)
#    target_link_libraries (xjserver ${TENSORFLOW_LIBS})
#    target_link_libraries (xjserver -ldl -lstdc++fs)
# endif()

if(UNIX)
	set(Torch_DIR /usr/local/libtorch/share/cmake/Torch)

	find_package(Torch REQUIRED)

	include_directories(${TORCH_INCLUDE_DIRS})

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPYTORCH")
#	set(XJ_SERVER_LIBS ${XJ_SERVER_LIBS} classifier)
	message(STATUS "===============" ${CMAKE_CXX_FLAGS} "===============")
endif()

if(UNIX)
	target_link_libraries (xjserver ${TORCH_LIBRARIES})
    target_link_libraries (xjserver -ldl -lstdc++fs)
endif()
#######################################################################################################################

target_link_libraries (xjserver ${PYLON_LIBS})
target_include_directories (xjserver PRIVATE ${PYLON_INCLUDE})


if(UNIX)
	target_link_libraries (xjserver ${BIODAQ_LIBS})
	target_include_directories (xjserver PRIVATE ${IOCARD_INCLUDE})
endif()


target_link_libraries (xjserver apb-web-server)

target_include_directories (xjserver PRIVATE ${Boost_INCLUDE_DIR})

add_subdirectory (test)
add_test (NAME app_test COMMAND app_test)

# install configuration file
install (FILES lvsuPz_config.json configuration.json DESTINATION ${INSTALL_CONFIG_DIR})

#add_definitions(-DTF2)
add_definitions(-DTORCH)
# add_definitions(-DBEFORE_SEALING)
add_definitions(-DAFTER_SEALING)