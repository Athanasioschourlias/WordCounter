cmake_minimum_required(VERSION 3.17)
project(it219113 C)

set(CMAKE_C_STANDARD 99)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# add the executable
add_executable(it219113 it219113.c Counter.h)

find_package(Threads REQUIRED)
target_link_libraries(it219113 PRIVATE Threads::Threads)