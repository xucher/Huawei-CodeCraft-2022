cmake_minimum_required(VERSION 3.17)
project(HWCompetition)

set(CMAKE_CXX_STANDARD 11)

add_executable(HWCompetition main.cpp io.cpp ls_construct.cpp ls_search.cpp maxflow.cpp util.cpp)
#add_executable(HWCompetition geneInstant.cpp io.cpp)