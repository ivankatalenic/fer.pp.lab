cmake_minimum_required(VERSION 3.25)
project(dz2 LANGUAGES CXX)

find_package(MPI REQUIRED)

add_executable(main main.cc)
set_property(TARGET main PROPERTY CXX_STANDARD 17)
target_link_libraries(main PRIVATE MPI::MPI_CXX)
