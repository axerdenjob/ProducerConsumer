# CMakeList.txt : CMake project for ProducerConsumer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Add source to this project's executable.
add_executable (ProducerConsumer "ProducerConsumer.cpp" "ProducerConsumer.h")

# TODO: Add tests and install targets if needed.
target_link_libraries(ProducerConsumer Threads::Threads)
