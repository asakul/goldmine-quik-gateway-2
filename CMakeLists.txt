cmake_minimum_required(VERSION 3.0)
set(PROJECT goldmine-quik-gateway)
project(${PROJECT})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Catch)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Catch/single_include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jsoncpp/json)

set(CMAKE_CXX_FLAGS "-std=gnu++11")

# boost
set (BOOST_ROOT ${CMAKE_PREFIX_PATH})
set (Boost_USE_STATIC_LIBS ON)
set (Boost_USE_MULTITHREADED ON)

add_definitions(-DBOOST_THREAD_USE_LIB)

find_package(Boost COMPONENTS system thread program_options date_time log_setup log filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(src
	3rdparty/jsoncpp/jsoncpp.cpp

	log.cpp
)

add_executable(${PROJECT} main.cpp ${src})
target_link_libraries(${PROJECT} ${Boost_LIBRARIES} -L${CMAKE_CURRENT_BINARY_DIR}/../goldmine -lgoldmine)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
	
set(test_sources
	tests/test.cpp
	)
	
add_executable(${PROJECT}-test ${test_sources} ${src})
set_target_properties(${PROJECT}-test PROPERTIES COMPILE_FLAGS "-O0 -g -fprofile-arcs -ftest-coverage")
set_target_properties(${PROJECT}-test PROPERTIES LINK_FLAGS "-fprofile-arcs -lgcov")
target_link_libraries(${PROJECT}-test  ${Boost_LIBRARIES} -L${CMAKE_CURRENT_BINARY_DIR}/../goldmine -lgoldmine)

if(UNIX)
	target_link_libraries(${PROJECT}-test ${Boost_LIBRARIES} -lpthread -ldl)
endif(UNIX)
