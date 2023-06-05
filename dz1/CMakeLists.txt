cmake_minimum_required(VERSION 3.25)
project(dz1 LANGUAGES CXX)

find_package(MPI REQUIRED)

add_executable(main main.cc)
target_link_libraries(main PRIVATE MPI::MPI_CXX)
