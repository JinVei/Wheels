cmake_minimum_required(VERSION 3.0) 

project(gc)


file(GLOB TEST_SRC  ./*.cpp ./*.h )
file(GLOB GARBAGECOLLECTOR_SRC  ./../*.h  ./../*.cpp)

source_group("test" FILES ${TEST_SRC} )
source_group("garbage collector" FILES ${GARBAGECOLLECTOR_SRC} )

include_directories(./ ./../)

add_executable(gc_test ${TEST_SRC} ${GARBAGECOLLECTOR_SRC})

