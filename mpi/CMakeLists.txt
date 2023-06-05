cmake_minimum_required(VERSION 3.25)
project(TestMPI LANGUAGES CXX)

find_package(MPI REQUIRED)

add_executable(hello main.cc)
target_link_libraries(hello PRIVATE MPI::MPI_CXX)
