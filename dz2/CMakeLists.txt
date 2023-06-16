cmake_minimum_required(VERSION 3.25)
project(dz2 LANGUAGES CXX)

find_package(MPI REQUIRED)

add_executable(main main.cc)
set_property(TARGET main PROPERTY CXX_STANDARD 17)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
target_compile_options(main PRIVATE /MT /EHsc /WX)
target_link_options(main PRIVATE /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)
target_link_libraries(main PRIVATE MPI::MPI_CXX)

install(TARGETS main)
install(FILES board.txt TYPE BIN)
