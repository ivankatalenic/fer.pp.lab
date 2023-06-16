cmake_minimum_required(VERSION 3.25)
project(dz3 LANGUAGES CXX)

find_package(OpenCL REQUIRED)

add_executable(main main.cc)
set_property(TARGET main PROPERTY CXX_STANDARD 17)
target_link_libraries(main PRIVATE OpenCL::OpenCL)

add_executable(zad1 zad1.cc)
set_property(TARGET zad1 PROPERTY CXX_STANDARD 17)
target_link_libraries(zad1 PRIVATE OpenCL::OpenCL)

add_executable(zad1_suboptimal zad1_suboptimal.cc)
set_property(TARGET zad1_suboptimal PROPERTY CXX_STANDARD 17)
target_link_libraries(zad1_suboptimal PRIVATE OpenCL::OpenCL)

add_executable(zad1_cpu zad1_cpu.cc)
set_property(TARGET zad1_cpu PROPERTY CXX_STANDARD 17)

add_executable(zad2 zad2.cc)
set_property(TARGET zad2 PROPERTY CXX_STANDARD 17)
target_link_libraries(zad2 PRIVATE OpenCL::OpenCL)

add_executable(zad2_cpu zad2_cpu.cc)
set_property(TARGET zad2_cpu PROPERTY CXX_STANDARD 17)
