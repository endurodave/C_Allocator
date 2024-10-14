# Collect all .c files in this subdirectory
file(GLOB SUBDIR_SOURCES "*.c")

# Collect all .h files in this subdirectory
file(GLOB SUBDIR_HEADERS "*.h")

# Create a library target
add_library(AllocatorLib STATIC ${SUBDIR_SOURCES} ${SUBDIR_HEADERS})

# Include directories for the library
target_include_directories(AllocatorLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")