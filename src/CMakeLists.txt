cmake_minimum_required(VERSION 3.10)
project(tinytv CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Add the executable target directly
add_executable(tinytv
    main.cpp                    # Program entry point
    tinytv.cpp                  # TinyTV library
    debug/debug_unix.cpp        # Debug library
    files/files_base.cpp        # Files library
    files/files_unix.cpp        # Files library
    display/display_base.cpp    # Display library
    display/display_unix.cpp    # Display library
    input/input_base.cpp        # Files library
    input/input_unix.cpp        # Files library
    demuxer/demuxer.cpp         # Container demuxer audio/video library
)

target_link_libraries(tinytv PRIVATE -lSDL2)