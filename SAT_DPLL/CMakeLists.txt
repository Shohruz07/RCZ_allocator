cmake_minimum_required(VERSION 3.10)
project(SAT_DPLL LANGUAGES CXX)

file(GLOB SAT_DPLL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)


add_executable(sat_dpll_app ${SAT_DPLL_SOURCES})


target_include_directories(sat_dpll_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)



target_link_libraries(sat_dpll_app PRIVATE Allocator)