cmake_minimum_required(VERSION 3.0) 

project(TwinStar)

set(BOOST_ROOT "../../../3rd/boost_1_66")
find_package(Boost 1.66.0 REQUIRED COMPONENTS filesystem system thread)

file(GLOB COMMOM_SRC ./commom/*.cpp)
file(GLOB COMMOM_HDR ./commom/*.h)
source_group("comom" FILES ${COMMOM_SRC} ${COMMOM_SRC} )
#source_group("comom"headers  FILES ${COMMOM_SRC} )

message("boost include : ${Boost_INCLUDE_DIRS}")

include_directories(${Boost_INCLUDE_DIRS})
add_library(Commom STATIC ${COMMOM_SRC} ${COMMOM_HDR})
target_link_libraries(Commom
	${Boost_FILESYSTEM_LIBRARY}
	${Boost_SYSTEM_LIBRARY}
	${Boost_THREAD_LIBRARY}
)



