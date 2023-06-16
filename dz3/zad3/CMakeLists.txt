cmake_minimum_required(VERSION 3.25)
project(zad3 LANGUAGES C CXX)

find_package(OpenCL REQUIRED)

add_executable(cfd cfd.cpp cfdio.cpp jacobi.cpp arraymalloc.cpp boundary.cpp)
set_property(TARGET cfd PROPERTY CXX_STANDARD 17)

add_executable(cfd_opencl cfd_opencl.cpp cfdio.cpp jacobi_opencl.cpp arraymalloc.cpp boundary.cpp)
set_property(TARGET cfd_opencl PROPERTY CXX_STANDARD 17)
target_link_libraries(cfd_opencl PRIVATE OpenCL::OpenCL)
