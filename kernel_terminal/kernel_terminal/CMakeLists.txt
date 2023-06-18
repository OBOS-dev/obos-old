# CMakeList.txt : CMake project for kernel_terminal, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (kssh "kernel_terminal.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET kssh PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
