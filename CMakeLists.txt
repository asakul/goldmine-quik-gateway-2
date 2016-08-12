cmake_minimum_required(VERSION 3.0)
set(PROJECT goldmine-quik-gateway)
project(${PROJECT})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Catch)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Catch/single_include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jsoncpp/json)

set(CMAKE_CXX_FLAGS "-std=gnu++11")
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

# boost
set (Boost_USE_STATIC_LIBS OFF)
set (Boost_USE_MULTITHREADED ON)

add_definitions(-DBOOST_THREAD_USE_LIB)

find_package(Boost COMPONENTS system thread program_options date_time filesystem log_setup log REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libgoldmine)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libcppio/include)

set(src
	3rdparty/jsoncpp/jsoncpp.cpp

	binary/rawbytearrayparser.cpp

	core/core.cpp
	core/dataimportserver.cpp
	core/quotetable.cpp

	core/broker/paperbroker.cpp
	core/broker/quikbroker.cpp
	core/broker/trans2quik/trans2quik.cpp

	core/tables/tableparserfactoryregistry.cpp
	core/tables/tableconstructor.cpp
	core/tables/parsers/currentparametertableparser.cpp
	core/tables/parsers/alldealstableparser.cpp

	xl/xlparser.cpp
	xl/xltable.cpp

	log.cpp

	ui/mainwindow.cpp
)

add_executable(${PROJECT} main.cpp ${src})
target_link_libraries(${PROJECT} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT} -L${CMAKE_CURRENT_BINARY_DIR}/../libgoldmine -lgoldmine -L${CMAKE_CURRENT_BINARY_DIR}/../libcppio -lcppio -lfltk)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(test_sources
	tests/test.cpp

	tests/xl_test.cpp
	)

add_executable(${PROJECT}-test ${test_sources} ${src})
set_target_properties(${PROJECT}-test PROPERTIES COMPILE_FLAGS "-O0 -g -fprofile-arcs -ftest-coverage")
set_target_properties(${PROJECT}-test PROPERTIES LINK_FLAGS "-fprofile-arcs -lgcov")
target_link_libraries(${PROJECT}-test  ${Boost_LIBRARIES} -L${CMAKE_CURRENT_BINARY_DIR}/../libgoldmine -lgoldmine -L${CMAKE_CURRENT_BINARY_DIR}/../libcppio -lcppio -lfltk)

if(UNIX)
	target_link_libraries(${PROJECT}-test ${Boost_LIBRARIES} -lpthread -ldl)
endif(UNIX)
